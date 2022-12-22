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

#ifndef MODELARTS_TASK_IO_H_
#define MODELARTS_TASK_IO_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "config.h"
#include "status.h"

namespace modelarts {

constexpr const char *VCN_PROOCOL_SDK = "sdk";
constexpr const char *VCN_PROOCOL_RESTFUL = "restful";

class TaskIO {
 public:
  TaskIO() = default;
  virtual ~TaskIO() = default;
  virtual modelbox::Status Parse(const std::string &data);
  virtual void SetInput(bool input) { input_ = input; }
  virtual bool IsInput() const { return input_; }
  virtual std::string GetType() const { return type_; }
  virtual std::string ToString() const = 0;

 protected:
  bool input_{false};
  std::string type_;
};

class ObsIO : public TaskIO {
 public:
  ObsIO() = default;
  ~ObsIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  std::string GetBucket() const { return bucket_; }
  std::string GetPath() const { return path_; }

 private:
  std::string bucket_;
  std::string path_;
};

class VisIO : public TaskIO {
 public:
  VisIO() = default;
  ~VisIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  std::string GetStreamName() const { return streamName_; }
  std::string GetProjectId() const { return projectId_; }

 private:
  std::string streamName_;
  std::string projectId_;
};

class DisIO : public TaskIO {
 public:
  DisIO() = default;
  ~DisIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  std::string GetStreamName() const { return streamName_; }
  std::string GetStreamId() const { return streamId_; }
  std::string GetProjectId() const { return projectId_; }

 private:
  std::string streamName_;
  std::string streamId_;
  std::string projectId_;
};

class UrlIO : public TaskIO {
 public:
  UrlIO() = default;
  ~UrlIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  std::string GetUrl() const { return url_; }
  std::string GetUrlType() const { return url_type_; }

 private:
  std::string url_;
  std::string url_type_;
};

class EdgeCameraIO : public TaskIO {
 public:
  EdgeCameraIO() = default;
  ~EdgeCameraIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  std::string GetStreamId() const { return streamId_; }
  std::string GetRtspStr() const { return rtspStr_; }

 private:
  std::string streamId_;
  std::string rtspStr_;
};

class EdgeRestfulIO : public TaskIO {
 public:
  EdgeRestfulIO() = default;
  ~EdgeRestfulIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  bool GetCertificate() const { return certificate_; }
  std::string GetRtspPath() const { return rtspPath_; }
  std::string GetUrlStr() const { return urlStr_; }
  std::map<std::string, std::string> GetHeaders() const { return headers_; }

 private:
  bool certificate_{false};
  std::string rtspPath_;
  std::string urlStr_;
  std::map<std::string, std::string> headers_;
};

class WebhookIO : public TaskIO {
 public:
  WebhookIO() = default;
  ~WebhookIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  std::string GetUrlStr() const { return urlStr_; }
  std::map<std::string, std::string> GetHeaders() const { return headers_; }

 private:
  std::string urlStr_;
  std::map<std::string, std::string> headers_;
};

class VcnIO : public TaskIO {
 public:
  VcnIO() = default;
  ~VcnIO() = default;
  modelbox::Status Parse(const std::string &data) override;
  std::string ToString() const override;
  std::string GetIP() const { return ip_; }
  std::string GetPort() const { return port_; }
  std::string GetUserName() const { return userName_; }
  std::string GetPassword() const { return password_; }
  std::string GetStreamId() const { return streamId_; }
  uint32_t GetStreamType() const { return streamType_; }
  std::string GetVcnProtocol() const { return vcnProtocol_; }

 private:
  std::string ip_;
  std::string port_;
  std::string userName_;
  std::string password_;
  std::string streamId_;
  uint32_t streamType_{0};
  std::string vcnProtocol_{modelarts::VCN_PROOCOL_RESTFUL};
};

using IOType = std::tuple<std::string, bool>;

class IOFactory {
 public:
  static std::shared_ptr<IOFactory> &GetInstance() {
    static auto io = std::shared_ptr<IOFactory>(new (std::nothrow) IOFactory());
    return io;
  }

  void Register(const IOType &type,
                const std::function<std::shared_ptr<TaskIO>()> &func);

  ~IOFactory() = default;

  modelbox::Status Parse(const std::string &data, bool isInput,
                         std::shared_ptr<TaskIO> &io);

 private:
  IOFactory() = default;
  std::map<IOType, std::function<std::shared_ptr<TaskIO>()>> ioType_;
};

class IoRegister {
 public:
  IoRegister(const IOType &type,
             const std::function<std::shared_ptr<TaskIO>()> &func);
  ~IoRegister() = default;
};

#define REGISTER_TASK_IO(name, is_input, clazz)                                \
  __attribute__((unused)) static IoRegister g_##is_input##_##clazz##_register( \
      std::tuple<std::string, bool>{name, is_input},                           \
      []() -> std::shared_ptr<TaskIO> {                                        \
        auto item = std::make_shared<clazz>();                                 \
        item->SetInput(is_input);                                              \
        return item;                                                           \
      });

}  // namespace modelarts

#endif  // MODELARTS_TASK_IO_H_