# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdatastream Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdatastream LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "datastream.q42")
list(APPEND test_data "typedef.q5")

qt_internal_add_test(tst_qdatastream
    SOURCES
        tst_qdatastream.cpp
    LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)
