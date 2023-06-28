# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpermission LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if (NOT QT_FEATURE_permissions)
    return()
endif()

qt_internal_add_test(tst_qpermission
    SOURCES
        tst_qpermission.cpp
    LIBRARIES
        Qt::Core
)
