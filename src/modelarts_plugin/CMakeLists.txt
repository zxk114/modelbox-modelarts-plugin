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

set(PLUGIN_NAME "modelbox-modelarts-plugin")
project(${PLUGIN_NAME})

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "cannot build the project in the source directory! Out-of-source build is enforced!")
endif()
 
file(GLOB UNIT_SOURCE *.cpp *.cc *.c)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include/)
include_directories(${LIBMODELARTS_CLIENT_INCLUDE})

add_library(${PLUGIN_NAME} SHARED ${UNIT_SOURCE})

set_target_properties(${PLUGIN_NAME} PROPERTIES 
    OUTPUT_NAME "${PLUGIN_NAME}"
    PREFIX ""
    SUFFIX ".so")

add_dependencies(${PLUGIN_NAME} ${LIBMODELARTS_CLIENT_LIBRARY})

target_link_libraries(${PLUGIN_NAME} pthread)
target_link_libraries(${PLUGIN_NAME} rt)
target_link_libraries(${PLUGIN_NAME} dl)
target_link_libraries(${PLUGIN_NAME} modelbox-unit-cpu-obs_client)
target_link_libraries(${PLUGIN_NAME} modelbox-common-cpu-iam_auth)
target_link_libraries(${PLUGIN_NAME} modelbox-common-cpu-iam_auth)
target_link_libraries(${PLUGIN_NAME} ${LIBMODELARTS_CLIENT_LIBRARY})

install(TARGETS ${PLUGIN_NAME} 
    COMPONENT ${UNIT_COMPONENT}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    OPTIONAL
    )

set(MA_PLUGIN ${PLUGIN_NAME} CACHE INTERNAL "")
set(MA_PLUGIN_SO_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.so)
set(MA_PLUGIN_SO_PATH ${MA_PLUGIN_SO_PATH} CACHE INTERNAL "")