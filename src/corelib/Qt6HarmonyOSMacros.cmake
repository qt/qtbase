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

# Promote QT_HARMONYOS_HDC / QT_HARMONYOS_HVIGOR from the environment to the
# matching cache var when the latter is not already set on the command line.
if(NOT DEFINED HARMONYOS_HDC
        AND DEFINED ENV{QT_HARMONYOS_HDC}
        AND NOT "$ENV{QT_HARMONYOS_HDC}" STREQUAL "")
    set(HARMONYOS_HDC "$ENV{QT_HARMONYOS_HDC}")
endif()
if(NOT DEFINED HARMONYOS_HVIGOR
        AND DEFINED ENV{QT_HARMONYOS_HVIGOR}
        AND NOT "$ENV{QT_HARMONYOS_HVIGOR}" STREQUAL "")
    set(HARMONYOS_HVIGOR "$ENV{QT_HARMONYOS_HVIGOR}")
endif()

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
# an internal qtbase build; in that case fall back to QtBase_BINARY_DIR.
function(_qt_internal_harmonyos_get_qt_install_dirs
         install_root_var libs_dir_var plugins_dir_var qml_dir_var data_dir_var)
    if(QT6_INSTALL_PREFIX)
        set(${install_root_var} "${QT6_INSTALL_PREFIX}" PARENT_SCOPE)
        set(${libs_dir_var}    "${QT6_INSTALL_LIBS}"    PARENT_SCOPE)
        set(${plugins_dir_var} "${QT6_INSTALL_PLUGINS}" PARENT_SCOPE)
        set(${qml_dir_var}     "${QT6_INSTALL_QML}"     PARENT_SCOPE)
        set(${data_dir_var}    "${QT6_INSTALL_DATA}"    PARENT_SCOPE)
    else()
        set(${install_root_var} "${QtBase_BINARY_DIR}"    PARENT_SCOPE)
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

# Append a JSON `"key": "value"` entry (with leading comma + newline) to
# `${out_var}` for the target's property, evaluated at generate-time via
# GENEX_EVAL so transitively-set values reach the JSON. Emits nothing if the
# property is unset or empty.
function(_qt_internal_harmonyos_add_deployment_property out_var json_key target property)
    set(property_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${property}>>")
    string(APPEND ${out_var}
        "$<$<BOOL:${property_genex}>:"
            ",\n    \"${json_key}\": \"${property_genex}\""
        ">"
    )
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Integer variant: emits the value without surrounding quotes. An integer 0
# is treated as unset (matches the historical semantics of the metadata loop).
function(_qt_internal_harmonyos_add_deployment_int_property out_var json_key target property)
    set(property_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${property}>>")
    string(APPEND ${out_var}
        "$<$<BOOL:${property_genex}>:"
            ",\n    \"${json_key}\": ${property_genex}"
        ">"
    )
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Append a JSON `"key": [ "v1", "v2" ]` entry. The semicolon-delimited list
# value is joined with $<COMMA> at generate-time. Emits nothing if the
# property is unset or empty.
function(_qt_internal_harmonyos_add_deployment_list_property out_var json_key target property)
    set(property_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${property}>>")
    string(APPEND ${out_var}
        "$<$<BOOL:${property_genex}>:"
            ",\n    \"${json_key}\": [\"$<JOIN:${property_genex},\"$<COMMA>\">\"]"
        ">"
    )
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Stash CMake-format copies of path-bearing target properties on internal
# `_qt_harmonyos_*` properties so the emission helpers can read them
# via genex without further escaping. Pre-release contract: users supply
# CMake-format paths (forward slashes), mirroring the Qt 6.6+ NEW behavior of
# the Android equivalent (`_qt_internal_android_format_deployment_paths`).
#
# qml-root-path concatenates two sources with a `;` glue when both are
# non-empty:
#   * QT_QML_ROOT_PATH             -- user-set roots
#   * _qt_internal_qml_root_path   -- roots auto-collected by
#       _qt_internal_collect_qml_root_paths when .qml files are added as
#       resources (populated for OHOS via the corresponding hook in
#       Qt6CoreMacros.cmake).
function(_qt_internal_harmonyos_format_deployment_paths target)
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
        _qt_harmonyos_qml_import_paths
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_QML_IMPORT_PATH>>"
        _qt_harmonyos_qml_root_paths
            "${qml_root_path_genex}"
        _qt_harmonyos_package_source_dir
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},QT_HARMONYOS_PACKAGE_SOURCE_DIR>>"
    )

    # QT_HARMONYOS_EXTRA_PLUGINS items are either CMake plugin target names or
    # absolute file paths; target names are resolved to $<TARGET_FILE:> here so
    # the emission helper sees a flat list of paths/genexes.
    get_target_property(_extra_plugins ${target} QT_HARMONYOS_EXTRA_PLUGINS)
    set(_resolved_extra_plugins "")
    if(_extra_plugins AND NOT _extra_plugins STREQUAL "_extra_plugins-NOTFOUND")
        foreach(item IN LISTS _extra_plugins)
            if(TARGET ${item})
                list(APPEND _resolved_extra_plugins "$<TARGET_FILE:${item}>")
            else()
                list(APPEND _resolved_extra_plugins "${item}")
            endif()
        endforeach()
    endif()
    set_target_properties(${target} PROPERTIES
        _qt_harmonyos_extra_plugins "${_resolved_extra_plugins}"
    )
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

    # Set default target architecture to the architecture used with the toolchain file.
    get_target_property(target_archs ${target} QT_HARMONYOS_TARGET_ARCHS)
    if(NOT target_archs OR target_archs STREQUAL "target_archs-NOTFOUND")
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_TARGET_ARCHS "${OHOS_ARCH}"
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
        set(target_archs "${OHOS_ARCH}")
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

    # Stash JSON-escaped copies of string-typed user-supplied properties on
    # internal `_qt_harmonyos_*` properties. The emission helpers below
    # substitute property values verbatim into the JSON; an unescaped `"` or
    # `\` (e.g. in an app LABEL containing quoted text, or a Windows-style
    # ICON path with backslashes) would otherwise corrupt the JSON.
    foreach(prop_kv IN ITEMS
            "QT_HARMONYOS_APP_VENDOR;_qt_harmonyos_app_vendor;string"
            "QT_HARMONYOS_APP_VERSION_NAME;_qt_harmonyos_app_version_name;string"
            "QT_HARMONYOS_APP_LABEL;_qt_harmonyos_app_label;string"
            "QT_HARMONYOS_APP_ICON;_qt_harmonyos_app_icon;path"
            "QT_HARMONYOS_MODULE_DESCRIPTION;_qt_harmonyos_module_description;string"
            "QT_HARMONYOS_ABILITY_ORIENTATION;_qt_harmonyos_ability_orientation;string"
            "QT_HARMONYOS_COMPATIBLE_SDK_VERSION;_qt_harmonyos_compatible_sdk_version;string"
            "QT_HARMONYOS_TARGET_SDK_VERSION;_qt_harmonyos_target_sdk_version;string"
            "QT_HARMONYOS_COMPILE_SDK_VERSION;_qt_harmonyos_compile_sdk_version;string"
        )
        list(GET prop_kv 0 _prop)
        list(GET prop_kv 1 _mirror)
        list(GET prop_kv 2 _type)
        get_target_property(_value ${target} ${_prop})
        if(_value)
            if(_type STREQUAL "path")
                file(TO_CMAKE_PATH "${_value}" _value)
            endif()
            _qt_internal_json_escape_content("${_value}" _value)
            set_target_properties(${target} PROPERTIES ${_mirror} "${_value}")
        endif()
    endforeach()

    # Cache path-bearing properties on internal `_qt_harmonyos_*`
    # properties for the genex-driven emission helpers below.
    _qt_internal_harmonyos_format_deployment_paths(${target})

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

    # App-level metadata set via qt_set_harmonyos_app_metadata. Properties are
    # read via $<TARGET_PROPERTY:> genex so transitively-set values (e.g. from
    # linked targets) reach the JSON; absent entries leave the template
    # default in place.
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-app-vendor" ${target} _qt_harmonyos_app_vendor)
    _qt_internal_harmonyos_add_deployment_int_property(JSON_CONTENT
        "harmonyos-app-version-code" ${target} QT_HARMONYOS_APP_VERSION_CODE)
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-app-version-name" ${target} _qt_harmonyos_app_version_name)
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-app-label" ${target} _qt_harmonyos_app_label)
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-app-icon" ${target} _qt_harmonyos_app_icon)

    # SDK versions land in entry/build-profile.json5 via harmonydeployqt
    # template substitution.
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-compatible-sdk-version" ${target} _qt_harmonyos_compatible_sdk_version)
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-target-sdk-version" ${target} _qt_harmonyos_target_sdk_version)
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-compile-sdk-version" ${target} _qt_harmonyos_compile_sdk_version)

    # Module-level metadata.
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-module-description" ${target} _qt_harmonyos_module_description)
    _qt_internal_harmonyos_add_deployment_list_property(JSON_CONTENT
        "harmonyos-module-device-types" ${target} QT_HARMONYOS_MODULE_DEVICE_TYPES)
    _qt_internal_harmonyos_add_deployment_property(JSON_CONTENT
        "harmonyos-ability-orientation" ${target} _qt_harmonyos_ability_orientation)

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

    # Package source directory. User-set QT_HARMONYOS_PACKAGE_SOURCE_DIR wins
    # over the bundled template (templates are only built for OHOS, so they
    # live under the target Qt prefix, not QT_HOST_PATH). $<IF:> picks the
    # right value at generate time; if both are empty (Qt install lacks the
    # template AND the user didn't set the property), harmonydeployqt's own
    # template search kicks in on the empty value.
    set(_template_default "")
    set(_template_dir
        "${_qt_install_root}/${_qt_install_data_dir}/src/harmonyos/templates")
    if(EXISTS "${_template_dir}")
        file(REAL_PATH "${_template_dir}" _template_default)
    endif()
    set(_user_pkg_src_genex
        "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},_qt_harmonyos_package_source_dir>>")
    string(APPEND JSON_CONTENT
        ",\n    \"harmonyos-package-source-directory\": "
        "\"$<IF:$<BOOL:${_user_pkg_src_genex}>,${_user_pkg_src_genex},${_template_default}>\""
    )

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

    # QML root + import paths. Both emitted as JSON arrays so multiple roots /
    # import paths can be forwarded to qmlimportscanner.
    _qt_internal_harmonyos_add_deployment_list_property(JSON_CONTENT
        "qml-root-path" ${target} _qt_harmonyos_qml_root_paths)
    _qt_internal_harmonyos_add_deployment_list_property(JSON_CONTENT
        "qml-import-paths" ${target} _qt_harmonyos_qml_import_paths)

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

    # User-supplied extra plugin paths/targets (resolved to $<TARGET_FILE:>
    # in _qt_internal_harmonyos_format_deployment_paths).
    _qt_internal_harmonyos_add_deployment_list_property(JSON_CONTENT
        "harmonyos-extra-plugins" ${target} _qt_harmonyos_extra_plugins)

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
#       [COMPATIBLE_SDK_VERSION <string>]
#       [TARGET_SDK_VERSION <string>]
#       [COMPILE_SDK_VERSION <string>]
#   )
#
# ICON accepts either a host filesystem path (deploy tool copies the image into
# AppScope/resources/base/media/ and emits a $media: reference) or a $media:
# reference (passed through verbatim). Relative paths resolve against the
# call site's CMAKE_CURRENT_SOURCE_DIR.
#
# The *_SDK_VERSION options land in entry/build-profile.json5. Values are
# substituted verbatim, so the user is responsible for the format expected by
# the HarmonyOS toolchain (e.g. COMPATIBLE_SDK_VERSION "5.0.0(12)",
# TARGET_SDK_VERSION "14"). Absent options leave the template default in place.
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
        COMPATIBLE_SDK_VERSION
        TARGET_SDK_VERSION
        COMPILE_SDK_VERSION
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
    if(DEFINED arg_COMPATIBLE_SDK_VERSION)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_COMPATIBLE_SDK_VERSION "${arg_COMPATIBLE_SDK_VERSION}")
    endif()
    if(DEFINED arg_TARGET_SDK_VERSION)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_TARGET_SDK_VERSION "${arg_TARGET_SDK_VERSION}")
    endif()
    if(DEFINED arg_COMPILE_SDK_VERSION)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_COMPILE_SDK_VERSION "${arg_COMPILE_SDK_VERSION}")
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
#       [ORIENTATION <orientation>]
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
#
# ORIENTATION sets the QAbility's screen-orientation policy. When unset, the
# generated module.json5 omits the field entirely, which makes the system
# default ("unspecified", effectively portrait-locked on phones) apply.
# Accepted values are the strings defined by the HarmonyOS module.json5
# schema, including "auto_rotation", "auto_rotation_restricted", "landscape",
# "portrait", "auto_rotation_landscape", "auto_rotation_portrait", and
# "locked". harmonydeployqt validates the value at HAP build time and warns
# (rather than aborts) on an unrecognised string.
function(_qt_internal_set_harmonyos_module_metadata target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Empty or invalid target for setting HarmonyOS module metadata: (${target})")
    endif()

    set(no_value_options "")
    set(single_value_options DESCRIPTION ORIENTATION)
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
    if(DEFINED arg_ORIENTATION)
        set_target_properties(${target} PROPERTIES
            QT_HARMONYOS_ABILITY_ORIENTATION "${arg_ORIENTATION}")
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

function(_qt_internal_harmonyos_test_bundle_settings_file out_var)
    set(${out_var} "${CMAKE_BINARY_DIR}/all_tests-harmony-deployment-settings.json" PARENT_SCOPE)
endfunction()

# Test bundle: deployment-settings JSON for an "uber" HAP containing every
# libtst_*.so under CMAKE_BINARY_DIR.
function(_qt_internal_harmonyos_generate_test_bundle_deployment_settings)
    _qt_internal_harmonyos_get_sdk_ndk_paths(sdk_root ndk_root)

    _qt_internal_harmonyos_test_bundle_settings_file(CONFIG_FILE)

    set(JSON_CONTENT "{\n")
    string(APPEND JSON_CONTENT "    \"test-bundle\": true,\n")
    string(APPEND JSON_CONTENT "    \"test-binaries-directory\": \"${CMAKE_BINARY_DIR}\",\n")
    string(APPEND JSON_CONTENT "    \"harmonyos-app-name\": \"QtAutoTests\",\n")
    string(APPEND JSON_CONTENT "    \"harmonyos-app-bundle-name\": \"org.qtproject.autotests\",\n")
    string(APPEND JSON_CONTENT "    \"harmonyos-target-arch\": [\"arm64-v8a\"]")

    if(sdk_root)
        string(APPEND JSON_CONTENT ",\n    \"sdk-root\": \"${sdk_root}\"")
    endif()

    if(ndk_root)
        string(APPEND JSON_CONTENT ",\n    \"ndk-root\": \"${ndk_root}\"")
    endif()

    _qt_internal_harmonyos_get_qt_install_dirs(
        _qt_install_root _qt_install_libs_dir _qt_install_plugins_dir _qt_install_qml_dir
        _qt_install_data_dir)
    string(APPEND JSON_CONTENT
        ",\n    \"qtLibsDirectory\": \"${_qt_install_root}/${_qt_install_libs_dir}\"")
    string(APPEND JSON_CONTENT
        ",\n    \"qtPluginsDirectory\": \"${_qt_install_root}/${_qt_install_plugins_dir}\"")
    string(APPEND JSON_CONTENT
        ",\n    \"qtQmlDirectory\": \"${_qt_install_root}/${_qt_install_qml_dir}\"")

    if(QT_HOST_PATH)
        string(APPEND JSON_CONTENT
            ",\n    \"qtLibExecsDirectory\": \"${QT_HOST_PATH}/${QT6_HOST_INFO_LIBEXECDIR}\"")
        string(APPEND JSON_CONTENT ",\n    \"qtHostDirectory\": \"${QT_HOST_PATH}\"")
    endif()

    # Templates are built only for OHOS, so they live under the target prefix.
    set(TEMPLATE_DIR
        "${_qt_install_root}/${_qt_install_data_dir}/src/harmonyos/templates")
    if(EXISTS "${TEMPLATE_DIR}")
        file(REAL_PATH "${TEMPLATE_DIR}" TEMPLATE_DIR)
        string(APPEND JSON_CONTENT
            ",\n    \"harmonyos-package-source-directory\": \"${TEMPLATE_DIR}\"")
    endif()

    # Strip the Qt install prefix that qt.toolchain.cmake prepends — we only
    # want user-supplied third-party prefixes (e.g. HARMONYOS_DEPS_ROOT/lib).
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

    # Collect permissions mirroring _qt_internal_harmonyos_generate_deployment_settings():
    # each test target's own permissions first (so explicit overrides win), then the
    # union of all transitive Qt module dependencies. Walk each unique dep only once.
    # De-duplicate by JSON "name" field; first occurrence wins.
    set(_all_permissions "")
    set(_seen_names "")
    set(_all_deps "")
    get_property(_test_targets GLOBAL PROPERTY QT_INTERNAL_HARMONYOS_TEST_TARGETS)
    set_property(GLOBAL PROPERTY QT_INTERNAL_HARMONYOS_TEST_TARGETS "")
    foreach(_test IN LISTS _test_targets)
        if(NOT TARGET "${_test}")
            continue()
        endif()
        get_target_property(_own_perms "${_test}" QT_HARMONYOS_PERMISSIONS)
        if(_own_perms AND NOT _own_perms STREQUAL "_own_perms-NOTFOUND")
            foreach(_entry IN LISTS _own_perms)
                string(JSON _name GET "${_entry}" name)
                if(NOT _name IN_LIST _seen_names)
                    list(APPEND _seen_names "${_name}")
                    list(APPEND _all_permissions "${_entry}")
                endif()
            endforeach()
        endif()
        __qt_internal_collect_all_target_dependencies("${_test}" _deps)
        list(APPEND _all_deps ${_deps})
    endforeach()
    list(REMOVE_DUPLICATES _all_deps)
    foreach(_dep IN LISTS _all_deps)
        get_target_property(_perms "${_dep}" QT_HARMONYOS_PERMISSIONS)
        if(NOT _perms OR _perms STREQUAL "_perms-NOTFOUND")
            continue()
        endif()
        foreach(_entry IN LISTS _perms)
            string(JSON _name GET "${_entry}" name)
            if(NOT _name IN_LIST _seen_names)
                list(APPEND _seen_names "${_name}")
                list(APPEND _all_permissions "${_entry}")
            endif()
        endforeach()
    endforeach()

    if(_all_permissions)
        list(JOIN _all_permissions ",\n        " _permissions_joined)
        string(APPEND JSON_CONTENT
            ",\n    \"permissions\": [\n        ${_permissions_joined}\n    ]")
    endif()

    string(APPEND JSON_CONTENT "\n}\n")

    file(GENERATE OUTPUT "${CONFIG_FILE}" CONTENT "${JSON_CONTENT}")
endfunction()

# Create all_tests_make_hap: invoke harmonydeployqt in test-bundle mode.
# When HARMONYOS_HVIGOR is set, the helper also runs hvigor to build the
# signed HAP; otherwise only the project is generated.
# hvigor writes entry-default-signed.hap when signing is configured and
# entry-default-unsigned.hap otherwise. Pick the matching name so the build
# edge's OUTPUT and the install fixture target what hvigor actually produces.
function(_qt_internal_harmonyos_test_bundle_hap_basename out_var)
    if(DEFINED ENV{QT_HARMONYOS_SIGNING_CERT_PATH}
            OR DEFINED ENV{QT_HARMONYOS_SIGNING_PROFILE}
            OR DEFINED ENV{QT_HARMONYOS_SIGNING_STORE_FILE}
            OR DEFINED ENV{QT_HARMONYOS_SIGNING_KEY_ALIAS}
            OR DEFINED ENV{QT_HARMONYOS_SIGNING_KEY_PASSWORD}
            OR DEFINED ENV{QT_HARMONYOS_SIGNING_STORE_PASSWORD})
        set(${out_var} "entry-default-signed.hap" PARENT_SCOPE)
    else()
        set(${out_var} "entry-default-unsigned.hap" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_harmonyos_add_all_tests_hap_target)
    _qt_internal_harmonyos_test_bundle_settings_file(CONFIG_FILE)
    cmake_language(DEFER CALL _qt_internal_harmonyos_generate_test_bundle_deployment_settings)
    set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/harmonyos-tests-bundle")

    _qt_internal_check_depfile_support(has_depfile_support)

    _qt_internal_harmonyos_test_bundle_hap_basename(_hap_basename)
    set(HAP_OUTPUT_FILE "${OUTPUT_DIR}/entry/build/default/outputs/default/${_hap_basename}")
    set(BINARIES_TXT_FILE "${OUTPUT_DIR}/binaries.txt")
    set(DEP_FILE_PATH "${CMAKE_BINARY_DIR}/all_tests_hap.d")

    if(QT_HOST_PATH)
        set(HARMONYDEPLOYQT_EXECUTABLE
            "${QT_HOST_PATH}/${QT6_HOST_INFO_BINDIR}/harmonydeployqt")
    else()
        set(HARMONYDEPLOYQT_EXECUTABLE
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${QT6_HOST_INFO_BINDIR}/harmonydeployqt")
    endif()

    set(extra_deploy_args "")
    if(HARMONYOS_HVIGOR)
        list(APPEND extra_deploy_args --hvigor "${HARMONYOS_HVIGOR}")
    endif()

    # Without hvigor the signed HAP is never produced and the install fixture
    # would later fail with a confusing "file not found".
    if(NOT HARMONYOS_HVIGOR
            AND (DEFINED ENV{QT_HARMONYOS_SIGNING_CERT_PATH}
                OR DEFINED ENV{QT_HARMONYOS_SIGNING_PROFILE}
                OR DEFINED ENV{QT_HARMONYOS_SIGNING_STORE_FILE}
                OR DEFINED ENV{QT_HARMONYOS_SIGNING_KEY_ALIAS}
                OR DEFINED ENV{QT_HARMONYOS_SIGNING_KEY_PASSWORD}
                OR DEFINED ENV{QT_HARMONYOS_SIGNING_STORE_PASSWORD}))
        message(WARNING "QT_HARMONYOS_SIGNING_* environment variables are set, but "
            "HARMONYOS_HVIGOR is not. The signed test HAP will not be produced; the "
            "HAP install fixture will fail. Pass -DHARMONYOS_HVIGOR=<path-to-hvigorw> "
            "or export QT_HARMONYOS_HVIGOR before configuring.")
    endif()

    # Match the EXCLUDE_FROM_ALL policy that qt_build_tests() applies to the
    # individual test binaries: build the HAP as part of the default target
    # only when QT_BUILD_TESTS_BY_DEFAULT is on.
    if(QT_INTERNAL_TEST_TARGETS_EXCLUDE_FROM_ALL)
        set(_all_keyword "")
    else()
        set(_all_keyword "ALL")
    endif()

    if(has_depfile_support)
        cmake_policy(PUSH)
        if(POLICY CMP0116)
            cmake_policy(SET CMP0116 NEW)
        endif()
        set(relative_to_dir ${CMAKE_BINARY_DIR})

        add_custom_command(OUTPUT "${HAP_OUTPUT_FILE}" "${BINARIES_TXT_FILE}"
            COMMAND ${HARMONYDEPLOYQT_EXECUTABLE}
                --test-bundle
                --input "${CONFIG_FILE}"
                --output "${OUTPUT_DIR}"
                --depfile "${DEP_FILE_PATH}"
                --depfile-base "${relative_to_dir}"
                ${extra_deploy_args}
            DEPENDS "${CONFIG_FILE}"
            DEPFILE "${DEP_FILE_PATH}"
            COMMENT "Generating HarmonyOS test bundle HAP"
            VERBATIM
        )
        cmake_policy(POP)

        add_custom_target(all_tests_make_hap ${_all_keyword}
            DEPENDS "${HAP_OUTPUT_FILE}" "${BINARIES_TXT_FILE}")
    else()
        add_custom_target(all_tests_make_hap ${_all_keyword}
            COMMAND ${HARMONYDEPLOYQT_EXECUTABLE}
                --test-bundle
                --input "${CONFIG_FILE}"
                --output "${OUTPUT_DIR}"
                ${extra_deploy_args}
            DEPENDS "${CONFIG_FILE}"
            COMMENT "Generating HarmonyOS test bundle HAP"
            VERBATIM
        )
    endif()
endfunction()

# FIXTURES_SETUP test that pushes the signed HAP to the device before any
# autotest runs. Requires HARMONYOS_HDC.
function(_qt_internal_harmonyos_add_hap_install_fixture hap_signed_file)
    if(NOT HARMONYOS_HDC)
        return()
    endif()

    set(install_script "${CMAKE_BINARY_DIR}/HarmonyOSInstallHapFixture.cmake")
    file(GENERATE OUTPUT "${install_script}" CONTENT
"set(hdc \"${HARMONYOS_HDC}\")
set(hap \"${hap_signed_file}\")
message(STATUS \"Installing HarmonyOS test HAP: \${hap}\")
execute_process(
    COMMAND \"\${hdc}\" app install -r \"\${hap}\"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
)
message(STATUS \"\${output}\")
if(NOT result EQUAL 0 OR output MATCHES \"error: failed to install bundle\")
    message(FATAL_ERROR \"HAP install failed (exit code: \${result}):\\n\${output}\")
endif()
"
    )

    add_test(NAME HarmonyOSTestBundleInstall
        COMMAND "${CMAKE_COMMAND}" -P "${install_script}"
    )
    set_tests_properties(HarmonyOSTestBundleInstall PROPERTIES
        FIXTURES_SETUP HarmonyOSTestBundle
        RUN_SERIAL TRUE
        TIMEOUT 120
    )
endfunction()

# Entry point. Idempotent; safe to call from qt_build_tests() for every module.
function(qt_internal_add_harmonyos_test_bundle)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "OHOS")
        message(WARNING "qt_internal_add_harmonyos_test_bundle() called on non-OHOS platform")
        return()
    endif()
    if(TARGET all_tests_make_hap)
        return()
    endif()
    _qt_internal_harmonyos_add_all_tests_hap_target()

    set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/harmonyos-tests-bundle")
    _qt_internal_harmonyos_test_bundle_hap_basename(_hap_basename)
    set(HAP_INSTALL_FILE
        "${OUTPUT_DIR}/entry/build/default/outputs/default/${_hap_basename}")
    _qt_internal_harmonyos_add_hap_install_fixture("${HAP_INSTALL_FILE}")
endfunction()
