# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusconnection Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdbusconnection LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qdbusconnection
    SOURCES
        tst_qdbusconnection.cpp tst_qdbusconnection.h
    LIBRARIES
        Qt::DBus
    TESTDATA
        tst_qdbusconnection.conf
)

qt_internal_add_executable(qdbusdelayeddeliveryreenablehelper
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        qdbusdelayeddeliveryreenablehelper.cpp
    LIBRARIES
        Qt::DBus
)

add_dependencies(tst_qdbusconnection
    qdbusdelayeddeliveryreenablehelper
)
