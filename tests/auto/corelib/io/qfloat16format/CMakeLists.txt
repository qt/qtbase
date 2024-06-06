# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfloat16format LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET 13.3)

qt_internal_add_test(tst_qfloat16format
    SOURCES
        tst_qfloat16format.cpp
)

if(NOT VXWORKS)
    set_target_properties(tst_qfloat16format
        PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED OFF
    )
endif()
