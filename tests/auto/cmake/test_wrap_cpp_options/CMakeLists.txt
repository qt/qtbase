# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_wrap_cpp_options)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt6Core REQUIRED HINTS ${Qt6Tests_PREFIX_PATH})

# Test options. The -i option removes the include "myobject.h" from the moc file
# causing a compile failure. -> Options work
qt_wrap_cpp(moc_files myobject.h OPTIONS -i)

add_executable(myobject myobject.cpp ${moc_files})
target_link_libraries(myobject PRIVATE Qt::Core)
