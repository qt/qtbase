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

        # Not every Qt target we link is a Qt module. Static builds in particular
        # may pull in plugins, e.g. via qt_import_qml_plugins(), and those don't
        # have module maps of their own to use.
        get_target_property(dep_interface_name ${dep} _qt_module_interface_name)
        if(NOT dep_interface_name)
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
    set(module_map_disable_path "${module_map_path}.disable.yaml")

    # Clang reaches a framework's module map through the symlink at the bundle
    # root, so that's the spelling to hand out, not the one we write to.
    set(module_map_lookup_path "${module_clang_modules_lookup_dir}/module.modulemap")

    qt_internal_generate_module_map_disable_overlay("${target}"
        "${module_map_disable_path}" "${module_map_lookup_path}.disable.yaml")

    get_target_property(is_framework ${target} FRAMEWORK)
    if(NOT is_framework)
        qt_install(FILES ${module_map_path} ${module_map_disable_path}
            DESTINATION "${module_install_interface_include_dir}")
    endif()

    set_target_properties(${target} PROPERTIES _qt_module_map_build_path "${module_map_path}")

    # Exported as a path relative to QT_BUILD_DIR so it stays valid for a relocated Qt.
    file(RELATIVE_PATH relative_module_map_path "${QT_BUILD_DIR}" "${module_map_lookup_path}")
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

# Generates a Clang VFS overlay next to the module's Clang module map, masking
# the module map by redirecting it to an empty file.
#
# The module maps are generated and shipped to support Swift consumers. Normal
# C++ consumers won't pick them up automatically, unless they build with explicit
# support for C++ Clang modules via -fmodules -fcxx-modules. While our module
# maps should work fine for C++ usage too, we want to avoid any risk of regression
# for C++ consumers, so if we detect C++ Clang modules enablement we automatically
# pass the overlay during compilation via -ivfsoverlay, making the includes textual
# again, without affecting the module maps of any other library, or those of the
# platform SDK.
#
# The overlay is propagated to consumers that enable QT_NO_CLANG_MODULE_MAPS,
# which _qt_internal_finalize_clang_module_maps does automatically on detecting
# that the consuming target builds with C++ Clang modules.
function(qt_internal_generate_module_map_disable_overlay target overlay_path lookup_path)
    # Redirecting to a file that doesn't exist makes Clang report the module map
    # as missing, which is explicitly supported, and is what we want. Redirecting
    # to an empty file would still leave a module map for Clang to load. Both the
    # redirected and the redirected-to path are relative to the overlay itself,
    # which keeps the overlay valid for a relocated Qt.
    string(JOIN "\n" overlay_content
        "{"
        "  'version': 0,"
        "  'root-relative': 'overlay-dir',"
        "  'overlay-relative': true,"
        "  'roots': ["
        "    { 'name': 'module.modulemap', 'type': 'file',"
        "      'external-contents': 'nonexistent.modulemap' }"
        "  ]"
        "}"
        ""
    )

    qt_configure_file(OUTPUT "${overlay_path}" CONTENT "${overlay_content}")

    # Clang matches the masked module map by string, against the path it looked
    # the module map up by, and 'root-relative: overlay-dir' resolves it against
    # the overlay's own directory as spelled on the command line. So the overlay
    # has to be passed by its lookup path for the masking to take effect.
    #
    # The overlay sits at the same location relative to the build dir as it does
    # relative to the install prefix, for both framework and non-framework builds.
    file(RELATIVE_PATH relative_overlay_path "${QT_BUILD_DIR}" "${lookup_path}")

    set(is_disabled "$<BOOL:$<TARGET_PROPERTY:QT_NO_CLANG_MODULE_MAPS>>")
    target_compile_options(${target} INTERFACE
        "$<BUILD_INTERFACE:$<${is_disabled}:-ivfsoverlay${lookup_path}>>"
        "$<INSTALL_INTERFACE:$<${is_disabled}:-ivfsoverlay$<INSTALL_PREFIX>/${relative_overlay_path}>>"
    )
endfunction()
