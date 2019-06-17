cmake_minimum_required(VERSION 3.0.0)
project(json-test VERSION 0.0.0)

option(X86_64 "编译x86_64架构程序" ON)

set(CMAKE_C_COMPILER arm-linux-gcc)
set(CMAKE_CXX_COMPILER arm-linux-g++)

if(X86_64)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
endif()

include(CTest)
enable_testing()

include_directories(inc)

add_executable(Midi2Json main.cpp src/json.cpp src/ws_log.cpp src/midi.c )

# if(NOT X86_64)
#     file( COPY ./release.sh DESTINATION ./)
# endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
