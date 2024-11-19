# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qdbusxml2cpp Tool:
#####################################################################

qt_get_tool_target_name(target_name qdbusxml2cpp)
qt_internal_add_tool(${target_name}
    REQUIRED_FOR_DOCS
    TRY_RUN
    TARGET_DESCRIPTION "Qt D-Bus XML to C++ Compiler"
    TOOLS_TARGET DBus
    SOURCES
        qdbusxml2cpp.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)
qt_internal_return_unless_building_tools()
