# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generate deployment tool json

# Locate newest Android sdk build tools revision
function(_qt_internal_android_get_sdk_build_tools_revision out_var)
    if (NOT QT_ANDROID_SDK_BUILD_TOOLS_REVISION)
        file(GLOB android_build_tools
            LIST_DIRECTORIES true
            RELATIVE "${ANDROID_SDK_ROOT}/build-tools"
            "${ANDROID_SDK_ROOT}/build-tools/*")
        list(FILTER android_build_tools INCLUDE REGEX "[0-9]+\.[0-9]+(\.[0-9]+)?")
        if(NOT android_build_tools)
            message(FATAL_ERROR "Could not locate Android SDK build tools under \"${ANDROID_SDK_ROOT}/build-tools\"")
        endif()
        list(SORT android_build_tools)
        list(REVERSE android_build_tools)
        list(GET android_build_tools 0 android_build_tools_latest)
    endif()
    set(${out_var} "${android_build_tools_latest}" PARENT_SCOPE)
endfunction()

# Returns the target specific Android SDK tools revision. The function falls
# back to the calculated value if the QT_ANDROID_SDK_BUILD_TOOLS_REVISION
# target property is not set.
function(_qt_internal_android_get_target_sdk_build_tools_revision out_var target)
    _qt_internal_android_get_sdk_build_tools_revision(android_sdk_build_tools)
    set(android_sdk_build_tools_genex "")
    string(APPEND android_sdk_build_tools_genex
        "$<IF:$<BOOL:$<TARGET_PROPERTY:${target},QT_ANDROID_SDK_BUILD_TOOLS_REVISION>>,"
            "$<TARGET_PROPERTY:${target},QT_ANDROID_SDK_BUILD_TOOLS_REVISION>,"
            "${android_sdk_build_tools}"
        ">"
    )

    set(${out_var} "${android_sdk_build_tools_genex}" PARENT_SCOPE)
endfunction()

# Returns the path to androiddeployqt.
function(_qt_internal_android_get_deployment_tool out_var)
    if(TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::androiddeployqt)
        set(${out_var} ${QT_CMAKE_EXPORT_NAMESPACE}::androiddeployqt PARENT_SCOPE)
    else()
        set(fall_back_absolute_path "${QT_HOST_PATH}/${QT6_HOST_INFO_BINDIR}/androiddeployqt")
        if(NOT EXISTS "${fall_back_absolute_path}")
            message(FATAL_ERROR "Unable to detect androiddeployqt in system installation."
                " Please reinstall Qt."
                " If the issue persists, please report a bug at https://bugreports.qt.io."
            )
        endif()

        set(${out_var} "${fall_back_absolute_path}" PARENT_SCOPE)
    endif()
endfunction()

# The function appends to the 'out_var' a 'json_property' that contains the 'tool' path. If 'tool'
# target or its IMPORTED_LOCATION are not found the function displays warning, but is not failing
# at the project configuring phase.
function(_qt_internal_add_tool_to_android_deployment_settings out_var tool json_property target)
    unset(tool_binary_path)
    __qt_internal_get_tool_imported_location(tool_binary_path ${tool})
    if("${tool_binary_path}" STREQUAL "")
        # Fallback search for the tool in host bin and host libexec directories
        find_program(tool_binary_path
            NAMES ${tool} ${tool}.exe
            PATHS
                "${QT_HOST_PATH}/${QT6_HOST_INFO_BINDIR}"
                "${QT_HOST_PATH}/${QT6_HOST_INFO_LIBEXECDIR}"
            NO_DEFAULT_PATH
        )
        if(NOT tool_binary_path)
            message(WARNING "Unable to locate ${tool}. Android package deployment of ${target}"
            " target can be incomplete. Make sure the host Qt has ${tool} installed.")
            return()
        endif()
    endif()

    file(TO_CMAKE_PATH "${tool_binary_path}" tool_binary_path)
    string(APPEND ${out_var}
        "   \"${json_property}\" : \"${tool_binary_path}\",\n")

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Generates a JSON array of permissions that the 'target' may have,
# returns an empty JSON array if no permissions were found.
function(_qt_internal_generate_android_permissions_json out_result target)

    set(${out_result} "[]" PARENT_SCOPE)

    if(NOT TARGET ${target})
        return()
    endif()

    get_target_property(permissions ${target} QT_ANDROID_PERMISSIONS)
    if(NOT permissions)
        return()
    endif()

    set(result "[")
    set(json_objects "")
    foreach(permission IN LISTS permissions)
        # Check if the permission has also extra attributes in addition to the permission name
        list(LENGTH permission permission_len)
        if(permission_len EQUAL 1)
            list(APPEND json_objects "{ \"name\": \"${permission}\" }")
        elseif(permission_len EQUAL 2)
            list(GET permission 0 name)
            list(GET permission 1 extras)
            list(APPEND json_objects "{ \"name\": \"${name}\", \"extras\": \"${extras}\" }")
        else()
            message(FATAL_ERROR "Invalid permission format: ${permission} ${permission_len}")
        endif()
    endforeach()

    # Join all JSON objects with a comma. This also avoids trailing commas JSON doesn't accept
    string(JOIN ",\n      " joined_json_objects ${json_objects})
    string(APPEND result "\n      ${joined_json_objects}\n   ]")
    set(${out_result} "${result}" PARENT_SCOPE)
endfunction()

# Generate the deployment settings json file for a cmake target.
function(qt6_android_generate_deployment_settings target)
    # Information extracted from mkspecs/features/android/android_deployment_settings.prf
    if (NOT TARGET ${target})
        message(FATAL_ERROR "${target} is not a cmake target")
    endif()

    # When parsing JSON file format backslashes and follow up symbols are regarded as special
    # characters. This puts Windows path format into a trouble.
    # _qt_internal_android_format_deployment_paths converts sensitive paths to the CMake format
    # that is supported by JSON as well. The function should be called as many times as
    # qt6_android_generate_deployment_settings, because users may change properties that contain
    # paths in between the calls.
    _qt_internal_android_format_deployment_paths(${target})

    # Avoid calling the function body twice because of 'file(GENERATE'.
    get_target_property(is_called ${target} _qt_is_android_generate_deployment_settings_called)
    if(is_called)
        return()
    endif()
    set_target_properties(${target} PROPERTIES
        _qt_is_android_generate_deployment_settings_called TRUE
    )

    get_target_property(android_executable_finalizer_called
        ${target} _qt_android_executable_finalizer_called)

    if(android_executable_finalizer_called)
        # Don't show deprecation when called by our own function implementations.
    else()
        message(DEPRECATION
            "Calling qt_android_generate_deployment_settings directly is deprecated since Qt 6.5. "
            "Use qt_add_executable instead.")
    endif()

    get_target_property(target_type ${target} TYPE)

    if (NOT "${target_type}" STREQUAL "MODULE_LIBRARY")
        message(SEND_ERROR "QT_ANDROID_GENERATE_DEPLOYMENT_SETTINGS only works on Module targets")
        return()
    endif()

    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    get_target_property(target_output_name ${target} OUTPUT_NAME)
    if (NOT target_output_name)
        set(target_output_name ${target})
    endif()

    # QtCreator requires the file name of deployment settings has no config related suffixes
    # to run androiddeployqt correctly. If we use multi-config generator for the first config
    # in a list avoid adding any configuration-specific suffixes.
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)
        set(config_suffix "$<$<NOT:$<CONFIG:${first_config_type}>>:-$<CONFIG>>")
    endif()
    set(deploy_file
      "${target_binary_dir}/android-${target}-deployment-settings${config_suffix}.json")

    set(file_contents "{\n")
    # content begin
    string(APPEND file_contents
        "   \"description\": \"This file is generated by cmake to be read by androiddeployqt and should not be modified by hand.\",\n")

    # Host Qt Android install path
    if (NOT QT_BUILDING_QT OR QT_STANDALONE_TEST_PATH)
        set(qt_path "${QT6_INSTALL_PREFIX}")
        set(android_plugin_dir_path "${qt_path}/${QT6_INSTALL_PLUGINS}/platforms")
        set(glob_expression "${android_plugin_dir_path}/*qtforandroid*${CMAKE_ANDROID_ARCH_ABI}.so")
        file(GLOB plugin_dir_files LIST_DIRECTORIES FALSE "${glob_expression}")
        if (NOT plugin_dir_files)
            message(SEND_ERROR
                "Detected Qt installation does not contain qtforandroid_${CMAKE_ANDROID_ARCH_ABI}.so in the following dir:\n"
                "${android_plugin_dir_path}\n"
                "This is most likely due to the installation not being a Qt for Android build. "
                "Please recheck your build configuration.")
            return()
        else()
            list(GET plugin_dir_files 0 android_platform_plugin_path)
            message(STATUS "Found android platform plugin at: ${android_platform_plugin_path}")
        endif()
    endif()

    _qt_internal_collect_qt_for_android_paths(file_contents)

    # Android SDK path
    file(TO_CMAKE_PATH "${ANDROID_SDK_ROOT}" android_sdk_root_native)
    string(APPEND file_contents
        "   \"sdk\": \"${android_sdk_root_native}\",\n")

    # Android SDK Build Tools Revision
    _qt_internal_android_get_target_sdk_build_tools_revision(android_sdk_build_tools_genex
        ${target})

    string(APPEND file_contents
        "   \"sdkBuildToolsRevision\": \"${android_sdk_build_tools_genex}\",\n")

    # Android NDK
    file(TO_CMAKE_PATH "${CMAKE_ANDROID_NDK}" android_ndk_root_native)
    string(APPEND file_contents
        "   \"ndk\": \"${android_ndk_root_native}\",\n")

    # Setup LLVM toolchain
    string(APPEND file_contents
        "   \"toolchain-prefix\": \"llvm\",\n")
    string(APPEND file_contents
        "   \"tool-prefix\": \"llvm\",\n")
    string(APPEND file_contents
        "   \"useLLVM\": true,\n")

    # NDK Toolchain Version
    string(APPEND file_contents
        "   \"toolchain-version\": \"${CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION}\",\n")

    # NDK Host
    string(APPEND file_contents
        "   \"ndk-host\": \"${ANDROID_NDK_HOST_SYSTEM_NAME}\",\n")

    get_target_property(qt_android_abis ${target} _qt_android_abis)
    if(NOT qt_android_abis)
        set(qt_android_abis "")
    endif()
    set(architecture_record_list "")
    foreach(abi IN LISTS qt_android_abis CMAKE_ANDROID_ARCH_ABI)
        if(abi STREQUAL "x86")
            set(arch_value "i686-linux-android")
        elseif(abi STREQUAL "x86_64")
            set(arch_value "x86_64-linux-android")
        elseif(abi STREQUAL "arm64-v8a")
            set(arch_value "aarch64-linux-android")
        elseif(abi)
            set(arch_value "arm-linux-androideabi")
        endif()
        list(APPEND architecture_record_list "\"${abi}\":\"${arch_value}\"")
    endforeach()

    list(JOIN architecture_record_list "," architecture_records)
    # Architecture
    string(APPEND file_contents
        "   \"abi\": \"${CMAKE_ANDROID_ARCH_ABI}\",\n")
    string(APPEND file_contents
        "   \"architectures\": { ${architecture_records} },\n")

    # deployment dependencies
    _qt_internal_add_android_deployment_multi_value_property(file_contents "deployment-dependencies"
        ${target} "QT_ANDROID_DEPLOYMENT_DEPENDENCIES" )

    # Extra plugins
    _qt_internal_add_android_deployment_multi_value_property(file_contents "android-extra-plugins"
        ${target} "_qt_android_native_extra_plugins" )

    # Extra libs
    _qt_internal_add_android_deployment_multi_value_property(file_contents "android-extra-libs"
        ${target} "_qt_android_native_extra_libs" )

    # Alternative path to Qt libraries on target device
    _qt_internal_add_android_deployment_property(file_contents "android-system-libs-prefix"
        ${target} "QT_ANDROID_SYSTEM_LIBS_PREFIX")

    # package source dir
    _qt_internal_add_android_deployment_property(file_contents "android-package-source-directory"
        ${target} "_qt_android_native_package_source_dir")

    # package name
    _qt_internal_add_android_deployment_property(file_contents "android-package-name"
        ${target} "QT_ANDROID_PACKAGE_NAME")

    # app name
    _qt_internal_add_android_deployment_property(file_contents "android-app-name"
        ${target} "QT_ANDROID_APP_NAME")

    # app icon
    _qt_internal_add_android_deployment_property(file_contents "android-app-icon"
        ${target} "QT_ANDROID_APP_ICON")

    # version code
    _qt_internal_add_android_deployment_property(file_contents "android-version-code"
        ${target} "QT_ANDROID_VERSION_CODE")

    # version name
    _qt_internal_add_android_deployment_property(file_contents "android-version-name"
        ${target} "QT_ANDROID_VERSION_NAME")

    # minimum SDK version
    _qt_internal_add_android_deployment_property(file_contents "android-min-sdk-version"
        ${target} "QT_ANDROID_MIN_SDK_VERSION")

    # target SDK version
    _qt_internal_add_android_deployment_property(file_contents "android-target-sdk-version"
        ${target} "QT_ANDROID_TARGET_SDK_VERSION")

    # compile SDK version
    _qt_internal_add_android_deployment_property(file_contents "android-compile-sdk-version"
        ${target} "QT_ANDROID_COMPILE_SDK_VERSION")

    # should Qt shared libs be excluded from deployment
    _qt_internal_add_android_deployment_property(file_contents "android-no-deploy-qt-libs"
        ${target} "QT_ANDROID_NO_DEPLOY_QT_LIBS")

    __qt_internal_collect_plugin_targets_from_dependencies_v2("${target}" plugin_targets)
    __qt_internal_collect_plugin_library_files_v2("${target}" "${plugin_targets}" plugin_targets)
    string(APPEND file_contents "   \"android-deploy-plugins\":\"${plugin_targets}\",\n")

    _qt_internal_generate_android_permissions_json(permissions_json_array "${target}")
    string(APPEND file_contents "   \"permissions\": ${permissions_json_array},\n")

    # App binary
    string(APPEND file_contents
        "   \"application-binary\": \"${target_output_name}\",\n")

    # App command-line arguments
    if (QT_ANDROID_APPLICATION_ARGUMENTS)
        string(APPEND file_contents
            "   \"android-application-arguments\": \"${QT_ANDROID_APPLICATION_ARGUMENTS}\",\n")
    endif()

    if(COMMAND _qt_internal_generate_android_qml_deployment_settings)
        _qt_internal_generate_android_qml_deployment_settings(file_contents ${target})
    else()
        string(APPEND file_contents
            "   \"qml-skip-import-scanning\": true,\n")
    endif()

    # Override rcc binary path
    _qt_internal_add_tool_to_android_deployment_settings(file_contents rcc "rcc-binary" "${target}")

    # Extra prefix paths
    foreach(prefix IN LISTS CMAKE_FIND_ROOT_PATH)
        if (NOT "${prefix}" STREQUAL "${qt_android_install_dir_native}"
            AND NOT "${prefix}" STREQUAL "${android_ndk_root_native}")
            file(TO_CMAKE_PATH "${prefix}" prefix)
            list(APPEND extra_prefix_list "\"${prefix}\"")
        endif()
    endforeach()
    string (REPLACE ";" "," extra_prefix_list "${extra_prefix_list}")
    string(APPEND file_contents
        "   \"extraPrefixDirs\" : [ ${extra_prefix_list} ],\n")

    # Create an empty target for the cases when we need to generate deployment setting but
    # qt_finalize_project is never called.
    if(NOT TARGET _qt_internal_apk_dependencies AND NOT QT_NO_COLLECT_BUILD_TREE_APK_DEPS)
        add_custom_target(_qt_internal_apk_dependencies)
    endif()

    # Extra library paths that could be used as a dependency lookup path by androiddeployqt.
    #
    # Unlike 'extraPrefixDirs', the 'extraLibraryDirs' key doesn't expect the 'lib' subfolder
    # when looking for dependencies.
    # TODO: add a public target property accessible from user space
    _qt_internal_add_android_deployment_list_property(file_contents "extraLibraryDirs"
        ${target} "_qt_android_extra_library_dirs"
        _qt_internal_apk_dependencies "_qt_android_extra_library_dirs"
    )

    if(QT_FEATURE_zstd)
        set(is_zstd_enabled "true")
    else()
        set(is_zstd_enabled "false")
    endif()
    string(APPEND file_contents
        "   \"zstdCompression\": ${is_zstd_enabled},\n")

    if(QT_ANDROID_GENERATE_JAVA_QTQUICKVIEW_CONTENTS)
        set(is_generate_java_qtquickview_contents "true")
    else()
        set(is_generate_java_qtquickview_contents "false")
    endif()
    string(APPEND file_contents
        "   \"generate-java-qtquickview-contents\": ${is_generate_java_qtquickview_contents},\n")
    # Last item in json file

    # base location of stdlibc++, will be suffixed by androiddeploy qt
    # Sysroot is set by Android toolchain file and is composed of ANDROID_TOOLCHAIN_ROOT.
    set(android_ndk_stdlib_base_path "${CMAKE_SYSROOT}/usr/lib/")
    string(APPEND file_contents
        "   \"stdcpp-path\": \"${android_ndk_stdlib_base_path}\"\n")

    # content end
    string(APPEND file_contents "}\n")

    file(GENERATE OUTPUT ${deploy_file} CONTENT "${file_contents}")

    set_target_properties(${target}
        PROPERTIES
            QT_ANDROID_DEPLOYMENT_SETTINGS_FILE ${deploy_file}
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_android_generate_deployment_settings)
        qt6_android_generate_deployment_settings(${ARGV})
    endfunction()
endif()

function(_qt_internal_add_android_permission target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Empty or invalid target for adding Android permission: (${target})")
    endif()

    cmake_parse_arguments(arg "" "NAME" "ATTRIBUTES" ${ARGN})

    if(NOT arg_NAME)
        message(FATAL_ERROR "NAME for adding Android permission cannot be empty (${target})")
    endif()

    set(permission_entry "${arg_NAME}")

    if(arg_ATTRIBUTES)
        # Permission with additional attributes
        list(LENGTH arg_ATTRIBUTES attributes_len)
        math(EXPR attributes_modulus "${attributes_len} % 2")
        if(NOT (attributes_len GREATER 1 AND attributes_modulus EQUAL 0))
            message(FATAL_ERROR "Android permission: ${arg_NAME} attributes: ${arg_ATTRIBUTES} must"
                                " be name-value pairs (for example: minSdkVersion 30)")
        endif()
        # Combine name-value pairs
        set(index 0)
        set(attributes "")
        while(index LESS attributes_len)
            list(GET arg_ATTRIBUTES ${index} name)
            math(EXPR index "${index} + 1")
            list(GET arg_ATTRIBUTES ${index} value)
            string(APPEND attributes "android:${name}=\'${value}\' ")
            math(EXPR index "${index} + 1")
        endwhile()
        set(permission_entry "${permission_entry}\;${attributes}")
    endif()

    # Append the permission to the target's property
    set_property(TARGET ${target} APPEND PROPERTY QT_ANDROID_PERMISSIONS "${permission_entry}")
endfunction()

function(qt6_add_android_permission target)
    _qt_internal_add_android_permission(${ARGV})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_android_permission target)
        qt6_add_android_permission(${ARGV})
    endfunction()
endif()

function(qt6_android_apply_arch_suffix target)
    get_target_property(called_from_qt_impl
        ${target} _qt_android_apply_arch_suffix_called_from_qt_impl)
    if(called_from_qt_impl)
        # Don't show deprecation when called by our own function implementations.
    else()
        message(DEPRECATION
            "Calling qt_android_apply_arch_suffix directly is deprecated since Qt 6.5. "
            "Use qt_add_executable or qt_add_library instead.")
    endif()

    get_target_property(target_type ${target} TYPE)
    if (target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
        set_property(TARGET "${target}" PROPERTY SUFFIX "_${CMAKE_ANDROID_ARCH_ABI}.so")
    elseif (target_type STREQUAL "STATIC_LIBRARY")
        set_property(TARGET "${target}" PROPERTY SUFFIX "_${CMAKE_ANDROID_ARCH_ABI}.a")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_android_apply_arch_suffix)
        qt6_android_apply_arch_suffix(${ARGV})
    endfunction()
endif()

# Add custom target to package the APK
function(qt6_android_add_apk_target target)
    # Avoid calling qt6_android_add_apk_target twice
    get_property(apk_targets GLOBAL PROPERTY _qt_apk_targets)
    if("${target}" IN_LIST apk_targets)
        return()
    endif()

    get_target_property(android_executable_finalizer_called
        ${target} _qt_android_executable_finalizer_called)

    if(android_executable_finalizer_called)
        # Don't show deprecation when called by our own function implementations.
    else()
        message(DEPRECATION
            "Calling qt_android_add_apk_target directly is deprecated since Qt 6.5. "
            "Use qt_add_executable instead.")
    endif()

    get_target_property(deployment_file ${target} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    if (NOT deployment_file)
        message(FATAL_ERROR "Target ${target} is not a valid android executable target\n")
    endif()
    # Use genex to get path to the deployment settings, the above check only to confirm that
    # qt6_android_add_apk_target is called on an android executable target.
    string(JOIN "" deployment_file
        "$<GENEX_EVAL:"
            "$<TARGET_PROPERTY:${target},QT_ANDROID_DEPLOYMENT_SETTINGS_FILE>"
        ">"
    )

    # Make global apk and aab targets depend on the current apk target.
    if(TARGET aab)
        add_dependencies(aab ${target}_make_aab)
    endif()
    if(TARGET aar)
        add_dependencies(aar ${target}_make_aar)
    endif()
    if(TARGET apk)
        add_dependencies(apk ${target}_make_apk)
        _qt_internal_create_global_apk_all_target_if_needed()
    endif()

    _qt_internal_android_get_deployment_tool(deployment_tool)

    # No need to use genex for the BINARY_DIR since it's read-only.
    get_target_property(target_binary_dir ${target} BINARY_DIR)

    if("$CACHE{QT_USE_TARGET_ANDROID_BUILD_DIR}" AND
       "$CACHE{QT_USE_TARGET_ANDROID_BUILD_DIR}" STREQUAL "${QT_USE_TARGET_ANDROID_BUILD_DIR}")
        set(apk_final_dir "${target_binary_dir}/android-build-${target}")
    else()
        if(QT_USE_TARGET_ANDROID_BUILD_DIR)
            message(WARNING "QT_USE_TARGET_ANDROID_BUILD_DIR needs to be set in CACHE")
        endif()

        get_property(known_android_build GLOBAL PROPERTY _qt_internal_known_android_build_dir)
        get_property(already_warned GLOBAL PROPERTY _qt_internal_already_warned_android_build_dir)
        set(apk_final_dir "${target_binary_dir}/android-build")
        if(NOT QT_SKIP_ANDROID_BUILD_DIR_CHECK AND "${apk_final_dir}" IN_LIST known_android_build
            AND NOT "${apk_final_dir}" IN_LIST already_warned)
            message(WARNING "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt contains multiple"
                " Qt Android executable targets. This can lead to mixing of deployment artifacts"
                " of targets defined there. Setting QT_USE_TARGET_ANDROID_BUILD_DIR=TRUE"
                " allows building multiple executable targets within a single CMakeLists.txt."
                " Note: This option is not supported by Qt Creator versions older than 13."
                " Set QT_SKIP_ANDROID_BUILD_DIR_CHECK=TRUE to suppress this warning."
            )
            set_property(GLOBAL APPEND PROPERTY _qt_internal_already_warned_android_build_dir
                "${apk_final_dir}")
        else()
            set_property(GLOBAL APPEND PROPERTY
                _qt_internal_known_android_build_dir "${apk_final_dir}")
        endif()
    endif()

    set(apk_file_name "${target}.apk")
    set(aar_file_name "${target}.aar")
    set(dep_file_name "${target}.d")
    set(apk_final_file_path "${apk_final_dir}/${apk_file_name}")
    set(aar_final_file_path "${apk_final_dir}/${aar_file_name}")
    set(dep_file_path "${apk_final_dir}/${dep_file_name}")
    set(target_file_copy_relative_path
        "libs/${CMAKE_ANDROID_ARCH_ABI}/$<TARGET_FILE_NAME:${target}>")

    set(extra_deps "")

    _qt_internal_android_get_use_terminal_for_deployment(uses_terminal)

    # Plugins still might be added after creating the deployment targets.
    if(NOT TARGET qt_internal_plugins)
        add_custom_target(qt_internal_plugins)
    endif()
    # Before running androiddeployqt, we need to make sure all plugins are built.
    list(APPEND extra_deps qt_internal_plugins)

    # This target is used by Qt Creator's Android support and by the ${target}_make_apk target
    # in case DEPFILEs are not supported.
    # Also the target is used to copy the library that belongs to ${target} when building multi-abi
    # apk to the abi-specific directory.
    _qt_internal_copy_file_if_different_command(copy_command
        "$<TARGET_FILE:${target}>"
        "${apk_final_dir}/${target_file_copy_relative_path}"
    )

    if(QT_FEATURE_sanitize_address)
        _qt_internal_android_find_asan_runtime_lib(asan_lib_path)
        _qt_internal_android_find_asan_wrap_sh(asan_wrap_sh_path)

        if(asan_lib_path AND asan_wrap_sh_path)
            get_filename_component(asan_lib_basename "${asan_lib_path}" NAME)
            set(asan_lib_dest
                "${apk_final_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}/${asan_lib_basename}")
            add_custom_command(
                OUTPUT "${asan_lib_dest}"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${asan_lib_path}"
                    "${asan_lib_dest}"
                DEPENDS "${asan_lib_path}"
                COMMENT "Copying Address Sanitizer library to apk folder"
            )

            # The wrap.sh has to go under resources/lib and not libs/
            # See https://developer.android.com/ndk/guides/asan#building
            set(asan_wrap_sh_dest
                "${apk_final_dir}/resources/lib/${CMAKE_ANDROID_ARCH_ABI}/wrap.sh")
            add_custom_command(
                OUTPUT "${asan_wrap_sh_dest}"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${asan_wrap_sh_path}"
                    "${asan_wrap_sh_dest}"
                DEPENDS "${asan_wrap_sh_path}"
                COMMENT "Copying Address Sanitizer wrap.sh to apk folder"
            )
        endif()
    endif()

    add_custom_target(${target}_prepare_apk_dir ALL
        COMMAND ${copy_command}
        COMMENT "Copying ${target} binary to apk folder"
        DEPENDS "${asan_lib_dest}" "${asan_wrap_sh_dest}"
        ${uses_terminal}
    )

    add_dependencies(${target}_prepare_apk_dir ${target} ${extra_deps})

    set(sign_apk "")
    if(QT_ANDROID_SIGN_APK)
        set(sign_apk "--sign")
    endif()
    set(sign_aab "")
    if(QT_ANDROID_SIGN_AAB)
        set(sign_aab "--sign")
    endif()

    set(extra_args "")
    if(QT_INTERNAL_NO_ANDROID_RCC_BUNDLE_CLEANUP)
        list(APPEND extra_args "--no-rcc-bundle-cleanup")
    endif()
    if(QT_ENABLE_VERBOSE_DEPLOYMENT)
        list(APPEND extra_args "--verbose")
    endif()

    _qt_internal_android_get_deployment_type_option(android_deployment_type_option "--release" "")
    list(APPEND extra_args "${android_deployment_type_option}")

    _qt_internal_check_depfile_support(has_depfile_support)

    if(has_depfile_support)
        cmake_policy(PUSH)
        if(POLICY CMP0116)
            # Without explicitly setting this policy to NEW, we get a warning
            # even though we ensure there's actually no problem here.
            # See https://gitlab.kitware.com/cmake/cmake/-/issues/21959
            cmake_policy(SET CMP0116 NEW)
            set(relative_to_dir ${CMAKE_CURRENT_BINARY_DIR})
        else()
            set(relative_to_dir ${CMAKE_BINARY_DIR})
        endif()

        # Add custom command that creates the apk and triggers rebuild if files listed in
        # ${dep_file_path} are changed.
        add_custom_command(OUTPUT "${apk_final_file_path}"
            COMMAND "${deployment_tool}"
                --input "${deployment_file}"
                --output "${apk_final_dir}"
                --apk "${apk_final_file_path}"
                --depfile "${dep_file_path}"
                --builddir "${relative_to_dir}"
                ${extra_args}
                ${sign_apk}
            COMMENT "Creating APK for ${target}"
            DEPENDS "${target}" "${deployment_file}" ${extra_deps} ${target}_prepare_apk_dir
            DEPFILE "${dep_file_path}"
            VERBATIM
            ${uses_terminal}
        )

        # Add custom command that creates the aar and triggers rebuild if files listed in
        # ${dep_file_path} are changed.
        add_custom_command(OUTPUT "${aar_final_file_path}"
            COMMAND "${deployment_tool}"
                --input "${deployment_file}"
                --output "${apk_final_dir}"
                --apk "${aar_final_file_path}"
                --depfile "${dep_file_path}"
                --builddir "${relative_to_dir}"
                --build-aar
                ${extra_args}
            COMMENT "Creating AAR for ${target}"
            DEPENDS "${target}" "${deployment_file}" ${extra_deps} ${target}_prepare_apk_dir
            DEPFILE "${dep_file_path}"
            VERBATIM
            ${uses_terminal}
        )
        cmake_policy(POP)

        # Create a ${target}_make_apk target to trigger the apk build.
        add_custom_target(${target}_make_apk DEPENDS "${apk_final_file_path}")
        # Create a ${target}_make_aar target to trigger the aar build.
        add_custom_target(${target}_make_aar DEPENDS "${aar_final_file_path}")
    else()
        add_custom_target(${target}_make_apk
            COMMAND  ${deployment_tool}
                --input ${deployment_file}
                --output ${apk_final_dir}
                --apk ${apk_final_file_path}
                ${extra_args}
                ${sign_apk}
            COMMENT "Creating APK for ${target}"
            VERBATIM
            ${uses_terminal}
        )

        add_custom_target(${target}_make_aar
            COMMAND  ${deployment_tool}
                --input ${deployment_file}
                --output ${apk_final_dir}
                --apk ${aar_final_file_path}
                --build-aar
                ${extra_args}
            COMMENT "Creating AAR for ${target}"
            VERBATIM
            ${uses_terminal}
        )

        add_dependencies(${target}_make_apk ${target}_prepare_apk_dir)
        add_dependencies(${target}_make_aar ${target}_prepare_apk_dir)
    endif()

    # Add target triggering AAB creation. Since the _make_aab target is not added to the ALL
    # set, we may avoid dependency check for it and admit that the target is "always out
    # of date".
    add_custom_target(${target}_make_aab
        COMMAND  ${deployment_tool}
            --input ${deployment_file}
            --output ${apk_final_dir}
            --apk ${apk_final_file_path}
            --aab
            ${sign_aab}
            ${extra_args}
        COMMENT "Creating AAB for ${target}"
        ${uses_terminal}
    )
    add_dependencies(${target}_make_aab ${target}_prepare_apk_dir)

    if(QT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT)
        # When building per-ABI external projects we only need to copy ABI-specific libraries and
        # resources to the "main" ABI android build folder.

        if("${QT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR}" STREQUAL "")
            message(FATAL_ERROR "QT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR is not set when building"
                " ABI specific external project. This should not happen and might mean an issue"
                " in Qt. Please report a bug with CMake traces attached.")
        endif()
        # Assume that external project mirrors build structure of the top-level ABI project and
        # replace the build root when specifying the output directory of androiddeployqt.
        file(RELATIVE_PATH androiddeployqt_output_path "${CMAKE_BINARY_DIR}" "${apk_final_dir}")
        set(androiddeployqt_output_path
            "${QT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR}/${androiddeployqt_output_path}")
        _qt_internal_copy_file_if_different_command(copy_command
            "$<TARGET_FILE:${target}>"
            "${androiddeployqt_output_path}/${target_file_copy_relative_path}"
        )
        if(has_depfile_support)
            set(deploy_android_deps_dir "${apk_final_dir}/${target}_deploy_android")
            set(timestamp_file "${deploy_android_deps_dir}/timestamp")
            set(dep_file "${deploy_android_deps_dir}/${target}.d")
            add_custom_command(OUTPUT "${timestamp_file}"
                DEPENDS ${target} ${extra_deps}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${deploy_android_deps_dir}"
                COMMAND ${CMAKE_COMMAND} -E touch "${timestamp_file}"
                COMMAND ${copy_command}
                COMMAND  ${deployment_tool}
                    --input ${deployment_file}
                    --output ${androiddeployqt_output_path}
                    --copy-dependencies-only
                    ${extra_args}
                    --depfile "${dep_file}"
                    --builddir "${CMAKE_BINARY_DIR}"
                COMMENT "Resolving ${CMAKE_ANDROID_ARCH_ABI} dependencies for the ${target} APK"
                DEPFILE "${dep_file}"
                VERBATIM
                ${uses_terminal}
            )
            add_custom_target(qt_internal_${target}_copy_apk_dependencies
                DEPENDS "${timestamp_file}")
        else()
            add_custom_target(qt_internal_${target}_copy_apk_dependencies
                DEPENDS ${target} ${extra_deps}
                COMMAND ${copy_command}
                COMMAND  ${deployment_tool}
                    --input ${deployment_file}
                    --output ${androiddeployqt_output_path}
                    --copy-dependencies-only
                    ${extra_args}
                COMMENT "Resolving ${CMAKE_ANDROID_ARCH_ABI} dependencies for the ${target} APK"
                ${uses_terminal}
            )
        endif()
    else()
        add_dependencies(${target}_prepare_apk_dir
            ${target}_copy_apk_dependencies)
    endif()

    set_property(GLOBAL APPEND PROPERTY _qt_apk_targets ${target})
    _qt_internal_collect_apk_dependencies_defer()
    _qt_internal_collect_apk_imported_dependencies_defer("${target}")
endfunction()

function(_qt_internal_create_global_android_targets)
    macro(_qt_internal_create_global_android_targets_impl target)
        string(TOUPPER "${target}" target_upper)
        if(NOT QT_NO_GLOBAL_${target_upper}_TARGET)
            if(NOT TARGET ${target})
                add_custom_target(${target} COMMENT "Building all apks")
            endif()
        endif()
    endmacro()

    # Create a top-level "apk" target for convenience, so that users can call 'ninja apk'.
    # It will trigger building all the apk build targets that are added as part of the project.
    # Allow opting out.
    _qt_internal_create_global_android_targets_impl(apk)

    # Create a top-level "aab" target for convenience, so that users can call 'ninja aab'.
    # It will trigger building all the apk build targets that are added as part of the project.
    # Allow opting out.
    _qt_internal_create_global_android_targets_impl(aab)

    # Create a top-level "aar" target for convenience, so that users can call 'ninja aar'.
    # It will trigger building all the aar build targets that are added as part of the project.
    # Allow opting out.
    _qt_internal_create_global_android_targets_impl(aar)
endfunction()

# The function collects all known non-imported shared libraries that are created in the build tree.
# It uses the CMake DEFER CALL feature if the CMAKE_VERSION is greater
# than or equal to 3.19.
# Note: Users that use cmake version less that 3.19 need to call qt_finalize_project
# in the end of a project's top-level CMakeLists.txt.
function(_qt_internal_collect_apk_dependencies_defer)
    # User opted-out the functionality
    if(QT_NO_COLLECT_BUILD_TREE_APK_DEPS)
        return()
    endif()

    get_property(is_called GLOBAL PROPERTY _qt_is_collect_apk_dependencies_defer_called)
    if(is_called) # Already scheduled
        return()
    endif()
    set_property(GLOBAL PROPERTY _qt_is_collect_apk_dependencies_defer_called TRUE)

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
        cmake_language(EVAL CODE "cmake_language(DEFER DIRECTORY \"${CMAKE_SOURCE_DIR}\"
            CALL _qt_internal_collect_apk_dependencies)")
    else()
        # User don't want to see the warning
        if(NOT QT_NO_WARN_BUILD_TREE_APK_DEPS)
            message(WARNING
                    "The CMake version you use is less than 3.19. APK dependencies, that are a"
                    " part of the project tree, might not be collected correctly."
                    " Please call qt_finalize_project in the end of a project's top-level"
                    " CMakeLists.txt file to make sure that all the APK dependencies are"
                    " collected correctly."
                    " You can pass -DQT_NO_WARN_BUILD_TREE_APK_DEPS=ON when configuring the project"
                    " to silence the warning.")
        endif()
    endif()
endfunction()

# The function collects project-built shared libraries that might be dependencies for
# the main apk targets. It stores their locations in a global custom target property.
function(_qt_internal_collect_apk_dependencies)
    # User opted-out the functionality
    if(QT_NO_COLLECT_BUILD_TREE_APK_DEPS)
        return()
    endif()

    get_property(is_called GLOBAL PROPERTY _qt_is_collect_apk_dependencies_called)
    if(is_called)
        return()
    endif()
    set_property(GLOBAL PROPERTY _qt_is_collect_apk_dependencies_called TRUE)

    get_property(apk_targets GLOBAL PROPERTY _qt_apk_targets)

    _qt_internal_collect_buildsystem_targets(libs
        "${CMAKE_SOURCE_DIR}" INCLUDE SHARED_LIBRARY MODULE_LIBRARY)
    list(REMOVE_DUPLICATES libs)

    if(NOT TARGET qt_internal_plugins)
        add_custom_target(qt_internal_plugins)
    endif()

    foreach(lib IN LISTS libs)
        if(NOT lib IN_LIST apk_targets)
            list(APPEND extra_library_dirs "$<TARGET_FILE_DIR:${lib}>")
            get_target_property(target_type ${lib} TYPE)
            # We collect all MODULE_LIBRARY targets since target APK may have implicit dependency
            # to the plugin that will cause the runtime issue. Plugins that were added using
            # qt6_add_plugin should be already added to the qt_internal_plugins dependency list,
            # but it's ok to re-add them.
            if(target_type STREQUAL "MODULE_LIBRARY")
                add_dependencies(qt_internal_plugins ${lib})
            endif()
        endif()
    endforeach()

    if(NOT TARGET _qt_internal_apk_dependencies)
        add_custom_target(_qt_internal_apk_dependencies)
    endif()
    set_target_properties(_qt_internal_apk_dependencies PROPERTIES
        _qt_android_extra_library_dirs "${extra_library_dirs}"
    )
endfunction()

# This function collects all imported shared libraries that might be dependencies for
# the main apk targets. The actual collection is deferred until the target's directory scope
# is processed.
# The function requires CMake 3.21 or later.
function(_qt_internal_collect_apk_imported_dependencies_defer target)
    # User opted-out of the functionality.
    if(QT_NO_COLLECT_IMPORTED_TARGET_APK_DEPS)
        return()
    endif()

    get_target_property(target_source_dir "${target}" SOURCE_DIR)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
        cmake_language(EVAL CODE "cmake_language(DEFER DIRECTORY \"${target_source_dir}\"
            CALL _qt_internal_collect_apk_imported_dependencies \"${target}\")")
    endif()
endfunction()

# This function collects imported shared libraries that might be dependencies for
# the main apk targets. It stores their locations on a custom target property for the given target.
# The function requires CMake 3.21 or later.
function(_qt_internal_collect_apk_imported_dependencies target)
    # User opted-out the functionality
    if(QT_NO_COLLECT_IMPORTED_TARGET_APK_DEPS)
        return()
    endif()

    get_target_property(target_source_dir "${target}" SOURCE_DIR)
    _qt_internal_collect_imported_shared_libraries_recursive(libs "${target_source_dir}")
    list(REMOVE_DUPLICATES libs)

    foreach(lib IN LISTS libs)
        list(APPEND extra_library_dirs "$<TARGET_FILE_DIR:${lib}>")
    endforeach()

    set_property(TARGET "${target}" APPEND PROPERTY
        _qt_android_extra_library_dirs "${extra_library_dirs}"
    )
endfunction()

# This function recursively walks the current directory and its parent directories to collect
# imported shared library targets.
# The recursion goes upwards instead of downwards because imported targets are usually not global,
# and we can't call get_target_property() on a target which is not available in the current
# directory or parent scopes.
# We also can't cache parent directories because the imported targets in a parent directory
# might change in-between collection calls.
# The function requires CMake 3.21 or later.
function(_qt_internal_collect_imported_shared_libraries_recursive out_var subdir)
    set(result "")

    get_directory_property(imported_targets DIRECTORY "${subdir}" IMPORTED_TARGETS)
    foreach(imported_target IN LISTS imported_targets)
        get_target_property(target_type "${imported_target}" TYPE)
        if(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
            # If the target has the _qt_package_version property set, it means it's an
            # 'official' qt target like a module or plugin, so we don't want to add it
            # to the list of extra paths to scan for in androiddeployqt, because they are
            # already handled via the regular 'qt' code path in the androiddeployqt.
            # Thus this will pick up only non-qt 3rd party targets.
            get_target_property(qt_package_version "${imported_target}" _qt_package_version)
            if(NOT qt_package_version)
                list(APPEND result "${imported_target}")
            endif()
        endif()
    endforeach()

    get_directory_property(parent_dir DIRECTORY "${subdir}" PARENT_DIRECTORY)
    if(parent_dir)
        _qt_internal_collect_imported_shared_libraries_recursive(result_inner "${parent_dir}")
    endif()

    list(APPEND result ${result_inner})
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# This function allows deciding whether apks should be built as part of the ALL target at first
# add_executable call point, rather than when the 'apk' target is created as part of the
# find_package(Core) call.
#
# It does so by creating a custom 'apk_all' target as an implementation detail.
#
# This is needed to ensure that the decision is made only when the value of QT_BUILDING_QT is
# available, which is defined in qt_repo_build() -> include(QtSetup), which is included after the
# execution of _qt_internal_create_global_apk_target.
function(_qt_internal_create_global_apk_all_target_if_needed)
    if(TARGET apk AND NOT TARGET apk_all)
        # Some Qt tests helper executables have their apk build process failing.
        # qt_internal_add_executables that are excluded from ALL should also not have apks built
        # for them.
        # Don't build apks by default when doing a Qt build.
        set(skip_add_to_all FALSE)
        if(QT_BUILDING_QT)
            set(skip_add_to_all TRUE)
        endif()

        option(QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL
            "Skip building apks as part of the default 'ALL' target" ${skip_add_to_all})

        set(part_of_all "ALL")
        if(QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL)
            set(part_of_all "")
        endif()

        add_custom_target(apk_all ${part_of_all})
        add_dependencies(apk_all apk)
    endif()
endfunction()

# The function converts the target property to a json record and appends it to the output
# variable.
function(_qt_internal_add_android_deployment_property out_var json_key target property)
    set(property_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${property}>>")
    string(APPEND ${out_var}
        "$<$<BOOL:${property_genex}>:"
            "   \"${json_key}\": \"${property_genex}\"\,\n"
        ">"
    )

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# The function converts the list properties of the targets to a json list record and appends it
# to the output variable.
# _qt_internal_add_android_deployment_list_property(out_var json_key [<target> <property>]...)
# The generated JSON object is the normal JSON array, e.g.:
#    "qml-root-path": ["qml/root/path1","qml/root/path2"],
function(_qt_internal_add_android_deployment_list_property out_var json_key)
    list(LENGTH ARGN argn_count)
    math(EXPR is_odd "${argn_count} % 2")
    if(is_odd)
        message(FATAL_ERROR "Invalid argument count")
    endif()

    set(skip_next FALSE)
    set(property_genex "")
    math(EXPR last_index "${argn_count} - 1")
    foreach(idx RANGE ${last_index})
        if(skip_next)
            set(skip_next FALSE)
            continue()
        endif()
        set(skip_next TRUE)

        math(EXPR property_idx "${idx} + 1")
        list(GET ARGN ${idx} target)
        list(GET ARGN ${property_idx} property)

        # Add comma if we have at least one element from the previous iteration
        if(property_genex)
            set(add_comma_genex
                "$<$<BOOL:${property_genex}>:$<COMMA>>"
            )
        endif()

        set(property_genex
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${property}>>"
        )
        set(add_quote_genex
            "$<$<BOOL:${property_genex}>:\">"
        )

        # Add comma only if next property genex contains non-empty value.
        set(add_comma_genex "$<$<BOOL:${property_genex}>:${add_comma_genex}>")
        string(JOIN "" list_join_genex
            "${list_join_genex}"
            "${add_comma_genex}${add_quote_genex}"
                "$<JOIN:"
                    "${property_genex},"
                    "\"$<COMMA>\""
                ">"
            "${add_quote_genex}"
        )
    endforeach()

    string(APPEND ${out_var}
        "   \"${json_key}\" : [ ${list_join_genex} ],\n")
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# The function converts the target list property to a json multi-value string record and appends it
# to the output variable.
# The generated JSON object is a simple string with the list property items separated by commas,
# e.g:
#    "android-extra-plugins": "plugin1,plugin2",
function(_qt_internal_add_android_deployment_multi_value_property out_var json_key target property)
    set(property_genex
        "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${property}>>"
    )
    string(JOIN "" list_join_genex
        "$<JOIN:"
            "${property_genex},"
            "$<COMMA>"
        ">"
    )
    string(APPEND ${out_var}
        "$<$<BOOL:${property_genex}>:"
            "   \"${json_key}\" : \"${list_join_genex}\",\n"
        ">"
    )

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# The function converts paths to the CMake format to make them acceptable for JSON.
# It doesn't overwrite public properties, but instead writes formatted values to internal
# properties.
function(_qt_internal_android_format_deployment_paths target)
    if(QT_BUILD_STANDALONE_TESTS OR QT_BUILDING_QT OR QT_INTERNAL_IS_STANDALONE_TEST)
        set(android_deployment_paths_policy NEW)
    else()
        set(policy_path_properties
            QT_QML_IMPORT_PATH
            QT_QML_ROOT_PATH
            QT_ANDROID_PACKAGE_SOURCE_DIR
            QT_ANDROID_EXTRA_PLUGINS
            QT_ANDROID_EXTRA_LIBS
        )

        # Check if any of paths contains the value and stop the evaluation if all properties are
        # empty or -NOTFOUND
        set(has_android_paths FALSE)
        foreach(prop_name IN LISTS policy_path_properties)
            get_target_property(prop_value ${target} ${prop_name})
            if(prop_value)
                set(has_android_paths TRUE)
                break()
            endif()
        endforeach()
        if(has_android_paths)
            __qt_internal_setup_policy(QTP0002 "6.6.0"
                "Target properties that specify android-specific paths may contain generator\
                expressions but they must evaluate to valid JSON strings.\
                Check https://doc.qt.io/qt-6/qt-cmake-policy-qtp0002.html for policy details."
            )
            qt6_policy(GET QTP0002 android_deployment_paths_policy)
        endif()
    endif()
    if(android_deployment_paths_policy STREQUAL "NEW")
        # When building standalone tests or Qt itself we obligate developers to not use
        # windows paths when setting QT_* properties below, so their values are used as is when
        # generating deployment settings.
        string(JOIN "" qml_root_path_genex
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_QML_ROOT_PATH>>"
            "$<"
                "$<AND:"
                    "$<BOOL:$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_QML_ROOT_PATH>>>,"
                    "$<BOOL:$<GENEX_EVAL:$<TARGET_PROPERTY:${target},_qt_internal_qml_root_path>>>"
                ">:;"
            ">"
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},_qt_internal_qml_root_path>>"
        )
        set_target_properties(${target} PROPERTIES
            _qt_native_qml_import_paths
                "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_QML_IMPORT_PATH>>"
            _qt_android_native_qml_root_paths
                "${qml_root_path_genex}"
            _qt_android_native_package_source_dir
                "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_ANDROID_PACKAGE_SOURCE_DIR>>"
            _qt_android_native_extra_plugins
                "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_ANDROID_EXTRA_PLUGINS>>"
            _qt_android_native_extra_libs
                "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_ANDROID_EXTRA_LIBS>>"
        )
    else()
        # User projects still may use windows paths inside the QT_* properties below, with
        # obligation to run the finalizer code.
        _qt_internal_android_format_deployment_path_property(${target}
            QT_QML_IMPORT_PATH _qt_native_qml_import_paths)

        _qt_internal_android_format_deployment_path_property(${target}
            QT_QML_ROOT_PATH _qt_android_native_qml_root_paths)

        _qt_internal_android_format_deployment_path_property(${target}
            _qt_internal_qml_root_path _qt_android_native_qml_root_paths APPEND)

        _qt_internal_android_format_deployment_path_property(${target}
            QT_ANDROID_PACKAGE_SOURCE_DIR _qt_android_native_package_source_dir)

        _qt_internal_android_format_deployment_path_property(${target}
            QT_ANDROID_EXTRA_PLUGINS _qt_android_native_extra_plugins)

        _qt_internal_android_format_deployment_path_property(${target}
            QT_ANDROID_EXTRA_LIBS _qt_android_native_extra_libs)
    endif()
endfunction()

# The function converts the value of target property to JSON compatible path and writes the
# result to out_property. Property might be either single value, semicolon separated list or system
# path spec.
# The APPEND argument controls the property is set. The argument should be added after all
# the required arguments.
function(_qt_internal_android_format_deployment_path_property target property out_property)
    set(should_append "")
    if(ARGC EQUAL 4)
        if("${ARGV3}" STREQUAL "APPEND")
            set(should_append APPEND)
        else()
            message(FATAL_ERROR "Unexpected argument ${ARGV3}")
        endif()
    elseif(ARGC GREATER 4)
        message(FATAL_ERROR "Unexpected arguments ${ARGN}")
    endif()

    get_target_property(_paths ${target} ${property})
    if(_paths)
        set(native_paths "")
        foreach(_path IN LISTS _paths)
            file(TO_CMAKE_PATH "${_path}" _path)
            list(APPEND native_paths "${_path}")
        endforeach()
        set_property(TARGET ${target} ${should_append} PROPERTY
            ${out_property} "${native_paths}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_android_add_apk_target)
        qt6_android_add_apk_target(${ARGV})
    endfunction()
endif()

# The function returns the installation path to Qt for Android for the specified ${abi}.
# By default function expects to find a layout as is installed by the Qt online installer:
#   Qt_install_dir/Version/
#   |__  gcc_64
#   |__  android_arm64_v8a
#   |__  android_armv7
#   |__  android_x86
#   |__  android_x86_64
function(_qt_internal_get_android_abi_prefix_path out_path abi)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL abi)
        # Required to build unit tests in developer build
        if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
            set(${out_path} "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
        else()
            set(${out_path} "${QT6_INSTALL_PREFIX}")
        endif()
    elseif(DEFINED QT_PATH_ANDROID_ABI_${abi})
        get_filename_component(${out_path} "${QT_PATH_ANDROID_ABI_${abi}}" ABSOLUTE)
    else()
        # Map the ABI value to the Qt for Android folder.
        if (abi STREQUAL "x86")
            set(abi_directory_suffix "${abi}")
        elseif (abi STREQUAL "x86_64")
            set(abi_directory_suffix "${abi}")
        elseif (abi STREQUAL "arm64-v8a")
            set(abi_directory_suffix "arm64_v8a")
        else()
            set(abi_directory_suffix "armv7")
        endif()

        get_filename_component(${out_path}
            "${_qt_cmake_dir}/../../../android_${abi_directory_suffix}" ABSOLUTE)
    endif()
    set(${out_path} "${${out_path}}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_android_abi_cmake_dir_path out_path abi)
    if(DEFINED QT_ANDROID_PATH_CMAKE_DIR_${abi})
        set(cmake_dir "${QT_ANDROID_PATH_CMAKE_DIR_${abi}}")
    else()
        _qt_internal_get_android_abi_prefix_path(prefix_path ${abi})
        if((PROJECT_NAME STREQUAL "QtBase" OR QT_SUPERBUILD) AND QT_BUILDING_QT AND
            NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_INTERNAL_IS_STANDALONE_TEST)
            set(cmake_dir "${QT_CONFIG_BUILD_DIR}")
        else()
            string(TOUPPER "${QT_CMAKE_EXPORT_NAMESPACE}" export_namespace_upper)
            set(cmake_dir "${prefix_path}/${${export_namespace_upper}_INSTALL_LIBS}/cmake")
        endif()
    endif()

    set(${out_path} "${cmake_dir}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_android_abi_toolchain_path out_path abi)
    set(toolchain_path "${QT_CMAKE_EXPORT_NAMESPACE}/qt.toolchain.cmake")
    _qt_internal_get_android_abi_cmake_dir_path(cmake_dir ${abi})
    get_filename_component(toolchain_path
        "${cmake_dir}/${toolchain_path}" ABSOLUTE)
    set(${out_path} "${toolchain_path}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_android_abi_subdir_path out_path subdir abi)
    set(install_paths_path "${QT_CMAKE_EXPORT_NAMESPACE}/QtInstallPaths.cmake")
    _qt_internal_get_android_abi_cmake_dir_path(cmake_dir ${abi})
    include("${cmake_dir}/${install_paths_path}")
    set(${out_path} "${${subdir}}" PARENT_SCOPE)
endfunction()

function(_qt_internal_collect_qt_for_android_paths out_var)
    get_target_property(qt_android_abis ${target} _qt_android_abis)
    if(NOT qt_android_abis)
        set(qt_android_abis "")
    endif()

    set(custom_qt_paths data libexecs libs plugins qml)
    foreach(type IN ITEMS prefix ${custom_qt_paths})
        set(${type}_records "")
    endforeach()

    foreach(abi IN LISTS qt_android_abis CMAKE_ANDROID_ARCH_ABI)
        _qt_internal_get_android_abi_prefix_path(qt_abi_prefix_path ${abi})
        file(TO_CMAKE_PATH "${qt_abi_prefix_path}" qt_abi_prefix_path)
        get_filename_component(qt_abi_prefix_path "${qt_abi_prefix_path}" ABSOLUTE)
        list(APPEND prefix_records "      \"${abi}\": \"${qt_abi_prefix_path}\"")
        foreach(type IN ITEMS ${custom_qt_paths})
            string(TOUPPER "${type}" upper_case_type)
            _qt_internal_get_android_abi_subdir_path(qt_abi_path
                QT6_INSTALL_${upper_case_type} ${abi})
            list(APPEND ${type}_records
                "      \"${abi}\": \"${qt_abi_path}\"")
        endforeach()
    endforeach()

    foreach(type IN ITEMS prefix ${custom_qt_paths})
        list(JOIN ${type}_records ",\n" ${type}_records_string)
        set(${type}_records_string "{\n${${type}_records_string}\n   }")
    endforeach()

    string(APPEND ${out_var}
        "   \"qt\": ${prefix_records_string},\n")
    string(APPEND ${out_var}
        "   \"qtDataDirectory\": ${data_records_string},\n")
    string(APPEND ${out_var}
        "   \"qtLibExecsDirectory\": ${libexecs_records_string},\n")
    string(APPEND ${out_var}
        "   \"qtLibsDirectory\": ${libs_records_string},\n")
    string(APPEND ${out_var}
        "   \"qtPluginsDirectory\": ${plugins_records_string},\n")
    string(APPEND ${out_var}
        "   \"qtQmlDirectory\": ${qml_records_string},\n")

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# The function collects list of existing Qt for Android using
# _qt_internal_get_android_abi_prefix_path and pre-defined set of known Android ABIs. The result is
# written to QT_DEFAULT_ANDROID_ABIS cache variable.
# Note that QT_DEFAULT_ANDROID_ABIS is not intended to be set outside the function and will be
# rewritten.
function(_qt_internal_collect_default_android_abis)
    set(known_android_abis armeabi-v7a arm64-v8a x86 x86_64)

    set(default_abis)
    foreach(abi IN LISTS known_android_abis)
        _qt_internal_get_android_abi_toolchain_path(qt_abi_toolchain_path ${abi})
        # It's expected that Qt for Android contains ABI specific toolchain file.
        if(EXISTS "${qt_abi_toolchain_path}"
            OR CMAKE_ANDROID_ARCH_ABI STREQUAL abi)
            list(APPEND default_abis ${abi})
        endif()
    endforeach()
    set(QT_DEFAULT_ANDROID_ABIS "${default_abis}" CACHE STRING
        "The list of autodetected Qt for Android ABIs" FORCE
    )
    set(QT_ANDROID_ABIS "${CMAKE_ANDROID_ARCH_ABI}" CACHE STRING
        "The list of Qt for Android ABIs used to build the project apk"
    )
    set(QT_ANDROID_BUILD_ALL_ABIS FALSE CACHE BOOL
        "Build project using the list of autodetected Qt for Android ABIs"
    )
endfunction()

# Returns a path to the timestamp file for the specific step of the multi-ABI Android project
function(_qt_internal_get_android_abi_step_stampfile out project abi step)
    get_target_property(build_dir ${project} _qt_android_build_directory)
    get_property(is_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi)
        set(${out} "${build_dir}/$<CONFIG>/${project}_${step}_stamp" PARENT_SCOPE)
    else()
        set(${out} "${build_dir}/${project}_${step}_stamp" PARENT_SCOPE)
    endif()
endfunction()

# Creates the multi-ABI Android projects and assigns the JOB_POOL to them if it's possible
function(_qt_internal_add_android_abi_project project abi)
    add_custom_target(${project})

    set(build_dir "${CMAKE_BINARY_DIR}/android_abi_builds/${abi}")
    set_target_properties(${project} PROPERTIES
        _qt_android_build_directory "${build_dir}"
    )

    file(MAKE_DIRECTORY "${build_dir}")
    if(CMAKE_GENERATOR MATCHES "^Ninja")
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS _qt_android_${project}_pool=1)
    endif()
endfunction()

# Adds the custom build step to the multi-ABI Android project
function(_qt_internal_add_android_abi_step project abi step)
    cmake_parse_arguments(arg "" "" "COMMAND;DEPENDS;TARGET_DEPENDS" ${ARGV})

    if(NOT arg_COMMAND)
        message(FATAL_ERROR "COMMAND is not set for ${project} step ${step} Android ABI ${abi}.")
    endif()

    set(dep_stamps "")
    foreach(dep ${arg_DEPENDS})
        _qt_internal_get_android_abi_step_stampfile(stamp ${project} ${abi} ${dep})
        list(APPEND dep_stamps "${stamp}")
    endforeach()

    get_target_property(build_dir ${project} _qt_android_build_directory)

    if(CMAKE_GENERATOR MATCHES "^Ninja")
        set(add_to_pool JOB_POOL _qt_android_${project}_pool)
    else()
        set(add_to_pool "")
    endif()

    if(NOT arg_TARGET_DEPENDS)
        set(arg_TARGET_DEPENDS "")
    endif()

    _qt_internal_get_android_abi_step_stampfile(stamp ${project} ${abi} ${step})
    if(step STREQUAL "configure" AND EXISTS "${stamp}")
        file(REMOVE "${stamp}")
    endif()
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${arg_COMMAND}
        COMMAND "${CMAKE_COMMAND}" -E touch "${stamp}"
        ${add_to_pool}
        DEPENDS
            ${dep_stamps}
            ${arg_TARGET_DEPENDS}
        WORKING_DIRECTORY
            "${build_dir}"
        VERBATIM
    )
    add_custom_target("${project}_${step}" DEPENDS "${stamp}")

    get_target_property(known_steps ${project} _qt_android_abi_steps)
    if(NOT CMAKE_GENERATOR MATCHES "^Ninja")
        if(NOT QT_NO_WARN_ANDROID_MULTI_ABI_GENERATOR)
            get_property(is_warned GLOBAL PROPERTY _qt_internal_warn_android_multi_abi_generator)
            if(NOT is_warned)
                set_property(GLOBAL PROPERTY _qt_internal_warn_android_multi_abi_generator TRUE)
                message(WARNING "Building Multi-ABI Qt projects with the '${CMAKE_GENERATOR}'"
                    " generator has limitations. All targets from non-main ABI will be built"
                    " unconditionally. Please use the 'Ninja' or 'Ninja Multi-config' generators"
                    " with ninja build instead. Set QT_NO_WARN_ANDROID_MULTI_ABI_GENERATOR to"
                    " 'TRUE' to suppress this warning."
                )
            endif()
        endif()
        if(known_steps)
            list(GET known_steps 0 first)
            add_dependencies(${first} ${project}_${step})
        endif()
    endif()

    if(known_steps)
        list(PREPEND known_steps ${project}_${step})
    else()
        set(known_steps ${project}_${step})
    endif()
    set_target_properties(${project} PROPERTIES _qt_android_abi_steps "${known_steps}")
endfunction()

# The function configures external projects for ABIs that target packages need to build with.
# Each target adds build step to the external project that is linked to the
# qt_internal_android_${abi}-${target}_build target in the primary ABI build tree.
function(_qt_internal_configure_android_multiabi_target target)
    # Functionality is only applicable for the primary ABI
    if(QT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT)
        return()
    endif()

    _qt_internal_android_get_target_abis(android_abis ${target})
    if(NOT android_abis)
        # User have an empty list of Qt for Android ABIs.
        message(FATAL_ERROR
            "The list of Android ABIs is empty, when building ${target}.\n"
            "You have the following options to select ABIs for a target:\n"
            " - Set the QT_ANDROID_ABIS variable before calling qt6_add_executable\n"
            " - Set the ANDROID_ABIS property for ${target}\n"
            " - Set QT_ANDROID_BUILD_ALL_ABIS flag to try building with\n"
            "   the list of autodetected Qt for Android:\n ${QT_DEFAULT_ANDROID_ABIS}"
        )
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        list(JOIN CMAKE_CONFIGURATION_TYPES "$<SEMICOLON>" escaped_configuration_types)
        set(config_arg "-DCMAKE_CONFIGURATION_TYPES=${escaped_configuration_types}")
    else()
        set(config_arg "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
    endif()

    unset(extra_cmake_args)

    # The flag is needed when building qt standalone tests only to avoid building
    # qt repo itself
    if(QT_BUILD_STANDALONE_TESTS)
        list(APPEND extra_cmake_args "-DQT_BUILD_STANDALONE_TESTS=ON")
    endif()

    if(NOT QT_ADDITIONAL_PACKAGES_PREFIX_PATH STREQUAL "")
        list(JOIN QT_ADDITIONAL_PACKAGES_PREFIX_PATH "$<SEMICOLON>" escaped_packages_prefix_path)
        list(APPEND extra_cmake_args
            "-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=${escaped_packages_prefix_path}")
    endif()

    if(NOT QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH STREQUAL "")
        list(JOIN QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH "$<SEMICOLON>"
            escaped_host_packages_prefix_path)
        list(APPEND extra_cmake_args
            "-DQT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH=${escaped_host_packages_prefix_path}")
    endif()

    if(ANDROID_SDK_ROOT)
        list(APPEND extra_cmake_args "-DANDROID_SDK_ROOT=${ANDROID_SDK_ROOT}")
    endif()

    # ANDROID_NDK_ROOT is invented by Qt and is what the qt toolchain file expects
    if(ANDROID_NDK_ROOT)
        list(APPEND extra_cmake_args "-DANDROID_NDK_ROOT=${ANDROID_NDK_ROOT}")

    # ANDROID_NDK is passed by Qt Creator and is also present in the android toolchain file.
    elseif(ANDROID_NDK)
        list(APPEND extra_cmake_args "-DANDROID_NDK_ROOT=${ANDROID_NDK}")
    endif()

    if(DEFINED QT_NO_PACKAGE_VERSION_CHECK)
        list(APPEND extra_cmake_args "-DQT_NO_PACKAGE_VERSION_CHECK=${QT_NO_PACKAGE_VERSION_CHECK}")
    endif()

    if(DEFINED QT_HOST_PATH_CMAKE_DIR)
        list(APPEND extra_cmake_args "-DQT_HOST_PATH_CMAKE_DIR=${QT_HOST_PATH_CMAKE_DIR}")
    endif()

    if(CMAKE_MAKE_PROGRAM)
        list(APPEND extra_cmake_args "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}")
    endif()

    if(CMAKE_C_COMPILER_LAUNCHER)
        list(JOIN CMAKE_C_COMPILER_LAUNCHER "$<SEMICOLON>"
            compiler_launcher)
        list(APPEND extra_cmake_args
            "-DCMAKE_C_COMPILER_LAUNCHER=${compiler_launcher}")
    endif()

    if(CMAKE_CXX_COMPILER_LAUNCHER)
        list(JOIN CMAKE_CXX_COMPILER_LAUNCHER "$<SEMICOLON>"
            compiler_launcher)
        list(APPEND extra_cmake_args
            "-DCMAKE_CXX_COMPILER_LAUNCHER=${compiler_launcher}")
    endif()

    if(DEFINED QT_USE_TARGET_ANDROID_BUILD_DIR)
        list(APPEND extra_cmake_args
            "-DQT_USE_TARGET_ANDROID_BUILD_DIR=${QT_USE_TARGET_ANDROID_BUILD_DIR}")
    endif()

    unset(user_cmake_args)
    foreach(var IN LISTS QT_ANDROID_MULTI_ABI_FORWARD_VARS)
        string(REPLACE ";" "$<SEMICOLON>" var_value "${${var}}")
        list(APPEND user_cmake_args "-D${var}=${var_value}")
    endforeach()

    set(missing_qt_abi_toolchains "")

    add_custom_target(${target}_copy_apk_dependencies)
    set(previous_copy_apk_dependencies_target ${target}_copy_apk_dependencies)
    # Create external projects for each android ABI except the main one.
    list(REMOVE_ITEM android_abis "${CMAKE_ANDROID_ARCH_ABI}")
    foreach(abi IN ITEMS ${android_abis})
        if(NOT "${abi}" IN_LIST QT_DEFAULT_ANDROID_ABIS)
            list(APPEND missing_qt_abi_toolchains ${abi})
            list(REMOVE_ITEM android_abis "${abi}")
            continue()
        endif()

        get_property(abi_external_projects GLOBAL
            PROPERTY _qt_internal_abi_external_projects)
        if(NOT abi_external_projects
            OR NOT "qt_internal_android_${abi}" IN_LIST abi_external_projects)
            _qt_internal_add_android_abi_project(qt_internal_android_${abi} ${abi})

            get_target_property(android_abi_build_dir qt_internal_android_${abi}
                _qt_android_build_directory)
            _qt_internal_get_android_abi_toolchain_path(qt_abi_toolchain_path ${abi})
            _qt_internal_add_android_abi_step(qt_internal_android_${abi} ${abi} configure
                COMMAND
                    "${CMAKE_COMMAND}"
                    "-G${CMAKE_GENERATOR}"
                    "-DCMAKE_TOOLCHAIN_FILE=${qt_abi_toolchain_path}"
                    "-DQT_HOST_PATH=${QT_HOST_PATH}"
                    "-DQT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT=ON"
                    "-DQT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR=${CMAKE_BINARY_DIR}"
                    "${config_arg}"
                    "${extra_cmake_args}"
                    "${user_cmake_args}"
                    "-B" "${android_abi_build_dir}"
                    "-S" "${CMAKE_SOURCE_DIR}"
            )
            set_property(GLOBAL APPEND PROPERTY
                _qt_internal_abi_external_projects "qt_internal_android_${abi}")
            if(NOT CMAKE_GENERATOR MATCHES "^Ninja")
                add_dependencies(qt_internal_android_${abi}_configure
                    ${previous_copy_apk_dependencies_target})
            endif()
        endif()

        get_target_property(android_abi_build_dir qt_internal_android_${abi}
            _qt_android_build_directory)
        _qt_internal_add_android_abi_step(qt_internal_android_${abi} ${abi} ${target}_build
            DEPENDS
                configure
            TARGET_DEPENDS
                ${target}
            COMMAND
                "${CMAKE_COMMAND}"
                --build "${android_abi_build_dir}"
                --config $<CONFIG>
                --target ${target}
        )

        _qt_internal_add_android_abi_step(qt_internal_android_${abi} ${abi}
            ${target}_copy_apk_dependencies
            DEPENDS
                ${target}_build
            COMMAND
                "${CMAKE_COMMAND}"
                --build "${android_abi_build_dir}"
                --config $<CONFIG>
                --target qt_internal_${target}_copy_apk_dependencies
        )
        set(external_project_copy_target
            "qt_internal_android_${abi}_${target}_copy_apk_dependencies")

        # Need to build dependency chain between the
        # qt_internal_android_${abi}-${target}_copy_apk_dependencies targets for all ABI's, to
        # prevent parallel execution of androiddeployqt processes. We cannot use Ninja job pools
        # here because it's not possible to define job pool for the step target in ExternalProject.
        # All tricks with interlayer targets don't work, because we only can bind interlayer target
        # to the job pool, but its dependencies can still be built in parallel.
        add_dependencies(${previous_copy_apk_dependencies_target}
            "${external_project_copy_target}")
        set(previous_copy_apk_dependencies_target "${external_project_copy_target}")
    endforeach()

    if(missing_qt_abi_toolchains)
        list(JOIN missing_qt_abi_toolchains ", " missing_qt_abi_toolchains_string)
        message(FATAL_ERROR "Cannot find toolchain files for the manually specified Android"
            " ABIs: ${missing_qt_abi_toolchains_string}"
            "\nNote that you also may manually specify the path to the required Qt for"
            " Android ABI using QT_PATH_ANDROID_ABI_<abi> CMake variable with the value"
            " of the installation prefix, and QT_ANDROID_PATH_CMAKE_DIR_<abi> with"
            " the location of the cmake directory for that ABI.\n")
    endif()

    list(JOIN android_abis ", " android_abis_string)
    if(android_abis_string)
        set(android_abis_string "${CMAKE_ANDROID_ARCH_ABI} (default), ${android_abis_string}")
    else()
        set(android_abis_string "${CMAKE_ANDROID_ARCH_ABI} (default)")
    endif()
    if(NOT QT_NO_ANDROID_ABI_STATUS_MESSAGE)
        message(STATUS "Configuring '${target}' for the following Android ABIs:"
                " ${android_abis_string}")
    endif()
    set_target_properties(${target} PROPERTIES _qt_android_abis "${android_abis}")
endfunction()

# The wrapper function that contains routines that need to be called to produce a valid Android
# package for the executable 'target'. The function is added to the finalizer list of the Core
# module and is executed implicitly when configuring user projects.
function(_qt_internal_android_executable_finalizer target)
    set_property(TARGET ${target} PROPERTY _qt_android_executable_finalizer_called TRUE)

    _qt_internal_expose_android_package_source_dir_to_ide(${target})

    _qt_internal_configure_android_multiabi_target("${target}")
    qt6_android_generate_deployment_settings("${target}")
    qt6_android_add_apk_target("${target}")
    _qt_internal_android_create_runner_wrapper("${target}")
endfunction()

# Helper to add the android executable finalizer.
function(_qt_internal_add_android_executable_finalizer target)
    set_property(TARGET ${target} APPEND PROPERTY
        INTERFACE_QT_EXECUTABLE_FINALIZERS
        _qt_internal_android_executable_finalizer
    )
endfunction()

# Generates an Android app runner script for target
function(_qt_internal_android_create_runner_wrapper target)
    get_target_property(is_test ${target} _qt_is_test_executable)
    get_target_property(is_manual_test ${target} _qt_is_manual_test)
    if(is_test AND NOT is_manual_test)
        qt_internal_android_test_runner_arguments("${target}" tool_path arguments)
    else()
        _qt_internal_android_app_runner_arguments("${target}" tool_path arguments)
    endif()

    set(args_splitter "")
    if(CMAKE_HOST_WIN32)
        set(args_splitter "^")
    else()
        set(args_splitter "\\")
    endif()

    list(PREPEND arguments "${tool_path}")
    set(formatted_command "")
    # format args in pairs and or single args over multiple lines with indentation
    foreach(item IN LISTS arguments)
        if(formatted_command STREQUAL "")
            set(formatted_command "${item}")
        elseif(item MATCHES "^--.*")
            set(formatted_command "${formatted_command} ${args_splitter}\n    ${item}")
        else()
            set(formatted_command "${formatted_command} \"${item}\"")
        endif()
    endforeach()

    get_target_property(wrapper_output_dir ${target} RUNTIME_OUTPUT_DIRECTORY)
    if(NOT wrapper_output_dir AND CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(wrapper_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()
    if(NOT wrapper_output_dir)
        get_target_property(wrapper_output_dir ${target} BINARY_DIR)
    endif()

    get_target_property(output_name ${target} OUTPUT_NAME)
    if(NOT output_name)
        set(output_name ${target})
    endif()

    if(CMAKE_HOST_WIN32)
        set(script_content "${formatted_command} ${args_splitter}\n    %*\n")
        set(wrapper_path "${wrapper_output_dir}/${output_name}.bat")
    else()
        set(script_content "#!/bin/sh\n\n${formatted_command} ${args_splitter}\n    $@\n")
        set(wrapper_path "${wrapper_output_dir}/${output_name}")
    endif()

    get_property(__qt_core_macros_module_base_dir GLOBAL PROPERTY __qt_core_macros_module_base_dir)
    set(template_file "${__qt_core_macros_module_base_dir}/Qt6CoreConfigureFileTemplate.in")
    set(qt_core_configure_file_contents "${script_content}")
    configure_file("${template_file}" "${wrapper_path}")

    if(CMAKE_HOST_UNIX)
        execute_process(COMMAND chmod +x ${wrapper_path})
    endif()
endfunction()

# Get the android runner script path and its arguments for a target
function(_qt_internal_android_app_runner_arguments target out_runner_path out_arguments)
    set(runner_dir "${QT_HOST_PATH}/${QT6_HOST_INFO_LIBEXECDIR}")
    set(${out_runner_path} "${runner_dir}/qt-android-runner.py" PARENT_SCOPE)

    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(${out_arguments}
        "--adb" "${ANDROID_SDK_ROOT}/platform-tools/adb"
        "--build-path" "${android_build_dir}"
        "--apk" "${android_build_dir}/${target}.apk"
        PARENT_SCOPE
    )
endfunction()

function(_qt_internal_android_get_target_android_build_dir out_build_dir target)
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    if(QT_USE_TARGET_ANDROID_BUILD_DIR)
        set(${out_build_dir} "${target_binary_dir}/android-build-${target}" PARENT_SCOPE)
    else()
        set(${out_build_dir} "${target_binary_dir}/android-build" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_expose_android_package_source_dir_to_ide target)
    get_target_property(android_package_source_dir ${target} QT_ANDROID_PACKAGE_SOURCE_DIR)
    if(android_package_source_dir)
        get_target_property(target_source_dir ${target} SOURCE_DIR)
        if(NOT IS_ABSOLUTE "${android_package_source_dir}")
            string(JOIN "/" android_package_source_dir
                "${target_source_dir}"
                "${android_package_source_dir}"
            )
        endif()

        if(EXISTS "${android_package_source_dir}")
            file(GLOB_RECURSE android_package_sources
                RELATIVE "${target_source_dir}"
                "${android_package_source_dir}/*"
            )
        endif()

        foreach(f IN LISTS android_package_sources)
            _qt_internal_expose_source_file_to_ide(${target} "${f}")
        endforeach()
    endif()
endfunction()

# Enables the terminal usage for the add_custom_command calls when verbose deployment is enabled.
function(_qt_internal_android_get_use_terminal_for_deployment out_var)
    if(QT_ENABLE_VERBOSE_DEPLOYMENT)
        set(${out_var} USES_TERMINAL PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

# Return the one of the deployment flags either release or debug depending on
# the preferred config. The returned "flags" are wrapped into generator
# expression so only usable at the generator stage.
function(_qt_internal_android_get_deployment_type_option out_var release_flag debug_flag)
    if(QT_ANDROID_DEPLOY_RELEASE)
        message(WARNING "QT_ANDROID_DEPLOY_RELEASE is not a valid Qt variable."
            " Please set QT_ANDROID_DEPLOYMENT_TYPE to RELEASE instead.")
    endif()
    # Setting QT_ANDROID_DEPLOYMENT_TYPE to a value other than Release disables
    # release package signing regardless of the build type.
    if(QT_ANDROID_DEPLOYMENT_TYPE)
        string(TOUPPER "${QT_ANDROID_DEPLOYMENT_TYPE}" deployment_type_upper)
        if("${deployment_type_upper}" STREQUAL "RELEASE")
            set(${out_var} "${release_flag}" PARENT_SCOPE)
        else()
            set(${out_var} "${debug_flag}" PARENT_SCOPE)
        endif()
    elseif(NOT QT_BUILD_TESTS)
        # Workaround for tests: do not set automatically --release flag if QT_BUILD_TESTS is set.
        # Release package need to be signed. Signing is currently not supported by CI.
        # What is more, also androidtestrunner is not working on release APKs,
        # For example running "adb shell run-as" on release APK will finish with the error:
        #    run-as: Package '[PACKAGE-NAME]' is not debuggable
        string(JOIN "" ${out_var}
            "$<IF:$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>,"
                "${release_flag},"
                "${debug_flag}"
            ">"
        )
        set(${out_var} "${${out_var}}" PARENT_SCOPE)
    else()
        set(${out_var} "${debug_flag}" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_android_get_target_abis out_abis target)
    get_target_property(target_abis ${target} QT_ANDROID_ABIS)
    if(target_abis)
        # Use target-specific Qt for Android ABIs.
        set(android_abis ${target_abis})
    elseif(QT_ANDROID_BUILD_ALL_ABIS)
        # Use autodetected Qt for Android ABIs.
        set(android_abis ${QT_DEFAULT_ANDROID_ABIS})
    elseif(QT_ANDROID_ABIS)
        # Use project-wide Qt for Android ABIs.
        set(android_abis ${QT_ANDROID_ABIS})
    else()
        set(android_abis "")
    endif()

    set(${out_abis} "${android_abis}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_find_asan_runtime_lib out_asan_lib_path)
    set(cached_asan_lib "_qt_android_asan_lib_${CMAKE_ANDROID_ARCH_ABI}")

    if(DEFINED CACHE{${cached_asan_lib}})
        if(EXISTS "${${cached_asan_lib}}")
            set(${out_asan_lib_path} "${${cached_asan_lib}}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(asan_arch_suffix "")
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(asan_arch_suffix "aarch64")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(asan_arch_suffix "arm")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(asan_arch_suffix "i686")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(asan_arch_suffix "x86_64")
    else()
        message(WARNING
            "Address Sanitizer: unsupported CMAKE_ANDROID_ARCH_ABI=${CMAKE_ANDROID_ARCH_ABI} value")
        set(${out_asan_lib_path} "" PARENT_SCOPE)
        return()
    endif()

    set(asan_lib_name "libclang_rt.asan-${asan_arch_suffix}-android.so")
    set(search_dir "${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/${ANDROID_NDK_HOST_SYSTEM_NAME}")

    if(NOT EXISTS "${search_dir}")
        message(WARNING "Address Sanitizer: the NDK toolchain path not found at ${search_dir}")
        set(${out_asan_lib_path} "" PARENT_SCOPE)
        return()
    endif()

    file(GLOB_RECURSE found_asan_lib_paths "${search_dir}/${asan_lib_name}")

    if(found_asan_lib_paths)
        list(GET found_asan_lib_paths 0 first_path)
        set(${cached_asan_lib} "${first_path}" CACHE INTERNAL
            "Cached path Android ASan runtime library for ${CMAKE_ANDROID_ARCH_ABI}")
        set(${out_asan_lib_path} "${first_path}" PARENT_SCOPE)
    else()
        message(WARNING "Address Sanitizer: could not find ${asan_lib_name} under ${search_dir}")
        set(${out_asan_lib_path} "" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_android_find_asan_wrap_sh out_wrap_sh_path)
    set(ndk_wrap_sh_path "${CMAKE_ANDROID_NDK}/wrap.sh/asan.sh")
    if(EXISTS "${ndk_wrap_sh_path}")
        set(${out_wrap_sh_path} "${ndk_wrap_sh_path}" PARENT_SCOPE)
    else()
        message(WARNING "Address Sanitizer: the wrap script not found at ${ndk_wrap_sh_path}.")
        set(${out_wrap_sh_path} "" PARENT_SCOPE)
    endif()
endfunction()

set(QT_INTERNAL_ANDROID_TARGET_BUILD_DIR_SUPPORT ON CACHE INTERNAL
    "Indicates that Qt supports per-target Android build directories")
