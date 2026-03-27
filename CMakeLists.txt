cmake_minimum_required(VERSION 3.20)
project(fleetcomm)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

include_directories(
    ${ZMQ_INCLUDE_DIRS}
    /opt/homebrew/Cellar/cppzmq/4.11.0/include
    third_party
)

link_directories(${ZMQ_LIBRARY_DIRS})

add_executable(fleetcomm
    src/main.cpp
    src/graph.cpp
)

target_include_directories(fleetcomm PRIVATE 
    ${ZMQ_INCLUDE_DIRS}
    /opt/homebrew/Cellar/cppzmq/4.11.0/include
)

target_link_libraries(fleetcomm ${ZMQ_LIBRARIES})
target_compile_options(fleetcomm PRIVATE ${ZMQ_CFLAGS_OTHER})
