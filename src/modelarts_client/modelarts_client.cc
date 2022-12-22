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

#include "modelarts_client.h"

namespace modelarts {

modelbox::Status ModelArtsClient::Init() {
  config_ = Config::GetInstance();
  if (config_ == nullptr) {
    MBLOG_ERROR << "get modelarts config failed";
    return modelbox::STATUS_FAULT;
  }

  auto key_path = config_->GetString(CONFIG_PATH_RSA, "") + "app_pri_key";
  if (access(key_path.c_str(), F_OK) != 0) {
    MBLOG_ERROR << "private key path is invalid , path:" << key_path;
    return modelbox::STATUS_FAULT;
  }
  cipher_ = std::make_shared<Cipher>();
  auto status = cipher_->Init(key_path, true);
  if (!status) {
    MBLOG_ERROR << "cipher init failed , path:" << key_path
                << " error:" << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  auto alg_type = config_->GetString(CONFIG_ALG_TYPE);
  communication_ = CommunicationFactory::Create(alg_type, config_, cipher_);
  if (communication_ == nullptr) {
    MBLOG_ERROR << "communication Create failed , alg_type:" << alg_type;
    return modelbox::STATUS_FAULT;
  }

  status = communication_->Init();
  if (!status) {
    MBLOG_ERROR << "communication init failed , alg_type:" << alg_type
                << " error:" << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  task_manager_ = std::make_shared<TaskManager>(communication_, config_);
  status = task_manager_->Init();
  if (!status) {
    MBLOG_ERROR << "take manager init failed , alg_type:"
                << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  MBLOG_INFO << "modelarts client init success.";
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status ModelArtsClient::Start() {
  auto status = communication_->Start();
  if (!status) {
    MBLOG_ERROR << "communication  start failed, error. "
                << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }
  status = task_manager_->Start();
  if (!status) {
    MBLOG_ERROR << "task manager  start failed, error. "
                << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  MBLOG_INFO << "modelarts client start success.";
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status ModelArtsClient::Stop() {
  task_manager_->Stop();
  communication_->Stop();

  MBLOG_INFO << "modelarts client stop success.";
  return modelbox::STATUS_SUCCESS;
}

void ModelArtsClient::RegisterTaskMsgCallBack(
    const CreateTaskMsgFunc &create_func,
    const DeleteTaskMsgFunc &delete_func) {
  task_manager_->SetCreateMsgFunc(create_func);
  task_manager_->SetDeleteMsgFunc(delete_func);
}
modelbox::Status ModelArtsClient::UpdateTaskStatus(
    const std::string &task_id, const TaskStatusCode &status) {
  return task_manager_->UpdateTaskStatus(task_id, status);
}

TaskStatusCode ModelArtsClient::GetTaskStatus(const std::string &task_id) {
  return task_manager_->GetTaskStatus(task_id);
}

}  // namespace modelarts