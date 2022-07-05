# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from eglfs_viv_wl.pro.

#####################################################################
## QEglFSVivWaylandIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSVivWaylandIntegrationPlugin
    OUTPUT_NAME qeglfs-viv-wl-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfsvivwlintegration.cpp qeglfsvivwlintegration.h
        qeglfsvivwlmain.cpp
    DEFINES
        EGL_API_FB=1
        LINUX=1
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Wayland::Server
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:eglfs_viv_wl.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_viv_wl.json"
