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


if(NOT TEST_WORKING_DIR)
    set(TEST_WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR}/test-working-dir)
	file(MAKE_DIRECTORY ${TEST_WORKING_DIR})
endif()

set(TEST_WORKING_DATA_DIR "${TEST_WORKING_DIR}/data")
file(MAKE_DIRECTORY ${TEST_WORKING_DATA_DIR})
set(TEST_WORKING_LIB_DIR "${TEST_WORKING_DIR}/lib")
file(MAKE_DIRECTORY ${TEST_WORKING_LIB_DIR})
set(TEST_WORKING_BIN_DIR "${TEST_WORKING_DIR}/bin")
file(MAKE_DIRECTORY ${TEST_WORKING_BIN_DIR})
set(TEST_WORKING_DRIVERS_DIR "${TEST_WORKING_DIR}/drivers")
file(MAKE_DIRECTORY ${TEST_WORKING_DRIVERS_DIR})
set(TEST_WORKING_GRAPH_DIR "${TEST_WORKING_DIR}/graph")
file(MAKE_DIRECTORY ${TEST_WORKING_GRAPH_DIR})
set(TEST_ASSETS "${CMAKE_CURRENT_LIST_DIR}/assets")
set(TEST_MA_PLUGIN_SO_PATH "${MA_PLUGIN_SO_PATH}")
set(TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(TEST_CIPHER_DIR "${CMAKE_CURRENT_LIST_DIR}/assets/cert")


add_definitions(-DBUILD_TEST)
set(TEST_MAIN_SOURCE ${CMAKE_CURRENT_LIST_DIR}/test_main.cc)
configure_file(${CMAKE_CURRENT_LIST_DIR}/test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h @ONLY)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})
include_directories(${HUAWEI_SECURE_C_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${NLOHMANN_INCLUDE_DIR})

find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system)

add_subdirectory(common)
add_subdirectory(mock_server)
add_subdirectory(modelarts_plugin_test)

list(REMOVE_DUPLICATES MODELBOX_UNIT_TEST_RUN_TARGETS)

add_custom_target(unittest
	DEPENDS ${MODELBOX_UNIT_TEST_RUN_TARGETS}
	WORKING_DIRECTORY ${TEST_WORKING_DIR}
	COMMENT "Run Unit Test..."
)

