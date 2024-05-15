# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qlibraryinfo LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()


qt_internal_add_test(tst_qlibraryinfo SOURCES tst_qlibraryinfo.cpp
    LIBRARIES
        Qt::CorePrivate

)

qt_add_resources(tst_qlibraryinfo "qtconffiles"
    PREFIX "/"
    FILES
        empty.qt.conf
        partial.qt.conf
        list.qt.conf
        merge.qt.conf
)
