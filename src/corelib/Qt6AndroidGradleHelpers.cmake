# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Returns the path to the template file from either user defined template directory, or
# Qt default template directory.
function(_qt_internal_android_get_template_path out_var target template_name)
    if(template_name STREQUAL "")
        message(FATAL_ERROR "Template name is empty."
            " This is a Qt issue, please report a bug at https://bugreports.qt.io.")
    endif()

    _qt_internal_android_template_dir(template_directory)
    get_filename_component(template_directory "${template_directory}" ABSOLUTE)

    # The paths are ordered according to their priority, from highest to lowest.
    set(possible_paths
        "${template_directory}/${template_name}.in"
    )

    get_target_property(android_target_type ${target} _qt_android_target_type)
    if(android_target_type STREQUAL "APPLICATION")
        _qt_internal_android_get_package_source_dir(user_template_directory ${target})
        get_filename_component(user_template_directory "${user_template_directory}" ABSOLUTE)

        # Add possible user manifest first so it takes precedence.
        if(template_name STREQUAL "app/AndroidManifest.xml")
            set(user_manifest "${user_template_directory}/AndroidManifest.xml")
            if(EXISTS "${user_manifest}")
                list(PREPEND possible_paths "${user_manifest}")
            endif()
        endif()

        # Add user template with the higher priority
        list(PREPEND possible_paths "${user_template_directory}/${template_name}.in")

        # When the user’s package source dir is already the module's subdir
        # (i.e app/ or dynamic_feature/), then don't add yet another app subdir.
        if(user_template_directory)
            get_filename_component(user_template_dir_basename "${user_template_directory}" NAME)
            _qt_internal_re_escape(user_template_dir_basename_re "${user_template_dir_basename}")
            if(template_name MATCHES "^${user_template_dir_basename_re}/(.+)$")
                set(trailing_name "${CMAKE_MATCH_1}")
                list(INSERT possible_paths 1 "${user_template_directory}/${trailing_name}.in")
            endif()
        endif()
    endif()

    set(template_path "")
    foreach(possible_path IN LISTS possible_paths)
        if(EXISTS "${possible_path}")
            set(template_path "${possible_path}")
            break()
        endif()
    endforeach()

    if(template_path STREQUAL "")
        message(FATAL_ERROR "'${template_name}' is not found."
            " This is a Qt issue, please report a bug at https://bugreports.qt.io.")
    endif()

    set(${out_var} "${template_path}" PARENT_SCOPE)
endfunction()

# Generates the settings.gradle file for the target. Writes the result to the target android build
# directory.
function(_qt_internal_android_generate_bundle_settings_gradle target)
    set(settings_gradle_filename "settings.gradle")
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(settings_gradle_file "${android_build_dir}/${settings_gradle_filename}")

    # Skip generating the file if it's already provided by user.
    get_target_property(deployment_files ${target} _qt_android_deployment_files)
    if("${settings_gradle_file}" IN_LIST deployment_files)
        return()
    endif()

    _qt_internal_android_get_template_path(template_file ${target} "${settings_gradle_filename}")

    _qt_internal_android_get_gradle_property(ROOT_PROJECT_NAME ${target}
        QT_ANDROID_APP_NAME "${target}")

    set(target_dynamic_features "$<TARGET_PROPERTY:${target},_qt_android_dynamic_features>")
    set(include_prefix "include(\":")
    set(include_suffix "\")")
    set(include_glue "${include_suffix}\n${include_prefix}")
    string(JOIN "" SUBPROJECTS
        "$<$<BOOL:${target_dynamic_features}>:"
            "${include_prefix}"
            "$<JOIN:${target_dynamic_features},${include_glue}>"
            "${include_suffix}"
        ">"
    )

    _qt_internal_configure_file(GENERATE OUTPUT ${settings_gradle_file}
        INPUT "${template_file}")
    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files
        "${settings_gradle_file}")
endfunction()

# Generates the source sets for the target.
function(_qt_internal_android_get_gradle_source_sets out_var target)
    set(known_types java aidl res resources renderscript assets jniLibs)
    set(source_set "")
    set(indent "\n            ")
    foreach(type IN LISTS known_types)
        set(source_dirs
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},_qt_android_gradle_${type}_source_dirs>>")
        string(JOIN "" source_set
            "${source_set}"
            "$<$<BOOL:${source_dirs}>:"
                "${indent}${type}.srcDirs = ['$<JOIN:${source_dirs},'$<COMMA> '>']"
            ">"
        )
    endforeach()

    set(manifest
        "$<TARGET_PROPERTY:${target},_qt_android_manifest>")
    string(JOIN "" source_set
        "${source_set}"
        "$<$<BOOL:${manifest}>:"
            "${indent}manifest.srcFile '${manifest}'"
        ">"
    )
    string(REGEX REPLACE "^${indent}" "" source_set "${source_set}")
    set(${out_var} "${source_set}" PARENT_SCOPE)
endfunction()

# Generates the gradle dependency list for the target.
function(_qt_internal_android_get_gradle_dependencies out_var target)
    # Use dependencies from file tree by default
    set(known_dependencies
        "implementation fileTree(dir: 'libs', include: ['*.jar', '*.aar'])")
    foreach(dep_type implementation api)
        string(JOIN "\n    " dep_prefix
            "\n    //noinspection GradleDependency"
            "${dep_type} "
        )
        set(dep_postfix "")
        set(dep_type_property "_qt_android_gradle_${dep_type}_dependencies")
        set(dep_property "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${dep_type_property}>>")
        string(JOIN "" known_dependencies
            "${known_dependencies}"
            "$<$<BOOL:${dep_property}>:"
                "${dep_prefix}$<JOIN:${dep_property},${dep_postfix}${dep_prefix}>${dep_postfix}"
            ">"
        )
    endforeach()
    set(${out_var} "${known_dependencies}" PARENT_SCOPE)
endfunction()

# Sets the default values of the gradle properties for the Android executable target.
function(_qt_internal_set_android_application_gradle_defaults target)
    _qt_internal_android_java_dir(android_java_dir)

    set(target_dynamic_features "$<TARGET_PROPERTY:${target},_qt_android_dynamic_features>")
    string(JOIN "" implementation_dependencies
        "$<$<BOOL:${target_dynamic_features}>:'com.google.android.play:feature-delivery:2.1.0'>"
    )
    # TODO: make androidx.core:core version configurable.
    # Currently, it is hardcoded to 1.17.0.
    list(APPEND implementation_dependencies "'androidx.core:core:1.17.0'")

    set_target_properties(${target} PROPERTIES
        _qt_android_gradle_java_source_dirs "${android_java_dir}/src;src;java"
        _qt_android_gradle_aidl_source_dirs "${android_java_dir}/src;src;aidl"
        _qt_android_gradle_res_source_dirs "${android_java_dir}/res;res"
        _qt_android_gradle_resources_source_dirs "resources"
        _qt_android_gradle_renderscript_source_dirs "src"
        _qt_android_gradle_assets_source_dirs "assets"
        _qt_android_gradle_jniLibs_source_dirs "libs"
        _qt_android_manifest "AndroidManifest.xml"
        _qt_android_gradle_implementation_dependencies "${implementation_dependencies}"
    )
endfunction()

# Generates the build.gradle file for the target. Writes the result to the target app deployment
# directory.
function(_qt_internal_android_generate_target_build_gradle target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "DEPLOYMENT_DIR" "")

    if(NOT arg_DEPLOYMENT_DIR)
        message(FATAL_ERROR "DEPLOYMENT_DIR is not specified.")
    endif()

    set(build_gradle_filename "build.gradle")
    set(out_file "${arg_DEPLOYMENT_DIR}/${build_gradle_filename}")

    # Skip generating the file if it's already provided by user.
    get_target_property(deployment_files ${target} _qt_android_deployment_files)
    if("${out_file}" IN_LIST deployment_files)
        return()
    endif()

    # TODO: The current build.gradle.in templates hardcodes couple values that needs to be
    # configurable in the future. For example the buildscript dependencies, or the use of
    # androidx.core:core:1.13.1 and the dependency for all user applications.

    _qt_internal_android_get_gradle_property(PACKAGE_NAME ${target}
        QT_ANDROID_PACKAGE_NAME "org.qtproject.example.$<MAKE_C_IDENTIFIER:${target}>")

    _qt_internal_android_get_target_sdk_build_tools_revision(ANDROID_BUILD_TOOLS_VERSION
        ${target})

    _qt_internal_detect_latest_android_platform(ANDROID_COMPILE_SDK_VERSION)
    if(NOT ANDROID_COMPILE_SDK_VERSION)
        message(FATAL_ERROR "Unable to detect the android platform in ${ANDROID_SDK_ROOT}. "
            "Please check your Android SDK installation.")
    endif()

    # Prefer base API when the latest platform uses a dotted API (e.g. android-36.1).
    if(ANDROID_COMPILE_SDK_VERSION MATCHES "^android-([0-9]+)\.[0-9]+$")
        set(android_compile_sdk_base "android-${CMAKE_MATCH_1}")
        if(ANDROID_SDK_ROOT AND EXISTS "${ANDROID_SDK_ROOT}/platforms/${android_compile_sdk_base}")
            set(ANDROID_COMPILE_SDK_VERSION "${android_compile_sdk_base}")
        endif()
    endif()

    _qt_internal_android_get_gradle_source_sets(SOURCE_SETS ${target})
    _qt_internal_android_get_gradle_dependencies(GRADLE_DEPENDENCIES ${target})

    _qt_internal_android_get_gradle_property(min_sdk_version ${target}
        QT_ANDROID_MIN_SDK_VERSION "28")

    _qt_internal_android_get_gradle_property(target_sdk_version ${target}
        QT_ANDROID_TARGET_SDK_VERSION "36")

    set(target_abis "$<TARGET_PROPERTY:${target},_qt_android_abis>")
    set(target_abi_list "$<JOIN:${target_abis};${CMAKE_ANDROID_ARCH_ABI},'$<COMMA> '>")

    string(JOIN "\n        " DEFAULT_CONFIG_VALUES
        "resConfig 'en'"
        "minSdkVersion ${min_sdk_version}"
        "targetSdkVersion ${target_sdk_version}"
        "ndk.abiFilters = ['${target_abi_list}']"
    )

    set(target_dynamic_features "$<TARGET_PROPERTY:${target},_qt_android_dynamic_features>")
    set(include_prefix "\":")
    set(include_suffix "\"")
    set(include_glue "${include_suffix}$<COMMA>${include_prefix}")
    string(APPEND ANDROID_DEPLOYMENT_EXTRAS
        "$<$<BOOL:${target_dynamic_features}>:dynamicFeatures = ["
            "${include_prefix}"
            "$<JOIN:${target_dynamic_features},${include_glue}>"
            "${include_suffix}]"
        ">"
    )

    get_target_property(android_target_type ${target} _qt_android_target_type)
    if(android_target_type STREQUAL "APPLICATION")
        set(GRADLE_PLUGIN_TYPE "com.android.application")
        set(template_subdir "app")
    elseif(android_target_type STREQUAL "DYNAMIC_FEATURE")
        set(GRADLE_PLUGIN_TYPE "com.android.dynamic-feature")
        set(template_subdir "dynamic_feature")
    else()
        message(FATAL_ERROR "Unsupported target type for android bundle deployment ${target}")
    endif()

    _qt_internal_android_get_template_path(template_file ${target}
        "${template_subdir}/${build_gradle_filename}")
    _qt_internal_configure_file(GENERATE
        OUTPUT "${out_file}"
        INPUT "${template_file}"
    )
    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${out_file}")
endfunction()

# Prepares the artifacts for the gradle build of the target.
function(_qt_internal_android_prepare_gradle_build target)
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    _qt_internal_android_get_target_deployment_dir(deployment_dir ${target})

    _qt_internal_android_copy_gradle_files(${target} "${android_build_dir}")
    _qt_internal_android_copy_target_package_sources(${target} "${deployment_dir}")
    _qt_internal_android_copy_android_resources(${target} "${deployment_dir}")
    _qt_internal_android_copy_app_binary(${target} "${deployment_dir}")
    _qt_internal_android_copy_stdlib(${target} "${deployment_dir}")
    _qt_internal_android_copy_extra_libs(${target} "${deployment_dir}")
    _qt_internal_android_copy_extra_plugins(${target} "${deployment_dir}")
    _qt_internal_android_copy_qml_dependencies(${target} "${deployment_dir}")
    _qt_internal_android_copy_non_qt_linked_libs(${target} "${deployment_dir}")
    _qt_internal_android_copy_qt_dependencies(${target} "${deployment_dir}")
    _qt_internal_android_copy_qml_plugins(${target} "${deployment_dir}")
    _qt_internal_android_generate_libs_xml(${target} "${deployment_dir}")

    _qt_internal_android_generate_bundle_gradle_properties(${target})
    _qt_internal_android_generate_bundle_settings_gradle(${target})
    _qt_internal_android_generate_bundle_local_properties(${target})
    _qt_internal_android_generate_target_build_gradle(${target} DEPLOYMENT_DIR "${deployment_dir}")
    _qt_internal_android_generate_target_gradle_properties(${target}
        DEPLOYMENT_DIR "${deployment_dir}")
    _qt_internal_android_generate_target_android_manifest(${target}
        DEPLOYMENT_DIR "${deployment_dir}")


    _qt_internal_android_add_gradle_build(${target} apk)
    _qt_internal_android_add_gradle_build(${target} aab)

    # Make global apk, aab, and aar targets depend on the respective targets.
    _qt_internal_android_add_global_package_dependencies(${target})
    _qt_internal_create_global_apk_all_target_if_needed()
endfunction()

# Returns the path to the output package file for the target.
function(_qt_internal_android_get_output_package_name out_var target)
    _qt_internal_android_package_path(package_build_dir ${target} ${type})
    _qt_internal_android_get_deployment_type_option(deployment_type_suffix "release" "debug")
    if(type STREQUAL "apk")
        _qt_internal_android_get_deployment_type_option(extra_suffix "-unsigned" "")
    else()
        set(extra_suffix "")
    endif()

    set(output_dir "${package_build_dir}/${deployment_type_suffix}")
    set(${out_var} "${output_dir}/app-${deployment_type_suffix}${extra_suffix}.${type}"
        PARENT_SCOPE)
endfunction()

# Adds the modern gradle build targets.
# These targets use the settings.gradle based build directory structure.
function(_qt_internal_android_add_gradle_build target type)
    _qt_internal_android_get_deployment_type_option(android_deployment_type_option
        "assembleRelease" "assembleDebug")

    _qt_internal_android_gradlew_name(gradlew_file_name)
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(gradlew "${android_build_dir}/${gradlew_file_name}")

    set(extra_args "")
    if(type STREQUAL "aab")
        set(extra_args "bundle")
    endif()

    set(package_file_path "${android_build_dir}/${target}.${type}")

    _qt_internal_android_get_output_package_name(package_build_file_path ${target} ${type})

    set(extra_deps "")
    if(TARGET ${target}_copy_feature_names)
        list(APPEND extra_deps ${target}_copy_feature_names)
    endif()

    if(TARGET ${target}_deploy_dynamic_features)
        list(APPEND extra_deps ${target}_deploy_dynamic_features)
    endif()

    foreach(qml_dep_target ${target}_copy_qml_plugins ${target}_build_qml_bundle)
        if(TARGET ${qml_dep_target})
            list(APPEND extra_deps ${qml_dep_target})
        endif()
    endforeach()

    _qt_internal_android_gradle_cleanup_commands(gradle_cleanup_commands
        "${gradlew}" "${android_build_dir}" "${target}")

    set(gradle_scripts "$<TARGET_PROPERTY:${target},_qt_android_deployment_files>")
    add_custom_command(OUTPUT "${package_file_path}"
        BYPRODUCTS "${package_build_file_path}"
        COMMAND
            "${gradlew}" ${android_deployment_type_option} ${extra_args}
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${package_build_file_path}" "${package_file_path}"
        ${gradle_cleanup_commands}
        DEPENDS
            ${target}
            ${gradle_scripts}
            ${target}_copy_gradle_files
            ${target}_copy_android_res_files
            ${target}_copy_app_binary
            ${target}_copy_stdlib
            ${target}_copy_non_qt_linked_libs
            ${target}_copy_extra_libs
            ${target}_copy_extra_plugins
            ${target}_copy_qml_plugins
            ${target}_copy_qt_files
            ${target}_update_libs_xml
            ${extra_deps}
        WORKING_DIRECTORY
            "${android_build_dir}"
        VERBATIM
    )

    _qt_internal_android_sign_package(signed_package ${target} ${type})

    add_custom_target(${target}_make_${type} DEPENDS "${package_file_path}" "${signed_package}")
endfunction()

function(_qt_internal_android_sign_package out_file target type)
    string(TOUPPER "${type}" type_upper)
    if(NOT QT_ANDROID_SIGN_${type_upper})
        set(${out_file} "" PARENT_SCOPE)
        return()
    endif()

    if(type STREQUAL "aab")
        find_program(jarsigner NAMES jarsigner)
        if(NOT jarsigner)
            message(FATAL_ERROR "jarsigner is not found. Please install"
                " a JDK version to sign '${target}'.")
        endif()

        set(program ${jarsigner})
        set(extra_args "")
    elseif(type STREQUAL "apk")
        _qt_internal_android_get_target_sdk_build_tools_revision(build_tools_version ${target})
        if(CMAKE_HOST_WIN32)
            set(suffix ".bat")
        else()
            set(suffix "")
        endif()
        set(build_tools_base_path "${ANDROID_SDK_ROOT}/build-tools/${build_tools_version}")
        set(program "${build_tools_base_path}/apksigner${suffix}")
        set(extra_args "-DZIPALIGN_PATH=${build_tools_base_path}/build-tools/zipalign${suffix}")
    endif()

    _qt_internal_android_get_output_package_name(package_build_file_path ${target} ${type})

    _qt_internal_android_package_path(package_build_dir ${target} ${type})
    _qt_internal_android_get_deployment_type_option(deployment_type_suffix
        "release" "debug")
    set(base_output_path "${package_build_dir}/${deployment_type_suffix}")
    set(package_build_file_path_signed
        "${base_output_path}/app-${deployment_type_suffix}-signed.${type}")

    set(package_file_path "${android_build_dir}/${target}-signed.${type}")

    set(sign_package_script "${_qt_6_config_cmake_dir}/QtAndroidSignPackage.cmake")
    add_custom_command(OUTPUT "${package_file_path}"
        BYPRODUCTS "${package_build_file_path_signed}"
        COMMAND
            ${CMAKE_COMMAND} "-DPROGRAM=${program}" "-DUNSIGNED_PACKAGE=${package_build_file_path}"
            "-DSIGNED_PACKAGE=${package_build_file_path_signed}" ${extra_args}
            -P "${sign_package_script}"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${package_build_file_path_signed}" "${package_file_path}"
        DEPENDS
            ${package_build_file_path}
            ${sign_package_script}
        WORKING_DIRECTORY
            "${android_build_dir}"
        VERBATIM
    )

    set(${out_file} "${package_file_path}" PARENT_SCOPE)
endfunction()

# Returns the path to the android executable package either apk or aab.
function(_qt_internal_android_package_path out_var target type)
    set(supported_package_types apk aab)
    if(NOT type IN_LIST supported_package_types)
        message(FATAL_ERROR "Invalid package type, supported types: ${supported_package_types}")
    endif()

    # aab packages are located in the bundle directory
    if(type STREQUAL "aab")
        set(type "bundle")
    endif()

    _qt_internal_android_get_target_deployment_dir(deployment_dir ${target})

    set(${out_var} "${deployment_dir}/build/outputs/${type}" PARENT_SCOPE)
endfunction()

# Returns the path to the gradle build directory.
function(_qt_internal_android_gradle_template_dir out_var)
    if(PROJECT_NAME STREQUAL "QtBase" OR QT_SUPERBUILD)
        set(${out_var} "${QtBase_SOURCE_DIR}/src/3rdparty/gradle" PARENT_SCOPE)
    else()
        set(${out_var} "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_DATA}/src/3rdparty/gradle" PARENT_SCOPE)
    endif()
endfunction()

# Returns the path to the Android templates resource directory.
function(_qt_internal_android_template_res_dir out_var)
    if(PROJECT_NAME STREQUAL "QtBase" OR QT_SUPERBUILD)
        set(android_templates_root "${QtBase_SOURCE_DIR}/src/android")
    else()
        set(android_templates_root
            "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_DATA}/src/android")
    endif()

    set(template_res_dir "${android_templates_root}/templates/res")
    if(NOT EXISTS "${template_res_dir}")
        message(FATAL_ERROR "Android template resource directory '${template_res_dir}' not found."
            " Please check your Qt installation.")
    endif()
    set(${out_var} "${template_res_dir}" PARENT_SCOPE)
endfunction()

# Returns the path to the android java dir.
function(_qt_internal_android_java_dir out_var)
    if(PROJECT_NAME STREQUAL "QtBase" OR QT_SUPERBUILD)
        set(${out_var} "${QtBase_SOURCE_DIR}/src/android/java" PARENT_SCOPE)
    else()
        set(${out_var} "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_DATA}/src/android/java" PARENT_SCOPE)
    endif()
endfunction()

# Returns the platform-spefic name of the gradlew script.
function(_qt_internal_android_gradlew_name out_var)
    if(CMAKE_HOST_WIN32)
        set(gradlew_file_name "gradlew.bat")
    else()
        set(gradlew_file_name "gradlew")
    endif()

    set(${out_var} "${gradlew_file_name}" PARENT_SCOPE)
endfunction()

# Return the path to the gradlew script.
function(_qt_internal_android_gradlew_path out_var target)
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(${out_var} "${android_build_dir}/${gradlew_file_name}" PARENT_SCOPE)
endfunction()

# Return Gradle clean command if QT_ANDROID_POST_BUILD_GRADLE_CLEANUP is set
function(_qt_internal_android_gradle_cleanup_commands out_var gradlew working_dir target)
    if(NOT QT_ANDROID_POST_BUILD_GRADLE_CLEANUP)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(commands "")
    list(APPEND commands
        COMMAND ${CMAKE_COMMAND} -E echo "Running gradle clean for ${target} in ${working_dir}..."
        COMMAND "${gradlew}" -p "${working_dir}" clean
    )
    set(${out_var} "${commands}" PARENT_SCOPE)
endfunction()

# Returns the generator expression for the gradle_property value. Defaults to the default_value
# argument.
function(_qt_internal_android_get_gradle_property out_var target target_property default_value)
    set(target_property_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},${target_property}>>")
    string(JOIN "" result
        "$<IF:$<BOOL:${target_property_genex}>,"
            "${target_property_genex},"
            "${default_value}"
        ">"
    )
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Generates gradle.properties for the specific target. Usually contains the
# target build type(executable, dynamic feature, library).
function(_qt_internal_android_generate_target_gradle_properties target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "DEPLOYMENT_DIR" "")

    if(NOT arg_DEPLOYMENT_DIR)
        message(FATAL_ERROR "DEPLOYMENT_DIR is not specified.")
    endif()

    set(gradle_properties_file_name "gradle.properties")
    set(out_file "${arg_DEPLOYMENT_DIR}/${gradle_properties_file_name}")
    # Skip generating the file if it's already provided by user.
    get_target_property(deployment_files ${target} _qt_android_deployment_files)
    if("${out_file}" IN_LIST deployment_files)
        return()
    endif()

    _qt_internal_android_get_template_path(template_file ${target}
        "app/${gradle_properties_file_name}")
    _qt_internal_configure_file(CONFIGURE
        OUTPUT "${out_file}"
        INPUT "${template_file}"
    )
    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${out_file}")
endfunction()

# Constucts generator expression that returns either target property or the default value
function(_qt_internal_android_get_manifest_property out_var target property default)
    set(target_property "$<TARGET_PROPERTY:${target},${property}>")
    string(JOIN "" out_genex
        "$<IF:$<BOOL:${target_property}>,"
            "${target_property},"
            "${default}"
        ">"
    )

    set(${out_var} "${out_genex}" PARENT_SCOPE)
endfunction()

# Converts androiddeployqt-style placeholders to CMake placeholders.
function(_qt_internal_android_convert_manifest_placeholders manifest_content out_placeholders)
    set(updated "${manifest_content}")
    if(updated MATCHES "%%INSERT_")
        string(REPLACE "-- %%INSERT_VERSION_CODE%% --" "@APP_VERSION_CODE@"
            updated "${updated}")
        string(REPLACE "-- %%INSERT_VERSION_NAME%% --" "@APP_VERSION_NAME@"
            updated "${updated}")
        string(REPLACE "-- %%INSERT_APP_NAME%% --" "@APP_NAME@"
            updated "${updated}")
        string(REPLACE "-- %%INSERT_APP_LIB_NAME%% --" "@APP_LIB_NAME@"
            updated "${updated}")
        string(REPLACE "-- %%INSERT_APP_ARGUMENTS%% --" "@APP_ARGUMENTS@"
            updated "${updated}")
        string(REPLACE "-- %%INSERT_APP_ICON%% --" "@APP_ICON@"
            updated "${updated}")
        string(REPLACE "android:icon=\"@APP_ICON@\"" "@APP_ICON@"
            updated "${updated}")
        string(REPLACE "<!-- %%INSERT_PERMISSIONS -->" "@APP_PERMISSIONS@"
            updated "${updated}")
        string(REPLACE "<!-- %%INSERT_FEATURES -->" "@APP_FEATURES@"
            updated "${updated}")
        string(REGEX REPLACE "package=\"[^\"]*\"" "package=\"@APP_PACKAGE_NAME@\""
            updated "${updated}")
    endif()

    set(${out_placeholders} "${updated}" PARENT_SCOPE)
endfunction()

# Generates the target AndroidManifest.xml
function(_qt_internal_android_generate_target_android_manifest target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "DEPLOYMENT_DIR" "")

    if(NOT arg_DEPLOYMENT_DIR)
        message(FATAL_ERROR "DEPLOYMENT_DIR is not specified.")
    endif()

    set(android_manifest_filename "AndroidManifest.xml")
    set(out_file "${arg_DEPLOYMENT_DIR}/${android_manifest_filename}")

    # Skip generating the file if it's already provided by user.
    get_target_property(deployment_files ${target} _qt_android_deployment_files)
    if("${out_file}" IN_LIST deployment_files)
        return()
    endif()

    _qt_internal_android_get_template_path(template_file ${target}
        "app/${android_manifest_filename}")
    set(temporary_file "${out_file}.tmp")

    # The file cannot be generated at cmake configure time because target properties
    # (app name, version, permissions) are resolved at generate/build time. We use a
    # temporary file and copy it as a build step to keep the manifest in sync.
    set(manifest_depends
        "${template_file}"
        "${temporary_file}"
    )
    if(TARGET ${target}_copy_package_sources)
        list(APPEND manifest_depends ${target}_copy_package_sources)
    endif()

    add_custom_command(OUTPUT "${out_file}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${temporary_file}" "${out_file}"
        DEPENDS ${manifest_depends}
    )

    _qt_internal_android_get_manifest_property(APP_PACKAGE_NAME ${target}
        QT_ANDROID_PACKAGE_NAME "org.qtproject.example.$<MAKE_C_IDENTIFIER:${target}>")
    _qt_internal_android_get_manifest_property(APP_NAME ${target}
        QT_ANDROID_APP_NAME "${target}")
    _qt_internal_android_get_manifest_property(APP_VERSION_CODE ${target}
        QT_ANDROID_VERSION_CODE "1")
    _qt_internal_android_get_manifest_property(APP_VERSION_NAME ${target}
        QT_ANDROID_VERSION_NAME "1")
    _qt_internal_android_get_manifest_property(APP_LIB_NAME ${target} OUTPUT_NAME "${target}")

    # For application icon we substitute the whole attribute definition, but not only value
    # otherwise it leads to the Manifest processing issue.
    set(target_property "$<TARGET_PROPERTY:${target},QT_ANDROID_APP_ICON>")
    string(JOIN "" APP_ICON
        "$<$<BOOL:${target_property}>:"
            "android:icon=\"${target_property}\""
        ">"
    )

    file(READ "${template_file}" manifest_content)
    _qt_internal_android_convert_manifest_placeholders("${manifest_content}" manifest_content)
    string(REPLACE ">" "$<ANGLE-R>" manifest_content "${manifest_content}")
    string(REPLACE ";" "$<SEMICOLON>" manifest_content "${manifest_content}")
    string(REPLACE "," "$<COMMA>" manifest_content "${manifest_content}")

    _qt_internal_android_convert_permissions(APP_PERMISSIONS ${target} XML)

    set(feature_prefix "\n    <uses-feature android:name=\"")
    set(feature_suffix " \" android:required=\"false\" /$<ANGLE-R>")
    set(feature_property "$<TARGET_PROPERTY:${target},QT_ANDROID_FEATURES>")
    string(JOIN "" APP_FEATURES
        "$<$<BOOL:${feature_property}>:"
            "${feature_prefix}"
            "$<JOIN:${feature_property},${feature_suffix},${feature_prefix}>"
            "${feature_suffix}"
        ">"
    )

    set(APP_ARGUMENTS "${QT_ANDROID_APPLICATION_ARGUMENTS}")

    _qt_internal_configure_file(GENERATE OUTPUT "${temporary_file}"
        CONTENT "${manifest_content}")

    set_property(TARGET ${target} APPEND PROPERTY
        _qt_android_deployment_files "${out_file}" "${temporary_file}")
endfunction()

# Generates the top-level gradle.properties in the android-build directory
# The file contains the information about the versions of the android build
# tools, the list of supported ABIs.
function(_qt_internal_android_generate_bundle_gradle_properties target)
    set(EXTRA_PROPERTIES "")

    set(gradle_properties_file_name "gradle.properties")
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(out_file "${android_build_dir}/${gradle_properties_file_name}")

    # Skip generating the file if it's already provided by user.
    get_target_property(deployment_files ${target} _qt_android_deployment_files)
    if("${out_file}" IN_LIST deployment_files)
        return()
    endif()

    _qt_internal_android_get_template_path(template_file ${target} "${gradle_properties_file_name}")
    _qt_internal_configure_file(CONFIGURE
        OUTPUT "${out_file}"
        INPUT "${template_file}"
    )
    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${out_file}")
endfunction()

# Generates the local.properties for gradle builds. Contains the path to the
# Android SDK root.
function(_qt_internal_android_generate_bundle_local_properties target)
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(out_file "${android_build_dir}/local.properties")

    # Skip generating the file if it's already provided by user.
    get_target_property(deployment_files ${target} _qt_android_deployment_files)
    if("${out_file}" IN_LIST deployment_files)
        return()
    endif()

    file(TO_CMAKE_PATH "${ANDROID_SDK_ROOT}" ANDROID_SDK_ROOT_NATIVE)
    _qt_internal_configure_file(CONFIGURE OUTPUT "${out_file}"
        CONTENT "sdk.dir=${ANDROID_SDK_ROOT_NATIVE}\n")
endfunction()

# Copies the customized Android package sources to the Android build directory
function(_qt_internal_android_copy_target_package_sources target deployment_dir)
    _qt_internal_android_get_package_source_dir(package_source_dir ${target})

    if(NOT package_source_dir)
        set_property(TARGET ${target} PROPERTY _qt_android_package_res_outputs "")
        return()
    endif()
    get_filename_component(package_source_dir "${package_source_dir}" ABSOLUTE)

    # Collect deployment files from use-defined package source directory
    file(GLOB_RECURSE package_files
        LIST_DIRECTORIES false
        RELATIVE "${package_source_dir}"
        "${package_source_dir}/*"
    )

    # Skip manifest copying because it will be handled and generated under
    # _qt_internal_android_generate_target_android_manifest().
    list(REMOVE_ITEM package_files "AndroidManifest.xml")

    # Save res outputs so default resources templates know to not override them.
    set(package_res_files "${package_files}")
    list(FILTER package_res_files INCLUDE REGEX "^res/")
    list(TRANSFORM package_res_files PREPEND "${deployment_dir}/"
        OUTPUT_VARIABLE package_res_outputs)
    set_property(TARGET ${target} PROPERTY _qt_android_package_res_outputs "${package_res_outputs}")

    # Do not copy files that we treat as CMake templates, having '.in' extention.
    #
    # TODO: If it ever will be an issue we may exclude only templates that are
    # known by our build system.
    list(FILTER package_files EXCLUDE REGEX ".+\\.in$")

    list(TRANSFORM package_files PREPEND "${deployment_dir}/" OUTPUT_VARIABLE out_package_files)
    list(TRANSFORM package_files PREPEND "${package_source_dir}/" OUTPUT_VARIABLE in_package_files)

    if(in_package_files)
        # TODO: Add cmake < 3.26 support
        if(CMAKE_VERSION VERSION_LESS 3.26)
            message(FATAL_ERROR "The use of QT_ANDROID_PACKAGE_SOURCE_DIR property with
                the QT_USE_ANDROID_MODERN_BUNDLE option enabled requires CMake version >= 3.26.")
        endif()
        set(copy_commands COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different
            "${package_source_dir}" "${deployment_dir}")
    else()
        # We actually have nothing to deploy.
        return()
    endif()

    add_custom_command(OUTPUT ${out_package_files}
        ${copy_commands}
        DEPENDS
            ${in_package_files}
        VERBATIM
    )

    set_target_properties(${target} PROPERTIES _qt_android_deployment_files "${out_package_files}")

    # This is used by _qt_internal_android_generate_target_android_manifest()
    # to ensure it's run after the package sources are copied.
    add_custom_target(${target}_copy_package_sources DEPENDS ${out_package_files})
endfunction()

# Copies gradle scripts to a build directory.
function(_qt_internal_android_copy_gradle_files target output_directory)
    _qt_internal_android_gradlew_name(gradlew_file_name)
    _qt_internal_android_gradle_template_dir(gradle_template_dir)

    set(gradlew_file_src "${gradle_template_dir}/${gradlew_file_name}")
    set(gradlew_file_dst "${output_directory}/${gradlew_file_name}")

    add_custom_command(OUTPUT "${gradlew_file_dst}"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${gradlew_file_src}" "${gradlew_file_dst}"
        DEPENDS "${gradlew_file_src}"
        COMMENT "Copying gradlew script for ${target}"
        VERBATIM
    )

    # TODO: make a more precise directory copying
    set(gradle_dir_src "${gradle_template_dir}/gradle")
    set(gradle_dir_dst "${output_directory}/gradle")
    add_custom_command(OUTPUT "${gradle_dir_dst}"
        COMMAND
            ${CMAKE_COMMAND} -E copy_directory "${gradle_dir_src}" "${gradle_dir_dst}"
        DEPENDS "${gradle_dir_src}"
        COMMENT "Copying gradle support files for ${target}"
        VERBATIM
    )

    add_custom_target(${target}_copy_gradle_files
        DEPENDS
            "${gradlew_file_dst}"
            "${gradle_dir_dst}"
    )
endfunction()

# Copies default Android resource templates to a build directory.
function(_qt_internal_android_copy_android_resources target deployment_dir)
    _qt_internal_android_template_res_dir(template_res_dir)

    set(res_dir_dst "${deployment_dir}/res")
    file(GLOB_RECURSE template_res_files_rel RELATIVE "${template_res_dir}" CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        "${template_res_dir}/*"
    )
    if(NOT template_res_files_rel)
        return()
    endif()

    list(TRANSFORM template_res_files_rel PREPEND "${template_res_dir}/"
        OUTPUT_VARIABLE template_res_files)
    list(TRANSFORM template_res_files_rel PREPEND "${res_dir_dst}/"
        OUTPUT_VARIABLE dst_res_files)

    # Exclude res outputs that are copied by QT_ANDROID_PACKAGE_SOURCE_DIR.
    get_target_property(package_res_outputs ${target} _qt_android_package_res_outputs)
    if(package_res_outputs AND NOT package_res_outputs STREQUAL "package_res_outputs-NOTFOUND")
        list(REMOVE_ITEM dst_res_files ${package_res_outputs})
    endif()

    add_custom_command(OUTPUT ${dst_res_files}
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${template_res_dir}" "${res_dir_dst}"
        DEPENDS
            "${template_res_dir}"
            ${template_res_files}
        COMMENT "Copying Android app res templates for ${target}"
        VERBATIM
    )

    add_custom_target(${target}_copy_android_res_files DEPENDS ${dst_res_files})
endfunction()

# Copies the libc++ shared runtime to a build directory.
function(_qt_internal_android_copy_stdlib target deployment_dir)
    get_target_property(no_deploy_qt_libs ${target} QT_ANDROID_NO_DEPLOY_QT_LIBS)
    if(no_deploy_qt_libs)
        return()
    endif()

    set(stdlib_triple_by_abi_arm64_v8a "aarch64-linux-android")
    set(stdlib_triple_by_abi_armeabi_v7a "arm-linux-androideabi")
    set(stdlib_triple_by_abi_x86 "i686-linux-android")
    set(stdlib_triple_by_abi_x86_64 "x86_64-linux-android")
    string(REPLACE "-" "_" cmake_abi "${CMAKE_ANDROID_ARCH_ABI}")
    set(stdlib_triple "${stdlib_triple_by_abi_${cmake_abi}}")
    if(NOT stdlib_triple)
        message(FATAL_ERROR "Unsupported ABI '${CMAKE_ANDROID_ARCH_ABI}' for libc++_shared.")
    endif()

    set(stdlib_src "${CMAKE_SYSROOT}/usr/lib/${stdlib_triple}/libc++_shared.so")
    if(NOT EXISTS "${stdlib_src}")
        message(FATAL_ERROR
            "The libc++ runtime library was not found at '${stdlib_src}'. "
            "Check your Android NDK installation.")
    endif()

    set(stdlib_dst_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    set(stdlib_dst "${stdlib_dst_dir}/libc++_shared.so")
    _qt_internal_android_append_to_libs_xml_section(${target} qt_libs "${stdlib_dst}")

    _qt_internal_android_get_deploy_command(deploy_stdlib_cmd "${stdlib_src}" "${stdlib_dst}")
    add_custom_command(OUTPUT "${stdlib_dst}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${stdlib_dst_dir}"
        COMMAND ${deploy_stdlib_cmd}
        DEPENDS "${stdlib_src}"
        COMMENT "Copying libc++_shared for ${target}"
        VERBATIM
    )

    add_custom_target(${target}_copy_stdlib DEPENDS "${stdlib_dst}")
endfunction()

# Copies additional native libraries requested via QT_ANDROID_EXTRA_LIBS.
function(_qt_internal_android_copy_extra_libs target deployment_dir)
    get_target_property(no_deploy_qt_libs ${target} QT_ANDROID_NO_DEPLOY_QT_LIBS)
    if(no_deploy_qt_libs)
        return()
    endif()

    if(TARGET ${target}_copy_extra_libs)
        return()
    endif()

    get_target_property(extra_libs ${target} QT_ANDROID_EXTRA_LIBS)
    if(NOT extra_libs)
        add_custom_target(${target}_copy_extra_libs)
        return()
    endif()

    set(extra_libs_dst_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    set(copy_outputs "")
    set(copy_depends "")
    set(copy_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${extra_libs_dst_dir}"
    )

    foreach(lib IN LISTS extra_libs)
        if(lib MATCHES "^\\$<")
            message(FATAL_ERROR
                "QT_ANDROID_EXTRA_LIBS entry '${lib}' for ${target} is a generator expression. "
                "This copy implementation uses add_custom_command(OUTPUT) and requires absolute "
                "paths so outputs can be tracked reliably.")
        endif()

        file(TO_CMAKE_PATH "${lib}" lib_path)
        if(NOT IS_ABSOLUTE "${lib_path}")
            message(FATAL_ERROR
                "QT_ANDROID_EXTRA_LIBS entry '${lib}' for ${target} must be an absolute path.")
        endif()

        get_filename_component(lib_name "${lib_path}" NAME)
        if(NOT lib_name MATCHES "\\.so$")
            message(FATAL_ERROR
                "QT_ANDROID_EXTRA_LIBS entry '${lib}' for ${target} must be a shared library.")
        endif()

        _qt_internal_android_append_to_libs_xml_section(${target} extra_libs "${lib_path}")
        set(dst "${extra_libs_dst_dir}/${lib_name}")
        list(APPEND copy_outputs "${dst}")
        list(APPEND copy_depends "${lib_path}")
        list(APPEND copy_commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${lib_path}" "${dst}"
        )
    endforeach()

    add_custom_command(
        OUTPUT ${copy_outputs}
        ${copy_commands}
        DEPENDS ${copy_depends}
        COMMENT "Copying extra native libraries for ${target}"
        VERBATIM
    )

    add_custom_target(${target}_copy_extra_libs DEPENDS ${copy_outputs})
endfunction()

# Copies additional plugins requested via QT_ANDROID_EXTRA_PLUGINS.
function(_qt_internal_android_copy_extra_plugins target deployment_dir)
    get_target_property(no_deploy_qt_libs ${target} QT_ANDROID_NO_DEPLOY_QT_LIBS)
    if(no_deploy_qt_libs)
        return()
    endif()

    if(TARGET ${target}_copy_extra_plugins)
        return()
    endif()

    get_target_property(extra_plugins ${target} QT_ANDROID_EXTRA_PLUGINS)
    if(NOT extra_plugins)
        add_custom_target(${target}_copy_extra_plugins)
        return()
    endif()

    set(extra_plugins_dst_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")

    _qt_internal_collect_buildsystem_targets(buildsystem_library_targets
        "${CMAKE_SOURCE_DIR}" INCLUDE SHARED_LIBRARY MODULE_LIBRARY)

    set(extra_plugin_targets "")
    foreach(plugin_dir IN LISTS extra_plugins)
        if(NOT plugin_dir)
            continue()
        endif()

        set(plugin_path "${plugin_dir}")
        if(NOT "${plugin_dir}" MATCHES "^[$]<")
            file(TO_CMAKE_PATH "${plugin_dir}" plugin_path)
            if(NOT IS_ABSOLUTE "${plugin_path}")
                message(WARNING "QT_ANDROID_EXTRA_PLUGINS entry '${plugin_dir}' for ${target} "
                                "is not an absolute path.")
            endif()
        endif()

        list(APPEND copy_commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${plugin_path}" "${extra_plugins_dst_dir}"
        )

        # Extra plugins might not have direct dependency to ${target}, so find build system
        # targets in the project that are needed for the extra plugins outputs.

        # 1. Extract the target name from $<TARGET_FILE_DIR:name> genexes.
        if("${plugin_dir}" MATCHES "^\\$<TARGET_FILE_DIR:([^>]+)>$")
            if(TARGET "${CMAKE_MATCH_1}")
                list(APPEND extra_plugin_targets "${CMAKE_MATCH_1}")
            else()
                message(WARNING "QT_ANDROID_EXTRA_PLUGINS entry '${plugin_dir}' for ${target} "
                                "skipped invalid auto-discovered dependency ${CMAKE_MATCH_1}")
            endif()
            continue()
        elseif("${plugin_dir}" MATCHES "^[$]<")
            message(WARNING "QT_ANDROID_EXTRA_PLUGINS entry '${plugin_dir}' for ${target}"
                            "skipped dependencies auto-discovery due to unsupported genex.")
            continue()
        endif()

        # 2. Find targets with LIBRARY_OUTPUT_DIRECTORY resolving to extra plugin directory.
        get_filename_component(plugin_path_abs "${plugin_path}" ABSOLUTE)
        foreach(library_target IN LISTS buildsystem_library_targets)
            get_target_property(library_outdir "${library_target}" LIBRARY_OUTPUT_DIRECTORY)
            if(NOT library_outdir)
                continue()
            endif()
            if(NOT IS_ABSOLUTE "${library_outdir}")
                get_target_property(library_binary_dir "${library_target}" BINARY_DIR)
                set(library_outdir "${library_binary_dir}/${library_outdir}")
            endif()
            get_filename_component(library_outdir "${library_outdir}" ABSOLUTE)
            if(library_outdir STREQUAL plugin_path_abs)
                list(APPEND extra_plugin_targets "${library_target}")
            endif()
        endforeach()
    endforeach()

    list(REMOVE_DUPLICATES extra_plugin_targets)

    add_custom_target(${target}_copy_extra_plugins
        ${copy_commands}
        DEPENDS ${target} ${extra_plugin_targets}
        COMMENT "Copying extra plugins for ${target}"
        VERBATIM
    )
endfunction()

function(_qt_internal_android_get_plugin_output_filename target out_filename)
    get_target_property(name "${target}" LIBRARY_OUTPUT_NAME)
    if(NOT name)
        get_target_property(name "${target}" OUTPUT_NAME)
    endif()
    if(name)
        set(${out_filename} "lib${name}_${CMAKE_ANDROID_ARCH_ABI}.so" PARENT_SCOPE)
        return()
    endif()
    get_target_property(imported_location "${target}" IMPORTED_LOCATION)
    if(imported_location)
        get_filename_component(filename "${imported_location}" NAME)
        set(${out_filename} "${filename}" PARENT_SCOPE)
    else()
        set(${out_filename} "" PARENT_SCOPE)
    endif()
endfunction()

# Copy the QML plugin targets discovered by qmlimportscanner.
function(_qt_internal_android_copy_qml_plugins target deployment_dir)
    get_target_property(no_deploy_qt_libs ${target} QT_ANDROID_NO_DEPLOY_QT_LIBS)
    if(no_deploy_qt_libs)
        return()
    endif()

    if(TARGET ${target}_copy_qml_plugins)
        return()
    endif()

    set(libs_abi_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    set(copy_commands "")
    set(copy_depends "")

    get_target_property(plugin_targets ${target} _qt_android_qml_plugins)
    set(seen_destinations "")
    foreach(plugin_target IN LISTS plugin_targets)
        if(NOT TARGET "${plugin_target}")
            continue()
        endif()
        _qt_internal_android_get_plugin_output_filename("${plugin_target}" plugin_filename)
        if(NOT plugin_filename)
            continue()
        endif()
        _qt_internal_path_join(plugin_dst "${libs_abi_dir}" "${plugin_filename}")
        if(plugin_dst IN_LIST seen_destinations)
            continue()
        endif()
        list(APPEND seen_destinations "${plugin_dst}")

        _qt_internal_android_append_to_libs_xml_section(${target} qt_libs "${plugin_filename}")
        _qt_internal_android_get_deploy_command(deploy_cmd
            "$<TARGET_FILE:${plugin_target}>" "${plugin_dst}")
        list(APPEND copy_commands COMMAND ${deploy_cmd})
        list(APPEND copy_depends "${plugin_target}")
    endforeach()

    if(NOT copy_commands)
        add_custom_target(${target}_copy_qml_plugins)
        return()
    endif()

    add_custom_target(${target}_copy_qml_plugins
        ${copy_commands}
        DEPENDS ${copy_depends}
        COMMENT "Copying QML plugin targets for ${target}"
        VERBATIM
    )
endfunction()

# Copies non-Qt shared libraries linked to the target.
function(_qt_internal_android_copy_non_qt_linked_libs target deployment_dir)
    if(QT_NO_COLLECT_BUILD_TREE_APK_DEPS)
        add_custom_target(${target}_copy_non_qt_linked_libs)
        return()
    endif()

    set(queue "${target}")
    set(processed "")
    set(linked_libs "")

    # Queue project plugin targets so that non-Qt libs they link against are deployed.
    get_target_property(plugin_targets ${target} _qt_android_qml_plugins)
    if(plugin_targets)
        list(APPEND queue ${plugin_targets})
    endif()

    while(queue)
        list(POP_FRONT queue current_target)
        get_target_property(current_alias "${current_target}" ALIASED_TARGET)
        if(current_alias)
            set(current_target "${current_alias}")
        endif()

        if(current_target IN_LIST processed)
            continue()
        endif()
        list(APPEND processed "${current_target}")

        foreach(property_name IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(link_entries "${current_target}" ${property_name})
            if(NOT link_entries OR link_entries STREQUAL "${property_name}-NOTFOUND")
                continue()
            endif()

            foreach(raw_entry IN LISTS link_entries)
                _qt_internal_android_extract_link_target("${raw_entry}" entry)
                if(NOT entry OR NOT TARGET "${entry}")
                    continue()
                endif()

                if(NOT entry IN_LIST processed AND NOT entry IN_LIST queue)
                    list(APPEND queue "${entry}")
                endif()

                get_target_property(entry_type "${entry}" TYPE)
                if(NOT entry_type OR NOT entry_type MATCHES "^(SHARED|MODULE)_LIBRARY$")
                    continue()
                endif()

                get_target_property(entry_imported "${entry}" IMPORTED)
                if(entry_imported)
                    continue()
                endif()

                # Skip if it's a Qt library.
                get_target_property(entry_qt_package_version "${entry}" _qt_package_version)
                if(entry_qt_package_version)
                    continue()
                endif()

                list(APPEND linked_libs "${entry}")
            endforeach()
        endforeach()
    endwhile()

    list(REMOVE_DUPLICATES linked_libs)

    if(NOT linked_libs)
        add_custom_target(${target}_copy_non_qt_linked_libs)
        return()
    endif()

    set(libs_abi_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    set(linked_libs_copy_commands COMMAND ${CMAKE_COMMAND} -E make_directory "${libs_abi_dir}")
    foreach(lib IN LISTS linked_libs)
        list(APPEND linked_libs_copy_commands COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${lib}>" "${libs_abi_dir}/$<TARGET_FILE_NAME:${lib}>"
        )
    endforeach()

    add_custom_target(${target}_copy_non_qt_linked_libs
        ${linked_libs_copy_commands}
        DEPENDS ${linked_libs}
        COMMENT "Copying linked shared libraries for ${target}"
        VERBATIM
    )
endfunction()

# Copies (or symlinks) the app's binary file to the deployment dir.
function(_qt_internal_android_copy_app_binary target deployment_dir)
    set(target_file_dst
        "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}/$<TARGET_FILE_NAME:${target}>")

    _qt_internal_android_get_use_terminal_for_deployment(uses_terminal)

    _qt_internal_android_get_deploy_command(deploy_command
        "$<TARGET_FILE:${target}>" "${target_file_dst}")
    if(QT_ANDROID_CREATE_SYMLINKS_ONLY)
        set(deploy_comment "Symlinking ${target} binary to apk folder")
    else()
        set(deploy_comment "Copying ${target} binary to apk folder")
    endif()

    set(libs_abi_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    add_custom_target(${target}_copy_app_binary ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${libs_abi_dir}"
        COMMAND ${deploy_command}
        COMMENT "${deploy_comment}"
        VERBATIM
        ${uses_terminal}
    )
endfunction()

# Returns command to deploy a file or create a symlink to the deployment dir.
function(_qt_internal_android_get_deploy_command out_cmd src dst)
    if(QT_ANDROID_CREATE_SYMLINKS_ONLY)
        set(${out_cmd} ${CMAKE_COMMAND} -E create_symlink "${src}" "${dst}" PARENT_SCOPE)
    else()
        _qt_internal_copy_file_if_different_command(${out_cmd} "${src}" "${dst}")
        set(${out_cmd} "${${out_cmd}}" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_android_extract_link_target raw_entry out_entry)
    set(entry "${raw_entry}")

    if(entry MATCHES
        "^\\\$<(LINK_ONLY|TARGET_NAME_IF_EXISTS|BUILD_LOCAL_INTERFACE|BUILD_INTERFACE):([^>]+)>$")
        set(entry "${CMAKE_MATCH_2}")
    elseif(entry MATCHES "^\\\$<")
        string(REGEX MATCH "Qt[0-9]*::[A-Za-z0-9_]+" entry "${entry}")
    endif()

    set(${out_entry} "${entry}" PARENT_SCOPE)
endfunction()

# Collects Qt modules that 'target' and its plugins depends on, with dependencies
# listed before targets that depend on them.
function(_qt_internal_android_collect_qt_modules target out_qt_modules)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type_upper)
    set(pending_stack "${target}")
    set(expanded_targets "")
    set(collected "")

    # Collect Qt modules from QML plugin targets
    get_target_property(plugin_targets ${target} _qt_android_qml_plugins)
    if(plugin_targets)
        list(APPEND pending_stack ${plugin_targets})
    endif()

    while(pending_stack)
        list(GET pending_stack -1 current_target)
        if(current_target IN_LIST expanded_targets)
            list(POP_BACK pending_stack)
            get_target_property(target_type "${current_target}" TYPE)
            if(target_type AND target_type MATCHES "^(SHARED|MODULE)_LIBRARY$")
                set(qt_ns ${QT_CMAKE_EXPORT_NAMESPACE})
                if(current_target MATCHES "^${qt_ns}::" OR TARGET "${qt_ns}::${current_target}")
                    # Skip QML plugin targets, they are deployed by _copy_qml_plugins.
                    get_target_property(is_qml_plugin "${current_target}"
                        _qt_qml_module_is_plugin_target)
                    if(NOT is_qml_plugin)
                        list(APPEND collected "${current_target}")
                    endif()
                endif()
            endif()
            continue()
        endif()

        list(APPEND expanded_targets "${current_target}")

        set(direct_deps "")

        # For QML plugin targets, use _qt_qml_module_backing_target to find the Qt module.
        get_target_property(qml_backing "${current_target}" _qt_qml_module_backing_target)
        if(qml_backing)
            set(qml_backing_target "${QT_CMAKE_EXPORT_NAMESPACE}::${qml_backing}")
            if(TARGET "${qml_backing_target}")
                if(NOT "${qml_backing_target}" IN_LIST expanded_targets)
                    list(APPEND direct_deps "${qml_backing_target}")
                endif()
            else()
                message(WARNING
                    "The plugin ${current_target} depends on ${qml_backing_target}, "
                    "but it wasn't found. Add it using:\n"
                    "find_package(${QT_CMAKE_EXPORT_NAMESPACE} OPTIONAL_COMPONENTS ${qml_backing})")
            endif()
        endif()

        foreach(property_name IN ITEMS
                LINK_LIBRARIES
                INTERFACE_LINK_LIBRARIES
                IMPORTED_LINK_DEPENDENT_LIBRARIES
                "IMPORTED_LINK_DEPENDENT_LIBRARIES_${build_type_upper}")
            get_target_property(link_entries "${current_target}" ${property_name})
            if(NOT link_entries OR link_entries STREQUAL "${property_name}-NOTFOUND")
                continue()
            endif()

            foreach(raw_entry IN LISTS link_entries)
                _qt_internal_android_extract_link_target("${raw_entry}" entry)
                if(NOT entry OR NOT TARGET "${entry}")
                    continue()
                endif()

                get_target_property(entry_alias "${entry}" ALIASED_TARGET)
                if(entry_alias)
                    set(entry "${entry_alias}")
                endif()

                if(NOT entry IN_LIST expanded_targets)
                    list(APPEND direct_deps "${entry}")
                endif()
            endforeach()
        endforeach()

        list(REMOVE_DUPLICATES direct_deps)
        list(APPEND pending_stack ${direct_deps})
    endwhile()

    list(REMOVE_DUPLICATES collected)
    set(${out_qt_modules} "${collected}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_get_qt_paths out_qt_prefix out_qt_libs_dir out_qt_data_dir)
    _qt_internal_get_android_abi_prefix_path(qt_prefix ${CMAKE_ANDROID_ARCH_ABI})
    _qt_internal_get_android_abi_subdir_path(qt_libs_dir QT6_INSTALL_LIBS ${CMAKE_ANDROID_ARCH_ABI})
    _qt_internal_get_android_abi_subdir_path(qt_data_dir QT6_INSTALL_DATA ${CMAKE_ANDROID_ARCH_ABI})

    _qt_internal_path_join(qt_libs_dir "${qt_prefix}" "${qt_libs_dir}")
    _qt_internal_path_join(qt_data_dir "${qt_prefix}" "${qt_data_dir}")

    set(${out_qt_prefix} "${qt_prefix}" PARENT_SCOPE)
    set(${out_qt_libs_dir} "${qt_libs_dir}" PARENT_SCOPE)
    set(${out_qt_data_dir} "${qt_data_dir}" PARENT_SCOPE)
endfunction()



function(_qt_internal_android_get_clean_dependency dependency out_dependency)
    if(dependency MATCHES "^\$<.*>$")
        set(${out_dependency} "" PARENT_SCOPE)
        return()
    endif()

    string(REGEX MATCH "^[^:]*" cleaned_dependency "${dependency}")
    if(NOT cleaned_dependency)
        set(cleaned_dependency "${dependency}")
    endif()
    set(${out_dependency} "${cleaned_dependency}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_get_dependency_abs_path target dependency out_abs_path)
    if(dependency STREQUAL "")
        set(${out_abs_path} "" PARENT_SCOPE)
        return()
    endif()

    _qt_internal_android_get_qt_paths(qt_prefix qt_libs_dir qt_data_dir)

    if(IS_ABSOLUTE "${dependency}")
        set(abs_path "${dependency}")
    else()
        set(base_dir "${qt_prefix}")
        if(dependency MATCHES "^lib/")
            string(REGEX REPLACE "^lib/" "" dependency "${dependency}")
            set(base_dir "${qt_libs_dir}")
        elseif(dependency MATCHES "^jar/")
            set(base_dir "${qt_data_dir}")
        endif()
        _qt_internal_path_join(abs_path "${base_dir}" "${dependency}")
    endif()

    # Add abi suffix if file name doesn't have it.
    set(abi_so_suffix "_${CMAKE_ANDROID_ARCH_ABI}.so")
    if(NOT abs_path MATCHES "${abi_so_suffix}$")
        string(REGEX REPLACE "\\.so$" "${abi_so_suffix}" abs_path "${abs_path}")
    endif()

    if(NOT EXISTS "${abs_path}")
        set(${out_abs_path} "" PARENT_SCOPE)
        return()
    endif()

    set(${out_abs_path} "${abs_path}" PARENT_SCOPE)
endfunction()

# Processes JAR dependencies for a Qt module and appends copy_commands in-place
function(_qt_internal_android_process_module_jars target module libs_root_dir
        inout_seen_destinations inout_copy_commands inout_copy_depends)
    set(seen_destinations "${${inout_seen_destinations}}")
    set(copy_commands "${${inout_copy_commands}}")
    set(copy_depends "${${inout_copy_depends}}")

    get_target_property(module_jar_deps "${module}" QT_ANDROID_BUNDLED_JAR_DEPENDENCIES)
    if(NOT module_jar_deps)
        set(module_jar_deps "")
    endif()
    foreach(dep IN LISTS module_jar_deps)
        _qt_internal_android_get_clean_dependency("${dep}" jar_dep)
        _qt_internal_android_get_dependency_abs_path("${module}" "${jar_dep}" jar_absolute)
        if(NOT jar_absolute)
            message(WARNING "The JAR dependency '${jar_dep}' is missing for ${module}.")
            continue()
        endif()

        string(REGEX REPLACE "^jar/" "" jar_relative "${jar_dep}")
        _qt_internal_path_join(destination "${libs_root_dir}" "${jar_relative}")
        if(destination IN_LIST seen_destinations)
            continue()
        endif()

        list(APPEND seen_destinations "${destination}")
        _qt_internal_android_get_deploy_command(deploy_jar_cmd "${jar_absolute}" "${destination}")
        list(APPEND copy_commands COMMAND ${deploy_jar_cmd})
        list(APPEND copy_depends "${jar_absolute}")
    endforeach()

    set(${inout_seen_destinations} "${seen_destinations}" PARENT_SCOPE)
    set(${inout_copy_commands} "${copy_commands}" PARENT_SCOPE)
    set(${inout_copy_depends} "${copy_depends}" PARENT_SCOPE)
endfunction()

# Deploys a module's shared library
function(_qt_internal_android_process_module_self target module libs_abi_dir
        inout_seen_destinations inout_copy_commands inout_copy_depends)
    set(seen_destinations "${${inout_seen_destinations}}")
    set(copy_commands "${${inout_copy_commands}}")
    set(copy_depends "${${inout_copy_depends}}")

    set(module_dst "${libs_abi_dir}/$<TARGET_FILE_NAME:${module}>")
    if(NOT module_dst IN_LIST seen_destinations)
        list(APPEND seen_destinations "${module_dst}")

        get_target_property(module_interface_name "${module}" _qt_module_interface_name)
        if(module_interface_name)
            set(module_lib_name "${QT_CMAKE_EXPORT_NAMESPACE}${module_interface_name}")
        else()
            set(module_lib_name "${module}")
        endif()
        set(module_lib_name "${module_lib_name}_${CMAKE_ANDROID_ARCH_ABI}")
        _qt_internal_android_append_to_libs_xml_section(${target} qt_libs "${module_lib_name}")
        _qt_internal_android_get_deploy_command(deploy_module_cmd
            "$<TARGET_FILE:${module}>" "${module_dst}")
        list(APPEND copy_commands COMMAND ${deploy_module_cmd})
        list(APPEND copy_depends "${module}")
    endif()

    set(${inout_seen_destinations} "${seen_destinations}" PARENT_SCOPE)
    set(${inout_copy_commands} "${copy_commands}" PARENT_SCOPE)
    set(${inout_copy_depends} "${copy_depends}" PARENT_SCOPE)
endfunction()

# Deploys QT_ANDROID_LIB_DEPENDENCIES for a module
function(_qt_internal_android_process_module_lib_deps target module libs_abi_dir
        inout_seen_destinations inout_copy_commands inout_copy_depends)
    set(seen_destinations "${${inout_seen_destinations}}")
    set(copy_commands "${${inout_copy_commands}}")
    set(copy_depends "${${inout_copy_depends}}")

    get_target_property(module_lib_deps "${module}" QT_ANDROID_LIB_DEPENDENCIES)
    if(NOT module_lib_deps)
        set(module_lib_deps "")
    endif()
    foreach(lib IN LISTS module_lib_deps)
        _qt_internal_android_get_clean_dependency("${lib}" lib_dep)
        _qt_internal_android_get_dependency_abs_path("${module}" "${lib_dep}" lib_absolute)
        if(NOT lib_absolute)
            message(WARNING "The library dependency '${lib_dep}' is missing for ${module}.")
            continue()
        endif()

        get_filename_component(filename "${lib_absolute}" NAME)
        # Add lib files to libs.xml so that they are explicitly loaded and JNI_OnLoad() called
        _qt_internal_android_append_to_libs_xml_section(${target} local_libs "${filename}")

        _qt_internal_path_join(destination "${libs_abi_dir}" "${filename}")
        if(destination IN_LIST seen_destinations)
            continue()
        endif()

        list(APPEND seen_destinations "${destination}")
        _qt_internal_android_get_deploy_command(deploy_lib_cmd "${lib_absolute}" "${destination}")
        list(APPEND copy_commands COMMAND ${deploy_lib_cmd})
        list(APPEND copy_depends "${lib_absolute}")
    endforeach()

    set(${inout_seen_destinations} "${seen_destinations}" PARENT_SCOPE)
    set(${inout_copy_commands} "${copy_commands}" PARENT_SCOPE)
    set(${inout_copy_depends} "${copy_depends}" PARENT_SCOPE)
endfunction()

# Resolves plugin paths for a module using the _qt_plugins property.
function(_qt_internal_android_list_module_plugins module out_plugin_paths)
    set(result "")

    string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type_upper)
    get_target_property(qt_plugins "${module}" _qt_plugins)
    if (NOT qt_plugins)
        set(qt_plugins "")
    endif()
    foreach(qt_plugin IN LISTS qt_plugins)
        set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_plugin}")
        if(NOT TARGET "${plugin_target}")
            message(WARNING
                "Skipping the deployment scanning of ${plugin_target} plugin "
                "for ${module} module because it's an invalid target.")
            continue()
        endif()
        get_target_property(plugin_path "${plugin_target}" "IMPORTED_LOCATION_${build_type_upper}")
        if(NOT plugin_path)
            get_target_property(plugin_path "${plugin_target}" IMPORTED_LOCATION)
        endif()
        if(EXISTS "${plugin_path}")
            list(APPEND result "${plugin_target}")
        else()
            message(WARNING
                "The ${plugin_target} plugin expected to exist at ${plugin_path}, but it doesn't.")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES result)
    set(${out_plugin_paths} "${result}" PARENT_SCOPE)
endfunction()

# Deploys a single plugin, accepts either a CMake target name or a file path.
function(_qt_internal_android_process_plugin_item target plugin_target_or_path libs_abi_dir
        inout_seen_destinations inout_copy_commands inout_copy_depends)
    set(seen_destinations "${${inout_seen_destinations}}")
    set(copy_commands "${${inout_copy_commands}}")
    set(copy_depends "${${inout_copy_depends}}")

    if(TARGET "${plugin_target_or_path}")
        string(TOUPPER "${CMAKE_BUILD_TYPE}" _build_type_upper)
        get_target_property(plugin_path "${plugin_target_or_path}"
            "IMPORTED_LOCATION_${_build_type_upper}")
        if(NOT plugin_path)
            get_target_property(plugin_path "${plugin_target_or_path}" IMPORTED_LOCATION)
        endif()
    else()
        set(plugin_path "${plugin_target_or_path}")
    endif()

    if(NOT plugin_path)
        return()
    endif()

    get_filename_component(plugin_name "${plugin_path}" NAME)
    _qt_internal_path_join(plugin_dst "${libs_abi_dir}" "${plugin_name}")
    if(NOT plugin_dst IN_LIST seen_destinations)
        list(APPEND seen_destinations "${plugin_dst}")
        if(NOT plugin_name MATCHES "^libplugins_")
            _qt_internal_android_append_to_libs_xml_section(${target} qt_libs "${plugin_name}")
        endif()
        _qt_internal_android_get_deploy_command(deploy_plugin_cmd "${plugin_path}" "${plugin_dst}")
        list(APPEND copy_commands COMMAND ${deploy_plugin_cmd})
        list(APPEND copy_depends "${plugin_path}")
    endif()

    set(${inout_seen_destinations} "${seen_destinations}" PARENT_SCOPE)
    set(${inout_copy_commands} "${copy_commands}" PARENT_SCOPE)
    set(${inout_copy_depends} "${copy_depends}" PARENT_SCOPE)
endfunction()

# Returns the shared Qt module targets declared as dependencies of a plugin via
# _qt_plugin_qt_module_dependencies, filtered to shared libraries only.
function(_qt_internal_android_list_plugin_modules plugin_target out_module_deps)
    set(result "")
    get_target_property(plugin_module_deps "${plugin_target}"
        _qt_plugin_qt_module_dependencies)
    foreach(dep_target IN LISTS plugin_module_deps)
        if(NOT TARGET "${dep_target}")
            continue()
        endif()
        get_target_property(type "${dep_target}" TYPE)
        if(type STREQUAL "SHARED_LIBRARY")
            list(APPEND result "${dep_target}")
        endif()
    endforeach()
    set(${out_module_deps} "${result}" PARENT_SCOPE)
endfunction()

# Copy Qt dependencies (libs, jars, plugins, other files, assets) to the deployment dir.
# Queue both Qt modules and QML plugins, then scan for their dependencies and add
# them to queue until we drain both module and plugin queues.
function(_qt_internal_android_copy_qt_dependencies target deployment_dir)
    set(copy_commands "")
    set(copy_depends "")
    set(seen_destinations "")

    set(libs_root_dir "${deployment_dir}/libs")
    set(libs_abi_dir "${libs_root_dir}/${CMAKE_ANDROID_ARCH_ABI}")
    set(assets_bundle_dir "${deployment_dir}/assets/android_rcc_bundle")
    list(APPEND copy_commands COMMAND ${CMAKE_COMMAND} -E make_directory "${libs_abi_dir}")
    list(APPEND copy_commands COMMAND ${CMAKE_COMMAND} -E make_directory "${assets_bundle_dir}")

    get_target_property(no_deploy_qt_libs ${target} QT_ANDROID_NO_DEPLOY_QT_LIBS)

    # Start with CMake links of this target
    _qt_internal_android_collect_qt_modules(${target} module_queue)

    set(plugin_queue "")
    set(processed_modules "")
    set(processed_plugins "")

    while(module_queue OR plugin_queue)
        while(module_queue)
            list(POP_FRONT module_queue module)
            if(module IN_LIST processed_modules)
                continue()
            endif()
            list(APPEND processed_modules "${module}")

            _qt_internal_android_process_module_jars("${target}" "${module}" "${libs_root_dir}"
                seen_destinations copy_commands copy_depends)

            if(no_deploy_qt_libs)
                continue()
            endif()

            _qt_internal_android_process_module_self("${target}" "${module}"
                "${libs_abi_dir}"
                seen_destinations copy_commands copy_depends)

            _qt_internal_android_process_module_lib_deps("${target}" "${module}" "${libs_abi_dir}"
                seen_destinations copy_commands copy_depends)

            _qt_internal_android_list_module_plugins("${module}" new_plugins)
            list(APPEND plugin_queue ${new_plugins})
        endwhile()

        while(plugin_queue)
            list(POP_FRONT plugin_queue plugin_item)
            if(plugin_item IN_LIST processed_plugins)
                continue()
            endif()
            list(APPEND processed_plugins "${plugin_item}")

            if(TARGET "${plugin_item}")
                # Collect extra Qt module deps declared by this plugin and feed them back into
                # module_queue; the outer loop re-enters while(module_queue) to process them.
                _qt_internal_android_list_plugin_modules("${plugin_item}" plugin_module_deps)
                foreach(dep_target IN LISTS plugin_module_deps)
                    if(NOT "${dep_target}" IN_LIST processed_modules
                            AND NOT "${dep_target}" IN_LIST module_queue)
                        list(APPEND module_queue "${dep_target}")
                    endif()
                endforeach()
            endif()

            _qt_internal_android_process_plugin_item("${target}" "${plugin_item}" "${libs_abi_dir}"
                seen_destinations copy_commands copy_depends)
        endwhile()
    endwhile()

    list(REMOVE_DUPLICATES copy_depends)
    list(FILTER copy_depends EXCLUDE REGEX "^$")

    if(copy_commands)
        add_custom_target(${target}_copy_qt_files
            ${copy_commands}
            DEPENDS ${copy_depends}
            COMMENT "Copying Qt deployment files for ${target}"
            VERBATIM
        )
    else()
        add_custom_target(${target}_copy_qt_files)
    endif()
endfunction()

# Appends a unique lib name for the target to the libs.xml section.
function(_qt_internal_android_append_to_libs_xml_section target section_name lib_name)
    if(NOT lib_name)
        return()
    endif()

    set(normalized_lib_name "${lib_name}")
    if(NOT section_name STREQUAL "local_libs")
        get_filename_component(file_name "${normalized_lib_name}" NAME)
        if(file_name MATCHES "^lib(.+)\.so$")
            set(normalized_lib_name "${CMAKE_MATCH_1}")
        else()
            string(REGEX REPLACE "\.so$" "" normalized_lib_name "${file_name}")
        endif()
    endif()

    set(full_section_name "_android_libs_xml_${section_name}")
    get_target_property(current_libs ${target} ${full_section_name})
    if(NOT current_libs OR current_libs STREQUAL "${full_section_name}-NOTFOUND")
        set(current_libs "")
    endif()

    list(FIND current_libs "${normalized_lib_name}" _qt_append_index)
    if(_qt_append_index EQUAL -1)
        list(APPEND current_libs "${normalized_lib_name}")
        set_target_properties(${target} PROPERTIES ${full_section_name} "${current_libs}")
    endif()
endfunction()

# Collects the target's libs.xml section and returns the XML string.
function(_qt_internal_android_assemble_libs_xml_section target section_name out_section)
    set(property_name "_android_libs_xml_${section_name}")
    get_target_property(libs ${target} ${property_name})
    if(NOT libs OR libs STREQUAL "${property_name}-NOTFOUND")
        set(libs "")
    endif()

    list(REMOVE_ITEM libs "")
    list(REMOVE_DUPLICATES libs)
    set(entries "")
    foreach(lib IN LISTS libs)
        string(APPEND entries "
        <item>${abi};${lib}</item>")
    endforeach()
    string(REGEX REPLACE "^
        " "" entries "${entries}") # fix indentation of first item

    set(${out_section} "${entries}" PARENT_SCOPE)
endfunction()

# Generate a populated libs.xml file.
function(_qt_internal_android_generate_libs_xml target deployment_dir)
    set(abi "${CMAKE_ANDROID_ARCH_ABI}")
    _qt_internal_android_assemble_libs_xml_section(${target} "qt_libs" qt_libs)
    _qt_internal_android_assemble_libs_xml_section(${target} "local_libs" local_libs)
    _qt_internal_android_assemble_libs_xml_section(${target} "extra_libs" extra_libs)

    _qt_internal_android_template_res_dir(template_res_dir)
    set(libs_xml_template "${template_res_dir}/values/libs.xml")
    if(NOT EXISTS "${libs_xml_template}")
        message(FATAL_ERROR "Android libs.xml template file not found at '${libs_xml_template}'.")
    endif()
    file(READ "${libs_xml_template}" content)

    string(REPLACE "<!-- %%INSERT_QT_LIBS%% -->" "${qt_libs}" content "${content}")
    string(REPLACE "<!-- %%INSERT_LOCAL_LIBS%% -->" "${local_libs}" content "${content}")
    string(REPLACE "<!-- %%INSERT_EXTRA_LIBS%% -->" "${extra_libs}" content "${content}")
    string(REPLACE "<!-- %%USE_LOCAL_QT_LIBS%% -->" "1" content "${content}")
    string(REPLACE "<!-- %%BUNDLE_LOCAL_QT_LIBS%% -->" "1" content "${content}")
    string(REPLACE "<!-- %%SYSTEM_LIBS_PREFIX%% -->" "" content "${content}")

    set(libs_xml_dst "${deployment_dir}/res/values/libs.xml")
    set(libs_xml_dst_tmp "${deployment_dir}/res/values/libs.xml.tmp")

    string(REPLACE "::" "_" sanitized_target "${target}")
    set(cmake_files_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles")
    set(libs_xml_update_script "${cmake_files_dir}/${sanitized_target}_update_libs_xml.cmake")
    file(GENERATE OUTPUT "${libs_xml_update_script}" CONTENT
"file(MAKE_DIRECTORY \"${deployment_dir}/res/values\")
file(WRITE \"${libs_xml_dst}\" [=[${content}]=])
")

    add_custom_target(${target}_update_libs_xml
        COMMAND ${CMAKE_COMMAND} -P "${libs_xml_update_script}"
        DEPENDS
            ${target}_copy_android_res_files
        COMMENT "Updating libs.xml for ${target}"
        VERBATIM
    )

    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${libs_xml_dst}")
endfunction()

function(_qt_internal_android_find_subdir_parent child parents out_parent)
    if(NOT child OR NOT parents)
        set(${out_parent} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(child_abs "${child}" ABSOLUTE)
    set(found "")
    foreach(parent IN LISTS parents)
        if(NOT parent)
            continue()
        endif()
        get_filename_component(parent_abs "${parent}" ABSOLUTE)
        file(RELATIVE_PATH relative_path "${parent_abs}" "${child_abs}")
        if(NOT relative_path MATCHES "^\\.\\.")
            set(found "${parent}")
            break()
        endif()
    endforeach()
    set(${out_parent} "${found}" PARENT_SCOPE)
endfunction()


function(_qt_internal_android_library_base_name path out_base_name)
    get_filename_component(file_name "${path}" NAME)
    string(REGEX REPLACE "^lib(.+)\\.so$" "\\1" trimmed "${file_name}")
    string(REGEX REPLACE "\\.so$" "" trimmed "${trimmed}")
    set(${out_base_name} "${trimmed}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_resolve_absolute_paths paths out_paths)
    set(abs_paths "")
    foreach(path IN LISTS paths)
        if(path)
            get_filename_component(abs_path "${path}" ABSOLUTE)
            if(EXISTS "${abs_path}")
                list(APPEND abs_paths "${abs_path}")
            endif()
        endif()
    endforeach()
    set(${out_paths} "${abs_paths}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_get_absolute_qml_paths target out_import_paths out_root_paths)
    get_target_property(qml_import_paths ${target} _qt_native_qml_import_paths)
    if(qml_import_paths STREQUAL "_qt_native_qml_import_paths-NOTFOUND")
        set(qml_import_paths "")
    endif()

    get_target_property(qml_root_paths ${target} _qt_android_native_qml_root_paths)
    if(qml_root_paths STREQUAL "_qt_android_native_qml_root_paths-NOTFOUND")
        set(qml_root_paths "")
    endif()

    _qt_internal_android_resolve_absolute_paths("${qml_import_paths}" qml_import_paths_abs)
    _qt_internal_android_resolve_absolute_paths("${qml_root_paths}" qml_root_paths_abs)

    _qt_internal_get_android_abi_prefix_path(qt_prefix ${CMAKE_ANDROID_ARCH_ABI})
    if(DEFINED QT6_INSTALL_QML)
        set(qt_qml_dir "${qt_prefix}/${QT6_INSTALL_QML}")
    else()
        set(qt_qml_dir "${qt_prefix}/qml")
    endif()
    if(EXISTS "${qt_qml_dir}")
        list(APPEND qml_import_paths_abs "${qt_qml_dir}")
    endif()

    foreach(root_path IN LISTS qml_root_paths_abs)
        list(APPEND qml_import_paths_abs "${root_path}")
    endforeach()
    list(REMOVE_DUPLICATES qml_import_paths_abs)

    set(${out_import_paths} "${qml_import_paths_abs}" PARENT_SCOPE)
    set(${out_root_paths} "${qml_root_paths_abs}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_get_qmlimportscanner_scan
    target qmlimportscanner_path qml_root_paths qml_import_paths out_scan_output)
    if(NOT qmlimportscanner_path)
        set(${out_scan_output} "" PARENT_SCOPE)
        return()
    endif()

    set(qmlimportscanner_command "${qmlimportscanner_path}")
    foreach(root_path IN LISTS qml_root_paths)
        list(APPEND qmlimportscanner_command "-rootPath" "${root_path}")
    endforeach()
    foreach(import_path IN LISTS qml_import_paths)
        list(APPEND qmlimportscanner_command "-importPath" "${import_path}")
    endforeach()

    execute_process(
        COMMAND ${qmlimportscanner_command}
        RESULT_VARIABLE qml_scan_result
        OUTPUT_VARIABLE qml_scan_output
        ERROR_VARIABLE qml_scan_error
    )
    if(NOT qml_scan_result EQUAL 0)
        message(WARNING "qmlimportscanner failed for ${target}: ${qml_scan_error}")
        set(${out_scan_output} "" PARENT_SCOPE)
        return()
    endif()

    string(STRIP "${qml_scan_output}" qml_scan_output)
    set(${out_scan_output} "${qml_scan_output}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_json_get_string json index key out_value)
    string(JSON value_type ERROR_VARIABLE type_error TYPE "${json}" ${index} ${key})
    if(type_error OR NOT value_type STREQUAL "STRING")
        set(${out_value} "" PARENT_SCOPE)
        return()
    endif()
    string(JSON value ERROR_VARIABLE value_error GET "${json}" ${index} ${key})
    if(value_error OR value STREQUAL "")
        set(${out_value} "" PARENT_SCOPE)
        return()
    endif()

    set(${out_value} "${value}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_json_get_bool json index key out_value)
    string(JSON value_type ERROR_VARIABLE type_error TYPE "${json}" ${index} ${key})
    if(type_error OR NOT value_type STREQUAL "BOOL")
        set(${out_value} FALSE PARENT_SCOPE)
        return()
    endif()
    string(JSON value ERROR_VARIABLE value_error GET "${json}" ${index} ${key})
    if(value_error)
        set(${out_value} FALSE PARENT_SCOPE)
        return()
    endif()
    string(TOLOWER "${value}" value_lower)
    if(value_lower STREQUAL "true")
        set(${out_value} TRUE PARENT_SCOPE)
    else()
        set(${out_value} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_android_parse_qmlimportscanner_output
        target qml_scan qml_import_paths qml_root_paths out_qml_modules out_qml_plugins)
    set(qml_modules "")
    set(qml_plugins "")

    if(qml_scan STREQUAL "")
        set(${out_qml_modules} "" PARENT_SCOPE)
        set(${out_qml_plugins} "" PARENT_SCOPE)
        return()
    endif()

    string(JSON module_count LENGTH "${qml_scan}")
    if(NOT module_count GREATER 0)
        set(${out_qml_modules} "" PARENT_SCOPE)
        set(${out_qml_plugins} "" PARENT_SCOPE)
        return()
    endif()

    math(EXPR module_last_index "${module_count} - 1")
    foreach(mod_index RANGE 0 ${module_last_index})
        _qt_internal_android_json_get_string("${qml_scan}" ${mod_index} path module_path)
        if(NOT module_path)
            continue()
        endif()
        get_filename_component(module_abs "${module_path}" ABSOLUTE)

        _qt_internal_android_find_subdir_parent("${module_abs}" "${qml_root_paths}" skip_root)
        if(skip_root)
            continue()
        endif()

        _qt_internal_android_find_subdir_parent("${module_abs}" "${qml_import_paths}" import_root)
        if(NOT import_root)
            continue()
        endif()

        file(RELATIVE_PATH module_relative "${import_root}" "${module_abs}")
        file(TO_CMAKE_PATH "${module_relative}" module_relative)

        set(skip_module FALSE)
        _qt_internal_android_json_get_string("${qml_scan}" ${mod_index} plugin plugin_name)
        if(plugin_name)
            _qt_internal_android_json_get_string("${qml_scan}" ${mod_index} linkTarget link_target)
            if(TARGET "${link_target}")
                get_property(plugin_target TARGET "${link_target}"
                    PROPERTY _qt_qml_module_installed_plugin_target)
                if(NOT plugin_target)
                    get_property(plugin_target TARGET "${link_target}"
                        PROPERTY _qt_qml_module_plugin_target)
                endif()
                if(NOT plugin_target)
                    set(plugin_target "${link_target}")
                endif()
                # Don't pass STATIC targets to the Android loader.
                get_target_property(plugin_type "${plugin_target}" TYPE)
                if(NOT plugin_type MATCHES "^(SHARED|MODULE)_LIBRARY$")
                    continue()
                endif()
                list(APPEND qml_plugins "${plugin_target}")
            else()
                _qt_internal_android_json_get_bool("${qml_scan}" ${mod_index}
                    pluginIsOptional optional)
                if(NOT optional AND NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
                    message(WARNING "QML plugin '${plugin_name}' not found for ${target}.")
                    set(skip_module TRUE)
                endif()
            endif()
        endif()

        _qt_internal_android_json_get_string("${qml_scan}" ${mod_index} prefer module_prefer)

        if(NOT skip_module)
            list(APPEND qml_modules "${module_abs}::${module_relative}::${module_prefer}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES qml_plugins)

    set(${out_qml_modules} "${qml_modules}" PARENT_SCOPE)
    set(${out_qml_plugins} "${qml_plugins}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_copy_qml_modules_outputs
    target qml_bundle_dir qml_modules out_outputs)
    if(NOT qml_modules)
        set(${out_outputs} "" PARENT_SCOPE)
        return()
    endif()

    set(commands "")
    foreach(module_entry IN LISTS qml_modules)
        string(REPLACE "::" ";" module_parts "${module_entry}")
        list(GET module_parts 0 module_abs)
        list(GET module_parts 1 module_rel)
        list(GET module_parts 2 module_prefer)

        if(NOT module_abs OR NOT module_rel)
            continue()
        endif()

        set(destination_dir "${qml_bundle_dir}/qml/${module_rel}")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E make_directory "${destination_dir}")

        if(module_prefer MATCHES "^:/")
            # QML sources are compiled into the plugin's resources, so copy only the qmldir file.
            list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${module_abs}/qmldir" "${destination_dir}/qmldir")
        else()
            list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${module_abs}" "${destination_dir}")
        endif()
    endforeach()

    if(NOT commands)
        set(${out_outputs} "" PARENT_SCOPE)
        return()
    endif()

    add_custom_target(${target}_copy_qml_modules
        ${commands}
        COMMENT "Copying QML modules for ${target}"
        VERBATIM
    )

    set(${out_outputs} "${target}_copy_qml_modules" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_copy_qml_dependencies target deployment_dir)
    _qt_internal_android_get_absolute_qml_paths(${target} qml_import_paths qml_root_paths)
    if(NOT qml_import_paths OR NOT qml_root_paths)
        return()
    endif()

    __qt_internal_get_tool_imported_location(qmlimportscanner_path "qmlimportscanner")
    if(NOT qmlimportscanner_path)
        message(WARNING
            "qmlimportscanner not found. Skipping QML dependency scanning for ${target}.")
        return()
    endif()

    __qt_internal_get_tool_imported_location(rcc_path "rcc")
    if(NOT rcc_path)
        message(WARNING "rcc not found. Skipping QML dependency bundling for ${target}.")
        return()
    endif()

    _qt_internal_android_get_qmlimportscanner_scan(${target} "${qmlimportscanner_path}"
        "${qml_root_paths}" "${qml_import_paths}" qml_scan_output)
    if(qml_scan_output STREQUAL "")
        return()
    endif()

    _qt_internal_android_parse_qmlimportscanner_output(${target} "${qml_scan_output}"
        "${qml_import_paths}" "${qml_root_paths}" qml_modules qml_plugins)

    if(NOT qml_modules AND NOT qml_plugins)
        return()
    endif()

    if(qml_plugins)
        set_property(TARGET ${target} APPEND PROPERTY _qt_android_qml_plugins "${qml_plugins}")
    endif()

    set(qml_bundle_dir "${deployment_dir}/assets/android_rcc_bundle")
    _qt_internal_android_copy_qml_modules_outputs(${target} "${qml_bundle_dir}" "${qml_modules}"
        copy_qml_modules_outputs)

    if(copy_qml_modules_outputs)
        set(bundle_rcc "${deployment_dir}/assets/android_rcc_bundle.rcc")
        add_custom_command(OUTPUT "${bundle_rcc}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${qml_bundle_dir}"
            COMMAND ${CMAKE_COMMAND} -E chdir "${qml_bundle_dir}"
                "${rcc_path}" --project -o "${qml_bundle_dir}/android_rcc_bundle.qrc"
            COMMAND "${rcc_path}" --binary --root=/android_rcc_bundle/ -o "${bundle_rcc}"
                "${qml_bundle_dir}/android_rcc_bundle.qrc"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${qml_bundle_dir}"
            DEPENDS ${copy_qml_modules_outputs}
            COMMENT "Generating QML resource bundle for ${target}"
            VERBATIM
        )

        set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${bundle_rcc}")
        add_custom_target(${target}_build_qml_bundle DEPENDS "${bundle_rcc}")
        add_dependencies(${target}_build_qml_bundle "${copy_qml_modules_outputs}")
    endif()
endfunction()
