# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    qss/*)
list(APPEND test_data ${test_data_glob})

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_baseline_stylesheet LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_baseline_stylesheet
    SOURCES
        ../shared/baselineprotocol.cpp ../shared/baselineprotocol.h ../shared/lookup3.cpp
        ../shared/qbaselinetest.cpp ../shared/qbaselinetest.h
        ../shared/qwidgetbaselinetest.cpp ../shared/qwidgetbaselinetest.h
        tst_baseline_stylesheet.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
        Qt::Network
    TESTDATA ${test_data}
)

qt6_add_resources(tst_baseline_stylesheet "tst_baseline_stylesheet"
    PREFIX
        "/"
    FILES
        "icons/align-center.png"
        "icons/align-left.png"
        "icons/align-right.png"
        "icons/arrow-up.png"
)
