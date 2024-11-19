# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qimageiohandler Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qimageiohandler LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)

list(APPEND test_data ${test_data_glob})

add_subdirectory(plugin)

qt_internal_add_test(tst_qimageiohandler
    SOURCES
        tst_qimageiohandler.cpp
        pluginlog.h
    LIBRARIES
        Qt::Gui
        TestImagePlugin
    TESTDATA ${test_data}
)

add_dependencies(tst_qimageiohandler TestImagePlugin)
