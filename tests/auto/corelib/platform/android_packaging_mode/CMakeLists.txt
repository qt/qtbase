# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_android_packaging_mode LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_android_packaging_mode_legacy
    SOURCES
        tst_android_packaging_mode.cpp
    DEFINES
        QT_TEST_EXPECT_LEGACY_PACKAGING
    LIBRARIES
        Qt::CorePrivate
)

set_property(TARGET tst_android_packaging_mode_legacy PROPERTY QT_ANDROID_LEGACY_PACKAGING ON)

if(NOT QT_FEATURE_sanitize_address)
    qt_internal_add_test(tst_android_packaging_mode_modern
        SOURCES
            tst_android_packaging_mode.cpp
        LIBRARIES
            Qt::CorePrivate
    )
endif()
