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

#ifndef MODELARTS_RESTFUL_COMMUNICATION_H_
#define MODELARTS_RESTFUL_COMMUNICATION_H_

#include <map>

#include "communication.h"
#include "modelbox/server/http_helper.h"

namespace modelarts {

const std::map<MAHttpStatusCode, modelbox::HttpStatusCode> http_status_map_ = {
    {STATUS_HTTP_OK, modelbox::HttpStatusCodes::OK},
    {STATUS_HTTP_CREATED, modelbox::HttpStatusCodes::CREATED},
    {STATUS_HTTP_ACCEPTED, modelbox::HttpStatusCodes::ACCEPTED},
    {STATUS_HTTP_NO_CONTENT, modelbox::HttpStatusCodes::NO_CONTENT},
    {STATUS_HTTP_BAD_REQUEST, modelbox::HttpStatusCodes::BAD_REQUEST},
    {STATUS_HTTP_NOT_FOUND, modelbox::HttpStatusCodes::NOT_FOUND},
    {STATUS_HTTP_INTERNAL_ERROR, modelbox::HttpStatusCodes::INTERNAL_ERROR}};

class RestfulCommunication : public Communication {
 public:
  RestfulCommunication(const std::shared_ptr<Config> &config,
                       const std::shared_ptr<Cipher> &cipher);
  ~RestfulCommunication() = default;

  modelbox::Status Init() override;
  modelbox::Status Start() override;
  modelbox::Status Stop() override;
  modelbox::Status SendMsg(const std::string &msg) override;

 private:
  modelbox::Status SetupSSLServerConfig(
      const std::string &cert, const std::string &key,
      modelbox::HttpServerConfig &server_config);
  void MsgProcess(const httplib::Request &request, httplib::Response &response);

  bool CheckUrlVaild(const std::string &request_url,
                     httplib::Response &response);

  std::string ParseTaskId(const std::string &uri);

  modelbox::HttpStatusCode StatusToHttpStatus(MAHttpStatusCode status) const;

  std::string GetMsgType(const std::string &method);
  std::string GetMsgRequestInfo(const httplib::Request &request);

  modelbox::Status GetStringByPath(const std::string &path,
                                   std::string &output_str);

  modelbox::Status GetSignerUrlInfo(std::string &host, std::string &uri);

  modelbox::Status GetAkSk(std::string &ak, std::string &sk);
  std::string FilterHttpPrefix(const std::string &url);

 private:
  std::shared_ptr<modelbox::HttpServer> server_;
  std::mutex msg_concurrency_mutex_;
};

}  // namespace modelarts

#endif  // MODELARTS_RESTFUL_COMMUNICATION_H_
