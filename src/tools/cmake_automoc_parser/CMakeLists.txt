# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## moc Tool:
#####################################################################

qt_get_tool_target_name(target_name cmake_automoc_parser)
qt_internal_add_tool(${target_name}
    CORE_LIBRARY Bootstrap
    TARGET_DESCRIPTION "Qt CMake AUTOMOC Parser"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core
    SOURCES
        main.cpp
    DEFINES
        QT_MOC
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
)
qt_internal_return_unless_building_tools()
