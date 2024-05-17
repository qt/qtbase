# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(remotecontrolledcar LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Widgets)

qt_standard_project_setup()

add_subdirectory(car)
add_subdirectory(controller)
