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

#include "communication_factory.h"

namespace modelarts {
std::unordered_map<std::string, CreateCommunicationFunc>
    CommunicationFactory::create_communication_map_;

std::shared_ptr<Communication> CommunicationFactory::Create(
    const std::string &alg_type, const std::shared_ptr<Config> &config,
    const std::shared_ptr<Cipher> &cipher) {
  auto item = create_communication_map_.find(alg_type);
  if (item == create_communication_map_.end()) {
    MBLOG_ERROR << "can not match alg type: " << alg_type;
    return nullptr;
  }
  return item->second(config, cipher);
}

void CommunicationFactory::Regist(const std::string &alg_type,
                                  const CreateCommunicationFunc &create_func) {
  create_communication_map_[alg_type] = create_func;
}

}  // namespace modelarts