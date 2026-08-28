# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(clipboard LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## clipboard Binary:
#####################################################################

qt_internal_add_manual_test(clipboard
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
    NO_PCH_SOURCES
        main.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
# Resources:
set(data_resource_files
    "data/qticon64.png"
)

qt_internal_add_resource(clipboard "data"
    PREFIX
        "/"
    FILES
        ${data_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(clipboard CONDITION (QT_MAJOR_VERSION GREATER 4)
    LIBRARIES
        Qt::Widgets
)
