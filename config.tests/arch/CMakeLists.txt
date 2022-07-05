# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

cmake_minimum_required(VERSION 3.16)
project(arch LANGUAGES CXX)

add_executable(architecture_test)
set_property(TARGET architecture_test PROPERTY MACOSX_BUNDLE FALSE)
target_sources(architecture_test PRIVATE arch.cpp)

if(EMSCRIPTEN)
    target_compile_options(architecture_test PRIVATE -O2 -msimd128 -msse -msse2)
endif()
