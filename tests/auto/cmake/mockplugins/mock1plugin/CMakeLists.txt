# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QMock1Plugin
    NO_UNITY_BUILD
    CLASS_NAME QMock1Plugin
    PLUGIN_TYPE mockplugin
    SOURCES
        qmock1plugin.cpp qmock1plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)
