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

function(addGuiTimerTest test)
    qt_internal_add_test(${test}
        SOURCES
            ../../../corelib/kernel/qtimer/tst_qtimer.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
            Qt::TestPrivate
    )
endfunction()

addGuiTimerTest(tst_qguitimer)
qt_internal_extend_target(tst_qguitimer
    DEFINES
        tst_QTimer=tst_QGuiTimer
)

if(QT_FEATURE_glib AND UNIX)
    addGuiTimerTest(tst_qguitimer_no_glib)
    qt_internal_extend_target(tst_qguitimer_no_glib
        DEFINES
            DISABLE_GLIB
            tst_QTimer=tst_QGuiTimer_no_glib # Class name in the unittest
    )
endif()
