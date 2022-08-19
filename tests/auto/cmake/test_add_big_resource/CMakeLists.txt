# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_add_big_resource)

# Make sure that AUTOMOC does not interfere with qt_add_big_resources
set(CMAKE_AUTOMOC ON)

find_package(Qt6Core REQUIRED)

qt_wrap_cpp(moc_files myobject.h)

qt_add_big_resources(rcc_files "test_add_big_resource.qrc" "test_add_big_resource2.qrc"
    "subdir/test_add_big_resource.qrc")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject PRIVATE Qt::Core)
