# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_datadevicev1 LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from datadevicev1.pro.

#####################################################################
## tst_datadevicev1 Test:
#####################################################################

qt_internal_add_test(tst_datadevicev1
    SOURCES
        tst_datadevicev1.cpp
    LIBRARIES
        SharedClientTest
)
