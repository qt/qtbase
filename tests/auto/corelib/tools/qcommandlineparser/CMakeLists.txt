# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcommandlineparser Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qcommandlineparser LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qcommandlineparser
    SOURCES
        tst_qcommandlineparser.cpp
)
add_subdirectory(testhelper)
if(QT_FEATURE_process AND NOT ANDROID)
    add_dependencies(tst_qcommandlineparser qcommandlineparser_test_helper)
endif()
