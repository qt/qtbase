# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# qt_configure_file(OUTPUT output-file <INPUT input-file | CONTENT content>)
# input-file is relative to ${CMAKE_CURRENT_SOURCE_DIR}
# output-file is relative to ${CMAKE_CURRENT_BINARY_DIR}
#
# This function is the universal replacement for file(CONFIGURE CMake command.
function(qt_configure_file)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "OUTPUT;INPUT;CONTENT" "")
    if("CONTENT" IN_LIST ARGV)
        if(arg_INPUT)
            message(WARNING "Both CONTENT and INPUT are specified. CONTENT will be used to generate"
                " output")
        endif()
        _qt_internal_configure_file(CONFIGURE OUTPUT "${arg_OUTPUT}" CONTENT "${arg_CONTENT}")
    elseif(arg_INPUT)
        _qt_internal_configure_file(CONFIGURE OUTPUT "${arg_OUTPUT}" INPUT "${arg_INPUT}")
    else()
        message(FATAL_ERROR "No input value provided to _qt_internal_configure_file.")
    endif()
endfunction()

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
    set(opt_args
        ALLOW_OVERRIDE_FILES
    )
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(extra_code "")

    # Need to check for FEATURE_developer_build as well, because QT_FEATURE_developer_build is not
    # yet available when configuring the file for the BuildInternals package.
    if(FEATURE_developer_build OR QT_FEATURE_developer_build)
        string(APPEND extra_code "
# Disabling version check because Qt was configured with -developer-build.
set(__qt_disable_package_version_check TRUE)
set(__qt_disable_package_version_check_due_to_developer_build TRUE)")
    endif()

    set(QT_PACKAGE_NAME "${package_name}")

    set(QT_VERSION_OVERRIDE_CODE "")
    if(arg_ALLOW_OVERRIDE_FILES AND NOT QT_NO_INJECT_CONFIG_VERSION_OVERRIDE_FILES)
        # Read the contents of the file into a variable, which will be appended to the template.
        file(STRINGS "${QT_CMAKE_DIR}/QtCMakePackageVersionFileOverride.cmake.in"
            QT_VERSION_OVERRIDE_CODE)

        # Skip license
        while(QT_VERSION_OVERRIDE_CODE)
            list(POP_FRONT QT_VERSION_OVERRIDE_CODE line)
            if (line MATCHES "^# SPDX")
                list(POP_FRONT QT_VERSION_OVERRIDE_CODE line)
                break()
            endif()
        endwhile()

        string(REPLACE ";" "\n" QT_VERSION_OVERRIDE_CODE "${QT_VERSION_OVERRIDE_CODE}")

        # Need to replace template expansion variables.
        string(CONFIGURE "${QT_VERSION_OVERRIDE_CODE}" QT_VERSION_OVERRIDE_CODE @ONLY)
    endif()

    configure_file("${QT_CMAKE_DIR}/QtCMakePackageVersionFile.cmake.in" "${out_path}" @ONLY)
endfunction()
