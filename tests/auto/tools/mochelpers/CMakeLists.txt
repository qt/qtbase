# Copyright (C) 2024 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmetaenum Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_mochelpers LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_mochelpers
    SOURCES
        tst_mochelpers.cpp
)
