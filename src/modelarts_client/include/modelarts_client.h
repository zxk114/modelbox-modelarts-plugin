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

#ifndef MODELARTS_CLIENT_H_
#define MODELARTS_CLIENT_H_

#include <cipher.h>
#include <communication_factory.h>
#include <config.h>
#include <log.h>
#include <task_manager.h>

#include <string>

namespace modelarts {

class ModelArtsClient {
 public:
  ModelArtsClient() = default;
  ~ModelArtsClient() = default;

  modelbox::Status Init();
  modelbox::Status Start();
  modelbox::Status Stop();
  void RegisterTaskMsgCallBack(const CreateTaskMsgFunc &create_func,
                               const DeleteTaskMsgFunc &delete_func);
  modelbox::Status UpdateTaskStatus(const std::string &task_id,
                                    const TaskStatusCode &status);
  TaskStatusCode GetTaskStatus(const std::string &task_id);

 public:
  std::shared_ptr<Config> config_;
  std::shared_ptr<Cipher> cipher_;
  std::shared_ptr<Communication> communication_;
  std::shared_ptr<TaskManager> task_manager_;
};
}  // namespace modelarts

#endif  // MODELARTS_CLIENT_H_