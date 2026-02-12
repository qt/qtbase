# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from wl-shell.pro.

#####################################################################
## WlShellIntegrationPrivate Module:
#####################################################################

qt_internal_add_module(WlShellIntegrationPrivate
    CONFIG_MODULE_NAME wl_shell_integration
    INTERNAL_MODULE
    SOURCES
        qwaylandwlshellintegration.cpp qwaylandwlshellintegration_p.h
        qwaylandwlshellsurface.cpp qwaylandwlshellsurface_p.h
    LIBRARIES
        Qt::GuiPrivate
        Qt::WaylandClientPrivate
        Wayland::Client
    NO_GENERATE_CPP_EXPORTS
    QT_LICENSE_ID QT_COMMERCIAL_OR_LGPL3
)

#### Keys ignored in scope 2:.:.:wl-shell-integration.pro:<TRUE>:
# MODULE = "wl_shell_integration"

## Scopes:
#####################################################################

qt_internal_extend_target(WlShellIntegrationPrivate CONDITION QT_FEATURE_xkbcommon
    LIBRARIES
        XKB::XKB
)
#####################################################################
## QWaylandWlShellIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QWaylandWlShellIntegrationPlugin
    OUTPUT_NAME wl-shell-plugin
    PLUGIN_TYPE wayland-shell-integration
    SOURCES
        main.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::WaylandClientPrivate
        Qt::WlShellIntegrationPrivate
)

#### Keys ignored in scope 4:.:.:wl-shell-plugin.pro:<TRUE>:
# OTHER_FILES = "wl-shell.json"
