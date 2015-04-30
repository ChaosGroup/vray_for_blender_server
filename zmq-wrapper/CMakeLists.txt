project(zmq_wrapper)

file(GLOB SOURCES . "*.cpp")
file(GLOB HEADERS . "*.h")

set(ZMQ_ROOT "D:/libs/ZeroMQ 4.0.4")

include_directories(${ZMQ_ROOT}/include)
link_directories(${ZMQ_ROOT}/lib)

add_library(zmq_wrapper STATIC ${SOURCES} ${HEADERS})
target_link_libraries(zmq_wrapper PUBLIC zmq-v120-mt-gd-4_0_4)

