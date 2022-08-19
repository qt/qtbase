# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_add_resources_delayed_file)

find_package(Qt6Core REQUIRED)

qt_add_resources(rcc_files "${CMAKE_CURRENT_BINARY_DIR}/test_add_resources_delayed_file.qrc")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_add_resources_delayed_file.qrc" "<!DOCTYPE RCC><RCC version=\"1.0\">
<qresource prefix=\"/\">
    <file>${CMAKE_CURRENT_SOURCE_DIR}/resource_file.txt</file>
</qresource>
</RCC>
")

add_executable(test_add_resources_delayed_file main.cpp ${rcc_files})
target_link_libraries(test_add_resources_delayed_file PRIVATE Qt::Core)
