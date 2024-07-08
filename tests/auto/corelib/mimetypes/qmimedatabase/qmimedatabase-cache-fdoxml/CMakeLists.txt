# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmimedatabase LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qmimedatabase-cache-fdoxml Test:
#####################################################################

qt_internal_add_test(tst_qmimedatabase-cache-fdoxml
    RUN_SERIAL
    SOURCES
        ../tst_qmimedatabase.h
        tst_qmimedatabase-cache-fdoxml.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Concurrent
)

# Resources:
set(mimetypes_resource_files
    "../3rdparty/freedesktop.org.xml"
)
set(testdata_resource_files
    "../add-extension.xml"
    "../circular-inheritance.xml"
    "../invalid-magic1.xml"
    "../invalid-magic2.xml"
    "../invalid-magic3.xml"
    "../magic-and-hierarchy.foo"
    "../magic-and-hierarchy.xml"
    "../magic-and-hierarchy2.foo"
    "../qml-again.xml"
    "../test.qml"
    "../text-x-objcsrc.xml"
    "../text-plain-subclass.xml"
    "../webm-glob-deleteall.xml"
    "../yast2-metapackage-handler-mimetypes.xml"
)

qt_internal_add_resource(tst_qmimedatabase-cache-fdoxml "testdata"
    PREFIX
        "/tst_qmimedatabase/qmime"
    BASE
        ".."
    FILES
        ${testdata_resource_files}
)

qt_internal_add_resource(tst_qmimedatabase-cache-fdoxml "testdata-fdoxml"
    PREFIX
        "/tst_qmimedatabase/qmime"
    BASE
        "../3rdparty"
    FILES
        ${mimetypes_resource_files}
)

qt_internal_add_resource(tst_qmimedatabase-cache-fdoxml "testfiles"
    PREFIX
        "/files"
    FILES
        "../test.txt"
        "../test.qml"
)

set(corelib_source_dir ../../../../../../src/corelib)
include(${corelib_source_dir}/mimetypes/mimetypes_resources.cmake)
corelib_add_mimetypes_resources(tst_qmimedatabase-cache-fdoxml)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmimedatabase-cache-fdoxml CONDITION GCC
    COMPILE_OPTIONS
        -W
        -Wall
        -Wextra
        -Wno-long-long
        -Wnon-virtual-dtor
        -Wshadow
)

qt_internal_extend_target(tst_qmimedatabase-cache-fdoxml CONDITION UNIX AND NOT APPLE AND NOT QNX
    DEFINES
        USE_XDG_DATA_DIRS
)
