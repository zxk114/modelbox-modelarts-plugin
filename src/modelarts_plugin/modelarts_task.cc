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

#include "modelarts_task.h"

#include <modelbox/base/log.h>
#include <modelbox/obs_client.h>
#include <utils.h>

namespace modelartsplugin {

MATask::MATask(std::shared_ptr<modelarts::TaskInfo> task_info,
               std::shared_ptr<modelbox::TaskManager> modelbox_task_manager,
               std::shared_ptr<modelarts::ModelArtsClient> ma_client)
    : task_info_(task_info),
      modelbox_task_manager_(modelbox_task_manager),
      ma_client_(ma_client){};

MATask::~MATask() = default;

modelbox::Status MATask::GetObsFileListByPath() {
  auto obs_client = modelbox::ObsClient::GetInstance();
  modelbox::ObsOptions opt;
  auto obs =
      std::dynamic_pointer_cast<const modelarts::ObsIO>(task_info_->GetInput());
  opt.bucket = obs->GetBucket();
  opt.path = obs->GetPath();
  opt.end_point =
      ma_client_->config_->GetString(modelarts::CONFIG_ENDPOINT_OBS);
  auto status = obs_client->GetObjectsList(opt, input_path_list_);
  if (!status) {
    MBLOG_ERROR << " get obs objects list failed, error: " << status
                << " config:" << obs->ToString();
    return modelbox::STATUS_FAULT;
  }

  if (input_path_list_.empty()) {
    MBLOG_ERROR << " there is no obs file, config:" << obs->ToString();
    return modelbox::STATUS_FAULT;
  }
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::Init() {
  auto input_type = task_info_->GetInput()->GetType();
  if (input_type == "obs") {
    auto obs = std::dynamic_pointer_cast<const modelarts::ObsIO>(
        task_info_->GetInput());
    auto input_path = obs->GetPath();
    if (input_path.rfind("/") == input_path.length() - 1) {
      return GetObsFileListByPath();
    }
  }

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::Stop() {
  MBLOG_INFO << "modelbox task begin stop . modelarts taskid: "
             << task_info_->GetTaskId()
             << "  modelbox taskid:" << modelbox_task_->GetTaskId();

  auto status = modelbox_task_->Stop();
  if (!status) {
    MBLOG_ERROR << "modelbox task stop failed.  modelarts taskid: "
                << task_info_->GetTaskId()
                << "  modelbox taskid:" << modelbox_task_->GetTaskId()
                << " error: " << status.WrapErrormsgs();
    return modelbox::STATUS_FAULT;
  }

  MBLOG_INFO << "modelbox task stop success. modelarts taskid: "
             << task_info_->GetTaskId()
             << "  modelbox taskid:" << modelbox_task_->GetTaskId();

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::Delete() {
  return modelbox_task_manager_->DeleteTaskById(modelbox_task_->GetTaskId());
}

modelbox::Status MATask::Run() {
  auto task = modelbox_task_manager_->CreateTask(modelbox::TASK_ONESHOT);
  modelbox_task_ = std::dynamic_pointer_cast<modelbox::OneShotTask>(task);
  if (modelbox_task_ == nullptr) {
    return {modelbox::STATUS_FAULT, "modelbox task create failed."};
  }

  auto status = PreProcess();
  if (!status) {
    return {modelbox::STATUS_FAULT,
            "modelbox task preprocess failed. " + status.WrapErrormsgs()};
  }

  if (func_ == nullptr) {
    return {modelbox::STATUS_FAULT,
            "modelbox task status callback not be set, please set it first. "};
  }
  modelbox_task_->RegisterStatusCallback(func_);

  status = modelbox_task_->Start();
  if (!status) {
    return {modelbox::STATUS_FAULT,
            "modelbox task create failed. " + status.WrapErrormsgs()};
  }

  MBLOG_INFO << " modelarts task run success, modelarts taskid:  "
             << task_info_->GetTaskId()
             << " modelbox taskid::" << modelbox_task_->GetTaskId();
  return modelbox::STATUS_SUCCESS;
}

void MATask::RegisterStatusCallback(
    modelbox::TaskStatusCallback func) {
  func_ = func;
}

modelbox::Status MATask::FillObsInputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto obs = std::dynamic_pointer_cast<const modelarts::ObsIO>(io);
  info_json["obsEndPoint"] =
      ma_client_->config_->GetString(modelarts::CONFIG_ENDPOINT_OBS);
  info_json["bucket"] = obs->GetBucket();
  if (input_path_list_.empty()) {
    info_json["path"] = obs->GetPath();
  } else {
    info_json["path"] = input_path_list_.back();
    input_path_running_ = info_json["path"];
    input_path_list_.pop_back();
  }

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillCameraInputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto camera = std::dynamic_pointer_cast<const modelarts::EdgeCameraIO>(io);
  info_json["url"] = camera->GetRtspStr();
  info_json["url_type"] = "stream";
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillUrlInputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto url = std::dynamic_pointer_cast<const modelarts::UrlIO>(io);
  info_json["url"] = url->GetUrl();
  info_json["url_type"] = url->GetUrlType();
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillVcnInputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto vcn = std::dynamic_pointer_cast<const modelarts::VcnIO>(io);

  info_json["ip"] = vcn->GetIP();
  info_json["port"] = vcn->GetPort();
  info_json["userName"] = vcn->GetUserName();

  auto encrypted_pwd = vcn->GetPassword();
  std::string pwd;
  auto status = ma_client_->cipher_->DecryptFromBase64(encrypted_pwd, pwd);
  if (!status) {
    MBLOG_ERROR << "FillVcnInputInfo, DecryptFromBase64 failed. error: "
                << status.WrapErrormsgs();
    pwd = encrypted_pwd;
  }
  info_json["password"] = pwd;
  info_json["cameraCode"] = vcn->GetStreamId();
  info_json["streamType"] = vcn->GetStreamType();

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillVisInputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto vis = std::dynamic_pointer_cast<const modelarts::VisIO>(io);
  info_json["visEndPoint"] =
      ma_client_->config_->GetString(modelarts::CONFIG_ENDPOINT_VIS);
  info_json["streamName"] = vis->GetStreamName();
  info_json["projectId"] = vis->GetProjectId();
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillEdgeRestfulInputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto restful = std::dynamic_pointer_cast<const modelarts::EdgeRestfulIO>(io);
  info_json["request_url"] = restful->GetUrlStr();
  info_json["response_url_position"] = restful->GetRtspPath();
  if (!restful->GetHeaders().empty()) {
    nlohmann::json header_json;
    for (auto &it : restful->GetHeaders()) {
      header_json[it.first] = it.second;
    }
    info_json["headers"] = header_json;
  }

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillDisOutputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto dis = std::dynamic_pointer_cast<const modelarts::DisIO>(io);
  info_json["type"] = "dis";
  info_json["name"] = "dis";
  nlohmann::json config_json;
  config_json["disEndPoint"] =
      ma_client_->config_->GetString(modelarts::CONFIG_ENDPOINT_DIS);
  config_json["region"] =
      ma_client_->config_->GetString(modelarts::CONFIG_REGION);
  config_json["steamName"] = dis->GetStreamName();
  config_json["projectId"] = dis->GetProjectId();
  info_json["cfg"] = config_json.dump();
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillObsOutputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto obs = std::dynamic_pointer_cast<const modelarts::ObsIO>(io);
  info_json["type"] = "obs";
  info_json["name"] = "obs";
  nlohmann::json config_json;
  config_json["obsEndPoint"] =
      ma_client_->config_->GetString(modelarts::CONFIG_ENDPOINT_OBS);
  config_json["bucket"] = obs->GetBucket();
  config_json["path"] = obs->GetPath();
  info_json["cfg"] = config_json.dump();
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillWebhookOutputInfo(
    nlohmann::json &info_json,
    const std::shared_ptr<const modelarts::TaskIO> &io) {
  auto webhook = std::dynamic_pointer_cast<const modelarts::WebhookIO>(io);
  info_json["type"] = "webhook";
  info_json["name"] = "webhook";
  nlohmann::json config_json;
  config_json["url"] = webhook->GetUrlStr();
  if (!webhook->GetHeaders().empty()) {
    nlohmann::json header_json;
    for (auto &it : webhook->GetHeaders()) {
      header_json[it.first] = it.second;
    }
    config_json["headers"] = header_json;
  }

  info_json["cfg"] = config_json.dump();
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::PreProcess() {
  auto status = FillInputData();
  if (!status) {
    MBLOG_ERROR << "fill input data for modelbox task failed.";
    return status;
  }

  status = FillSessionConfig();
  if (!status) {
    MBLOG_ERROR << "fill session data for modelbox task failed.";
    return status;
  }

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::BuildModelBoxTaskInputInfo(std::string &input_config,
                                                    std::string &source_type) {
  auto input_type = task_info_->GetInput()->GetType();
  modelbox::Status status;
  nlohmann::json info_json;
  if (input_type == "obs") {
    source_type = "obs";
    status = FillObsInputInfo(info_json, task_info_->GetInput());
  } else if (input_type == "vis") {
    source_type = "vis";
    status = FillVisInputInfo(info_json, task_info_->GetInput());
  } else if (input_type == "vcn") {
    auto vcn = std::dynamic_pointer_cast<const modelarts::VcnIO>(task_info_->GetInput());
    source_type = (vcn->GetVcnProtocol() == modelarts::VCN_PROOCOL_SDK)
                      ? "vcn"
                      : "vcn_restful";
    status = FillVcnInputInfo(info_json, task_info_->GetInput());
  } else if (input_type == "edgecamera") {
    source_type = "url";
    status = FillCameraInputInfo(info_json, task_info_->GetInput());
  } else if (input_type == "url") {
    source_type = "url";
    status = FillUrlInputInfo(info_json, task_info_->GetInput());
  } else if (input_type == "restful") {
    source_type = "restful";
    status = FillEdgeRestfulInputInfo(info_json, task_info_->GetInput());
  } else {
    MBLOG_WARN << "input type is not support, type: " << input_type;
    return modelbox::STATUS_FAULT;
  }

  input_config = info_json.dump();
  MBLOG_INFO << "input type: " << source_type
             << ", config:" << modelarts::DataMasking(input_config);
  return status;
}

modelbox::Status MATask::BuildModelBoxTaskOutputInfo(
    std::string &output_config) {
  modelbox::Status status;
  nlohmann::json output_json = nlohmann::json::array();
  for (auto output : task_info_->GetOutputs()) {
    nlohmann::json info_json;
    auto output_type = output->GetType();
    if (output_type == "obs") {
      status = FillObsOutputInfo(info_json, output);
    } else if (output_type == "dis") {
      status = FillDisOutputInfo(info_json, output);
    } else if (output_type == "webhook") {
      status = FillWebhookOutputInfo(info_json, output);
    } else {
      MBLOG_WARN << "output type is not support, type: " << output_type;
      return modelbox::STATUS_FAULT;
    }

    if (!status) {
      MBLOG_ERROR << "build modelbox task output failed. type: " << output_type
                  << " error:" << status.WrapErrormsgs();
      continue;
    }
    output_json.push_back(info_json);
  }
  nlohmann::json output_config_str;
  output_config_str["brokers"] = output_json;
  output_config = output_config_str.dump();
  MBLOG_INFO << "output config:" << modelarts::DataMasking(output_config);
  return status;
}

std::string MATask::GetInputStringForActualPath() {
  auto input_type = task_info_->GetInput()->GetType();
  if (input_type == "obs" && input_path_running_ != "") {
    nlohmann::json json_data;
    auto obs = std::dynamic_pointer_cast<const modelarts::ObsIO>(
        task_info_->GetInput());
    json_data["data"] = {{"bucket", obs->GetBucket()},
                         {"path", input_path_running_}};
    json_data["type"] = "obs";
    return json_data.dump();
  } else {
    return task_info_->GetInput()->ToString();
  }
}

modelbox::Status MATask::FillSessionConfig() {
  std::string output_config;
  auto status = BuildModelBoxTaskOutputInfo(output_config);
  if (!status) {
    MBLOG_ERROR << "build modelbox task output failed.";
    return status;
  }

  auto config = modelbox_task_->GetSessionConfig();
  config->SetProperty("flowunit.output_broker.config", output_config);
  auto input_str = GetInputStringForActualPath();
  config->SetProperty("nodes.modelarts_task_input", input_str);
  MBLOG_INFO << "modelbox task input: " << input_str;

  std::vector<std::string> outputs;
  for (auto &it : task_info_->GetOutputs()) {
    outputs.push_back(it->ToString());
    MBLOG_INFO << "modelbox task output: " << it->ToString();
  }

  config->SetProperty("nodes.modelarts_task_output", outputs);
  config->SetProperty("nodes.modelarts_task_config", task_info_->GetConfig());
  config->SetProperty("nodes.modelarts_task_id", task_info_->GetTaskId());

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status MATask::FillInputData() {
  std::string input_cfg;
  std::string source_type;
  std::string input_name = "input1";
  auto status = BuildModelBoxTaskInputInfo(input_cfg, source_type);
  if (!status) {
    MBLOG_ERROR << "build modelbox task input failed.";
    return status;
  }

  auto buff_list = modelbox_task_->CreateBufferList();
  status = buff_list->Build({input_cfg.size()});
  if (!status) {
    MBLOG_ERROR << "input buffer build failed.";
    return status;
  }

  auto buff = buff_list->At(0);
  if (buff == nullptr) {
    MBLOG_ERROR << "buffer is null.";
    return modelbox::STATUS_FAULT;
  }

  auto buffer_ptr = buff->MutableData();
  if (buffer_ptr == nullptr) {
    MBLOG_ERROR << "buffer_ptr is null.";
    return modelbox::STATUS_FAULT;
  }

  auto ret = memcpy_s(buffer_ptr, buff->GetBytes(), input_cfg.data(),
                      input_cfg.size());
  if (ret > 0) {
    MBLOG_ERROR << "memcpy failed. dest size:" << buff->GetBytes()
                << " src size:" << input_cfg.size();
    return modelbox::STATUS_FAULT;
  }

  buff->Set("source_type", source_type);
  std::unordered_map<std::string, std::shared_ptr<modelbox::BufferList>> datas;
  datas.emplace(input_name, buff_list);
  status = modelbox_task_->FillData(datas);
  if (!status) {
    MBLOG_ERROR << "modelbox task fill data failed." << status.WrapErrormsgs();
    return status;
  }

  return modelbox::STATUS_SUCCESS;
}

void MATask::SetTaskFinishStatus(bool status) {
  std::lock_guard<std::mutex> lock(task_finish_lock_);
  is_finish_ = status;
  task_finish_cv_.notify_all();
}

bool MATask::GetTaskFinishStatus() const { return is_finish_; }

void MATask::WaitTaskFinish() {
  std::unique_lock<std::mutex> lock(task_finish_lock_);
  task_finish_cv_.wait(lock, [&]() { return GetTaskFinishStatus(); });
}

bool MATask::TaskCanFinish() { return input_path_list_.empty(); }

}  // namespace modelartsplugin