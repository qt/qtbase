# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QMinimalIntegrationPlugin Plugin:
#####################################################################

qt_find_package(WrapFreetype MODULE PROVIDED_TARGETS WrapFreetype::WrapFreetype)

qt_internal_add_plugin(QMinimalIntegrationPlugin
    OUTPUT_NAME qminimal
    PLUGIN_TYPE platforms
    DEFAULT_IF "minimal" IN_LIST QT_QPA_PLATFORMS
    SOURCES
        main.cpp
        qminimalbackingstore.cpp qminimalbackingstore.h
        qminimalintegration.cpp qminimalintegration.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(QMinimalIntegrationPlugin CONDITION QT_FEATURE_freetype
    LIBRARIES
        WrapFreetype::WrapFreetype
)
