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

#ifndef MODELARTS_PLUGIN_H_
#define MODELARTS_PLUGIN_H_

#include <modelarts_manager.h>

#include <string>

namespace modelartsplugin {

class ModelArtsPlugin : public modelbox::Plugin {
 public:
  ModelArtsPlugin() = default;
  ~ModelArtsPlugin() override = default;

  virtual bool Init(std::shared_ptr<modelbox::Configuration> config) override;
  virtual bool Start() override;
  virtual bool Stop() override;

 private:
  std::shared_ptr<ModelArtsManager> ma_manager_;
};

extern "C" {
std::shared_ptr<modelbox::Plugin> CreatePlugin();
}

}  // namespace modelartsplugin

#endif  // MODELARTS_PLUGIN_H_