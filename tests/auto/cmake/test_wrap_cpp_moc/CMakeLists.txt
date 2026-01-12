# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_wrap_cpp_moc)

find_package(Qt6Core REQUIRED)

add_executable(example main.cpp)

set(moc_files "")
qt_wrap_cpp(moc_files main.cpp TARGET example)

if(NOT moc_files MATCHES "main\\.moc$")
    message(FATAL_ERROR "test_qt_wrap_cpp_moc: moc_files has unexpected content: ${moc_files}")
endif()

target_link_libraries(example PRIVATE Qt::Core)
