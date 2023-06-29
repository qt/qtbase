# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwizard Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qwizard LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set(qwizard_resource_files
    "images/background.png"
    "images/banner.png"
    "images/logo.png"
    "images/watermark.png"
)

qt_internal_add_test(tst_qwizard
    SOURCES
        tst_qwizard.cpp
        tst_qwizard_2.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${qwizard_resource_files}
    BUILTIN_TESTDATA
)

