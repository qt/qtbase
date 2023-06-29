# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcompleter Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qcompleter LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qcompleter
    SOURCES
        tst_qcompleter.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
