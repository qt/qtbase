# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_clientextension LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_clientextension
    SOURCES
        tst_clientextension.cpp
    LIBRARIES
        SharedClientTest
)

qt6_generate_wayland_protocol_client_sources(tst_clientextension
    PRIVATE_CODE
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/test.xml
)

qt6_generate_wayland_protocol_server_sources(tst_clientextension
    PRIVATE_CODE
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/test.xml
)
