# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_nooutput LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from nooutput.pro.

#####################################################################
## tst_nooutput Test:
#####################################################################

qt_internal_add_test(tst_nooutput
    SOURCES
        tst_nooutput.cpp
    LIBRARIES
        SharedClientTest
)
