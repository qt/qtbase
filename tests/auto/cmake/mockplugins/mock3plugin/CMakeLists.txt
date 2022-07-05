# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QMock3Plugin
    CLASS_NAME QMock3Plugin
    PLUGIN_TYPE mockplugin
    SOURCES
        qmock3plugin.cpp qmock3plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)
