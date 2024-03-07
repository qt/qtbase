# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The common implementation of qt_configure_file functionality.
macro(qt_configure_file_impl)
    if(NOT arg_OUTPUT)
        message(FATAL_ERROR "No output file provided to qt_configure_file.")
    endif()

    # We use this check for the cases when the specified CONTENT is empty. The value of arg_CONTENT
    # is undefined, but we still want to create a file with empty content.
    if(NOT "CONTENT" IN_LIST arg_KEYWORDS_MISSING_VALUES)
        if(arg_INPUT)
            message(WARNING "Both CONTENT and INPUT are specified. CONTENT will be used to generate"
                " output")
        endif()
        set(template_name "QtFileConfigure.txt.in")
        # When building qtbase, use the source template file.
        # Otherwise use the installed file (basically wherever Qt6 package is found).
        # This should work for non-prefix and superbuilds as well.
        if(QtBase_SOURCE_DIR)
            set(input_file "${QtBase_SOURCE_DIR}/cmake/${template_name}")
        else()
            set(input_file "${_qt_6_config_cmake_dir}/${template_name}")
        endif()
        set(__qt_file_configure_content "${arg_CONTENT}")
    elseif(arg_INPUT)
        set(input_file "${arg_INPUT}")
    else()
        message(FATAL_ERROR "No input value provided to qt_configure_file.")
    endif()

    configure_file("${input_file}" "${arg_OUTPUT}" @ONLY)
endmacro()

# qt_configure_file(OUTPUT output-file <INPUT input-file | CONTENT content>)
# input-file is relative to ${CMAKE_CURRENT_SOURCE_DIR}
# output-file is relative to ${CMAKE_CURRENT_BINARY_DIR}
#
# This function is similar to file(GENERATE OUTPUT) except it writes the content
# to the file at configure time, rather than at generate time.
#
# TODO: Once we require 3.18+, this can use file(CONFIGURE) in its implementation,
# or maybe its usage can be replaced by file(CONFIGURE). Until then, it  uses
# configure_file() with a generic input file as source, when used with the CONTENT
# signature.
function(qt_configure_file)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "OUTPUT;INPUT;CONTENT" "")
    qt_configure_file_impl()
endfunction()

# A version of cmake_parse_arguments that makes sure all arguments are processed and errors out
# with a message about ${type} having received unknown arguments.
#
# TODO: Remove when all usage of qt_parse_all_arguments were replaced by
# cmake_parse_all_arguments(PARSEARGV) instances
macro(qt_parse_all_arguments result type flags options multiopts)
    cmake_parse_arguments(${result} "${flags}" "${options}" "${multiopts}" ${ARGN})
    if(DEFINED ${result}_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments were passed to ${type} (${${result}_UNPARSED_ARGUMENTS}).")
    endif()
endmacro()

# Print all variables defined in the current scope.
macro(qt_debug_print_variables)
    cmake_parse_arguments(__arg "DEDUP" "" "MATCH;IGNORE" ${ARGN})
    message("Known Variables:")
    get_cmake_property(__variableNames VARIABLES)
    list (SORT __variableNames)
    if (__arg_DEDUP)
        list(REMOVE_DUPLICATES __variableNames)
    endif()

    foreach(__var ${__variableNames})
        set(__ignore OFF)
        foreach(__i ${__arg_IGNORE})
            if(__var MATCHES "${__i}")
                set(__ignore ON)
                break()
            endif()
        endforeach()

        if (__ignore)
            continue()
        endif()

        set(__show OFF)
        foreach(__i ${__arg_MATCH})
            if(__var MATCHES "${__i}")
                set(__show ON)
                break()
            endif()
        endforeach()

        if (__show)
            message("    ${__var}=${${__var}}.")
        endif()
    endforeach()
endmacro()

macro(assert)
    if (${ARGN})
    else()
        message(FATAL_ERROR "ASSERT: ${ARGN}.")
    endif()
endmacro()

# Takes a list of path components and joins them into one path separated by forward slashes "/",
# and saves the path in out_var.
function(qt_path_join out_var)
    _qt_internal_path_join(path ${ARGN})
    set(${out_var} ${path} PARENT_SCOPE)
endfunction()

function(qt_remove_args out_var)
    _qt_internal_remove_args(result ${ARGN})
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Creates a regular expression that exactly matches the given string
# Found in https://gitlab.kitware.com/cmake/cmake/issues/18580
function(qt_re_escape out_var str)
    string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" regex "${str}")
    set(${out_var} ${regex} PARENT_SCOPE)
endfunction()

# Input: string
# Output: regex string to match the string case insensitively
# Example: "Release" -> "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$"
#
# Regular expressions like this are used in cmake_install.cmake files for case-insensitive string
# comparison.
function(qt_create_case_insensitive_regex out_var input)
    set(result "^(")
    string(LENGTH "${input}" n)
    math(EXPR n "${n} - 1")
    foreach(i RANGE 0 ${n})
        string(SUBSTRING "${input}" ${i} 1 c)
        string(TOUPPER "${c}" uc)
        string(TOLOWER "${c}" lc)
        string(APPEND result "[${uc}${lc}]")
    endforeach()
    string(APPEND result ")$")
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Gets a target property, and returns "" if the property was not found
function(qt_internal_get_target_property out_var target property)
    get_target_property(result "${target}" "${property}")
    if("${result}" STREQUAL "result-NOTFOUND")
        set(result "")
    endif()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Creates a wrapper ConfigVersion.cmake file to be loaded by find_package when checking for
# compatible versions. It expects a ConfigVersionImpl.cmake file in the same directory which will
# be included to do the regular version checks.
# The version check result might be overridden by the wrapper.
# package_name is used by the content of the wrapper file to include the basic package version file.
#   example: Qt6Gui
# out_path should be the build path where the write the file.
function(qt_internal_write_qt_package_version_file package_name out_path)
    set(extra_code "")

    # Need to check for FEATURE_developer_build as well, because QT_FEATURE_developer_build is not
    # yet available when configuring the file for the BuildInternals package.
    if(FEATURE_developer_build OR QT_FEATURE_developer_build)
        string(APPEND extra_code "
# Disabling version check because Qt was configured with -developer-build.
set(__qt_disable_package_version_check TRUE)
set(__qt_disable_package_version_check_due_to_developer_build TRUE)")
    endif()

    configure_file("${QT_CMAKE_DIR}/QtCMakePackageVersionFile.cmake.in" "${out_path}" @ONLY)
endfunction()
