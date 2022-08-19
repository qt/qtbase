# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qvkgen.pro.

#####################################################################
## qvkgen Tool:
#####################################################################

qt_get_tool_target_name(target_name qvkgen)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Vulkan Header Generator"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Gui # special case
    SOURCES
        qvkgen.cpp
    LIBRARIES
        Qt::Core # special case
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qvkgen.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt Vulkan Header Generator"
# _OPTION = "host_build"
