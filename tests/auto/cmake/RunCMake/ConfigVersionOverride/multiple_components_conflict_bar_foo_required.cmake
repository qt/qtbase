# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Test failed case where Bar is first and it has version different
# than requested.

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
setup_mock_package("1.2.3")
setup_mock_module_package(Foo "6.140.0")
setup_mock_module_package(Bar "1.2.3")

write_override_file(Foo "Override_0000_test.cmake" [[
set(PACKAGE_VERSION "6.140")
set(PACKAGE_VERSION_COMPATIBLE TRUE)
]])

find_package(${mock_pkg_name} 6.140 REQUIRED COMPONENTS Bar Foo
    PATHS "${mock_pkg_dir}" NO_DEFAULT_PATH
)
