# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_androidnativesurfaces LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_androidnativesurfaces
    SOURCES
        tst_androidnativesurfaces.cpp
    LIBRARIES
        Qt::Gui
)

set_property(TARGET tst_androidnativesurfaces APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/testdata
)

if(QT_FEATURE_egl)
    qt_find_package(EGL PROVIDED_TARGETS EGL::EGL)
    qt_internal_extend_target(tst_androidnativesurfaces LIBRARIES EGL::EGL)
endif()
