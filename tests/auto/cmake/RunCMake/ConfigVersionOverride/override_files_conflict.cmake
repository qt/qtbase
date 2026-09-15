# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
setup_mock_package("1.2.3")
setup_mock_module_package(Foo "6.140.0")
setup_mock_module_package(Boo "6.130.0")

write_override_file(Foo "Override_0000_test.cmake" [[
set(PACKAGE_VERSION "6.140")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
]])

write_override_file(Boo "Override_0000_test.cmake" [[
set(PACKAGE_VERSION "6.130")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
]])

find_package(${mock_pkg_name} 6.140 COMPONENTS Foo Boo QUIET
    PATHS "${mock_pkg_dir}" NO_DEFAULT_PATH
)

if(${mock_pkg_name}_FOUND)
    message(FATAL_ERROR
        "Expected ${mock_pkg_name} to not be found: Foo and Bar override files missmatch"
    )
endif()

