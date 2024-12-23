# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qcoreapplication LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qcoreapplication Test:
#####################################################################

if (WIN32)
    set(target_version "1.2.3.4")
else()
    set(target_version "1.2.3")
endif()

qt_internal_add_test(tst_qcoreapplication
    VERSION ${target_version}
    SOURCES
        tst_qcoreapplication.cpp tst_qcoreapplication.h
    LIBRARIES
        Qt::CorePrivate
)

if (APPLE)
    set_property(TARGET tst_qcoreapplication PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    set_property(TARGET tst_qcoreapplication PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
endif()

if (ANDROID)
    set_property(TARGET tst_qcoreapplication PROPERTY QT_ANDROID_VERSION_NAME ${target_version})
endif()

qt_internal_add_test(tst_static_qcoreapplication
    SOURCES
        tst_static_qcoreapplication.cpp
        maybeshow.h
    LIBRARIES
        Qt::CorePrivate
)

qt_internal_add_executable(apphelper_core
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        apphelper.cpp
        maybeshow.h
    LIBRARIES
        Qt::CorePrivate
)
set_target_properties(apphelper_core PROPERTIES OUTPUT_NAME apphelper)

add_dependencies(tst_qcoreapplication
    apphelper_core
)
