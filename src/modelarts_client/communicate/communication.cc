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

#include "communication.h"

namespace modelarts {

Communication::Communication(const std::shared_ptr<Config> &config,
                             const std::shared_ptr<Cipher> &cipher)
    : config_(config), cipher_(cipher) {}

modelbox::Status Communication::RegisterMsgHandle(
    const std::string &msgtype, MsgHandler callback,
    MsgPostHandler post_callback) {
  register_msgrecive_process_[msgtype] = callback;
  register_msgrecive_post_process_[msgtype] = post_callback;
  MBLOG_INFO << "register callback. msgType: " << msgtype;
  return modelbox::STATUS_SUCCESS;
}

Communication::MsgHandler Communication::FindMsgHandle(
    const std::string &msgtype) {
  auto callback = register_msgrecive_process_.find(msgtype);
  if (callback == register_msgrecive_process_.end()) {
    return nullptr;
  }
  return callback->second;
}

Communication::MsgPostHandler Communication::FindMsgPostHandle(
    const std::string &msgtype) {
  auto callback = register_msgrecive_post_process_.find(msgtype);
  if (callback == register_msgrecive_post_process_.end()) {
    return nullptr;
  }
  return callback->second;
}

}  // namespace modelarts