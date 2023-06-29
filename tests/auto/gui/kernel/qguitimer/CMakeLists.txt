# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguitimer Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qguitimer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qguitimer
    SOURCES
        ../../../corelib/kernel/qtimer/tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)

if(QT_FEATURE_glib AND UNIX)
    # only exists as a different dispatcher for XCB
    qt_internal_add_test(tst_qguitimer_no_glib
        SOURCES
            ../../../corelib/kernel/qtimer/tst_qtimer.cpp
        DEFINES
            DISABLE_GLIB
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
    )
endif()
