# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QMock5Plugin
    NO_UNITY_BUILD
    CLASS_NAME QMock5Plugin
    PLUGIN_TYPE mockplugin
    DEFAULT_IF FALSE
    SOURCES
        qmock5plugin.cpp qmock5plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins3
)
