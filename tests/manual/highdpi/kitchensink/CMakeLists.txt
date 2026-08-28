# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(kitchensink LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## kitchensink Binary:
#####################################################################

qt_internal_add_manual_test(kitchensink
    SOURCES
        dragwidget.cpp dragwidget.h
        main.cpp
    DEFINES
        HAVE_SCREEN_BASE_DPI
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

# Resources:
set(kitchensink_resource_files
    "qticon16.png"
    "qticon16@2x.png"
    "qticon16@3x.png"
    "qticon32.png"
    "qticon32@2x.png"
    "qticon64.png"
)

qt_internal_add_resource(kitchensink "kitchensink"
    PREFIX
        "/"
    FILES
        ${kitchensink_resource_files}
)
