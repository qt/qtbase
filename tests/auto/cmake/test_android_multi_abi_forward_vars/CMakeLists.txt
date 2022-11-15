# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_android_multi_abi_forward_vars)

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

if(NOT TEST_SINGLE_VALUE_ARG STREQUAL "TestValue")
    message(FATAL_ERROR
        "TEST_SINGLE_VALUE_ARG is not valid: ${TEST_SINGLE_VALUE_ARG}")
endif()

if(NOT TEST_LIST_VALUE_ARG STREQUAL "TestValue;TestValue2;TestValue3")
    message(FATAL_ERROR
        "TEST_LIST_VALUE_ARG is not valid: ${TEST_LIST_VALUE_ARG}")
endif()

if(NOT TEST_ESCAPING_VALUE_ARG STREQUAL "TestValue\;TestValue2\;TestValue3")
    message(FATAL_ERROR
        "TEST_ESCAPING_VALUE_ARG is not valid: ${TEST_ESCAPING_VALUE_ARG}")
endif()

if(NOT TEST_SPACES_VALUE_ARG STREQUAL "TestValue TestValue2 TestValue3")
    message(FATAL_ERROR
        "TEST_SPACES_VALUE_ARG is not valid: ${TEST_SPACES_VALUE_ARG}")
endif()

# Make sure that ABI external projects are created
qt6_add_executable(test_executable main.cpp)

target_link_libraries(test_executable PRIVATE Qt::Core Qt::Gui)
