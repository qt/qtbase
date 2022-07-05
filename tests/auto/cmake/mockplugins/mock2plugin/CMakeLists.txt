# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QMock2Plugin
    CLASS_NAME QMock2Plugin
    PLUGIN_TYPE mockplugin
    SOURCES
        qmock2plugin.cpp qmock2plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)
