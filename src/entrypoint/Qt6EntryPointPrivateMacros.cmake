# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Builds the genex predicate that gates linking against EntryPointPrivate.
#
# Called with just an out-var, it emits the one-arg $<TARGET_PROPERTY:prop>
# form, which evaluates against the consuming "head" target. That's what we
# want from INTERFACE_LINK_LIBRARIES, where CMake resolves the head target
# for us.
#
# Called with an explicit target, it emits the two-arg
# $<TARGET_PROPERTY:tgt,prop> form. This is needed for XCODE_ATTRIBUTE_*
# values, since CMake's Xcode generator evaluates them without propagating
# a head target, so the one-arg form would silently expand to empty.
function(_qt_internal_get_entrypoint_conditions out_var)
    if(ARGC GREATER 1)
        set(prefix "${ARGV1},")
    else()
        set(prefix "")
    endif()
    set(genex_bool_prop "$<BOOL:$<TARGET_PROPERTY:${prefix}QT_NO_ENTRYPOINT>>")
    set(genex_bool_prop_old "$<BOOL:$<TARGET_PROPERTY:${prefix}qt_no_entrypoint>>")
    set(conds "$<NOT:$<OR:${genex_bool_prop},${genex_bool_prop_old}>>")
    list(APPEND conds "$<STREQUAL:$<TARGET_PROPERTY:${prefix}TYPE>,EXECUTABLE>")
    if(WIN32)
        list(APPEND conds "$<BOOL:$<TARGET_PROPERTY:${prefix}WIN32_EXECUTABLE>>")
    endif()
    list(JOIN conds "," conds)
    set(${out_var} "$<AND:${conds}>" PARENT_SCOPE)
endfunction()
