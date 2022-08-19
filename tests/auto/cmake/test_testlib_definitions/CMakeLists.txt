# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_testlib_definitions)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

macro(test_testlib_project _module)
    find_package(Qt6${_module} REQUIRED)
    find_package(Qt6Test REQUIRED)

    set(main_file "${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp")
    set(moc_file "${CMAKE_CURRENT_BINARY_DIR}/main.moc")

    qt_generate_moc("${main_file}" "${moc_file}")

    add_executable(testapp_${_module} "${main_file}" "${moc_file}")
    target_link_libraries(testapp_${_module}
        Qt::${_module}
        Qt::Test
    )
endmacro()

add_subdirectory(core_only)
if(NOT NO_GUI)
    add_subdirectory(gui)
endif()
if(NOT NO_WIDGETS)
    add_subdirectory(widgets)
endif()
