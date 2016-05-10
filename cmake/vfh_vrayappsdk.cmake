#
# Copyright (c) 2015, Chaos Software Ltd
#
# V-Ray Application SDK For C++
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

string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" _HOST_SYSTEM_NAME)

set(APPSDK_VERSION "447" CACHE STRING "V-Ray AppSDK version")
set(APPSDK_PATH "$ENV{HOME}/src/appsdk_releases" CACHE PATH "V-Ray AppSDK location")
set(APPSDK_ROOT "${APPSDK_PATH}/${APPSDK_VERSION}/${_HOST_SYSTEM_NAME}" CACHE PATH "V-Ray AppSDK root" FORCE)

if(WIN32)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".dll")
endif()

macro(use_vray_appsdk)
	if(NOT EXISTS ${APPSDK_ROOT})
		message(FATAL_ERROR "V-Ray AppSDK root (\"${APPSDK_ROOT}\") doesn't exist!")
	endif()

	find_library(VRAY_APPSDK_LIB
		NAMES VRaySDKLibrary
		PATHS ${APPSDK_ROOT}/bin ${APPSDK_ROOT}/lib
	)

	if(NOT VRAY_APPSDK_LIB)
		message(FATAL_ERROR "V-Ray AppSDK libraries are not found! Check APPSDK_PATH variable (current search path ${APPSDK_ROOT}/bin)")
	else()
		message(STATUS "Using V-Ray AppSDK: ${APPSDK_ROOT}")
	endif()

	include_directories(${APPSDK_ROOT}/cpp/include)
	link_directories(${APPSDK_ROOT}/bin)
	link_directories(${APPSDK_ROOT}/lib)

	# DEPRECATED: should be removed
	link_directories(${APPSDK_PATH}/devel)
endmacro()


macro(link_with_vray_appsdk _name)
	set(APPSDK_LIBS
		VRaySDKLibrary
	)
	target_link_libraries(${_name} ${APPSDK_LIBS})
endmacro()
