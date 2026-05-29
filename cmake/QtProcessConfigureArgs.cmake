# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This script reads Qt configure arguments from config.opt,
# translates the arguments to CMake arguments and calls CMake.
#
# This file is to be used in CMake script mode with the following variables set:
# OPTFILE: A text file containing the options that were passed to configure
#          with one option per line.
# MODULE_ROOT: The source directory of the module to be built.
#              If empty, qtbase/top-level is assumed.
# TOP_LEVEL: TRUE, if this is a top-level build.

# The CMake version required for running the configure script.
# This must be less than or equal to the lowest QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT_*.
cmake_minimum_required(VERSION 3.19)

include(${CMAKE_CURRENT_LIST_DIR}/QtFeatureCommon.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInformation.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/QtVcpkgManifestHelpers.cmake)

set(cmake_args "")
macro(push)
    list(APPEND cmake_args ${ARGN})
endmacro()

macro(pop_path_argument)
    list(POP_FRONT configure_args path)
    string(REGEX REPLACE "^\"(.*)\"$" "\\1" path "${path}")
    file(TO_CMAKE_PATH "${path}" path)
endmacro()

function(qt_configure_print_to_stdout text)
    set(tmp_candidates
        "${CMAKE_CURRENT_BINARY_DIR}"
        "$ENV{TMPDIR}"
        "$ENV{TEMP}"
        "/tmp"
    )
    set(tmp "")
    foreach(dir IN LISTS tmp_candidates)
        if(dir STREQUAL "")
            continue()
        endif()
        set(candidate "${dir}/.qt_configure_stdout_tmp")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E touch "${candidate}"
            RESULT_VARIABLE touch_result
        )
        if(touch_result EQUAL 0)
            set(tmp "${candidate}")
            break()
        endif()
    endforeach()
    if(tmp STREQUAL "")
        message("${text}")  # last resort fallback (stderr)
        return()
    endif()
    file(WRITE "${tmp}" "${text}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E cat "${tmp}")
    file(REMOVE "${tmp}")
endfunction()

function(is_non_empty_valid_arg arg value)
    if(value STREQUAL "")
        message(FATAL_ERROR "Value supplied to command line option '${arg}' is empty.")
    elseif(value MATCHES "^-.*")
        message(FATAL_ERROR
                "Value supplied to command line option '${arg}' is invalid: ${value}")
    endif()
endfunction()

function(warn_in_per_repo_build arg)
    if(NOT TOP_LEVEL)
        message(WARNING "Command line option ${arg} is only effective in top-level builds")
    endif()
endfunction()

function(is_valid_qt_hex_version arg version)
    if(NOT version MATCHES "^0x[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$")
        message(FATAL_ERROR "Incorrect version ${version} specified for ${arg}")
    endif()
endfunction()

if("${MODULE_ROOT}" STREQUAL "")
    # If MODULE_ROOT is not set, assume that we want to build qtbase or top-level.
    get_filename_component(MODULE_ROOT ".." ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    set(qtbase_or_top_level_build TRUE)
else()
    # If MODULE_ROOT is passed without drive letter, we try to add it to the path.
    # The check is necessary; otherwise, `get_filename_component` returns an empty string.
    if(NOT MODULE_ROOT STREQUAL ".")
        get_filename_component(MODULE_ROOT "." REALPATH BASE_DIR "${MODULE_ROOT}")
    endif()
    set(qtbase_or_top_level_build FALSE)
endif()
set(configure_filename "configure.cmake")
set(commandline_filename "qt_cmdline.cmake")
if(TOP_LEVEL)
    get_filename_component(MODULE_ROOT "../.." ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    file(GLOB commandline_files "${MODULE_ROOT}/*/${commandline_filename}")
    if(EXISTS "${MODULE_ROOT}/${commandline_filename}")
        list(PREPEND commandline_files "${MODULE_ROOT}/${commandline_filename}")
    endif()
else()
    set(commandline_files "${MODULE_ROOT}/${commandline_filename}")
endif()
file(STRINGS "${OPTFILE}" configure_args)

# list(TRANSFORM ...) unexpectedly removes semicolon escaping in list items. So the list arguments
# seem to be broken. The 'bracket argument' suppresses this behavior. Right before forwarding
# command line arguments to the cmake call, 'bracket arguments' are replaced by escaped semicolons
# back.
list(TRANSFORM configure_args REPLACE ";" "[[;]]")

list(FILTER configure_args EXCLUDE REGEX "^[ \t]*$")
list(TRANSFORM configure_args STRIP)
unset(generator)
set(auto_detect_compiler TRUE)
set(auto_detect_generator ${qtbase_or_top_level_build})
set(dry_run FALSE)
set(no_prefix_option FALSE)
set(skipped_qtrepos "")
set(use_vcpkg FALSE)
set(generate_vcpkg_manifest "unknown")
unset(device_options)
unset(options_json_file)
set_property(GLOBAL PROPERTY UNHANDLED_ARGS "")
while(NOT "${configure_args}" STREQUAL "")
    list(POP_FRONT configure_args raw_arg)

    # Condense '--foo-bar' arguments into '-foo-bar'.
    string(REGEX REPLACE "^--([^-])" "-\\1" arg "${raw_arg}")

    if(arg STREQUAL "-cmake-generator")
        list(POP_FRONT configure_args generator)
    elseif(arg STREQUAL "-cmake-use-default-generator")
        set(auto_detect_generator FALSE)
    elseif(arg STREQUAL "-dry-run")
        set(dry_run TRUE)
    elseif(arg MATCHES "^-(no-)?vcpkg$")
        if(CMAKE_MATCH_1 STREQUAL "no-")
            set(use_vcpkg FALSE)
        else()
            set(use_vcpkg TRUE)
        endif()
    elseif(arg MATCHES "^-(no-)?generate-vcpkg-manifest$")
        if(CMAKE_MATCH_1 STREQUAL "no-")
            set(generate_vcpkg_manifest FALSE)
        else()
            set(generate_vcpkg_manifest TRUE)
        endif()
    elseif(arg STREQUAL "-no-guess-compiler")
        set(auto_detect_compiler FALSE)
    elseif(arg STREQUAL "-list-features")
        set(list_features TRUE)
        if(configure_args)
            list(GET configure_args 0 _next_arg)
            if(NOT _next_arg MATCHES "^-")
                list(POP_FRONT configure_args list_features_module)
            endif()
        endif()
    elseif(arg STREQUAL "-list-modules")
        set(list_modules TRUE)
    elseif(arg MATCHES "^-h(elp)?$")
        set(display_module_help TRUE)
    elseif(arg STREQUAL "-write-options-for-conan")
        list(POP_FRONT configure_args options_json_file)
    elseif(arg STREQUAL "-skip")
        warn_in_per_repo_build("${arg}")
        list(POP_FRONT configure_args qtrepos)
        is_non_empty_valid_arg("${arg}" "${qtrepos}")
        list(TRANSFORM qtrepos REPLACE "," ";")
        foreach(qtrepo IN LISTS qtrepos)
            push("-DBUILD_${qtrepo}=OFF")
            list(APPEND skipped_qtrepos "${qtrepo}")
        endforeach()
    elseif(arg STREQUAL "-skip-tests")
        list(POP_FRONT configure_args qtrepos)
        is_non_empty_valid_arg("${arg}" "${qtrepos}")
        list(TRANSFORM qtrepos REPLACE "," ";")
        foreach(qtrepo IN LISTS qtrepos)
            push("-DQT_BUILD_TESTS_PROJECT_${qtrepo}=OFF")
        endforeach()
    elseif(arg STREQUAL "-skip-examples")
        list(POP_FRONT configure_args qtrepos)
        is_non_empty_valid_arg("${arg}" "${qtrepos}")
        list(TRANSFORM qtrepos REPLACE "," ";")
        foreach(qtrepo IN LISTS qtrepos)
            push("-DQT_BUILD_EXAMPLES_PROJECT_${qtrepo}=OFF")
        endforeach()
    elseif(arg STREQUAL "-submodules")
        warn_in_per_repo_build("${arg}")
        list(POP_FRONT configure_args submodules)
        is_non_empty_valid_arg("${arg}" "${submodules}")
        list(TRANSFORM submodules REPLACE "," "[[;]]")
        push("-DQT_BUILD_SUBMODULES=${submodules}")
    elseif(arg STREQUAL "-qt-host-path")
        pop_path_argument()
        push("-DQT_HOST_PATH=${path}")
    elseif(arg STREQUAL "-hostdatadir")
        pop_path_argument()
        if(NOT path MATCHES "(^|/)mkspecs$")
            string(APPEND path "/mkspecs")
        endif()
        push("-DINSTALL_MKSPECSDIR=${path}")
    elseif(arg STREQUAL "-developer-build")
        set(developer_build TRUE)
        push("-DFEATURE_developer_build=ON")
    elseif(arg STREQUAL "-no-prefix")
        set(no_prefix_option TRUE)
        push("-DFEATURE_no_prefix=ON")
    elseif(arg STREQUAL "-cmake-file-api")
        set(cmake_file_api TRUE)
    elseif(arg STREQUAL "-no-cmake-file-api")
        set(cmake_file_api FALSE)
    elseif(arg STREQUAL "-verbose")
        list(APPEND cmake_args "--log-level=STATUS")
    elseif(arg STREQUAL "-disable-deprecated-up-to")
        list(POP_FRONT configure_args version)
        is_valid_qt_hex_version("${arg}" "${version}")
        push("-DQT_DISABLE_DEPRECATED_UP_TO=${version}")
    elseif(raw_arg STREQUAL "--")
        # Everything after this argument will be passed to CMake verbatim.
        list(APPEND cmake_args "${configure_args}")
        break()
    else()
        set_property(GLOBAL APPEND PROPERTY UNHANDLED_ARGS "${raw_arg}")
    endif()
endwhile()

# Turn on vcpkg usage if requested.
# Set the manifest directory to where we generate the manifest file.
if(use_vcpkg)
    push(-DQT_USE_VCPKG=ON)
    push("-DVCPKG_MANIFEST_DIR=${CMAKE_CURRENT_BINARY_DIR}")
endif()

# By default, generate a manifest if using vcpkg.
if(generate_vcpkg_manifest STREQUAL "unknown")
    set(generate_vcpkg_manifest ${use_vcpkg})
endif()

# Read the specified manually generator value from CMake command line.
# The '-cmake-generator' argument has higher priority than CMake command line.
if(NOT generator)
    set(is_next_arg_generator_name FALSE)
    foreach(arg IN LISTS cmake_args)
        if(is_next_arg_generator_name)
            set(is_next_arg_generator_name FALSE)
            if(NOT arg MATCHES "^-.*")
                set(generator "${arg}")
                set(auto_detect_generator FALSE)
            endif()
        elseif(arg MATCHES "^-G(.*)")
            set(generator "${CMAKE_MATCH_1}")
            if(generator)
                set(auto_detect_generator FALSE)
            else()
                set(is_next_arg_generator_name TRUE)
            endif()
        endif()
    endforeach()
endif()

# Attempt to detect the generator type, either single or multi-config
if("${generator}" STREQUAL "Xcode"
    OR "${generator}" STREQUAL "Ninja Multi-Config"
    OR "${generator}" MATCHES "^Visual Studio")
    set(multi_config ON)
else()
    set(multi_config OFF)
endif()

# Tell the build system we are configuring via the configure script so we can act on that.
# The cache variable is unset at the end of configuration.
push("-DQT_INTERNAL_CALLED_FROM_CONFIGURE:BOOL=TRUE")

if(FRESH_REQUESTED)
    push("-DQT_INTERNAL_FRESH_REQUESTED:BOOL=TRUE")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
        push("--fresh")
    else()
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeCache.txt"
                            "${CMAKE_BINARY_DIR}/CMakeFiles")
    endif()
endif()

####################################################################################################
# Define functions/macros that are called in configure.cmake files
#
# Every function that's called in a configure.cmake file must be defined here.
# Most are empty stubs.
####################################################################################################

set_property(GLOBAL PROPERTY COMMANDLINE_KNOWN_FEATURES "")

# Define a Qt feature.
#
# Arguments that start with VCPKG_ affect the creation of vcpkg features. If
# either VCPKG_DEFAULT or VCPGK_OPTIONAL are given, a corresponding vcpkg
# feature will be created.
#
#   VCPKG_DEFAULT
#     Specifies to create a vcpkg feature that will be added to the manifest's
#     default features.
#
#   VCPKG_OPTIONAL
#     Specifies to create an optional vcpkg feature.
#
#   VCPKG_DESCRIPTION <string>
#     Optional description of the vcpkg feature. If not given, the description
#     is derived from PURPOSE, or LABEL, or the feature name.
#
#   VCPKG_DEPENDENT_FEATURES <features>
#     List of vcpkg features this feature depends upon.
#     For example, the "jpeg" feature would use "VCPKG_DEPENDENT_FEATURES gui".
function(qt_feature feature)
    set(no_value_options
        VCPKG_DEFAULT
        VCPKG_OPTIONAL
    )
    set(single_value_options
        LABEL
        PURPOSE
        SECTION
        VCPKG_DESCRIPTION
    )
    set(multi_value_options
        VCPKG_DEPENDENT_FEATURES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    get_property(_current_module GLOBAL PROPERTY COMMANDLINE_CURRENT_CONFIGURE_MODULE)
    set_property(GLOBAL APPEND PROPERTY COMMANDLINE_KNOWN_FEATURES "${feature}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_PURPOSE_${feature} "${arg_PURPOSE}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_SECTION_${feature} "${arg_SECTION}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_MODULE_${feature} "${_current_module}")

    if(NOT generate_vcpkg_manifest)
        return()
    endif()

    set(unknown_vcpkg_args "${arg_UNPARSED_ARGUMENTS}")
    list(FILTER unknown_vcpkg_args INCLUDE REGEX "^VCPKG_")
    if(NOT "${unknown_vcpkg_args}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments passed to qt_feature: ${unknown_vcpkg_args}")
    endif()

    set(create_vcpkg_feature FALSE)
    if(arg_VCPKG_DEFAULT OR arg_VCPKG_OPTIONAL)
        set(create_vcpkg_feature TRUE)
    else()
        get_cmake_property(vcpkg_features_to_create _QT_VCPKG_FEATURES_TO_CREATE)
        if("${feature}" IN_LIST vcpkg_features_to_create)
            set(create_vcpkg_feature TRUE)
            list(REMOVE_ITEM vcpkg_features_to_create "${feature}")
            set_property(GLOBAL PROPERTY
                _QT_VCPKG_FEATURES_TO_CREATE ${vcpkg_features_to_create}
            )
        endif()
    endif()

    if(NOT create_vcpkg_feature)
        return()
    endif()

    # Determine the description
    if(DEFINED arg_VCPKG_DESCRIPTION)
        set(description "${arg_VCPKG_DESCRIPTION}")
    elseif(DEFINED arg_PURPOSE)
        set(description "${arg_PURPOSE}")
    elseif(DEFINED arg_LABEL)
        set(description "${arg_LABEL}")
    else()
        set(description "${feature} support")
    endif()

    # Determine the dependent features (e.g. gui for freetype)
    set(dependent_features "")
    get_cmake_property(vcpkg_scope _QT_VCPKG_SCOPE)
    if(vcpkg_scope)
        list(APPEND dependent_features "${vcpkg_scope}")
    endif()
    if(DEFINED arg_VCPKG_DEPENDENT_FEATURES)
        list(APPEND dependent_features "${arg_VCPKG_DEPENDENT_FEATURES}")
    endif()

    # Features that aren't dependencies of other features are added to default-features
    # unless VCPKG_DEFAULT or VCPKG_OPTIONAL are specified.
    set(additional_args "")
    if(NOT arg_VCKG_DEFAULT AND NOT arg_VCPKG_OPTIONAL AND dependent_features STREQUAL "")
        list(APPEND additional_args DEFAULT)
    endif()
    qt_vcpkg_feature("${feature}" "${description}" ${additional_args})

    foreach(dependent_feature IN LISTS dependent_features)
        qt_vcpkg_add_feature_dependencies("${dependent_feature}" "${feature}")
    endforeach()
endfunction()

function(qt_feature_alias feature)
    cmake_parse_arguments(arg "NEGATE" "PURPOSE;SECTION;MESSAGE;ALIAS_OF_FEATURE;ALIAS_OF_CACHE" ""
        ${ARGN})
    set_property(GLOBAL APPEND PROPERTY COMMANDLINE_KNOWN_FEATURES "${feature}")
    # Mark the feature as aliased
    set(alias_note "alias of ")
    if(arg_NEGATE)
        string(APPEND alias_note "NOT ")
    endif()
    if(arg_ALIAS_OF_FEATURE)
        string(APPEND alias_note "${arg_ALIAS_OF_FEATURE} Feature")
    else()
        string(APPEND alias_note "${arg_ALIAS_OF_CACHE} Cache")
    endif()
    set(arg_PURPOSE "(${alias_note}) ${arg_PURPOSE}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_PURPOSE_${feature} "${arg_PURPOSE}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_SECTION_${feature} "${arg_SECTION}")
endfunction()

function(qt_feature_deprecated feature)
    cmake_parse_arguments(arg "" "PURPOSE;SECTION;MESSAGE" "" ${ARGN})
    set_property(GLOBAL APPEND PROPERTY COMMANDLINE_KNOWN_FEATURES "${feature}")
    # Mark the feature as deprecated
    set(arg_PURPOSE "(DEPRECATED) ${arg_PURPOSE} ${arg_MESSAGE}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_PURPOSE_${feature} "${arg_PURPOSE}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_SECTION_${feature} "${arg_SECTION}")
endfunction()

function(qt_feature_vcpkg_scope name)
    set_property(GLOBAL PROPERTY _QT_VCPKG_SCOPE ${name})
endfunction()

function(find_package)
    message(FATAL_ERROR "find_package must not be used directly in configure.cmake. "
        "Use qt_find_package or guard the call with an if(NOT QT_CONFIGURE_RUNNING) block.")
endfunction()

function(qt_init_vcpkg_manifest_if_needed)
    get_property(manifest_initialized GLOBAL PROPERTY _QT_VCPKG_MANIFEST_JSON SET)
    if(manifest_initialized)
        return()
    endif()

    if(TOP_LEVEL)
        set(package_name "qt")
    else()
        get_filename_component(package_name "${MODULE_ROOT}" NAME)
    endif()
    qt_vcpkg_manifest_init(NAME "${package_name}")
endfunction()

function(qt_find_package name)
    if(NOT generate_vcpkg_manifest)
        return()
    endif()

    set(no_value_options "")
    set(single_value_options
        VCPKG_ADD_TO_FEATURE
        VCPKG_DEFAULT_FEATURES
        VCPKG_PLATFORM
        VCPKG_PORT
        VCPKG_VERSION
    )
    set(multi_value_options
        VCPKG_FEATURES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(unknown_vcpkg_args "${arg_UNPARSED_ARGUMENTS}")
    list(FILTER unknown_vcpkg_args INCLUDE REGEX "^VCPKG_")
    if(NOT "${unknown_vcpkg_args}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments passed to qt_find_package: ${unknown_vcpkg_args}")
    endif()

    if(NOT DEFINED arg_VCPKG_PORT)
        return()
    endif()

    qt_init_vcpkg_manifest_if_needed()

    set(dependency_args "${arg_VCPKG_PORT}")
    if(DEFINED arg_VCPKG_VERSION)
        list(APPEND dependency_args VERSION "${arg_VCPKG_VERSION}")
    endif()
    if(DEFINED arg_VCPKG_PLATFORM)
        list(APPEND dependency_args PLATFORM "${arg_VCPKG_PLATFORM}")
    endif()
    if(DEFINED arg_VCPKG_DEFAULT_FEATURES)
        list(APPEND dependency_args DEFAULT_FEATURES "${arg_VCPKG_DEFAULT_FEATURES}")
    endif()
    if(DEFINED arg_VCPKG_ADD_TO_FEATURE)
        list(APPEND dependency_args ADD_TO_FEATURE "${arg_VCPKG_ADD_TO_FEATURE}")
        set_property(GLOBAL APPEND PROPERTY
            _QT_VCPKG_FEATURES_TO_CREATE "${arg_VCPKG_ADD_TO_FEATURE}"
        )
    endif()
    if(DEFINED arg_VCPKG_FEATURES)
        list(APPEND dependency_args FEATURES "${arg_VCPKG_FEATURES}")
    endif()
    qt_vcpkg_add_dependency(${dependency_args})
endfunction()

macro(defstub name)
    function(${name})
    endfunction()
endmacro()

defstub(qt_add_qmake_lib_dependency)
defstub(qt_run_config_compile_test)
defstub(qt_config_compile_test)
defstub(qt_config_compile_test_armintrin)
defstub(qt_config_compile_test_machine_tuple)
defstub(qt_config_compile_test_x86simd)
defstub(qt_config_compile_test_loongarchsimd)
defstub(qt_config_compiler_supports_flag_test)
defstub(qt_config_linker_supports_flag_test)
defstub(qt_configure_add_report_entry)
defstub(qt_configure_add_summary_build_mode)
defstub(qt_configure_add_summary_build_parts)
defstub(qt_configure_add_summary_build_type_and_config)
defstub(qt_configure_add_summary_entry)
defstub(qt_configure_add_summary_section)
defstub(qt_configure_end_summary_section)
defstub(qt_extra_definition)
defstub(qt_feature_config)
defstub(qt_feature_definition)
defstub(set_package_properties)
defstub(qt_qml_find_python)
defstub(qt_internal_check_if_linker_is_available)
defstub(qt_internal_add_sbom)
defstub(qt_internal_extend_sbom)
defstub(qt_internal_sbom_add_license)
defstub(qt_internal_extend_sbom_dependencies)
defstub(qt_find_package_extend_sbom)

####################################################################################################
# Define functions/macros that are called in qt_cmdline.cmake files
####################################################################################################

unset(commandline_known_options)
unset(commandline_custom_handlers)
set(commandline_nr_of_prefixes 0)

macro(qt_commandline_subconfig subconfig)
    list(APPEND commandline_subconfigs "${subconfig}")
endmacro()

macro(qt_commandline_custom handler)
    list(APPEND commandline_custom_handlers ${handler})
endmacro()

function(qt_commandline_option name)
    set(options CONTROLS_FEATURE)
    set(oneValueArgs TYPE NAME VALUE CMAKE_VARIABLE)
    set(multiValueArgs VALUES MAPPING)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(commandline_known_options "${commandline_known_options};${name}" PARENT_SCOPE)
    set(commandline_option_${name} "${arg_TYPE}" PARENT_SCOPE)
    set(input_name ${name})
    if(NOT "${arg_NAME}" STREQUAL "")
        set(input_name ${arg_NAME})
        set(commandline_option_${name}_variable "${arg_NAME}" PARENT_SCOPE)
    endif()
    if(DEFINED arg_CMAKE_VARIABLE)
        set_property(GLOBAL PROPERTY INPUTCMAKEVAR_${input_name} "${arg_CMAKE_VARIABLE}")
    endif()
    set(mapping_type "${arg_TYPE}")
    if(arg_CONTROLS_FEATURE)
        set(mapping_type "boolean")
    endif()
    set_property(GLOBAL PROPERTY INPUTTYPE_${input_name} "${mapping_type}")
    if(NOT "${arg_VALUE}" STREQUAL "")
        set(commandline_option_${name}_value "${arg_VALUE}" PARENT_SCOPE)
    endif()
    if(arg_VALUES)
        set(commandline_option_${name}_values ${arg_VALUES} PARENT_SCOPE)
    elseif(arg_MAPPING)
        set(commandline_option_${name}_mapping ${arg_MAPPING} PARENT_SCOPE)
    endif()
endfunction()

# Add the common command line options for every qt repo.
macro(qt_add_common_commandline_options)
    qt_commandline_option(headersclean TYPE boolean)
    qt_commandline_option(sbom TYPE boolean CMAKE_VARIABLE QT_GENERATE_SBOM)

    # Semi-public, undocumented.
    qt_commandline_option(sbom-all TYPE boolean CMAKE_VARIABLE QT_SBOM_GENERATE_AND_VERIFY_ALL)

    qt_commandline_option(sbom-spdx-v2 TYPE boolean
        CMAKE_VARIABLE QT_SBOM_GENERATE_SPDX_V2)

    qt_commandline_option(sbom-cyclonedx-v1_6 TYPE boolean
        CMAKE_VARIABLE QT_SBOM_GENERATE_CYDX_V1_6)

    qt_commandline_option(sbom-cyclonedx-v1_6-required TYPE boolean
        CMAKE_VARIABLE QT_SBOM_REQUIRE_GENERATE_CYDX_V1_6)

    qt_commandline_option(sbom-cyclonedx-v1_6-verify-required TYPE boolean
        CMAKE_VARIABLE QT_SBOM_REQUIRE_VERIFY_CYDX_V1_6)

    qt_commandline_option(sbom-cyclonedx-v1_6-verbose TYPE boolean
        CMAKE_VARIABLE QT_SBOM_VERBOSE_CYDX_V1_6)

    qt_commandline_option(sbom-json TYPE boolean CMAKE_VARIABLE QT_SBOM_GENERATE_SPDX_V2_JSON)
    qt_commandline_option(sbom-json-required TYPE boolean
        CMAKE_VARIABLE QT_SBOM_REQUIRE_GENERATE_SPDX_V2_JSON
    )

    qt_commandline_option(sbom-verify TYPE boolean CMAKE_VARIABLE QT_SBOM_VERIFY_SPDX_V2)
    qt_commandline_option(sbom-verify-required TYPE boolean
        CMAKE_VARIABLE QT_SBOM_REQUIRE_VERIFY_SPDX_V2)
endmacro()

function(qt_commandline_prefix arg var)
    set(idx ${commandline_nr_of_prefixes})
    set(commandline_prefix_${idx} "${arg}" "${var}" PARENT_SCOPE)
    math(EXPR n "${commandline_nr_of_prefixes} + 1")
    set(commandline_nr_of_prefixes ${n} PARENT_SCOPE)
endfunction()

# Check the following variable in configure.cmake files to guard code that is not covered by the
# stub functions above.
set(QT_CONFIGURE_RUNNING ON)

# Make sure that configure sees all qt_find_package calls.
set(QT_FIND_ALL_PACKAGES_ALWAYS ON)


####################################################################################################
# Load qt_cmdline.cmake files
####################################################################################################

qt_add_common_commandline_options()

set(cmdline_visited_dirs "")
while(commandline_files)
    list(POP_FRONT commandline_files commandline_file)
    get_filename_component(commandline_file_directory "${commandline_file}" DIRECTORY)

    # Skip qt_cmdline.cmake and configure.cmake files from skipped repositories.
    if(TOP_LEVEL)
        get_filename_component(repo "${commandline_file_directory}" NAME)
        if(repo IN_LIST skipped_qtrepos)
            continue()
        endif()
    endif()

    get_filename_component(current_configure_module "${commandline_file_directory}" NAME)
    if(current_configure_module STREQUAL "src")
        get_filename_component(_parent_dir "${commandline_file_directory}" DIRECTORY)
        get_filename_component(current_configure_module "${_parent_dir}" NAME)
    endif()
    set_property(GLOBAL PROPERTY COMMANDLINE_CURRENT_CONFIGURE_MODULE "${current_configure_module}")

    # Record this directory as visited by the qt_cmdline.cmake traversal.
    if(IS_ABSOLUTE "${commandline_file_directory}")
        get_filename_component(abs_module_root "${MODULE_ROOT}" ABSOLUTE)
        file(RELATIVE_PATH cmdline_marker_rel "${abs_module_root}" "${commandline_file_directory}")
    else()
        set(cmdline_marker_rel "${commandline_file_directory}")
    endif()
    if(NOT cmdline_marker_rel)
        set(cmdline_marker_rel ".")
    endif()
    list(APPEND cmdline_visited_dirs "${cmdline_marker_rel}")
    set(configure_file "${commandline_file_directory}/${configure_filename}")
    unset(commandline_subconfigs)
    if(EXISTS "${configure_file}")
        include("${configure_file}")
        set_property(GLOBAL PROPERTY _QT_VCPKG_SCOPE)
    endif()
    if(EXISTS "${commandline_file}")
        include("${commandline_file}")
    endif()

    if(commandline_subconfigs)
        list(TRANSFORM commandline_subconfigs PREPEND "${commandline_file_directory}/")
        list(TRANSFORM commandline_subconfigs APPEND "/${commandline_filename}")
        list(PREPEND commandline_files "${commandline_subconfigs}")
    endif()
endwhile()

# Write the list of directories visited by the qt_cmdline.cmake traversal,
# relative to MODULE_ROOT. It is consumed by the chain-coverage check in
# qt_internal_add_module.
list(REMOVE_DUPLICATES cmdline_visited_dirs)
list(JOIN cmdline_visited_dirs "\n" cmdline_visited_dirs_content)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/.qt/cmdline_visited_dirs" "${cmdline_visited_dirs_content}\n")

get_property(commandline_known_features GLOBAL PROPERTY COMMANDLINE_KNOWN_FEATURES)

####################################################################################################
# Process the data from the qt_cmdline.cmake files
####################################################################################################

function(qtConfAddNote)
    message(${ARGV})
endfunction()

function(qtConfAddWarning)
    message(WARNING ${ARGV})
endfunction()

function(qtConfAddError)
    message(FATAL_ERROR ${ARGV})
endfunction()

set_property(GLOBAL PROPERTY CONFIG_INPUTS "")

function(qtConfCommandlineSetInput name val)
    if(NOT "${commandline_option_${name}_variable}" STREQUAL "")
        set(name "${commandline_option_${name}_variable}")
    endif()
    if(NOT "${INPUT_${name}}" STREQUAL "")
        set(oldval "${INPUT_${name}}")
        if("${oldval}" STREQUAL "${val}")
            qtConfAddNote("Option '${name}' with value '${val}' was specified twice")
        else()
            qtConfAddNote("Overriding option '${name}' with '${val}' (was: '${oldval}')")
        endif()
    endif()

    set_property(GLOBAL PROPERTY INPUT_${name} "${val}")
    set_property(GLOBAL APPEND PROPERTY CONFIG_INPUTS ${name})
endfunction()

function(qtConfCommandlineAppendInput name val)
    get_property(oldval GLOBAL PROPERTY INPUT_${name})
    if(NOT "${oldval}" STREQUAL "")
        string(PREPEND val "${oldval};")
    endif()
    qtConfCommandlineSetInput(${name} "${val}")
endfunction()

function(qtConfCommandlineSetInputType input_name type_name)
    set_property(GLOBAL PROPERTY INPUTTYPE_${input_name} "${type_name}")
endfunction()

function(qtConfCommandlineSetBooleanInput name val)
    qtConfCommandlineSetInput(${name} ${val})
    qtConfCommandlineSetInputType(${name} boolean)
endfunction()

function(qtConfCommandlineEnableFeature name)
    qtConfCommandlineSetBooleanInput(${name} yes)
endfunction()

function(qtConfCommandlineDisableFeature name)
    qtConfCommandlineSetBooleanInput(${name} no)
endfunction()

function(qtConfValidateValue opt val out_var)
    set(${out_var} TRUE PARENT_SCOPE)

    set(valid_values ${commandline_option_${arg}_values})
    list(LENGTH valid_values n)
    if(n EQUAL 0)
        return()
    endif()

    foreach(v ${valid_values})
        if(val STREQUAL v)
            return()
        endif()
    endforeach()

    set(${out_var} FALSE PARENT_SCOPE)
    list(JOIN valid_values " " valid_values_str)
    qtConfAddError("Invalid value '${val}' supplied to command line option '${opt}'."
        "\nAllowed values: ${valid_values_str}\n")
endfunction()

function(qt_commandline_mapped_enum_value opt key out_var)
    unset(result)
    set(mapping ${commandline_option_${opt}_mapping})
    while(mapping)
        list(POP_FRONT mapping mapping_key)
        list(POP_FRONT mapping mapping_value)
        if(mapping_key STREQUAL key)
            set(result "${mapping_value}")
            break()
        endif()
    endwhile()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qtConfHasNextCommandlineArg out_var)
    get_property(args GLOBAL PROPERTY UNHANDLED_ARGS)
    list(LENGTH args n)
    if(n GREATER 0)
        set(result TRUE)
    else()
        set(result FALSE)
    endif()
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

function(qtConfPeekNextCommandlineArg out_var)
    get_property(args GLOBAL PROPERTY UNHANDLED_ARGS)
    list(GET args 0 result)
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

function(qtConfGetNextCommandlineArg out_var)
    get_property(args GLOBAL PROPERTY UNHANDLED_ARGS)
    list(POP_FRONT args result)
    set_property(GLOBAL PROPERTY UNHANDLED_ARGS ${args})
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

function(qt_commandline_boolean arg val nextok)
    if("${val}" STREQUAL "")
        set(val "yes")
    endif()
    if(NOT val STREQUAL "yes" AND NOT val STREQUAL "no")
        message(FATAL_ERROR "Invalid value '${val}' given for boolean command line option '${arg}'.")
    endif()
    qtConfCommandlineSetInput("${arg}" "${val}")
endfunction()

function(qt_commandline_string arg val nextok)
    if(nextok)
        qtConfGetNextCommandlineArg(val)
        if("${val}" MATCHES "^-")
            qtConfAddError("No value supplied to command line options '${opt}'.")
        endif()
    endif()
    qtConfValidateValue("${opt}" "${val}" success)
    if(success)
        qtConfCommandlineSetInput("${opt}" "${val}")
    endif()
endfunction()

# Handle command line arguments of type "path" exactly like strings.
# They are treated differently by translate_input, however.
function(qt_commandline_path arg val nextok)
    qt_commandline_string("${arg}" "${val}" "${nextok}")
endfunction()

# Handle command line arguments of type "stringList" exactly like strings.
# They are treated differently by translate_input, however.
function(qt_commandline_stringList arg val nextok)
    qt_commandline_string("${arg}" "${val}" "${nextok}")
endfunction()

function(qt_commandline_optionalString arg val nextok)
    if("${val}" STREQUAL "")
        if(nextok)
            qtConfPeekNextCommandlineArg(val)
        endif()
        if(val MATCHES "^-.*|[A-Z0-9_+]=.*" OR val STREQUAL "")
            set(val "yes")
        else()
            qtConfGetNextCommandlineArg(val)
        endif()
    endif()
    qtConfValidateValue("${arg}" "${val}" success)
    if(success)
        qtConfCommandlineSetInput("${arg}" "${val}")
    endif()
endfunction()

function(qt_commandline_addString arg val nextok)
    if("${val}" STREQUAL "" AND nextok)
        qtConfGetNextCommandlineArg(val)
    endif()
    if(val MATCHES "^-.*" OR val STREQUAL "")
        qtConfAddError("No value supplied to command line option '${arg}'.")
    endif()
    qtConfValidateValue("${arg}" "${val}" success)
    if(success)
        if(DEFINED command_line_option_${arg}_variable)
            set(arg ${command_line_option_${arg}_variable})
        endif()
        set_property(GLOBAL APPEND PROPERTY "INPUT_${arg}" "${val}")
        set_property(GLOBAL APPEND PROPERTY CONFIG_INPUTS ${arg})
    endif()
endfunction()

function(qt_commandline_enum arg val nextok)
    if("${val}" STREQUAL "")
        set(val "yes")
    endif()
    unset(mapped)
    if(DEFINED "commandline_option_${arg}_mapping")
        qt_commandline_mapped_enum_value("${arg}" "${val}" mapped)
    elseif(DEFINED "commandline_option_${arg}_values")
        if(val IN_LIST commandline_option_${arg}_values)
            set(mapped ${val})
        endif()
    endif()
    if("${mapped}" STREQUAL "")
        qtConfAddError("Invalid value '${val}' supplied to command line option '${arg}'.")
    endif()
    qtConfCommandlineSetInput("${arg}" "${mapped}")
endfunction()

function(qt_commandline_void arg val nextok)
    if(NOT "${val}" STREQUAL "")
        qtConfAddError("Command line option '${arg}' expects no argument ('${val}' given).")
    endif()
    if(DEFINED commandline_option_${arg}_value)
        set(val ${commandline_option_${arg}_value})
    endif()
    if("${val}" STREQUAL "")
        set(val yes)
    endif()
    qtConfCommandlineSetInput("${arg}" "${val}")
endfunction()

function(qt_call_function func)
    set(call_code "${func}(")
    math(EXPR n "${ARGC} - 1")
    foreach(i RANGE 1 ${n})
        string(APPEND call_code "\"${ARGV${i}}\" ")
    endforeach()
    string(APPEND call_code ")")
    string(REPLACE "\\" "\\\\" call_code "${call_code}")
    if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
        set(incfile qt_tmp_func_call.cmake)
        file(WRITE "${incfile}" "${call_code}")
        include(${incfile})
        file(REMOVE "${incfile}")
    else()
        cmake_language(EVAL CODE "${call_code}")
    endif()
endfunction()

if(display_module_help)
    set(help [[
Options:
  -help, -h ............ Display this help screen

  -feature-<feature> ... Enable <feature>
  -no-feature-<feature>  Disable <feature> [none]
  -list-features [module]  List available features, optionally filtered by
                           module directory (e.g., sql, network). Note that some
                           features have dedicated command line options as well.
  -list-modules .......... List available module names for use with -list-features.
]])
    set(help_file "${MODULE_ROOT}/config_help.txt")
    if(EXISTS "${help_file}")
        file(READ "${help_file}" help_file_content)
        string(APPEND help "${help_file_content}")
    endif()
    qt_configure_print_to_stdout("${help}")
    return()
endif()

if(list_modules)
    unset(modules)
    foreach(feature ${commandline_known_features})
        get_property(purpose GLOBAL PROPERTY COMMANDLINE_FEATURE_PURPOSE_${feature})
        if(purpose)
            get_property(feature_module GLOBAL PROPERTY COMMANDLINE_FEATURE_MODULE_${feature})
            list(APPEND modules "${feature_module}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES modules)
    list(SORT modules)
    list(JOIN modules "\n" modules)
    qt_configure_print_to_stdout("${modules}\n")
    return()
endif()

if(list_features)
    unset(lines)
    foreach(feature ${commandline_known_features})
        get_property(section GLOBAL PROPERTY COMMANDLINE_FEATURE_SECTION_${feature})
        get_property(purpose GLOBAL PROPERTY COMMANDLINE_FEATURE_PURPOSE_${feature})
        if(purpose)
            if(DEFINED list_features_module)
                get_property(feature_module GLOBAL PROPERTY COMMANDLINE_FEATURE_MODULE_${feature})
                if(NOT feature_module STREQUAL list_features_module)
                    continue()
                endif()
            endif()
            if(NOT "${section}" STREQUAL "")
                string(APPEND section ": ")
            endif()
            qt_configure_get_padded_string("${feature}" "${section}${purpose}" line
                PADDING_LENGTH 25 MIN_PADDING 1)
            list(APPEND lines "${line}")
        endif()
    endforeach()
    list(SORT lines)
    list(JOIN lines "\n" lines)
    qt_configure_print_to_stdout("${lines}\n")
    return()
endif()

function(write_options_json_file)
    if(qtbase_or_top_level_build)
        # Add options that are handled directly by this script.
        qt_commandline_option(qt-host-path TYPE string)
        qt_commandline_option(no-guess-compiler TYPE void)
    endif()

    set(indent "    ")
    set(content
        "{"
        "${indent}\"options\": {")
    string(APPEND indent "    ")
    list(LENGTH commandline_known_options commandline_known_options_length)
    set(i 1)
    foreach(opt ${commandline_known_options})
        list(APPEND content "${indent}\"${opt}\": {")
        string(APPEND indent "    ")
        list(APPEND content "${indent}\"type\": \"${commandline_option_${opt}}\",")
        if(NOT "${commandline_option_${opt}_values}" STREQUAL "")
            set(values "${commandline_option_${opt}_values}")
            list(TRANSFORM values PREPEND "\"")
            list(TRANSFORM values APPEND "\"")
            list(JOIN values ", " values)
            list(APPEND content "${indent}\"values\": [${values}]")
        elseif(NOT "${commandline_option_${opt}_mapping}" STREQUAL "")
            list(LENGTH commandline_option_${opt}_mapping last)
            math(EXPR last "${last} - 1")
            set(values "")
            list(APPEND content "${indent}\"values\": [")
            foreach(k RANGE 0 "${last}" 2)
                list(GET commandline_option_${opt}_mapping ${k} value)
                list(APPEND values ${value})
            endforeach()
            list(TRANSFORM values PREPEND "\"")
            list(TRANSFORM values APPEND "\"")
            list(JOIN values ", " values)
            list(APPEND content
                "${indent}    ${values}"
                "${indent}]")
        else()
            list(APPEND content "${indent}\"values\": []")
        endif()
        string(SUBSTRING "${indent}" 4 -1 indent)
        math(EXPR i "${i} + 1")
        if(i LESS commandline_known_options_length)
            list(APPEND content "${indent}},")
        else()
            list(APPEND content "${indent}}")
        endif()
    endforeach()
    string(SUBSTRING "${indent}" 4 -1 indent)

    set(features ${commandline_known_features})
    list(TRANSFORM features PREPEND "\"")
    list(TRANSFORM features APPEND "\"")
    list(JOIN features ", " features)

    list(APPEND content
        "${indent}},"
        "${indent}\"features\": [${features}]"
        "}")
    string(REPLACE ";" "\n" content "${content}")
    file(WRITE "${options_json_file}" "${content}")
endfunction()

if(options_json_file)
    write_options_json_file()
    return()
endif()

set(cmake_var_assignments)

while(1)
    qtConfHasNextCommandlineArg(has_next)
    if(NOT has_next)
        break()
    endif()
    qtConfGetNextCommandlineArg(arg)

    set(handled FALSE)
    foreach(func ${commandline_custom_handlers})
        qt_call_function("qt_commandline_${func}" handled "${arg}")
        if(handled)
            break()
        endif()
    endforeach()
    if(handled)
        continue()
    endif()

    # Handle variable assignments
    if(arg MATCHES "^([a-zA-Z0-9_][a-zA-Z0-9_-]*)=(.*)")
        list(APPEND cmake_var_assignments "${arg}")
        continue()
    endif()

    # parse out opt and val
    set(nextok FALSE)
    if(arg MATCHES "^--?enable-(.*)")
        set(opt "${CMAKE_MATCH_1}")
        set(val "yes")
    # Handle builtin [-no]-feature-xxx
    elseif(arg MATCHES "^--?(no-)?feature-(.*)")
        set(opt "${CMAKE_MATCH_2}")
        if(NOT opt IN_LIST commandline_known_features)
            qtConfAddError("Enabling/Disabling unknown feature '${opt}'.")
        endif()
        if("${CMAKE_MATCH_1}" STREQUAL "")
            set(val "ON")
        else()
            set(val "OFF")
        endif()
        qt_feature_normalize_name("${opt}" normalized_feature_name)
        push(-DFEATURE_${normalized_feature_name}=${val})
        continue()
    elseif(arg MATCHES "^--?(disable|no)-(.*)")
        set(opt "${CMAKE_MATCH_2}")
        set(val "no")
    elseif(arg MATCHES "^--([^=]+)=(.*)")
        set(opt "${CMAKE_MATCH_1}")
        set(val "${CMAKE_MATCH_2}")
    elseif(arg MATCHES "^--(.*)")
        set(opt "${CMAKE_MATCH_1}")
        unset(val)
    elseif(arg MATCHES "^-(.*)")
        set(nextok TRUE)
        set(opt "${CMAKE_MATCH_1}")
        unset(val)
        if(NOT DEFINED commandline_option_${opt} AND opt MATCHES "(qt|system)-(.*)")
            set(opt "${CMAKE_MATCH_2}")
            set(val "${CMAKE_MATCH_1}")
        endif()
    else()
        qtConfAddError("Invalid command line parameter '${arg}'.")
    endif()

    set(type ${commandline_option_${opt}})
    if("${type}" STREQUAL "")
        # No match in the regular options, try matching the prefixes
        math(EXPR n "${commandline_nr_of_prefixes} - 1")
        foreach(i RANGE ${n})
            list(GET commandline_prefix_${i} 0 pfx)
            if(arg MATCHES "^-${pfx}(.*)")
                list(GET commandline_prefix_${i} 1 opt)
                set(val "${CMAKE_MATCH_1}")
                set(type addString)
                break()
            endif()
        endforeach()
    endif()

    if("${type}" STREQUAL "")
        qtConfAddError("Unknown command line option '${arg}'.")
    endif()

    if(NOT COMMAND "qt_commandline_${type}")
        qtConfAddError("Unknown type '${type}' for command line option '${opt}'.")
    endif()
    qt_call_function("qt_commandline_${type}" "${opt}" "${val}" "${nextok}")
endwhile()

####################################################################################################
# Translate some of the INPUT_xxx values to CMake arguments
####################################################################################################

# Turn the global properties into proper variables
get_property(config_inputs GLOBAL PROPERTY CONFIG_INPUTS)
list(REMOVE_DUPLICATES config_inputs)
foreach(var ${config_inputs})
    get_property(INPUT_${var} GLOBAL PROPERTY INPUT_${var})
    if("${commandline_input_${var}_type}" STREQUAL "")
        get_property(commandline_input_${var}_type GLOBAL PROPERTY INPUTTYPE_${var})
    endif()
    get_property(commandline_input_${var}_cmake_variable GLOBAL PROPERTY INPUTCMAKEVAR_${var})
endforeach()

macro(drop_input name)
    list(REMOVE_ITEM config_inputs ${name})
endmacro()

macro(translate_boolean_input name cmake_var)
    if("${INPUT_${name}}" STREQUAL "yes")
        push("-D${cmake_var}=ON")
        drop_input(${name})
    elseif("${INPUT_${name}}" STREQUAL "no")
        push("-D${cmake_var}=OFF")
        drop_input(${name})
    endif()
endmacro()

macro(translate_string_input name cmake_var)
    if(DEFINED INPUT_${name})
        push("-D${cmake_var}=${INPUT_${name}}")
        drop_input(${name})
    endif()
endmacro()

macro(translate_path_input name cmake_var)
    if(DEFINED INPUT_${name})
        set(path "${INPUT_${name}}")
        string(REGEX REPLACE "^\"(.*)\"$" "\\1" path "${path}")
        file(TO_CMAKE_PATH "${path}" path)
        push("-D${cmake_var}=${path}")
        drop_input(${name})
    endif()
endmacro()

macro(translate_list_input name cmake_var)
    if(DEFINED INPUT_${name})
        list(JOIN INPUT_${name} "[[;]]" value)
        list(APPEND cmake_args "-D${cmake_var}=${value}")
        drop_input(${name})
    endif()
endmacro()

macro(translate_input name cmake_var)
    if("${commandline_input_${name}_type}" STREQUAL "boolean")
        translate_boolean_input(${name} ${cmake_var})
    elseif("${commandline_input_${name}_type}" STREQUAL "path")
        translate_path_input(${name} ${cmake_var})
    elseif("${commandline_input_${name}_type}" STREQUAL "string")
        translate_string_input(${name} ${cmake_var})
    elseif("${commandline_input_${name}_type}" STREQUAL "addString"
            OR "${commandline_input_${name}_type}" STREQUAL "stringList")
        translate_list_input(${name} ${cmake_var})
    else()
        message(FATAL_ERROR
            "translate_input cannot handle input '${name}' "
            "of type '${commandline_input_${name}_type}'."
        )
    endif()
endmacro()

# Check whether to guess the compiler for the given language.
#
# Sets ${out_var} to FALSE if one of the following holds:
# - the environment variable ${env_var} is non-empty
# - the CMake variable ${cmake_var} is set on the command line
#
# Otherwise, ${out_var} is set to TRUE.
function(check_whether_to_guess_compiler out_var env_var cmake_var)
    set(result TRUE)
    if(NOT "$ENV{${env_var}}" STREQUAL "")
        set(result FALSE)
    else()
        set(filtered_args ${cmake_args})
        list(FILTER filtered_args INCLUDE REGEX "^(-D)?${cmake_var}=")
        if(NOT "${filtered_args}" STREQUAL "")
            set(result FALSE)
        endif()
    endif()
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

# Try to guess the mkspec from the -platform configure argument.
function(guess_compiler_from_mkspec)
    if(NOT auto_detect_compiler)
        return()
    endif()

    check_whether_to_guess_compiler(guess_c_compiler CC CMAKE_C_COMPILER)
    check_whether_to_guess_compiler(guess_cxx_compiler CXX CMAKE_CXX_COMPILER)
    string(REGEX MATCH "(^|;)-DQT_QMAKE_TARGET_MKSPEC=\([^;]+\)" m "${cmake_args}")
    set(mkspec ${CMAKE_MATCH_2})
    if(guess_c_compiler OR guess_cxx_compiler)
        set(c_compiler "")
        set(cxx_compiler "")
        if(mkspec MATCHES "-clang-msvc$")
            set(c_compiler "clang-cl")
            set(cxx_compiler "clang-cl")
        elseif(mkspec MATCHES "-clang(-|$)" AND NOT mkspec MATCHES "android")
            set(c_compiler "clang")
            set(cxx_compiler "clang++")
        elseif(mkspec MATCHES "-msvc(-|$)")
            set(c_compiler "cl")
            set(cxx_compiler "cl")
        endif()
        if(guess_c_compiler AND NOT c_compiler STREQUAL "")
            push("-DCMAKE_C_COMPILER=${c_compiler}")
        endif()
        if(guess_cxx_compiler AND NOT cxx_compiler STREQUAL "")
            push("-DCMAKE_CXX_COMPILER=${cxx_compiler}")
        endif()
    endif()
    if(mkspec MATCHES "-libc\\+\\+$")
        push("-DFEATURE_stdlib_libcpp=ON")
    endif()
    set(cmake_args "${cmake_args}" PARENT_SCOPE)
endfunction()

function(check_qt_build_parts type)
    set(input "INPUT_${type}")
    set(buildFlag "TRUE")
    if("${type}" STREQUAL "nomake")
        set(buildFlag "FALSE")
    endif()

    foreach(part ${${input}})
        qt_feature_normalize_name("${part}" partUpperCase)
        string(TOUPPER "${partUpperCase}" partUpperCase)
        push("-DQT_BUILD_${partUpperCase}=${buildFlag}")
    endforeach()
    set(cmake_args "${cmake_args}" PARENT_SCOPE)
endfunction()

# Translate command line arguments that have CMAKE_VARIABLE set.
foreach(input IN LISTS config_inputs)
    if(NOT "${commandline_input_${input}_cmake_variable}" STREQUAL "")
        translate_input("${input}" "${commandline_input_${input}_cmake_variable}")
    endif()
endforeach()

drop_input(commercial)
drop_input(confirm-license)
translate_boolean_input(shared BUILD_SHARED_LIBS)

if(NOT "${INPUT_device}" STREQUAL "")
    push("-DQT_QMAKE_TARGET_MKSPEC=devices/${INPUT_device}")
    drop_input(device)
endif()
guess_compiler_from_mkspec()

if(DEFINED INPUT_android-ndk-platform)
    drop_input(android-ndk-platform)
    push("-DANDROID_PLATFORM=${INPUT_android-ndk-platform}")
endif()
if(DEFINED INPUT_android-abis)
    if(INPUT_android-abis MATCHES ",")
        qtConfAddError("The -android-abis option cannot handle more than one ABI "
            "when building with CMake.")
    endif()
    translate_string_input(android-abis ANDROID_ABI)
endif()

if(DEFINED INPUT_ohos-abis)
    if(INPUT_ohos-abis MATCHES ",")
        qtConfAddError("The -ohos-abis option cannot handle more than one ABI "
            "when building with CMake.")
    endif()
    translate_string_input(ohos-abis OHOS_ABI)
endif()

drop_input(make)
drop_input(nomake)

check_qt_build_parts(nomake)
check_qt_build_parts(make)

drop_input(debug)
drop_input(release)
drop_input(debug_and_release)
drop_input(force_debug_info)
unset(build_configs)
if(INPUT_debug)
    set(build_configs Debug)
elseif("${INPUT_debug}" STREQUAL "no")
    set(build_configs Release)
elseif(INPUT_debug_and_release)
    set(build_configs Release Debug)
endif()
if(INPUT_force_debug_info)
    list(TRANSFORM build_configs REPLACE "^Release$" "RelWithDebInfo")
endif()

# Code coverage handling
drop_input(gcov)
if(INPUT_gcov)
    if(NOT "${INPUT_coverage}" STREQUAL "")
        if(NOT "${INPUT_coverage}" STREQUAL "gcov")
        qtConfAddError("The -gcov argument is provided, but -coverage is set"
            " to ${INPUT_coverage}")
        endif()
    else()
        set(INPUT_coverage "gcov")
        list(APPEND config_inputs coverage)
    endif()
endif()
if(NOT "${INPUT_coverage}" STREQUAL "")
    if(build_configs)
        if(NOT "Debug" IN_LIST build_configs)
            qtConfAddError("The -coverage argument requires Qt configured with 'Debug' config.")
        endif()
    else()
        set(build_configs "Debug")
    endif()
endif()

list(LENGTH build_configs nr_of_build_configs)
if(nr_of_build_configs EQUAL 1 AND NOT multi_config)
    push("-DCMAKE_BUILD_TYPE=${build_configs}")
elseif(nr_of_build_configs GREATER 1 OR multi_config)
    set(multi_config ON)
    string(REPLACE ";" "[[;]]" escaped_build_configs "${build_configs}")
    # We must not use the push macro here to avoid variable expansion.
    # That would destroy our escaping.
    list(APPEND cmake_args "-DCMAKE_CONFIGURATION_TYPES=${escaped_build_configs}")
endif()

drop_input(ltcg)
if("${INPUT_ltcg}" STREQUAL "yes")
    foreach(config ${build_configs})
        string(TOUPPER "${config}" ucconfig)
        if(NOT ucconfig STREQUAL "DEBUG")
            push("-DCMAKE_INTERPROCEDURAL_OPTIMIZATION_${ucconfig}=ON")
        endif()
    endforeach()
endif()

# Handle -D, -I and friends.
translate_list_input(defines QT_EXTRA_DEFINES)
translate_list_input(fpaths QT_EXTRA_FRAMEWORKPATHS)
translate_list_input(includes QT_EXTRA_INCLUDEPATHS)
translate_list_input(lpaths QT_EXTRA_LIBDIRS)
translate_list_input(rpaths QT_EXTRA_RPATHS)

if(cmake_file_api OR (developer_build AND NOT DEFINED cmake_file_api))
    foreach(file cache-v2 cmakeFiles-v1 codemodel-v2 toolchains-v1)
        file(WRITE "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/${file}" "")
    endforeach()
endif()

# Translate unhandled input variables to either -DINPUT_foo=value or -DFEATURE_foo=ON/OFF. If the
# input's name matches a feature name and the corresponding command-line option's type is boolean
# then we assume it's controlling a feature.
foreach(input ${config_inputs})
    qt_feature_normalize_name("${input}" cmake_input)
    if("${commandline_input_${input}_type}" STREQUAL "boolean"
        AND input IN_LIST commandline_known_features)
        translate_boolean_input("${input}" "FEATURE_${cmake_input}")
    else()
        push("-DINPUT_${cmake_input}=${INPUT_${input}}")
    endif()
endforeach()

if(no_prefix_option AND DEFINED INPUT_prefix)
    qtConfAddError("Can't specify both -prefix and -no-prefix options at the same time.")
endif()

if(NOT generator AND auto_detect_generator)
    find_program(ninja ninja)
    if(ninja)
        set(generator Ninja)
        if(multi_config)
            string(APPEND generator " Multi-Config")
        endif()
    else()
        if(CMAKE_HOST_UNIX)
            set(generator "Unix Makefiles")
        elseif(CMAKE_HOST_WIN32)
            find_program(msvc_compiler cl.exe)
            if(msvc_compiler)
                set(generator "NMake Makefiles")
                find_program(jom jom)
                if(jom)
                    string(APPEND generator " JOM")
                endif()
            else()
                set(generator "MinGW Makefiles")
            endif()
        endif()
    endif()
endif()

if(multi_config
  AND NOT "${generator}" STREQUAL "Xcode"
  AND NOT "${generator}" STREQUAL "Ninja Multi-Config"
  AND NOT "${generator}" MATCHES "^Visual Studio")
    message(FATAL_ERROR "Multi-config build is only supported by Xcode, Ninja Multi-Config and \
Visual Studio generators. Current generator is \"${generator}\".
Note: Use '-cmake-generator <generator name>' option to specify the generator manually.")
endif()

if(generator)
    push(-G "${generator}")
endif()

# Add CMake variable assignments near the end to allow users to overwrite what configure sets.
foreach(arg IN LISTS cmake_var_assignments)
    push("-D${arg}")
endforeach()

push("${MODULE_ROOT}")

if(INPUT_sysroot)
    qtConfAddWarning("The -sysroot option is deprecated and no longer has any effect. "
                     "It is recommended to use a toolchain file instead, i.e., "
                     "-DCMAKE_TOOLCHAIN_FILE=<filename>. "
                     "Alternatively, you may use -DCMAKE_SYSROOT option "
                     "to pass the sysroot to CMake.\n")
endif()

function(qt_generate_vcpkg_manifest)
    # Check if manifest was initialized (i.e., any dependencies were found)
    get_property(manifest_initialized GLOBAL PROPERTY _QT_VCPKG_MANIFEST_JSON SET)
    if(NOT manifest_initialized)
        # For repositories that don't need 3rd-party libs, generate an empty vcpkg.json.
        qt_init_vcpkg_manifest_if_needed()
    endif()

    # Write the manifest file
    set(manifest_file "${CMAKE_CURRENT_BINARY_DIR}/vcpkg.json")
    qt_vcpkg_write_manifest("${manifest_file}")
endfunction()

function(qt_select_vcpkg_features out_var cmake_args)
    # Extract enabled/disabled features from cmake_args.
    set(normalized_enabled_features "")
    set(normalized_disabled_features "")
    foreach(arg IN LISTS cmake_args)
        if(arg MATCHES "^-DFEATURE_([^=]+)=(.*)")
            if(CMAKE_MATCH_2)
                list(APPEND normalized_enabled_features "${CMAKE_MATCH_1}")
            else()
                list(APPEND normalized_disabled_features "${CMAKE_MATCH_1}")
            endif()
        endif()
    endforeach()

    if(normalized_enabled_features STREQUAL "" AND normalized_disabled_features STREQUAL "")
        return()
    endif()

    # Convert normalized feature names back to original names
    set(enabled_features "")
    set(disabled_features "")
    foreach(original_feature IN LISTS commandline_known_features)
        qt_feature_normalize_name("${original_feature}" normalized_feature)
        if(normalized_feature IN_LIST normalized_enabled_features)
            list(APPEND enabled_features "${original_feature}")
        endif()
        if(normalized_feature IN_LIST normalized_disabled_features)
            list(APPEND disabled_features "${original_feature}")
        endif()
    endforeach()

    set(manifest_file_path "${CMAKE_CURRENT_BINARY_DIR}/vcpkg.json")
    if(NOT EXISTS "${manifest_file_path}")
        return()
    endif()

    file(READ "${manifest_file_path}" json)
    qt_vcpkg_set_internal_manifest_data("${json}")

    # Enable all default vcpkg feature that were not explicitly disabled.
    qt_vcpkg_get_default_features(vcpkg_default_features "${json}")
    set(vcpkg_features_to_enable ${vcpkg_default_features})
    list(REMOVE_ITEM vcpkg_features_to_enable ${disabled_features})

    # Enable all vcpkg features that were explicitly enabled. Filter out non-vcpkg features.
    qt_vcpkg_get_features(all_vcpkg_features "${json}")
    set(enabled_features_without_vcpkg_features "${enabled_features}")
    list(REMOVE_ITEM enabled_features_without_vcpkg_features ${all_vcpkg_features})
    list(APPEND vcpkg_features_to_enable ${enabled_features})
    list(REMOVE_ITEM vcpkg_features_to_enable ${enabled_features_without_vcpkg_features})
    list(REMOVE_DUPLICATES vcpkg_features_to_enable)

    # If we're enabling exactly the default features, we don't have to explicitly enable them.
    if(vcpkg_features_to_enable STREQUAL vcpkg_default_features)
        return()
    endif()

    list(APPEND cmake_args "-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON")
    list(JOIN vcpkg_features_to_enable "[[;]]" manifest_args)
    list(APPEND cmake_args "-DVCPKG_MANIFEST_FEATURES=${manifest_args}")
    set("${out_var}" "${cmake_args}" PARENT_SCOPE)
endfunction()

if(generate_vcpkg_manifest)
    qt_generate_vcpkg_manifest()
endif()

if(use_vcpkg)
    qt_select_vcpkg_features(cmake_args "${cmake_args}")
endif()

# Restore the escaped semicolons in arguments that are lists
list(TRANSFORM cmake_args REPLACE "\\[\\[;\\]\\]" "\\\\;")

if(dry_run)
    if(CMAKE_COMMAND MATCHES " ")
        set(pretty_command_line "\"${CMAKE_COMMAND}\"")
    else()
        set(pretty_command_line "${CMAKE_COMMAND}")
    endif()
    foreach(arg IN LISTS cmake_args)
        if(arg MATCHES "[ ;]")
            set(arg "\"${arg}\"")
        endif()
        string(APPEND pretty_command_line " ${arg}")
    endforeach()
    message("${pretty_command_line}")
    return()
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" ${cmake_args}
    COMMAND_ECHO STDOUT
    RESULT_VARIABLE exit_code)
if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR "CMake exited with code ${exit_code}.")
endif()
