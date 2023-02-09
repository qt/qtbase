# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QEglFSOpenWFDIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSOpenWFDIntegrationPlugin
    OUTPUT_NAME qeglfs-openwfd-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfsopenwfdintegration.cpp qeglfsopenwfdintegration.h
        qeglfsopenwfdmain.cpp
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
)
