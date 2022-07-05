# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QMock6Plugin
    CLASS_NAME QMock6Plugin
    PLUGIN_TYPE mockauxplugin
    SOURCES
        qmock6plugin.cpp qmock6plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins3
)
