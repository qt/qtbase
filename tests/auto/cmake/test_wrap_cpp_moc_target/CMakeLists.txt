# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_wrap_cpp_moc_target)

find_package(Qt6Core REQUIRED)

add_executable(example main.cpp)

qt_wrap_cpp(example main.cpp)

target_link_libraries(example PRIVATE Qt::Core)
