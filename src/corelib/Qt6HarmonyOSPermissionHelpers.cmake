# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Add the specific HarmonyOS permission to the target. The permission is stored
# in the QT_HARMONYOS_PERMISSIONS property (the property is not a public API)
# as a CMake list, where each element is a JSON object string built with
# string(JSON SET) of the following shape:
#
#   {"name":"ohos.permission.CAMERA","reason":"$string:cam_reason",
#    "usedScene":{"abilities":["QAbility"],"when":"always"}}
#
# Synopsis
#   _qt_internal_add_harmonyos_permission(target NAME <permission>
#       [REASON <string-or-$string:ref>]
#       [USED_SCENE_ABILITIES <ability> [<ability>...]]
#       [USED_SCENE_WHEN inuse|always]
#   )
#
# Arguments
#
# `target`
#   The HarmonyOS target.
#
# `NAME`
#   The permission name. E.g. 'ohos.permission.CAMERA'.
#
# `REASON`
#   Optional reason string (or $string:<ref> resource reference) shown to the
#   user when requesting the permission.
#
# `USED_SCENE_ABILITIES`
#   Optional list of abilities that use this permission. Defaults to
#   QAbility (Qt's single UIAbility) if a usedScene block is being
#   emitted (i.e. REASON or USED_SCENE_WHEN was given).
#
# `USED_SCENE_WHEN`
#   Optional usage scenario: 'inuse' or 'always'. Defaults to 'inuse'
#   if a usedScene block is being emitted.
function(_qt_internal_add_harmonyos_permission target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Empty or invalid target for adding HarmonyOS permission: (${target})")
    endif()

    set(no_value_options "")
    set(single_value_options
        NAME
        REASON
        USED_SCENE_WHEN
    )
    set(multi_value_options
        USED_SCENE_ABILITIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(NOT arg_NAME)
        message(FATAL_ERROR
            "NAME for adding HarmonyOS permission cannot be empty (${target})")
    endif()

    # Build a JSON object string starting from {} and using string(JSON SET).
    set(permission_entry "{}")
    string(JSON permission_entry SET "${permission_entry}" name "\"${arg_NAME}\"")

    if(DEFINED arg_REASON)
        string(JSON permission_entry SET "${permission_entry}" reason "\"${arg_REASON}\"")
    endif()

    # Build usedScene whenever the permission prompts (REASON given) or any
    # usedScene field was set explicitly. Apply sensible defaults — Qt apps
    # have a single QAbility entry point, and "inuse" matches the
    # foreground-only access pattern that fits most user_grant permissions.
    if(DEFINED arg_REASON OR arg_USED_SCENE_ABILITIES OR DEFINED arg_USED_SCENE_WHEN)
        if(NOT arg_USED_SCENE_ABILITIES)
            set(arg_USED_SCENE_ABILITIES "QAbility")
        endif()
        if(NOT DEFINED arg_USED_SCENE_WHEN)
            set(arg_USED_SCENE_WHEN "inuse")
        endif()

        string(JSON permission_entry SET "${permission_entry}" usedScene "{}")

        # Initialize abilities as empty array, then SET each element by index.
        string(JSON permission_entry SET "${permission_entry}"
            usedScene abilities "[]")
        set(idx 0)
        foreach(ability IN LISTS arg_USED_SCENE_ABILITIES)
            string(JSON permission_entry SET "${permission_entry}"
                usedScene abilities ${idx} "\"${ability}\"")
            math(EXPR idx "${idx} + 1")
        endforeach()

        string(JSON permission_entry SET "${permission_entry}"
            usedScene when "\"${arg_USED_SCENE_WHEN}\"")
    endif()

    # Append the permission JSON object string to the target's property.
    # The string is multi-line (string(JSON SET) emits pretty-printed output);
    # JSON is whitespace-insensitive so consumers happily parse either form.
    set_property(TARGET ${target} APPEND PROPERTY
        QT_HARMONYOS_PERMISSIONS "${permission_entry}")
endfunction()
