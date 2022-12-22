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


option(ENABLE_FRAME_POINTER "enable frame pointer on 64bit system with flag -fno-omit-frame-pointer, on 32bit system, it is always enabled" ON)

include (CheckFunctionExists)
check_function_exists(dladdr HAVE_DLADDR)
check_function_exists(nanosleep HAVE_NANOSLEEP)

option(USE_CN_MIRROR "download from cn mirror" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-strict-aliasing")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-strict-aliasing")

add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-D_GNU_SOURCE)

if(OS_LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif(OS_LINUX)
