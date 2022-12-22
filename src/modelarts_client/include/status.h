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

#ifndef MODELARTS_STATUS_H_
#define MODELARTS_STATUS_H_

#include "modelbox/base/status.h"

namespace modelarts {
enum MAHttpStatusCode {
  STATUS_HTTP_OK = 200,
  STATUS_HTTP_CREATED = 201,
  STATUS_HTTP_ACCEPTED = 202,
  STATUS_HTTP_NO_CONTENT = 204,
  STATUS_HTTP_BAD_REQUEST = 400,
  STATUS_HTTP_NOT_FOUND = 404,
  STATUS_HTTP_INTERNAL_ERROR = 500
};

}

#endif  // MODELARTS_STATUS_H_