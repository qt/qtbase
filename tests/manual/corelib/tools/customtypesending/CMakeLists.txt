# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(customtypesending LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

cmake_minimum_required(VERSION 3.16)

qt_internal_add_manual_test(customtypesending
    GUI
    SOURCES
        main.cpp
        message.cpp message.h
        window.cpp window.h
    LIBRARIES
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
