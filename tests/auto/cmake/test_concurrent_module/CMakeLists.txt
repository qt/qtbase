# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


cmake_minimum_required(VERSION 3.16)

project(test_concurrent_module)

find_package(Qt6Concurrent 6.0.0 REQUIRED)

add_executable(mainapp main.cpp)

target_link_libraries(mainapp PRIVATE Qt::Concurrent)
