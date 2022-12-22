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

#include "ma_mock_server.h"

#include "modelbox/base/log.h"
#include "modelbox/base/uuid.h"
#include "test_case_utils.h"

modelbox::Status MaMockServer::Start() {
  web::http::experimental::listener::http_listener_config listener_config;
  listener_config.set_timeout(std::chrono::seconds(1000));
  listener_ =
      std::make_shared<web::http::experimental::listener::http_listener>(
          MA_MOCK_ENDPOINT, listener_config);
  listener_->support(
      web::http::methods::POST,
      [this](web::http::http_request request) { this->HandleFunc(request); });

  listener_->support(
      web::http::methods::GET,
      [this](web::http::http_request request) { this->HandleFunc(request); });

  try {
    listener_->open().wait();
  } catch (std::exception const& e) {
    MBLOG_ERROR << "ma mock server start error:" << e.what();
    return modelbox::STATUS_FAULT;
  }

  MBLOG_INFO << "ma mock server start success.";
  return modelbox::STATUS_OK;
}

void MaMockServer::Stop() {
  try {
    listener_->close().wait();
  } catch (std::exception const& e) {
    MBLOG_ERROR << "ma mock server stop error:" << e.what();
    return ;
  }
}

modelbox::Status MaMockServer::HandleFunc(web::http::http_request request) {
  if (custom_handle_ == nullptr ||
      custom_handle_(request) == modelbox::STATUS_NOTFOUND) {
    request.extract_string().then([=](utility::string_t request_body) {
      MBLOG_INFO << "ma mock server get request [" << request.method() << ", "
                 << request.request_uri().to_string() << "]";
      DefaultHandleFunc(request, request_body);
    });
  }
  return modelbox::STATUS_OK;
}

void MaMockServer::DefaultHandleFunc(web::http::http_request request,
                                     utility::string_t request_body) {
  auto method = request.method();
  auto uri = request.relative_uri().to_string();
  MBLOG_INFO << "Mock Server Recive Msg, method: " << method << " url: " << uri
             << "request_body:" << request_body;
  web::http::http_response response(web::http::status_codes::InternalError);
  if (uri == "/v2/notifications") {
    auto j = nlohmann::json::parse(request_body);
    if (j["business"] == "task") {
      response = HandleTaskMsg(request_body);
    } else if (j["business"] == "instance") {
      response = HandleInstanceMsg(request_body);
    }
  } else {
    MBLOG_ERROR << "ma mock server default handle function not found. ";
  }

  try {
    request.reply(response);
  } catch (const std::exception& e) {
    MBLOG_ERROR << "ma mock request reply faied, " << e.what();
  }
}

modelbox::Status MaMockServer::RegisterCustomHandle(RequestHandler callback) {
  if (custom_handle_ != nullptr) {
    MBLOG_ERROR << "custom_handle_ is nullptr ";
    return modelbox::STATUS_EXIST;
  }

  custom_handle_ = callback;
  return modelbox::STATUS_OK;
}

web::http::http_response MaMockServer::HandleInstanceMsg(
    const std::string& msg) {
  auto msg_json = nlohmann::json::parse(msg);
  auto instance_id = msg_json["instance_id"];
  instance_info_[instance_id] = msg_json["data"]["state"];
  std::lock_guard<std::mutex> lock(task_info_mutex_);
  task_info_.clear();
  for (auto& task : msg_json["data"]["tasks"]) {
    task_info_[task["id"]] = task["state"];
    MBLOG_INFO << "get instance " << instance_id << " state "
               << msg_json["data"]["state"] << " task" << task["id"]
               << " state " << task["state"];
  }

  web::http::http_response response;
  response.set_status_code(web::http::status_codes::Accepted);
  return response;
}

web::http::http_response MaMockServer::HandleTaskMsg(const std::string& msg) {
  auto msg_json = nlohmann::json::parse(msg);
  auto task_id = msg_json["data"]["id"];
  std::lock_guard<std::mutex> lock(task_info_mutex_);
  task_info_[task_id] = msg_json["data"]["state"];
  for (auto& task : task_info_) {
    MBLOG_INFO << "get task " << task.first << " state " << task.second;
  }

  web::http::http_response response;
  response.set_status_code(web::http::status_codes::Accepted);
  return response;
}

modelbox::Status MaMockServer::CreateTask(const std::string& msg,
                                          std::string& task_id) {
  web::json::value request_body;
  std::string task_uuid;
  try {
    if (modelbox::GetUUID(&task_uuid) != modelbox::STATUS_OK) {
      MBLOG_ERROR << "generate task uuid failed.";
      return modelbox::STATUS_FAULT;
    }

    if (GenCreateMaPluginTaskMsg(task_uuid, msg, request_body) !=
        modelbox::STATUS_OK) {
      return modelbox::STATUS_FAULT;
    }
  } catch (std::exception& e) {
    MBLOG_ERROR << "create task error: " << e.what();
    return modelbox::STATUS_FAULT;
  }

  MBLOG_INFO << "post url: " << MA_PLUGIN_CREATE_TASK_URL;

  web::http::http_request request;
  request.set_method(web::http::methods::POST);
  request.set_body(request_body);
  request.headers()["Content-Type"] = "application/json";
  request.headers()["X-Auth-Token"] = "token";

  auto response = DoRequestUrl(MA_PLUGIN_CREATE_TASK_URL, request);
  if (response.status_code() != web::http::status_codes::Created) {
    MBLOG_ERROR << "create ma task failed, httpcode:" << response.status_code()
                << " , response: " << response.extract_string().get();
    return modelbox::STATUS_FAULT;
  }

  task_id = task_uuid;
  return modelbox::STATUS_OK;
}

modelbox::Status MaMockServer::DeleteTask(const std::string& task_id) {
  web::http::http_request request;
  request.set_method(web::http::methods::DEL);
  request.headers()["Content-Type"] = "application/json";
  request.headers()["X-Auth-Token"] = "token";
  auto response =
      DoRequestUrl(MA_PLUGIN_CREATE_TASK_URL + "/" + task_id, request);

  if (response.status_code() != web::http::status_codes::Accepted) {
    MBLOG_ERROR << "delete ma task failed, httpcode:" << response.status_code()
                << " , response: " << response.extract_string().get();
    return modelbox::STATUS_FAULT;
  }
  return modelbox::STATUS_OK;
}

modelbox::Status MaMockServer::GenCreateMaPluginTaskMsg(
    const std::string& task_id, const std::string& msg,
    web::json::value& request_body) {
  try {
    request_body = web::json::value::parse(R"(
    {
      "id": ")" + task_id + R"(",
      "config":{},
      "outputs":[],
      "input":{}
    })");

    auto body = web::json::value::parse(msg);
    request_body["input"] = body["input"];
    request_body["config"] = body["config"];
    request_body["outputs"] = body["outputs"];

    MBLOG_INFO << "create ma plugin request body " << request_body.serialize();

  } catch (const std::exception& e) {
    MBLOG_ERROR << "create ma plugin request body failed" << e.what();
    return modelbox::STATUS_FAULT;
  }
  return modelbox::STATUS_OK;
}