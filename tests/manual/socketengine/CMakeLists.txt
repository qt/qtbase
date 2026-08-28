# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_socketengine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_socketengine Binary:
#####################################################################

if (NOT QT_FEATURE_private_tests)
    return()
endif()

qt_internal_add_manual_test(tst_socketengine
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::Test
)
