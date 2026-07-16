# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include("${CMAKE_CURRENT_LIST_DIR}/check_json_list.cmake")

check_metatypes_json_list(EXPECTED_ENTRY_COUNT 4
    EXPECTED_CLASS_NAMES ParentHeader ParentSource ChildHeader ChildSource)

# Each header moc and each source .moc must resolve under its own directory.
check_json_list_entries(
    EXPECTED_PATTERNS
        "/include(_[^/]+)?/moc_bar\\.cpp\\.json"
        "/include(_[^/]+)?/bar\\.moc\\.json"
        "/include(_[^/]+)?/sub_bar/moc_bar\\.cpp\\.json"
        "/include(_[^/]+)?/sub_bar/bar\\.moc\\.json"
)
