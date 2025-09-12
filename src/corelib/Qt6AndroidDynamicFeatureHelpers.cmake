# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
# Collects the dynamic features for the target

function(_qt_internal_android_add_dynamic_feature_deployment target)
    get_target_property(dynamic_features ${target} _qt_android_dynamic_features)
    if(NOT dynamic_features)
        return()
    endif()

    foreach(dynamic_feature IN LISTS dynamic_features)
        get_target_property(is_imported ${dynamic_feature} IMPORTED)
        if(is_imported)
            message(FATAL_ERROR "Imported target ${dynamic_feature} can not be a"
                " 'DYNAMIC_FEATURE'.")
        endif()

        get_target_property(type ${dynamic_feature} TYPE)
        if(NOT type STREQUAL "SHARED_LIBRARY")
            message(FATAL_ERROR "Cannot make ${dynamic_feature} 'DYNAMIC_FEATURE'."
                " The target must be the 'SHARED_LIBRARY'.")
        endif()

        get_target_property(android_type ${dynamic_feature} _qt_android_target_type)
        if(NOT android_type STREQUAL "" AND NOT android_type MATCHES "-NOTFOUND$"
            AND NOT android_type STREQUAL "DYNAMIC_FEATURE")
            message(FATAL_ERROR "Cannot make ${dynamic_feature} 'DYNAMIC_FEATURE',"
                " it's already '${android_type}'. The target must be the plain 'SHARED_LIBRARY'.")
        endif()

        # Mark the target as DYNAMIC_FEATURE, since we used it in this role once.
        set_target_properties(${dynamic_feature} PROPERTIES _qt_android_target_type DYNAMIC_FEATURE)
        _qt_internal_set_android_dynamic_feature_gradle_defaults(${dynamic_feature})

        _qt_internal_android_get_dynamic_feature_deployment_dir(dynamic_feature_deployment_dir
            ${target} ${dynamic_feature})
        _qt_internal_android_generate_target_build_gradle(${dynamic_feature}
            DEPLOYMENT_DIR "${dynamic_feature_deployment_dir}")
        _qt_internal_android_generate_dynamic_feature_manifest(${target} ${dynamic_feature})
        _qt_internal_android_copy_dynamic_feature(${target} ${dynamic_feature})
    endforeach()
endfunction()

# Sets the default values of the gradle properties for the Android dynamic feature target.
function(_qt_internal_set_android_dynamic_feature_gradle_defaults target)
    _qt_internal_android_java_dir(android_java_dir)

    # TODO: make androidx.core:core versionc configurable.
    # Currently, it is hardcoded to 1.17.0.
    set(implementation_dependencies "project(':app')" "'androidx.core:core:1.17.0'")

    set_target_properties(${target} PROPERTIES
        _qt_android_gradle_java_source_dirs "src;java"
        _qt_android_gradle_aidl_source_dirs "src;aidl"
        _qt_android_gradle_res_source_dirs "res"
        _qt_android_gradle_resources_source_dirs "resources"
        _qt_android_gradle_renderscript_source_dirs "src"
        _qt_android_gradle_assets_source_dirs "assets"
        _qt_android_gradle_jniLibs_source_dirs "libs"
        _qt_android_manifest "AndroidManifest.xml"
        _qt_android_gradle_implementation_dependencies "${implementation_dependencies}"
    )
endfunction()

# Copies the dynamic feature library to the respective gradle build tree.
function(_qt_internal_android_copy_dynamic_feature target dynamic_feature)
    if(NOT TARGET ${dynamic_feature})
        message(FATAL_ERROR "${dynamic_feature} is not a target.")
    endif()

    _qt_internal_android_get_dynamic_feature_deployment_dir(dynamic_feature_deployment_dir
        ${target} ${dynamic_feature})

    set(dynamic_feature_libs_dir "${dynamic_feature_deployment_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}")
    get_target_property(output_name ${dynamic_feature} OUTPUT_NAME)
    if(NOT output_name)
        get_target_property(suffix "${dynamic_feature}" SUFFIX)
        set(output_name "lib${dynamic_feature}${suffix}")
    endif()
    set(output_file_path "${dynamic_feature_libs_dir}/${output_name}")
    _qt_internal_copy_file_if_different_command(copy_command
        "$<TARGET_FILE:${dynamic_feature}>"
        "${output_file_path}"
    )
    add_custom_command(OUTPUT ${output_file_path}
        COMMAND ${copy_command}
        DEPENDS ${dynamic_feature}
        COMMENT "Copying ${dynamic_feature} dynamic feature to ${target} deployment directory"
    )
    add_custom_target(${target}_deploy_dynamic_features DEPENDS "${output_file_path}")
endfunction()

# Generates the feature name strings and copy them to the respective deployment directory.
function(_qt_internal_android_generate_dynamic_feature_names target)
    get_target_property(dynamic_features ${target} _qt_android_dynamic_features)
    if(NOT dynamic_features)
        return()
    endif()

    # Collect the titles
    string(JOIN "\n" content
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<resources>"
    )
    foreach(feature IN LISTS dynamic_features)
        string(APPEND content "\n    <string name=\"${feature}_title\">${feature}</string>")
    endforeach()
    string(APPEND content "\n</resources>")

    _qt_internal_android_get_target_deployment_dir(deployment_dir ${target})
    # TODO: androiddeployqt wipes the android build directory. Generate feature_names.xml target
    # build dir and copy after androiddeployqt run. We should skip feature_names.xml copying when
    # androiddeployqt is not involved into the deployment process anymore.
    #
    # set(output_file "${deployment_dir}/res/values/feature_names.xml")
    set(output_file "$<TARGET_PROPERTY:${target},BINARY_DIR>/res/values/feature_names.xml")
    _qt_internal_configure_file(GENERATE OUTPUT "${output_file}" CONTENT "${content}")
    set(output_file_in_deployment_dir "${deployment_dir}/res/values/feature_names.xml")
    add_custom_command(OUTPUT "${output_file_in_deployment_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${output_file}"
            "${output_file_in_deployment_dir}"
        DEPENDS "${output_file}" ${target}_android_deploy_aux
        VERBATIM
    )
    add_custom_target(${target}_copy_feature_names
        DEPENDS
            "${output_file_in_deployment_dir}"
    )
endfunction()

# Returns the dynamic feature deployment directory in the target build tree.
function(_qt_internal_android_get_dynamic_feature_deployment_dir out_var target dynamic_feature)
    _qt_internal_android_get_target_android_build_dir(android_build_dir ${target})
    set(${out_var} "${android_build_dir}/${dynamic_feature}" PARENT_SCOPE)
endfunction()

# Generates the AndroidManifest.xml file for the dynamic_feature.
function(_qt_internal_android_generate_dynamic_feature_manifest target dynamic_feature)
    set(android_manifest_filename AndroidManifest.xml)
    _qt_internal_android_get_dynamic_feature_deployment_dir(dynamic_feature_deployment_dir ${target}
        ${dynamic_feature})

    _qt_internal_android_get_template_path(template_file ${target}
        "dynamic_feature/${android_manifest_filename}")

    set(APP_TARGET "${target}")
    set(TITLE_VAR "@string/${dynamic_feature}_title")

    set(output_file "${dynamic_feature_deployment_dir}/AndroidManifest.xml")
    _qt_internal_configure_file(CONFIGURE OUTPUT "${output_file}" INPUT "${template_file}")
endfunction()
