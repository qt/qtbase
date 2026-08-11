# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Test failed case where Bar is last and it has version different
# than requested and Foo depends on Bar.

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
setup_mock_package("1.2.3")
setup_mock_module_package(Foo "6.140.0")
setup_mock_module_package(Boo "6.140.0")
setup_mock_module_package(Bar "1.2.3")

write_override_file(Foo "Override_0000_test.cmake" [[
set(PACKAGE_VERSION "6.140")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
set(PACKAGE_VERSION_EXACT TRUE)
]])

write_override_file(Boo "Override_0000_test.cmake" [[
set(PACKAGE_VERSION "6.140")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
set(PACKAGE_VERSION_EXACT TRUE)
]])


write_module_dependency(Foo ${mock_pkg_name}Bar "1.2.3")


find_package(${mock_pkg_name} 6.140 EXACT COMPONENTS Foo Boo
    PATHS "${mock_pkg_dir}" NO_DEFAULT_PATH
)

if(NOT ${mock_pkg_name}_FOUND)
    message(FATAL_ERROR "Expected ${mock_pkg_name} to be found via version override")
endif()

find_package(${mock_pkg_name} 6.140 COMPONENTS Foo Boo Bar
    PATHS "${mock_pkg_dir}" NO_DEFAULT_PATH
)
