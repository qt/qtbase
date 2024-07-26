# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_wrap_cpp_moc)

find_package(Qt6Core REQUIRED)

add_executable(example main.cpp)

qt_wrap_cpp(moc_files main.cpp TARGET example)

# expect the generated moc files to be empty when a source file is passed
if (NOT moc_files STREQUAL "")
    message(FATAL_ERROR "test_qt_wrap_cpp_moc: moc_files should be empty")
endif()

target_link_libraries(example PRIVATE Qt::Core)
