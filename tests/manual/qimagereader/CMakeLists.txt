# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qimagereader LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qimagereader Binary:
#####################################################################

qt_internal_add_manual_test(qimagereader
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
set(qimagereader_resource_files
    "Qt_logostrap_CMYK.jpg"
)
qt_add_resources(qimagereader "qimagereader"
    PREFIX
        "/"
    FILES
        ${qimagereader_resource_files}
)
