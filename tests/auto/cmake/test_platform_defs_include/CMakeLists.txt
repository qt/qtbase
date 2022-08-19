# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_platform_defs_include)

find_package(Qt6Core REQUIRED)

add_executable(myobject main.cpp)
target_link_libraries(myobject PRIVATE Qt6::Core)
