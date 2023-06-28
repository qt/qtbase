# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsocketnotifier LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsocketnotifier Test:
#####################################################################

qt_internal_add_test(tst_qsocketnotifier
    SOURCES
        tst_qsocketnotifier.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsocketnotifier CONDITION WIN32
    LIBRARIES
        ws2_32
)
