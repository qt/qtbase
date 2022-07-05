# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

cmake_minimum_required(VERSION 3.16)

project(test_interface_try_compile)

find_package(Qt6Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(test_interface_try_compile_exe main.cpp)
target_link_libraries(test_interface_try_compile_exe Qt::Widgets)

