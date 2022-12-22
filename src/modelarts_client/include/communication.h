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

#ifndef MODELARTS_COMMUNICATION_H_
#define MODELARTS_COMMUNICATION_H_

#include <cipher.h>
#include <config.h>
#include <status.h>

#include <nlohmann/json.hpp>

namespace modelarts {

constexpr const char *MA_CREATE_TYPE = "MA_CREATE_TYPE";
constexpr const char *MA_DELETE_TYPE = "MA_DELETE_TYPE";
constexpr const char *MA_QUERY_TYPE = "MA_QUERY_TYPE";
constexpr const char *MA_DELETE_ALL_TYPE = "MA_DELETE_ALL_TYPE";
constexpr const char *MA_ERROR_CODE = "error_code";
constexpr const char *MA_ERROR_MSG = "error_msg";

class Communication {
 public:
  Communication(const std::shared_ptr<Config> &config,
                const std::shared_ptr<Cipher> &cipher);
  virtual ~Communication() = default;

  using MsgHandler = std::function<MAHttpStatusCode(
      const std::string &msg, std::string &resp, std::shared_ptr<void> &ptr)>;
  using MsgPostHandler =
      std::function<void(const std::string &msg, const std::string &resp,
                         std::shared_ptr<void> &ptr)>;

  virtual modelbox::Status Init() = 0;
  virtual modelbox::Status Start() = 0;
  virtual modelbox::Status Stop() = 0;
  virtual modelbox::Status SendMsg(const std::string &msg) = 0;
  virtual modelbox::Status RegisterMsgHandle(const std::string &msgtype,
                                             MsgHandler callback,
                                             MsgPostHandler post_callback);
  virtual MsgHandler FindMsgHandle(const std::string &msgtype);
  virtual MsgPostHandler FindMsgPostHandle(const std::string &msgtype);

 public:
  std::shared_ptr<Config> config_;
  std::shared_ptr<Cipher> cipher_;

 protected:
  std::unordered_map<std::string, MsgHandler> register_msgrecive_process_;
  std::unordered_map<std::string, MsgPostHandler>
      register_msgrecive_post_process_;
};

}  // namespace modelarts

#endif  // MODELARTS_COMMUNICATION_H_
