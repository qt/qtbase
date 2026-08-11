# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
setup_mock_package("1.2.3")
setup_mock_module_package(Foo "6.140.0")

write_override_file(Foo "Override_0000_test.cmake" [[
set(PACKAGE_VERSION "6.140")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
]])

set(QT_NO_CONFIG_VERSION_OVERRIDE_FILES ON)

find_package(${mock_pkg_name} 6.140 COMPONENTS Foo QUIET
    PATHS "${mock_pkg_dir}" NO_DEFAULT_PATH
)

if(${mock_pkg_name}_FOUND)
    message(FATAL_ERROR
        "Expected ${mock_pkg_name} to not be found: the base version "
        "${mock_pkg_base_version} does not satisfy the requested 6.140 once "
        "overrides are disabled at runtime")
endif()

message(STATUS "runtime_opt_out: OK, ${mock_pkg_name}_FOUND=${${mock_pkg_name}_FOUND}")
