# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qt_namespace_property LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core)

get_property(namespace_property
    TARGET Qt::Core
    PROPERTY QT_NAMESPACE
)

add_executable(test main.cpp)
target_link_libraries(test PRIVATE Qt::Core)

if(namespace_property)
    target_compile_definitions(test PRIVATE QT_NAMESPACE_FROM_PROPERTY=${namespace_property})
else()
    target_compile_definitions(test PRIVATE QT_NO_NAMESPACE)
endif()
