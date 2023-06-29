# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtextimagehandler LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

list(APPEND test_data "data/image.png")
list(APPEND test_data "data/image@2x.png")

qt_internal_add_test(tst_qtextimagehandler
    SOURCES
    tst_qtextimagehandler.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA
        ${test_data}
)

qt_internal_add_resource(tst_qtextimagehandler "qtextimagehandler"
    PREFIX
        "/"
    FILES
        ${test_data}
)
