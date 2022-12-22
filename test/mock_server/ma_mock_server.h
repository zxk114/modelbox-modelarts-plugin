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

#ifndef TEST_MA_MOCK_SERVER_H_

#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>

#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

#include "modelbox/base/log.h"
#include "modelbox/base/status.h"
#include "test_config.h"

class MaMockServer {
 public:
  MaMockServer() = default;
  virtual ~MaMockServer() = default;

  using RequestHandler =
      std::function<modelbox::Status(web::http::http_request &request)>;

  modelbox::Status Start();
  void Stop();

  modelbox::Status CreateTask(const std::string &msg, std::string &task_id);

  modelbox::Status DeleteTask(const std::string &task_id);

  modelbox::Status RegisterCustomHandle(RequestHandler callback);

  std::string GetInstanceState(const std::string &instance_id) {
    return instance_info_.find(instance_id) == instance_info_.end()
               ? "NOT_FOUND"
               : instance_info_.find(instance_id)->second;
  };

  std::string GetTaskState(const std::string task_id) {
    std::lock_guard<std::mutex> lock(task_info_mutex_);
    return task_info_.find(task_id) == task_info_.end()
               ? "NOT_FOUND"
               : task_info_.find(task_id)->second;
  }

 private:
  modelbox::Status HandleFunc(web::http::http_request request);
  void DefaultHandleFunc(web::http::http_request request,
                         utility::string_t request_body);
  web::http::http_response HandleInstanceMsg(const std::string &msg);
  web::http::http_response HandleTaskMsg(const std::string &msg);

  modelbox::Status GenCreateMaPluginTaskMsg(const std::string &task_id,
                                            const std::string &msg,
                                            web::json::value &request_body);

  std::unordered_map<std::string, std::string> instance_info_;
  std::unordered_map<std::string, std::string> task_info_;
  std::mutex task_info_mutex_;
  RequestHandler custom_handle_{nullptr};
  std::shared_ptr<web::http::experimental::listener::http_listener> listener_;
};

#define TEST_MA_MOCK_SERVER_H_

#endif  // TEST_MA_MOCK_SERVER_H_