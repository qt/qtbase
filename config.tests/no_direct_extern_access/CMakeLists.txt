# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16)
project(direct_extern_access LANGUAGES CXX)

# this is the test found in https://sourceware.org/bugzilla/show_bug.cgi?id=29087
# and https://sourceware.org/bugzilla/show_bug.cgi?id=29377

add_library(no_extern_access_lib SHARED lib.cpp)
add_executable(no_extern_access_main other.cpp main.cpp)

target_compile_options(no_extern_access_lib PRIVATE "-Werror")
target_compile_options(no_extern_access_lib PUBLIC "$<$<CXX_COMPILER_ID:GNU>:-mno-direct-extern-access>")
target_compile_options(no_extern_access_lib PUBLIC "$<$<CXX_COMPILER_ID:Clang>:-fno-direct-access-external-data>")

target_link_libraries(no_extern_access_main no_extern_access_lib)
