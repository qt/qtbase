# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qresourceengine Test:
#####################################################################

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qresourceengine LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
set(test_data "parentdir.txt")
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testqrc/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.rcc)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qresourceengine
    SOURCES
        tst_qresourceengine.cpp
    LIBRARIES
        Qt::CorePrivate
        moctestplugin
    TESTDATA ${test_data}
)

set_source_files_properties("world.txt"
    PROPERTIES QT_DISCARD_FILE_CONTENTS TRUE
)

qt_internal_add_resource(tst_qresourceengine "qt_resource_empty"
    PREFIX
        "/empty"
    FILES
        "world.txt"
)

qt_add_resources(additional_sources testqrc/test.qrc)
target_sources(tst_qresourceengine PRIVATE ${additional_sources})

if(ANDROID)
    qt_add_resources(additional_sources android_testdata.qrc)
    target_sources(tst_qresourceengine PRIVATE ${additional_sources})
endif()

qt_add_binary_resources(tst_qresourceengine_runtime_resource "testqrc/test.qrc"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/runtime_resource.rcc"
    OPTIONS -root "/runtime_resource/" -binary)
add_dependencies(tst_qresourceengine tst_qresourceengine_runtime_resource)

add_subdirectory(staticplugin)
