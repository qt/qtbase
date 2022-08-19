# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_plugin_flavor)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt6Core REQUIRED HINTS ${Qt6Tests_PREFIX_PATH})

qt6_add_plugin(test_plugin ${PLUGIN_OPTIONS})

set(plugin_source_path "${CMAKE_CURRENT_BINARY_DIR}/plugin.cpp")
file(GENERATE OUTPUT "${plugin_source_path}" CONTENT "int foo() {return 0;}")
target_sources(test_plugin PRIVATE "${plugin_source_path}")

get_target_property(plugin_target_type test_plugin TYPE)

if(NOT EXPECTED_PLUGIN_TARGET_TYPE)
    message(FATAL_ERROR "No value given for EXPECTED_PLUGIN_TARGET_TYPE variable")
endif()

if(NOT plugin_target_type STREQUAL "${EXPECTED_PLUGIN_TARGET_TYPE}")
    set(info "")
    list(APPEND info "PLUGIN_OPTIONS: ${PLUGIN_OPTIONS}")
    list(APPEND info "QT6_IS_SHARED_LIBS_BUILD: ${QT6_IS_SHARED_LIBS_BUILD}")
    list(APPEND info "EXPECTED_PLUGIN_TARGET_TYPE: ${EXPECTED_PLUGIN_TARGET_TYPE}")
    list(JOIN info "\n" info)
    message(FATAL_ERROR
            "Computed plugin target type '${plugin_target_type}' did not match expected type '${EXPECTED_PLUGIN_TARGET_TYPE}'\n${info}")
endif()
