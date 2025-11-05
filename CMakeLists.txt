cmake_minimum_required(VERSION 3.5)
project(rsylar)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -fPIC -g -std=c++11 -Wall -Wno-deprecated  -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -O0 -fPIC -g -std=c11 -Wall -Wno-deprecated  -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
enable_language(ASM)
# -fno-omit-frame-pointer -fsanitize=thread

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # 对于 Clang，使用 -fstandalone-debug
    add_compile_options(-fstandalone-debug)
else()
    # 对于 GCC，保留 -rdynamic
    add_compile_options(-rdynamic)
endif()

include_directories(.)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_package(Protobuf REQUIRED)
if(NOT Protobuf_FOUND)
	message(FATAL_ERROR "Protobuf not found, please install protobuf and protoc")
endif()

find_package(Boost REQUIRED COMPONENTS serialization)
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost.Serialization not found")
endif()


set(LIB_SRC
    sylar/address.cc
    sylar/bytearray.cc
    sylar/config.cc
    sylar/daemon.cc
    sylar/env.cc
    sylar/fd_manager.cc
    sylar/fiber.cc
    sylar/hook.cc
    sylar/http/http.cc
    sylar/http/parser11/http11_parser.rl.cc
    sylar/http/parser11/httpclient_parser.rl.cc
    sylar/http/http_connection.cc
    sylar/http/http_parser.cc
    sylar/http/http_session.cc
    sylar/http/http_server.cc
    sylar/http/servlet.cc
    sylar/log.cc
    sylar/libco/coctx.h
    sylar/libco/coctx.cpp
    sylar/libco/coctx_swap.S
    sylar/mutex.cc
    sylar/scheduler.cc
    sylar/socket.cc
    sylar/stream.cc
    sylar/streams/socket_stream.cc
    sylar/tcp_server.cc
    sylar/thread.cc
    sylar/timer.cc
    sylar/uri.rl.cc
    sylar/util.cc
    sylar/util/json_util.cc
    sylar/iomanager.cc

    )

add_library(rsylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(rsylar)

set(LIBS
        yaml-cpp
        jsoncpp
        ${OPENSSL_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        )
# Ensure rsylar shared library records its external dependencies
target_link_libraries(rsylar PUBLIC ${LIBS})


add_subdirectory(rpc)
add_subdirectory(skipList)
add_subdirectory(raft)
add_subdirectory(tests)


