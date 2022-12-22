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

#ifndef MODELARTS_CONFIG_H_
#define MODELARTS_CONFIG_H_

#include <log.h>
#include <stdlib.h>

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

#include "modelbox/base/configuration.h"

namespace modelarts {

constexpr const char *CONFIG_INSTANCE_ID = "alg.instanceid";
constexpr const char *CONFIG_ALG_TYPE = "alg.type";
constexpr const char *CONFIG_MAX_INPUT_COUNT = "alg.maxInputCount";
constexpr const char *CONFIG_TASK_URI = "alg.task.uri";
constexpr const char *CONFIG_TASK_PORT = "alg.task.port";
constexpr const char *CONFIG_NOTIFY_URL = "alg.notify.url";
constexpr const char *CONFIG_DEVELOPER_PROJECTID = "developer.projectid";
constexpr const char *CONFIG_DEVELOPER_DOMAIN_NAME = "developer.domain_name";
constexpr const char *CONFIG_DEVELOPER_DOAMIN_ID = "developer.domain_id";
constexpr const char *CONFIG_DEVELOPER_AK = "developer.ak";
constexpr const char *CONFIG_DEVELOPER_SK = "developer.sk";
constexpr const char *CONFIG_REGION = "region";
constexpr const char *CONFIG_ENDPOINT_IAM = "endpoint.iam";
constexpr const char *CONFIG_ENDPOINT_MA_INFER = "endpoint.ma_infer";
constexpr const char *CONFIG_ENDPOINT_OBS = "endpoint.obs";
constexpr const char *CONFIG_ENDPOINT_DIS = "endpoint.dis";
constexpr const char *CONFIG_ENDPOINT_VIS = "endpoint.vis";
constexpr const char *CONFIG_PATH_RSA = "path.rsa";
constexpr const char *CONFIG_PATH_CERT = "path.cert";
constexpr const char *CONFIG_TOPIC_UPSTREAM = "topic.upstream";
constexpr const char *CONFIG_TOPIC_DOWNSTREAM = "topic.downstream";

class Config {
 public:
  Config();
  ~Config();

  static std::shared_ptr<Config> &GetInstance() {
    static std::once_flag oc;
    static auto config = std::make_shared<Config>();
    std::call_once(oc, [&]() {
      auto ret = config->LoadConfig();
      if (!ret) {
        config = nullptr;
      }
    });
    return config;
  };

  std::string GetString(const std::string &key,
                        const std::string &def = "") const;
  void SetProperty(const std::string &key, const std::string &def);
  bool GetBool(const std::string &key, bool def = false) const;
  int GetInt(const std::string &key, int def = 0) const;
  modelbox::Status LoadConfig();

 private:
  modelbox::Status LoadEnvConfig(const std::string &env);
  modelbox::Status LoadJsonConfig(const std::string &str,
                                  std::map<std::string, std::string> &key_map);

 private:
  std::vector<std::string> configKey_;
  std::shared_ptr<modelbox::Configuration> configuration_;
};

}  // namespace modelarts

#endif  // MODELARTS_CONFIG_H_