# This function can be used to add sources/libraries/etc. to the specified CMake target
# if the provided CONDITION evaluates to true.
function(qt_internal_extend_target target)
    # Don't try to extend_target when cross compiling an imported host target (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    if (NOT TARGET "${target}")
        message(FATAL_ERROR "Trying to extend non-existing target \"${target}\".")
    endif()
    qt_parse_all_arguments(arg "qt_extend_target" "HEADER_MODULE" "PRECOMPILED_HEADER"
        "CONDITION;${__default_public_args};${__default_private_args};${__default_private_module_args};COMPILE_FLAGS;NO_PCH_SOURCES" ${ARGN})
    if ("x${arg_CONDITION}" STREQUAL x)
        set(arg_CONDITION ON)
    endif()

    qt_evaluate_config_expression(result ${arg_CONDITION})
    if (${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_extend_target(${target} CONDITION ${arg_CONDITION} ...): Evaluated")
        endif()
        set(dbus_sources "")
        foreach(adaptor ${arg_DBUS_ADAPTOR_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${adaptor}" ADAPTOR BASENAME "${arg_DBUS_ADAPTOR_BASENAME}" FLAGS "${arg_DBUS_ADAPTOR_FLAGS}")
            list(APPEND dbus_sources "${adaptor}")
        endforeach()

        foreach(interface ${arg_DBUS_INTERFACE_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${interface}" INTERFACE BASENAME "${arg_DBUS_INTERFACE_BASENAME}" FLAGS "${arg_DBUS_INTERFACE_FLAGS}")
            list(APPEND dbus_sources "${interface}")
        endforeach()

        get_target_property(target_type ${target} TYPE)
        set(is_library FALSE)
        if (${target_type} STREQUAL "STATIC_LIBRARY" OR ${target_type} STREQUAL "SHARED_LIBRARY")
            set(is_library TRUE)
        endif()
        foreach(lib ${arg_PUBLIC_LIBRARIES} ${arg_LIBRARIES})
            # Automatically generate PCH for 'target' using public dependencies.
            # But only if 'target' is a library/module that does not specify its own PCH file.
            if(NOT arg_PRECOMPILED_HEADER AND ${is_library})
                qt_update_precompiled_header_with_library("${target}" "${lib}")
            endif()

            string(REGEX REPLACE "_nolink$" "" base_lib "${lib}")
            if(NOT base_lib STREQUAL lib)
                qt_create_nolink_target("${base_lib}" ${target})
            endif()
        endforeach()

        # Set-up the target

        # CMake versions less than 3.19 don't support adding the source files to the PRIVATE scope
        # of the INTERFACE libraries. These PRIVATE sources are only needed by IDEs to display
        # them in a project tree, so to avoid build issues and appearing the sources in
        # INTERFACE_SOURCES property of HEADER_MODULE let's simply exclude them for compatibility
        # with CMake versions less than 3.19.
        if(NOT arg_HEADER_MODULE OR CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            target_sources("${target}" PRIVATE ${arg_SOURCES} ${dbus_sources})
            if (arg_COMPILE_FLAGS)
                set_source_files_properties(${arg_SOURCES} PROPERTIES
                    COMPILE_FLAGS "${arg_COMPILE_FLAGS}")
            endif()
        endif()

        set(public_visibility_option "PUBLIC")
        set(private_visibility_option "PRIVATE")
        if(arg_HEADER_MODULE)
            set(public_visibility_option "INTERFACE")
            set(private_visibility_option "INTERFACE")
        endif()
        target_include_directories("${target}"
                                   ${public_visibility_option} ${arg_PUBLIC_INCLUDE_DIRECTORIES}
                                   ${private_visibility_option} ${arg_INCLUDE_DIRECTORIES})
        target_compile_definitions("${target}"
                                    ${public_visibility_option} ${arg_PUBLIC_DEFINES}
                                    ${private_visibility_option} ${arg_DEFINES})
        target_link_libraries("${target}"
                              ${public_visibility_option} ${arg_PUBLIC_LIBRARIES}
                              ${private_visibility_option} ${arg_LIBRARIES})
        target_compile_options("${target}"
                               ${public_visibility_option} ${arg_PUBLIC_COMPILE_OPTIONS}
                               ${private_visibility_option} ${arg_COMPILE_OPTIONS})
        target_link_options("${target}"
                            ${public_visibility_option} ${arg_PUBLIC_LINK_OPTIONS}
                            ${private_visibility_option} ${arg_LINK_OPTIONS})

        if(NOT arg_HEADER_MODULE)
            set_property (TARGET "${target}" APPEND PROPERTY
                AUTOMOC_MOC_OPTIONS "${arg_MOC_OPTIONS}"
            )
        endif()

        # When computing the private library dependencies, we need to check not only the known
        # modules added by this repo's qt_build_repo(), but also all module dependencies that
        # were found via find_package().
        qt_internal_get_qt_all_known_modules(known_modules)

        # When a public module depends on a private module (Gui on CorePrivate)
        # make its private module depend on the other private module (GuiPrivate will depend on
        # CorePrivate).
        set(qt_libs_private "")
        foreach(it ${known_modules})
            list(FIND arg_LIBRARIES "Qt::${it}Private" pos)
            if(pos GREATER -1)
                list(APPEND qt_libs_private "Qt::${it}Private")
            endif()
        endforeach()

        set(target_private "${target}Private")
        get_target_property(is_internal_module ${target} _qt_is_internal_module)
        # Internal modules don't have Private targets but we still need to
        # propagate their private dependencies.
        if(is_internal_module)
            set(target_private "${target}")
        endif()
        if(TARGET "${target_private}")
            target_link_libraries("${target_private}"
                                  INTERFACE ${arg_PRIVATE_MODULE_INTERFACE})
        elseif(arg_PRIVATE_MODULE_INTERFACE)
            set(warning_message "")
            string(APPEND warning_message
                "The PRIVATE_MODULE_INTERFACE option was provided the values:"
                "'${arg_PRIVATE_MODULE_INTERFACE}' "
                "but there is no ${target}Private target to assign them to."
                "Ensure the target exists or remove the option.")
            message(AUTHOR_WARNING "${warning_message}")
        endif()
        qt_register_target_dependencies("${target}"
                                        "${arg_PUBLIC_LIBRARIES};${arg_PRIVATE_MODULE_INTERFACE}"
                                        "${qt_libs_private};${arg_LIBRARIES}")


        qt_autogen_tools(${target}
                         ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
                         DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS})

        qt_update_precompiled_header("${target}" "${arg_PRECOMPILED_HEADER}")
        qt_update_ignore_pch_source("${target}" "${arg_NO_PCH_SOURCES}")
        ## Ignore objective-c files for PCH (not supported atm)
        qt_ignore_pch_obj_c_sources("${target}" "${arg_SOURCES}")

    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_extend_target(${target} CONDITION ${arg_CONDITION} ...): Skipped")
        endif()
    endif()
endfunction()

function(qt_is_imported_target target out_var)
    if(NOT TARGET "${target}")
        set(target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
    endif()
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Invalid target given to qt_is_imported_target: ${target}")
    endif()
    get_target_property(is_imported "${target}" IMPORTED)
    set(${out_var} "${is_imported}" PARENT_SCOPE)
endfunction()

# Add Qt::target and Qt6::target as aliases for the target
function(qt_internal_add_target_aliases target)
    set(versionless_alias "Qt::${target}")
    set(versionfull_alias "Qt${PROJECT_VERSION_MAJOR}::${target}")
    set_target_properties("${target}" PROPERTIES _qt_versionless_alias "${versionless_alias}")
    set_target_properties("${target}" PROPERTIES _qt_versionfull_alias "${versionfull_alias}")

    get_target_property(type "${target}" TYPE)
    if (type STREQUAL EXECUTABLE)
        add_executable("${versionless_alias}" ALIAS "${target}")
        add_executable("${versionfull_alias}" ALIAS "${target}")
    else()
        add_library("${versionless_alias}" ALIAS "${target}")
        add_library("${versionfull_alias}" ALIAS "${target}")
    endif()
endfunction()

function(qt_get_cmake_configurations out_var)
    set(possible_configs "${CMAKE_BUILD_TYPE}")
    if(CMAKE_CONFIGURATION_TYPES)
        set(possible_configs "${CMAKE_CONFIGURATION_TYPES}")
    endif()
    set(${out_var} "${possible_configs}" PARENT_SCOPE)
endfunction()

function(qt_clone_property_for_configs target property configs)
    get_target_property(value "${target}" "${property}")
    foreach(config ${configs})
        string(TOUPPER "${config}" upper_config)
        set_property(TARGET "${target}" PROPERTY "${property}_${upper_config}" "${value}")
    endforeach()
endfunction()

function(qt_handle_multi_config_output_dirs target)
    qt_get_cmake_configurations(possible_configs)
    qt_clone_property_for_configs(${target} LIBRARY_OUTPUT_DIRECTORY "${possible_configs}")
    qt_clone_property_for_configs(${target} RUNTIME_OUTPUT_DIRECTORY "${possible_configs}")
    qt_clone_property_for_configs(${target} ARCHIVE_OUTPUT_DIRECTORY "${possible_configs}")
endfunction()

# Set target properties that are the same for all modules, plugins, executables
# and 3rdparty libraries.
function(qt_set_common_target_properties target)
    if(QT_FEATURE_reduce_exports)
        set_target_properties(${target} PROPERTIES
            C_VISIBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            OBJC_VISIBILITY_PRESET hidden
            OBJCXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN 1)
    endif()
    qt_internal_set_compile_pdb_names("${target}")
endfunction()

# Set common, informational target properties.
#
# On Windows, these properties are used to generate the version information resource.
function(qt_set_target_info_properties target)
    cmake_parse_arguments(arg "" "${__default_target_info_args}" "" ${ARGN})
    if("${arg_TARGET_VERSION}" STREQUAL "")
        set(arg_TARGET_VERSION "${PROJECT_VERSION}.0")
    endif()
    if("${arg_TARGET_PRODUCT}" STREQUAL "")
        set(arg_TARGET_PRODUCT "Qt6")
    endif()
    if("${arg_TARGET_DESCRIPTION}" STREQUAL "")
        set(arg_TARGET_DESCRIPTION "C++ Application Development Framework")
    endif()
    if("${arg_TARGET_COMPANY}" STREQUAL "")
        set(arg_TARGET_COMPANY "The Qt Company Ltd.")
    endif()
    if("${arg_TARGET_COPYRIGHT}" STREQUAL "")
        set(arg_TARGET_COPYRIGHT "Copyright (C) 2022 The Qt Company Ltd.")
    endif()
    set_target_properties(${target} PROPERTIES
        QT_TARGET_VERSION "${arg_TARGET_VERSION}"
        QT_TARGET_COMPANY_NAME "${arg_TARGET_COMPANY}"
        QT_TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
        QT_TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
        QT_TARGET_PRODUCT_NAME "${arg_TARGET_PRODUCT}")
endfunction()

# Uses the QT_DELAYED_TARGET_* property values to set the final QT_TARGET_* properties.
# Needed when doing executable finalization at the end of a subdirectory scope
# (aka scope finalization).
function(qt_internal_set_target_info_properties_from_delayed_properties target)
    set(args "")
    foreach(prop ${__default_target_info_args})
        get_target_property(prop_value "${target}" "QT_DELAYED_${prop}")
        list(APPEND args "${prop}" "${prop_value}")
    endforeach()
    qt_set_target_info_properties(${target} ${args})
endfunction()

# Updates the QT_DELAYED_ properties with values from the QT_ variants, in case if they were
# set in-between a qt_add_* call and before scope finalization.
function(qt_internal_update_delayed_target_info_properties target)
    foreach(prop ${__default_target_info_args})
        get_target_property(prop_value "${target}" "QT_${prop}")
        get_target_property(delayed_prop_value ${target} "QT_DELAYED_${prop}")
        set(final_value "${delayed_prop_value}")
        if(prop_value)
            set(final_value "${prop_value}")
        endif()
        set_target_properties(${target} PROPERTIES "QT_DELAYED_${prop}" "${final_value}")
    endforeach()
endfunction()

function(qt_internal_check_directory_or_type name dir type default result_var)
    if ("x${dir}" STREQUAL x)
        if("x${type}" STREQUAL x)
            message(FATAL_ERROR
                "qt_internal_add_plugin called without setting either PLUGIN_TYPE or ${name}.")
        endif()
        set(${result_var} "${default}" PARENT_SCOPE)
    else()
        set(${result_var} "${dir}" PARENT_SCOPE)
    endif()
endfunction()

macro(qt_internal_get_export_additional_targets_keywords option_args single_args multi_args)
    set(${option_args}
    )
    set(${single_args}
        EXPORT_NAME_PREFIX
    )
    set(${multi_args}
        TARGETS
        TARGET_EXPORT_NAMES
    )
endmacro()

# Create a Qt*AdditionalTargetInfo.cmake file that is included by Qt*Config.cmake
# and sets IMPORTED_*_<CONFIG> properties on the exported targets.
#
# The file also makes the targets global if the QT_PROMOTE_TO_GLOBAL_TARGETS property is set in the
# consuming project.
# When using a CMake version lower than 3.21, only the specified TARGETS are made global.
# E.g. transitive non-Qt 3rd party targets of the specified targets are not made global.
#
# EXPORT_NAME_PREFIX:
#    The portion of the file name before AdditionalTargetInfo.cmake
# CONFIG_INSTALL_DIR:
#    Installation location for the target info file
# TARGETS:
#    The internal target names. Those must be actual targets.
# TARGET_EXPORT_NAMES:
#    The target names how they appear in the QtXXXTargets.cmake files.
#    The names get prefixed by ${QT_CMAKE_EXPORT_NAMESPACE}:: unless they already are.
#    This argument may be empty, then the target export names are the same as the internal ones.
#
# TARGETS and TARGET_EXPORT_NAMES must contain exactly the same number of elements.
# Example: TARGETS = qmljs_native
#          TARGET_EXPORT_NAMES = Qt6::qmljs
#
function(qt_internal_export_additional_targets_file)
    qt_internal_get_export_additional_targets_keywords(option_args single_args multi_args)
    cmake_parse_arguments(arg
        "${option_args}"
        "${single_args};CONFIG_INSTALL_DIR"
        "${multi_args}"
        ${ARGN})

    qt_internal_append_export_additional_targets()

    set_property(GLOBAL APPEND PROPERTY _qt_export_additional_targets_ids "${id}")
    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_targets_export_name_prefix_${id} "${arg_EXPORT_NAME_PREFIX}")
    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_targets_config_install_dir_${id} "${arg_CONFIG_INSTALL_DIR}")

    qt_add_list_file_finalizer(qt_internal_export_additional_targets_file_finalizer)
endfunction()

function(qt_internal_get_export_additional_targets_id export_name out_var)
    string(MAKE_C_IDENTIFIER "${export_name}" id)
    set(${out_var} "${id}" PARENT_SCOPE)
endfunction()

# Uses outer-scope variables to keep the implementation less verbose.
macro(qt_internal_append_export_additional_targets)
    qt_internal_validate_export_additional_targets(
        EXPORT_NAME_PREFIX "${arg_EXPORT_NAME_PREFIX}"
        TARGETS ${arg_TARGETS}
        TARGET_EXPORT_NAMES ${arg_TARGET_EXPORT_NAMES})

    qt_internal_get_export_additional_targets_id("${arg_EXPORT_NAME_PREFIX}" id)

    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_targets_${id} "${arg_TARGETS}")
    set_property(GLOBAL APPEND
        PROPERTY _qt_export_additional_target_export_names_${id} "${arg_TARGET_EXPORT_NAMES}")
endmacro()

# Can be called to add additional targets to the file after the initial setup call.
# Used for resources.
function(qt_internal_add_targets_to_additional_targets_export_file)
    qt_internal_get_export_additional_targets_keywords(option_args single_args multi_args)
    cmake_parse_arguments(arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
        ${ARGN})

    qt_internal_append_export_additional_targets()
endfunction()

function(qt_internal_validate_export_additional_targets)
    qt_internal_get_export_additional_targets_keywords(option_args single_args multi_args)
    cmake_parse_arguments(arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
        ${ARGN})

    if(NOT arg_EXPORT_NAME_PREFIX)
        message(FATAL_ERROR "qt_internal_validate_export_additional_targets: "
            "Missing EXPORT_NAME_PREFIX argument.")
    endif()

    list(LENGTH arg_TARGETS num_TARGETS)
    list(LENGTH arg_TARGET_EXPORT_NAMES num_TARGET_EXPORT_NAMES)
    if(num_TARGET_EXPORT_NAMES GREATER 0)
        if(NOT num_TARGETS EQUAL num_TARGET_EXPORT_NAMES)
            message(FATAL_ERROR "qt_internal_validate_export_additional_targets: "
                "TARGET_EXPORT_NAMES is set but has ${num_TARGET_EXPORT_NAMES} elements while "
                "TARGETS has ${num_TARGETS} elements. "
                "They must contain the same number of elements.")
        endif()
    else()
        set(arg_TARGET_EXPORT_NAMES ${arg_TARGETS})
    endif()

    set(arg_TARGETS "${arg_TARGETS}" PARENT_SCOPE)
    set(arg_TARGET_EXPORT_NAMES "${arg_TARGET_EXPORT_NAMES}" PARENT_SCOPE)
endfunction()

# The finalizer might be called multiple times in the same scope, but only the first one will
# process all the ids.
function(qt_internal_export_additional_targets_file_finalizer)
    get_property(ids GLOBAL PROPERTY _qt_export_additional_targets_ids)

    foreach(id ${ids})
        qt_internal_export_additional_targets_file_handler("${id}")
    endforeach()

    set_property(GLOBAL PROPERTY _qt_export_additional_targets_ids "")
endfunction()

function(qt_internal_export_additional_targets_file_handler id)
    get_property(arg_EXPORT_NAME_PREFIX GLOBAL PROPERTY
        _qt_export_additional_targets_export_name_prefix_${id})
    get_property(arg_CONFIG_INSTALL_DIR GLOBAL PROPERTY
        _qt_export_additional_targets_config_install_dir_${id})
    get_property(arg_TARGETS GLOBAL PROPERTY
        _qt_export_additional_targets_${id})
    get_property(arg_TARGET_EXPORT_NAMES GLOBAL PROPERTY
        _qt_export_additional_target_export_names_${id})

    list(LENGTH arg_TARGETS num_TARGETS)

    # Determine the release configurations we're currently building
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(active_configurations ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(active_configurations ${CMAKE_BUILD_TYPE})
    endif()
    unset(active_release_configurations)
    foreach(config ${active_configurations})
        string(TOUPPER ${config} ucconfig)
        if(NOT ucconfig STREQUAL "DEBUG")
            list(APPEND active_release_configurations ${config})
        endif()
    endforeach()

    if(active_release_configurations)
        # Use the first active release configuration as *the* release config for imported targets
        # and for QT_DEFAULT_IMPORT_CONFIGURATION.
        list(GET active_release_configurations 0 release_cfg)
        string(TOUPPER ${release_cfg} uc_release_cfg)
        set(uc_default_cfg ${uc_release_cfg})

        # Determine the release configurations we do *not* build currently
        set(configurations_to_export Release;RelWithDebInfo;MinSizeRel)
        list(REMOVE_ITEM configurations_to_export ${active_configurations})
    else()
        # There are no active release configurations.
        # Use the first active configuration for QT_DEFAULT_IMPORT_CONFIGURATION.
        unset(uc_release_cfg)
        list(GET active_configurations 0 default_cfg)
        string(TOUPPER ${default_cfg} uc_default_cfg)
        unset(configurations_to_export)
    endif()

    set(content "# Additional target information for ${arg_EXPORT_NAME_PREFIX}
if(NOT DEFINED QT_DEFAULT_IMPORT_CONFIGURATION)
    set(QT_DEFAULT_IMPORT_CONFIGURATION ${uc_default_cfg})
endif()
")

    math(EXPR n "${num_TARGETS} - 1")
    foreach(i RANGE ${n})
        list(GET arg_TARGETS ${i} target)
        list(GET arg_TARGET_EXPORT_NAMES ${i} target_export_name)

        set(full_target ${target_export_name})
        if(NOT full_target MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::")
            string(PREPEND full_target "${QT_CMAKE_EXPORT_NAMESPACE}::")
        endif()

        # Tools are already made global unconditionally in QtFooToolsConfig.cmake.
        # And the
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type STREQUAL "EXECUTABLE")
            string(APPEND content
                "__qt_internal_promote_target_to_global_checked(${full_target})\n")
        endif()

        # INTERFACE libraries don't have IMPORTED_LOCATION-like properties.
        # OBJECT libraries have properties like IMPORTED_OBJECTS instead.
        # Skip the rest of the procesing for those.
        if(target_type STREQUAL "INTERFACE_LIBRARY" OR target_type STREQUAL "OBJECT_LIBRARY")
            continue()
        endif()

        set(properties_retrieved TRUE)

        # Non-prefix debug-and-release builds: add check for the existence of the debug binary of
        # the target.  It is not built by default.
        if(NOT QT_WILL_INSTALL AND QT_FEATURE_debug_and_release)
            get_target_property(excluded_genex ${target} EXCLUDE_FROM_ALL)
            if(NOT excluded_genex STREQUAL "")
                string(APPEND content "
# ${full_target} is not built by default in the Debug configuration. Check existence.
get_target_property(_qt_imported_location ${full_target} IMPORTED_LOCATION_DEBUG)
if(NOT EXISTS \"$\\{_qt_imported_location}\")
    get_target_property(_qt_imported_configs ${full_target} IMPORTED_CONFIGURATIONS)
    list(REMOVE_ITEM _qt_imported_configs DEBUG)
    set_property(TARGET ${full_target} PROPERTY IMPORTED_CONFIGURATIONS $\\{_qt_imported_configs})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_LOCATION_DEBUG)
endif()\n\n")
            endif()
        endif()

        set(write_implib FALSE)
        set(write_soname FALSE)
        if(target_type STREQUAL "SHARED_LIBRARY")
            if(WIN32)
                set(write_implib TRUE)
            else()
                set(write_soname TRUE)
            endif()
        endif()

        if(NOT "${uc_release_cfg}" STREQUAL "")
            string(APPEND content "get_target_property(_qt_imported_location ${full_target} IMPORTED_LOCATION_${uc_release_cfg})\n")
            if(write_implib)
                string(APPEND content "get_target_property(_qt_imported_implib ${full_target} IMPORTED_IMPLIB_${uc_release_cfg})\n")
            endif()
            if(write_soname)
                string(APPEND content "get_target_property(_qt_imported_soname ${full_target} IMPORTED_SONAME_${uc_release_cfg})\n")
            endif()
        endif()
        string(APPEND content "get_target_property(_qt_imported_location_default ${full_target} IMPORTED_LOCATION_$\\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        if(write_implib)
            string(APPEND content "get_target_property(_qt_imported_implib_default ${full_target} IMPORTED_IMPLIB_$\\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        endif()
        if(write_soname)
            string(APPEND content "get_target_property(_qt_imported_soname_default ${full_target} IMPORTED_SONAME_$\\{QT_DEFAULT_IMPORT_CONFIGURATION})\n")
        endif()
        foreach(config ${configurations_to_export} "")
            string(TOUPPER "${config}" ucconfig)
            if("${config}" STREQUAL "")
                set(property_suffix "")
                set(var_suffix "_default")
                string(APPEND content "\n# Default configuration")
            else()
                set(property_suffix "_${ucconfig}")
                set(var_suffix "")
                string(APPEND content "
# Import target \"${full_target}\" for configuration \"${config}\"
set_property(TARGET ${full_target} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${ucconfig})
")
            endif()
            string(APPEND content "
if(_qt_imported_location${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_LOCATION${property_suffix} \"$\\{_qt_imported_location${var_suffix}}\")
endif()")
            if(write_implib)
                string(APPEND content "
if(_qt_imported_implib${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_IMPLIB${property_suffix} \"$\\{_qt_imported_implib${var_suffix}}\")
endif()")
            endif()
            if(write_soname)
                string(APPEND content "
if(_qt_imported_soname${var_suffix})
    set_property(TARGET ${full_target} PROPERTY IMPORTED_SONAME${property_suffix} \"$\\{_qt_imported_soname${var_suffix}}\")
endif()")
            endif()
            string(APPEND content "\n")
        endforeach()
    endforeach()

    if(properties_retrieved)
        string(APPEND content "
unset(_qt_imported_location)
unset(_qt_imported_location_default)
unset(_qt_imported_soname)
unset(_qt_imported_soname_default)
unset(_qt_imported_configs)")
    endif()

    qt_path_join(output_file "${arg_CONFIG_INSTALL_DIR}"
        "${arg_EXPORT_NAME_PREFIX}AdditionalTargetInfo.cmake")
    if(NOT IS_ABSOLUTE "${output_file}")
        qt_path_join(output_file "${QT_BUILD_DIR}" "${output_file}")
    endif()
    qt_configure_file(OUTPUT "${output_file}" CONTENT "${content}")
    qt_install(FILES "${output_file}" DESTINATION "${arg_CONFIG_INSTALL_DIR}")
endfunction()

function(qt_internal_export_modern_cmake_config_targets_file)
    cmake_parse_arguments(__arg "" "EXPORT_NAME_PREFIX;CONFIG_INSTALL_DIR" "TARGETS" ${ARGN})

    set(export_name "${__arg_EXPORT_NAME_PREFIX}VersionlessTargets")
    foreach(target ${__arg_TARGETS})
        if (TARGET "${target}Versionless")
            continue()
        endif()

        add_library("${target}Versionless" INTERFACE)
        target_link_libraries("${target}Versionless" INTERFACE "${target}")
        set_target_properties("${target}Versionless" PROPERTIES
            EXPORT_NAME "${target}"
            _qt_is_versionless_target "TRUE")
        set_property(TARGET "${target}Versionless"
                     APPEND PROPERTY EXPORT_PROPERTIES _qt_is_versionless_target)

        qt_install(TARGETS "${target}Versionless" EXPORT ${export_name})
    endforeach()
    qt_install(EXPORT ${export_name} NAMESPACE Qt:: DESTINATION "${__arg_CONFIG_INSTALL_DIR}")
endfunction()

function(qt_internal_create_tracepoints name tracepoints_file)
    string(TOLOWER "${name}" provider_name)
    string(PREPEND provider_name "qt")
    set(header_filename "${provider_name}_tracepoints_p.h")
    set(header_path "${CMAKE_CURRENT_BINARY_DIR}/${header_filename}")

    if(QT_FEATURE_lttng OR QT_FEATURE_etw)
        set(source_path "${CMAKE_CURRENT_BINARY_DIR}/${provider_name}_tracepoints.cpp")
        qt_configure_file(OUTPUT "${source_path}"
            CONTENT "#define TRACEPOINT_CREATE_PROBES
#define TRACEPOINT_DEFINE
#include \"${header_filename}\"")
        target_sources(${name} PRIVATE "${source_path}")
        target_compile_definitions(${name} PRIVATE Q_TRACEPOINT)

        if(QT_FEATURE_lttng)
            set(tracegen_arg "lttng")
            target_link_libraries(${name} PRIVATE LTTng::UST)
        elseif(QT_FEATURE_etw)
            set(tracegen_arg "etw")
        endif()

        if(NOT "${QT_HOST_PATH}" STREQUAL "")
            qt_path_join(tracegen
                "${QT_HOST_PATH}"
                "${QT${PROJECT_VERSION_MAJOR}_HOST_INFO_LIBEXECDIR}"
                "tracegen")
        else()
            set(tracegen "${QT_CMAKE_EXPORT_NAMESPACE}::tracegen")
        endif()

        get_filename_component(tracepoints_filepath "${tracepoints_file}" ABSOLUTE)
        add_custom_command(OUTPUT "${header_path}"
            COMMAND ${tracegen} ${tracegen_arg} "${tracepoints_filepath}" "${header_path}"
            VERBATIM)
        add_custom_target(${name}_tracepoints_header DEPENDS "${header_path}")
        add_dependencies(${name} ${name}_tracepoints_header)
    else()
        qt_configure_file(OUTPUT "${header_path}" CONTENT "#include <private/qtrace_p.h>\n")
    endif()
endfunction()

function(qt_internal_set_compile_pdb_names target)
    if(MSVC)
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "STATIC_LIBRARY" OR target_type STREQUAL "OBJECT_LIBRARY")
            set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME "${INSTALL_CMAKE_NAMESPACE}${target}")
            set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME_DEBUG "${INSTALL_CMAKE_NAMESPACE}${target}d")
        endif()
    endif()
endfunction()

# Installs pdb files for given target into the specified install dir.
#
# MSVC generates 2 types of pdb files:
#  - compile-time generated pdb files (compile flag /Zi + /Fd<pdb_name>)
#  - link-time genereated pdb files (link flag /debug + /PDB:<pdb_name>)
#
# CMake allows changing the names of each of those pdb file types by setting
# the COMPILE_PDB_NAME_<CONFIG> and PDB_NAME_<CONFIG> properties. If they are
# left empty, CMake will compute the default names itself (or rather in certain cases
# leave it up to te compiler), without actually setting the property values.
#
# For installation purposes, CMake only provides a generator expression to the
# link time pdb file path, not the compile path one, which means we have to compute the
# path to the compile path pdb files ourselves.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/18393 for details.
#
# For shared libraries and executables, we install the linker provided pdb file via the
# TARGET_PDB_FILE generator expression.
#
# For static libraries there is no linker invocation, so we need to install the compile
# time pdb file. We query the ARCHIVE_OUTPUT_DIRECTORY property of the target to get the
# path to the pdb file, and reconstruct the file name. We use a generator expression
# to append a possible debug suffix, in order to allow installation of all Release and
# Debug pdb files when using Ninja Multi-Config.
function(qt_internal_install_pdb_files target install_dir_path)
    if(MSVC)
        get_target_property(target_type ${target} TYPE)

        if(target_type STREQUAL "EXECUTABLE")
            qt_get_cmake_configurations(cmake_configs)
            list(LENGTH cmake_configs all_configs_count)
            list(GET cmake_configs 0 first_config)
            foreach(cmake_config ${cmake_configs})
                set(suffix "")
                if(all_configs_count GREATER 1 AND NOT cmake_config STREQUAL first_config)
                    set(suffix "/${cmake_config}")
                endif()
                qt_install(FILES "$<TARGET_PDB_FILE:${target}>"
                           CONFIGURATIONS ${cmake_config}
                           DESTINATION "${install_dir_path}${suffix}"
                           OPTIONAL)
            endforeach()

        elseif(target_type STREQUAL "SHARED_LIBRARY"
                OR target_type STREQUAL "MODULE_LIBRARY")
            qt_install(FILES "$<TARGET_PDB_FILE:${target}>"
                       DESTINATION "${install_dir_path}"
                       OPTIONAL)

        elseif(target_type STREQUAL "STATIC_LIBRARY")
            get_target_property(lib_dir "${target}" ARCHIVE_OUTPUT_DIRECTORY)
            if(NOT lib_dir)
                message(FATAL_ERROR
                        "Can't install pdb file for static library ${target}. "
                        "The ARCHIVE_OUTPUT_DIRECTORY path is not known.")
            endif()
            set(pdb_name "${INSTALL_CMAKE_NAMESPACE}${target}$<$<CONFIG:Debug>:d>.pdb")
            qt_path_join(compile_time_pdb_file_path "${lib_dir}" "${pdb_name}")

            qt_install(FILES "${compile_time_pdb_file_path}"
                       DESTINATION "${install_dir_path}" OPTIONAL)
        elseif(target_type STREQUAL "OBJECT_LIBRARY")
            get_target_property(pdb_dir "${target}" COMPILE_PDB_OUTPUT_DIRECTORY)
            if(NOT pdb_dir)
                get_target_property(pdb_dir "${target}" BINARY_DIR)
                if(QT_GENERATOR_IS_MULTI_CONFIG)
                    qt_path_join(pdb_dir "${pdb_dir}" "$<CONFIG>")
                endif()
            endif()
            set(pdb_name "${INSTALL_CMAKE_NAMESPACE}${target}$<$<CONFIG:Debug>:d>.pdb")
            qt_path_join(compile_time_pdb_file_path "${pdb_dir}" "${pdb_name}")

            qt_install(FILES "${compile_time_pdb_file_path}"
                DESTINATION "${install_dir_path}" OPTIONAL)
        endif()
    endif()
endfunction()

# Certain targets might have dependencies on libraries that don't have an Apple Silicon arm64
# slice. When doing a universal macOS build, force those targets to be built only for the
# Intel x86_64 arch.
# This behavior can be disabled for all targets by setting the QT_FORCE_MACOS_ALL_ARCHES cache
# variable to TRUE or by setting the target specific cache variable
# QT_FORCE_MACOS_ALL_ARCHES_${target} to TRUE.
#
# TODO: Ideally we'd use something like _apple_resolve_supported_archs_for_sdk_from_system_lib
# from CMake's codebase to parse which architectures are available in a library, but it's
# not straightforward to extract the library absolute file path from a CMake target. Furthermore
# Apple started using a built-in dynamic linker cache of all system-provided libraries as per
# https://gitlab.kitware.com/cmake/cmake/-/issues/20863
# so if the target is a library in the dynamic cache, that might further complicate how to get
# the list of arches in it.
function(qt_internal_force_macos_intel_arch target)
    if(MACOS AND QT_IS_MACOS_UNIVERSAL AND NOT QT_FORCE_MACOS_ALL_ARCHES
            AND NOT QT_FORCE_MACOS_ALL_ARCHES_${target})
        set(arches "x86_64")
        set_target_properties(${target} PROPERTIES OSX_ARCHITECTURES "${arches}")
    endif()
endfunction()

function(qt_disable_apple_app_extension_api_only target)
    set_target_properties("${target}" PROPERTIES QT_NO_APP_EXTENSION_ONLY_API TRUE)
endfunction()

# Common function to add Qt prefixes to the target name
function(qt_internal_qtfy_target out_var target)
    set(${out_var} "Qt${target}" PARENT_SCOPE)
    set(${out_var}_versioned "Qt${PROJECT_VERSION_MAJOR}${target}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_main_cmake_configuration out_var)
    if(CMAKE_BUILD_TYPE)
        set(config "${CMAKE_BUILD_TYPE}")
    elseif(QT_MULTI_CONFIG_FIRST_CONFIG)
        set(config "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()
    set("${out_var}" "${config}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_upper_case_main_cmake_configuration out_var)
    qt_internal_get_main_cmake_configuration("${out_var}")
    string(TOUPPER "${${out_var}}" upper_config)
    set("${out_var}" "${upper_config}" PARENT_SCOPE)
endfunction()

function(qt_internal_adjust_main_config_runtime_output_dir target output_dir)
    # When building Qt with multiple configurations, place the main configuration executable
    # directly in ${output_dir}, rather than a ${output_dir}/<CONFIG> subdirectory.
    qt_internal_get_upper_case_main_cmake_configuration(main_cmake_configuration)
    set_target_properties("${target}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${main_cmake_configuration} "${output_dir}"
    )
endfunction()

# Marks a target with a property that it is a library (shared or static) which was built using the
# internal Qt API (qt_internal_add_module, qt_internal_add_plugin, etc) as opposed to it being
# a user project library (qt_add_library, qt_add_plugin, etc).
#
# Needed to allow selectively applying certain flags via PlatformXInternal targets.
function(qt_internal_mark_as_internal_library target)
    set_target_properties(${target} PROPERTIES _qt_is_internal_library TRUE)
endfunction()

function(qt_internal_link_internal_platform_for_object_library target)
    # We need to apply iOS bitcode flags to object libraries that are associated with internal
    # modules or plugins (e.g. object libraries added by qt_internal_add_resource,
    # qt_internal_add_plugin, etc.)
    # The flags are needed when building iOS apps because Xcode expects bitcode to be
    # present by default.
    # Achieve this by compiling the cpp files with the PlatformModuleInternal compile flags.
    target_link_libraries("${target}" PRIVATE Qt::PlatformModuleInternal)
endfunction()

# Use ${dep_target}'s include dirs when building ${target}.
#
# Assumes ${dep_target} is an INTERFACE_LIBRARY that only propagates include dirs and ${target}
# is a Qt module / plugin.
#
# Building ${target} requires ${dep_target}'s include dirs.
# Using ${target} does not require ${dep_target}'s include dirs.
#
# The main use case is adding the private header-only dependency PkgConfig::ATSPI2.
function(qt_internal_add_target_include_dirs target dep_target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} is not a valid target.")
    endif()
    if(NOT TARGET "${dep_target}")
        message(FATAL_ERROR "${dep_target} is not a valid target.")
    endif()

    target_include_directories("${target}" PRIVATE
        "$<TARGET_PROPERTY:${dep_target},INTERFACE_INCLUDE_DIRECTORIES>")
endfunction()

# Use ${dep_target}'s include dirs when building ${target} and optionally propagate the include
# dirs to consumers of ${target}.

# Assumes ${dep_target} is an INTERFACE_LIBRARY that only propagates include dirs and ${target}
# is a Qt module / plugin.
#
# Building ${target} requires ${dep_target}'s include dirs.
#
# User projects that don't have ${dep_target}'s headers installed in their system should still
# configure successfully.
#
# To achieve that, consumers of ${target} will only get the include directories of ${dep_target}
# if the latter package and target exists.
#
# A find_package(dep_target) dependency is added to ${target}'s ModuleDependencies.cmake file.
#
# We use target_include_directories(PRIVATE) instead of target_link_libraries(PRIVATE) because the
# latter would propagate a mandatory LINK_ONLY dependency on the ${dep_target} in a static Qt build.
#
# The main use case is for propagating WrapVulkanHeaders::WrapVulkanHeaders.
function(qt_internal_add_target_include_dirs_and_optionally_propagate target dep_target)
    qt_internal_add_target_include_dirs(${target} ${dep_target})

    target_link_libraries("${target}" INTERFACE "$<TARGET_NAME_IF_EXISTS:${dep_target}>")

    qt_record_extra_third_party_dependency("${target}" "${dep_target}")
endfunction()

# The function disables one or multiple internal global definitions that are defined by the
# qt_internal_add_global_definition function for a specific 'target'.
function(qt_internal_undefine_global_definition target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} is not a target.")
    endif()

    if("${ARGN}" STREQUAL "")
        message(FATAL_ERROR "The function expects at least one definition as an argument.")
    endif()

    foreach(definition IN LISTS ARGN)
        set(undef_property_name "QT_INTERNAL_UNDEF_${definition}")
        set_target_properties(${target} PROPERTIES "${undef_property_name}" TRUE)
    endforeach()
endfunction()
