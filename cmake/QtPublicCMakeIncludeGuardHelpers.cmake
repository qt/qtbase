# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Manual block to opt out of the include guard.
if(NOT QT_NO_CMAKE_INCLUDE_GUARD)
    include_guard(GLOBAL)
endif()

# An improved re-implementation of `include_guard()`
#
# Aside from the options that `include_guard` supports, which roughly map to `#pragma once` in C++
# (because it operates based on CMAKE_CURRENT_LIST_FILE), the function also supports a string-based
# GUARD_KEY option to implement an include guard similar to a C++ `#ifndef` include guard. This
# prevents including copies stored at different paths.
#
# The include guard can be opted out of, by setting QT_NO_CMAKE_INCLUDE_GUARD to ON. Then the macro
# is a no-op.
#
# Synopsis
#
#   __qt_internal_cmake_include_guard(
#     [GUARD_KEY <key>]
#     [LOCAL|DIRECTORY|GLOBAL]
#   )
#
# Arguments
#
# `GUARD_KEY`
#   A string key to identify the include guard. If specified, the include guard is implemented by
#   setting a global property with a name based on the key, and checking for its existence at the
#   call site. Currently only works when used in conjunction with `GLOBAL`.
#   Recommended, because it's more robust than path based checks.
#
# `LOCAL`
#   Equivalent to `include_guard()`.
#
# `DIRECTORY`
#   Equivalent to `include_guard(DIRECTORY)`.
#
# `GLOBAL`
#   Equivalent to `include_guard(GLOBAL)`.
#
# Notes
# The file providing the macro needs to be included early enough, to be available for use in
# files that use it.
macro(__qt_internal_cmake_include_guard)
    # Only parse and handle the arguments if the opt is not set.
    if(NOT QT_NO_CMAKE_INCLUDE_GUARD)
        set(_qt_cig_opt_args
            LOCAL
            DIRECTORY
            GLOBAL
        )
        set(_qt_cig_single_args
            GUARD_KEY
        )
        cmake_parse_arguments(_qt_cig_arg "${_qt_cig_opt_args}" "${_qt_cig_single_args}" "" ${ARGN})
        if(DEFINED _qt_cig_arg_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR "__qt_internal_cmake_include_guard "
                "unknown arguments: '${_qt_cig_arg_UNPARSED_ARGUMENTS}'")
        endif()

        if(_qt_cig_arg_GUARD_KEY)
            if(_qt_cig_arg_GLOBAL)
                # Use a global property to track when to exit early.
                set(_qt_cig_prop_name "_qt_cmake_include_guard_${_qt_cig_arg_GUARD_KEY}")
                get_property(_qt_cig_already_included GLOBAL PROPERTY "${_qt_cig_prop_name}")
                if(_qt_cig_already_included)
                    unset(_qt_cig_prop_name)
                    unset(_qt_cig_already_included)

                    # Returns out of the file where the macro was called.
                    return()
                endif()
                set_property(GLOBAL PROPERTY "${_qt_cig_prop_name}" "ON")
                unset(_qt_cig_prop_name)
                unset(_qt_cig_already_included)
            else()
                message(FATAL_ERROR "GUARD_KEY is currently only supported with the GLOBAL option.")
            endif()
        else()
            # For non GUARD_KEY guards, use the default cmake implementation which is based on the
            # file path / CMAKE_CURRENT_LIST_FILE.
            if(_qt_cig_arg_GLOBAL)
                include_guard(GLOBAL)
            elseif(_qt_cig_arg_DIRECTORY)
                include_guard(DIRECTORY)
            elseif(_qt_cig_arg_LOCAL)
                include_guard()
            else()
                message(FATAL_ERROR "One of LOCAL, DIRECTORY or GLOBAL option must be specified.")
            endif()
        endif()
    endif()
endmacro()
