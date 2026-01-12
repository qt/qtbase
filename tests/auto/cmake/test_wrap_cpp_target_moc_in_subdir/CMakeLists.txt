# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.22)

project(test_wrap_cpp_target_moc_in_subdir)

find_package(Qt6Core REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE Qt::Core)
add_subdirectory(subdir)
