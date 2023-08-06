# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Note that these are only the keywords that are unique to qt_internal_add_plugin().
# That function also supports the keywords defined by _qt_internal_get_add_plugin_keywords().
macro(qt_internal_get_internal_add_plugin_keywords option_args single_args multi_args)
    set(${option_args}
        EXCEPTIONS
        ALLOW_UNDEFINED_SYMBOLS
        SKIP_INSTALL
        NO_UNITY_BUILD
    )
    set(${single_args}
        OUTPUT_DIRECTORY
        INSTALL_DIRECTORY
        ARCHIVE_INSTALL_DIRECTORY
        ${__default_target_info_args}
    )
    set(${multi_args}
        ${__default_private_args}
        ${__default_public_args}
        DEFAULT_IF
    )
endmacro()

# This is the main entry point for defining Qt plugins.
# A CMake target is created with the given target.
# The target name should end with "Plugin" so static plugins are linked automatically.
# The PLUGIN_TYPE parameter is needed to place the plugin into the correct plugins/ sub-directory.
function(qt_internal_add_plugin target)
    qt_internal_set_qt_known_plugins("${QT_KNOWN_PLUGINS}" "${target}")

    _qt_internal_get_add_plugin_keywords(
        public_option_args
        public_single_args
        public_multi_args
    )
    qt_internal_get_internal_add_plugin_keywords(
        internal_option_args
        internal_single_args
        internal_multi_args
    )
    set(option_args ${public_option_args} ${internal_option_args})
    set(single_args ${public_single_args} ${internal_single_args})
    set(multi_args  ${public_multi_args}  ${internal_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    # Put this behind a cache option for now. It's too noisy for general use
    # until most repos are updated.
    option(QT_WARN_PLUGIN_PUBLIC_KEYWORDS "Warn if a plugin specifies a PUBLIC keyword" ON)
    if(QT_WARN_PLUGIN_PUBLIC_KEYWORDS)
        foreach(publicKeyword IN LISTS __default_public_args)
            if(NOT "${arg_${publicKeyword}}" STREQUAL "")
                string(REPLACE "PUBLIC_" "" privateKeyword "${publicKeyword}")
                message(AUTHOR_WARNING
                    "Plugins are not intended to be linked to. "
                    "They should not have any public properties, but ${target} "
                    "sets ${publicKeyword} to the following value:\n"
                    "    ${arg_${publicKeyword}}\n"
                    "Update your project to use ${privateKeyword} instead.\n")
            endif()
        endforeach()
    endif()

    qt_remove_args(plugin_args
        ARGS_TO_REMOVE
            ${internal_option_args}
            ${internal_single_args}
            ${internal_multi_args}
        ALL_ARGS
            ${option_args}
            ${single_args}
            ${multi_args}
        ARGS
            ${ARGN}
    )

    # When creating a static plugin, retrieve the plugin initializer target name, but don't
    # automatically propagate the plugin initializer.
    list(APPEND plugin_args
        __QT_INTERNAL_NO_PROPAGATE_PLUGIN_INITIALIZER
        OUTPUT_TARGETS plugin_init_target
    )

    qt6_add_plugin(${target} ${plugin_args})
    qt_internal_mark_as_internal_library(${target})

    set(plugin_type "")
    # TODO: Transitional: Remove the TYPE option handling after all repos have been converted to use
    # PLUGIN_TYPE.
    if(arg_TYPE)
        set(plugin_type "${arg_TYPE}")
    elseif(arg_PLUGIN_TYPE)
        set(plugin_type "${arg_PLUGIN_TYPE}")
    endif()

    if((NOT plugin_type STREQUAL "qml_plugin") AND (NOT target MATCHES "(.*)Plugin$"))
        message(AUTHOR_WARNING "The internal plugin target name '${target}' should end with the 'Plugin' suffix.")
    endif()

    qt_get_sanitized_plugin_type("${plugin_type}" plugin_type_escaped)

    set(output_directory_default "${QT_BUILD_DIR}/${INSTALL_PLUGINSDIR}/${plugin_type}")
    set(install_directory_default "${INSTALL_PLUGINSDIR}/${plugin_type}")

    qt_internal_check_directory_or_type(OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" "${plugin_type}"
        "${output_directory_default}" output_directory)
    if (NOT arg_SKIP_INSTALL)
        qt_internal_check_directory_or_type(INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}"
            "${plugin_type}"
            "${install_directory_default}" install_directory)
        set(archive_install_directory ${arg_ARCHIVE_INSTALL_DIRECTORY})
        if (NOT archive_install_directory AND install_directory)
            set(archive_install_directory "${install_directory}")
        endif()
    endif()

    qt_set_target_info_properties(${target} ${ARGN})

    set_target_properties(${target} PROPERTIES
        _qt_package_version "${PROJECT_VERSION}"
    )
    set_property(TARGET ${target}
                 APPEND PROPERTY
                 EXPORT_PROPERTIES "_qt_package_version")

    # Override the OUTPUT_NAME that qt6_add_plugin() set, we need to account for
    # QT_LIBINFIX, which is specific to building Qt.
    # Make sure the Qt6 plugin library names are like they were in Qt5 qmake land.
    # Whereas the Qt6 CMake target names are like the Qt5 CMake target names.
    get_target_property(output_name ${target} OUTPUT_NAME)
    set_property(TARGET "${target}" PROPERTY OUTPUT_NAME "${output_name}${QT_LIBINFIX}")

    # Add a custom target with the Qt5 qmake name for a more user friendly ninja experience.
    if(arg_OUTPUT_NAME AND NOT TARGET "${output_name}")
        # But don't create such a target if it would just differ in case from "${target}"
        # and we're not using Ninja. See https://gitlab.kitware.com/cmake/cmake/-/issues/21915
        string(TOUPPER "${output_name}" uc_output_name)
        string(TOUPPER "${target}" uc_target)
        if(NOT uc_output_name STREQUAL uc_target OR CMAKE_GENERATOR MATCHES "^Ninja")
            add_custom_target("${output_name}")
            add_dependencies("${output_name}" "${target}")
        endif()
    endif()

    qt_set_common_target_properties("${target}")
    qt_internal_add_target_aliases("${target}")
    qt_skip_warnings_are_errors_when_repo_unclean("${target}")
    _qt_internal_apply_strict_cpp("${target}")

    set_target_properties("${target}" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${output_directory}"
        RUNTIME_OUTPUT_DIRECTORY "${output_directory}"
        ARCHIVE_OUTPUT_DIRECTORY "${output_directory}"
        QT_PLUGIN_TYPE "${plugin_type_escaped}"
        # Save the non-sanitized plugin type values for qmake consumption via .pri files.
        QT_QMAKE_PLUGIN_TYPE "${plugin_type}"
    )

    qt_handle_multi_config_output_dirs("${target}")

    qt_internal_library_deprecation_level(deprecation_define)

    qt_autogen_tools_initial_setup(${target})

    unset(plugin_install_package_suffix)

    # The generic plugins should be enabled by default.
    # But platform plugins should always be disabled by default, and only one is enabled
    # based on the platform (condition specified in arg_DEFAULT_IF).
    if(plugin_type_escaped STREQUAL "platforms")
        set(_default_plugin 0)
    else()
        set(_default_plugin 1)
    endif()

    if(DEFINED arg_DEFAULT_IF)
        if(${arg_DEFAULT_IF})
            set(_default_plugin 1)
        else()
            set(_default_plugin 0)
        endif()
    endif()

    # Save the Qt module in the plug-in's properties and vice versa
    if(NOT plugin_type_escaped STREQUAL "qml_plugin")
        qt_internal_get_module_for_plugin("${target}" "${plugin_type_escaped}" qt_module)

        set(qt_module_target "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_module}")
        if(NOT TARGET "${qt_module_target}")
            message(FATAL_ERROR "Failed to associate Qt plugin with Qt module. ${qt_module_target} is not a known CMake target")
        endif()

        set_target_properties("${target}" PROPERTIES QT_MODULE "${qt_module}")
        set(plugin_install_package_suffix "${qt_module}")


        get_target_property(aliased_target ${qt_module_target} ALIASED_TARGET)
        if(aliased_target)
            set(qt_module_target ${aliased_target})
        endif()
        get_target_property(is_imported_qt_module ${qt_module_target} IMPORTED)

        if(NOT is_imported_qt_module)
            # This QT_PLUGINS assignment is only used by QtPostProcessHelpers to decide if a
            # QtModulePlugins.cmake file should be generated.
            set_property(TARGET "${qt_module_target}" APPEND PROPERTY QT_PLUGINS "${target}")
        endif()

        set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
        get_target_property(type "${plugin_target_versioned}" TYPE)
        if(type STREQUAL STATIC_LIBRARY)
            # Associate plugin with its Qt module when both are both built in the same repository.
            # Check that by comparing the PROJECT_NAME of each.
            # This covers auto-linking of the majority of plugins to executables and in-tree tests.
            # Linking of plugins in standalone tests (when the Qt module will be an imported target)
            # is handled instead by the complicated genex logic in QtModulePlugins.cmake.in.
            set(is_plugin_and_module_in_same_project FALSE)
            if(NOT is_imported_qt_module)
                get_target_property(module_source_dir ${qt_module_target} SOURCE_DIR)
                get_directory_property(module_project_name
                    DIRECTORY ${module_source_dir}
                    DEFINITION PROJECT_NAME
                )
                if(module_project_name STREQUAL PROJECT_NAME)
                    set(is_plugin_and_module_in_same_project TRUE)
                endif()

                # When linking static plugins with the special logic in qt_internal_add_executable,
                # make sure to skip non-default plugins.
                if(is_plugin_and_module_in_same_project AND _default_plugin)
                    set_property(TARGET ${qt_module_target} APPEND PROPERTY
                                 _qt_initial_repo_plugins
                                 "${target}")
                    set_property(TARGET ${qt_module_target} APPEND PROPERTY
                                 _qt_initial_repo_plugin_class_names
                                 "$<TARGET_PROPERTY:${target},QT_PLUGIN_CLASS_NAME>"
                    )
                endif()
            endif()

            # Associate plugin with its Qt module when the plugin is built in the current repository
            # but the module is built in a different repository (qtsvg's QSvgPlugin associated with
            # qtbase's QtGui).
            # The association is done in a separate property, to ensure that reconfiguring in-tree tests
            # in qtbase doesn't accidentally cause linking to a plugin from a previously built qtsvg.
            # Needed for in-tree tests like in qtsvg, qtimageformats.
            # This is done for each Qt module regardless if it's an imported target or not, to handle
            # both per-repo and top-level builds (in per-repo build of qtsvg QtGui is imported, in a
            # top-level build Gui is not imported, but in both cases qtsvg tests need to link to
            # QSvgPlugin).
            #
            # TODO: Top-level in-tree tests and qdeclarative per-repo in-tree tests that depend on
            #       static Qml plugins won't work due to the requirement of running qmlimportscanner
            #       at configure time, but qmlimportscanner is not built at that point. Moving the
            #       execution of qmlimportscanner to build time is non-trivial because qmlimportscanner
            #       not only generates a cpp file to compile but also outputs a list of static plugins
            #       that should be linked and there is no straightforward way to tell CMake to link
            #       against a list of libraries that was discovered at build time (apart from
            #       response files, which apparently might not work on all platforms).
            #       qmake doesn't have this problem because each project is configured separately so
            #       qmlimportscanner is always built by the time it needs to be run for a test.
            if(NOT is_plugin_and_module_in_same_project AND _default_plugin)
                string(MAKE_C_IDENTIFIER "${PROJECT_NAME}" current_project_name)
                set(prop_prefix "_qt_repo_${current_project_name}")
                set_property(TARGET ${qt_module_target} APPEND PROPERTY
                             ${prop_prefix}_plugins "${target}")
                set_property(TARGET ${qt_module_target} APPEND PROPERTY
                             ${prop_prefix}_plugin_class_names
                    "$<TARGET_PROPERTY:${target},QT_PLUGIN_CLASS_NAME>"
                )
            endif()
        endif()

        qt_internal_add_autogen_sync_header_dependencies(${target} ${qt_module_target})
    endif()

    # Change the configuration file install location for qml plugins into the Qml package location.
    if(plugin_type_escaped STREQUAL "qml_plugin" AND TARGET "${INSTALL_CMAKE_NAMESPACE}::Qml")
        set(plugin_install_package_suffix "Qml/QmlPlugins")
    endif()

    # Save the install package suffix as a property, so that the Dependencies file is placed
    # in the correct location.
    if(plugin_install_package_suffix)
        set_target_properties("${target}" PROPERTIES
                              _qt_plugin_install_package_suffix "${plugin_install_package_suffix}")
    endif()

    if(TARGET qt_plugins)
        add_dependencies(qt_plugins "${target}")
    endif()

    # Record plugin for current repo.
    if(qt_repo_plugins AND TARGET ${qt_repo_plugins})
        add_dependencies(${qt_repo_plugins} "${target}")
    endif()

    if(plugin_type STREQUAL "platforms")
        if(TARGET qpa_plugins)
            add_dependencies(qpa_plugins "${target}")
        endif()

        if(_default_plugin AND TARGET qpa_default_plugins)
            add_dependencies(qpa_default_plugins "${target}")
        endif()
    endif()

    set_property(TARGET "${target}" PROPERTY QT_DEFAULT_PLUGIN "${_default_plugin}")
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES "QT_PLUGIN_CLASS_NAME;QT_PLUGIN_TYPE;QT_MODULE;QT_DEFAULT_PLUGIN")

    set(private_includes
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
         # For the syncqt headers
        "$<BUILD_INTERFACE:${QT_BUILD_DIR}/include>"
         ${arg_INCLUDE_DIRECTORIES}
    )

    set(public_includes
        ${arg_PUBLIC_INCLUDE_DIRECTORIES}
    )

    if(arg_NO_UNITY_BUILD)
        set(arg_NO_UNITY_BUILD "NO_UNITY_BUILD")
    else()
        set(arg_NO_UNITY_BUILD "")
    endif()

    qt_internal_extend_target("${target}"
        ${arg_NO_UNITY_BUILD}
        SOURCES ${arg_SOURCES}
        NO_PCH_SOURCES
            ${arg_NO_PCH_SOURCES}
        NO_UNITY_BUILD_SOURCES
            ${arg_NO_UNITY_BUILD_SOURCES}
        INCLUDE_DIRECTORIES
            ${private_includes}
        SYSTEM_INCLUDE_DIRECTORIES
            ${arg_SYSTEM_INCLUDE_DIRECTORIES}
        PUBLIC_INCLUDE_DIRECTORIES
            ${public_includes}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformPluginInternal
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        DEFINES
            ${arg_DEFINES}
            ${deprecation_define}
        PUBLIC_DEFINES
            ${arg_PUBLIC_DEFINES}
        FEATURE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        DBUS_ADAPTOR_SOURCES ${arg_DBUS_ADAPTOR_SOURCES}
        DBUS_ADAPTOR_FLAGS ${arg_DBUS_ADAPTOR_FLAGS}
        DBUS_INTERFACE_SOURCES ${arg_DBUS_INTERFACE_SOURCES}
        DBUS_INTERFACE_FLAGS ${arg_DBUS_INTERFACE_FLAGS}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )

    qt_internal_add_repo_local_defines("${target}")

    qt_internal_set_exceptions_flags("${target}" ${arg_EXCEPTIONS})


    set(qt_libs_private "")
    qt_internal_get_qt_all_known_modules(known_modules)
    foreach(it ${known_modules})
        list(FIND arg_LIBRARIES "Qt::${it}Private" pos)
        if(pos GREATER -1)
            list(APPEND qt_libs_private "Qt::${it}Private")
        endif()
    endforeach()

    qt_register_target_dependencies("${target}" "${arg_PUBLIC_LIBRARIES}" "${qt_libs_private}")

    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL STATIC_LIBRARY)
        if(qt_module_target)
            qt_internal_link_internal_platform_for_object_library("${plugin_init_target}")
        endif()
    endif()

    if (NOT arg_SKIP_INSTALL)
        # Handle creation of cmake files for consumers of find_package().
        # If we are part of a Qt module, the plugin cmake files are installed as part of that
        # module.
        # For qml plugins, they are all installed into the QtQml package location for automatic
        # discovery.
        if(plugin_install_package_suffix)
            set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${plugin_install_package_suffix}")
        else()
            set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
        endif()

        qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
        qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

        qt_internal_export_additional_targets_file(
            TARGETS ${target} ${plugin_init_target}
            EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
            CONFIG_INSTALL_DIR "${config_install_dir}")

        qt_internal_get_min_new_policy_cmake_version(min_new_policy_version)
        qt_internal_get_max_new_policy_cmake_version(max_new_policy_version)
        configure_package_config_file(
            "${QT_CMAKE_DIR}/QtPluginConfig.cmake.in"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
            INSTALL_DESTINATION "${config_install_dir}"
        )
        write_basic_package_version_file(
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
        )
        qt_internal_write_qt_package_version_file(
            "${INSTALL_CMAKE_NAMESPACE}${target}"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        )

        qt_install(FILES
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
            DESTINATION "${config_install_dir}"
            COMPONENT Devel
        )

        # Make the export name of plugins be consistent with modules, so that
        # qt_add_resource adds its additional targets to the same export set in a static Qt build.
        set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")
        qt_install(TARGETS
                   "${target}"
                   ${plugin_init_target}
                   EXPORT ${export_name}
                   RUNTIME DESTINATION "${install_directory}"
                   LIBRARY DESTINATION "${install_directory}"
                   OBJECTS DESTINATION "${install_directory}"
                   ARCHIVE DESTINATION "${archive_install_directory}"
        )
        qt_install(EXPORT ${export_name}
                   NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}::
                   DESTINATION "${config_install_dir}"
        )
        if(BUILD_SHARED_LIBS)
            qt_apply_rpaths(TARGET "${target}" INSTALL_PATH "${install_directory}" RELATIVE_RPATH)
            qt_internal_apply_staging_prefix_build_rpath_workaround()
        endif()
    endif()

    if (NOT arg_ALLOW_UNDEFINED_SYMBOLS)
        ### fixme: cmake is missing a built-in variable for this. We want to apply it only to
        # modules and plugins that belong to Qt.
        qt_internal_add_link_flags_no_undefined("${target}")
    endif()

    qt_internal_add_linker_version_script(${target})
    set(finalizer_extra_args "")
    if(NOT arg_SKIP_INSTALL)
        list(APPEND finalizer_extra_args INSTALL_PATH "${install_directory}")
    endif()
    qt_add_list_file_finalizer(qt_finalize_plugin ${target} ${finalizer_extra_args})

    if(NOT arg_SKIP_INSTALL)
        qt_enable_separate_debug_info(${target} "${install_directory}")
        qt_internal_install_pdb_files(${target} "${install_directory}")
    endif()
endfunction()

function(qt_finalize_plugin target)
    cmake_parse_arguments(arg "" "INSTALL_PATH" "" ${ARGN})
    if(WIN32 AND BUILD_SHARED_LIBS)
        _qt_internal_generate_win32_rc_file("${target}")
    endif()

    # Generate .prl and .pri files for static plugins.
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL STATIC_LIBRARY)
        if(arg_INSTALL_PATH)
            qt_generate_prl_file(${target} "${arg_INSTALL_PATH}")
        endif()

        # There's no point in generating pri files for qml plugins.
        # We didn't do it in Qt5 times.
        get_target_property(plugin_type "${target}" QT_PLUGIN_TYPE)
        if(NOT plugin_type STREQUAL "qml_plugin")
            qt_generate_plugin_pri_file("${target}")
        endif()
    endif()
endfunction()

function(qt_get_sanitized_plugin_type plugin_type out_var)
    # Used to handle some edge cases such as platforms/darwin
    string(REGEX REPLACE "[-/]" "_" plugin_type "${plugin_type}")
    set("${out_var}" "${plugin_type}" PARENT_SCOPE)
endfunction()

# Utility function to find the module to which a plug-in belongs.
function(qt_internal_get_module_for_plugin target target_type out_var)
    qt_internal_get_qt_all_known_modules(known_modules)

    qt_get_sanitized_plugin_type("${target_type}" target_type)
    foreach(qt_module ${known_modules})
        get_target_property(module_type "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_module}" TYPE)
        # Assuming interface libraries can't have plugins. Otherwise we'll need to fix the property
        # name, because the current one would be invalid for interface libraries.
        if(module_type STREQUAL "INTERFACE_LIBRARY")
            continue()
        endif()

        get_target_property(plugin_types
                           "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_module}"
                            MODULE_PLUGIN_TYPES)
        if(plugin_types AND target_type IN_LIST plugin_types)
            set("${out_var}" "${qt_module}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(FATAL_ERROR "The plug-in '${target}' does not belong to any Qt module.")
endfunction()

function(qt_internal_add_darwin_permission_plugin permission)
    string(TOLOWER "${permission}" permission_lower)
    string(TOUPPER "${permission}" permission_upper)
    set(permission_source_file "platform/darwin/qdarwinpermissionplugin_${permission_lower}.mm")
    set(plugin_target "QDarwin${permission}PermissionPlugin")
    set(plugin_name "qdarwin${permission_lower}permission")
    qt_internal_add_plugin(${plugin_target}
        STATIC # Force static, even in shared builds
        OUTPUT_NAME ${plugin_name}
        PLUGIN_TYPE permissions
        DEFAULT_IF FALSE
        SOURCES
            ${permission_source_file}
        DEFINES
            QT_DARWIN_PERMISSION_PLUGIN=${permission}
        LIBRARIES
            Qt::Core
            Qt::CorePrivate
            ${FWFoundation}
    )

    # Disable PCH since CMake falls over on single .mm source targets
    set_target_properties(${plugin_target} PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
    )

    # Generate plugin JSON file
    set(content "{ \"Permissions\": [ \"Q${permission}Permission\" ] }")
    get_target_property(plugin_build_dir "${plugin_target}" BINARY_DIR)
    set(output_file "${plugin_build_dir}/${plugin_target}.json")
    qt_configure_file(OUTPUT "${output_file}" CONTENT "${content}")

    # Associate required usage descriptions
    set(usage_descriptions_property "_qt_info_plist_usage_descriptions")
    set_target_properties(${plugin_target} PROPERTIES
        ${usage_descriptions_property} "NS${permission}UsageDescription"
    )
    set_property(TARGET ${plugin_target} APPEND PROPERTY
        EXPORT_PROPERTIES ${usage_descriptions_property}
    )
    set(usage_descriptions_genex "$<JOIN:$<TARGET_PROPERTY:${plugin_target},${usage_descriptions_property}>, >")
    set(extra_plugin_pri_content
        "QT_PLUGIN.${plugin_name}.usage_descriptions = ${usage_descriptions_genex}"
    )

    # Support granular check and request implementations
    set(separate_request_source_file
        "${plugin_build_dir}/qdarwinpermissionplugin_${permission_lower}_request.mm")
    set(separate_request_genex
        "$<BOOL:$<TARGET_PROPERTY:${plugin_target},_qt_darwin_permissison_separate_request>>")
    file(GENERATE OUTPUT "${separate_request_source_file}" CONTENT
        "
        #define BUILDING_PERMISSION_REQUEST 1
        #include \"${CMAKE_CURRENT_SOURCE_DIR}/${permission_source_file}\"
        "
        CONDITION "${separate_request_genex}"
    )
    if(CMAKE_VERSION VERSION_LESS "3.18")
        set_property(SOURCE "${separate_request_source_file}" PROPERTY GENERATED TRUE)
    endif()
    target_sources(${plugin_target} PRIVATE
        "$<${separate_request_genex}:${separate_request_source_file}>"
    )
    set_property(TARGET ${plugin_target} APPEND PROPERTY
        EXPORT_PROPERTIES _qt_darwin_permissison_separate_request
    )
    set(permission_request_symbol "_QDarwin${permission}PermissionRequest")
    set(permission_request_flag "-Wl,-u,${permission_request_symbol}")
    set(has_usage_description_property "_qt_has_${plugin_target}_usage_description")
    set(has_usage_description_genex "$<BOOL:$<TARGET_PROPERTY:${has_usage_description_property}>>")
    target_link_options(${plugin_target} INTERFACE
        "$<$<AND:${separate_request_genex},${has_usage_description_genex}>:${permission_request_flag}>")
     list(APPEND extra_plugin_pri_content
        "QT_PLUGIN.${plugin_name}.request_flag = $<${separate_request_genex}:${permission_request_flag}>"
    )

    # Expose properties to qmake
    set_property(TARGET ${plugin_target} PROPERTY
        QT_PLUGIN_PRI_EXTRA_CONTENT ${extra_plugin_pri_content}
    )
endfunction()
