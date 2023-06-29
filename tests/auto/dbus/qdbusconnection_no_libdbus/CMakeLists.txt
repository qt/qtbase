# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusconnection_no_libdbus Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdbusconnection_no_libdbus LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qdbusconnection_no_libdbus
    SOURCES
        ../qdbusconnection_no_bus/tst_qdbusconnection_no_bus.cpp
    DEFINES
        SIMULATE_LOAD_FAIL
        tst_QDBusConnectionNoBus=tst_QDBusConnectionNoLibDBus1
    LIBRARIES
        Qt::DBus
)
