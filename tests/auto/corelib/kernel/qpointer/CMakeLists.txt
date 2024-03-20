# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpointer Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpointer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if (NOT QT_FEATURE_thread)
    return()
endif()

qt_internal_add_test(tst_qpointer
    SOURCES
        tst_qpointer.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpointer CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
