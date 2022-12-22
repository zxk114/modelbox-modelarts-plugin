/*
 * Copyright 2022 The Modelbox Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "task_manager.h"

#include <nlohmann/json.hpp>

#include "utils.h"

namespace modelarts {

static std::unordered_map<TaskStatusCode, std::string> g_task_status_map = {
    {TASK_STATUS_PENDING, "PENDING"},
    {TASK_STATUS_RUNNING, "RUNNING"},
    {TASK_STATUS_SUCCEEDED, "SUCCEEDED"},
    {TASK_STATUS_FAILED, "FAILED"}

};

constexpr const char *ERROR_CODE_PREFIX = "ERROR.";

TaskManager::TaskManager(const std::shared_ptr<Communication> &communication,
                         const std::shared_ptr<Config> &config)
    : communication_(communication), config_(config) {}

TaskManager::~TaskManager() {
  std::lock_guard<std::mutex> lock(task_group_map_mutex_);
  task_group_map_.clear();
}

modelbox::Status TaskInfo::Parse(const std::string &data) {
  MBLOG_INFO << "TaskInfo::Parse, " << DataMasking(data);
  try {
    auto j = nlohmann::json::parse(data);

    nlohmann::json::json_pointer point("/id");
    if (j.contains(point)) {
      taskid_ = j[point].get<std::string>();
    }

    point = nlohmann::json::json_pointer("/config");
    if (j.contains(point)) {
      config_ = j[point].dump();
    }

    auto ioFactory = IOFactory::GetInstance();
    point = nlohmann::json::json_pointer("/input");
    {
      auto status = ioFactory->Parse(j[point].dump(), true, input_);
      if (!status) {
        return {status, "parse task input failed. "};
      }
    }

    MBLOG_INFO << "TaskInfo::Parse input success. ";
    point = nlohmann::json::json_pointer("/outputs");
    if (j.contains(point) && j[point].is_array()) {
      std::shared_ptr<TaskIO> output;
      for (auto ite = j[point].begin(); ite != j[point].end(); ++ite) {
        auto status = ioFactory->Parse(ite->dump(), false, output);
        if (!status) {
          return {status, "parse task output failed."};
        }
        outputs_.push_back(output);
      }
    }
    MBLOG_INFO << "TaskInfo::Parse outputs success. ";

    if (outputs_.empty()) {
      MBLOG_WARN << "TaskInfo:: Parse outputs finish, no output. ";
    }
  } catch (std::exception &e) {
    auto msg = std::string("Parse task info failed, error: ") + e.what();
    MBLOG_ERROR << msg << " data: " << DataMasking(data);
    return {modelbox::STATUS_FAULT, msg};
  }

  return modelbox::STATUS_SUCCESS;
}

static std::string GetHttpErrorMsg(TaskErrorCode error_code,
                                   MAHttpStatusCode http_status) {
  if (error_code >= TASK_ERROR_BUTT) {
    return "";
  }
  char s[5];
  int len = snprintf_s(s, sizeof(s), sizeof(s) - 1, "%03d", error_code);
  if (len < 0) {
    return "";
  }
  std::string error_str(s);
  nlohmann::json err_json = {{MA_ERROR_CODE, ERROR_CODE_PREFIX + error_str},
                             {MA_ERROR_MSG, g_errode_map[error_code]}};
  return err_json.dump();
}

void TaskManager::RegisterMsgHandles() {
  communication_->RegisterMsgHandle(
      MA_CREATE_TYPE,
      [this](const std::string &msg, std::string &resp,
             std::shared_ptr<void> &ptr) -> MAHttpStatusCode {
        return this->CreateTaskProcess(msg, resp, ptr);
      },
      [this](const std::string &msg, const std::string &resp,
             std::shared_ptr<void> &ptr) {
        return this->CreateTaskPostProcess(msg, resp, ptr);
      });

  communication_->RegisterMsgHandle(
      MA_DELETE_TYPE,
      [this](const std::string &msg, std::string &resp,
             std::shared_ptr<void> &ptr) -> MAHttpStatusCode {
        return this->DeleteTaskProcess(msg, resp, ptr);
      },
      [this](const std::string &msg, const std::string &resp,
             std::shared_ptr<void> &ptr) {
        return this->DeleteTaskPostProcess(msg, resp, ptr);
      });
  communication_->RegisterMsgHandle(
      MA_QUERY_TYPE,
      [this](const std::string &msg, std::string &resp,
             std::shared_ptr<void> &ptr) -> MAHttpStatusCode {
        return this->QueryTaskProcess(msg, resp, ptr);
      },
      [this](const std::string &msg, const std::string &resp,
             std::shared_ptr<void> &ptr) {
        return this->QueryTaskPostProcess(msg, resp, ptr);
      });

  communication_->RegisterMsgHandle(
      MA_DELETE_ALL_TYPE,
      [this](const std::string &msg, std::string &resp,
             std::shared_ptr<void> &ptr) -> MAHttpStatusCode {
        return this->DeleteAllTaskProcess(msg, resp, ptr);
      },
      [this](const std::string &msg, const std::string &resp,
             std::shared_ptr<void> &ptr) {
        return this->CreateTaskPostProcess(msg, resp, ptr);
      });
}

modelbox::Status TaskManager::Init() {
  RegisterMsgHandles();
  instance_id_ = config_->GetString(CONFIG_INSTANCE_ID, "");
  if (instance_id_ == "") {
    MBLOG_ERROR << "TaskManager init failed, instance_id is null. ";
    return modelbox::STATUS_FAULT;
  }

  max_task_num_ = config_->GetInt(CONFIG_MAX_INPUT_COUNT, 0);
  if (max_task_num_ == 0) {
    MBLOG_ERROR << "TaskManager init failed, max_task_num_ is 0. ";
    return modelbox::STATUS_FAULT;
  }
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status TaskManager::Start() {
  StartInstanceHeartBeatThread();
  return modelbox::STATUS_SUCCESS;
}

std::string TaskManager::GetInstanceInfo() {
  try {
    std::vector<nlohmann::json> tasks;
    {
      std::lock_guard<std::mutex> lock(task_group_map_mutex_);

      for (auto &item : task_group_map_) {
        std::string task_detail = item.second->GetTaskDetailToString();
        nlohmann::json task_json = nlohmann::json::parse(task_detail);
        tasks.emplace_back(task_json);
      }
    }

    nlohmann::json j = {{"business", "instance"},
                        {"instance_id", instance_id_},
                        {"data", {{"state", "RUNNING"}, {"tasks", tasks}}}};
    return j.dump();
  } catch (const std::exception &e) {
    MBLOG_ERROR << " HeartBeat: get instance info failed . " << e.what();
    return "";
  }
}

void TaskManager::StartInstanceHeartBeatThread() {
  std::unique_lock<std::mutex> lck(upload_mutex_);
  stop_ = false;
  heatbeat_thread_ = std::make_shared<std::thread>([&]() {
    while (!stop_) {
      if (communication_ != nullptr) {
        auto msg = GetInstanceInfo();
        auto status = communication_->SendMsg(msg);
        if (!status) {
          MBLOG_WARN << " HeartBeat: send instance msg failed . "
                     << status.WrapErrormsgs();
        } else {
          wait_time_ = 60;
        }
      } else {
        MBLOG_WARN << "communication is not ready.";
      }

      std::unique_lock<std::mutex> lck(upload_mutex_);
      upload_cond_.wait_for(lck, std::chrono::seconds(wait_time_),
                            [&]() { return stop_ || update_; });
      if (stop_) {
        MBLOG_INFO << " HeartBeat: thread stop . ";
        return;
      }
      update_ = false;
    }
  });

  MBLOG_INFO << " HeartBeat thread start success . ";
  return;
}

void TaskManager::SendInstanceInfoToMA() {
  std::unique_lock<std::mutex> lck(upload_mutex_);
  update_ = true;
  upload_cond_.notify_one();
  MBLOG_INFO << "notify to update instance info. ";
  return;
}

void TaskManager::SendTaskInfoToMA(std::shared_ptr<TaskGroup> task_group) {
  if (communication_ == nullptr) {
    MBLOG_WARN << "communication is not ready.";
    return;
  }

  try {
    std::string task_detail = task_group->GetTaskDetailToString();
    nlohmann::json j = {{"business", "task"},
                        {"instance_id", instance_id_},
                        {"data", nlohmann::json::parse(task_detail)}};
    auto status = communication_->SendMsg(j.dump());
    if (!status) {
      MBLOG_ERROR << "send task in to MA  failed. error: "
                  << status.WrapErrormsgs();
    }
  } catch (const std::exception &e) {
    MBLOG_ERROR << "send task in to MA  failed. error:" << e.what();
  }
  return;
}

modelbox::Status TaskManager::Stop() {
  {
    std::unique_lock<std::mutex> lck(upload_mutex_);
    stop_ = true;
    upload_cond_.notify_one();
  }
  if (heatbeat_thread_ != nullptr) {
    heatbeat_thread_->join();
  }

  MBLOG_INFO << "TaskManager stop.";
  return modelbox::STATUS_SUCCESS;
}

int TaskManager::GetRunningTaskCount() {
  int sum = 0;
  std::lock_guard<std::mutex> lock(task_group_map_mutex_);
  for (auto &it : task_group_map_) {
    auto status = it.second->GetTaskStatus();
    if (status != TASK_STATUS_SUCCEEDED && status != TASK_STATUS_FAILED) {
      sum += 1;
    }
  }
  return sum;
}

std::shared_ptr<TaskGroup> TaskManager::FindTask(const std::string &task_id) {
  std::lock_guard<std::mutex> lock(task_group_map_mutex_);
  auto it = task_group_map_.find(task_id);
  if (it == task_group_map_.end()) {
    return nullptr;
  }
  return it->second;
}

std::shared_ptr<TaskGroup> TaskManager::CreateTaskGroup(
    const std::string &msg, MAHttpStatusCode &http_code, std::string &resp) {
  auto task_info = std::make_shared<TaskInfo>();
  auto status = task_info->Parse(msg);
  if (!status) {
    MBLOG_ERROR << "prase task msg failed. error: " << status.WrapErrormsgs()
                << " body" << DataMasking(msg);
    resp = GetHttpErrorMsg(TASK_ERROR_PARAMETER_INCORRECT,
                           STATUS_HTTP_BAD_REQUEST);
    http_code = STATUS_HTTP_BAD_REQUEST;
    return nullptr;
  }

  auto task_id = task_info->GetTaskId();
  auto task_group = FindTask(task_id);
  if (task_group != nullptr) {
    MBLOG_ERROR << "task is already exist, taskid: " << task_id;
    resp = GetHttpErrorMsg(TASK_ERROR_TASK_IS_EXIST, STATUS_HTTP_BAD_REQUEST);
    http_code = STATUS_HTTP_BAD_REQUEST;
    return nullptr;
  }

  task_group = std::make_shared<TaskGroup>(task_info, instance_id_);
  return task_group;
}

MAHttpStatusCode TaskManager::CreateTaskProcess(const std::string &msg,
                                                std::string &resp,
                                                std::shared_ptr<void> &ptr) {
  MAHttpStatusCode http_code = MAHttpStatusCode::STATUS_HTTP_OK;
  auto task_group = CreateTaskGroup(msg, http_code, resp);
  if (task_group == nullptr) {
    return http_code;
  }

  auto running_task = GetRunningTaskCount();
  if (running_task + 1 > max_task_num_) {
    MBLOG_WARN << "task number over limit. max_task_num is " << max_task_num_
               << " taskid: " << task_group->GetTaskId();
  }

  auto ret = create_func(task_group->GetTaskInfo());
  if (!ret) {
    MBLOG_ERROR << "create task msg func return false. taskid: "
                << task_group->GetTaskId();
    resp = GetHttpErrorMsg(TASK_ERROR_TASK_CREATE_FAILED,
                           STATUS_HTTP_INTERNAL_ERROR);
    return STATUS_HTTP_INTERNAL_ERROR;
  }

  task_group->SetTaskStatus(TASK_STATUS_RUNNING);

  {
    std::lock_guard<std::mutex> lock(task_group_map_mutex_);
    task_group_map_[task_group->GetTaskId()] = task_group;
  }

  MBLOG_INFO << "create iva task success, taskid: " << task_group->GetTaskId();
  resp = "{}";
  ptr = task_group;
  return STATUS_HTTP_CREATED;
}

MAHttpStatusCode TaskManager::QueryTaskProcess(const std::string &msg,
                                               std::string &resp,
                                               std::shared_ptr<void> &ptr) {
  auto task_id = msg;
  auto task_group = FindTask(task_id);
  if (task_group == nullptr) {
    MBLOG_ERROR << "query task failed, task is not exist, taskid:  " << task_id;
    resp = GetHttpErrorMsg(TASK_ERROR_TASK_IS_NOT_EXIST, STATUS_HTTP_NOT_FOUND);
    return STATUS_HTTP_NOT_FOUND;
  }

  resp = task_group->GetTaskDetailToString();
  if (resp == "") {
    MBLOG_ERROR << "query task failed, taskid:  " << task_id;
    resp = GetHttpErrorMsg(TASK_ERROR_TASK_QUERY_FAILED,
                           STATUS_HTTP_INTERNAL_ERROR);
    return STATUS_HTTP_INTERNAL_ERROR;
  }

  MBLOG_ERROR << "query task success, taskid:  " << task_id;
  return STATUS_HTTP_OK;
}

MAHttpStatusCode TaskManager::DeleteTaskProcess(const std::string &task_id,
                                                std::string &resp,
                                                std::shared_ptr<void> &ptr) {
  auto task_group = FindTask(task_id);
  if (task_group == nullptr) {
    MBLOG_ERROR << "delete task failed, task is not exist, taskid:  "
                << task_id;
    resp = GetHttpErrorMsg(TASK_ERROR_TASK_IS_NOT_EXIST, STATUS_HTTP_NOT_FOUND);
    return STATUS_HTTP_NOT_FOUND;
  }

  auto task_status = task_group->GetTaskStatus();
  if (task_status == TASK_STATUS_RUNNING) {
    auto ret = delete_func(task_id);
    if (!ret) {
      MBLOG_ERROR << "delete task msg func return false. taskid: " << task_id;
      resp = GetHttpErrorMsg(TASK_ERROR_TASK_DELETE_FAILED,
                             STATUS_HTTP_INTERNAL_ERROR);
      return STATUS_HTTP_INTERNAL_ERROR;
    }
  }

  MBLOG_INFO << "delete iva task success, taskid: " << task_group->GetTaskId();
  resp = "{}";
  ptr = task_group;
  return STATUS_HTTP_ACCEPTED;
}

MAHttpStatusCode TaskManager::DeleteAllTaskProcess(const std::string &msg,
                                                   std::string &resp,
                                                   std::shared_ptr<void> &ptr) {
  std::vector<std::string> taskIds;
  {
    std::lock_guard<std::mutex> lock(task_group_map_mutex_);
    for (auto &task : task_group_map_) {
      taskIds.push_back(task.second->GetTaskId());
    }
  }

  for (auto &taskid : taskIds) {
    std::string tmp_resp;
    if (DeleteAllTaskProcess(taskid, tmp_resp, ptr) != STATUS_HTTP_ACCEPTED) {
      MBLOG_INFO << "failed to  DeleteAllTaskProcess, taskid: " << taskid
                 << " resp" << tmp_resp;
    }
  }

  return STATUS_HTTP_ACCEPTED;
}

void TaskManager::CreateTaskPostProcess(const std::string &msg,
                                        const std::string &resp,
                                        std::shared_ptr<void> &ptr) {
  if (ptr != nullptr) {
    std::shared_ptr<TaskGroup> task_group =
        std::static_pointer_cast<TaskGroup>(ptr);
    SendTaskInfoToMA(task_group);
  }
  SendInstanceInfoToMA();
  return;
}
void TaskManager::DeleteTaskPostProcess(const std::string &msg,
                                        const std::string &resp,
                                        std::shared_ptr<void> &ptr) {
  SendInstanceInfoToMA();
  return;
}

void TaskManager::QueryTaskPostProcess(const std::string &msg,
                                       const std::string &resp,
                                       std::shared_ptr<void> &ptr) {
  return;
}

void TaskManager::SetCreateMsgFunc(CreateTaskMsgFunc func) {
  if (func != nullptr) {
    create_func = func;
  }
}

void TaskManager::SetDeleteMsgFunc(DeleteTaskMsgFunc func) {
  if (func != nullptr) {
    delete_func = func;
  }
}

modelbox::Status TaskManager::UpdateTaskStatus(const std::string &task_id,
                                               const TaskStatusCode &status) {
  auto task_group = FindTask(task_id);
  if (task_group == nullptr) {
    MBLOG_ERROR << "update task status failed, this task is not exist, taskid: "
                << task_id;
    return modelbox::STATUS_FAULT;
  }

  auto old_status = task_group->GetTaskStatus();

  if (old_status == status) {
    return modelbox::STATUS_SUCCESS;
  }

  task_group->SetTaskStatus(status);

  SendTaskInfoToMA(task_group);

  if (status == TASK_STATUS_SUCCEEDED || status == TASK_STATUS_FAILED) {
    std::lock_guard<std::mutex> lock(task_group_map_mutex_);
    if (task_group_map_.find(task_group->GetTaskId()) !=
        task_group_map_.end()) {
      task_group_map_.erase(task_group->GetTaskId());
    }
  }

  SendInstanceInfoToMA();

  return modelbox::STATUS_SUCCESS;
}

TaskStatusCode TaskManager::GetTaskStatus(const std::string &task_id) {
  auto task_group = FindTask(task_id);
  if (task_group == nullptr) {
    MBLOG_ERROR << "get task status failed, this task is not exist, taskid: "
                << task_id;
    return TASK_STATUS_BUTT;
  }
  return task_group->GetTaskStatus();
}

std::string TaskGroup::GetTaskDetailToString() {
  auto status_code = GetTaskStatus();
  if (status_code >= TASK_STATUS_BUTT) {
    return "";
  }
  try {
    nlohmann::json j = {{"id", GetTaskId()},
                        {"state", g_task_status_map[status_code]}};
    return j.dump();
  } catch (const std::exception &e) {
    MBLOG_ERROR << "get task info string failed, error: " << e.what();
    return "";
  }
}

}  // namespace modelarts
