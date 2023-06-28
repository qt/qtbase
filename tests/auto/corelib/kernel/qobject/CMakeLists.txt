# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qobject Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qobject LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qobject
    SOURCES
        tst_qobject.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::TestPrivate
)

## Scopes:
#####################################################################
add_subdirectory(signalbug)
add_dependencies(tst_qobject signalbug_helper)
