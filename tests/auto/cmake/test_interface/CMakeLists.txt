# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_interface)

find_package(Qt6Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(test_interface_exe WIN32 main.cpp mainwindow.cpp)

# No need to specify include directories, compile definitions, the PIC flag, or to
# link explicitly to Qt::WinMain.
target_link_libraries(test_interface_exe Qt::Widgets)

# Fix try_compile to inherit the parent configuration.
set(CMAKE_TRY_COMPILE_CONFIGURATION "${CMAKE_BUILD_TYPE}")

# Can't use source file based try_compile, because it doesn't handle object libraries
# referenced in generator expressions properly.
try_compile(_TRY_COMPILE_RES
    "${CMAKE_CURRENT_BINARY_DIR}/widget_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/widget_test"
    widget_test
    OUTPUT_VARIABLE TC_OV
)

if (NOT _TRY_COMPILE_RES)
    message(SEND_ERROR "The use of try_compile with Qt::Widgets failed. The output was :\n${TC_OV}")
endif()
