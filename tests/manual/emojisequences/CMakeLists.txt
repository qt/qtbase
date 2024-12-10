# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(emojisequences LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)

qt_standard_project_setup()

qt_internal_add_manual_test(emojisequences
    GUI
    SOURCES
        main.cpp
        mainwindow.h mainwindow.cpp
        mainwindow.ui
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

set(emojisequences_resource_files
    "emoji-test.txt"
)

qt_internal_add_resource(emojisequences "emojisequences"
    PREFIX
        "/"
    FILES
        ${emojisequences_resource_files}
)
