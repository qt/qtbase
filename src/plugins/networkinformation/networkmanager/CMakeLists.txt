# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QNetworkManagerNetworkInformationPlugin
    OUTPUT_NAME qnetworkmanager
    CLASS_NAME QNetworkManagerNetworkInformationBackendFactory
    PLUGIN_TYPE networkinformation
    DEFAULT_IF LINUX
    SOURCES
        qnetworkmanagernetworkinformationbackend.cpp
        qnetworkmanagerservice.h
        qnetworkmanagerservice.cpp
    LIBRARIES
        Qt::DBus
        Qt::NetworkPrivate
)
