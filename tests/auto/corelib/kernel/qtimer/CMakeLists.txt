# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtimer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if (NOT QT_FEATURE_thread)
    return()
endif()

qt_internal_add_test(tst_qtimer
    SOURCES
        tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)

if(QT_FEATURE_glib AND UNIX)
    qt_internal_add_test(tst_qtimer_no_glib
        SOURCES
            tst_qtimer.cpp
        DEFINES
            DISABLE_GLIB
        LIBRARIES
            Qt::CorePrivate
            Qt::TestPrivate
    )
endif()
