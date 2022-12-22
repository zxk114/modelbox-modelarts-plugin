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

set(PLUGIN_NAME "modelarts-client")
project(${PLUGIN_NAME})

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "cannot build the project in the source directory! Out-of-source build is enforced!")
endif()
 
file(GLOB_RECURSE UNIT_SOURCE *.cpp *.cc *.c)

exclude_files_from_dir_in_list(UNIT_SOURCE "${UNIT_SOURCE}" "${CMAKE_BINARY_DIR}/")

set(INCLUDE ${CMAKE_CURRENT_LIST_DIR}/include/)
include_directories(${INCLUDE})
include_directories(${MQTT_INCLUDE})
include_directories(${APIGW_CPP_INCLUDE_DIR})
include_directories(${NLOHMANN_INCLUDE_DIR})
include_directories(${CPP_HTTPLIB_INCLUDE})
include_directories(/usr/local/include/modelbox)


add_library(${PLUGIN_NAME} SHARED ${UNIT_SOURCE})

set_target_properties(${PLUGIN_NAME} PROPERTIES 
    OUTPUT_NAME "${PLUGIN_NAME}"
    PREFIX ""
    SUFFIX ".so")

target_link_libraries(${PLUGIN_NAME} pthread)
target_link_libraries(${PLUGIN_NAME} rt)
target_link_libraries(${PLUGIN_NAME} dl)
target_link_libraries(${PLUGIN_NAME} ${MQTT_LIBRARY})
target_link_libraries(${PLUGIN_NAME} ${APIGW_CPP_LIBRARIES})

install(TARGETS ${PLUGIN_NAME} 
    COMPONENT ${UNIT_COMPONENT}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    OPTIONAL
    )

set(LIBMODELARTS_CLIENT_LIBRARY ${PLUGIN_NAME} CACHE INTERNAL "")
set(LIBMODELARTS_CLIENT_INCLUDE ${INCLUDE} CACHE INTERNAL "")