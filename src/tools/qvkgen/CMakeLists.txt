# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qvkgen Tool:
#####################################################################

qt_get_tool_target_name(target_name qvkgen)
qt_internal_add_tool(${target_name}
    REQUIRED_FOR_DOCS
    TARGET_DESCRIPTION "Qt Vulkan Header Generator"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Gui
    SOURCES
        qvkgen.cpp
    LIBRARIES
        Qt::Core
)
qt_internal_return_unless_building_tools()
