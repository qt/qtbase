# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(x86intrin LANGUAGES CXX)
add_executable(x86intrin main.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|IntelLLVM|QCC")
    target_compile_options(x86intrin PUBLIC
        "-march=cannonlake" "-mrdrnd" "-mrdseed" "-maes" "-msha" "-w")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(x86intrin PUBLIC "-arch:AVX512" "-W0")
endif()
