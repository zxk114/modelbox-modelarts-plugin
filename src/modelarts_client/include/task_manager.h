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

#ifndef MODELARTS_TASK_MANAGER_H_
#define MODELARTS_TASK_MANAGER_H_

#include <communication.h>
#include <config.h>
#include <securec.h>
#include <status.h>
#include <task_io.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace modelarts {

enum TaskStatusCode {
  TASK_STATUS_PENDING,
  TASK_STATUS_RUNNING,
  TASK_STATUS_SUCCEEDED,
  TASK_STATUS_FAILED,
  TASK_STATUS_BUTT
};


enum TaskErrorCode {
  TASK_ERROR_PARAMETER_INCORRECT,
  TASK_ERROR_TASK_IS_EXIST,
  TASK_ERROR_TASK_NUM_OVER_LIMIT,
  TASK_ERROR_TASK_CREATE_FAILED,
  TASK_ERROR_TASK_IS_NOT_EXIST,
  TASK_ERROR_TASK_DELETE_FAILED,
  TASK_ERROR_TASK_QUERY_FAILED,
  TASK_ERROR_BUTT
};

static std::unordered_map<TaskErrorCode, std::string> g_errode_map = {
    {TASK_ERROR_PARAMETER_INCORRECT, "The input parameter is not correct!"},
    {TASK_ERROR_TASK_IS_EXIST, "The task is already exist!"},
    {TASK_ERROR_TASK_NUM_OVER_LIMIT, "The task number is over limit!"},
    {TASK_ERROR_TASK_CREATE_FAILED, "The task create failed!"},
    {TASK_ERROR_TASK_IS_NOT_EXIST, "The task is not exist!"},
    {TASK_ERROR_TASK_DELETE_FAILED, "The task delete failed!"},
    {TASK_ERROR_TASK_QUERY_FAILED, "The task query failed!"}};

class TaskInfo {
 public:
  TaskInfo() = default;
  ~TaskInfo() = default;
  modelbox::Status Parse(const std::string &data);
  std::string GetTaskId() const { return taskid_; };
  std::string GetConfig() const { return config_; };
  std::shared_ptr<TaskIO> GetInput() const { return input_; };
  std::vector<std::shared_ptr<TaskIO>> GetOutputs() const { return outputs_; };

 private:
  std::string taskid_;
  std::string config_;
  std::shared_ptr<TaskIO> input_;
  std::vector<std::shared_ptr<TaskIO>> outputs_;
};

class TaskGroup : public std::enable_shared_from_this<TaskGroup> {
 public:
  TaskGroup(const std::shared_ptr<TaskInfo> &task_info,
            const std::string &instance_id)
      : task_info_(task_info),
        instance_id_(instance_id),
        task_status_(TASK_STATUS_PENDING){};
  virtual ~TaskGroup() = default;

  std::string GetTaskId() const { return task_info_->GetTaskId(); };
  TaskStatusCode GetTaskStatus() const { return task_status_; };
  void SetTaskStatus(const TaskStatusCode &status) { task_status_ = status; };
  std::shared_ptr<TaskInfo> GetTaskInfo() const { return task_info_; };
  std::string GetTaskDetailToString();

 private:
  std::shared_ptr<TaskInfo> task_info_;
  std::string instance_id_;
  std::atomic<TaskStatusCode> task_status_;
  std::string error_code_{TASK_ERROR_BUTT};
};

using CreateTaskMsgFunc =
    std::function<bool(const std::shared_ptr<TaskInfo> task)>;

using DeleteTaskMsgFunc = std::function<bool(const std::string &task_id)>;

class TaskManager : public std::enable_shared_from_this<TaskManager> {
 public:
  TaskManager(const std::shared_ptr<Communication> &communication,
              const std::shared_ptr<Config> &config);
  virtual ~TaskManager();
  modelbox::Status Init();
  modelbox::Status Start();
  modelbox::Status Stop();

  MAHttpStatusCode CreateTaskProcess(const std::string &msg, std::string &resp,
                                     std::shared_ptr<void> &ptr);
  MAHttpStatusCode DeleteTaskProcess(const std::string &task_id,
                                     std::string &resp,
                                     std::shared_ptr<void> &ptr);
  MAHttpStatusCode DeleteAllTaskProcess(const std::string &msg,
                                        std::string &resp,
                                        std::shared_ptr<void> &ptr);
  MAHttpStatusCode QueryTaskProcess(const std::string &msg, std::string &resp,
                                    std::shared_ptr<void> &ptr);
  void CreateTaskPostProcess(const std::string &msg, const std::string &resp,
                             std::shared_ptr<void> &ptr);
  void DeleteTaskPostProcess(const std::string &msg, const std::string &resp,
                             std::shared_ptr<void> &ptr);
  void QueryTaskPostProcess(const std::string &msg, const std::string &resp,
                            std::shared_ptr<void> &ptr);

  void SendInstanceInfoToMA();
  void SendTaskInfoToMA(std::shared_ptr<TaskGroup> task_group);
  void SetCreateMsgFunc(CreateTaskMsgFunc func);
  void SetDeleteMsgFunc(DeleteTaskMsgFunc func);
  modelbox::Status UpdateTaskStatus(const std::string &task_id,
                                    const TaskStatusCode &status);
  TaskStatusCode GetTaskStatus(const std::string &task_id);

 private:
  std::shared_ptr<TaskGroup> FindTask(const std::string &task_id);
  int GetWorkTaskCount();
  void RegisterMsgHandles();
  std::string GetInstanceInfo();
  void StartInstanceHeartBeatThread();
  int GetRunningTaskCount();
  std::shared_ptr<TaskGroup> CreateTaskGroup(const std::string &msg,
                                             MAHttpStatusCode &code,
                                             std::string &resp);

 private:
  std::string instance_id_;
  int max_task_num_{0};
  std::mutex task_group_map_mutex_;
  std::unordered_map<std::string, std::shared_ptr<TaskGroup>> task_group_map_;
  std::shared_ptr<Communication> communication_;
  std::shared_ptr<Config> config_;
  std::shared_ptr<std::thread> heatbeat_thread_;
  std::condition_variable upload_cond_;
  std::mutex upload_mutex_;
  bool stop_{false};
  bool update_{false};
  int wait_time_{5};
  CreateTaskMsgFunc create_func;
  DeleteTaskMsgFunc delete_func;
};

}  // namespace modelarts

#endif  // MODELARTS_TASK_MANAGER_H_