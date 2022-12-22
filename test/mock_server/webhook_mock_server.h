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

#ifndef TEST_WEBHOOK_MOCK_SERVER_H_
#define TEST_WEBHOOK_MOCK_SERVER_H_

#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>

#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

#include "modelbox/base/status.h"
#include "test_config.h"

class WebHookMockServer {
 public:
  WebHookMockServer() = default;
  virtual ~WebHookMockServer() = default;

  modelbox::Status Start();
  void Stop();

  using MsgHandler =
      std::function<modelbox::Status(web::http::http_request &request)>;

  modelbox::Status RegisterCustomHandle(MsgHandler callback);
  modelbox::Status HandleFunc(web::http::http_request request);

 private:
  std::shared_ptr<web::http::experimental::listener::http_listener> listener_;
  MsgHandler check_msg_vaild_{nullptr};
};

#endif  // TEST_WEBHOOK_MOCK_SERVER_H_