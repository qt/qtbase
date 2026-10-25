# Copyright (C) 2026 Reza Bakhshi Laktasaraei
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qaccessibilitywin LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT WIN32 OR NOT QT_FEATURE_accessibility)
    return()
endif()

qt_internal_add_test(tst_qaccessibilitywin
    SOURCES
        tst_qaccessibilitywin.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Widgets
        ole32
        oleaut32
        uuid
)
