# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsharedmemory LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qsharedmemory
    SOURCES
        tst_qsharedmemory.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsharedmemory CONDITION LINUX
    LIBRARIES
        rt
)
add_subdirectory(producerconsumer)
if(QT_FEATURE_process)
    add_dependencies(tst_qsharedmemory producerconsumer_helper)
endif()
