# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtpaths App:
#####################################################################

qt_get_tool_target_name(target_name qtpaths)
qt_internal_add_tool(${target_name}
    TRY_RUN
    TARGET_DESCRIPTION "Qt tool that provides the standard paths of the Qt framework"
    TOOLS_TARGET Core
    INSTALL_VERSIONED_LINK
    SOURCES
        qtpaths.cpp
    DEFINES
        QTPATHS_VERSION_STR="2.0"
)
qt_internal_return_unless_building_tools()

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION QT_FEATURE_settings
    LIBRARIES
        QtLibraryInfo
)

if(WIN32 AND TARGET ${target_name})
    set_target_properties(${target_name} PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()
