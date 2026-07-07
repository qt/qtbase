# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include("${CMAKE_CURRENT_LIST_DIR}/check_json_list.cmake")

check_metatypes_json_list(EXPECTED_ENTRY_COUNT 2 EXPECTED_CLASS_NAMES Foo Foo2)

# Each source must resolve to its own moc under its own directory.
check_json_list_entries(
    EXPECTED_PATTERNS
        "/include(_[^/]+)?/moc_foo\\.cpp\\.json"
        "/include(_[^/]+)?/sub/moc_foo\\.cpp\\.json"
)
