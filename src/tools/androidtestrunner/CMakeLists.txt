# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## androidtestrunner App:
#####################################################################

qt_get_tool_target_name(target_name androidtestrunner)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Android Test Runner Tool"
    TOOLS_TARGET Core
    SOURCES
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
)
qt_internal_return_unless_building_tools()
set_target_properties(${target_name} PROPERTIES
    WIN32_EXECUTABLE FALSE
)

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION windows
    COMPILE_OPTIONS
        -U__STRICT_ANSI__
)
