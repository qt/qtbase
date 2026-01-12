# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.22)

project(test_wrap_cpp_target_mixed_files)

find_package(Qt6Core REQUIRED)

add_executable(example
    main.cpp
    myobject1.cpp
    myobject2.cpp
)
target_link_libraries(example PRIVATE Qt::Core)

# Test multiple headers AND sources in single call
qt_wrap_cpp(example
    myobject1.h
    myobject2.h
    main.cpp
)
