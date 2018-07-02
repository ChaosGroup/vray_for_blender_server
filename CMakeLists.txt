#
# Copyright (c) 2015, Chaos Software Ltd
#
# VRayAppSDK Server Library
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
#

cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(VRayZmqServer)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(vfb_macros)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(LIBS_ROOT        ""  CACHE PATH "Custom libs root - zmq, sodium")
set(QT_ROOT          ""  CACHE PATH "Optional QT5 path")
set(INSTALL_LOCAL    ON  CACHE BOOL "Do Install step")

if (${INSTALL_LOCAL} AND NOT EXISTS ${VRAY_ZMQ_SERVER_INSTALL_PREFIX})
	message(FATAL_ERROR "Missing VRAY_ZMQ_SERVER_INSTALL_PREFIX for option INSTALL_LOCAL")
endif()

message(STATUS "V-Ray AppSDK ZMQ server install path: \"${VRAY_ZMQ_SERVER_INSTALL_PREFIX}\"")

include_directories(extern/vray-zmq-wrapper/include)
include_directories(extern/vray-zmq-wrapper/extern/cppzmq)

file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB WRAPPER_HEADERS extern/vray-zmq-wrapper/include/*.h)
list(APPEND HEADERS "${WRAPPER_HEADERS}")
file(GLOB WRAPPER_HEADERS extern/vray-zmq-wrapper/include/*.hpp)
list(APPEND HEADERS "${WRAPPER_HEADERS}")

use_qt(${QT_ROOT})
use_vray_appsdk(${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/appsdk)
use_zmq(${LIBS_ROOT})

if(WITH_STATIC_LIBC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

add_executable(${PROJECT_NAME} "${SOURCES};${HEADERS}")
link_with_vray_appsdk(${PROJECT_NAME})
link_with_zmq(${PROJECT_NAME})
link_with_qt()

if(UNIX AND NOT APPLE)
	target_link_libraries(${PROJECT_NAME} pthread rt dl)
endif()

set(INSTALL_PREFIX "${VRAY_ZMQ_SERVER_INSTALL_PREFIX}/V-Ray/VRayZmqServer/")

if(WIN32)
	install(TARGETS ${PROJECT_NAME} RUNTIME
					DESTINATION ${INSTALL_PREFIX})
elseif(UNIX)

	if(APPLE)
		install(TARGETS ${PROJECT_NAME} RUNTIME
					DESTINATION ${INSTALL_PREFIX})
	else()
		install(TARGETS ${PROJECT_NAME} RUNTIME
					DESTINATION ${INSTALL_PREFIX})
	endif()
endif()

