# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qeventdispatcher Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qeventdispatcher LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(test_names "tst_qeventdispatcher")
if(QT_FEATURE_glib AND UNIX)
    list(APPEND test_names "tst_qeventdispatcher_no_glib")
endif()

foreach(test ${test_names})
    qt_internal_add_test(${test}
        NO_BATCH
        DEFINES
            QTEST_THROW_ON_FAIL
            QTEST_THROW_ON_SKIP
        SOURCES
            tst_qeventdispatcher.cpp
    )
endforeach()

if (TARGET tst_qeventdispatcher_no_glib)
    qt_internal_extend_target(tst_qeventdispatcher_no_glib
        DEFINES
            DISABLE_GLIB
            tst_QEventDispatcher=tst_QEventDispatcher_no_glib
    )
endif()
