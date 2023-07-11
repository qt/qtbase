# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qchronotimer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if (NOT QT_FEATURE_thread)
    return()
endif()

function(addChronoTimerTest test)
    qt_internal_add_test(${test}
        SOURCES
            tst_qchronotimer.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::TestPrivate
    )
endfunction()

addChronoTimerTest(tst_qchronotimer)

if(QT_FEATURE_glib AND UNIX)
    addChronoTimerTest(tst_qchronotimer_no_glib)
    qt_internal_extend_target(tst_qchronotimer_no_glib
        DEFINES
            DISABLE_GLIB
            tst_QChronoTimer=tst_QChronoTimer_no_glib # Class name in the unittest
    )
endif()
