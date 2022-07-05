# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QMock5Plugin
    CLASS_NAME QMock5Plugin
    PLUGIN_TYPE mockplugin
    DEFAULT_IF FALSE
    SOURCES
        qmock5plugin.cpp qmock5plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins3
)
