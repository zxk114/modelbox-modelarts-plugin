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
#include "ma_mock_server.h"

class CreateSingleTask : public TestCaseBase {
 public:
  modelbox::Status RegisterCustomHandle() override;
  std::string CreateTestToml() override;

 protected:
  web::json::value GenCreateTaskRequestBody(bool is_stream);
  void InitWebhookResult();
  void WaitWebhookResult(uint32_t timeout_ms, uint32_t count = 10);
  uint32_t GetWebhookResult();
  void WaitTaskState(const std::string &taskid, const std::string &state,
                     uint32_t timeout_ms);
  void WaitInstanceState(const std::string &state, uint32_t timeout_ms);
  std::shared_ptr<std::atomic<uint64_t>> webhook_count_ =
      std::make_shared<std::atomic<uint64_t>>(0);
};
