# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qtexttableborders LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qtexttableborders Binary:
#####################################################################

qt_internal_add_manual_test(qtexttableborders
    GUI
    SOURCES
        main.cpp
        widget.cpp widget.h widget.ui
    LIBRARIES
        Qt::Gui
        Qt::PrintSupport
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
set(resources_resource_files
    "table-border-test-header.html"
    "table-border-test.html"
)

qt_internal_add_resource(qtexttableborders "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)
