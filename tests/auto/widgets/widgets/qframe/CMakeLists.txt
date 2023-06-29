# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qframe Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qframe LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qframe
    LOWDPI
    SOURCES
        tst_qframe.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)
