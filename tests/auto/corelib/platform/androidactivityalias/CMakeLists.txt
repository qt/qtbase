# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_androidactivityalias LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Custom manifests still not handled fully by modern bundle builds
set(QT_USE_ANDROID_MODERN_BUNDLE OFF)

qt_internal_add_test(tst_androidactivityalias
    SOURCES
        tst_androidactivityalias.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Core
        Qt::Gui
)

if(ANDROID)
    set_property(TARGET tst_androidactivityalias
        PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()
