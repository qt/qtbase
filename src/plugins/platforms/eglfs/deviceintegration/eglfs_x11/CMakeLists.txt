# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QEglFSX11IntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSX11IntegrationPlugin
    OUTPUT_NAME qeglfs-x11-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfsx11integration.cpp qeglfsx11integration.h
        qeglfsx11main.cpp
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
        X11::X11
        X11::XCB
        XCB::XCB
    NO_UNITY_BUILD # X11 define clashes
)
