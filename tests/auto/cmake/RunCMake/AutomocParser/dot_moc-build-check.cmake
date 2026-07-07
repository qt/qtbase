# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include("${CMAKE_CURRENT_LIST_DIR}/check_json_list.cmake")

check_metatypes_json_list(EXPECTED_ENTRY_COUNT 1 EXPECTED_CLASS_NAMES Impl)
