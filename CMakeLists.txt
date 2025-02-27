cmake_minimum_required(VERSION 3.5)
project(rsylar)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -fPIC -g -std=c++11 -Wall -Wno-deprecated  -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -O0 -fPIC -g -std=c11 -Wall -Wno-deprecated  -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")


if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # 对于 Clang，使用 -fstandalone-debug
    add_compile_options(-fstandalone-debug)
else()
    # 对于 GCC，保留 -rdynamic
    add_compile_options(-rdynamic)
endif()

include_directories(.)

set(LIB_SRC
    sylar/config.cc
    sylar/env.cc
    sylar/fiber.cc
    sylar/log.cc
    sylar/mutex.cc
    sylar/scheduler.cc
    sylar/thread.cc
    sylar/util.cc

    )

add_library(rsylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(rsylar)

set(LIBS
        rsylar
        yaml-cpp
        )


rsylar_add_executable(test_config "tests/test_config.cc" rsylar "${LIBS}")
rsylar_add_executable(test_env "tests/test_env.cc" rsylar "${LIBS}")
rsylar_add_executable(test_fiber "tests/test_fiber.cc" rsylar "${LIBS}")
rsylar_add_executable(test_log "tests/test_log.cc" rsylar "${LIBS}")
rsylar_add_executable(test_scheduler "tests/test_scheduler.cc" rsylar "${LIBS}")
rsylar_add_executable(test_scheduler1 "tests/test_scheduler1.cc" rsylar "${LIBS}")
rsylar_add_executable(test_thread "tests/test_thread.cc" rsylar "${LIBS}")


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
