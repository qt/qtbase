# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtranslator Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtranslator LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if (NOT QT_FEATURE_thread)
    return()
endif()

qt_internal_add_test(tst_qtranslator
    SOURCES
        tst_qtranslator.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
set(qtranslator_resource_files
    "dependencies_la.qm"
    "hellotr_empty.qm"
    "hellotr_la.qm"
    "msgfmt_from_po.qm"
)

qt_internal_add_resource(tst_qtranslator "qtranslator"
    PREFIX
        "/tst_qtranslator"
    FILES
        ${qtranslator_resource_files}
)

## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(android_testdata_resource_files
        "dependencies_la.qm"
        "hellotr_empty.qm"
        "hellotr_la.qm"
        "msgfmt_from_po.qm"
    )

    qt_internal_add_resource(tst_qtranslator "android_testdata"
        PREFIX
            "/android_testdata"
        FILES
            ${android_testdata_resource_files}
    )
endif()
