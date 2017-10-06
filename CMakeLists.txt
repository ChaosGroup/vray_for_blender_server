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

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(LIBS_ROOT "" CACHE PATH "Custom libs root - zmq, sodium")
set(QT_ROOT "" CACHE PATH "Optional QT4 path")

set(MSVC_DIR_NAME "")
if (MSVC_VERSION EQUAL 1800)
	set(MSVC_DIR_NAME "vc12")
elseif(MSVC_VERSION EQUAL 1900)
	set(MSVC_DIR_NAME "vc14")
endif()

if (WIN32)
	if (MSVC_DIR_NAME EQUAL "")
		message(FATAL_ERROR "Only supported versions of MSVC are 2013 (vc12) and 2015 (vc14)")
	endif()
endif()

include_directories(${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/${MSVC_DIR_NAME}/zmq/include)
if(WIN32)
	link_directories(${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/${MSVC_DIR_NAME}/zmq/lib)
	link_directories(${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/${MSVC_DIR_NAME}/sodium/lib)
endif()

if(WIN32)
	if (MSVC)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	endif()
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -L/usr/lib64")
	link_directories(/usr/lib64)
endif()

include_directories(extern/vray-zmq-wrapper/include)
include_directories(extern/vray-zmq-wrapper/extern/cppzmq)

include(vfh_vrayappsdk)

file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB WRAPPER_HEADERS extern/vray-zmq-wrapper/include/*.h)
list(APPEND HEADERS "${WRAPPER_HEADERS}")
file(GLOB WRAPPER_HEADERS extern/vray-zmq-wrapper/include/*.hpp)
list(APPEND HEADERS "${WRAPPER_HEADERS}")

use_vray_appsdk(${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/${MSVC_DIR_NAME}/appsdk)

if(WITH_STATIC_LIBC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

add_executable(${PROJECT_NAME} "${SOURCES};${HEADERS}")
link_with_vray_appsdk(${PROJECT_NAME})

if (QT_ROOT)
	set(QT_INCLUDES
		${QT_ROOT}/include
		${QT_ROOT}/include/QtCore
		${QT_ROOT}/include/QtGui
	)

	if (WIN32)
		set(QT_LIBPATH ${QT_ROOT}/lib/)
		set(QT_LIB_EXT "4.lib")
		set(QT_LIB_PREFIX "")
	else()
		if (APPLE)
			set(QT_LIBPATH ${QT_ROOT}/lib/)
			set(QT_LIB_EXT ".dylib")
			set(QT_LIB_PREFIX "lib")
		else()
			set(QT_LIBPATH ${QT_ROOT}/lib/linux_x64/gcc-4.1)
			set(QT_LIB_EXT ".so.4")
			set(QT_LIB_PREFIX "lib")
		endif()
	endif()

	include_directories(${QT_INCLUDES})
	link_directories(${QT_LIBPATH})
	target_link_libraries(${PROJECT_NAME} ${QT_LIBPATH}/${QT_LIB_PREFIX}QtCore${QT_LIB_EXT})
	target_link_libraries(${PROJECT_NAME} ${QT_LIBPATH}/${QT_LIB_PREFIX}QtGui${QT_LIB_EXT})
else()
	find_package(Qt4 REQUIRED)
	include_directories(${Qt4_INCLUDE_DIRS})
	include(${QT_USE_FILE})
	target_link_libraries(${PROJECT_NAME} ${QT_QTMAIN_LIBRARY} ${QT_LIBRARIES})
endif()


set(SYSTEM_LIBS "")
if(UNIX)
	if(APPLE)
	else()
		set(SYSTEM_LIBS
			pthread rt dl
		)
	endif()
endif()

target_link_libraries(${PROJECT_NAME} ${SYSTEM_LIBS})

add_definitions(-DVRAY_SDK_INTEROPERABILITY)
add_definitions(-DSODIUM_STATIC)
add_definitions(-DZMQ_STATIC)
if(UNIX)
	target_link_libraries(${PROJECT_NAME}
		${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/${MSVC_DIR_NAME}/zmq/lib/Release/libzmq.a
		${LIBS_ROOT}/${CMAKE_SYSTEM_NAME}/${MSVC_DIR_NAME}/sodium/lib/Release/libsodium.a
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
		install(TARGETS ${PROJECT_NAME} RUNTIME
					DESTINATION ${INSTALL_PREFIX})
	else()
		install(TARGETS ${PROJECT_NAME} RUNTIME
					DESTINATION ${INSTALL_PREFIX})
	endif()
endif()

