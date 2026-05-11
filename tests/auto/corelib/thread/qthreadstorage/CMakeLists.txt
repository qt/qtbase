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

if(QT_FEATURE_process AND NOT ANDROID AND NOT OHOS)
    add_subdirectory(crashonexit)
    add_dependencies(tst_qthreadstorage crashOnExit_helper)

    qt_internal_add_executable(tst_qthreadstorage_warnonexit
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qthreadstorage/crashOnExit_helper"
        SOURCES
            warnonexit.cpp
    )
    add_dependencies(tst_qthreadstorage tst_qthreadstorage_warnonexit)
endif()
