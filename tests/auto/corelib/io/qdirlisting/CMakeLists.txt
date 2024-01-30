# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdirlisting Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdirlisting LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "entrylist")

qt_internal_add_test(tst_qdirlisting
    SOURCES
        tst_qdirlisting.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
    TESTDATA ${test_data}
)

# Resources:
set(qdirlisting_resource_files
    "entrylist/directory/dummy"
    "entrylist/file"
)

qt_internal_add_resource(tst_qdirlisting "qdirlisting"
    PREFIX
        "/testdata/"
    FILES
        ${qdirlisting_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdirlisting CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
