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

#include "test_case_create_task.h"

void CreateSingleTask::InitWebhookResult() {
  *webhook_count_ = 0;
  return;
}

void CreateSingleTask::WaitWebhookResult(uint32_t timeout_ms, uint32_t count) {
  uint32_t time_count_ms = 0;

  while (time_count_ms <= timeout_ms && GetWebhookResult() <= count) {
    usleep(200 * 1000);
    time_count_ms += 200;
  }
  return;
}

uint32_t CreateSingleTask::GetWebhookResult() { return *webhook_count_; }

void CreateSingleTask::WaitTaskState(const std::string &taskid,
                                     const std::string &state,
                                     uint32_t timeout_ms) {
  uint32_t time_count_ms = 0;
  std::string get_state;
  while (time_count_ms <= timeout_ms && get_state != state) {
    get_state = ma_server_->GetTaskState(taskid);
    usleep(200 * 1000);
    time_count_ms += 200;
  }

  return;
}

void CreateSingleTask::WaitInstanceState(const std::string &state,
                                         uint32_t timeout_ms) {
  uint32_t time_count_ms = 0;
  std::string get_state;
  while (time_count_ms <= timeout_ms && get_state != state) {
    get_state = ma_server_->GetInstanceState("MOCK_INSTANCE_ID");
    usleep(200 * 1000);
    time_count_ms += 200;
  }

  return;
}

modelbox::Status CreateSingleTask::RegisterCustomHandle() {
  webhook_server_->RegisterCustomHandle(
      [this](web::http::http_request request) {
        request.extract_string().then([=](utility::string_t request_body) {
          MBLOG_DEBUG << "get webhook result: " << request_body;
          (*this->webhook_count_)++;
          utility::string_t resp_body = "OK";
          try {
            request.reply(web::http::status_codes::OK, resp_body);
          } catch (const std::exception &e) {
            MBLOG_ERROR << "webhook request reply failed, " << e.what();
          }
        });
        return modelbox::STATUS_OK;
      });

  return modelbox::STATUS_OK;
}

std::string CreateSingleTask::CreateTestToml() {
  return std::string(TEST_GRAPH_DIR) + "/create_task_case.toml";
}

web::json::value CreateSingleTask::GenCreateTaskRequestBody(bool is_stream) {
  auto create_body = GenCreateTaskMsg();
  std::string url_str =
      std::string(TEST_ASSETS) + "/video/avc1_5s_480x320_24fps_yuv420_8bit.mp4";
  std::string url_type = is_stream ? "stream" : "file";
  create_body["input"] = web::json::value::parse(R"({
        "type": "url",
        "data":
        {
            "url":")" + url_str + R"(",
            "url_type":")" + url_type + R"("
        }
    })");

  create_body["outputs"][0] = MA_PLUGIN_WEBHOOK_OUTPUT;

  return create_body;
}

TEST_F(CreateSingleTask, TestCase_file) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  auto request_body = GenCreateTaskRequestBody(false);
  std::string task_id;
  auto ret = ma_server_->CreateTask(request_body.serialize(), task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "RUNNING";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  get_state = "NOT_FOUND";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  EXPECT_EQ(GetWebhookResult(), 120);
};

TEST_F(CreateSingleTask, TestCase_multi_file) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  std::vector<std::string> taskid_list(10);
  for (size_t i = 0; i < taskid_list.size(); i++) {
    auto request_body = GenCreateTaskRequestBody(false);
    auto ret = ma_server_->CreateTask(request_body.serialize(), taskid_list[i]);
    EXPECT_EQ(ret, modelbox::STATUS_OK);
  }

  for (size_t i = 0; i < taskid_list.size(); i++) {
    get_state = "RUNNING";
    WaitTaskState(taskid_list[i], get_state, timeout_ms);
    EXPECT_EQ(ma_server_->GetTaskState(taskid_list[i]), get_state);
  }

  for (size_t i = 0; i < taskid_list.size(); i++) {
    get_state = "NOT_FOUND";
    WaitTaskState(taskid_list[i], get_state, timeout_ms);
    EXPECT_EQ(ma_server_->GetTaskState(taskid_list[i]), get_state);
  }

  EXPECT_EQ(GetWebhookResult(), 120 * taskid_list.size());
};

TEST_F(CreateSingleTask, TestCase_stream) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  auto request_body = GenCreateTaskRequestBody(true);
  std::string task_id;
  auto ret = ma_server_->CreateTask(request_body.serialize(), task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "RUNNING";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  uint32_t count = 121;
  WaitWebhookResult(timeout_ms, count);
  EXPECT_GT(GetWebhookResult(), count);

  ret = ma_server_->DeleteTask(task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "NOT_FOUND";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);
};

TEST_F(CreateSingleTask, TestCase_multi_stream) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  std::vector<std::string> taskid_list(10);
  for (size_t i = 0; i < taskid_list.size(); i++) {
    auto request_body = GenCreateTaskRequestBody(true);
    auto ret = ma_server_->CreateTask(request_body.serialize(), taskid_list[i]);
    EXPECT_EQ(ret, modelbox::STATUS_OK);
  }

  for (size_t i = 0; i < taskid_list.size(); i++) {
    get_state = "RUNNING";
    WaitTaskState(taskid_list[i], get_state, timeout_ms);
    EXPECT_EQ(ma_server_->GetTaskState(taskid_list[i]), get_state);
  }

  uint32_t count = 121 * taskid_list.size();
  WaitWebhookResult(timeout_ms, count);
  EXPECT_GT(GetWebhookResult(), count);

  for (size_t i = 0; i < taskid_list.size(); i++) {
    auto ret = ma_server_->DeleteTask(taskid_list[i]);
    EXPECT_EQ(ret, modelbox::STATUS_OK);
  }

  for (size_t i = 0; i < taskid_list.size(); i++) {
    get_state = "NOT_FOUND";
    WaitTaskState(taskid_list[i], get_state, timeout_ms);
    EXPECT_EQ(ma_server_->GetTaskState(taskid_list[i]), get_state);
  }
};

TEST_F(CreateSingleTask, TestCase_vcn_obs_dis_webhook) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  auto request_body = GenCreateTaskMsg();
  request_body["input"] = MA_PLUGIN_VCN_INPUT;
  request_body["outputs"][0] = MA_PLUGIN_DIS_OUTPUT;
  request_body["outputs"][1] = MA_PLUGIN_OBS_OUTPUT;
  request_body["outputs"][2] = MA_PLUGIN_WEBHOOK_OUTPUT;

  std::string task_id;
  auto ret = ma_server_->CreateTask(request_body.serialize(), task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "RUNNING";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  sleep(1);

  ret = ma_server_->DeleteTask(task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "NOT_FOUND";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);
};

TEST_F(CreateSingleTask, TestCase_restful_obs_dis_webhook) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  auto request_body = GenCreateTaskMsg();
  request_body["input"] = MA_PLUGIN_RESTFUL_INPUT;
  request_body["outputs"][0] = MA_PLUGIN_DIS_OUTPUT;
  request_body["outputs"][1] = MA_PLUGIN_OBS_OUTPUT;
  request_body["outputs"][2] = MA_PLUGIN_WEBHOOK_OUTPUT;

  std::string task_id;
  auto ret = ma_server_->CreateTask(request_body.serialize(), task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "RUNNING";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  sleep(1);

  ret = ma_server_->DeleteTask(task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "NOT_FOUND";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);
};

TEST_F(CreateSingleTask, TestCase_obs_obs) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  auto request_body = GenCreateTaskMsg();
  request_body["input"] = MA_PLUGIN_OBS_INPUT;
  request_body["outputs"][0] = MA_PLUGIN_OBS_OUTPUT;

  std::string task_id;
  auto ret = ma_server_->CreateTask(request_body.serialize(), task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "RUNNING";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  sleep(1);

  ret = ma_server_->DeleteTask(task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "NOT_FOUND";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);
};

TEST_F(CreateSingleTask, TestCase_vis_dis) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  auto request_body = GenCreateTaskMsg();
  request_body["input"] = MA_PLUGIN_VIS_INPUT;
  request_body["outputs"][0] = MA_PLUGIN_DIS_OUTPUT;

  std::string task_id;
  auto ret = ma_server_->CreateTask(request_body.serialize(), task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "RUNNING";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  sleep(1);

  ret = ma_server_->DeleteTask(task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "NOT_FOUND";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);
};

TEST_F(CreateSingleTask, TestCase_edgecamera_dis_obs) {
  const uint32_t timeout_ms = 100000;
  std::string get_state = "RUNNING";
  WaitInstanceState(get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetInstanceState("MOCK_INSTANCE_ID"), get_state);

  auto request_body = GenCreateTaskMsg();
  request_body["input"] = MA_PLUGIN_EDGECAMERA_INPUT;
  request_body["outputs"][0] = MA_PLUGIN_DIS_OUTPUT;
  request_body["outputs"][1] = MA_PLUGIN_OBS_OUTPUT;

  std::string task_id;
  auto ret = ma_server_->CreateTask(request_body.serialize(), task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "RUNNING";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);

  sleep(1);

  ret = ma_server_->DeleteTask(task_id);
  EXPECT_EQ(ret, modelbox::STATUS_OK);

  get_state = "NOT_FOUND";
  WaitTaskState(task_id, get_state, timeout_ms);
  EXPECT_EQ(ma_server_->GetTaskState(task_id), get_state);
};