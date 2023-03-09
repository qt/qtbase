# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Check whether no unity build is requested where it is disabled by default.
function(_qt_internal_validate_no_unity_build prefix)
    if(${prefix}_NO_UNITY_BUILD OR ${prefix}_NO_UNITY_BUILD_SOURCES)
        message(WARNING
            "Unity build is disabled by default for this target, and its sources. "
            "You may remove the NO_UNITY_BUILD and/or NO_UNITY_BUILD_SOURCES arguments.")
    endif()
endfunction()

function(qt_update_ignore_unity_build_sources target sources)
    if (sources)
        set_source_files_properties(${sources} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
    endif()
endfunction()
