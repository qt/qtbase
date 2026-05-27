# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# Internal HarmonyOS deployment functions for automatic HAP target generation
#
# This file implements the finalizer pattern for HarmonyOS, similar to Android.
# When a Qt executable is created, these functions are called automatically to:
# 1. Set smart defaults for deployment properties
# 2. Generate deployment configuration
# 3. Create the ${target}_make_hap build target
#

# Detect the HarmonyOS SDK and NDK root directories from environment, CMake variables,
# or the compiler path, and set the output variables in the caller's scope.
function(_qt_internal_harmonyos_get_sdk_ndk_paths sdk_root_var ndk_root_var)
    set(sdk_root "")
    set(ndk_root "")

    if(DEFINED OHOS_SDK_ROOT)
        set(sdk_root "${OHOS_SDK_ROOT}")
    elseif(DEFINED ENV{OHOS_SDK_ROOT})
        set(sdk_root "$ENV{OHOS_SDK_ROOT}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS" AND CMAKE_CXX_COMPILER)
        # Derive from compiler path: .../sdk/default/openharmony/native/llvm/bin/clang++
        get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(_llvm_dir "${_compiler_dir}" DIRECTORY)
        get_filename_component(_native_dir "${_llvm_dir}" DIRECTORY)
        get_filename_component(sdk_root "${_native_dir}" DIRECTORY)
    endif()

    if(DEFINED OHOS_NDK_ROOT)
        set(ndk_root "${OHOS_NDK_ROOT}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS" AND CMAKE_CXX_COMPILER)
        # Derive from compiler path: .../native/llvm/bin/clang++
        get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(_llvm_dir "${_compiler_dir}" DIRECTORY)
        get_filename_component(ndk_root "${_llvm_dir}" DIRECTORY)
    endif()

    set(${sdk_root_var} "${sdk_root}" PARENT_SCOPE)
    set(${ndk_root_var} "${ndk_root}" PARENT_SCOPE)
endfunction()

# Resolve the Qt installation root and subdirectory variables.
# QT6_INSTALL_PREFIX is set by QtInstallPaths.cmake (via find_package), but not during
# an internal qtbase build; in that case fall back to CMAKE_INSTALL_PREFIX.
function(_qt_internal_harmonyos_get_qt_install_dirs
         install_root_var libs_dir_var plugins_dir_var qml_dir_var data_dir_var)
    if(QT6_INSTALL_PREFIX)
        set(${install_root_var} "${QT6_INSTALL_PREFIX}" PARENT_SCOPE)
        set(${libs_dir_var}    "${QT6_INSTALL_LIBS}"    PARENT_SCOPE)
        set(${plugins_dir_var} "${QT6_INSTALL_PLUGINS}" PARENT_SCOPE)
        set(${qml_dir_var}     "${QT6_INSTALL_QML}"     PARENT_SCOPE)
        set(${data_dir_var}    "${QT6_INSTALL_DATA}"    PARENT_SCOPE)
    else()
        set(${install_root_var} "${CMAKE_INSTALL_PREFIX}" PARENT_SCOPE)
        set(${libs_dir_var}    "${INSTALL_LIBDIR}"        PARENT_SCOPE)
        set(${plugins_dir_var} "${INSTALL_PLUGINSDIR}"    PARENT_SCOPE)
        set(${qml_dir_var}     "${INSTALL_QMLDIR}"        PARENT_SCOPE)
        set(${data_dir_var}    "${INSTALL_DATADIR}"       PARENT_SCOPE)
    endif()
endfunction()

# Helper function to extract project libraries from QtDeployTargets.cmake
function(_qt_internal_harmonyos_extract_project_libraries target output_var)
    # QtDeployTargets.cmake is generated in ${CMAKE_BINARY_DIR}/.qt/
    set(deploy_targets_file "${CMAKE_BINARY_DIR}/.qt/QtDeployTargets.cmake")

    # Check if file exists
    if(NOT EXISTS "${deploy_targets_file}")
        if(QT_INTERNAL_VERBOSE)
            message(STATUS "QtDeployTargets.cmake not found at: ${deploy_targets_file}")
        endif()
        set(${output_var} "" PARENT_SCOPE)
        return()
    endif()

    # include() evaluates the file as CMake code, which correctly handles path escaping,
    # CMake list-separator semicolons, and other special characters.  Regex parsing was
    # rejected because it would need to re-implement CMake's own string escaping rules.
    set(__QT_DEPLOY_TARGETS "")
    include("${deploy_targets_file}")

    # TODO The following is a check to help catching projects that need to be
    # re-configured after the introduction of __QT_DEPLOY_TARGETS. This can be
    # removed after we can assume that no "old projects" exist anymore.
    if(__QT_DEPLOY_TARGETS STREQUAL "")
        message(FATAL_ERROR
            "The deployment information is missing the __QT_DEPLOY_TARGETS variable. "
            "This can be fixed by re-configuring the project."
        )
    endif()

    # Collect all __QT_DEPLOY_TARGET_*_FILE variables
    set(library_paths "")
    foreach(target_name IN LISTS __QT_DEPLOY_TARGETS)
        # Skip the main application target itself
        if(target_name STREQUAL "${target}")
            continue()
        endif()

        set(lib_path "${__QT_DEPLOY_TARGET_${target_name}_FILE}")

        # Only include .so files (shared/module libraries)
        if(lib_path MATCHES "\\.so$")
            list(APPEND library_paths "${lib_path}")
        endif()
    endforeach()

    # Remove duplicates
    if(library_paths)
        list(REMOVE_DUPLICATES library_paths)
    endif()

    set(${output_var} "${library_paths}" PARENT_SCOPE)
endfunction()

# Collect directly-linked SHARED_LIBRARY CMake targets into a list of their
# output file paths (using generator expressions so paths are resolved at
# generation time).  Also respects an explicit QT_HARMONYOS_EXTRA_LIBS property.
# Analogous to Qt Android's QT_ANDROID_EXTRA_LIBS mechanism.
function(_qt_internal_harmonyos_collect_extra_libs target output_var)
    set(result "")

    get_target_property(link_libs "${target}" LINK_LIBRARIES)
    if(link_libs AND NOT link_libs STREQUAL "link_libs-NOTFOUND")
        foreach(lib IN LISTS link_libs)
            # Skip generator expressions and linker flags — they can't be
            # tested with if(TARGET ...) and don't refer to CMake targets.
            if(lib MATCHES "^\\$<" OR lib MATCHES "^-")
                continue()
            endif()
            if(NOT TARGET "${lib}")
                continue()
            endif()
            get_target_property(lib_type "${lib}" TYPE)
            if(lib_type STREQUAL "SHARED_LIBRARY")
                list(APPEND result "$<TARGET_FILE:${lib}>")
            endif()
        endforeach()
    endif()

    # Honour explicit QT_HARMONYOS_EXTRA_LIBS target property (file paths or genexes)
    get_target_property(extra_libs "${target}" QT_HARMONYOS_EXTRA_LIBS)
    if(extra_libs AND NOT extra_libs STREQUAL "extra_libs-NOTFOUND")
        list(APPEND result ${extra_libs})
    endif()

    # TODO Remove QT_OHOS_EXTRA_LIBS after a grace period but latest after 6.12.0.
    get_target_property(extra_libs "${target}" QT_OHOS_EXTRA_LIBS)
    if(extra_libs AND NOT extra_libs STREQUAL "extra_libs-NOTFOUND")
        list(APPEND result ${extra_libs})
    endif()

    list(REMOVE_DUPLICATES result)
    set(${output_var} "${result}" PARENT_SCOPE)
endfunction()

# Sets default values for HarmonyOS deployment properties if not already set
function(_qt_internal_set_harmonyos_deployment_defaults target)
    # Set default bundle name: org.qtproject.example.${target_sanitized}
    get_target_property(bundle_name ${target} QT_HARMONYOS_APP_BUNDLE_NAME)
    if(NOT bundle_name OR bundle_name STREQUAL "bundle_name-NOTFOUND")
        # Sanitize target name to be a valid identifier
        string(MAKE_C_IDENTIFIER "${target}" target_sanitized)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_APP_BUNDLE_NAME "org.qtproject.example.${target_sanitized}"
        )
    endif()

    # Set default app name: ${target}
    get_target_property(app_name ${target} QT_HARMONYOS_APP_NAME)
    if(NOT app_name OR app_name STREQUAL "app_name-NOTFOUND")
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_APP_NAME "${target}"
        )
    endif()

    # Set default target architectures: arm64-v8a
    get_target_property(target_archs ${target} QT_HARMONYOS_TARGET_ARCHS)
    if(NOT target_archs OR target_archs STREQUAL "target_archs-NOTFOUND")
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_TARGET_ARCHS "arm64-v8a"
        )
    endif()
endfunction()

# Generates the HarmonyOS deployment settings JSON file
function(_qt_internal_harmonyos_generate_deployment_settings target)
    # Query bundle name from target property (now guaranteed to exist)
    get_target_property(bundle_name ${target} QT_HARMONYOS_APP_BUNDLE_NAME)

    # Query app name from target property
    get_target_property(app_name ${target} QT_HARMONYOS_APP_NAME)
    if(NOT app_name OR app_name STREQUAL "app_name-NOTFOUND")
        set(app_name "${target}")
    endif()

    # Query target architectures from property
    get_target_property(target_archs ${target} QT_HARMONYOS_TARGET_ARCHS)
    if(NOT target_archs OR target_archs STREQUAL "target_archs-NOTFOUND")
        set(target_archs "arm64-v8a")
    endif()

    # Check for multi-arch support (not yet implemented)
    if(target_archs MATCHES ";")
        list(LENGTH target_archs arch_count)
        if(arch_count GREATER 1)
            message(WARNING "Multiple architectures specified for ${target}: ${target_archs}. "
                "Multi-architecture HAP generation is not yet supported. "
                "Only the first architecture will be used.")
        endif()
    endif()

    # Detect SDK and NDK paths
    _qt_internal_harmonyos_get_sdk_ndk_paths(sdk_root ndk_root)

    # Query QML-related properties from target
    set(qml_root_path "")
    set(qml_import_paths "")

    # Check if this target is a QML module
    get_target_property(qml_module_uri ${target} QT_QML_MODULE_URI)
    if(qml_module_uri AND NOT qml_module_uri STREQUAL "qml_module_uri-NOTFOUND")
        # This target is a QML module, query its properties
        get_target_property(qml_module_target_path ${target} QT_QML_MODULE_TARGET_PATH)
        if(qml_module_target_path AND
           NOT qml_module_target_path STREQUAL "qml_module_target_path-NOTFOUND")
            # Use the QML module's target path
            set(qml_root_path "${qml_module_target_path}")
        endif()
    endif()

    # Get binary path (will be a generator expression for multi-config)
    set(BINARY_PATH "$<TARGET_FILE:${target}>")

    # Convert target architectures to JSON array
    if(target_archs MATCHES ";")
        # List format
        set(TARGET_ARCHS_JSON "")
        foreach(arch ${target_archs})
            if(TARGET_ARCHS_JSON)
                string(APPEND TARGET_ARCHS_JSON ", ")
            endif()
            string(APPEND TARGET_ARCHS_JSON "\"${arch}\"")
        endforeach()
    else()
        # Single value or semicolon-separated string
        string(REPLACE ";" "\", \"" TARGET_ARCHS_JSON "${target_archs}")
        set(TARGET_ARCHS_JSON "\"${TARGET_ARCHS_JSON}\"")
    endif()

    # Generate JSON config file
    set(CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/${target}-harmony-deployment-settings.json")

    set(JSON_CONTENT "{\n")
    string(APPEND JSON_CONTENT "    \"application-binary\": \"${BINARY_PATH}\",\n")
    string(APPEND JSON_CONTENT "    \"harmonyos-app-name\": \"${app_name}\",\n")
    string(APPEND JSON_CONTENT "    \"harmonyos-app-bundle-name\": \"${bundle_name}\",\n")
    string(APPEND JSON_CONTENT "    \"harmonyos-target-arch\": [${TARGET_ARCHS_JSON}]")

    # App-level metadata set via qt_set_harmonyos_app_metadata; emit only what the
    # user explicitly set so harmonydeployqt knows to leave the others as the
    # template default.
    foreach(prop_kv IN ITEMS
            "QT_HARMONYOS_APP_VENDOR;harmonyos-app-vendor;string"
            "QT_HARMONYOS_APP_VERSION_CODE;harmonyos-app-version-code;integer"
            "QT_HARMONYOS_APP_VERSION_NAME;harmonyos-app-version-name;string"
            "QT_HARMONYOS_APP_LABEL;harmonyos-app-label;string"
            "QT_HARMONYOS_APP_ICON;harmonyos-app-icon;path"
            "QT_HARMONYOS_MODULE_DESCRIPTION;harmonyos-module-description;string")
        list(GET prop_kv 0 prop_name)
        list(GET prop_kv 1 json_key)
        list(GET prop_kv 2 json_type)
        get_target_property(prop_value ${target} ${prop_name})
        if(prop_value)
            if(json_type STREQUAL "integer")
                string(APPEND JSON_CONTENT ",\n    \"${json_key}\": ${prop_value}")
            else()
                if(json_type STREQUAL "path")
                    file(TO_CMAKE_PATH "${prop_value}" prop_value)
                endif()
                _qt_internal_json_escape_content("${prop_value}" prop_value)
                string(APPEND JSON_CONTENT ",\n    \"${json_key}\": \"${prop_value}\"")
            endif()
        endif()
    endforeach()

    # Module-level deviceTypes is a list; emit as a JSON array of quoted strings.
    get_target_property(module_device_types ${target} QT_HARMONYOS_MODULE_DEVICE_TYPES)
    if(module_device_types AND NOT module_device_types STREQUAL "module_device_types-NOTFOUND")
        set(device_types_json "")
        foreach(dt IN LISTS module_device_types)
            if(device_types_json)
                string(APPEND device_types_json ", ")
            endif()
            string(APPEND device_types_json "\"${dt}\"")
        endforeach()
        string(APPEND JSON_CONTENT
            ",\n    \"harmonyos-module-device-types\": [${device_types_json}]")
    endif()

    if(sdk_root)
        file(TO_CMAKE_PATH "${sdk_root}" sdk_root)
        string(APPEND JSON_CONTENT ",\n    \"sdk-root\": \"${sdk_root}\"")
    endif()

    if(ndk_root)
        file(TO_CMAKE_PATH "${ndk_root}" ndk_root)
        string(APPEND JSON_CONTENT ",\n    \"ndk-root\": \"${ndk_root}\"")
    endif()

    # Add Qt installation directories (following androiddeployqt pattern)
    # For cross-compilation: target dirs from current build, host tools from QT_HOST_PATH
    _qt_internal_harmonyos_get_qt_install_dirs(
        _qt_install_root _qt_install_libs_dir _qt_install_plugins_dir _qt_install_qml_dir
        _qt_install_data_dir)
    string(APPEND JSON_CONTENT
        ",\n    \"qtLibsDirectory\": \"${_qt_install_root}/${_qt_install_libs_dir}\"")
    string(APPEND JSON_CONTENT
        ",\n    \"qtPluginsDirectory\": \"${_qt_install_root}/${_qt_install_plugins_dir}\"")
    string(APPEND JSON_CONTENT
        ",\n    \"qtQmlDirectory\": \"${_qt_install_root}/${_qt_install_qml_dir}\"")

    # Host Qt tools directory (macOS build for qmlimportscanner, etc.)
    if(QT_HOST_PATH)
        file(TO_CMAKE_PATH "${QT_HOST_PATH}" QT_HOST_PATH)
        string(APPEND JSON_CONTENT
            ",\n    \"qtLibExecsDirectory\": \"${QT_HOST_PATH}/${QT6_HOST_INFO_LIBEXECDIR}\"")
        string(APPEND JSON_CONTENT ",\n    \"qtHostDirectory\": \"${QT_HOST_PATH}\"")
    endif()

    # Templates are only built for OHOS, so they live under the target Qt
    # prefix, not QT_HOST_PATH.
    set(TEMPLATE_DIR
        "${_qt_install_root}/${_qt_install_data_dir}/src/harmonyos/templates")
    if(EXISTS "${TEMPLATE_DIR}")
        file(REAL_PATH "${TEMPLATE_DIR}" TEMPLATE_DIR)
        string(APPEND JSON_CONTENT
            ",\n    \"harmonyos-package-source-directory\": \"${TEMPLATE_DIR}\"")
    endif()

    # Extra library search directories for third-party deps (e.g. HARMONYOS_DEPS_ROOT/lib for ICU).
    # CMAKE_FIND_ROOT_PATH has the Qt install prefix prepended by qt.toolchain.cmake; skip it so
    # we only emit paths that point to user-supplied dependency prefixes.
    set(_extra_root_paths "${CMAKE_FIND_ROOT_PATH}")
    list(REMOVE_DUPLICATES _extra_root_paths)
    if(QT6_INSTALL_PREFIX)
        list(REMOVE_ITEM _extra_root_paths "${QT6_INSTALL_PREFIX}")
    endif()
    set(_extra_libs_dirs_json "")
    foreach(_root_path IN LISTS _extra_root_paths)
        set(_lib_dir "${_root_path}/lib")
        if(EXISTS "${_lib_dir}")
            file(TO_CMAKE_PATH "${_lib_dir}" _lib_dir)
            if(_extra_libs_dirs_json)
                string(APPEND _extra_libs_dirs_json ", ")
            endif()
            string(APPEND _extra_libs_dirs_json "\"${_lib_dir}\"")
        endif()
    endforeach()
    if(_extra_libs_dirs_json)
        string(APPEND JSON_CONTENT ",\n    \"extra-libs-dirs\": [${_extra_libs_dirs_json}]")
    endif()

    if(qml_root_path)
        file(TO_CMAKE_PATH "${qml_root_path}" qml_root_path)
        string(APPEND JSON_CONTENT ",\n    \"qml-root-path\": \"${qml_root_path}\"")
    endif()

    if(qml_import_paths)
        # Convert list to JSON array
        set(QML_IMPORT_PATHS_JSON "")
        foreach(path ${qml_import_paths})
            file(TO_CMAKE_PATH "${path}" path)
            if(QML_IMPORT_PATHS_JSON)
                string(APPEND QML_IMPORT_PATHS_JSON ", ")
            endif()
            string(APPEND QML_IMPORT_PATHS_JSON "\"${path}\"")
        endforeach()
        string(APPEND JSON_CONTENT ",\n    \"qml-import-paths\": [${QML_IMPORT_PATHS_JSON}]")
    endif()

    # Collect permissions: the executable's own first (so its entries win
    # for duplicate names), then transitive permissions contributed by Qt
    # modules (and any other linked targets) via QT_HARMONYOS_PERMISSIONS.
    set(harmonyos_permissions "")
    get_target_property(user_permissions ${target} QT_HARMONYOS_PERMISSIONS)
    if(user_permissions AND NOT user_permissions STREQUAL "user_permissions-NOTFOUND")
        list(APPEND harmonyos_permissions ${user_permissions})
    endif()

    __qt_internal_collect_all_target_dependencies(${target} dep_targets)
    foreach(dep IN LISTS dep_targets)
        get_target_property(dep_perms ${dep} QT_HARMONYOS_PERMISSIONS)
        if(dep_perms AND NOT dep_perms STREQUAL "dep_perms-NOTFOUND")
            list(APPEND harmonyos_permissions ${dep_perms})
        endif()
    endforeach()

    if(harmonyos_permissions)
        # De-duplicate by the JSON "name" field, keeping the first occurrence.
        set(seen_names "")
        set(unique_permissions "")
        foreach(entry IN LISTS harmonyos_permissions)
            string(JSON name GET "${entry}" name)
            if(NOT name IN_LIST seen_names)
                list(APPEND seen_names "${name}")
                list(APPEND unique_permissions "${entry}")
            endif()
        endforeach()
        list(JOIN unique_permissions ",\n        " harmonyos_permissions_joined)
        string(APPEND JSON_CONTENT
            ",\n    \"permissions\": [\n        ${harmonyos_permissions_joined}\n    ]")
    endif()

    # Extract project libraries from QtDeployTargets.cmake
    _qt_internal_harmonyos_extract_project_libraries(${target} PROJECT_LIBS)

    # Also collect directly-linked SHARED_LIBRARY CMake targets (e.g. test helper libs
    # like qmetatype_lib1 that are NOT Qt libraries and would otherwise be omitted from
    # harmonydeployqt's ELF dependency scan which only tracks libQt6* libs).
    _qt_internal_harmonyos_collect_extra_libs(${target} EXTRA_LIBS)
    if(EXTRA_LIBS)
        list(APPEND PROJECT_LIBS ${EXTRA_LIBS})
        list(REMOVE_DUPLICATES PROJECT_LIBS)
    endif()

    if(PROJECT_LIBS)
        # Convert list to JSON array
        set(PROJECT_LIBS_JSON "")
        foreach(lib_path ${PROJECT_LIBS})
            if(PROJECT_LIBS_JSON)
                string(APPEND PROJECT_LIBS_JSON ", ")
            endif()
            # Use the path directly - generator expressions resolved by file(GENERATE)
            string(APPEND PROJECT_LIBS_JSON "\"${lib_path}\"")
        endforeach()
        string(APPEND JSON_CONTENT ",\n    \"project-libraries\": [${PROJECT_LIBS_JSON}]")
    endif()

    string(APPEND JSON_CONTENT "\n}\n")

    # Write config file at generation time (will contain generator expressions)
    file(GENERATE OUTPUT "${CONFIG_FILE}" CONTENT "${JSON_CONTENT}")

    # Store config file path on target for use in HAP target creation
    set_target_properties(${target} PROPERTIES
        QT_HARMONYOS_DEPLOYMENT_SETTINGS_FILE "${CONFIG_FILE}"
    )
endfunction()

# Creates the ${target}_make_hap custom target
function(_qt_internal_harmonyos_add_hap_target target)
    # Check if target already created (avoid duplicates)
    get_target_property(hap_target_created ${target} _qt_harmonyos_hap_target_created)
    if(hap_target_created)
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_harmonyos_hap_target_created TRUE)

    # Get config file path
    get_target_property(CONFIG_FILE ${target} QT_HARMONYOS_DEPLOYMENT_SETTINGS_FILE)
    if(NOT CONFIG_FILE)
        message(WARNING "Cannot create HAP target for ${target}: deployment settings not generated")
        return()
    endif()

    # Set default output directory
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/harmonyos-build")

    # Check if the generator supports DEPFILE
    _qt_internal_check_depfile_support(has_depfile_support)

    # Define HAP output file and depfile paths
    set(HAP_OUTPUT_FILE "${OUTPUT_DIR}/entry/build/default/outputs/default/${target}.hap")
    set(DEP_FILE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${target}_hap.d")

    # Find harmonydeployqt tool
    if(QT_HOST_PATH)
        set(HARMONYDEPLOYQT_EXECUTABLE "${QT_HOST_PATH}/${QT6_HOST_INFO_BINDIR}/harmonydeployqt")
    else()
        set(HARMONYDEPLOYQT_EXECUTABLE
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${QT6_HOST_INFO_BINDIR}/harmonydeployqt")
    endif()

    # Check for hvigor path: target property takes precedence over global variable
    get_target_property(_hvigor_path ${target} QT_HARMONYOS_HVIGOR)
    if(NOT _hvigor_path OR _hvigor_path STREQUAL "_hvigor_path-NOTFOUND")
        if(DEFINED QT_HARMONYOS_HVIGOR)
            set(_hvigor_path "${QT_HARMONYOS_HVIGOR}")
        else()
            set(_hvigor_path "")
        endif()
    endif()

    if(_hvigor_path)
        set(_hvigor_args --hvigor "${_hvigor_path}")
    else()
        set(_hvigor_args "")
    endif()

    # Create deployment target with DEPFILE support when available
    if(has_depfile_support)
        # Use add_custom_command with DEPFILE for dependency tracking
        cmake_policy(PUSH)
        if(POLICY CMP0116)
            cmake_policy(SET CMP0116 NEW)
            set(relative_to_dir ${CMAKE_CURRENT_BINARY_DIR})
        else()
            set(relative_to_dir ${CMAKE_BINARY_DIR})
        endif()

        add_custom_command(OUTPUT "${HAP_OUTPUT_FILE}"
            COMMAND ${HARMONYDEPLOYQT_EXECUTABLE}
                --input "${CONFIG_FILE}"
                --output "${OUTPUT_DIR}"
                ${_hvigor_args}
                --depfile "${DEP_FILE_PATH}"
                --depfile-base "${relative_to_dir}"
            DEPENDS ${target} "${CONFIG_FILE}"
            DEPFILE "${DEP_FILE_PATH}"
            COMMENT "Generating HarmonyOS HAP for ${target}"
            VERBATIM
        )
        cmake_policy(POP)

        add_custom_target(${target}_make_hap DEPENDS "${HAP_OUTPUT_FILE}")
    else()
        # Fallback for generators/CMake versions without DEPFILE support
        # (see _qt_internal_check_depfile_support for the exact conditions)
        add_custom_target(${target}_make_hap
            COMMAND ${HARMONYDEPLOYQT_EXECUTABLE}
                --input "${CONFIG_FILE}"
                --output "${OUTPUT_DIR}"
                ${_hvigor_args}
            DEPENDS ${target}
            COMMENT "Generating HarmonyOS HAP for ${target}"
            VERBATIM
        )
    endif()
endfunction()

# Main finalizer function called automatically by Qt's finalization system
# This is the entry point that gets registered with INTERFACE_QT_EXECUTABLE_FINALIZERS
function(_qt_internal_harmonyos_executable_finalizer target)
    # Prevent duplicate calls
    get_target_property(already_called ${target} _qt_harmonyos_executable_finalizer_called)
    if(already_called)
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_harmonyos_executable_finalizer_called TRUE)

    # Check target type - process executables and module libraries
    # (OHOS uses MODULE_LIBRARY for apps)
    get_target_property(target_type ${target} TYPE)
    if(NOT target_type STREQUAL "EXECUTABLE" AND NOT target_type STREQUAL "MODULE_LIBRARY")
        return()
    endif()

    # Set defaults, generate settings, create HAP target
    _qt_internal_set_harmonyos_deployment_defaults(${target})
    _qt_internal_harmonyos_generate_deployment_settings(${target})
    _qt_internal_harmonyos_add_hap_target(${target})
endfunction()

# Helper to register the finalizer with a target
# This adds the finalizer function to INTERFACE_QT_EXECUTABLE_FINALIZERS property
function(_qt_internal_add_harmonyos_executable_finalizer target)
    set_property(TARGET ${target} APPEND PROPERTY
        INTERFACE_QT_EXECUTABLE_FINALIZERS
        _qt_internal_harmonyos_executable_finalizer
    )
endfunction()

function(qt6_add_harmonyos_permission target)
    _qt_internal_add_harmonyos_permission(${ARGV})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_harmonyos_permission target)
        qt6_add_harmonyos_permission(${ARGV})
    endfunction()
endif()

# Set HarmonyOS app-level metadata that lands in AppScope/app.json5.
# These values are conceptually app-scoped (one app = one bundle); on a target
# that builds the app's main module they get baked into the deployment-settings
# JSON and consumed by harmonydeployqt at HAP build time.
#
# Synopsis
#   qt_set_harmonyos_app_metadata(target
#       [VENDOR <string>]
#       [VERSION_CODE <integer>]
#       [VERSION_NAME <string>]
#       [LABEL <string-or-$string:ref>]
#       [ICON <path-or-$media:ref>]
#   )
#
# ICON accepts either a host filesystem path (deploy tool copies the image into
# AppScope/resources/base/media/ and emits a $media: reference) or a $media:
# reference (passed through verbatim). Relative paths resolve against the
# call site's CMAKE_CURRENT_SOURCE_DIR.
function(_qt_internal_set_harmonyos_app_metadata target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Empty or invalid target for setting HarmonyOS app metadata: (${target})")
    endif()

    set(no_value_options "")
    set(single_value_options
        VENDOR
        VERSION_CODE
        VERSION_NAME
        LABEL
        ICON
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(DEFINED arg_VENDOR)
        set_target_properties(${target} PROPERTIES QT_HARMONYOS_APP_VENDOR "${arg_VENDOR}")
    endif()
    if(DEFINED arg_VERSION_CODE)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_APP_VERSION_CODE "${arg_VERSION_CODE}")
    endif()
    if(DEFINED arg_VERSION_NAME)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_APP_VERSION_NAME "${arg_VERSION_NAME}")
    endif()
    if(DEFINED arg_LABEL)
        set_target_properties(${target} PROPERTIES QT_HARMONYOS_APP_LABEL "${arg_LABEL}")
    endif()
    if(DEFINED arg_ICON)
        set(icon_value "${arg_ICON}")
        if(NOT icon_value MATCHES "^\\$media:")
            if(NOT IS_ABSOLUTE "${icon_value}")
                set(icon_value "${CMAKE_CURRENT_SOURCE_DIR}/${icon_value}")
            endif()
        endif()
        set_target_properties(${target} PROPERTIES QT_HARMONYOS_APP_ICON "${icon_value}")
    endif()
endfunction()

function(qt6_set_harmonyos_app_metadata target)
    _qt_internal_set_harmonyos_app_metadata(${ARGV})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_set_harmonyos_app_metadata target)
        qt6_set_harmonyos_app_metadata(${ARGV})
    endfunction()
endif()

# Set HarmonyOS module-level metadata that lands in entry/src/main/module.json5.
#
# Synopsis
#   qt_set_harmonyos_module_metadata(target
#       [DESCRIPTION <string-or-$string:ref>]
#       [DEVICE_TYPES <type> [<type>...]]
#   )
#
# DESCRIPTION accepts a literal string (appears verbatim in the manifest and
# is not localizable) or a $string:<ref> resource reference (localizable via
# the user's string.json files).
#
# DEVICE_TYPES is a list of HarmonyOS device-type identifiers (e.g. tablet,
# 2in1, phone). When unset, harmonydeployqt picks the Qt default. Note that
# 'phone' is intentionally excluded from the default because of QTFOROH-1076
# (main window does not restore itself to the desired state after being
# minimized on phone); revisit once that is fixed.
function(_qt_internal_set_harmonyos_module_metadata target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Empty or invalid target for setting HarmonyOS module metadata: (${target})")
    endif()

    set(no_value_options "")
    set(single_value_options DESCRIPTION)
    set(multi_value_options DEVICE_TYPES)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(DEFINED arg_DESCRIPTION)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_MODULE_DESCRIPTION "${arg_DESCRIPTION}")
    endif()
    if(arg_DEVICE_TYPES)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_MODULE_DEVICE_TYPES "${arg_DEVICE_TYPES}")
    endif()
endfunction()

function(qt6_set_harmonyos_module_metadata target)
    _qt_internal_set_harmonyos_module_metadata(${ARGV})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_set_harmonyos_module_metadata target)
        qt6_set_harmonyos_module_metadata(${ARGV})
    endfunction()
endif()
