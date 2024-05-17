# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

message("Test default creating of versionless targets")
find_package(Qt6Core REQUIRED)

if(NOT TARGET Qt6::Core)
    message(SEND_ERROR "Qt6::Core target not defined!")
endif()

if(NOT TARGET Qt::Core)
    message(SEND_ERROR "Qt::Core target not defined!")
endif()

check_versionless_targets()
