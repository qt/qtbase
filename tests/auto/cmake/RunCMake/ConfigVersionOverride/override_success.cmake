# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
setup_mock_package("1.2.3")
setup_mock_module_package(Foo "6.140.0")

write_override_file(Foo "Override_0000_test.cmake" [[
set(PACKAGE_VERSION "6.140")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
set(PACKAGE_VERSION_EXACT TRUE)
]])

find_package(${mock_pkg_name} 6.140 EXACT COMPONENTS Foo
    PATHS "${mock_pkg_dir}" NO_DEFAULT_PATH
)

if(NOT ${mock_pkg_name}_FOUND)
    message(FATAL_ERROR "Expected ${mock_pkg_name} to be found via version override")
endif()
if(NOT ${mock_pkg_name}_VERSION STREQUAL "6.140")
    message(FATAL_ERROR
        "Expected ${mock_pkg_name}_VERSION to be overridden to 6.140, got "
        "'${${mock_pkg_name}_VERSION}'")
endif()

message(STATUS "override_success: OK, ${mock_pkg_name}_VERSION=${${mock_pkg_name}_VERSION}")
