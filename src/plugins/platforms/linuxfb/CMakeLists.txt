# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QLinuxFbIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QLinuxFbIntegrationPlugin
    OUTPUT_NAME qlinuxfb
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES linuxfb
    SOURCES
        main.cpp
        qlinuxfbintegration.cpp qlinuxfbintegration.h
        qlinuxfbscreen.cpp qlinuxfbscreen.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::FbSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(QLinuxFbIntegrationPlugin CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)

qt_internal_extend_target(QLinuxFbIntegrationPlugin CONDITION TARGET Qt::KmsSupportPrivate
    SOURCES
        qlinuxfbdrmscreen.cpp qlinuxfbdrmscreen.h
    LIBRARIES
        Qt::KmsSupportPrivate
)
