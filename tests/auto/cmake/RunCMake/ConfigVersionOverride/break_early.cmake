# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
setup_mock_package("1.2.3")
setup_mock_module_package(Foo "6.140.0")

write_override_file(Foo "Override_0000_first.cmake" [[
set(PACKAGE_VERSION "6.140")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
set(__qt_version_override_break_early TRUE)
]])
write_override_file(Foo "Override_0001_second.cmake" [[
set(PACKAGE_VERSION "6.150")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
]])

find_package(${mock_pkg_name} 6.140 COMPONENTS Foo
    PATHS "${mock_pkg_dir}" NO_DEFAULT_PATH
)

if(NOT ${mock_pkg_name}_FOUND)
    message(FATAL_ERROR "Expected ${mock_pkg_name} to be found")
endif()
if(NOT ${mock_pkg_name}_VERSION STREQUAL "6.140")
    message(FATAL_ERROR
        "Expected the break-early override to win with version 6.140, got "
        "'${${mock_pkg_name}_VERSION}'")
endif()

message(STATUS "break_early: OK, ${mock_pkg_name}_VERSION=${${mock_pkg_name}_VERSION}")
