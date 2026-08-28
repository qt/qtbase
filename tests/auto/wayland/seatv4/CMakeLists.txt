# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_seatv4 LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from seatv4.pro.

#####################################################################
## tst_seatv4 Test:
#####################################################################

qt_internal_add_test(tst_seatv4
    SOURCES
        tst_seatv4.cpp
    LIBRARIES
        SharedClientTest
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_seatv4 CONDITION QT_FEATURE_cursor
    LIBRARIES
        Qt::GuiPrivate
        Wayland::Cursor
)
