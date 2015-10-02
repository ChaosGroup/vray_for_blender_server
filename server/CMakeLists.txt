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

add_executable(${PROJECT_NAME} "${SOURCES};${HEADERS}")

link_with_vray_appsdk(${PROJECT_NAME})

set(SYSTEM_LIBS "")
if(UNIX)
	if(APPLE)
	else()
		set(SYSTEM_LIBS
			pthread
		)
	endif()
endif()

target_link_libraries(${PROJECT_NAME} vray_zmq_wrapper ${ZMQ_LIB} ${SYSTEM_LIBS})

if(WIN32)
	install(TARGETS ${PROJECT_NAME} RUNTIME
					DESTINATION "C:/Program Files/Chaos Group/V-Ray/VRayZmqServer/")
elseif(UNIX)
	if(APPLE)
		install(TARGETS ${PROJECT_NAME}
					DESTINATION "/Applications/ChaosGroup/V-Ray/VRayZmqServer/")
	else()
		install(TARGETS ${PROJECT_NAME}
					DESTINATION "/usr/ChaosGroup/V-Ray/VRayZmqServer/")
	endif()
endif()
