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

project(testcase)


file(GLOB_RECURSE SOURCES *.cc *.cpp)

configure_file(${CMAKE_CURRENT_LIST_DIR}/graph/create_task_case.toml.in ${TEST_WORKING_GRAPH_DIR}/create_task_case.toml @ONLY)

LIST(APPEND TEST_PLATFORM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
set(TEST_PLATFORM_INCLUDE ${TEST_PLATFORM_INCLUDE} CACHE INTERNAL "")

LIST(APPEND TEST_PLATFORM_SOURCE ${SOURCES})
set(TEST_PLATFORM_SOURCE ${TEST_PLATFORM_SOURCE} CACHE INTERNAL "")

list(REMOVE_DUPLICATES TEST_PLATFORM_INCLUDE)

include_directories(${TEST_PLATFORM_INCLUDE})

list(REMOVE_DUPLICATES TEST_PLATFORM_SOURCE)
add_executable(test_platform EXCLUDE_FROM_ALL
          ${TEST_PLATFORM_SOURCE}
		  ${TEST_SOURCE}
		  ${TEST_MAIN_SOURCE}
)

add_dependencies(test_platform ${MA_PLUGIN})

add_custom_target(all-drivers)
add_custom_command(TARGET all-drivers PRE_BUILD
    COMMAND rm -rf ${TEST_WORKING_LIB_DIR}/*
)

add_dependencies(all-drivers ${MA_PLUGIN})
add_custom_command(TARGET all-drivers POST_BUILD
    COMMAND cp $<TARGET_FILE:${MA_PLUGIN}> ${TEST_WORKING_LIB_DIR}/
)
add_custom_command(TARGET all-drivers POST_BUILD
    COMMAND cp $<TARGET_FILE:${LIBMODELARTS_CLIENT_LIBRARY}> ${TEST_WORKING_LIB_DIR}/
)

add_dependencies(test_platform ${MA_PLUGIN})
add_dependencies(test_platform all-drivers)

target_link_libraries(test_platform pthread)
target_link_libraries(test_platform rt)
target_link_libraries(test_platform dl)
target_link_libraries(test_platform gtest_main)
target_link_libraries(test_platform gmock_main)
target_link_libraries(test_platform modelbox)
target_link_libraries(test_platform cpprest ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

add_custom_target(test_platform_exe 
COMMAND ${TEST_RUNNER_LIST} ${CMAKE_CURRENT_BINARY_DIR}/test_platform
DEPENDS test_platform
WORKING_DIRECTORY ${TEST_WORKING_DIR}
COMMENT "Run test platform test...")

list(APPEND MODELBOX_UNIT_TEST_RUN_TARGETS test_platform_exe)
set(MODELBOX_UNIT_TEST_RUN_TARGETS ${MODELBOX_UNIT_TEST_RUN_TARGETS} CACHE INTERNAL "")


