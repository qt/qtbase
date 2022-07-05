# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QMock4Plugin
    CLASS_NAME QMock4Plugin
    PLUGIN_TYPE mockplugin
    DEFAULT_IF FALSE
    SOURCES
        qmock4plugin.cpp qmock4plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)
