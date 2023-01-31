# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_update_ignore_unity_build_sources target sources)
    if (sources)
        set_source_files_properties(${sources} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
    endif()
endfunction()
