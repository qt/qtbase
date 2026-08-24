# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Find the HarmonyOS toolchain file within the SDK that's specified with the
# OHOS_SDK_ROOT variable. If the toolchain file cannot be detected, ${out_var}
# will be set to the value of `fallback`.
function(__qt_internal_detect_ohos_toolchain_file out_var fallback)
    set(result "${fallback}")
    if(NOT "${OHOS_SDK_ROOT}" STREQUAL "")
        set(candidate "${OHOS_SDK_ROOT}/native/build/cmake/ohos.toolchain.cmake")
        if(EXISTS "${candidate}")
            set(result "${candidate}")
            message(DEBUG "HarmonyOS toolchain file within SDK detected: ${result}")
        endif()
    endif()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

function(__qt_internal_show_error_no_ohos_toolchain_file_found_when_building_qt)
    message(FATAL_ERROR
        "Cannot find the toolchain file ohos.toolchain.cmake. "
        "Please specify the toolchain file with -DCMAKE_TOOLCHAIN_FILE=<file> "
        "or provide a path to a valid HarmonyOS SDK installation via the OHOS_SDK_ROOT "
        "CMake variable or the -ohos-sdk configure argument.")
endfunction()

function(__qt_internal_show_error_no_ohos_toolchain_file_found_when_using_qt)
    message(FATAL_ERROR
        "Cannot find the toolchain file ohos.toolchain.cmake. "
        "Please specify the toolchain file with -DQT_CHAINLOAD_TOOLCHAIN_FILE=<file> "
        "or provide a path to a valid HarmonyOS SDK installation via the OHOS_SDK_ROOT "
        "environment variable.")
endfunction()
