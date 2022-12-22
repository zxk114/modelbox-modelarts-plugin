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

#include "test_case_base.h"

#include "test_case_utils.h"

void SetEnv() {
  std::string config = R"({
    "cloud_endpoint": {
        "obs_endpoint": "obs.cn-north-7.myhuaweicloud.com",
        "dis_endpoint": "https://127.0.0.1",
        "region": "cn-north-7",
        "vis_endpoint": "https://vis.cn-north-7.myhuaweicloud.com",
        "modelarts_infers_endpoint": "http://127.0.0.1:7500",
        "iam_endpoint": "http://127.0.0.1:7000"
    },
    "notification_url": "http://127.0.0.1:7500/v2/notifications",
    "instance_id": "MOCK_INSTANCE_ID",
    "service": {
        "port": 6500,
        "task_uri": "/v1/tasks"
    },
    "isv": {
        "project_id": "DEVELOP_USER_PROJECT_ID",
        "sign_ak": "11111-2222-3333-4444", 
        "sign_sk": "SKPyLubc1Ab5l8X4oQaX2gLwRIhl0rcPYbltQQ/Xhi/pJ+3akNrT1frymK7C3aLOslzIUWVGvmp0Oh8ovCOQczjVncjPK1671eoiNj7siZ0WT2SnN80LvnTl3GK8iot4ZDmjn16EHqNrhRN+RtUd0fV+5aoABEHI562vgoEthAbyVBQPX+pupSjO4W9uJdYdoSKLURFmfIC7DOPutSZs/nZProHDp4LGEVXQavh2cTIkd5GjMH2YFLZcLL8ckrWPWvLRTymvwING3KwdFyw3VNvRtlewHEpUfLRNA+AWvNTDh2LVgWKJd2AFjbz9wIpnhXkAB+F8K1ZusDDyYxFtiDz38owHQDSSZOBGj4X0Ab6UHjfbeiOaWykNm7CeB94MRs/CZj6CTR1fjG1+UXo/p4vKRNTkSQjxd/FXl4HYkyqZy+RFeQCByZ7yaiStHr5zxs715qEYKhRv6Vf2ZaFzBbgHD+KLoyLG6piDR2rVBN68iUaj42GuX3qBgwjpDPYA",
        "domain_id": "DEVELOP_USER_DOMAIN_ID"
    },
    "input_count_max": 10,
    "algorithm": {
        "multi_task": "yes",
        "alg_type": "cloud"
    },
    "deploy": {
        "service": "service"
    },
    "path": {"cert": ")" +
                       std::string(TEST_CIPHER_DIR) + R"(/",
        "rsa": ")" + std::string(TEST_CIPHER_DIR) +
                       R"(/" 
    },
    "topic": {
        "upstream": "$hw/modelarts/callback",
        "downstream": "modelarts/message"
    }
    })";
  setenv("MODELARTS_SVC_CONFIG", config.c_str(), true);
}

modelbox::Status TestCaseBase::CreateModelboxConfig(
    const std::string &toml_file_path, std::string &modelbox_config_path) {
  std::string conf = R"(
            [server]
            ip = "0.0.0.0"
            port = "6500"
            flow_path = ")" +
                     toml_file_path + R"("
            [key]
            https_cert_path = ")" +
                     std::string(TEST_CIPHER_DIR) + "/https_cert.pem" + R"("
            https_cert_privatekey_path = ")" +
                     std::string(TEST_CIPHER_DIR) + "/https_cert.key" + R"("
            [plugin]
            files = [")" +
                     std::string(TEST_LIB_DIR) +
                     "/modelbox-modelarts-plugin.so" + "" + R"("]
            [log]  
            path = "/var/log/modelbox/modelbox-server.log"
                                        )";
  if (modelbox::CreateDirectory(std::string(TEST_WORKING_DIR) + "/config") !=
      modelbox::STATUS_OK) {
    MBLOG_ERROR << "create directory failed, directory: "
                << std::string(TEST_WORKING_DIR) + "/config";
    return modelbox::STATUS_FAULT;
  }

  modelbox_config_path =
      std::string(TEST_WORKING_DIR) + "/config/modelbox-test.conf";
  struct stat buffer;
  if (stat(modelbox_config_path.c_str(), &buffer) == 0) {
    remove(modelbox_config_path.c_str());
  }

  std::ofstream ofs(modelbox_config_path);
  ofs.write(conf.data(), conf.size());
  ofs.flush();
  ofs.close();

  MBLOG_ERROR << "create modelbox config file success : "
              << modelbox_config_path;
  return modelbox::STATUS_OK;
}

modelbox::Status TestCaseBase::StartMockServer() {
  if (modelbox::CreateDirectory(std::string(TEST_WORKING_DIR) + "/graph") !=
      modelbox::STATUS_OK) {
    MBLOG_ERROR << "create directory failed, directory: "
                << std::string(TEST_WORKING_DIR) + "/graph";
    return modelbox::STATUS_FAULT;
  }

  SetEnv();

  auto toml_file_path_ = CreateTestToml();

  CreateModelboxConfig(toml_file_path_, modelbox_conig_path_);

  webhook_server_ = std::make_shared<WebHookMockServer>();
  webhook_server_->Start();

  ma_server_ = std::make_shared<MaMockServer>();
  ma_server_->Start();

  RegisterCustomHandle();

  MBLOG_INFO << "open modelbox proccess. ";

  std::string cmd_str = "/usr/local/bin/modelbox -c " + modelbox_conig_path_ +
                        " -fV -p /var/run/modelbox/modelbox.pid &";

  if (system(cmd_str.c_str())) {
    MBLOG_INFO << " execute cmd failed, cmd_str: " << cmd_str;
    return modelbox::STATUS_FAULT;
  }

  MBLOG_INFO << "mock server start success.";
  return modelbox::STATUS_OK;
}

void TestCaseBase::StopMockServer() {
  ma_server_->Stop();
  webhook_server_->Stop();

  MBLOG_INFO << "close modelbox proccess. ";

  std::string cmd_str =
      "kill -9 $(ps -ef |grep -v grep |grep modelbox | awk '{print $2}')";

  if (system(cmd_str.c_str())) {
    MBLOG_INFO << " execute cmd failed, cmd_str: " << cmd_str;
  }
   
  sleep(5) ;
  MBLOG_INFO << "mock server stop complete.";
}