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

#include "modelarts_plugin.h"

namespace modelartsplugin {
std::shared_ptr<modelbox::Plugin> CreatePlugin() {
  MBLOG_INFO << "ModelArts create success.";
  return std::make_shared<ModelArtsPlugin>();
}

bool ModelArtsPlugin::Init(std::shared_ptr<modelbox::Configuration> config) {
  MBLOG_INFO << "ModelArts plugin init.";
  ma_manager_ = std::make_shared<ModelArtsManager>();
  auto status = ma_manager_->Init(config);
  if (!status) {
    status = {status, "ModelArts plugin init failed."};
    MBLOG_WARN << status.WrapErrormsgs();
    return false;
  }

  MBLOG_INFO << "ModelArts plugin init success.";
  return true;
}

bool ModelArtsPlugin::Start() {
  MBLOG_INFO << "ModelArts plugin start.";
  auto status = ma_manager_->Start();
  if (!status) {
    status = {status, "ModelArts plugin start failed."};
    MBLOG_WARN << status.WrapErrormsgs();
    return false;
  }

  MBLOG_INFO << "ModelArts plugin start success.";
  return true;
}

bool ModelArtsPlugin::Stop() {
  MBLOG_INFO << "ModelArts plugin stop.";
  auto status = ma_manager_->Stop();
  if (!status) {
    status = {status, "ModelArts plugin stop failed."};
    MBLOG_WARN << status.WrapErrormsgs();
    return false;
  }

  MBLOG_INFO << "ModelArts plugin stop success.";
  return true;
}

}  // namespace modelartsplugin
