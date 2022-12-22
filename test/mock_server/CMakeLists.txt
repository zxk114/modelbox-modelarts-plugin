#
# Copyright 2022 The Modelbox Project Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE SOURCES *.cc *.cpp)

set(INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})

LIST(APPEND TEST_PLATFORM_INCLUDE ${INCLUDE})

LIST(APPEND TEST_PLATFORM_SOURCE ${SOURCES})

set(TEST_PLATFORM_SOURCE ${TEST_PLATFORM_SOURCE} CACHE INTERNAL "")
set(TEST_PLATFORM_INCLUDE ${TEST_PLATFORM_INCLUDE} CACHE INTERNAL "")