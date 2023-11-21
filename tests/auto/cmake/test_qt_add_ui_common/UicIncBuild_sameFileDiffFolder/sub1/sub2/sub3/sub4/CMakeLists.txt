# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(UicIncrementalBuild_sameFileDifferentFolder LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

set(CMAKE_AUTOMOC ON)

qt_add_executable(example
    ../../../../src/ui_files/mainwindow.ui
    ../../../../src/ui_files/widget1.ui
    ../../../../widget1.ui
    ../../../../src/mainwindow.h
    ../../../../src/widget1.h
    ../../../../src/widget2.h
    ../../../../src/main.cpp
    ../../../../src/mainwindow.cpp
    ../../../../src/widget1.cpp
    ../../../../src/widget2.cpp
)

target_link_libraries(example PRIVATE Qt6::Widgets
                                      Qt6::Core
                                      Qt6::Gui)

qt6_add_ui(example
    INCLUDE_PREFIX "../../../../src/ui_files"
    SOURCES "../../../../src/ui_files/mainwindow.ui"
            "../../../../src/ui_files/widget1.ui"
    OPTIONS "$<$<CONFIG:Debug>:-a>")

qt6_add_ui(example
    INCLUDE_PREFIX "../../../../"
    SOURCES "../../../../widget1.ui"
    OPTIONS "$<$<CONFIG:Debug>:-a>")

