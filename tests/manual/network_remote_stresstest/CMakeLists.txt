# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_network_remote_stresstest LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_network_remote_stresstest Binary:
#####################################################################

qt_internal_add_manual_test(tst_network_remote_stresstest
    SOURCES
        tst_network_remote_stresstest.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::Test
)

# Resources:
set(url-list_resource_files
    "url-list.txt"
)

qt_internal_add_resource(tst_network_remote_stresstest "url-list"
    PREFIX
        "/"
    FILES
        ${url-list_resource_files}
)

