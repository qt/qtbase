# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(Tslib MODULE)

#####################################################################
## QTsLibPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QTsLibPlugin
    OUTPUT_NAME qtslibplugin
    PLUGIN_TYPE generic
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
    LIBRARIES
        PkgConfig::Tslib
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::InputSupportPrivate
)
