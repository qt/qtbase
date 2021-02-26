# This function is used to define a "Qt tool", such as moc, uic or rcc.
# The BOOTSTRAP option allows building it as standalone program, otherwise
# it will be linked against QtCore.
#
# USER_FACING can be passed to mark the tool as a program that is supposed to be
# started directly by users.
#
# We must pass this function a target name obtained from
# qt_get_tool_target_name like this:
#     qt_get_tool_target_name(target_name my_tool)
#     qt_internal_add_tool(${target_name})
#
# Option Arguments:
#     INSTALL_VERSIONED_LINK
#         Prefix build only. On installation, create a versioned hard-link of the installed file.
#         E.g. create a link of "bin/qmake6" to "bin/qmake".
#
# One-value Arguments:
#     EXTRA_CMAKE_FILES
#         List of additional CMake files that will be installed alongside the tool's exported CMake
#         files.
#     INSTALL_DIR
#         Takes a path, relative to the install prefix, like INSTALL_LIBEXECDIR.
#         If this argument is omitted, the default is INSTALL_BINDIR.
#     TOOLS_TARGET
#         Specifies the module this tool belongs to. The module's Qt6${module}Tools.cmake file
#         will then contain targets for this tool.
#
function(qt_internal_add_tool target_name)
    qt_tool_target_to_name(name ${target_name})
    set(option_keywords BOOTSTRAP NO_QT NO_INSTALL USER_FACING INSTALL_VERSIONED_LINK)
    set(one_value_keywords TOOLS_TARGET EXTRA_CMAKE_FILES INSTALL_DIR
                           ${__default_target_info_args})
    qt_parse_all_arguments(arg "qt_internal_add_tool" "${option_keywords}"
                               "${one_value_keywords}"
                               "${__default_private_args}" ${ARGN})

    # Handle case when a tool does not belong to a module and it can't be built either (like
    # during a cross-compile).
    if(NOT arg_TOOLS_TARGET AND NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${name}\" has not been assigned to a module via"
                            " TOOLS_TARGET (so it can't be found) and it can't be built"
                            " (QT_WILL_BUILD_TOOLS is ${QT_WILL_BUILD_TOOLS}).")
    endif()

    if(CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING AND (name STREQUAL target_name))
        message(FATAL_ERROR
            "qt_internal_add_tool must be passed a target obtained from qt_get_tool_target_name.")
    endif()

    set(full_name "${QT_CMAKE_EXPORT_NAMESPACE}::${name}")
    set(imported_tool_target_found FALSE)
    if(TARGET ${full_name})
        get_property(path TARGET ${full_name} PROPERTY LOCATION)
        message(STATUS "Tool '${full_name}' was found at ${path}.")
        set(imported_tool_target_found TRUE)
        if(CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            return()
        endif()
    endif()

    if(arg_TOOLS_TARGET AND (NOT QT_WILL_BUILD_TOOLS OR QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            AND NOT imported_tool_target_found)
        set(tools_package_name "Qt6${arg_TOOLS_TARGET}Tools")
        message(STATUS "Searching for tool '${full_name}' in package ${tools_package_name}.")

        # Create the tool targets, even if QT_NO_CREATE_TARGETS is set.
        # Otherwise targets like Qt6::moc are not available in a top-level cross-build.
        set(BACKUP_QT_NO_CREATE_TARGETS ${QT_NO_CREATE_TARGETS})
        set(QT_NO_CREATE_TARGETS OFF)

        # Only search in path provided by QT_HOST_PATH. We need to do it with CMAKE_PREFIX_PATH
        # instead of PATHS option, because any find_dependency call inside a Tools package would
        # not get the proper prefix when using PATHS.
        set(BACKUP_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
        set(CMAKE_PREFIX_PATH "${QT_HOST_PATH}")

        # Search both with sysroots prepended as well as in the host system. When cross compiling
        # the mode_package might be set to ONLY only, and the Qt6 tools packages are actually
        # in the host system.
        set(BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "BOTH")
        find_package(
            ${tools_package_name}
            ${PROJECT_VERSION}
            NO_PACKAGE_ROOT_PATH
            NO_CMAKE_ENVIRONMENT_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_PACKAGE_REGISTRY
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE}")
        set(CMAKE_PREFIX_PATH "${BACKUP_CMAKE_PREFIX_PATH}")
        set(QT_NO_CREATE_TARGETS ${BACKUP_QT_NO_CREATE_TARGETS})

        if(${${tools_package_name}_FOUND} AND TARGET ${full_name})
            # Even if the tool is already visible, make sure that our modules remain associated
            # with the tools.
            qt_internal_append_known_modules_with_tools("${arg_TOOLS_TARGET}")
            get_property(path TARGET ${full_name} PROPERTY LOCATION)
            message(STATUS "${full_name} was found at ${path} using package ${tools_package_name}.")
            if (NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
                return()
            endif()
        endif()
    endif()

    if(NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${full_name}\" was not found in the "
                           "${tools_package_name} package. "
                           "Package found: ${${tools_package_name}_FOUND}")
    else()
        if(QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            message(STATUS "Tool '${target_name}' will be cross-built from source.")
        else()
            message(STATUS "Tool '${full_name}' will be built from source.")
        endif()
    endif()

    set(disable_autogen_tools "${arg_DISABLE_AUTOGEN_TOOLS}")
    if (arg_NO_QT)
        # FIXME: Remove NO_QT again once qmake can use a "normal" Qt!
        if (arg_BOOTSTRAP)
            message(FATAL_ERROR "Tool can not be NO_QT and BOOTSTRAP at the same time!")
        endif()
        set(corelib "")
    else()
        if (arg_BOOTSTRAP)
            set(corelib ${QT_CMAKE_EXPORT_NAMESPACE}::Bootstrap)
            list(APPEND disable_autogen_tools "uic" "moc" "rcc")
        else()
            set(corelib ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
        endif()
    endif()

    set(bootstrap "")
    if(arg_BOOTSTRAP)
        set(bootstrap BOOTSTRAP)
    endif()

    set(no_qt "")
    if(arg_NO_QT)
        set(no_qt NO_QT)
    endif()

    set(install_dir "${INSTALL_BINDIR}")
    if(arg_INSTALL_DIR)
        set(install_dir "${arg_INSTALL_DIR}")
    endif()

    qt_internal_add_executable("${target_name}" OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${install_dir}"
        ${bootstrap}
        ${no_qt}
        NO_INSTALL
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES
            QT_USE_QSTRINGBUILDER
            ${arg_DEFINES}
        PUBLIC_LIBRARIES ${corelib}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformToolInternal
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        DISABLE_AUTOGEN_TOOLS ${disable_autogen_tools}
        TARGET_VERSION "${arg_TARGET_VERSION}"
        TARGET_PRODUCT "${arg_TARGET_PRODUCT}"
        TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
        TARGET_COMPANY "${arg_TARGET_COMPANY}"
        TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
    )
    qt_internal_add_target_aliases("${target_name}")
    _qt_internal_apply_strict_cpp("${target_name}")

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19.0" AND QT_FEATURE_debug_and_release)
        set_property(TARGET "${target_name}"
            PROPERTY EXCLUDE_FROM_ALL "$<NOT:$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>>")
    endif()

    if (NOT target_name STREQUAL name)
        set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME ${name}
            EXPORT_NAME ${name}
        )
    endif()

    if(TARGET host_tools)
        add_dependencies(host_tools "${target_name}")
        if(bootstrap OR no_qt)
            add_dependencies(bootstrap_tools "${target_name}")
        endif()
    endif()

    if(arg_EXTRA_CMAKE_FILES)
        set_target_properties(${target_name} PROPERTIES
            EXTRA_CMAKE_FILES "${arg_EXTRA_CMAKE_FILES}"
        )
    endif()

    if(arg_USER_FACING)
        set_property(GLOBAL APPEND PROPERTY QT_USER_FACING_TOOL_TARGETS ${target_name})
    endif()

    # If building with a multi-config configuration, the main configuration tool will be placed in
    # ./bin, while the rest will be in <CONFIG> specific subdirectories.
    qt_get_tool_cmake_configuration(tool_cmake_configuration)
    set_target_properties("${target_name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${tool_cmake_configuration} "${QT_BUILD_DIR}/${install_dir}"
    )

    if(NOT arg_NO_INSTALL AND arg_TOOLS_TARGET)
        # Assign a tool to an export set, and mark the module to which the tool belongs.
        qt_internal_append_known_modules_with_tools("${arg_TOOLS_TARGET}")

        # Also append the tool to the module list.
        qt_internal_append_known_module_tool("${arg_TOOLS_TARGET}" "${target_name}")

        qt_get_cmake_configurations(cmake_configs)

        set(install_initial_call_args
            EXPORT "${INSTALL_CMAKE_NAMESPACE}${arg_TOOLS_TARGET}ToolsTargets")

        foreach(cmake_config ${cmake_configs})
            qt_get_install_target_default_args(
                OUT_VAR install_targets_default_args
                RUNTIME "${install_dir}"
                CMAKE_CONFIG "${cmake_config}"
                ALL_CMAKE_CONFIGS "${cmake_configs}")

            # Make installation optional for targets that are not built by default in this config
            if(QT_FEATURE_debug_and_release
                    AND NOT (cmake_config STREQUAL QT_MULTI_CONFIG_FIRST_CONFIG))
                set(install_optional_arg OPTIONAL)
              else()
                unset(install_optional_arg)
            endif()

            qt_install(TARGETS "${target_name}"
                       ${install_initial_call_args}
                       ${install_optional_arg}
                       CONFIGURATIONS ${cmake_config}
                       ${install_targets_default_args})
            unset(install_initial_call_args)
        endforeach()

        if(arg_INSTALL_VERSIONED_LINK)
            qt_internal_install_versioned_link("${install_dir}" "${target_name}")
        endif()

        qt_apply_rpaths(TARGET "${target_name}" INSTALL_PATH "${install_dir}" RELATIVE_RPATH)

    endif()

    qt_enable_separate_debug_info(${target_name} "${install_dir}")
    qt_internal_install_pdb_files(${target_name} "${install_dir}")
endfunction()

function(qt_export_tools module_name)
    # Bail out when cross-compiling, unless QT_BUILD_TOOLS_WHEN_CROSSCOMPILING is on.
    if(CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        return()
    endif()

    # If no tools were defined belonging to this module, don't create a config and targets file.
    if(NOT "${module_name}" IN_LIST QT_KNOWN_MODULES_WITH_TOOLS)
        return()
    endif()

    # The tools target name. For example: CoreTools
    set(target "${module_name}Tools")

    set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
    qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
    qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

    # Add the extra cmake statements to make the tool targets global, so it doesn't matter where
    # find_package is called.
    # Also assemble a list of tool targets to expose in the config file for informational purposes.
    set(extra_cmake_statements "")
    set(tool_targets "")
    set(tool_targets_non_prefixed "")

    # List of package dependencies that need be find_package'd when using the Tools package.
    set(package_deps "")

    # Additional cmake files to install
    set(extra_cmake_files "")

    foreach(tool_name ${QT_KNOWN_MODULE_${module_name}_TOOLS})
        # Specific tools can have package dependencies.
        # e.g. qtwaylandscanner depends on WaylandScanner (non-qt package).
        get_target_property(extra_packages "${tool_name}" QT_EXTRA_PACKAGE_DEPENDENCIES)
        if(extra_packages)
            list(APPEND package_deps "${extra_packages}")
        endif()

        get_target_property(_extra_cmake_files "${tool_name}" EXTRA_CMAKE_FILES)
        if (_extra_cmake_files)
            foreach(cmake_file ${_extra_cmake_files})
                file(COPY "${cmake_file}" DESTINATION "${config_build_dir}")
                list(APPEND extra_cmake_files "${cmake_file}")
            endforeach()
        endif()

        if (CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            string(REGEX REPLACE "_native$" "" tool_name ${tool_name})
        endif()
        set(extra_cmake_statements "${extra_cmake_statements}
if (NOT QT_NO_CREATE_TARGETS)
    get_property(is_global TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL)
    if(NOT is_global)
        set_property(TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL TRUE)
    endif()
endif()
")
        list(APPEND tool_targets "${QT_CMAKE_EXPORT_NAMESPACE}::${tool_name}")
        list(APPEND tool_targets_non_prefixed "${tool_name}")
    endforeach()

    string(APPEND extra_cmake_statements
"set(${QT_CMAKE_EXPORT_NAMESPACE}${module_name}Tools_TARGETS \"${tool_targets}\")")

    set(extra_cmake_includes "")
    foreach(extra_cmake_file ${extra_cmake_files})
        get_filename_component(extra_cmake_include "${extra_cmake_file}" NAME)
        list(APPEND extra_cmake_includes "${extra_cmake_include}")
    endforeach()

    # Extract package dependencies that were determined in QtPostProcess, but only if ${module_name}
    # is an actual target.
    # module_name can be a non-existent target, if the tool doesn't have an existing associated
    # module, e.g. qtwaylandscanner.
    if(TARGET "${module_name}")
        get_target_property(module_package_deps "${module_name}" _qt_tools_package_deps)
        if(module_package_deps)
            list(APPEND package_deps "${module_package_deps}")
        endif()
    endif()

    # Configure and install dependencies file for the ${module_name}Tools package.
    configure_file(
        "${QT_CMAKE_DIR}/QtModuleToolsDependencies.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
        @ONLY
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    if(extra_cmake_files)
        qt_install(FILES
           ${extra_cmake_files}
           DESTINATION "${config_install_dir}"
           COMPONENT Devel
        )
    endif()

    # Configure and install the ${module_name}Tools package Config file.
    qt_internal_get_min_new_policy_cmake_version(min_new_policy_version)
    qt_internal_get_max_new_policy_cmake_version(max_new_policy_version)
    configure_package_config_file(
        "${QT_CMAKE_DIR}/QtModuleToolsConfig.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )
    write_basic_package_version_file(
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
        ARCH_INDEPENDENT
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")
    qt_install(EXPORT "${export_name}"
               NAMESPACE "${QT_CMAKE_EXPORT_NAMESPACE}::"
               DESTINATION "${config_install_dir}")

    qt_internal_export_additional_targets_file(
        TARGETS ${QT_KNOWN_MODULE_${module_name}_TOOLS}
        TARGET_EXPORT_NAMES ${tool_targets}
        EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
        CONFIG_INSTALL_DIR "${config_install_dir}")

    # Create versionless targets file.
    configure_file(
        "${QT_CMAKE_DIR}/QtModuleToolsVersionlessTargets.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}VersionlessTargets.cmake"
        @ONLY
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}VersionlessTargets.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )
endfunction()

function(qt_get_tool_cmake_configuration out_var)
    qt_get_main_cmake_configuration("${out_var}")
    string(TOUPPER "${${out_var}}" upper_config)
    set("${out_var}" "${upper_config}" PARENT_SCOPE)
endfunction()

function(qt_get_main_cmake_configuration out_var)
    if(CMAKE_BUILD_TYPE)
        set(config "${CMAKE_BUILD_TYPE}")
    elseif(QT_MULTI_CONFIG_FIRST_CONFIG)
        set(config "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()
    set("${out_var}" "${config}" PARENT_SCOPE)
endfunction()

# Returns the target name for the tool with the given name.
#
# In most cases, the target name is the same as the tool name.
# If the user specifies to build tools when cross-compiling, then the
# suffix "_native" is appended.
function(qt_get_tool_target_name out_var name)
    if (CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        set(${out_var} ${name}_native PARENT_SCOPE)
    else()
        set(${out_var} ${name} PARENT_SCOPE)
    endif()
endfunction()

# Returns the tool name for a given tool target.
# This is the inverse of qt_get_tool_target_name.
function(qt_tool_target_to_name out_var target)
    set(name ${target})
    if (CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        string(REGEX REPLACE "_native$" "" name ${target})
    endif()
    set(${out_var} ${name} PARENT_SCOPE)
endfunction()

# Sets QT_WILL_BUILD_TOOLS if tools will be built.
function(qt_check_if_tools_will_be_built)
    if(QT_FORCE_FIND_TOOLS OR (CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING))
        set(will_build_tools FALSE)
    else()
        set(will_build_tools TRUE)
    endif()
    set(QT_WILL_BUILD_TOOLS ${will_build_tools} CACHE INTERNAL "Are tools going to be built" FORCE)
endfunction()

# Equivalent of qmake's qtNomakeTools(directory1 directory2).
# If QT_BUILD_TOOLS_BY_DEFAULT is true, then targets within the given directories will be excluded
# from the default 'all' target, as well as from install phase. The private variable is checked by
# qt_internal_add_executable.
function(qt_exclude_tool_directories_from_default_target)
    if(NOT QT_BUILD_TOOLS_BY_DEFAULT)
        set(absolute_path_directories "")
        foreach(directory ${ARGV})
            list(APPEND absolute_path_directories "${CMAKE_CURRENT_SOURCE_DIR}/${directory}")
        endforeach()
        set(__qt_exclude_tool_directories "${absolute_path_directories}" PARENT_SCOPE)
    endif()
endfunction()
