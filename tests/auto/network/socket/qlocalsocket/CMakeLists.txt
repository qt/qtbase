# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qlocalsocket LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(QT_FEATURE_process)
    add_subdirectory(socketprocess)
endif()

qt_internal_add_test(tst_qlocalsocket
    SOURCES
        tst_qlocalsocket.cpp
    DEFINES
        QLOCALSERVER_DEBUG
        QLOCALSOCKET_DEBUG
    LIBRARIES
        Qt::Network
        Qt::TestPrivate
)
if(QT_FEATURE_process)
    add_dependencies(tst_qlocalsocket socketprocess)
endif()
