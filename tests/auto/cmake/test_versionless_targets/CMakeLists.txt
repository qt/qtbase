# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(versionless_targets)

set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

find_package(Qt6Core REQUIRED)

if (NOT TARGET Qt6::Core)
    message(SEND_ERROR "Qt6::Core target not defined!")
endif()

if (TARGET Qt::Core)
    message(SEND_ERROR "Qt::Core target defined despite QT_NO_CREATE_VERSIONLESS_TARGETS!")
endif()

set(QT_NO_CREATE_VERSIONLESS_TARGETS OFF)

find_package(Qt6Core REQUIRED)

if (NOT TARGET Qt::Core)
    message(SEND_ERROR "Qt::Core target not defined!")
endif()
