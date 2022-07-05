# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from evdevtouch.pro.

#####################################################################
## QEvdevTouchScreenPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEvdevTouchScreenPlugin
    OUTPUT_NAME qevdevtouchplugin
    PLUGIN_TYPE generic
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::InputSupportPrivate
)

#### Keys ignored in scope 1:.:.:evdevtouch.pro:<TRUE>:
# OTHER_FILES = "evdevtouch.json"
# PLUGIN_EXTENDS = "-"
