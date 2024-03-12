# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(_qt_internal_normalize_project_version)
    # Default if PROJECT_VERSION is missing or empty
    if(NOT DEFINED PROJECT_VERSION OR PROJECT_VERSION STREQUAL "")
        set(_version "1.0.0.0")
    else()
        set(_version "${PROJECT_VERSION}")
    endif()

    # Split version into components
    string(REPLACE "." ";" _ver_list "${_version}")

    # Ensure exactly 4 segments
    list(LENGTH _ver_list _len)
    while(_len LESS 4)
        list(APPEND _ver_list "0")
        list(LENGTH _ver_list _len)
    endwhile()

    # Truncate extra segments
    list(SUBLIST _ver_list 0 4 _ver_list)

    # Windows Application Manifests require a 4-part version (Major.Minor.Build.Revision).
    # Each segment is stored as a 16-bit unsigned integer (USHORT).
    # Values exceeding 65535 will cause schema validation errors in MSIX/AppX
    # or 'Side-by-Side' (SxS) configuration failures in Win32 executables.
    # We clamp the values here to ensure the manifest remains valid for Windows.
    set(_ver_list_tmp "")
    foreach(i RANGE 0 3)
        list(GET _ver_list ${i} seg)
        if(seg LESS 0)
            message(WARNING
                        "Version segment '${seg}' is less than 0. Capping to 0.")
            set(seg 0)
        elseif(seg GREATER 65535)
            message(WARNING
                "Version segment '${seg}' exceeds 65535. Capping to 65535.")
            set(seg 65535)
        endif()
        list(APPEND _ver_list_tmp ${seg})
    endforeach()

    list(GET _ver_list_tmp 0 _maj)
    list(GET _ver_list_tmp 1 _min)
    list(GET _ver_list_tmp 2 _build)
    list(GET _ver_list_tmp 3 _revision)

    # Recompose canonical version
    set(PROJECT_VERSION
        "${_maj}.${_min}.${_build}.${_revision}"
        PARENT_SCOPE)
endfunction()

function(_qt_internal_finalize_windows_app target)
    get_target_property(targetType "${target}" TYPE)
    if(NOT "${targetType}" STREQUAL "EXECUTABLE" AND
            NOT "${targetType}" STREQUAL "SHARED_LIBRARY")
        return()
    endif()

    # If the project already specifies a custom file, we don't override it.
    get_target_property(sources "${target}" SOURCES)
    set(manifest_in "${sources}")
    list(FILTER manifest_in INCLUDE REGEX ".*\.manifest")
    if(manifest_in)
        message(DEBUG "Skipping manifest magic due to user specified manifest file")
        return()
    endif()

    set(manifest_in "${__qt_internal_cmake_windows_support_files_path}/app.exe.manifest.in")

    get_target_property(output_name ${target} OUTPUT_NAME)
    if(NOT output_name)
        set(output_name "${target}")
    endif()
    string(MAKE_C_IDENTIFIER "${target}" target_identifier)
    set(manifest_out_dir "${CMAKE_CURRENT_BINARY_DIR}")
    set(manifest_out "${manifest_out_dir}/${output_name}.exe.manifest")

    _qt_internal_normalize_project_version()

    get_target_property(project_identifier "${target}" QT_WINDOWS_APP_PROJECT_IDENTIFIER)
    if(NOT project_identifier)
        message(DEBUG "QT_WINDOWS_APP_PROJECT_IDENTIFIER not set, "
                      "using com.yourcompany.${target_identifier} as fallback")
        set(QT_WINDOWS_APP_PROJECT_IDENTIFIER "com.yourcompany.${target_identifier}")
    else()
        set(QT_WINDOWS_APP_PROJECT_IDENTIFIER "${project_identifier}")
    endif()

    get_target_property(project_executionlevel "${target}" QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL)
    if(NOT project_executionlevel)
        message(DEBUG "QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL not set, "
                      "using asInvoker as fallback")
        set(QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL "asInvoker")
    else()
        # Validate value
        set(_valid_execution_levels
            "asInvoker"
            "highestAvailable"
            "requireAdministrator"
        )

        if(NOT project_executionlevel IN_LIST _valid_execution_levels)
            message(WARNING
                "Invalid QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL value: "
                "'${project_executionlevel}' for target '${target}'."
                "Defaulting to 'asInvoker'."
                " Valid values are: asInvoker, highestAvailable, requireAdministrator."
            )
            set(QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL "asInvoker")
        else()
            set(QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL "${project_executionlevel}")
        endif()
    endif()

    # Call configure_file to substitute Qt-specific @FOO@ values, not ${FOO} values.
    configure_file(
        "${manifest_in}"
        "${manifest_out}"
        @ONLY
    )

    target_sources("${target}" PRIVATE "${manifest_out}")
    if(MSVC)
        target_link_options(${target} PRIVATE "/MANIFEST:NO")
    endif()
endfunction()

function(qt6_add_win_app_sdk target)
    if(NOT MSVC)
        message(WARNING
                "qt6_add_win_app_sdk doesn't work when targeting platforms other than MSVC.")
        return()
    endif()

    set(no_value_options INTERFACE PUBLIC PRIVATE)
    set(single_value_options "")
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    set(propagation PRIVATE)
    if(arg_PUBLIC)
        set(propagation PUBLIC)
    elseif(arg_INTERFACE)
        set(propagation INTERFACE)
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

    set(winappsdk_generated_include_dir "${generated_headers_path}/winrt")
    # If headers are not already generated
    if(NOT EXISTS "${winappsdk_generated_include_dir}")

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

        find_path(winappsdk_include_dir
            NAMES MddBootstrap.h
            HINTS ${win_app_sdk_root}/include
            NO_CACHE)

        find_library(winappsdk_library
            NAMES Microsoft.WindowsAppRuntime
            HINTS ${WINAPPSDK_LIBRARY_DIR} "${win_app_sdk_root}"
                                           "${win_app_sdk_root}/lib"
                                           "${win_app_sdk_root}/lib/win10-${win_app_sdk_arch}"
            NO_CACHE)

        find_library(winappsdk_bootstrap_library
            NAMES Microsoft.WindowsAppRuntime.Bootstrap
            HINTS ${WINAPPSDK_LIBRARY_DIR} "${win_app_sdk_root}"
                                           "${win_app_sdk_root}/lib"
                                           "${win_app_sdk_root}/lib/win10-${win_app_sdk_arch}"
            NO_CACHE)

        if(winappsdk_include_dir AND winappsdk_library AND winappsdk_bootstrap_library)
            execute_process(COMMAND
                ${cpp_win_rt_path} -out "${generated_headers_path}" -ref sdk
                -in "${win_app_sdk_root}/lib/uap10.0"
                -in "${win_app_sdk_root}/lib/uap10.0.17763"
                -in "${win_app_sdk_root}/lib/uap10.0.18362"
                -in "${web_view_root}/lib")

            if(NOT EXISTS "${winappsdk_generated_include_dir}")
                message(FATAL_ERROR "Windows App SDK  library headers generation failed.")
            endif()
        else()
            message(FATAL_ERROR "Windows App SDK  library not found")
        endif()
    endif()

    target_include_directories(${target} ${propagation} "${win_app_sdk_root}/include")
    target_include_directories(${target}
                               ${propagation} "${generated_headers_path}")
    target_link_directories(${target}
                            ${propagation} "${win_app_sdk_root}/lib/win10-${win_app_sdk_arch}")
    target_link_directories(${target}
                        ${propagation} "${win_app_sdk_root}/runtimes/win-${win_app_sdk_arch}/native")
    target_link_libraries(${target}
        ${propagation} Microsoft.WindowsAppRuntime.lib Microsoft.WindowsAppRuntime.Bootstrap.lib)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_add_win_app_sdk)
        qt6_add_win_app_sdk(${ARGV})
    endmacro()
endif()
