# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt6_add_win_app_sdk target)
    if(NOT MSVC)
        message(WARNING
                "qt6_add_win_app_sdk doesn't work when targeting platforms other than MSVC.")
        return()
    endif()

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR
       CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
        set(win_app_sdk_arch "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
         set(win_app_sdk_arch "arm")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(win_app_sdk_arch "x64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(win_app_sdk_arch "x86")
    endif()

    if(WIN_APP_SDK_ROOT)
        set(win_app_sdk_root "${WIN_APP_SDK_ROOT}")
    elseif(DEFINED ENV{WIN_APP_SDK_ROOT})
        set(win_app_sdk_root "$ENV{WIN_APP_SDK_ROOT}")
    endif()

    if(WEB_VIEW_ROOT)
        set(web_view_root "${WEB_VIEW_ROOT}")
    elseif(DEFINED ENV{WEB_VIEW_ROOT})
        set(web_view_root "$ENV{WEB_VIEW_ROOT}")
    endif()

    set(generated_headers_path "${CMAKE_CURRENT_BINARY_DIR}/winrt_includes")

    find_path(WINAPPSDK_GENERATED_INCLUDE_DIR
        NAMES winrt/Microsoft.UI.h
        HINTS "${generated_headers_path}")
    # If headers are not already generated
    if(NOT WINAPPSDK_GENERATED_INCLUDE_DIR)
        if(CPP_WIN_RT_PATH)
            set(cpp_win_rt_path "${CPP_WIN_RT_PATH}")
        elseif(DEFINED ENV{CPP_WIN_RT_PATH})
            set(cpp_win_rt_path "$ENV{CPP_WIN_RT_PATH}")
        endif()
        if(NOT cpp_win_rt_path)
            find_file(CPP_WIN_RT_PATH
                NAMES cppwinrt.exe
                HINTS "C:/Program Files*/Windows Kits/*/bin/*/*${win_app_sdk_arch}*/")
            set(cpp_win_rt_path ${CPP_WIN_RT_PATH})
        endif()
        if(NOT cpp_win_rt_path)
            message(FATAL_ERROR "cppwinrt.exe could not be found")
        endif()

        find_path(WINAPPSDK_INCLUDE_DIR
            NAMES MddBootstrap.h
            HINTS ${win_app_sdk_root}/include)

        find_library(WINAPPSDK_LIBRARY
            NAMES Microsoft.WindowsAppRuntime
            HINTS ${WINAPPSDK_LIBRARY_DIR} "${win_app_sdk_root}"
                                           "${win_app_sdk_root}/lib"
                                           "${win_app_sdk_root}/lib/win10-${win_app_sdk_arch}")

        find_library(WINAPPSDK_BOOTSTRAP_LIBRARY
            NAMES Microsoft.WindowsAppRuntime.Bootstrap
            HINTS ${WINAPPSDK_LIBRARY_DIR} "${win_app_sdk_root}"
                                           "${win_app_sdk_root}/lib"
                                           "${win_app_sdk_root}/lib/win10-${win_app_sdk_arch}")

        if(WINAPPSDK_INCLUDE_DIR AND WINAPPSDK_LIBRARY AND WINAPPSDK_BOOTSTRAP_LIBRARY)
            execute_process(COMMAND
                ${cpp_win_rt_path} -out "${generated_headers_path}" -ref sdk
                -in "${win_app_sdk_root}/lib/uap10.0"
                -in "${win_app_sdk_root}/lib/uap10.0.17763"
                -in "${win_app_sdk_root}/lib/uap10.0.18362"
                -in "${web_view_root}/lib")

            find_path(WINAPPSDK_GENERATED_INCLUDE_DIR
                NAMES winrt/Microsoft.UI.h
                HINTS "${generated_headers_path}")

            if(NOT WINAPPSDK_GENERATED_INCLUDE_DIR)
                message(FATAL_ERROR "Windows App SDK  library headers generation failed")
            endif()
        else()
            message(FATAL_ERROR "Windows App SDK  library not found")
        endif()
    endif()

    target_include_directories(${target} PRIVATE "${win_app_sdk_root}/include")
    target_include_directories(${target}
                               PRIVATE "${generated_headers_path}")
    target_link_directories(${target}
                            PRIVATE "${win_app_sdk_root}/lib/win10-${win_app_sdk_arch}")
    target_link_directories(${target}
                        PRIVATE "${win_app_sdk_root}/runtimes/win-${win_app_sdk_arch}/native")
    target_link_libraries(${target}
        PRIVATE Microsoft.WindowsAppRuntime.lib Microsoft.WindowsAppRuntime.Bootstrap.lib)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_add_win_app_sdk)
        qt6_add_win_app_sdk(${ARGV})
    endmacro()
endif()
