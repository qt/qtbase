# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_harmonywindow_defaultability Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_harmonywindow_defaultability LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_harmonywindow_defaultability
    SOURCES
        tst_harmonywindow_defaultability.cpp
    LIBRARIES
        Qt::Gui
        ace_napi.z
)
