# Copyright (C) 2026 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfilesystemengine Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfilesystemengine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qfilesystemengine
    SOURCES
        tst_qfilesystemengine.cpp
    LIBRARIES
        Qt::CorePrivate
)
