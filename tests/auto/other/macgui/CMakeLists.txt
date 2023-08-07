# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_macgui LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT APPLE)
    return()
endif()
if(NOT TARGET Qt::Widgets)
    return()
endif()

#####################################################################
## tst_macgui Test:
#####################################################################

qt_internal_add_test(tst_macgui
    SOURCES
        guitest.cpp guitest.h
        tst_macgui.cpp
    NO_PCH_SOURCES
        guitest.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_macgui CONDITION MACOS
    LIBRARIES
        ${FWApplicationServices}
)
