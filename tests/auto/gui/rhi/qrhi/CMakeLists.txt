# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qrhi Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qrhi LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
file(GLOB_RECURSE qrhi_resource_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    data/*
)

qt_internal_add_test(tst_qrhi
    SOURCES
        tst_qrhi.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qrhi_resource_files}
    BUILTIN_TESTDATA
)
