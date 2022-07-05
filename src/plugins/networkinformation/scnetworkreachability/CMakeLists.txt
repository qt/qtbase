# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QSCNetworkReachabilityNetworkInformationPlugin
    OUTPUT_NAME qscnetworkreachability
    CLASS_NAME QSCNetworkReachabilityNetworkInformationBackendFactory
    PLUGIN_TYPE networkinformation
    DEFAULT_IF APPLE
    SOURCES
        qscnetworkreachabilitynetworkinformationbackend.mm
    LIBRARIES
        Qt::NetworkPrivate
        ${FWSystemConfiguration}
)
