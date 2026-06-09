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
        "${template_directory}/${template_name}"
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
        list(PREPEND possible_paths
            "${user_template_directory}/${template_name}.in"
            "${user_template_directory}/${template_name}"
        )

        # When the user’s package source dir is already the module's subdir
        # (i.e app/ or dynamic_feature/), then don't add yet another app subdir.
        if(user_template_directory)
            get_filename_component(user_template_dir_basename "${user_template_directory}" NAME)
            _qt_internal_re_escape(user_template_dir_basename_re "${user_template_dir_basename}")
            if(template_name MATCHES "^${user_template_dir_basename_re}/(.+)$")
                set(trailing_name "${CMAKE_MATCH_1}")
                list(INSERT possible_paths 1 "${user_template_directory}/${trailing_name}.in")
                list(INSERT possible_paths 2 "${user_template_directory}/${trailing_name}")
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

# Reads template into out_var and adds it to CMAKE_CONFIGURE_DEPENDS for reconfigure-on-edit.
function(_qt_internal_android_read_template out_var template)
    file(READ "${template}" content)
    _qt_internal_append_cmake_configure_depends("${template}")
    set(${out_var} "${content}" PARENT_SCOPE)
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

    _qt_internal_android_read_template(settings_gradle_content "${template_file}")
    _qt_internal_configure_file(GENERATE OUTPUT ${settings_gradle_file}
        CONTENT "${settings_gradle_content}")
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
        "$<$<BOOL:${target_dynamic_features}>:libs.play.feature.delivery>"
    )
    # Default dependency versions are defined in the version catalog.
    list(APPEND implementation_dependencies "libs.androidx.core")

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

    _qt_internal_android_get_gradle_property(PACKAGE_NAME ${target}
        QT_ANDROID_PACKAGE_NAME "org.qtproject.example.$<MAKE_C_IDENTIFIER:${target}>")

    _qt_internal_android_get_target_sdk_build_tools_revision(ANDROID_BUILD_TOOLS_VERSION
        ${target})

    _qt_internal_detect_latest_android_platform(ANDROID_COMPILE_SDK_VERSION)
    if(NOT ANDROID_COMPILE_SDK_VERSION)
        message(FATAL_ERROR "Unable to detect the android platform in ${ANDROID_SDK_ROOT}. "
            "Please check your Android SDK installation.")
    endif()

    set(ANDROID_COMPILE_SDK_RELEASE "")
    set(ANDROID_COMPILE_SDK_MINOR "0")
    if(ANDROID_COMPILE_SDK_VERSION MATCHES "^android-([0-9]+)\\.([0-9]+)$")
        set(ANDROID_COMPILE_SDK_RELEASE "${CMAKE_MATCH_1}")
        set(ANDROID_COMPILE_SDK_MINOR "${CMAKE_MATCH_2}")
    elseif(ANDROID_COMPILE_SDK_VERSION MATCHES "^android-([0-9]+)$")
        set(ANDROID_COMPILE_SDK_RELEASE "${CMAKE_MATCH_1}")
    elseif(ANDROID_COMPILE_SDK_VERSION MATCHES "^([0-9]+)\\.([0-9]+)$")
        set(ANDROID_COMPILE_SDK_RELEASE "${CMAKE_MATCH_1}")
        set(ANDROID_COMPILE_SDK_MINOR "${CMAKE_MATCH_2}")
    elseif(ANDROID_COMPILE_SDK_VERSION MATCHES "^([0-9]+)$")
        set(ANDROID_COMPILE_SDK_RELEASE "${CMAKE_MATCH_1}")
    else()
        message(FATAL_ERROR "Unsupported android platform format '${ANDROID_COMPILE_SDK_VERSION}'.")
    endif()

    _qt_internal_android_get_gradle_source_sets(SOURCE_SETS ${target})
    _qt_internal_android_get_gradle_dependencies(GRADLE_DEPENDENCIES ${target})

    _qt_internal_android_get_gradle_property(min_sdk_version ${target}
        QT_ANDROID_MIN_SDK_VERSION "28")

    _qt_internal_android_get_gradle_property(target_sdk_version ${target}
        QT_ANDROID_TARGET_SDK_VERSION "36")

    get_target_property(android_target_type ${target} _qt_android_target_type)

    set(default_config_lines
        "minSdk = ${min_sdk_version}"
        "targetSdk = ${target_sdk_version}"
    )
    # AGP rejects abiFilters inside a dynamic-feature module; it inherits the
    # set from the application module. Only emit the ndk block for the app.
    if(android_target_type STREQUAL "APPLICATION")
        set(target_abis "$<TARGET_PROPERTY:${target},_qt_android_abis>")
        set(target_abi_list
            "$<JOIN:${target_abis};${CMAKE_ANDROID_ARCH_ABI},'$<COMMA> '>")
        list(APPEND default_config_lines
            "ndk {"
            "    abiFilters += ['${target_abi_list}']"
            "}"
        )
    endif()
    string(JOIN "\n        " DEFAULT_CONFIG_VALUES ${default_config_lines})

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

    if(android_target_type STREQUAL "APPLICATION")
        _qt_internal_android_get_manifest_property(APP_PACKAGE_NAME ${target}
            QT_ANDROID_PACKAGE_NAME "org.qtproject.example.$<MAKE_C_IDENTIFIER:${target}>")
        _qt_internal_android_get_manifest_property(APP_NAME ${target}
            QT_ANDROID_APP_NAME "${target}")
        _qt_internal_android_get_manifest_property(APP_VERSION_CODE ${target}
            QT_ANDROID_VERSION_CODE "1")
        _qt_internal_android_get_manifest_property(APP_VERSION_NAME ${target}
            QT_ANDROID_VERSION_NAME "1")
        _qt_internal_android_get_manifest_property(APP_LIB_NAME ${target}
            OUTPUT_NAME "${target}")
        set(APP_ARGUMENTS "${QT_ANDROID_APPLICATION_ARGUMENTS}")

        # Legacy packaging
        if(QT_FEATURE_sanitize_address)
            message(STATUS "QT_FEATURE_sanitize_address is set, using legacy packaging by default.")
            set(legacy_packaging_value "true")
        else()
            set(legacy_packaging_value
                "$<IF:$<BOOL:$<TARGET_PROPERTY:${target},QT_ANDROID_LEGACY_PACKAGING>>,true,false>")
        endif()
        string(APPEND ANDROID_DEPLOYMENT_EXTRAS
            "\n    packaging {"
            "\n        jniLibs {"
            "\n            useLegacyPackaging = ${legacy_packaging_value}"
            "\n        }"
            "\n    }"
        )

        set(template_subdir "app")
    elseif(android_target_type STREQUAL "DYNAMIC_FEATURE")
        set(template_subdir "dynamic_feature")
    else()
        message(FATAL_ERROR "Unsupported target type for android bundle deployment ${target}")
    endif()

    _qt_internal_android_get_template_path(template_file ${target}
        "${template_subdir}/${build_gradle_filename}")
    _qt_internal_android_read_template(build_gradle_content "${template_file}")
    _qt_internal_configure_file(GENERATE
        OUTPUT "${out_file}"
        CONTENT "${build_gradle_content}"
    )
    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${out_file}")
endfunction()

# Registers a library copy target as a dependency of ${target}_deploy_libraries.
function(_qt_internal_android_add_deploy_libraries_dependency target dep_target)
    if(NOT TARGET ${dep_target})
        return()
    endif()
    if(NOT TARGET ${target}_deploy_libraries)
        add_custom_target(${target}_deploy_libraries)
    endif()
    add_dependencies(${target}_deploy_libraries ${dep_target})
endfunction()

# Records deploy artifact files for gradle's file-level dependency tracking.
function(_qt_internal_android_register_deploy_files target)
    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deploy_files "${ARGN}")
endfunction()

# Lists the files a secondary ABI deploys, matching what the copy steps actually write.
function(_qt_internal_android_get_abi_deploy_files out_var target abi)
    _qt_internal_android_get_target_deployment_dir(deployment_dir ${target})
    set(libs_abi_dir "${deployment_dir}/libs/${abi}")

    # The app binary is always deployed.
    set(deploy_files "${libs_abi_dir}/lib${target}_${abi}.so")

    # libc++ and the extra libs are skipped when Qt libs aren't deployed
    get_target_property(no_deploy_qt_libs ${target} QT_ANDROID_NO_DEPLOY_QT_LIBS)
    if(NOT no_deploy_qt_libs)
        list(APPEND deploy_files "${libs_abi_dir}/libc++_shared.so")

        get_target_property(extra_libs ${target} QT_ANDROID_EXTRA_LIBS)
        if(extra_libs)
            foreach(lib IN LISTS extra_libs)
                get_filename_component(lib_name "${lib}" NAME)
                if(lib_name)
                    list(APPEND deploy_files "${libs_abi_dir}/${lib_name}")
                endif()
            endforeach()
        endif()
    endif()

    set(${out_var} "${deploy_files}" PARENT_SCOPE)
endfunction()

# Prepares the artifacts for the gradle build of the target.
function(_qt_internal_android_prepare_gradle_build target)
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    _qt_internal_android_get_target_deployment_dir(deployment_dir ${target})

    # Gradle files and resources are ABI-independent, only the main ABI packages them.
    if(NOT QT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT)
        _qt_internal_android_copy_gradle_files(${target} "${android_build_dir}")
        _qt_internal_android_copy_target_package_sources(${target} "${deployment_dir}")
        _qt_internal_android_copy_android_resources(${target} "${deployment_dir}")
    endif()

    _qt_internal_android_copy_app_binary(${target} "${deployment_dir}")
    _qt_internal_android_copy_stdlib(${target} "${deployment_dir}")
    _qt_internal_android_copy_extra_plugins(${target} "${deployment_dir}")
    # TODO: don't call qmlimportscanner again at all for multi-ABI external builds.
    _qt_internal_android_parse_qmlimportscanner_output(${target})
    _qt_internal_android_collect_qt_modules_and_plugins(${target})
    # Needs to run after all dependencies are collected
    _qt_internal_android_include_openssl_if_needed(${target})
    _qt_internal_android_copy_extra_libs(${target} "${deployment_dir}")
    _qt_internal_android_copy_non_qt_linked_libs(${target} "${deployment_dir}")
    _qt_internal_android_copy_qt_dependencies(${target} "${deployment_dir}")
    _qt_internal_android_copy_qml_plugins(${target} "${deployment_dir}")

    # Multi-ABI external builds need to only care about the libraries deployment.
    if(QT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT)
        add_custom_target(qt_internal_${target}_copy_apk_dependencies
            DEPENDS ${target}_deploy_libraries)
        add_custom_target(${target}_make_apk)
        add_custom_target(${target}_make_aab)
        return()
    endif()

    _qt_internal_android_create_rcc_bundle(${target} "${deployment_dir}")
    _qt_internal_android_generate_libs_xml(${target} "${deployment_dir}")

    _qt_internal_android_generate_bundle_gradle_properties(${target})
    _qt_internal_android_generate_bundle_settings_gradle(${target})
    _qt_internal_android_generate_bundle_local_properties(${target})
    _qt_internal_android_generate_target_build_gradle(${target} DEPLOYMENT_DIR "${deployment_dir}")
    _qt_internal_android_generate_target_android_manifest(${target}
        DEPLOYMENT_DIR "${deployment_dir}")


    _qt_internal_android_add_gradle_build(${target} apk)
    _qt_internal_android_add_gradle_build(${target} aab)

    _qt_internal_android_setup_qml_imports_reconfigure_trigger(${target})

    # Make global apk, aab, and aar targets depend on the respective targets.
    _qt_internal_android_add_global_package_dependencies(${target})
    _qt_internal_create_global_apk_all_target_if_needed()
endfunction()

# Returns the path to the output package file for the target.
function(_qt_internal_android_get_output_package_name out_var target type)
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
    if(type STREQUAL "aab")
        _qt_internal_android_get_deployment_type_option(android_deployment_type_option
            "bundleRelease" "bundleDebug")
    else()
        _qt_internal_android_get_deployment_type_option(android_deployment_type_option
            "assembleRelease" "assembleDebug")
    endif()

    _qt_internal_android_gradlew_name(gradlew_file_name)
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(gradlew "${android_build_dir}/${gradlew_file_name}")

    _qt_internal_android_get_output_package_name(package_build_file_path ${target} ${type})

    set(extra_deps "")
    foreach(dep ${target}_copy_feature_names
                 ${target}_copy_apk_dependencies
                 ${target}_copy_gradle_files
                 ${target}_copy_android_res_files
                 ${target}_copy_package_sources
                 ${target}_build_qml_bundle
                 ${target}_update_libs_xml)
        if(TARGET ${dep})
            list(APPEND extra_deps ${dep})
        endif()
    endforeach()

    set(gradle_scripts "$<TARGET_PROPERTY:${target},_qt_android_deployment_files>")
    # File-level deps so gradle re-runs on deploy artifact changes.
    set(deploy_files "$<TARGET_PROPERTY:${target},_qt_android_deploy_files>")
    add_custom_command(OUTPUT "${package_build_file_path}"
        COMMAND "${gradlew}" ${android_deployment_type_option}
        DEPENDS
            ${target}
            ${gradle_scripts}
            ${deploy_files}
            ${target}_deploy_libraries
            ${extra_deps}
        WORKING_DIRECTORY "${android_build_dir}"
        VERBATIM
    )

    set(package_file_path "${android_build_dir}/${target}.${type}")
    _qt_internal_android_sign_package(signed_package_path ${target} ${type}
        "${package_build_file_path}")
    if(NOT signed_package_path)
        add_custom_command(OUTPUT "${package_file_path}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${package_build_file_path}" "${package_file_path}"
            DEPENDS "${package_build_file_path}"
            VERBATIM
        )
    endif()

    _qt_internal_android_gradle_cleanup_commands(gradle_cleanup_commands
        "${gradlew}" "${android_build_dir}" "${target}")

    if(gradle_cleanup_commands)
        add_custom_target(${target}_${type}_cleanup
            ${gradle_cleanup_commands}
            DEPENDS "${package_file_path}"
            WORKING_DIRECTORY "${android_build_dir}"
            VERBATIM
        )
        add_custom_target(${target}_make_${type})
        add_dependencies(${target}_make_${type} ${target}_${type}_cleanup)
    else()
        add_custom_target(${target}_make_${type} DEPENDS "${package_file_path}")
    endif()
endfunction()

function(_qt_internal_android_sign_package out_file target type unsigned_build_file_path)
    string(TOUPPER "${type}" type_upper)
    if(type STREQUAL "aar" OR NOT QT_ANDROID_SIGN_${type_upper})
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
        set(extra_args "-DZIPALIGN_PATH=${build_tools_base_path}/zipalign${suffix}")
    endif()

    _qt_internal_android_get_output_package_name(package_build_file_path ${target} ${type})

    _qt_internal_android_package_path(package_build_dir ${target} ${type})
    _qt_internal_android_get_deployment_type_option(deployment_type_suffix
        "release" "debug")
    set(base_output_path "${package_build_dir}/${deployment_type_suffix}")
    set(package_build_file_path_signed
        "${base_output_path}/app-${deployment_type_suffix}-signed.${type}")

    set(package_file_path "${android_build_dir}/${target}.${type}")

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
            ${unsigned_build_file_path}
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

# Sanitize the target name to be used in file names
function(_qt_internal_android_sanitize_target_name out_var target)
    string(REPLACE "::" "_" sanitized "${target}")
    set(${out_var} "${sanitized}" PARENT_SCOPE)
endfunction()

# Returns (and creates) the per-directory staging dir for Android build tracking artifacts.
function(_qt_internal_android_staging_dir out_var)
    set(dir "${CMAKE_CURRENT_BINARY_DIR}/.qt/android")
    file(MAKE_DIRECTORY "${dir}")
    set(${out_var} "${dir}" PARENT_SCOPE)
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

    _qt_internal_android_read_template(manifest_content "${template_file}")
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

    _qt_internal_configure_file(GENERATE OUTPUT "${out_file}"
        CONTENT "${manifest_content}")

    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${out_file}")
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
        CONFIGURE_DEPENDS
        RELATIVE "${package_source_dir}"
        "${package_source_dir}/*"
    )

    # Skip manifest copying because it will be handled and generated under
    # _qt_internal_android_generate_target_android_manifest().
    list(REMOVE_ITEM package_files "AndroidManifest.xml")

    if("res/xml/qtprovider_paths.xml" IN_LIST package_files)
        message(WARNING
            "${package_source_dir}/res/xml/qtprovider_paths.xml in the package "
            "source directory of ${target} is being excluded. Qt now bundles "
            "its own copy of it under the Android sources directory, and a "
            "duplicate would fail the Gradle build. Remove this file from your "
            "package source directory to silence this warning.")
        list(REMOVE_ITEM package_files "res/xml/qtprovider_paths.xml")
    endif()

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
        # Copy files individually so that the processed AndroidManifest.xml isn't overwritten
        # by the unprocessed user-provided copy.
        set(copy_commands "")
        set(seen_dirs "")
        foreach(rel_file IN LISTS package_files)
            get_filename_component(dst_dir "${deployment_dir}/${rel_file}" DIRECTORY)
            if(NOT dst_dir IN_LIST seen_dirs)
                list(APPEND seen_dirs "${dst_dir}")
                list(APPEND copy_commands
                    COMMAND "${CMAKE_COMMAND}" -E make_directory "${dst_dir}")
            endif()
            list(APPEND copy_commands
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${package_source_dir}/${rel_file}"
                    "${deployment_dir}/${rel_file}")
        endforeach()
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

    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files ${out_package_files})

    # This is used by _qt_internal_android_generate_target_android_manifest()
    # to ensure it's run after the package sources are copied.
    add_custom_target(${target}_copy_package_sources DEPENDS ${out_package_files})
endfunction()

# Copies gradle scripts to a build directory.
function(_qt_internal_android_copy_gradle_files target output_directory)
    _qt_internal_android_gradlew_name(gradlew_file_name)
    _qt_internal_android_gradle_template_dir(gradle_template_dir)
    _qt_internal_android_template_dir(android_template_dir)

    set(gradlew_src "${gradle_template_dir}/${gradlew_file_name}")
    set(gradlew_dst "${output_directory}/${gradlew_file_name}")

    set(wrapper_srcs "")
    set(wrapper_dsts "")
    set(wrapper_src_dir "${gradle_template_dir}/gradle/wrapper")
    set(wrapper_dst_dir "${output_directory}/gradle/wrapper")
    set(wrapper_filenames "gradle-wrapper.jar" "gradle-wrapper.properties")
    foreach(file IN LISTS wrapper_filenames)
        list(APPEND wrapper_srcs "${wrapper_src_dir}/${file}")
        list(APPEND wrapper_dsts "${wrapper_dst_dir}/${file}")
    endforeach()

    set(libs_versions_filename "libs.versions.toml")
    set(libs_versions_src "${android_template_dir}/gradle/${libs_versions_filename}")
    set(libs_versions_dst "${output_directory}/gradle/${libs_versions_filename}")

    add_custom_command(
        OUTPUT "${gradlew_dst}" ${wrapper_dsts} "${libs_versions_dst}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${gradlew_src}" "${gradlew_dst}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${wrapper_dst_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${wrapper_srcs} "${wrapper_dst_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libs_versions_src}" "${libs_versions_dst}"
        DEPENDS "${gradlew_src}" ${wrapper_srcs} "${libs_versions_src}"
        VERBATIM
    )
    add_custom_target(${target}_copy_gradle_files
        DEPENDS "${gradlew_dst}" ${wrapper_dsts} "${libs_versions_dst}"
    )
    _qt_internal_android_register_deploy_files(${target}
        "${gradlew_dst}" ${wrapper_dsts} "${libs_versions_dst}")
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
    if(package_res_outputs)
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
    _qt_internal_android_register_deploy_files(${target} ${dst_res_files})
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
    _qt_internal_android_add_deploy_libraries_dependency(${target} ${target}_copy_stdlib)
    _qt_internal_android_register_deploy_files(${target} "${stdlib_dst}")
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

        _qt_internal_android_append_to_libs_xml_section(${target} extra_libs "${lib_name}")
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
    _qt_internal_android_add_deploy_libraries_dependency(${target} ${target}_copy_extra_libs)
    _qt_internal_android_register_deploy_files(${target} ${copy_outputs})
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
        return()
    endif()

    set(extra_plugins_dst_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")

    _qt_internal_collect_buildsystem_targets(buildsystem_library_targets
        "${CMAKE_SOURCE_DIR}" INCLUDE SHARED_LIBRARY MODULE_LIBRARY)

    set(copy_commands "")
    set(copy_depends "")
    set(extra_plugin_targets "")
    foreach(plugin_dir IN LISTS extra_plugins)
        if(NOT plugin_dir)
            continue()
        endif()

        if("${plugin_dir}" MATCHES "^[$]<")
            # Genex paths: defer to copy_directory which evaluates at build time.
            list(APPEND copy_commands COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${plugin_dir}" "${extra_plugins_dst_dir}")
            if("${plugin_dir}" MATCHES "^\\$<TARGET_FILE_DIR:([^>]+)>$")
                if(TARGET "${CMAKE_MATCH_1}")
                    list(APPEND extra_plugin_targets "${CMAKE_MATCH_1}")
                else()
                    message(WARNING "QT_ANDROID_EXTRA_PLUGINS entry '${plugin_dir}' for ${target} "
                                    "skipped invalid auto-discovered dependency ${CMAKE_MATCH_1}")
                endif()
            else()
                message(WARNING "QT_ANDROID_EXTRA_PLUGINS entry '${plugin_dir}' for ${target} "
                                "skipped dependencies auto-discovery due to unsupported genex.")
            endif()
            continue()
        endif()

        file(TO_CMAKE_PATH "${plugin_dir}" plugin_path)
        if(NOT IS_ABSOLUTE "${plugin_path}")
            message(WARNING "QT_ANDROID_EXTRA_PLUGINS entry '${plugin_dir}' for ${target} "
                            "is not an absolute path.")
        endif()

        # Copy at build time to also pick up plugins generated into the directory;
        # the glob only feeds DEPENDS.
        list(APPEND copy_commands COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${plugin_path}" "${extra_plugins_dst_dir}")
        file(GLOB_RECURSE plugin_files CONFIGURE_DEPENDS LIST_DIRECTORIES false
            "${plugin_path}/*")
        list(APPEND copy_depends ${plugin_files})

        # Find targets with LIBRARY_OUTPUT_DIRECTORY resolving to extra plugin directory.
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

    if(NOT copy_commands)
        return()
    endif()

    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    _qt_internal_android_staging_dir(android_staging_dir)
    set(stamp "${android_staging_dir}/${sanitized_target}_copy_extra_plugins.stamp")
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${extra_plugins_dst_dir}"
        ${copy_commands}
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${copy_depends} ${target} ${extra_plugin_targets}
        COMMENT "Copying extra plugins for ${target}"
        VERBATIM
    )
    add_custom_target(${target}_copy_extra_plugins DEPENDS "${stamp}")
    _qt_internal_android_add_deploy_libraries_dependency(${target} ${target}_copy_extra_plugins)
    _qt_internal_android_register_deploy_files(${target} "${stamp}")
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

    get_target_property(plugin_targets ${target} _qt_android_qml_plugins)
    if(NOT plugin_targets)
        return()
    endif()

    set(libs_abi_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    set(copy_commands "")
    set(copy_depends "")

    foreach(plugin_target IN LISTS plugin_targets)
        if(NOT TARGET "${plugin_target}")
            continue()
        endif()
        set(plugin_filename "$<TARGET_FILE_NAME:${plugin_target}>")
        _qt_internal_android_append_to_libs_xml_section(${target} qt_libs "${plugin_filename}")
        _qt_internal_android_get_deploy_command(deploy_cmd
            "$<TARGET_FILE:${plugin_target}>" "${libs_abi_dir}/${plugin_filename}")
        list(APPEND copy_commands COMMAND ${deploy_cmd})
        list(APPEND copy_depends "${plugin_target}")
    endforeach()

    if(NOT copy_commands)
        return()
    endif()

    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    _qt_internal_android_staging_dir(android_staging_dir)
    set(stamp "${android_staging_dir}/${sanitized_target}_copy_qml_plugins.stamp")
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${libs_abi_dir}"
        ${copy_commands}
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${copy_depends}
        COMMENT "Copying QML plugin targets for ${target}"
        VERBATIM
    )
    add_custom_target(${target}_copy_qml_plugins DEPENDS "${stamp}")
    _qt_internal_android_add_deploy_libraries_dependency(${target} ${target}_copy_qml_plugins)
    _qt_internal_android_register_deploy_files(${target} "${stamp}")
endfunction()

# Copies non-Qt shared libraries linked to the target.
function(_qt_internal_android_copy_non_qt_linked_libs target deployment_dir)
    if(QT_NO_COLLECT_BUILD_TREE_APK_DEPS)
        return()
    endif()

    if(TARGET ${target}_copy_non_qt_linked_libs)
        return()
    endif()

    set(queue "${target}")
    set(processed "")
    set(linked_libs "")

    # Queue project plugin targets so that non-Qt libs they link against are deployed.
    get_target_property(qml_plugin_targets ${target} _qt_android_qml_plugins)
    if(qml_plugin_targets)
        list(APPEND queue ${qml_plugin_targets})
    endif()

    get_target_property(plugin_targets ${target} _qt_android_qt_plugins)
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

        _qt_internal_android_extract_link_libraries_targets("${current_target}" link_targets)
        foreach(entry IN LISTS link_targets)
            if(NOT entry IN_LIST processed AND NOT entry IN_LIST queue)
                list(APPEND queue "${entry}")
            endif()

            get_target_property(entry_type "${entry}" TYPE)
            if(NOT entry_type OR NOT entry_type MATCHES "^(SHARED|MODULE)_LIBRARY$")
                continue()
            endif()

            # Skip if it's a Qt library.
            get_target_property(entry_qt_package_version "${entry}" _qt_package_version)
            if(entry_qt_package_version)
                continue()
            endif()

            list(APPEND linked_libs "${entry}")
        endforeach()
    endwhile()

    list(REMOVE_DUPLICATES linked_libs)

    if(NOT linked_libs)
        return()
    endif()

    set(libs_abi_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    set(linked_libs_copy_commands COMMAND ${CMAKE_COMMAND} -E make_directory "${libs_abi_dir}")
    foreach(lib IN LISTS linked_libs)
        _qt_internal_android_get_deploy_command(deploy_cmd
            "$<TARGET_FILE:${lib}>" "${libs_abi_dir}/$<TARGET_FILE_NAME:${lib}>")
        list(APPEND linked_libs_copy_commands COMMAND ${deploy_cmd})
        _qt_internal_android_append_to_libs_xml_section(${target}
            extra_libs "$<TARGET_FILE_NAME:${lib}>")
    endforeach()

    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    _qt_internal_android_staging_dir(android_staging_dir)
    set(stamp "${android_staging_dir}/${sanitized_target}_copy_non_qt_linked_libs.stamp")
    add_custom_command(OUTPUT "${stamp}"
        ${linked_libs_copy_commands}
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${linked_libs}
        COMMENT "Copying linked shared libraries for ${target}"
        VERBATIM
    )
    add_custom_target(${target}_copy_non_qt_linked_libs DEPENDS "${stamp}")
    _qt_internal_android_add_deploy_libraries_dependency(${target}
        ${target}_copy_non_qt_linked_libs)
    _qt_internal_android_register_deploy_files(${target} "${stamp}")
endfunction()

# Copies (or symlinks) the app's binary file to the deployment dir.
function(_qt_internal_android_copy_app_binary target deployment_dir)
    set(libs_abi_dir "${deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    set(target_file_dst "${libs_abi_dir}/$<TARGET_FILE_NAME:${target}>")

    _qt_internal_android_get_use_terminal_for_deployment(uses_terminal)

    _qt_internal_android_get_deploy_command(deploy_command
        "$<TARGET_FILE:${target}>" "${target_file_dst}")
    if(QT_ANDROID_CREATE_SYMLINKS_ONLY)
        set(deploy_comment "Symlinking ${target} binary to apk folder")
    else()
        set(deploy_comment "Copying ${target} binary to apk folder")
    endif()

    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    _qt_internal_android_staging_dir(android_staging_dir)
    set(stamp "${android_staging_dir}/${sanitized_target}_copy_app_binary.stamp")
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${libs_abi_dir}"
        COMMAND ${deploy_command}
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${target}
        COMMENT "${deploy_comment}"
        VERBATIM
        ${uses_terminal}
    )
    add_custom_target(${target}_copy_app_binary ALL DEPENDS "${stamp}")
    _qt_internal_android_add_deploy_libraries_dependency(${target} ${target}_copy_app_binary)
    _qt_internal_android_register_deploy_files(${target} "${stamp}")
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

# Warns once that ${missing} is not an imported target and so cannot be deployed.
function(_qt_internal_android_warn_unresolved_dependency dependent missing)
    get_property(warned GLOBAL PROPERTY _qt_internal_android_warned_undeployable_deps)
    if(missing IN_LIST warned)
        return()
    endif()
    set_property(GLOBAL APPEND PROPERTY _qt_internal_android_warned_undeployable_deps "${missing}")

    string(REGEX REPLACE "^${QT_CMAKE_EXPORT_NAMESPACE}::" "" component "${missing}")
    message(WARNING
        "The target \"${dependent}\" depends on \"${missing}\", but it is not "
        "an imported target, so its backing library cannot be deployed and "
        "will be missing from the Android bundle. Import it using:\n"
        "  find_package(${QT_CMAKE_EXPORT_NAMESPACE} COMPONENTS ${component})")
endfunction()

# Returns the list of valid CMake targets from the link library properties of the given target.
function(_qt_internal_android_extract_link_libraries_targets target out_targets)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type_upper)
    set(result "")
    foreach(property_name IN ITEMS
            LINK_LIBRARIES
            INTERFACE_LINK_LIBRARIES
            IMPORTED_LINK_DEPENDENT_LIBRARIES
            "IMPORTED_LINK_DEPENDENT_LIBRARIES_${build_type_upper}")
        get_target_property(link_entries "${target}" ${property_name})
        if(NOT link_entries)
            continue()
        endif()
        foreach(raw_entry IN LISTS link_entries)
            # Unwrap genex wrappers (e.g. $<LINK_ONLY:Qt6::Core>) to get the target name.
            if(raw_entry MATCHES "^\\$<[^:]+:(.+)>$")
                set(entry "${CMAKE_MATCH_1}")
            else()
                set(entry "${raw_entry}")
            endif()
            if(TARGET "${entry}")
                get_target_property(entry_alias "${entry}" ALIASED_TARGET)
                if(entry_alias)
                    set(entry "${entry_alias}")
                endif()
                list(APPEND result "${entry}")
            elseif(entry MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::")
                _qt_internal_android_warn_unresolved_dependency("${target}" "${entry}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES result)
    set(${out_targets} "${result}" PARENT_SCOPE)
endfunction()

# Resolves a QML's module backing target
function(_qt_internal_android_resolve_backing_target target out_var)
    set(${out_var} "" PARENT_SCOPE)
    get_target_property(qml_backing "${target}" _qt_qml_module_backing_target)
    if(NOT qml_backing)
        return()
    endif()

    set (qt_ns "${QT_CMAKE_EXPORT_NAMESPACE}")
    if(TARGET "${qml_backing}")
        set(${out_var} "${qml_backing}" PARENT_SCOPE)
    elseif(TARGET "${qt_ns}::${qml_backing}")
        set(${out_var} "${qt_ns}::${qml_backing}" PARENT_SCOPE)
    else()
        _qt_internal_android_warn_unresolved_dependency("${target}" "${qt_ns}::${qml_backing}")
    endif()
endfunction()

# Collects Qt modules that a target depends on and their associated plugins.
function(_qt_internal_android_collect_qt_modules_and_plugins target)
    set(pending_stack "${target}")
    set(visited "")
    set(collected_modules "")
    set(collected_plugins "")

    get_target_property(qml_plugins ${target} _qt_android_qml_plugins)
    if(qml_plugins)
        list(APPEND pending_stack ${qml_plugins})
    endif()

    set(qt_ns ${QT_CMAKE_EXPORT_NAMESPACE})
    while(pending_stack)
        list(GET pending_stack -1 current_target)

        if(current_target IN_LIST visited)
            list(POP_BACK pending_stack)
            get_target_property(target_type "${current_target}" TYPE)
            get_target_property(is_qml_plugin "${current_target}" _qt_qml_module_is_plugin_target)
            if (is_qml_plugin OR NOT target_type MATCHES "^(SHARED|MODULE)_LIBRARY$")
                continue()
            endif()
            if(current_target MATCHES "^${qt_ns}::" OR TARGET "${qt_ns}::${current_target}")
                if(NOT current_target IN_LIST collected_modules)
                    list(APPEND collected_modules "${current_target}")
                endif()
            endif()
            continue()
        endif()
        list(APPEND visited "${current_target}")

        set(direct_deps "")
        _qt_internal_android_resolve_backing_target("${current_target}" backing)
        list(APPEND direct_deps "${backing}")

        _qt_internal_android_list_module_plugins("${current_target}" module_plugins)
        foreach(plugin_target IN LISTS module_plugins)
            if(NOT plugin_target IN_LIST collected_plugins)
                list(APPEND collected_plugins "${plugin_target}")
            endif()
        endforeach()

        foreach(dep_source IN ITEMS "${current_target}" ${module_plugins})
            _qt_internal_android_list_plugin_modules("${dep_source}" plugin_module_deps)
            list(APPEND direct_deps ${plugin_module_deps})
        endforeach()

        _qt_internal_android_extract_link_libraries_targets("${current_target}" link_targets)
        list(APPEND direct_deps ${link_targets})

        list(FILTER direct_deps EXCLUDE REGEX "^$")
        list(REMOVE_DUPLICATES direct_deps)
        list(REMOVE_ITEM direct_deps "${current_target}")
        list(APPEND pending_stack ${direct_deps})
    endwhile()

    list(REMOVE_DUPLICATES collected_modules)
    list(REMOVE_DUPLICATES collected_plugins)

    set_property(TARGET ${target} PROPERTY _qt_android_qt_modules "${collected_modules}")
    set_property(TARGET ${target} PROPERTY _qt_android_qt_plugins "${collected_plugins}")
endfunction()

# Adds a deploy command into commands/depends lists in the caller's scope, skipping duplicates.
function(_qt_internal_android_add_dependency_deploy_step)
    set(one_value_args
        SOURCE
        DESTINATION
        DEPENDS_ITEM
        SEEN_DESTINATIONS_VAR
        COPY_COMMANDS_VAR
        COPY_DEPENDS_VAR
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "${one_value_args}" "")

    if("${arg_DESTINATION}" IN_LIST ${arg_SEEN_DESTINATIONS_VAR})
        return()
    endif()
    _qt_internal_android_get_deploy_command(cmd "${arg_SOURCE}" "${arg_DESTINATION}")
    get_filename_component(dst_dir "${arg_DESTINATION}" DIRECTORY)
    list(APPEND ${arg_COPY_COMMANDS_VAR} COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}")
    list(APPEND ${arg_SEEN_DESTINATIONS_VAR} "${arg_DESTINATION}")
    list(APPEND ${arg_COPY_COMMANDS_VAR} COMMAND ${cmd})
    list(APPEND ${arg_COPY_DEPENDS_VAR} "${arg_DEPENDS_ITEM}")
    set(${arg_SEEN_DESTINATIONS_VAR} "${${arg_SEEN_DESTINATIONS_VAR}}" PARENT_SCOPE)
    set(${arg_COPY_COMMANDS_VAR} "${${arg_COPY_COMMANDS_VAR}}" PARENT_SCOPE)
    set(${arg_COPY_DEPENDS_VAR} "${${arg_COPY_DEPENDS_VAR}}" PARENT_SCOPE)
endfunction()

# Deploys QT_ANDROID_BUNDLED_JAR_DEPENDENCIES declared by dep.
function(_qt_internal_android_process_bundled_jars dep libs_root_dir
        inout_seen_destinations inout_copy_commands inout_copy_depends)
    get_target_property(jar_deps "${dep}" QT_ANDROID_BUNDLED_JAR_DEPENDENCIES)
    if(NOT jar_deps)
        return()
    endif()

    _qt_internal_get_android_abi_prefix_path(qt_prefix ${CMAKE_ANDROID_ARCH_ABI})
    _qt_internal_get_android_abi_subdir_path(qt_data_dir QT6_INSTALL_DATA ${CMAKE_ANDROID_ARCH_ABI})
    _qt_internal_path_join(qt_data_dir "${qt_prefix}" "${qt_data_dir}")

    foreach(jar IN LISTS jar_deps)
        if(IS_ABSOLUTE "${jar}")
            set(jar_absolute "${jar}")
        else()
            _qt_internal_path_join(jar_absolute "${qt_data_dir}" "${jar}")
        endif()
        if(NOT EXISTS "${jar_absolute}")
            message(WARNING "The JAR dependency '${jar_absolute}' is missing for ${dep}.")
            continue()
        endif()

        string(REGEX REPLACE "^jar/" "" jar_relative "${jar}")
        _qt_internal_path_join(destination "${libs_root_dir}" "${jar_relative}")
        _qt_internal_android_add_dependency_deploy_step(
            SOURCE "${jar_absolute}"
            DESTINATION "${destination}"
            DEPENDS_ITEM "${jar_absolute}"
            SEEN_DESTINATIONS_VAR ${inout_seen_destinations}
            COPY_COMMANDS_VAR ${inout_copy_commands}
            COPY_DEPENDS_VAR ${inout_copy_depends})
    endforeach()

    set(${inout_seen_destinations} "${${inout_seen_destinations}}" PARENT_SCOPE)
    set(${inout_copy_commands} "${${inout_copy_commands}}" PARENT_SCOPE)
    set(${inout_copy_depends} "${${inout_copy_depends}}" PARENT_SCOPE)
endfunction()

# Deploys the dep's own .so and registers it in libs.xml qt_libs, unless dep
# has QT_PLUGIN_TYPE set (plugins are handled by the Qt plugin loader).
function(_qt_internal_android_deploy_qt_dependency)
    set(one_value_args
        TARGET
        DEPENDENCY_TARGET
        LIBS_ABI_DIR
        SEEN_DESTINATIONS_VAR
        COPY_COMMANDS_VAR
        COPY_DEPENDS_VAR
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "${one_value_args}" "")

    if(NOT TARGET "${arg_DEPENDENCY_TARGET}")
        return()
    endif()

    set(destination "${arg_LIBS_ABI_DIR}/$<TARGET_FILE_NAME:${arg_DEPENDENCY_TARGET}>")
    if("${destination}" IN_LIST ${arg_SEEN_DESTINATIONS_VAR})
        return()
    endif()

    get_target_property(plugin_type "${arg_DEPENDENCY_TARGET}" QT_PLUGIN_TYPE)
    if(NOT plugin_type)
        get_target_property(iface_name "${arg_DEPENDENCY_TARGET}" _qt_module_interface_name)
        if(iface_name)
            set(lib_name "${QT_CMAKE_EXPORT_NAMESPACE}${iface_name}_${CMAKE_ANDROID_ARCH_ABI}")
        else()
            set(lib_name "$<TARGET_FILE_NAME:${arg_DEPENDENCY_TARGET}>")
        endif()
        _qt_internal_android_append_to_libs_xml_section(${arg_TARGET} qt_libs "${lib_name}")
    endif()

    _qt_internal_android_add_dependency_deploy_step(
        SOURCE "$<TARGET_FILE:${arg_DEPENDENCY_TARGET}>"
        DESTINATION "${destination}"
        DEPENDS_ITEM "${arg_DEPENDENCY_TARGET}"
        SEEN_DESTINATIONS_VAR ${arg_SEEN_DESTINATIONS_VAR}
        COPY_COMMANDS_VAR ${arg_COPY_COMMANDS_VAR}
        COPY_DEPENDS_VAR ${arg_COPY_DEPENDS_VAR})

    set(${arg_SEEN_DESTINATIONS_VAR} "${${arg_SEEN_DESTINATIONS_VAR}}" PARENT_SCOPE)
    set(${arg_COPY_COMMANDS_VAR} "${${arg_COPY_COMMANDS_VAR}}" PARENT_SCOPE)
    set(${arg_COPY_DEPENDS_VAR} "${${arg_COPY_DEPENDS_VAR}}" PARENT_SCOPE)
endfunction()

# Deploys QT_ANDROID_LIB_DEPENDENCIES declared by dep (module or plugin),
# registering each in the libs.xml local_libs section for JNI_OnLoad().
function(_qt_internal_android_process_lib_deps)
    set(one_value_args
        TARGET
        DEPENDENCY_TARGET
        LIBS_ABI_DIR
        SEEN_DESTINATIONS_VAR
        COPY_COMMANDS_VAR
        COPY_DEPENDS_VAR
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "${one_value_args}" "")

    get_target_property(lib_deps "${arg_DEPENDENCY_TARGET}" QT_ANDROID_LIB_DEPENDENCIES)
    if(NOT lib_deps)
        return()
    endif()

    _qt_internal_get_android_abi_prefix_path(qt_prefix ${CMAKE_ANDROID_ARCH_ABI})
    set(abi_so_suffix "_${CMAKE_ANDROID_ARCH_ABI}.so")

    foreach(lib IN LISTS lib_deps)
        if(IS_ABSOLUTE "${lib}")
            set(lib_absolute "${lib}")
        else()
            _qt_internal_path_join(lib_absolute "${qt_prefix}" "${lib}")
        endif()

        if(NOT lib_absolute MATCHES "${abi_so_suffix}$")
            string(REGEX REPLACE "\\.so$" "${abi_so_suffix}" lib_abi "${lib_absolute}")
            if(EXISTS "${lib_abi}")
                set(lib_absolute "${lib_abi}")
            endif()
        endif()

        if(NOT EXISTS "${lib_absolute}")
            message(WARNING
                "The library dependency '${lib_absolute}' is missing for ${arg_DEPENDENCY_TARGET}.")
            continue()
        endif()

        get_filename_component(filename "${lib_absolute}" NAME)
        _qt_internal_android_append_to_libs_xml_section(${arg_TARGET} local_libs "${filename}")

        _qt_internal_path_join(destination "${arg_LIBS_ABI_DIR}" "${filename}")
        _qt_internal_android_add_dependency_deploy_step(
            SOURCE "${lib_absolute}"
            DESTINATION "${destination}"
            DEPENDS_ITEM "${lib_absolute}"
            SEEN_DESTINATIONS_VAR ${arg_SEEN_DESTINATIONS_VAR}
            COPY_COMMANDS_VAR ${arg_COPY_COMMANDS_VAR}
            COPY_DEPENDS_VAR ${arg_COPY_DEPENDS_VAR})
    endforeach()

    set(${arg_SEEN_DESTINATIONS_VAR} "${${arg_SEEN_DESTINATIONS_VAR}}" PARENT_SCOPE)
    set(${arg_COPY_COMMANDS_VAR} "${${arg_COPY_COMMANDS_VAR}}" PARENT_SCOPE)
    set(${arg_COPY_DEPENDS_VAR} "${${arg_COPY_DEPENDS_VAR}}" PARENT_SCOPE)
endfunction()

# Resolves plugin paths for a module using the _qt_plugins property.
function(_qt_internal_android_list_module_plugins module out_plugin_paths)
    set(result "")

    get_target_property(qt_plugins "${module}" _qt_plugins)
    if(NOT qt_plugins)
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
        get_target_property(plugin_type "${plugin_target}" TYPE)
        if(plugin_type MATCHES "^(SHARED|MODULE)_LIBRARY$")
            list(APPEND result "${plugin_target}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES result)
    set(${out_plugin_paths} "${result}" PARENT_SCOPE)
endfunction()

# Returns the shared Qt module targets declared as dependencies of a plugin via
# _qt_plugin_qt_module_dependencies, filtered to shared libraries only.
function(_qt_internal_android_list_plugin_modules plugin_target out_module_deps)
    get_target_property(plugin_module_deps "${plugin_target}" _qt_plugin_qt_module_dependencies)
    if(NOT plugin_module_deps)
        set(${out_module_deps} "" PARENT_SCOPE)
        return()
    endif()

    set(result "")
    foreach(dep_target IN LISTS plugin_module_deps)
        if(NOT TARGET "${dep_target}")
            _qt_internal_android_warn_unresolved_dependency("${plugin_target}" "${dep_target}")
            continue()
        endif()
        get_target_property(type "${dep_target}" TYPE)
        if(type STREQUAL "SHARED_LIBRARY")
            list(APPEND result "${dep_target}")
        endif()
    endforeach()
    set(${out_module_deps} "${result}" PARENT_SCOPE)
endfunction()

# Copy Qt dependencies (libs, jars, plugins) to the deployment dir.
function(_qt_internal_android_copy_qt_dependencies target deployment_dir)
    set(copy_commands "")
    set(copy_depends "")
    set(seen_destinations "")

    set(libs_root_dir "${deployment_dir}/libs")
    set(libs_abi_dir "${libs_root_dir}/${CMAKE_ANDROID_ARCH_ABI}")
    list(APPEND copy_commands COMMAND ${CMAKE_COMMAND} -E make_directory "${libs_abi_dir}")

    get_target_property(no_deploy_qt_libs ${target} QT_ANDROID_NO_DEPLOY_QT_LIBS)
    get_target_property(qt_modules ${target} _qt_android_qt_modules)
    get_target_property(qt_plugins ${target} _qt_android_qt_plugins)

    foreach(dep IN LISTS qt_modules qt_plugins)
        _qt_internal_android_process_bundled_jars("${dep}" "${libs_root_dir}"
            seen_destinations copy_commands copy_depends)
        if(NOT no_deploy_qt_libs)
            _qt_internal_android_deploy_qt_dependency(
                TARGET "${target}"
                DEPENDENCY_TARGET "${dep}"
                LIBS_ABI_DIR "${libs_abi_dir}"
                SEEN_DESTINATIONS_VAR seen_destinations
                COPY_COMMANDS_VAR copy_commands
                COPY_DEPENDS_VAR copy_depends)
            _qt_internal_android_process_lib_deps(
                TARGET "${target}"
                DEPENDENCY_TARGET "${dep}"
                LIBS_ABI_DIR "${libs_abi_dir}"
                SEEN_DESTINATIONS_VAR seen_destinations
                COPY_COMMANDS_VAR copy_commands
                COPY_DEPENDS_VAR copy_depends)
        endif()
    endforeach()

    # Plugins aren't linked, so their permissions aren't transitive like modules.
    _qt_internal_android_collect_permissions(${target} SOURCE_TARGETS ${qt_plugins})

    list(REMOVE_DUPLICATES copy_depends)
    list(FILTER copy_depends EXCLUDE REGEX "^$")

    if(NOT copy_commands)
        return()
    endif()

    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    _qt_internal_android_staging_dir(android_staging_dir)
    set(stamp "${android_staging_dir}/${sanitized_target}_copy_qt_files.stamp")
    add_custom_command(OUTPUT "${stamp}"
        ${copy_commands}
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${copy_depends}
        COMMENT "Copying Qt deployment files for ${target}"
        VERBATIM
    )
    add_custom_target(${target}_copy_qt_files DEPENDS "${stamp}")
    _qt_internal_android_add_deploy_libraries_dependency(${target} ${target}_copy_qt_files)
    _qt_internal_android_register_deploy_files(${target} "${stamp}")
endfunction()

# Appends a unique lib name for the target to the libs.xml section.
function(_qt_internal_android_append_to_libs_xml_section target section_name lib_name)
    if(NOT lib_name)
        return()
    endif()

    set(normalized_lib_name "${lib_name}")
    # qt_libs are loaded by base name; other sections keep the full file name.
    if(section_name STREQUAL "qt_libs" AND NOT lib_name MATCHES "^[$]<")
        get_filename_component(file_name "${normalized_lib_name}" NAME)
        if(file_name MATCHES "^lib(.+)\\.so$")
            set(normalized_lib_name "${CMAKE_MATCH_1}")
        else()
            string(REGEX REPLACE "\\.so$" "" normalized_lib_name "${file_name}")
        endif()
    endif()

    set(full_section_name "_android_libs_xml_${section_name}")
    get_target_property(current_libs ${target} ${full_section_name})
    if(NOT current_libs)
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
    if(NOT libs)
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

    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    _qt_internal_android_staging_dir(android_staging_dir)
    set(libs_xml_staged "${android_staging_dir}/${sanitized_target}_libs.xml")
    file(GENERATE OUTPUT "${libs_xml_staged}" CONTENT "${content}")

    add_custom_target(${target}_update_libs_xml
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libs_xml_staged}" "${libs_xml_dst}"
        DEPENDS ${target}_copy_android_res_files
        COMMENT "Updating libs.xml for ${target}"
        VERBATIM
    )

    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${libs_xml_dst}")
endfunction()

function(_qt_internal_android_get_qml_root_paths target out_root_paths)
    get_target_property(qml_root_paths ${target} QT_QML_ROOT_PATH)
    if(NOT qml_root_paths)
        get_target_property(qml_root_paths ${target} SOURCE_DIR)
        get_target_property(target_binary_dir ${target} BINARY_DIR)
        list(APPEND qml_root_paths "${target_binary_dir}")
    endif()

    get_target_property(extra_root_paths ${target} QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS)
    if(extra_root_paths)
        list(APPEND qml_root_paths ${extra_root_paths})
    endif()
    if(QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS)
        list(APPEND qml_root_paths ${QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS})
    endif()

    set(${out_root_paths} "${qml_root_paths}" PARENT_SCOPE)
endfunction()

function(_qt_internal_android_get_qmlimportscanner_scan target qmlimportscanner_path out_qml_scan)
    if(NOT qmlimportscanner_path)
        set(${out_qml_scan} "" PARENT_SCOPE)
        return()
    endif()

    _qt_internal_android_get_qml_root_paths(${target} qml_root_paths)
    _qt_internal_collect_qml_import_paths(qml_import_paths ${target})

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
        set(${out_qml_scan} "" PARENT_SCOPE)
        return()
    endif()

    string(STRIP "${qml_scan_output}" qml_scan_output)
    set(${out_qml_scan} "${qml_scan_output}" PARENT_SCOPE)
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

# Reconfigures CMake within a single ninja build when QML imports change.
function(_qt_internal_android_setup_qml_imports_reconfigure_trigger target)
    if(NOT TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::Qml OR QT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT)
        return()
    endif()

    _qt_internal_android_staging_dir(android_staging_dir)
    set(qml_scan_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/QtAndroidScanQmlImports.cmake")

    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    set(snapshot_file "${android_staging_dir}/${sanitized_target}_qml_imports.snapshot")
    set(stamp_file "${android_staging_dir}/${sanitized_target}_qml_imports.stamp")

    set(source_dirs_arg "")
    _qt_internal_android_get_qml_root_paths(${target} qml_root_paths)
    foreach(path IN LISTS qml_root_paths)
        if(source_dirs_arg)
            string(APPEND source_dirs_arg ";")
        endif()
        string(APPEND source_dirs_arg "${path}")
    endforeach()

    set(qml_scan_cmd "${CMAKE_COMMAND}"
        "-DSOURCE_DIRS=${source_dirs_arg}"
        "-DSNAPSHOT=${snapshot_file}"
        "-DSTAMP=${stamp_file}"
        -P "${qml_scan_script}")

    # Seed the snapshot once so the first build doesn't regen on a missing snapshot.
    if(NOT EXISTS "${snapshot_file}")
        execute_process(COMMAND ${qml_scan_cmd})
    endif()

    # CONFIGURE_DEPENDS catches file add/remove, same list feeds the scan rule's DEPENDS.
    set(qml_sources "")
    foreach(path IN LISTS qml_root_paths)
        if(IS_DIRECTORY "${path}")
            file(GLOB_RECURSE found CONFIGURE_DEPENDS LIST_DIRECTORIES false
                "${path}/*.qml" "${path}/*.js" "${path}/*.mjs")
            list(APPEND qml_sources ${found})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES qml_sources)

    # FIXME: This relies on a mid-build regen, which only ninja does. Makefiles
    # check configure dependencies at startup, so a fresh import needs two builds.
    add_custom_command(
        OUTPUT "${stamp_file}"
        BYPRODUCTS "${snapshot_file}"
        COMMAND ${qml_scan_cmd}
        DEPENDS ${qml_sources}
        COMMENT "Checking QML imports for ${target}"
        VERBATIM
    )
    add_custom_target(${target}_qml_imports_check ALL DEPENDS "${stamp_file}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${snapshot_file}")

    foreach(pkg_target ${target}_make_apk ${target}_make_aab)
        if(TARGET ${pkg_target})
            add_dependencies(${pkg_target} ${target}_qml_imports_check)
        endif()
    endforeach()
endfunction()

function(_qt_internal_android_parse_qmlimportscanner_output target)
    if(NOT TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)
        return()
    endif()

    __qt_internal_get_tool_imported_location(qmlimportscanner_path "qmlimportscanner")
    if(NOT qmlimportscanner_path)
        message(WARNING
            "qmlimportscanner not found. Skipping QML dependency scanning for ${target}.")
        return()
    endif()

    _qt_internal_android_get_qmlimportscanner_scan(${target} "${qmlimportscanner_path}" qml_scan)
    if(qml_scan STREQUAL "")
        return()
    endif()

    set(qml_modules "")
    set(qml_plugins "")

    string(JSON module_count LENGTH "${qml_scan}")
    if(NOT module_count GREATER 0)
        return()
    endif()

    math(EXPR module_last_index "${module_count} - 1")
    foreach(mod_index RANGE 0 ${module_last_index})
        _qt_internal_android_json_get_string("${qml_scan}" ${mod_index} path module_path)
        if(NOT module_path)
            continue()
        endif()

        set(skip_module FALSE)
        _qt_internal_android_json_get_string("${qml_scan}" ${mod_index} plugin plugin_name)
        if(NOT plugin_name)
            continue()
        endif()

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
            _qt_internal_android_json_get_bool("${qml_scan}" ${mod_index} pluginIsOptional optional)
            if(NOT optional AND NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
                message(WARNING "QML plugin '${plugin_name}' not found for ${target}.")
                set(skip_module TRUE)
            endif()
        endif()

        if(NOT skip_module)
            get_filename_component(module_abs "${module_path}" ABSOLUTE)
            _qt_internal_android_json_get_string("${qml_scan}" ${mod_index} relativePath module_rel)
            list(APPEND qml_modules "${module_abs}::${module_rel}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES qml_modules)
    list(REMOVE_DUPLICATES qml_plugins)

    set_property(TARGET ${target} PROPERTY _qt_android_qml_modules "${qml_modules}")
    set_property(TARGET ${target} PROPERTY _qt_android_qml_plugins "${qml_plugins}")
endfunction()

function(_qt_internal_android_create_rcc_bundle target deployment_dir)
    get_target_property(qml_modules ${target} _qt_android_qml_modules)
    if(NOT qml_modules)
        return()
    endif()

    __qt_internal_get_tool_imported_location(rcc_path "rcc")
    if(NOT rcc_path)
        message(WARNING "rcc not found. Skipping QML dependency bundling for ${target}.")
        return()
    endif()

    # Stage outside deployment_dir so the rcc bundle cleanup can't invalidate the stamp.
    _qt_internal_android_sanitize_target_name(sanitized_target ${target})
    _qt_internal_android_staging_dir(android_staging_dir)
    set(qml_bundle_dir "${android_staging_dir}/${sanitized_target}-qml-bundle")

    set(commands "")
    set(qmldir_inputs "")
    foreach(module_entry IN LISTS qml_modules)
        string(REPLACE "::" ";" module_parts "${module_entry}")
        list(GET module_parts 0 module_abs)
        list(GET module_parts 1 module_rel)
        if(NOT module_abs OR NOT module_rel)
            continue()
        endif()

        if(NOT EXISTS "${module_abs}/qmldir")
            continue()
        endif()

        set(destination_dir "${qml_bundle_dir}/qml/${module_rel}")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E make_directory "${destination_dir}")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${module_abs}/qmldir" "${destination_dir}/qmldir")
        list(APPEND qmldir_inputs "${module_abs}/qmldir")
    endforeach()

    if(NOT commands)
        return()
    endif()

    set(qml_modules_stamp "${android_staging_dir}/${sanitized_target}_copy_qml_modules.stamp")
    add_custom_command(OUTPUT "${qml_modules_stamp}"
        ${commands}
        COMMAND ${CMAKE_COMMAND} -E touch "${qml_modules_stamp}"
        DEPENDS ${qmldir_inputs}
        COMMENT "Copying QML modules for ${target}"
        VERBATIM
    )
    add_custom_target(${target}_copy_qml_modules DEPENDS "${qml_modules_stamp}")
    _qt_internal_android_register_deploy_files(${target} "${qml_modules_stamp}")

    set(extra_args "")
    if(NOT QT_FEATURE_zstd)
        list(APPEND extra_args "--no-zstd")
    endif()

    set(bundle_rcc "${deployment_dir}/assets/android_rcc_bundle.rcc")
    set(bundle_qrc "${qml_bundle_dir}/android_rcc_bundle.qrc")
    get_filename_component(bundle_rcc_dir "${bundle_rcc}" DIRECTORY)
    add_custom_command(OUTPUT "${bundle_rcc}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${bundle_rcc_dir}"
        COMMAND ${CMAKE_COMMAND} -E chdir "${qml_bundle_dir}"
            "${rcc_path}" --project -o "${bundle_qrc}"
        COMMAND "${rcc_path}" --binary ${extra_args}
            --root=/android_rcc_bundle/ -o "${bundle_rcc}" "${bundle_qrc}"
        DEPENDS "${qml_modules_stamp}"
        COMMENT "Generating QML resource bundle for ${target}"
        VERBATIM
    )

    set_property(TARGET ${target} APPEND PROPERTY _qt_android_deployment_files "${bundle_rcc}")
    add_custom_target(${target}_build_qml_bundle DEPENDS "${bundle_rcc}")
endfunction()
