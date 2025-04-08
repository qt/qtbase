# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_android Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_android LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_android
    SOURCES
        tst_android.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

if(ANDROID)
    set_property(TARGET tst_android APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
    )
endif()

qt_internal_extend_target(tst_android CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
