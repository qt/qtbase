# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qget LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qget Binary:
#####################################################################

qt_internal_add_manual_test(qget
    SOURCES
        downloadmanager.cpp
        qget.cpp qget.h
        transferitem.cpp
    NO_PCH_SOURCES
        downloadmanager.cpp # undef QT_NO_FOREACH
        qget.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Network
)
