# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## rcc Tool:
#####################################################################

qt_get_tool_target_name(target_name rcc)
qt_internal_add_tool(${target_name}
    TRY_RUN
    TARGET_DESCRIPTION "Qt Resource Compiler"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core
    SOURCES
        main.cpp
        rcc.cpp rcc.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
        QT_RCC
        QT_USE_NODISCARD_FILE_OPEN
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
)
qt_internal_return_unless_building_tools()

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION QT_FEATURE_zstd
    LIBRARIES
        WrapZSTD::WrapZSTD
)
