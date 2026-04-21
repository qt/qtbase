# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generates the generator expression that converts the 'target'
# QT_ANDROID_PERMISSIONS property to the specific 'type'.
#
# It's expected that each element in QT_ANDROID_PERMISSIONS list has specific
# format:
# <name>\;<permission>\\\;<extra1>\;<value>\\\;<extra2>\;<value>
#
# Synopsis
#   _qt_internal_android_convert_permissions(out_var target <JSON|XML>)
#
# Arguments
#
# `out_var`
#    The name of the variable where the resulting generator expression is
#    stored.
#
# `target`
#    The name of the target.
#
# `JSON`
#   Generate JSON array known by androiddeployqt.
#
# `XML`
#   Generate XML content compatible with AndroidManifest.xml.
#
# `DEPENDENCIESXML`
#   Generate XML content compatible with <module>-android-dependencies.xml.
#   This format doesn't produce generator expression, so it should be used in
#   the scope finalizer.
function(_qt_internal_android_convert_permissions out_var target type)
    if(type STREQUAL "DEPENDENCIESXML")
        set(output "")
        get_target_property(permissions ${target} QT_ANDROID_PERMISSIONS)
        if(NOT permissions)
            return()
        endif()
        foreach(permission IN LISTS permissions)
            list(JOIN permission "=\"" permission)
            list(LENGTH permission permission_length)
            if(permission_length LESS 1)
                message(FATAL_ERROR "Invalid QT_ANDROID_PERMISSIONS format for target"
                    " ${target}: ${arg_PERMISSIONS}")
            endif()

            list(GET permission 0 permission_name)
            string(APPEND output "<permission ${permission_name}\"")

            math(EXPR permission_length "${permission_length} - 1")
            set(extras_string "")
            if(permission_length GREATER_EQUAL 1)
                set(attributes "")
                foreach(i RANGE 1 ${permission_length})
                    list(GET permission ${i} permission_attribute)
                    list(APPEND attributes "android:${permission_attribute}'")
                endforeach()
                list(JOIN attributes " " attributes)
                string(REPLACE "\"" "'" attributes "${attributes}")
                set(extras_string " extras=\"${attributes}\"")
            endif()
            string(APPEND output "${extras_string}/>\n")
        endforeach()
        set(${out_var} "${output}" PARENT_SCOPE)
        return()
    endif()

    set(permissions_property "$<TARGET_PROPERTY:${target},QT_ANDROID_PERMISSIONS>")
    set(permissions_genex "$<$<BOOL:${permissions_property}>:")
    if(type STREQUAL "JSON")
        set(pref "{ \"")
        set(post "\" }")
        set(indent "\n      ")
        string(APPEND permissions_genex
            "  \"permissions\": [${indent}$<JOIN:"
                "$<JOIN:"
                    "${pref}$<JOIN:"
                        "${permissions_property},"
                        "${post}$<COMMA>${indent}${pref}"
                    ">${post},"
                    "\": \""
                ">,"
                "\"$<COMMA> \""
            ">\n    ]$<COMMA>\n"
        )
    elseif(type STREQUAL "XML")
        set(pref "<uses-permission\n android:")
        set(post "' /$<ANGLE-R>\n")
        string(APPEND permissions_genex
            "$<JOIN:"
                "$<JOIN:"
                    "${pref}$<JOIN:"
                        "${permissions_property},"
                        "${post}${pref}"
                    ">${post}\n,"
                    "='"
                ">,"
                "' android:"
            ">"
        )
    else()
        message(FATAL_ERROR "Invalid type ${type}. Supported types: JSON, XML")
    endif()
    string(APPEND permissions_genex ">")

    set(${out_var} "${permissions_genex}" PARENT_SCOPE)
endfunction()

# Add the specific Android permission to the target. The permission is stored
# in the QT_ANDROID_PERMISSIONS property(the property is not a public API)
# and has the following format:
#    <name>\;<permission>\\\;<extra1>\;<value>\\\;<extra2>\;<value>
#
# Synopsis
#   _qt_internal_add_android_permission(target NAME <permission>
#       ATTRIBUTES <extra1> <value1>
#           [<extra2> <value2>]...
#   )
#
# Arguments
#
# `target`
#   The Android target.
#
# `NAME`
#   The permission name. E.g. 'android.permission.CAMERA'.
#
# `ATTRIBUTES`
#   Extra permission attribute key-value pairs.
#   See https://developer.android.com/guide/topics/manifest/uses-permission-element
#   for details.
function(_qt_internal_add_android_permission target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Empty or invalid target for adding Android permission: (${target})")
    endif()

    cmake_parse_arguments(arg "" "NAME" "ATTRIBUTES" ${ARGN})

    if(NOT arg_NAME)
        message(FATAL_ERROR "NAME for adding Android permission cannot be empty (${target})")
    endif()

    set(permission_entry "name\;${arg_NAME}")
    if(arg_ATTRIBUTES)
        # Permission with additional attributes
        list(LENGTH arg_ATTRIBUTES attributes_len)
        math(EXPR attributes_modulus "${attributes_len} % 2")
        if(NOT (attributes_len GREATER 1 AND attributes_modulus EQUAL 0))
            message(FATAL_ERROR "Android permission: ${arg_NAME} attributes: ${arg_ATTRIBUTES}"
                                " must be name-value pairs (for example: minSdkVersion 30)")
        endif()
        # Combine name-value pairs
        set(index 0)
        while(index LESS attributes_len)
            list(GET arg_ATTRIBUTES ${index} name)
            math(EXPR index "${index} + 1")
            list(GET arg_ATTRIBUTES ${index} value)
            string(APPEND permission_entry "\\\;${name}\;${value}")
            math(EXPR index "${index} + 1")
        endwhile()
    endif()

    # Append the permission to the target's property
    set_property(TARGET ${target} APPEND PROPERTY QT_ANDROID_PERMISSIONS "${permission_entry}")
endfunction()

# Merges QT_ANDROID_PERMISSIONS from each SOURCE_TARGETS source into target's own property.
#
# Synopsis
#   _qt_internal_android_collect_permissions(target
#       SOURCE_TARGETS <source> [<source>...])
function(_qt_internal_android_collect_permissions target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCE_TARGETS")
    if(NOT arg_SOURCE_TARGETS)
        return()
    endif()

    get_target_property(existing ${target} QT_ANDROID_PERMISSIONS)

    set(new_entries "")
    foreach(src_target IN LISTS arg_SOURCE_TARGETS)
        if(NOT TARGET "${src_target}")
            continue()
        endif()
        get_target_property(src_permissions "${src_target}" QT_ANDROID_PERMISSIONS)
        if(NOT src_permissions)
            continue()
        endif()
        foreach(entry IN LISTS src_permissions)
            if("${entry}" IN_LIST existing OR "${entry}" IN_LIST new_entries)
                continue()
            endif()
            string(REPLACE ";" [[\;]] entry_escaped "${entry}")
            if(new_entries)
                string(APPEND new_entries ";${entry_escaped}")
            else()
                set(new_entries "${entry_escaped}")
            endif()
        endforeach()
    endforeach()

    if(new_entries)
        set_property(TARGET ${target} APPEND PROPERTY QT_ANDROID_PERMISSIONS "${new_entries}")
    endif()
endfunction()
