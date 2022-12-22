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

#ifndef TEST_CASE_UTILS_H_
#define TEST_CASE_UTILS_H_
#include <cpprest/http_msg.h>

#include <string>

#include "test_config.h"

const std::string MA_PLUGIN_MOCK_ENDPOINT = "http://127.0.0.1:6500";
const std::string IAM_MOCK_ENDPOINT = "http://127.0.0.1:7000";
const std::string MA_MOCK_ENDPOINT = "http://127.0.0.1:7500";
const std::string WEBHOOK_MOCK_ENDPOINT = "http://127.0.0.1:22360";
const std::string MA_PLUGIN_CREATE_TASK_URL =
    MA_PLUGIN_MOCK_ENDPOINT + "/v1/tasks";
const web::json::value MA_PLUGIN_WEBHOOK_OUTPUT = web::json::value::parse(R"({
    "data":{
        "headers":{
            "Content-Type": "application/json",
            "key": "aaa"
        },
        "url": ")" + WEBHOOK_MOCK_ENDPOINT +
                                                                          R"("
        },
        "type":"webhook"
})");

const web::json::value MA_PLUGIN_DIS_OUTPUT = web::json::value::parse(R"({
        "type": "dis",
        "data":
        {
            "stream_name": "test",
            "project_id":"1111111111111111"
        }
    })");
const web::json::value MA_PLUGIN_OBS_OUTPUT = web::json::value::parse(R"({
        "type": "obs",
        "data":
        {
            "bucket": "test",
            "path":"output/"
        }
    })");

const web::json::value MA_PLUGIN_OBS_INPUT = web::json::value::parse(R"({
        "type": "obs",
        "data":
        {
            "bucket": "test",
            "path":"input/test"
        }
    })");

const web::json::value MA_PLUGIN_VCN_INPUT = web::json::value::parse(R"({
        "type": "vcn",
        "data":
        {
            "stream_id": "111111111",
            "stream_type":2,
            "stream_ip":"1.1.1.1",
            "stream_port":"9000",
            "stream_user":"user",
            "protocol":"restful",
            "stream_pwd":""
        }
    })");

const web::json::value MA_PLUGIN_RESTFUL_INPUT = web::json::value::parse(R"({
        "type": "restful",
        "data":
        {
            "url": "http://127.0.0.1/test",
            "certificate":false,
            "rtsp_path":"data/url",
            "headers":{
                "Content-Type": "application/json",
                "key": "aaa"
            }
        }
    })");

const web::json::value MA_PLUGIN_URL_INPUT = web::json::value::parse(R"({
        "type": "url",
        "data":
        {
            "url": "http://127.0.0.1/test.mp4",
            "url_type":"file"
        }
    })");

const web::json::value MA_PLUGIN_VIS_INPUT = web::json::value::parse(R"({
        "type": "vis",
        "data":
        {
            "stream_name": "aaa",
            "project_id":"111111111111111"
        }
    })");

const web::json::value MA_PLUGIN_EDGECAMERA_INPUT = web::json::value::parse(R"({
        "type": "edgecamera",
        "data":
        {
            "id": "11111111111111111",
            "rtsp": "http://127.0.0.1/test.mp4"
        }
    })");

web::http::http_response DoRequestUrl(const std::string &url,
                                      web::http::http_request request);
web::json::value GenCreateTaskMsg();

#endif  // TEST_CASE_UTILS_H_