# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguichronotimer Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qguichronotimer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

function(addGuiChronoTimerTest test)
    qt_internal_add_test(${test}
        SOURCES
            ../../../corelib/kernel/qchronotimer/tst_qchronotimer.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
            Qt::TestPrivate
    )
endfunction()

addGuiChronoTimerTest(tst_qguichronotimer)
qt_internal_extend_target(tst_qguichronotimer
    DEFINES
        tst_QChronoTimer=tst_QGuiChronoTimer
)

if(QT_FEATURE_glib AND UNIX)
    addGuiChronoTimerTest(tst_qguichronotimer_no_glib)
    qt_internal_extend_target(tst_qguichronotimer_no_glib
        DEFINES
            DISABLE_GLIB
            tst_QChronoTimer=tst_QGuiChronoTimer_no_glib # Class name in the unittest
    )
endif()
