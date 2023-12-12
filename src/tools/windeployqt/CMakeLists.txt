# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## windeployqt Tool:
#####################################################################

qt_get_tool_target_name(target_name windeployqt)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Core
    USER_FACING
    INSTALL_VERSIONED_LINK
    TARGET_DESCRIPTION "Qt Windows Deployment Tool"
    SOURCES
        qmlutils.cpp qmlutils.h
        qtmoduleinfo.cpp qtmoduleinfo.h
        qtplugininfo.cpp qtplugininfo.h
        utils.cpp utils.h
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
        QT_NO_QPAIR
    LIBRARIES
        Qt::CorePrivate
)
qt_internal_return_unless_building_tools()

qt_internal_extend_target(${target_name} CONDITION WIN32
    PUBLIC_LIBRARIES
        shlwapi
)
