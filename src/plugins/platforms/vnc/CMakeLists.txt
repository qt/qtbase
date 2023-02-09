# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QVncIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QVncIntegrationPlugin
    OUTPUT_NAME qvnc
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES vnc
    SOURCES
        main.cpp
        qvnc.cpp qvnc_p.h
        qvncclient.cpp qvncclient.h
        qvncintegration.cpp qvncintegration.h
        qvncscreen.cpp qvncscreen.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::FbSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
)

## Scopes:
#####################################################################

qt_internal_extend_target(QVncIntegrationPlugin CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)
