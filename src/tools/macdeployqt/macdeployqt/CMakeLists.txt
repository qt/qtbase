# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## macdeployqt Tool:
#####################################################################

qt_get_tool_target_name(target_name macdeployqt)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Core
    USER_FACING
    INSTALL_VERSIONED_LINK
    TARGET_DESCRIPTION "Qt macOS Deployment Tool"
    SOURCES
        ../shared/shared.cpp
        main.cpp
    LIBRARIES
        ${FWCoreFoundation}
)
qt_internal_return_unless_building_tools()
