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

#include "modelarts_manager.h"

#include <modelbox/iam_auth.h>

namespace modelartsplugin {

constexpr const char *GRAPH_PATH = "server.flow_path";
constexpr const char *JOB_NAME = "modelarts";

std::string GetGraphContentPath(
    const std::shared_ptr<modelbox::Configuration> &config) {
  std::string graph_content_path;
  auto graph_path = config->GetString(GRAPH_PATH);
  std::string extension = graph_path.substr(graph_path.find_last_of(".") + 1);
  if (extension == "json" || extension == "toml") {
    graph_content_path = graph_path;
  } else {
    std::vector<std::string> files;
    auto status =
        modelbox::ListFiles(graph_path, "*", &files, modelbox::LIST_FILES_FILE);
    if (!status) {
      MBLOG_ERROR << "ListFiles failed. path:" << graph_path
                  << ", error: " << status.WrapErrormsgs();
      return "";
    }
    if (files.size() == 0) {
      MBLOG_ERROR << "no toml file in path:" << graph_path;
      return "";
    }
    graph_content_path = files.front();
  }
  return graph_content_path;
}

bool ModelArtsManager::CreateTaskProc(
    const std::shared_ptr<modelarts::TaskInfo> &task_info) {
  auto ma_task =
      std::make_shared<MATask>(task_info, modelbox_task_manager_, ma_client_);

  auto status = ma_task->Init();
  if (!status) {
    MBLOG_ERROR << "modelarts task init failed." << status.WrapErrormsgs();
    return false;
  }

  ma_task->RegisterStatusCallback(
      [this](modelbox::OneShotTask *task, modelbox::TaskStatus status) {
        this->ModelBoxTaskStatusCallBack(task, status);
      });

  status = ma_task->Run();
  if (!status) {
    MBLOG_ERROR << "modelbox task run failed." << status.WrapErrormsgs();
    return false;
  }

  MBLOG_INFO << " modelbox task start success. modelarts taskid: "
             << task_info->GetTaskId()
             << "  modelbox taskid:" << ma_task->modelbox_task_->GetTaskId();

  {
    std::lock_guard<std::mutex> lock(task_list_map_mutex_);
    task_list_map_[task_info->GetTaskId()] = ma_task;
  }

  return true;
}

bool ModelArtsManager::DeleteTaskProc(const std::string &task_id) {
  auto ma_task = FindTaskByMaTaskId(task_id);
  if (ma_task == nullptr) {
    MBLOG_ERROR << "delete modelarts task failed, not find modelbox task "
                << task_id;
    return false;
  }

  auto status = ma_task->Stop();
  if (!status) {
    MBLOG_ERROR << "modelarts task stop failed.  modelarts taskid: " << task_id;
    return false;
  }

  ma_task->WaitTaskFinish();

  MBLOG_ERROR << "modelarts task stop success.  modelarts taskid: " << task_id;

  return true;
}

std::shared_ptr<MATask> ModelArtsManager::FindTaskByMaTaskId(
    const std::string &task_id) {
  std::lock_guard<std::mutex> lock(task_list_map_mutex_);
  auto it = task_list_map_.find(task_id);
  if (it == task_list_map_.end()) {
    return nullptr;
  }
  return it->second;
}

std::shared_ptr<MATask> ModelArtsManager::FindTaskByModelboxTaskId(
    const std::string &task_id) {
  std::lock_guard<std::mutex> lock(task_list_map_mutex_);

  for (auto it = task_list_map_.begin(); it != task_list_map_.end(); it++) {
    if (it->second->modelbox_task_ == nullptr) {
      return nullptr;
    }
    if (it->second->modelbox_task_->GetTaskId() == task_id) {
      return it->second;
    }
  }

  return nullptr;
}

void ModelArtsManager::ModelBoxTaskStatusCallBack(modelbox::OneShotTask *task,
                                                  modelbox::TaskStatus status) {
  auto ma_task = FindTaskByModelboxTaskId(task->GetTaskId());
  if (ma_task == nullptr) {
    MBLOG_INFO << "ModelBoxTaskStatusCallBack: cannot find  modelarts task , "
                  "modelbox task id: "
               << task->GetTaskId() << " status:" << status;
  }

  std::string ma_taskid = ma_task->task_info_->GetTaskId();
  MBLOG_INFO
      << "ModelBoxTaskStatusCallBack: Receive callback, modelarts taskid: "
      << ma_taskid << " modelbox taskid: " << task->GetTaskId()
      << " status:" << status;

  modelarts::TaskStatusCode status_code = modelarts::TASK_STATUS_RUNNING;
  bool finish = false;
  switch (status) {
    case modelbox::ABNORMAL:
      finish = true;
      status_code = modelarts::TASK_STATUS_FAILED;
      break;
    case modelbox::STOPPED:
      finish = true;
      status_code = modelarts::TASK_STATUS_SUCCEEDED;
      break;
    case modelbox::FINISHED:
      if (!ma_task->TaskCanFinish()) {
        ma_task->Delete();
        ma_task->Run();
        return;
      } else {
        finish = true;
        status_code = modelarts::TASK_STATUS_SUCCEEDED;
      }
      break;
    default:
      break;
  }

  if (finish) {
    ma_client_->UpdateTaskStatus(ma_task->task_info_->GetTaskId(), status_code);
    ma_task->Delete();
    ma_task->SetTaskFinishStatus(true);
    {
      std::lock_guard<std::mutex> lock(task_list_map_mutex_);
      task_list_map_.erase(ma_task->task_info_->GetTaskId());
    }
  }

  MBLOG_INFO
      << "ModelBoxTaskStatusCallBack: Receive callback end, modelarts taskid: "
      << ma_taskid << " modelbox taskid: " << task->GetTaskId();
}

modelbox::Status ModelArtsManager::Init(
    const std::shared_ptr<modelbox::Configuration> &config) {
  ma_client_ = std::make_shared<modelarts::ModelArtsClient>();
  auto status = ma_client_->Init();
  if (!status) {
    MBLOG_ERROR << "modelarts client init failed. error: "
                << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  modelbox_job_manager_ = std::make_shared<modelbox::JobManager>();
  auto graph_path = GetGraphContentPath(config);
  if (graph_path == "") {
    MBLOG_ERROR << "there is no valid graph toml. graph path:" << graph_path;
    return modelbox::STATUS_FAULT;
  }

  modelbox_job_ = modelbox_job_manager_->CreateJob(JOB_NAME, graph_path);
  if (modelbox_job_ == nullptr) {
    MBLOG_ERROR << "create job failed. graph path:" << graph_path;
    return modelbox::STATUS_FAULT;
  }

  status = modelbox_job_->Init();
  if (!status) {
    MBLOG_ERROR << "job init failed. error:" << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  auto max_task_num =
      ma_client_->config_->GetInt(modelarts::CONFIG_MAX_INPUT_COUNT, 0);
  modelbox_task_manager_ = modelbox_job_->CreateTaskManger(max_task_num);
  if (modelbox_task_manager_ == nullptr) {
    MBLOG_ERROR << "create task manager failed. graph path:" << graph_path;
    return modelbox::STATUS_FAULT;
  }

  ma_client_->RegisterTaskMsgCallBack(
      [this](const std::shared_ptr<modelarts::TaskInfo> task) -> bool {
        return this->CreateTaskProc(task);
      },
      [this](const std::string &task_id) -> bool {
        return this->DeleteTaskProc(task_id);
      });

  auto cert = modelbox::IAMAuth::GetInstance();
  status = cert->Init();
  if (!status) {
    MBLOG_ERROR << "iam auth init failed. error:" << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  auto iam_endpoint =
      ma_client_->config_->GetString(modelarts::CONFIG_ENDPOINT_IAM, "");
  if (iam_endpoint.empty()) {
    MBLOG_ERROR << "iam endpoint is null. ";
  } else {
    cert->SetIAMHostAddress(iam_endpoint);
  }

  return modelbox::STATUS_SUCCESS;
}
modelbox::Status ModelArtsManager::Start() {
  auto status = modelbox_job_->Build();
  if (!status) {
    MBLOG_ERROR << "modelbox job build failed. error:"
                << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }
  modelbox_job_->Run();

  status = modelbox_task_manager_->Start();
  if (!status) {
    MBLOG_ERROR << "modelbox task manager start failed. error:"
                << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  status = ma_client_->Start();
  if (!status) {
    MBLOG_ERROR << "modelarts client start failed. " << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status ModelArtsManager::Stop() {
  ma_client_->Stop();
  modelbox_task_manager_->Stop();
  return modelbox::STATUS_SUCCESS;
}

}  // namespace modelartsplugin