// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatforminputcontextplugin_p.h"

QT_BEGIN_NAMESPACE

QPlatformInputContextPlugin::QPlatformInputContextPlugin(QObject *parent)
    : QObject(parent)
{
}

QPlatformInputContextPlugin::~QPlatformInputContextPlugin()
{
}

QT_END_NAMESPACE

#include "moc_qplatforminputcontextplugin_p.cpp"
