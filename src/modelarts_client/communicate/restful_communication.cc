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

#include "restful_communication.h"

#include "communication_factory.h"
#include "signer.h"
#include "utils.h"

namespace modelarts {

constexpr const char *MA_TASK_IP = "0.0.0.0";

REGISTER_COMMUNICATE("cloud", RestfulCommunication);
RestfulCommunication::RestfulCommunication(
    const std::shared_ptr<Config> &config,
    const std::shared_ptr<Cipher> &cipher)
    : Communication(config, cipher) {}

modelbox::Status RestfulCommunication::GetAkSk(std::string &ak,
                                               std::string &sk) {
  auto tmp_ak = config_->GetString(CONFIG_DEVELOPER_AK);
  if (tmp_ak.empty()) {
    return {modelbox::STATUS_FAULT, "GetAkSk failed. ak is null"};
  }

  auto tmp_sk_encoded = config_->GetString(CONFIG_DEVELOPER_SK);
  if (tmp_sk_encoded.empty()) {
    return {modelbox::STATUS_FAULT, "GetAkSk failed. sk is null"};
  }
  std::string tmp_sk;
  auto ret = cipher_->DecryptFromBase64(tmp_sk_encoded, tmp_sk);
  if (!ret || tmp_sk.empty()) {
    MBLOG_ERROR << "DecryptFromBase64 failed. use original sk , ret: " + ret.WrapErrormsgs();
    tmp_sk = tmp_sk_encoded;
  }

  ak = tmp_ak;
  sk = tmp_sk;
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status SendWithRetry(
    const std::string &url,
    const std::shared_ptr<RequestParams> &request_self) {
  int retry_count = 10;
  do {
    httplib::Headers headers;
    for (auto header : *(request_self->getHeaders())) {
      headers.insert({header.getKey(), header.getValue()});
      MBLOG_DEBUG << header.getKey() << ", " << header.getValue();
    }

    modelbox::HttpRequest request(modelbox::HttpMethods::POST, url);
    request.SetHeaders(headers);
    request.SetBody(request_self->getPayload());
    auto ret = SendHttpRequest(request);
    if (!ret) {
      --retry_count;
      MBLOG_WARN << "Send request failed, retry count: " << retry_count
                 << ", error: " << ret.WrapErrormsgs();
      continue;
    }

    auto response = request.GetResponse();
    if (response.status / 100 == 2) {
      MBLOG_INFO << "SendMsg success.";
      return modelbox::STATUS_SUCCESS;
    }
    --retry_count;
    auto msg = std::string("HttpRequest failed, status code:") +
               std::to_string(response.status) + std::string(" respbody: ") +
               response.body;
    MBLOG_ERROR << "SendMsg failed. retry count:" << retry_count
                << " msg:" << msg;
    std::this_thread::sleep_for(std::chrono::seconds(5));

  } while (retry_count != 0);

  MBLOG_ERROR << "SendMsg failed.";
  return modelbox::STATUS_FAULT;
}

modelbox::Status RestfulCommunication::SendMsg(const std::string &msg) {
  MBLOG_INFO << "start send message: " << DataMasking(msg);
  try {
    std::string ak;
    std::string sk;
    auto ret = GetAkSk(ak, sk);
    if (!ret) {
      MBLOG_ERROR << "SendMsg failed, error:" << ret.WrapErrormsgs();
      return modelbox::STATUS_FAULT;
    }
    std::string host;
    std::string uri;
    ret = GetSignerUrlInfo(host, uri);
    if (!ret) {
      MBLOG_ERROR << "SendMsg failed, error:" << ret.WrapErrormsgs();
      return modelbox::STATUS_FAULT;
    }

    std::shared_ptr<RequestParams> request_self =
        std::make_shared<RequestParams>("POST", host, "/" + uri + "/", "", msg);
    request_self->addHeader("content-type", "application/json");
    Signer signer(ak, sk);
    signer.createSignature(request_self.get());
    std::string url = config_->GetString(CONFIG_NOTIFY_URL);
    MBLOG_INFO << "send msg to modelarts, url: " << url
               << " , payload: " << DataMasking(msg);
    ret = SendWithRetry(url, request_self);
    if (!ret) {
      MBLOG_ERROR << "SendMsg failed, error:" << ret.WrapErrormsgs();
      return modelbox::STATUS_FAULT;
    }
  } catch (std::exception &e) {
    MBLOG_WARN << "SendMsg failed , exception." << e.what();
    return modelbox::STATUS_FAULT;
  }

  return modelbox::STATUS_SUCCESS;
}

modelbox::Status RestfulCommunication::SetupSSLServerConfig(
    const std::string &cert, const std::string &key,
    modelbox::HttpServerConfig &server_config) {
  std::string cert_str;
  auto ret = GetStringByPath(cert, cert_str);
  if (ret != modelbox::STATUS_SUCCESS) {
    MBLOG_ERROR << "failed to read file from cert path. " << cert;
    return modelbox::STATUS_FAULT;
  }

  std::string key_str;
  ret = GetStringByPath(key, key_str);
  if (ret != modelbox::STATUS_SUCCESS) {
    MBLOG_ERROR << "failed to read file from cert path. " << key;
    return modelbox::STATUS_FAULT;
  }

  auto setup_ssl_ctx_callback = [cert_str, key_str](SSL_CTX &ssl_ctx) {
    modelbox::HardeningSSL(&ssl_ctx);
    auto ret =
        modelbox::UseCertificate(ssl_ctx, cert_str.data(), cert_str.size());
    if (!ret) {
      MBLOG_ERROR << "load cert failed, err. " << ret;
      return false;
    }
    ret = modelbox::UsePrivateKey(ssl_ctx, key_str.data(), key_str.size());
    if (!ret) {
      MBLOG_ERROR << "load key failed, err. " << ret;
      return false;
    }

    return true;
  };

  server_config.SetSSLConfigCallback(setup_ssl_ctx_callback);
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status RestfulCommunication::Start() {
  server_->Start();
  MBLOG_INFO << "restful communication start.";
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status RestfulCommunication::Stop() {
  server_->Stop();
  MBLOG_INFO << "restful communication stop.";
  return modelbox::STATUS_SUCCESS;
}

modelbox::Status RestfulCommunication::Init() {
  std::string ip = MA_TASK_IP;
  std::string task_port = config_->GetString(CONFIG_TASK_PORT);
  std::string task_uri = config_->GetString(CONFIG_TASK_URI);

  modelbox::HttpServerConfig server_config;
  server_config.SetTimeout(std::chrono::seconds(10));
  std::string endpoint = "http://" + ip + ":" + task_port;

  server_ = std::make_shared<modelbox::HttpServer>(endpoint, server_config);
  auto task_func = std::bind(&RestfulCommunication::MsgProcess, this,
                             std::placeholders::_1, std::placeholders::_2);
  server_->Register(task_uri, modelbox::HttpMethods::POST, task_func);
  server_->Register(task_uri, modelbox::HttpMethods::DELETE, task_func);
  server_->Register(task_uri, modelbox::HttpMethods::GET, task_func);

  auto ret = server_->GetStatus();
  if (!ret) {
    MBLOG_ERROR << "Init server failed, err. " << ret;
    return modelbox::STATUS_FAULT;
  }
  MBLOG_INFO << "restful communication init success. ";
  return modelbox::STATUS_SUCCESS;
}

void RestfulCommunication::MsgProcess(const httplib::Request &request,
                                      httplib::Response &response) {
  try {
    MBLOG_INFO << "MsgProcess: Receive message method: " << request.method;

    if (!CheckUrlVaild(request.path, response)) {
      MBLOG_INFO << "CheckUrlVaild failed. ";
      return;
    }
    auto msg_type = GetMsgType(request.method);
    auto callback = FindMsgHandle(msg_type);
    if (callback == nullptr) {
      MBLOG_ERROR << "MsgProcess: FindMsgHandle failed, msg_type: " << msg_type;
      response.status = modelbox::HttpStatusCodes::BAD_REQUEST;
      return;
    }
    auto request_info = GetMsgRequestInfo(request);
    MBLOG_DEBUG << "Request body: " << request_info;
    std::lock_guard<std::mutex> lock(msg_concurrency_mutex_);
    std::string resp = "{}";
    std::shared_ptr<void> ptr;
    auto status = callback(request_info, resp, ptr);
    auto http_status = StatusToHttpStatus(status);
    MBLOG_INFO << "MsgProcess: reply. http_status:" << http_status
               << " body:" << resp;
    response.status = http_status;
    response.set_content(resp, modelbox::JSON);
    MBLOG_INFO << "send reply success. ";

    auto post_callback = FindMsgPostHandle(msg_type);
    if (post_callback == nullptr) {
      MBLOG_ERROR << "MsgProcess: FindPostMsgHandle failed, msg_type: "
                  << msg_type;
      return;
    }
    post_callback(request_info, resp, ptr);
  } catch (std::exception &e) {
    MBLOG_WARN << DataMasking(request.body);
    nlohmann::json err_json = {
        {MA_ERROR_CODE, modelbox::HttpStatusCodes::INTERNAL_ERROR},
        {MA_ERROR_MSG, std::string("exception: ") + e.what()}};
    MBLOG_WARN << "MsgProcess exception. error:" << err_json.dump();
    response.status = modelbox::HttpStatusCodes::INTERNAL_ERROR;
    response.set_content(err_json.dump(), modelbox::JSON);
  }
}

bool RestfulCommunication::CheckUrlVaild(const std::string &request_url,
                                         httplib::Response &response) {
  std::string task_uri = config_->GetString(CONFIG_TASK_URI);
  if (request_url.find(task_uri) != 0) {
    nlohmann::json err_json = {
        {MA_ERROR_CODE, modelbox::HttpStatusCodes::BAD_REQUEST},
        {MA_ERROR_MSG, "invalid url. url: " + request_url}};
    MBLOG_WARN << "MsgProcess url invalid." << err_json.dump();
    response.status = modelbox::HttpStatusCodes::BAD_REQUEST;
    response.set_content(err_json.dump(), modelbox::JSON);
    return false;
  }
  return true;
}

std::string RestfulCommunication::ParseTaskId(const std::string &uri) {
  std::string task_uri = config_->GetString(CONFIG_TASK_URI);
  std::string sub = std::string(task_uri) + "/";
  auto pos = uri.find(sub);
  if (pos != 0) {
    return "";
  }
  return uri.substr(sub.length());
}

modelbox::HttpStatusCode RestfulCommunication::StatusToHttpStatus(
    MAHttpStatusCode status) const {
  auto iter = http_status_map_.find(status);
  if (iter == http_status_map_.end()) {
    MBLOG_ERROR << "StatusToHttpStatus failed , status: " << status;
  }
  return iter->second;
}

std::string RestfulCommunication::GetMsgType(const std::string &method) {
  if (method == modelbox::HttpMethods::POST) {
    return MA_CREATE_TYPE;
  }
  if (method == modelbox::HttpMethods::DELETE) {
    return MA_DELETE_TYPE;
  }
  if (method == modelbox::HttpMethods::GET) {
    return MA_QUERY_TYPE;
  }
  MBLOG_ERROR << "GetMsgType failed , method: " << method << " is not support";
  return "";
}
std::string RestfulCommunication::GetMsgRequestInfo(
    const httplib::Request &request) {
  if (request.method == modelbox::HttpMethods::POST) {
    return request.body;
  }
  return ParseTaskId(request.path);
}

modelbox::Status RestfulCommunication::GetStringByPath(
    const std::string &path, std::string &output_str) {
  const size_t max_file_size = 128 * 1024;
  std::ifstream file(path, std::ios::binary);
  if (file.fail()) {
    MBLOG_ERROR << "open filestreamn failed, file path: " << path
                << " error: " << strerror(errno);
    return modelbox::STATUS_FAULT;
  }
  Defer { file.close(); };
  file.seekg(0, file.end);
  size_t size = file.tellg();
  if (size == 0 || size > max_file_size) {
    MBLOG_ERROR << "GetStringByPath:file size <=0, file path" << path;
    return modelbox::STATUS_FAULT;
  }

  char data[size] = {0};
  file.seekg(0, file.beg);
  file.read(data, size);
  if (file.fail()) {
    MBLOG_ERROR << "read file failed, file path " << path
                << " error: " << strerror(errno);
    return modelbox::STATUS_FAULT;
  }

  output_str = data;
  return modelbox::STATUS_SUCCESS;
}

std::string RestfulCommunication::FilterHttpPrefix(const std::string &url) {
  std::string http_header = "http://";
  std::string https_header = "https://";
  if (url.find(http_header) != std::string::npos) {
    return url.substr(http_header.length());
  } else if (url.find(https_header) != std::string::npos) {
    return url.substr(https_header.length());
  }
  return url;
}

modelbox::Status RestfulCommunication::GetSignerUrlInfo(std::string &host,
                                                        std::string &uri) {
  std::string endpoint = config_->GetString(CONFIG_ENDPOINT_MA_INFER);
  std::string url = config_->GetString(CONFIG_NOTIFY_URL);

  if (endpoint.empty() || url.empty()) {
    MBLOG_ERROR << "endpoint or url is null, endpoint:" << endpoint
                << " url:" << url;
    return modelbox::STATUS_FAULT;
  }

  if (url.find(endpoint) == std::string::npos) {
    MBLOG_ERROR << "url or endpoint is invailed, endpoint:" << endpoint
                << " url:" << url;
    return modelbox::STATUS_FAULT;
  }

  if ((endpoint.length() + 1) > url.length()) {
    MBLOG_ERROR << "url or endpoint is invailed, endpoint:" << endpoint
                << " url:" << url;
    return modelbox::STATUS_FAULT;
  }
  uri = url.substr(endpoint.length() + 1);
  host = FilterHttpPrefix(endpoint);
  return modelbox::STATUS_SUCCESS;
}

}  // namespace modelarts
