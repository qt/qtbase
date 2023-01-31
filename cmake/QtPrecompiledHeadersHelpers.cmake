# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_update_precompiled_header target precompiled_header)
    if (precompiled_header AND BUILD_WITH_PCH)
        set_property(TARGET "${target}" APPEND PROPERTY "PRECOMPILE_HEADERS" "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:${precompiled_header}>")
    endif()
endfunction()

function(qt_update_precompiled_header_with_library target library)
    if (TARGET "${library}")
        get_target_property(TARGET_TYPE "${library}" TYPE)
        if (NOT TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
            get_target_property(HEADER "${library}" MODULE_HEADER)
            qt_update_precompiled_header("${target}" "${HEADER}")
        endif()
    endif()
endfunction()

function(qt_update_ignore_pch_source target sources)
    if (sources)
        set_source_files_properties(${sources} PROPERTIES
                                               SKIP_PRECOMPILE_HEADERS ON
                                               SKIP_UNITY_BUILD_INCLUSION ON)
    endif()
endfunction()

function(qt_ignore_pch_obj_c_sources target sources)
    # No obj-cxx PCH support for versions lower than 3.16.
    if(CMAKE_VERSION VERSION_LESS 3.16.0)
        list(FILTER sources INCLUDE REGEX "\\.mm$")
        qt_update_ignore_pch_source("${target}" "${sources}")
    endif()
endfunction()
