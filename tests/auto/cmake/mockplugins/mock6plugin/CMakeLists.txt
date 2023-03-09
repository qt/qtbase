# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QMock6Plugin
    NO_UNITY_BUILD
    CLASS_NAME QMock6Plugin
    PLUGIN_TYPE mockauxplugin
    SOURCES
        qmock6plugin.cpp qmock6plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins3
)
