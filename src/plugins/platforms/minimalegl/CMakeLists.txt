# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(EGL)

#####################################################################
## QMinimalEglIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QMinimalEglIntegrationPlugin
    OUTPUT_NAME qminimalegl
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES minimalegl
    SOURCES
        main.cpp
        qminimaleglintegration.cpp qminimaleglintegration.h
        qminimaleglscreen.cpp qminimaleglscreen.h
        qminimaleglwindow.cpp qminimaleglwindow.h
    DEFINES
        QT_EGL_NO_X11
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        EGL::EGL
)

## Scopes:
#####################################################################

qt_internal_extend_target(QMinimalEglIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        qminimaleglbackingstore.cpp qminimaleglbackingstore.h
    LIBRARIES
        Qt::OpenGL
)
