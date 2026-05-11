# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

__qt_internal_cmake_include_guard(GLOBAL GUARD_KEY "QtPublicJsonHelpers")

# Escape backslash and double-quote characters in the given content so that the
# result can be safely embedded between double quotes in a JSON scalar.
function(_qt_internal_json_escape_content content out_var)
    string(REPLACE "\\" "\\\\" escaped_content "${content}")
    string(REPLACE "\"" "\\\"" escaped_content "${escaped_content}")
    set(${out_var} "${escaped_content}" PARENT_SCOPE)
endfunction()
