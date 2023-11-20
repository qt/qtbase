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

function(addTimerTest test)
    qt_internal_add_test(${test}
        SOURCES
            tst_qtimer.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::TestPrivate
    )
endfunction()

addTimerTest(tst_qtimer)

if(QT_FEATURE_glib AND UNIX)
    addTimerTest(tst_qtimer_no_glib)
    qt_internal_extend_target(tst_qtimer_no_glib
        DEFINES
            DISABLE_GLIB
            tst_QTimer=tst_QTimer_no_glib # Class name in the unittest
    )
endif()

