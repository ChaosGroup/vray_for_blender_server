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

project(VRayServerZmq)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(WIN32)
	set(CMAKE_PREFIX_PATH "D:\\libs\\boost_1_58_0")
endif()

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

if(BOOST_ROOT)
	find_package(Boost PATHS ${BOOST_ROOT} NO_DEFAULT_PATH)
endif()
find_package(Boost)
include_directories(${Boost_INCLUDE_DIR})


if(WIN32)
	set(ZMQ_ROOT "" CACHE PATH "ZMQ lib location")
	set(ZMQ_LIB  zmq-v120-mt-gd-4_0_4)

	include_directories(${ZMQ_ROOT}/include)
	link_directories(${ZMQ_ROOT}/lib)
else()
	set(ZMQ_LIB  zmq)
endif()

if(WIN32)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

add_subdirectory(extern/vray-zmq-wrapper)

include_directories(extern/vray-zmq-wrapper/include)
include_directories(extern/vray-zmq-wrapper/extern/cppzmq)

add_subdirectory(server)
