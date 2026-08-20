# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## harmonyostestrunner App:
#####################################################################

qt_get_tool_target_name(target_name harmonyostestrunner)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt HarmonyOS Test Runner Tool"
    TOOLS_TARGET Core
    SOURCES
        hdc.cpp hdc.h
        shellhelpers.cpp shellhelpers.h
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Core
)
qt_internal_return_unless_building_tools()
set_target_properties(${target_name} PROPERTIES
    WIN32_EXECUTABLE FALSE
)
