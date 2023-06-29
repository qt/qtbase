# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsplitter Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsplitter LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "extradata.txt")
list(APPEND test_data "setSizes3.dat")

qt_internal_add_test(tst_qsplitter
    SOURCES
        tst_qsplitter.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)
