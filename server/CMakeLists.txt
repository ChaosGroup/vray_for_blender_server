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

project(qt_server)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")
file(GLOB UI      "*.ui")

QT5_WRAP_UI(UIS_HDRS ${UI})

add_executable(${PROJECT_NAME} "${SOURCES};${HEADERS};${UI};${UIS_HDRS}")

target_link_libraries(${PROJECT_NAME} zmq_wrapper Qt5::Gui Qt5::Core Qt5::Widgets)
