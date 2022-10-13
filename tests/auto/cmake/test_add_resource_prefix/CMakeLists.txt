# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_add_resource_prefix)

find_package(Qt6 REQUIRED
    COMPONENTS Core Test
)

set(CMAKE_AUTOMOC ON)

add_executable(test_add_resource_prefix main.cpp)

# Tests if "/" is being used when PREFIX is missing
qt_add_resources(test_add_resource_prefix "resources_without_prefix"
    FILES resource_file.txt)

# Tests if the PREFIX parameter is being respected
qt_add_resources(test_add_resource_prefix "resources_with_prefix"
    PREFIX "/resources"
    FILES resource_file.txt)

target_link_libraries(test_add_resource_prefix PRIVATE Qt::Core Qt::Test)
