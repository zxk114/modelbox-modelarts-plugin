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

#include <config.h>

namespace modelarts {

Config::Config()
    : configKey_({CONFIG_INSTANCE_ID,
                  CONFIG_ALG_TYPE,
                  CONFIG_MAX_INPUT_COUNT,
                  CONFIG_NOTIFY_URL,
                  CONFIG_TASK_URI,
                  CONFIG_TASK_PORT,
                  CONFIG_DEVELOPER_PROJECTID,
                  CONFIG_DEVELOPER_DOMAIN_NAME,
                  CONFIG_DEVELOPER_DOAMIN_ID,
                  CONFIG_DEVELOPER_AK,
                  CONFIG_DEVELOPER_SK,
                  CONFIG_REGION,
                  CONFIG_ENDPOINT_IAM,
                  CONFIG_ENDPOINT_MA_INFER,
                  CONFIG_ENDPOINT_OBS,
                  CONFIG_ENDPOINT_DIS,
                  CONFIG_ENDPOINT_VIS,
                  CONFIG_PATH_RSA,
                  CONFIG_PATH_CERT,
                  CONFIG_TOPIC_UPSTREAM,
                  CONFIG_TOPIC_DOWNSTREAM}){};

Config::~Config() { configKey_.clear(); };

std::string Config::GetString(const std::string &key,
                              const std::string &def) const {
  return configuration_->GetString(key, def);
};
void Config::SetProperty(const std::string &key, const std::string &def) {
  configuration_->SetProperty(key, def);
};
bool Config::GetBool(const std::string &key, bool def) const {
  return configuration_->GetBool(key, def);
};
int Config::GetInt(const std::string &key, int def) const {
  int32_t tmp = def;
  return configuration_->GetInt32(key, tmp);
};

modelbox::Status Config::LoadConfig() {
  modelbox::ConfigurationBuilder builder;
  configuration_ = builder.Build();
  auto config = getenv("MODELARTS_SVC_CONFIG");
  if (config == nullptr) {
    MBLOG_ERROR << "env MODELARTS_SVC_CONFIG is not exist";
    return modelbox::STATUS_BADCONF;
  }

  auto status = LoadEnvConfig(config);
  if (!status) {
    MBLOG_ERROR << "load env config fail， error: " << status.WrapErrormsgs();
    return status;
  }

  MBLOG_INFO << "load config success ";
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Config::LoadEnvConfig(const std::string &env) {
  std::map<std::string, std::string> key_map = {
      {CONFIG_ENDPOINT_IAM, "/cloud_endpoint/iam_endpoint"},
      {CONFIG_ENDPOINT_MA_INFER, "/cloud_endpoint/modelarts_infers_endpoint"},
      {CONFIG_ENDPOINT_OBS, "/cloud_endpoint/obs_endpoint"},
      {CONFIG_ENDPOINT_DIS, "/cloud_endpoint/dis_endpoint"},
      {CONFIG_ENDPOINT_VIS, "/cloud_endpoint/vis_endpoint"},
      {CONFIG_REGION, "/cloud_endpoint/region"},
      {CONFIG_NOTIFY_URL, "/notification_url"},
      {CONFIG_INSTANCE_ID, "/instance_id"},
      {CONFIG_TASK_URI, "/service/task_uri"},
      {CONFIG_TASK_PORT, "/service/port"},
      {CONFIG_MAX_INPUT_COUNT, "/input_count_max"},
      {CONFIG_ALG_TYPE, "/algorithm/alg_type"},
      {CONFIG_DEVELOPER_PROJECTID, "/isv/project_id"},
      {CONFIG_DEVELOPER_DOAMIN_ID, "/isv/domain_id"},
      {CONFIG_DEVELOPER_DOMAIN_NAME, "/isv/domain_name"},
      {CONFIG_DEVELOPER_AK, "/isv/sign_ak"},
      {CONFIG_DEVELOPER_SK, "/isv/sign_sk"},
      {CONFIG_PATH_RSA, "/path/rsa"},
      {CONFIG_PATH_CERT, "/path/cert"},
      {CONFIG_TOPIC_UPSTREAM, "/topic/upstream"},
      {CONFIG_TOPIC_DOWNSTREAM, "/topic/upstream"}};

  auto status = LoadJsonConfig(env, key_map);
  if (!status) {
    return {status, "load json config failed."};
  }
  MBLOG_INFO << "load json config success";
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status Config::LoadJsonConfig(
    const std::string &str, std::map<std::string, std::string> &key_map) {
  try {
    nlohmann::json j = nlohmann::json::parse(str);
    for (auto &item : key_map) {
      nlohmann::json::json_pointer point(item.second);
      if (!j.contains(point)) {
        MBLOG_DEBUG << point.to_string() << " not exist";
        continue;
      }

      auto value = j[point];
      if (value.is_string()) {
        configuration_->SetProperty(item.first, value.get<std::string>());
        MBLOG_DEBUG << item.first << ":" << value.get<std::string>();
      } else if (value.is_number()) {
        configuration_->SetProperty(item.first, value.get<int>());
        MBLOG_DEBUG << item.first << ":" << value.get<int>();
      } else {
        MBLOG_WARN << point.to_string() << " unknow type";
      }
    }
  } catch (std::exception &e) {
    MBLOG_WARN << str;
    auto msg = std::string("Parse env failed. ") + e.what();
    MBLOG_WARN << msg;
    return {modelbox::STATUS_BADCONF, msg};
  }

  return modelbox::STATUS_SUCCESS;
}

}  // namespace modelarts
