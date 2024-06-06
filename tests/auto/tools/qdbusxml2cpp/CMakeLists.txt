# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusxml2cpp Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdbusxml2cpp LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qdbusxml2cpp
    SOURCES
        tst_qdbusxml2cpp.cpp
)

add_dependencies(tst_qdbusxml2cpp ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp)
