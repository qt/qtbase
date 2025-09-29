# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qapplicationstatic_static_qcoreapplication LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

get_target_property(target_type Qt6::Core TYPE)
if (NOT WIN32 OR NOT target_type STREQUAL STATIC_LIBRARY)
    qt_internal_add_test(tst_qapplicationstatic_static_qcoreapplication
        SOURCES
            tst_qapplicationstatic_static_qcoreapplication.cpp
    )
endif()
