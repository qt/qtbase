# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(settingseditor LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_internal_add_manual_test(settingseditor
    SOURCES
        locationdialog.cpp locationdialog.h
        main.cpp
        mainwindow.cpp mainwindow.h
        settingstree.cpp settingstree.h
        variantdelegate.cpp variantdelegate.h
    LIBRARIES
        Qt::Widgets
)
