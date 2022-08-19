# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_json_plugin_includes)

find_package(Qt6Core REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/jsontestplugin.json" "{}\n")

qt_wrap_cpp(moc_files plugin.h)

add_library(plugin plugin.cpp ${moc_files})
target_link_libraries(plugin Qt::Core)
