# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(armintrin LANGUAGES CXX)

include(../../cmake/QtPlatformSupport.cmake)
include(../../cmake/QtCompilerOptimization.cmake)

string(TOUPPER "${SIMD}" upper_simd)

if(NOT DEFINED "QT_CFLAGS_ARM_${upper_simd}")
    # Don't use CMake error() because a configure error also fails the try_compile() call.
    # Instead use a compile flag that doesn't exist to force a compiler error.
    set(QT_CFLAGS_${upper_simd} "--qt-cflags-not-found")
endif()

add_executable("SimdTest${SIMD}")
target_sources("SimdTest${SIMD}" PRIVATE main.cpp)
target_compile_options("SimdTest${SIMD}" PRIVATE ${QT_CFLAGS_ARM_${upper_simd}})
target_compile_definitions("SimdTest${SIMD}" PRIVATE QT_COMPILER_SUPPORTS_${upper_simd})
