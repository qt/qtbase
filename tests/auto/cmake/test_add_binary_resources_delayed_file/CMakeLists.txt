# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_add_binary_resources_delayed_file)

find_package(Qt6Core REQUIRED)

qt_add_binary_resources(rcc_file "${CMAKE_CURRENT_BINARY_DIR}/test_add_binary_resources_delayed_file.qrc" "${CMAKE_CURRENT_SOURCE_DIR}/existing.qrc")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_add_binary_resources_delayed_file.qrc" "<!DOCTYPE RCC><RCC version=\"1.0\">
<qresource prefix=\"/\">
    <file alias=\"resource_file.txt\">${CMAKE_CURRENT_SOURCE_DIR}/resource_file.txt</file>
</qresource>
</RCC>
")

add_executable(test_add_binary_resources_delayed_file main.cpp)
target_link_libraries(test_add_binary_resources_delayed_file PRIVATE Qt::Core)
