# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QOhosNetworkInformationPlugin
    OUTPUT_NAME qohosnetworkinformation
    CLASS_NAME QOhosNetworkInformationBackendFactory
    PLUGIN_TYPE networkinformation
    DEFAULT_IF OHOS
    EXCEPTIONS
    SOURCES
        qohosnetworkinformationbackend.cpp
        qohosnetconnection.cpp qohosnetconnection_p.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../platforms/ohos
    LIBRARIES
        Qt::NetworkPrivate
        Qt::CorePrivate
        NodeAddonApi::NodeAddonApi
        ace_napi.z
    DEFINES
        "QT_ENABLE_STRICT_MODE_UP_TO=QT_VERSION_CHECK(6,11,0)"
        QT_NO_CAST_FROM_ASCII
        NAPI_CPP_EXCEPTIONS
)
