# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


cmake_minimum_required(VERSION 3.16)

project(test_private_includes)

find_package(Qt6Gui REQUIRED)

add_executable(testapp main.cpp)
target_link_libraries(testapp PRIVATE Qt::GuiPrivate)
