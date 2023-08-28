# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdnslookup Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdnslookup LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qdnslookup
    SOURCES
        tst_qdnslookup.cpp
    LIBRARIES
        Qt::Network
        Qt::TestPrivate
)

qt_internal_extend_target(tst_qdnslookup CONDITION WIN32
    LIBRARIES
        iphlpapi
)
