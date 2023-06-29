# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmake LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmake
    SOURCES
        testcompiler.cpp testcompiler.h
        tst_qmake.cpp
    TESTDATA ${test_data}
)

set(dependencies
    Qt::moc
    Qt::qmake
)
if(TARGET Qt::Gui)
    list(APPEND dependencies Qt::Gui)
endif()
if(TARGET Qt::Widgets)
    list(APPEND dependencies Qt::Widgets)
endif()
if(TARGET Qt::rcc)
    list(APPEND dependencies Qt::rcc)
endif()
if(TARGET Qt::uic)
    list(APPEND dependencies Qt::uic)
endif()
# Add dependencies that are implicitly used inside the test
add_dependencies(tst_qmake ${dependencies})
