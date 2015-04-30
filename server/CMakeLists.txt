cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

project(qt_server)

set(CMAKE_PREFIX_PATH "D:\\libs\\QT\\5.4\\msvc2013_64")


set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)


file(GLOB SOURCES . "*.cpp")
file(GLOB HEADERS . "*.h")

set(ZMQ_ROOT "D:/libs/ZeroMQ 4.0.4")

include_directories(${ZMQ_ROOT}/include)
link_directories(${ZMQ_ROOT}/lib)

add_executable(qt_server ${SOURCES} ${HEADERS})
target_link_libraries(qt_server zmq_wrapper Qt5::Gui Qt5::Core Qt5::Widgets)
