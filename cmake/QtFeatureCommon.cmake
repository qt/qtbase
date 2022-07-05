# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

function(qt_feature_normalize_name name out_var)
    # Normalize the feature name to something CMake can deal with.
    if(name MATCHES "c\\+\\+")
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "x" name "${name}")
    else()
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" name "${name}")
    endif()
    set(${out_var} "${name}" PARENT_SCOPE)
endfunction()
