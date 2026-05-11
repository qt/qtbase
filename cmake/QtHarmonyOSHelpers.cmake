# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# HarmonyOS specific functions/macros/properties required for building Qt Modules
#

# Public-ish wrapper used by qtbase modules to declare a HarmonyOS permission.
# In addition to recording the permission on the target via the underlying
# helper, this also adds QT_HARMONYOS_PERMISSIONS to EXPORT_PROPERTIES so the
# property survives the install + find_package round-trip and can be read
# back from imported Qt module targets in user projects.
function(qt_internal_add_harmonyos_permission target)
    _qt_internal_add_harmonyos_permission(${ARGV})
    get_target_property(export_props ${target} EXPORT_PROPERTIES)
    if(NOT export_props OR NOT QT_HARMONYOS_PERMISSIONS IN_LIST export_props)
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES QT_HARMONYOS_PERMISSIONS)
    endif()
endfunction()
