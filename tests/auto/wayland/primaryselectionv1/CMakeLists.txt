# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_primaryselectionv1 LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from primaryselectionv1.pro.

#####################################################################
## tst_primaryselectionv1 Test:
#####################################################################

qt_internal_add_test(tst_primaryselectionv1
    SOURCES
        tst_primaryselectionv1.cpp
    LIBRARIES
        SharedClientTest
)
