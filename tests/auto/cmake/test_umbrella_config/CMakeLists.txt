# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_umbrella_config)

add_subdirectory(components_found)

if (Qt6_FOUND)
    message(SEND_ERROR "Qt6_FOUND variable leaked!")
endif()

if (Qt6Core_FOUND)
    message(SEND_ERROR "Qt6Core_FOUND variable leaked!")
endif()

if (TARGET Qt6::Core)
    message(SEND_ERROR "Qt6::Core target leaked!")
endif()

add_subdirectory(components_not_found)
