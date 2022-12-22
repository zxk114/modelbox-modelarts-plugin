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

#ifndef TEST_CASE_BASE_H_
#define TEST_CASE_BASE_H_
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ma_mock_server.h"
#include "webhook_mock_server.h"

class TestCaseBase : public testing::Test {
 public:
  virtual modelbox::Status RegisterCustomHandle() = 0;
  virtual std::string CreateTestToml() = 0;
  virtual modelbox::Status CreateModelboxConfig(
      const std::string &toml_file_path, std::string &modelbox_config_path);

 protected:
  static void SetUpTestSuite(){};

  virtual void SetUp() {
    auto ret = StartMockServer();
    ASSERT_EQ(ret, modelbox::STATUS_OK);
  }

  virtual void TearDown() { StopMockServer(); };

 public:
  std::shared_ptr<MaMockServer> ma_server_;
  std::shared_ptr<WebHookMockServer> webhook_server_;

 private:
  modelbox::Status StartMockServer();
  void StopMockServer();
  std::string toml_file_path_;
  std::string modelbox_conig_path_;
};

#endif  // TEST_CASE_BASE_H_