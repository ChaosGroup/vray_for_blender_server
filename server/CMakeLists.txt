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

project(VRayZmqServer)

include(vfh_vrayappsdk)

file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")

use_vray_appsdk()

if(WITH_STATIC_LIBC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

add_executable(${PROJECT_NAME} "${SOURCES};${HEADERS}")

set(SYSTEM_LIBS "")
if(UNIX)
	if(APPLE)
	else()
		set(SYSTEM_LIBS
			pthread rt dl
		)
	endif()
endif()

target_link_libraries(${PROJECT_NAME} vray_zmq_wrapper ${SYSTEM_LIBS})

add_definitions(-DSODIUM_STATIC)
add_definitions(-DZMQ_STATIC)
if(UNIX)
	target_link_libraries(${PROJECT_NAME}
		${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/zmq/lib/Release/libzmq.a
		${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/sodium/lib/Release/libsodium.a
		)
elseif(WIN32)
	target_link_libraries(${PROJECT_NAME} debug Debug/libsodium)
	target_link_libraries(${PROJECT_NAME} debug Debug/libzmq)
	target_link_libraries(${PROJECT_NAME} optimized Release/libsodium)
	target_link_libraries(${PROJECT_NAME} optimized Release/libzmq)
	target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()


if(WIN32)
	set(INSTALL_PREFIX "C:/Program Files/Chaos Group/")
elseif(UNIX)
	if(APPLE)
		set(INSTALL_PREFIX "/Applications/ChaosGroup/")
	else()
		set(INSTALL_PREFIX "/usr/ChaosGroup/")
	endif()
endif()

if(CMAKE_INSTALL_PREFIX)
	set(INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/V-Ray/VRayZmqServer/")
endif()



if(WIN32)
	install(TARGETS ${PROJECT_NAME} RUNTIME
					DESTINATION ${INSTALL_PREFIX})
elseif(UNIX)

	if(APPLE)
		install(TARGETS ${PROJECT_NAME}
					DESTINATION ${INSTALL_PREFIX})
	else()
		install(TARGETS ${PROJECT_NAME}
					DESTINATION ${INSTALL_PREFIX})
	endif()
endif()
