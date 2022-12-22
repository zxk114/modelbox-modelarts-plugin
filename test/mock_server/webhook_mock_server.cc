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

#include "webhook_mock_server.h"

#include <memory>

#include "modelbox/base/log.h"
#include "test_case_utils.h"

modelbox::Status WebHookMockServer::Start() {
  web::http::experimental::listener::http_listener_config listener_config;
  listener_config.set_timeout(std::chrono::seconds(1000));
  listener_ =
      std::make_shared<web::http::experimental::listener::http_listener>(
          WEBHOOK_MOCK_ENDPOINT, listener_config);
  listener_->support(
      web::http::methods::POST,
      [this](web::http::http_request request) { this->HandleFunc(request); });

  try {
    listener_->open().wait();
  } catch (std::exception const& e) {
    MBLOG_ERROR << "webhook mock server start error:" << e.what();
    return modelbox::STATUS_FAULT;
  }

  MBLOG_INFO << "webhook mock server start success.";
  return modelbox::STATUS_OK;
}

void WebHookMockServer::Stop() {
  try {
    listener_->close().wait();
  } catch (std::exception const& e) {
    MBLOG_ERROR << "webhook mock server stop error:" << e.what();
    return;
  }
};

modelbox::Status WebHookMockServer::RegisterCustomHandle(MsgHandler callback) {
  check_msg_vaild_ = callback;
  return modelbox::STATUS_OK;
}

modelbox::Status WebHookMockServer::HandleFunc(
    web::http::http_request request) {
  if (check_msg_vaild_ == nullptr ||
      check_msg_vaild_(request) != modelbox::STATUS_OK) {
    return modelbox::STATUS_FAULT;
  }
  return modelbox::STATUS_OK;
}