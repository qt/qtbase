# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Declare a HarmonyOS system dialog that blocks this module's autotests until it is answered, so
# harmonyostestrunner can answer it. Stored in the private _qt_harmonyos_blocking_test_dialogs
# target property as a CMake list of JSON object strings:
#
#   {"name":"runtime-permission","prompt":"allow .+ to (discover|connect|access|use)",
#    "buttons":["^allow$","^confirm$"],"position":"rightmost"}
#
# Synopsis
#   _qt_internal_add_harmonyos_blocking_test_dialog(target NAME <name> PROMPT <regexp>
#       BUTTONS <regexp> [<regexp>...] [POSITION rightmost|leftmost]
#   )
#
# Arguments
#
# `NAME`
#   Identifies the dialog in the runner's log, e.g. 'runtime-permission'.
#
# `PROMPT`
#   Regular expression matched against every text on screen. When it matches, the dialog is
#   considered to be the one described here. The system renders that text in the device UI
#   language, so a declaration only matches on a device set to the language it was written for.
#   The declarations in Qt are English; on a device in another language the runner reports the
#   dialog it cannot match and presses nothing.
#
# `BUTTONS`
#   Regular expressions matched against the on-screen texts to find the buttons to press, in the
#   order they must be pressed. A pattern that matches nothing is skipped, which lets one rule cover
#   dialog variants that differ in how many buttons they take: the permission dialog appears both as
#   Deny/Allow, answered by pressing Allow, and as a list of choices followed by a separate Confirm.
#
# `POSITION`
#   Which of the matching buttons to press: 'rightmost' (default) or 'leftmost'. Needed for dialogs
#   whose buttons carry the same label, where the affirmative one can only be told apart by where it
#   sits.
function(_qt_internal_add_harmonyos_blocking_test_dialog target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Empty or invalid target for adding HarmonyOS system dialog: (${target})")
    endif()

    set(no_value_options "")
    set(single_value_options
        NAME
        PROMPT
        POSITION
    )
    set(multi_value_options
        BUTTONS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    foreach(required NAME PROMPT BUTTONS)
        if(NOT arg_${required})
            message(FATAL_ERROR
                "${required} for adding HarmonyOS system dialog cannot be empty (${target})")
        endif()
    endforeach()

    if(NOT arg_POSITION)
        set(arg_POSITION "rightmost")
    elseif(NOT arg_POSITION STREQUAL "rightmost" AND NOT arg_POSITION STREQUAL "leftmost")
        message(FATAL_ERROR
            "POSITION for HarmonyOS system dialog must be rightmost or leftmost (${target})")
    endif()

    _qt_internal_json_escape_content("${arg_NAME}" escaped_name)
    _qt_internal_json_escape_content("${arg_PROMPT}" escaped_prompt)

    set(dialog_entry "{}")
    string(JSON dialog_entry SET "${dialog_entry}" name "\"${escaped_name}\"")
    string(JSON dialog_entry SET "${dialog_entry}" prompt "\"${escaped_prompt}\"")
    set(buttons_json "[]")
    set(button_index 0)
    foreach(button IN LISTS arg_BUTTONS)
        _qt_internal_json_escape_content("${button}" escaped_button)
        string(JSON buttons_json SET "${buttons_json}" ${button_index} "\"${escaped_button}\"")
        math(EXPR button_index "${button_index} + 1")
    endforeach()
    string(JSON dialog_entry SET "${dialog_entry}" buttons "${buttons_json}")
    string(JSON dialog_entry SET "${dialog_entry}" position "\"${arg_POSITION}\"")

    set_property(TARGET ${target} APPEND PROPERTY
        _qt_harmonyos_blocking_test_dialogs "${dialog_entry}")
endfunction()
