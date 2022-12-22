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

#ifndef MODELARTS_COMMUNICATION_FACTORY_H_
#define MODELARTS_COMMUNICATION_FACTORY_H_

#include <communication.h>
#include <config.h>

namespace modelarts {
using CreateCommunicationFunc = std::function<std::shared_ptr<Communication>(
    std::shared_ptr<Config>, std::shared_ptr<Cipher>)>;

class CommunicationFactory {
  CommunicationFactory() = default;
  ~CommunicationFactory() = default;

 public:
  static std::shared_ptr<Communication> Create(
      const std::string &alg_type, const std::shared_ptr<Config> &config,
      const std::shared_ptr<Cipher> &cipher);

  static void Regist(const std::string &alg_type,
                     const CreateCommunicationFunc &create_func);

 private:
  static std::unordered_map<std::string, CreateCommunicationFunc>
      create_communication_map_;
};

#define REGISTER_COMMUNICATE(alg_type, clazz)                                \
  __attribute__((unused)) static auto g_##clazz##_register = []() {          \
    CommunicationFactory::Regist(                                            \
        alg_type,                                                            \
        [](std::shared_ptr<Config> config, std::shared_ptr<Cipher> cipher) { \
          return std::make_shared<clazz>(config, cipher);                    \
        });                                                                  \
    return 0;                                                                \
  }();

}  // namespace modelarts

#endif  // MODELARTS_COMMUNICATION_FACTORY_H_