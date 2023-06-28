# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qthreadstorage Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qthreadstorage LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qthreadstorage
    SOURCES
        tst_qthreadstorage.cpp
)

## Scopes:
#####################################################################

if(NOT ANDROID)
    add_subdirectory(crashonexit)
    if(QT_FEATURE_process)
        add_dependencies(tst_qthreadstorage crashOnExit_helper)
    endif()
endif()
