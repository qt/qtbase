# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QMock4Plugin
    NO_UNITY_BUILD
    CLASS_NAME QMock4Plugin
    PLUGIN_TYPE mockplugin
    DEFAULT_IF FALSE
    SOURCES
        qmock4plugin.cpp qmock4plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)
