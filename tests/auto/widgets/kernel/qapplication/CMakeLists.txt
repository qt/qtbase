# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qapplication LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

add_subdirectory(desktopsettingsaware)
add_subdirectory(modal)
add_subdirectory(test)

add_dependencies(tst_qapplication
    desktopsettingsaware_helper
    modal_helper
)

qt_internal_add_test(tst_static_qapplication
    SOURCES
        ../../../corelib/kernel/qcoreapplication/tst_static_qcoreapplication.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Widgets
)
