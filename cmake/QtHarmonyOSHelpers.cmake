# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# HarmonyOS specific functions/macros/properties required for building Qt Modules
#

# Public-ish wrapper used by qtbase modules to declare a HarmonyOS permission.
# In addition to recording the permission on the target via the underlying
# helper, this also adds _qt_harmonyos_permissions to EXPORT_PROPERTIES so the
# property survives the install + find_package round-trip and can be read
# back from imported Qt module targets in user projects.
function(qt_internal_add_harmonyos_permission target)
    _qt_internal_add_harmonyos_permission(${ARGV})
    get_target_property(export_props ${target} EXPORT_PROPERTIES)
    if(NOT export_props OR NOT _qt_harmonyos_permissions IN_LIST export_props)
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_harmonyos_permissions)
    endif()
endfunction()

function(qt_internal_add_harmonyos_blocking_test_dialog target)
    _qt_internal_add_harmonyos_blocking_test_dialog(${ARGV})
    get_target_property(export_props ${target} EXPORT_PROPERTIES)
    if(NOT export_props OR NOT _qt_harmonyos_blocking_test_dialogs IN_LIST export_props)
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_harmonyos_blocking_test_dialogs)
    endif()
endfunction()

# The source, a file or a directory, is copied with its name into the rawfile
# directory that every test in the test bundle shares, under DESTINATION or at its
# root. Directories of the same name merge; a file provided twice, a file and a
# directory of the same name, or two names differing only in case fail the
# deployment of the whole bundle, so keep a test's files under its own
# subdirectory. A generated source needs the test to depend on the target that
# generates it.
function(qt_internal_add_harmonyos_test_rawfiles target source)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Empty or invalid target for adding HarmonyOS test rawfiles: (${target})")
    endif()
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "DESTINATION" "")
    _qt_internal_validate_all_args_are_parsed(arg)
    cmake_path(SET destination NORMALIZE "${arg_DESTINATION}")
    if(IS_ABSOLUTE "${destination}" OR destination MATCHES "^\\.\\.(/|$)")
        message(FATAL_ERROR
            "DESTINATION must be a path inside the rawfile directory: ${arg_DESTINATION}")
    endif()
    get_filename_component(absolute_source "${source}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    _qt_internal_json_escape_content("${absolute_source}" escaped_source)
    _qt_internal_json_escape_content("${destination}" escaped_destination)
    set(rawfile_entry "{}")
    string(JSON rawfile_entry SET "${rawfile_entry}" source "\"${escaped_source}\"")
    string(JSON rawfile_entry SET "${rawfile_entry}" destination "\"${escaped_destination}\"")
    set_property(TARGET ${target} APPEND PROPERTY _qt_harmonyos_test_rawfiles "${rawfile_entry}")
endfunction()
