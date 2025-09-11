# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_generate_module_map target)
    if(NOT QT_FEATURE_clang_module_maps)
        return()
    endif()

    get_target_property(module_has_headers ${target} _qt_module_has_headers)
    if(NOT module_has_headers)
        # Skip NO_MODULE_HEADERS or NO_SYNC_QT modules
        return()
    endif()

    get_target_property(is_private_module ${target} _qt_is_private_module)
    if(is_private_module)
        return()
    endif()

    get_target_property(is_internal_module ${target} _qt_is_internal_module)
    if(is_internal_module)
        return()
    endif()

    get_target_property(is_public_module ${target} _qt_is_public_module)
    if(NOT is_public_module)
        return()
    endif()

    qt_internal_collect_module_headers(module_headers ${target})

    qt_internal_module_info(module "${target}")

    set(found_non_utility_header FALSE)
    foreach(header IN LISTS module_headers_public)
        get_source_file_property(utility_header "${header}" _qt_utility_header)
        get_filename_component(header_name ${header} NAME)
        if("${header_name}" STREQUAL "qt${module_lower}global.h")
            # Treat as utility header
        elseif(NOT utility_header)
            set(found_non_utility_header TRUE)
            break()
        endif()
    endforeach()

    if(NOT found_non_utility_header)
        return()
    endif()

    string(JOIN "\n" module_map_content
        "$<$<BOOL:$<TARGET_PROPERTY:${target},FRAMEWORK>>:framework >module ${module} {"
        "    requires cplusplus17"
        "\n"
    )

    string(APPEND module_map_content "@SYNCQT_GENERATED_HEADER_LIST@")

    string(APPEND module_map_content "\n")

    qt_internal_collect_direct_target_dependencies(${target} target_deps)
    foreach(dep IN LISTS target_deps)
        if(dep MATCHES "^Qt::")
            string(REGEX REPLACE "Qt" "${QT_CMAKE_EXPORT_NAMESPACE}" dep ${dep})
        elseif(NOT dep MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::")
            continue()
        endif()
        if(dep MATCHES "Private$")
            continue()
        endif()
        if(NOT TARGET ${dep})
            continue()
        endif()

        get_target_property(is_private_module ${dep} _qt_is_private_module)
        if(is_private_module)
            continue()
        endif()
        get_target_property(is_internal_module ${dep} _qt_is_internal_module)
        if(is_internal_module OR dep MATCHES ".*Platform")
            continue()
        endif()

        qt_internal_module_info(dep_module "${dep}")
        string(APPEND module_map_content "    use ${dep_module}\n")
    endforeach()

    # Auto link to the required library/framework. Unfortunately the module map
    # can only have a single link line, so for static builds we're not able to
    # list all the transitive dependencies needed to use this module.
    get_target_property(is_header_module ${target} _qt_is_header_module)
    if(NOT is_header_module)
        string(APPEND module_map_content "\n"
            "    link "
                "$<$<BOOL:$<TARGET_PROPERTY:${target},FRAMEWORK>>:framework >"
                "\"$<TARGET_FILE_BASE_NAME:${target}>\"\n"
        )
    endif()

    string(APPEND module_map_content "\n"
        "    export *\n"
        "}\n")


    set(module_map_path "${module_clang_modules_dir}/module.modulemap")
    get_target_property(is_framework ${target} FRAMEWORK)
    if(NOT is_framework)
        qt_install(FILES ${module_map_path}
            DESTINATION "${module_install_interface_include_dir}")
    endif()

    set_target_properties(${target} PROPERTIES _qt_module_map_build_path "${module_map_path}")

    # Exported as a path relative to QT_BUILD_DIR so it stays valid for a relocated Qt.
    file(RELATIVE_PATH relative_module_map_path "${QT_BUILD_DIR}" "${module_map_path}")
    set_target_properties(${target} PROPERTIES _qt_module_map "${relative_module_map_path}")
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES "_qt_module_map")

    set(condition_args "")
    if(QT_MULTI_CONFIG_FIRST_CONFIG)
        list(APPEND condition_args CONDITION "$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>")
    endif()
    file(GENERATE
         OUTPUT "${module_modulemap_intermediate_path}"
         CONTENT "${module_map_content}"
         ${condition_args}
    )
endfunction()
