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
        qt_internal_sort_android_platforms(android_platforms ${android_platforms})
        list(REVERSE android_platforms)
        list(GET android_platforms 0 android_platform_latest)
        set(${out_var} "${android_platform_latest}" PARENT_SCOPE)
    else()
        set(${out_var} "${out_var}-NOTFOUND" PARENT_SCOPE)
    endif()
endfunction()
