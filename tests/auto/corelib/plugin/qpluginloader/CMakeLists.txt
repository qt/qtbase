# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpluginloader LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

add_subdirectory(lib)
add_subdirectory(staticplugin)
add_subdirectory(theplugin)
add_subdirectory(tst)
if(UNIX AND NOT ANDROID AND NOT APPLE)
    add_subdirectory(almostplugin)
endif()
if(MACOS AND QT_FEATURE_private_tests AND TARGET Qt::Gui)
    add_subdirectory(machtest)
endif()
