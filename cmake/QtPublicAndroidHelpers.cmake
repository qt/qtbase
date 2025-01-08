# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(_qt_internal_detect_latest_android_platform out_var)
    # Locate the highest available platform
    file(GLOB android_platforms
        LIST_DIRECTORIES true
        RELATIVE "${ANDROID_SDK_ROOT}/platforms"
        "${ANDROID_SDK_ROOT}/platforms/*")

    # If list is not empty
    if(android_platforms)
        _qt_internal_sort_android_platforms(android_platforms ${android_platforms})
        list(REVERSE android_platforms)
        list(GET android_platforms 0 android_platform_latest)
        set(${out_var} "${android_platform_latest}" PARENT_SCOPE)
    else()
        set(${out_var} "${out_var}-NOTFOUND" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_sort_android_platforms out_var)
    if(CMAKE_VERSION GREATER_EQUAL 3.18)
        set(platforms ${ARGN})
        list(SORT platforms COMPARE NATURAL)
    else()
        # Simulate natural sorting:
        # - prepend every platform with its version as three digits, zero-padded
        # - regular sort
        # - remove the padded version prefix
        set(platforms)
        foreach(platform IN LISTS ARGN)
            set(version "000")
            if(platform MATCHES ".*-([0-9]+)$")
                set(version ${CMAKE_MATCH_1})
                string(LENGTH "${version}" version_length)
                math(EXPR padding_length "3 - ${version_length}")
                string(REPEAT "0" ${padding_length} padding)
                string(PREPEND version ${padding})
            endif()
            list(APPEND platforms "${version}~${platform}")
        endforeach()
        list(SORT platforms)
        list(TRANSFORM platforms REPLACE "^.*~" "")
    endif()
    set("${out_var}" "${platforms}" PARENT_SCOPE)
endfunction()
