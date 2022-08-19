# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_add_resources_binary_generated)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt6Core REQUIRED HINTS ${Qt6Tests_PREFIX_PATH})

qt6_add_executable(test_add_resources_binary_generated main.cpp)

qt6_add_resources(test_add_resources_binary_generated resources1 FILES resource1.txt PREFIX "/"
                  OPTIONS "--binary")
qt6_add_resources(test_add_resources_binary_generated resources2 FILES resource2.txt PREFIX "/"
                  OPTIONS "--binary"
                  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/resources2_custom.rcc")

target_compile_definitions(test_add_resources_binary_generated
                  PRIVATE
                      RESOURCE1_FULL_PATH="${CMAKE_CURRENT_BINARY_DIR}/resources1.rcc")
target_compile_definitions(test_add_resources_binary_generated
                  PRIVATE
                      RESOURCE2_FULL_PATH="${CMAKE_CURRENT_BINARY_DIR}/resources2_custom.rcc")

if(NOT TARGET binary_resource_resources1)
    message(FATAL_ERROR "Generated resources binary target was not created")
endif()

if(NOT TARGET binary_resource_resources2)
    message(FATAL_ERROR "Generated resources binary target was not created")
endif()
