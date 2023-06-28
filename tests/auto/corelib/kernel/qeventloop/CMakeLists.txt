# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qeventloop Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qeventloop LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qeventloop
    SOURCES
        tst_qeventloop.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qeventloop CONDITION WIN32
    LIBRARIES
        user32
)

qt_internal_extend_target(tst_qeventloop CONDITION QT_FEATURE_glib
    DEFINES
        HAVE_GLIB
)
