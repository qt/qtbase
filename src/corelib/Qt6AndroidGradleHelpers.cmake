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

        # Add user template with the higher priority
        list(PREPEND possible_paths "${user_template_directory}/${template_name}.in")
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
    set(indent "            ")
    foreach(type IN LISTS known_types)
        set(source_dirs
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},_qt_android_gradle_${type}_source_dirs>>")
        string(JOIN "" source_set
            "${source_set}"
            "$<$<BOOL:${source_dirs}>:"
                "${indent}${type}.srcDirs = ['$<JOIN:${source_dirs},'$<COMMA> '>']\n"
            ">"
        )
    endforeach()

    set(manifest
        "$<TARGET_PROPERTY:${target},_qt_android_manifest>")
    string(JOIN "" source_set
        "${source_set}"
        "$<$<BOOL:${manifest}>:"
            "${indent}manifest.srcFile '${manifest}'\n"
        ">"
    )
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
        set(dep_property "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},_qt_android_gradle_${dep_type}_dependencies>>")
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
    # Currently, it is hardcoded to 1.16.0.
    list(APPEND implementation_dependencies "'androidx.core:core:1.16.0'")

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
    _qt_internal_android_copy_target_package_sources(${target})

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
    if(NOT deployment_type_suffix STREQUAL "release" AND type STREQUAL "apk")
        set(extra_suffix "-unsigned")
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

    set(gradle_scripts "$<TARGET_PROPERTY:${target},_qt_android_deployment_files>")
    add_custom_command(OUTPUT "${package_file_path}"
        BYPRODUCTS "${package_build_file_path}"
        COMMAND
            "${gradlew}" ${android_deployment_type_option} ${extra_args}
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${package_build_file_path}" "${package_file_path}"
        DEPENDS
            ${target}
            ${gradle_scripts}
            ${target}_copy_gradle_files
            ${target}_android_deploy_aux
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

    # The file cannot be generated at cmake configure time, because androiddeployqt
    # will override it at build time. We use this trick with temporary file to override
    # it after the aux run of androiddeployqt.
    add_custom_command(OUTPUT "${out_file}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${temporary_file}"
            "${out_file}"
        DEPENDS
            "${template_file}"
            "${temporary_file}"
            ${target}_android_deploy_aux
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
function(_qt_internal_android_copy_target_package_sources target)
    _qt_internal_android_get_package_source_dir(package_source_dir ${target})

    if(NOT package_source_dir)
        return()
    endif()
    get_filename_component(package_source_dir "${package_source_dir}" ABSOLUTE)

    # Collect deployment files from use-defined package source directory
    file(GLOB_RECURSE package_files
        LIST_DIRECTORIES false
        RELATIVE "${package_source_dir}"
        "${package_source_dir}/*"
    )

    # Do not copy files that we treat as CMake templates, having '.in' extention.
    #
    # TODO: If it ever will be an issue we may exclude only templates that are
    # known by our build system.
    list(FILTER package_files EXCLUDE REGEX ".+\\.in$")

    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    list(TRANSFORM package_files PREPEND "${android_build_dir}/" OUTPUT_VARIABLE out_package_files)
    list(TRANSFORM package_files PREPEND "${package_source_dir}/" OUTPUT_VARIABLE in_package_files)

    if(in_package_files)
        # TODO: Add cmake < 3.26 support
        if(CMAKE_VERSION VERSION_LESS 3.26)
            message(FATAL_ERROR "The use of QT_ANDROID_PACKAGE_SOURCE_DIR property with
                the QT_USE_ANDROID_MODERN_BUNDLE option enabled requires CMake version >= 3.26.")
        endif()
        set(copy_commands COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different
            "${package_source_dir}" "${android_build_dir}")
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
