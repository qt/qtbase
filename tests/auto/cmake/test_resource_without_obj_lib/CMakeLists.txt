# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_resource_without_obj_lib)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt6 REQUIRED
    COMPONENTS Core Test
    HINTS ${Qt6Tests_PREFIX_PATH}
)

qt6_add_library(helper_lib STATIC helper_lib.cpp)
qt6_add_resources(helper_lib "helper_res" FILES resource.txt PREFIX "/")

# Link to Core, to ensure both the helper_lib and the main executable
# inherit the QT_NAMESPACE if it is set, otherwise we get undefined
# linker errors due to the mismatch in symbol names.
target_link_libraries(helper_lib PRIVATE Qt6::Core)

set(CMAKE_AUTOMOC ON)

qt6_add_executable(test_resource_without_obj_lib main.cpp)
target_link_libraries(test_resource_without_obj_lib PRIVATE Qt6::Core Qt6::Test)

# Link against the library file and not the target, so that we can confirm
# the ability to manually initialize the resource via Q_INIT_RESOURCE.
target_link_libraries(test_resource_without_obj_lib PRIVATE $<TARGET_FILE:helper_lib>)

