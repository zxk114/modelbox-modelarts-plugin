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

#ifndef MODELARTS_MANAGER_H_
#define MODELARTS_MANAGER_H_

#include "modelarts_task.h"
#include "modelbox/server/job_manager.h"
#include "modelbox/server/plugin.h"

namespace modelartsplugin {

class ModelArtsManager {
 public:
  ModelArtsManager() = default;
  ~ModelArtsManager() = default;

  modelbox::Status Init(const std::shared_ptr<modelbox::Configuration> &config);
  modelbox::Status Start();
  modelbox::Status Stop();

 private:
  bool CreateTaskProc(const std::shared_ptr<modelarts::TaskInfo> &task);

  bool DeleteTaskProc(const std::string &task_id);
  void ModelBoxTaskStatusCallBack(modelbox::OneShotTask *task,
                                  modelbox::TaskStatus status);
  std::shared_ptr<MATask> FindTaskByMaTaskId(const std::string &task_id);
  std::shared_ptr<MATask> FindTaskByModelboxTaskId(const std::string &task_id);

 private:
  std::shared_ptr<modelarts::ModelArtsClient> ma_client_;
  std::shared_ptr<modelbox::JobManager> modelbox_job_manager_;
  std::shared_ptr<modelbox::Job> modelbox_job_;
  std::shared_ptr<modelbox::TaskManager> modelbox_task_manager_;
  std::unordered_map<std::string, std::shared_ptr<MATask>> task_list_map_;
  std::mutex task_list_map_mutex_;
};

}  // namespace modelartsplugin

#endif  // MODELARTS_MANAGER_H_