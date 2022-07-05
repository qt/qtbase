# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from xdgdesktopportal.pro.

#####################################################################
## QXdgDesktopPortalThemePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QXdgDesktopPortalThemePlugin
    OUTPUT_NAME qxdgdesktopportal
    PLUGIN_TYPE platformthemes
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
        qxdgdesktopportalfiledialog.cpp qxdgdesktopportalfiledialog_p.h
        qxdgdesktopportaltheme.cpp qxdgdesktopportaltheme.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:xdgdesktopportal.pro:<TRUE>:
# PLUGIN_EXTENDS = "-"
