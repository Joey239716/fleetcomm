cmake_minimum_required(VERSION 3.20)
project(fleetcomm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    find_package(ZeroMQ CONFIG REQUIRED)
    find_package(cppzmq CONFIG REQUIRED)
    find_package(protobuf CONFIG REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ZMQ REQUIRED libzmq)
    find_package(Protobuf REQUIRED)
endif()

# ── Protobuf codegen ──────────────────────────────────────────────────────
set(PROTO_DIR     ${CMAKE_SOURCE_DIR}/proto)
set(PROTO_GEN_DIR ${CMAKE_SOURCE_DIR}/src/gen)

add_custom_command(
    OUTPUT  ${PROTO_GEN_DIR}/fleetcomm.pb.cc
            ${PROTO_GEN_DIR}/fleetcomm.pb.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_GEN_DIR}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --proto_path=${PROTO_DIR}
            --cpp_out=${PROTO_GEN_DIR}
            ${PROTO_DIR}/fleetcomm.proto
    DEPENDS ${PROTO_DIR}/fleetcomm.proto
    COMMENT "Generating C++ protobuf from fleetcomm.proto"
)

# ── Executable ────────────────────────────────────────────────────────────
add_executable(fleetcomm
    src/main.cpp
    src/graph.cpp
    src/car.cpp
    src/traffic_simulation.cpp
    src/spatial_grid.cpp
    src/v2v_simulation.cpp
    ${PROTO_GEN_DIR}/fleetcomm.pb.cc
)

if(WIN32)
    target_compile_definitions(fleetcomm PRIVATE _USE_MATH_DEFINES)
    target_include_directories(fleetcomm PRIVATE
        ${PROTO_GEN_DIR}
        third_party
    )
    target_link_libraries(fleetcomm
        libzmq
        cppzmq
        protobuf::libprotobuf
    )
else()
    target_include_directories(fleetcomm PRIVATE
        ${ZMQ_INCLUDE_DIRS}
        ${Protobuf_INCLUDE_DIRS}
        ${PROTO_GEN_DIR}
        third_party
    )
    target_link_directories(fleetcomm PRIVATE ${ZMQ_LIBRARY_DIRS})
    target_link_libraries(fleetcomm
        ${ZMQ_LIBRARIES}
        protobuf::libprotobuf
    )
    target_compile_options(fleetcomm PRIVATE ${ZMQ_CFLAGS_OTHER})
endif()
