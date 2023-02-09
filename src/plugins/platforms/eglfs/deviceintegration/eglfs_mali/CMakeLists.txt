# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QEglFSMaliIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSMaliIntegrationPlugin
    OUTPUT_NAME qeglfs-mali-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfsmaliintegration.cpp qeglfsmaliintegration.h
        qeglfsmalimain.cpp
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
)
