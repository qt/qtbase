# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_network_stresstest LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_network_stresstest Binary:
#####################################################################

qt_internal_add_manual_test(tst_network_stresstest
    SOURCES
        minihttpserver.cpp minihttpserver.h
        tst_network_stresstest.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::Test
)

# Resources:
set(wwwfiles_resource_files
    "qtest"
)

qt_internal_add_resource(tst_network_stresstest "wwwfiles"
    PREFIX
        "/"
    FILES
        ${wwwfiles_resource_files}
)
