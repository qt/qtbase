# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qnetworkinformation LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(qnetworkinformation
    SOURCES
        tst_qnetworkinformation.cpp
    LIBRARIES
        Qt::Network
        Qt::Test
)

qt_internal_extend_target(qnetworkinformation CONDITION ANDROID OR UIKIT
    SOURCES
        mainwindow.h
    LIBRARIES
        Qt::Widgets
    DEFINES
        MOBILE)
