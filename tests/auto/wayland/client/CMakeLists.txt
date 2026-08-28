# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_client LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from client.pro.

#####################################################################
## tst_client Test:
#####################################################################

qt_internal_add_test(tst_client
    SOURCES
        tst_client.cpp
    LIBRARIES
        SharedClientTest
)

#### Keys ignored in scope 1:.:.:client.pro:<TRUE>:
# check.commands = "$(TESTRUNNER)" "$${PWD}/run-with-all-shells.sh" "$(TESTARGS)"
