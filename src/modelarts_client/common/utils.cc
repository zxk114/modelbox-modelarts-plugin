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

#include <log.h>
#include <utils.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

namespace modelarts {

constexpr const char *UTC_FORMAT = "%Y-%m-%dT%H:%M:%SZ";

std::string DateTimeToUTCString(time_t time) {
  struct tm time_info;
  gmtime_r(&time, &time_info);
  std::stringstream time_buffer;
  time_buffer << std::put_time(&time_info, UTC_FORMAT);
  return time_buffer.str();
}

time_t DateTimeFromUTCString(const std::string &time) {
  std::stringstream time_buffer(time);
  struct tm time_info;
  time_buffer >> std::get_time(&time_info, UTC_FORMAT);
  return timegm(&time_info);
}

std::string DataMasking(const std::string &data) {
  std::regex url_auth_pattern("://[^ /]*?:[^ /]*?@");
  auto result = std::regex_replace(data, url_auth_pattern, "://*:*@");
  std::regex ak(R"("ak"[ ]*?:[ ]*?".*?")");
  result = std::regex_replace(result, ak, R"("ak":"*")");
  std::regex sk(R"("sk"[ ]*?:[ ]*?".*?")");
  result = std::regex_replace(result, sk, R"("sk":"*")");
  std::regex securityToken(R"("securityToken"[ ]*?:[ ]*?".*?")");
  result = std::regex_replace(result, securityToken, R"("securityToken":"*")");
  std::regex sign_ak(R"("sign_ak"[ ]*?:[ ]*?".*?")");
  result = std::regex_replace(result, sign_ak, R"("sign_ak":"*")");
  std::regex sign_sk(R"("sign_sk"[ ]*?:[ ]*?".*?")");
  result = std::regex_replace(result, sign_sk, R"("sign_sk":"*")");
  std::regex passwd(R"("passwd"[ ]*?:[ ]*?".*?")");
  result = std::regex_replace(result, passwd, R"("passwd":"*")");
  std::regex password(R"("password"[ ]*?:[ ]*?".*?")");
  result = std::regex_replace(result, password, R"("password":"*")");
  std::regex vcn_stream_pwd(R"("vcn_stream_pwd"[ ]*?:[ ]*?".*?")");
  result =
      std::regex_replace(result, vcn_stream_pwd, R"("vcn_stream_pwd":"*")");
  return result;
}

bool IsExpire(const std::string &expire) {
  if (expire.empty()) {
    MBLOG_WARN << "expire is empty.";
    return true;
  }

  time_t tp = DateTimeFromUTCString(expire);
  if (tp == 0) {
    MBLOG_WARN << "expire not ISO_8601 format."
               << " expire:" << expire;
    return true;
  }

  auto expire_tp =
      time(nullptr) +
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(30))
          .count();
  if (tp < expire_tp) {
    MBLOG_WARN << "expire timeout. expire: " << expire
               << " expire_tp:" << DateTimeToUTCString(expire_tp);
    return true;
  }

  return false;
}

}  // namespace modelarts