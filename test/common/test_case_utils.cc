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

#include "test_case_utils.h"

#include <cpprest/http_client.h>

#include "modelbox/base/log.h"
#include "modelbox/base/uuid.h"
#include "test_config.h"

web::http::http_response DoRequestUrl(const std::string &url,
                                      web::http::http_request request) {
  web::http::http_response rsp;
  try {
    web::http::client::http_client_config client_config;
    client_config.set_timeout(utility::seconds(100));
    client_config.set_ssl_context_callback([&](boost::asio::ssl::context &ctx) {
      ctx.load_verify_file(std::string(TEST_CIPHER_DIR) +
                           "/cert/https_cert.pem");
    });
    client_config.set_validate_certificates(false);
    web::http::client::http_client client(web::http::uri_builder(url).to_uri(),
                                          client_config);
    rsp = client.request(request).get();
  } catch (const std::exception &e) {
    MBLOG_ERROR << "DoRequestUrl failed, error " << e.what();
  }
  return rsp;
}

web::json::value GenCreateTaskMsg() {
  auto create_body = web::json::value::parse(R"(
   {
    "name": "task",
    "config": {
    },
    "input":{},
    "outputs":[{}]
   })");
  return create_body;
}