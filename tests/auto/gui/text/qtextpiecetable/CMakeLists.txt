# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtextpiecetable LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(WIN32)
    return()
endif()
if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qtextpiecetable Test:
#####################################################################

qt_internal_add_test(tst_qtextpiecetable
    SOURCES
        ../qtextdocument/common.h
        tst_qtextpiecetable.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
