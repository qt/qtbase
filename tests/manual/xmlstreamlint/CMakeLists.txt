# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
     cmake_minimum_required(VERSION 3.16)
     project(xmlstreamlint LANGUAGES CXX)
     find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
 endif()

qt_internal_add_manual_test(xmlstreamlint
    SOURCES
        main.cpp
)

set_target_properties(xmlstreamlint PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(xmlstreamlint PRIVATE
    Qt::Core
    Qt::Xml
)

