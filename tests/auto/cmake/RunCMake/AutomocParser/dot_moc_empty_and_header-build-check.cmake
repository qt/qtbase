# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include("${CMAKE_CURRENT_LIST_DIR}/check_json_list.cmake")

# The (empty) <base>.moc and the header moc (via mocs_compilation.cpp) are both present,
# but only HeaderType exists.
check_metatypes_json_list(EXPECTED_ENTRY_COUNT 2 EXPECTED_CLASS_NAMES HeaderType)

check_json_list_entries(
    EXPECTED_PATTERNS
        "/include(_[^/]+)?/(\\.\\./)?[A-Z2-7]+/moc_foo\\.cpp\\.json" # in Base32 checksum dir
        "/include(_[^/]+)?/foo\\.moc\\.json"
)
