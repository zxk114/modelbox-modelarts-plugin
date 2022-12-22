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

#ifndef MODELARTS_TASK_H_
#define MODELARTS_TASK_H_
#include <string>

#include "modelarts_client.h"
#include "modelbox/base/status.h"
#include "modelbox/server/task_manager.h"

namespace modelartsplugin {

class MATask {
 public:
  MATask(std::shared_ptr<modelarts::TaskInfo> task_info,
         std::shared_ptr<modelbox::TaskManager> modelbox_task_manager,
         std::shared_ptr<modelarts::ModelArtsClient> ma_client);
  virtual ~MATask();

 public:
  modelbox::Status Init();
  modelbox::Status Run();
  modelbox::Status Stop();
  modelbox::Status Delete();
void RegisterStatusCallback(modelbox::TaskStatusCallback func);
  bool TaskCanFinish();
  void SetTaskFinishStatus(bool status);
  bool GetTaskFinishStatus() const;
  void WaitTaskFinish();

 private:
  modelbox::Status PreProcess();
  modelbox::Status FillInputData();
  modelbox::Status FillSessionConfig();
  modelbox::Status GetObsFileListByPath();
  std::string GetInputStringForActualPath();
  modelbox::Status BuildModelBoxTaskOutputInfo(std::string &output_config);
  modelbox::Status BuildModelBoxTaskInputInfo(std::string &input_config,
                                              std::string &source_type);

  modelbox::Status FillRtspInputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillCameraInputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillUrlInputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillObsInputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillVcnInputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillVisInputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillEdgeRestfulInputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillDisOutputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillObsOutputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);
  modelbox::Status FillWebhookOutputInfo(
      nlohmann::json &info_json,
      const std::shared_ptr<const modelarts::TaskIO> &io);

 public:
  std::shared_ptr<modelarts::TaskInfo> task_info_;
  std::shared_ptr<modelbox::OneShotTask> modelbox_task_;
  std::shared_ptr<modelbox::TaskManager> modelbox_task_manager_;
  std::shared_ptr<modelarts::ModelArtsClient> ma_client_;

 private:
  modelbox::TaskStatusCallback func_;
  std::string input_path_running_;
  std::vector<std::string> input_path_list_;

  std::atomic<bool> is_finish_{false};
  std::condition_variable task_finish_cv_;
  std::mutex task_finish_lock_;
};
}  // namespace modelartsplugin

#endif  // MODELARTS_TASK_H_