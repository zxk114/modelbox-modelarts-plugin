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

#include "task_io.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "log.h"
#include "utils.h"

namespace modelarts {
modelbox::Status TaskIO::Parse(const std::string &data) {
  try {
    auto j = nlohmann::json::parse(data);
    type_ = j["type"].get<std::string>();
    std::transform(type_.begin(), type_.end(), type_.begin(), ::tolower);
  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse base i/o failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }

  MBLOG_DEBUG << "parse base i/o  success.";
  return modelbox::STATUS_SUCCESS;
}

REGISTER_TASK_IO("obs", true, ObsIO);
REGISTER_TASK_IO("obs", false, ObsIO);

modelbox::Status ObsIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse obs failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/bucket");
    bucket_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/path");
    path_ = j[point].get<std::string>();
  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse obs failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse obs  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string ObsIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {{"bucket", bucket_}, {"path", path_}};
    json_data["type"] = "obs";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("obs tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

REGISTER_TASK_IO("vis", true, VisIO);

modelbox::Status VisIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse vis failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/stream_name");
    streamName_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/project_id");
    if (j.contains(point)) {
      projectId_ = j[point].get<std::string>();
    }
  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse vis failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse vis  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string VisIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {{"stream_name", streamName_},
                         {"project_id", projectId_}};
    json_data["type"] = "vis";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("vis tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

REGISTER_TASK_IO("dis", false, DisIO);

modelbox::Status DisIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse dis failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/stream_name");
    streamName_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/project_id");
    projectId_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/stream_id");
    if (j.contains(point)) {
      streamId_ = j[point].get<std::string>();
    }
  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse dis failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse dis  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string DisIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {{"stream_name", streamName_},
                         {"project_id", projectId_}};
    json_data["type"] = "dis";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("dis tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

REGISTER_TASK_IO("edgecamera", true, EdgeCameraIO);

modelbox::Status EdgeCameraIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse dis failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/id");
    streamId_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/rtsp");
    rtspStr_ = j[point].get<std::string>();

  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse edgecamera failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse edgecamera  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string EdgeCameraIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {{"id", streamId_}, {"rstp", rtspStr_}};
    json_data["type"] = "edgecamera";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("edgecamera tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

REGISTER_TASK_IO("url", true, UrlIO);

modelbox::Status UrlIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse url failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/url");
    url_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/url_type");
    if (j.contains(point)) {
      url_type_ = j[point].get<std::string>();
    } else {
      url_type_ = modelbox::RegexMatch(url_, "^rt[sm]p://.*$") ? "stream" : "file";
    }

  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse url failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse url  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string UrlIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {{"url", url_}, {"url_type", url_type_}};
    json_data["type"] = "url";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("url tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

REGISTER_TASK_IO("restful", true, EdgeRestfulIO);

modelbox::Status EdgeRestfulIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse restful failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/url");
    urlStr_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/certificate");
    certificate_ = j[point].get<bool>();

    point = nlohmann::json::json_pointer("/data/rtsp_path");
    rtspPath_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/headers");
    if (j.contains(point)) {
      headers_ = j[point].get<std::map<std::string, std::string>>();
    }

  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse restful failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse restful  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string EdgeRestfulIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {{"url", urlStr_},
                         {"url_type", rtspPath_},
                         {"certificate", certificate_},
                         {"headers", headers_}};
    json_data["type"] = "restful";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("restful tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

REGISTER_TASK_IO("vcn", true, VcnIO);

modelbox::Status VcnIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse vcn failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/stream_id");
    streamId_ = j[point].get<std::string>();

    streamType_ = 1;
    point = nlohmann::json::json_pointer("/data/stream_type");
    if (j.contains(point)) {
      streamType_ = j[point].get<uint32_t>();
    }

    point = nlohmann::json::json_pointer("/data/stream_ip");
    ip_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/stream_port");
    port_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/stream_user");
    userName_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/stream_pwd");
    password_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/protocol");
    if (j.contains(point)) {
      vcnProtocol_ = j[point].get<std::string>();
    }

  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse vcn failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse vcn  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string VcnIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {
        {"stream_id", ip_},         {"stream_type", streamType_},
        {"stream_ip", streamId_},   {"stream_port", port_},
        {"stream_user", userName_}, {"stream_pwd", password_},
        {"protocol", vcnProtocol_}};
    json_data["type"] = "vcn";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("vcn tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

REGISTER_TASK_IO("webhook", false, WebhookIO);

modelbox::Status WebhookIO::Parse(const std::string &data) {
  auto status = TaskIO::Parse(data);
  if (!status) {
    return {status, "parse webhook failed. "};
  }

  try {
    auto j = nlohmann::json::parse(data);
    auto point = nlohmann::json::json_pointer("/data/url");
    urlStr_ = j[point].get<std::string>();

    point = nlohmann::json::json_pointer("/data/headers");
    headers_ = j[point].get<std::map<std::string, std::string>>();

  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse webhook failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }
  MBLOG_DEBUG << "parse webhook  success.";
  return modelbox::STATUS_SUCCESS;
}

std::string WebhookIO::ToString() const {
  try {
    nlohmann::json json_data;
    json_data["data"] = {{"url", urlStr_}, {"headers", headers_}};
    json_data["type"] = "webhook";
    return json_data.dump();
  } catch (std::exception &e) {
    auto msg = std::string("webhook tostring failed. ") + e.what();
    MBLOG_WARN << msg;
  }
  return "";
}

IoRegister::IoRegister(const IOType &type,
                       const std::function<std::shared_ptr<TaskIO>()> &func) {
  IOFactory::GetInstance()->Register(type, func);
}

void IOFactory::Register(const IOType &type,
                         const std::function<std::shared_ptr<TaskIO>()> &func) {
  ioType_[type] = func;
}

modelbox::Status IOFactory::Parse(const std::string &data, bool isInput,
                                  std::shared_ptr<TaskIO> &io) {
  try {
    auto j = nlohmann::json::parse(data);
    auto type = j["type"].get<std::string>();
    MBLOG_INFO << "IOFactory: parse data type " << type;
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);

    auto item = ioType_.find(std::tuple<std::string, bool>{type, isInput});
    if (item == ioType_.end()) {
      auto msg = std::string("parse i/o, unsupport type: ") + type +
                 std::string(" isInput:") + std::to_string(isInput);
      return {modelbox::STATUS_FAULT, msg};
    }

    io = item->second();
    if (io == nullptr) {
      auto msg = std::string("parse i/o, unsupport type: ") + type +
                 std::string(" isInput:") + std::to_string(isInput);
      return {modelbox::STATUS_FAULT, msg};
    }

    auto status = io->Parse(data);
    if (!status) {
      auto msg = std::string("parse i/o failed, type: ") + type +
                 std::string(" isInput:") + std::to_string(isInput) +
                 std::string(" error:") + status.WrapErrormsgs();
      return {modelbox::STATUS_FAULT, msg};
    }
  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(data);
    auto msg = std::string("parse i/o failed, error: ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_FAULT, msg};
  }

  MBLOG_INFO << "parse i/o success, isInput: " << std::to_string(isInput);
  return modelbox::STATUS_SUCCESS;
}

}  // namespace modelarts