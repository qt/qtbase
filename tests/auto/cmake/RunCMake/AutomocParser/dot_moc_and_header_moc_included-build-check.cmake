# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include("${CMAKE_CURRENT_LIST_DIR}/check_json_list.cmake")

# Header moc (via the moc_<base>.cpp include) and the source's <base>.moc are both present.
check_metatypes_json_list(EXPECTED_ENTRY_COUNT 2 EXPECTED_CLASS_NAMES HeaderType SourceType)

check_json_list_entries(
    EXPECTED_PATTERNS
        "/include(_[^/]+)?/moc_foo\\.cpp\\.json"
        "/include(_[^/]+)?/foo\\.moc\\.json"
)
