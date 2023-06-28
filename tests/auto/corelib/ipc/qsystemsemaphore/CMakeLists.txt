# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsystemsemaphore Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsystemsemaphore LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qsystemsemaphore
    SOURCES
        tst_qsystemsemaphore.cpp
)

add_subdirectory(acquirerelease)
if(QT_FEATURE_process)
    add_dependencies(tst_qsystemsemaphore acquirerelease_helper)
endif()
