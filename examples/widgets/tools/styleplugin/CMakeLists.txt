# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(styleplugin LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/styleplugin")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_subdirectory(stylewindow)
add_subdirectory(plugin)
