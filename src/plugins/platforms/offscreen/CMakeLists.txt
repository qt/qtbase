# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QOffscreenIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOffscreenIntegrationPlugin
    OUTPUT_NAME qoffscreen
    PLUGIN_TYPE platforms
    DEFAULT_IF "offscreen" IN_LIST QT_QPA_PLATFORMS
    SOURCES
        main.cpp
        qoffscreencommon.cpp qoffscreencommon.h
        qoffscreenintegration.cpp qoffscreenintegration.h
        qoffscreenwindow.cpp qoffscreenwindow.h
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

qt_internal_extend_target(QOffscreenIntegrationPlugin CONDITION QT_FEATURE_opengl AND QT_FEATURE_xlib AND NOT QT_FEATURE_opengles2
    SOURCES
        qoffscreenintegration_x11.cpp qoffscreenintegration_x11.h
    LIBRARIES
        X11::X11
)
