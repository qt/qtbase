# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwasmwindowtreenode Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qwasmwindowtreenode LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qwasmwindowtreenode
    SOURCES
        tst_qwasmwindowtreenode.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
)
