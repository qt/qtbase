# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qscreen LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qscreen Binary:
#####################################################################

qt_internal_add_manual_test(qscreen
    SOURCES
        main.cpp
        propertyfield.cpp propertyfield.h
        propertywatcher.cpp propertywatcher.h
    NO_PCH_SOURCES
        main.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
