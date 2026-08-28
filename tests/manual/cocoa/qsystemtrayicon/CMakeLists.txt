# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qsystemtrayicon LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qsystemtrayicon Binary:
#####################################################################

qt_internal_add_manual_test(qsystemtrayicon
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

# Resources:
set(icons_resource_files
    "macsystray16x16.png"
    "macsystray18x18.png"
    "macsystray25x15.png"
    "macsystray32x32.png"
    "macsystray36x36.png"
    "macsystray50x30.png"
    "macsystray64x64.png"
)

qt_internal_add_resource(qsystemtrayicon "icons"
    PREFIX
        "/"
    FILES
        ${icons_resource_files}
)
