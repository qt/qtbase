# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_json Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_json LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(json_resource_files
    "bom.json"
    "test.json"
    "test2.json"
    "test3.json"
    "simple.duplicates.json"
    "test.duplicates.json"
    "test3.duplicates.json"
)

qt_internal_add_test(tst_json
    SOURCES
        tst_qtjson.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
    TESTDATA ${json_resource_files}
)

qt_internal_extend_target(tst_json CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)
