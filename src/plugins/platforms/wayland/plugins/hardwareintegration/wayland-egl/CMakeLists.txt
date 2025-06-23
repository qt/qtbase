# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from wayland-egl.pro.

#####################################################################
## QWaylandEglClientBufferPlugin Plugin:
#####################################################################
qt_find_package(EGL MODULE)

qt_internal_add_plugin(QWaylandEglClientBufferPlugin
    OUTPUT_NAME qt-plugin-wayland-egl
    PLUGIN_TYPE wayland-graphics-integration-client
    SOURCES
        main.cpp
        qwaylandeglclientbufferintegration.cpp qwaylandeglclientbufferintegration_p.h
        qwaylandeglinclude_p.h
        qwaylandeglwindow.cpp qwaylandeglwindow_p.h
        qwaylandglcontext.cpp qwaylandglcontext_p.h
    LIBRARIES
        ${CMAKE_DL_LIBS}
        EGL::EGL
        Qt::Core
        Qt::Gui
        Qt::OpenGLPrivate
        Qt::WaylandClientPrivate
        Wayland::Client
        Wayland::Egl
    QT_LICENSE_ID QT_COMMERCIAL_OR_LGPL3
)

qt_internal_extend_target(QWaylandEglClientBufferPlugin CONDITION QT_FEATURE_egl AND NOT QT_FEATURE_egl_x11
    DEFINES
        QT_EGL_NO_X11
)


#### Keys ignored in scope 1:.:.:wayland-egl.pro:<TRUE>:
# OTHER_FILES = "wayland-egl.json"
