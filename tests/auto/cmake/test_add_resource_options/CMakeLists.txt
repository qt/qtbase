# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_add_resource_options)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt6Core REQUIRED HINTS ${Qt6Tests_PREFIX_PATH})

qt_wrap_cpp(moc_files myobject.h)

# Test options. The -binary option generates a binary to dlopen instead of
# a source file to compile. The compiler will consider it garbage when used
# in the add_executable call.
qt_add_resources(rcc_files "test_macro_options.qrc" OPTIONS -binary)

# Test if OPTIONS can handle a quoted parameter. CMake would fail immediately!
qt_add_resources(rcc_files_quoted_option "test_macro_options.qrc" OPTIONS -root "/")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject PRIVATE Qt::Core)
