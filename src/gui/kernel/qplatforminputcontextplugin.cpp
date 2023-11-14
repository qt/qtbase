// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
