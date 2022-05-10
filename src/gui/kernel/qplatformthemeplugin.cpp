// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformthemeplugin.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformThemePlugin
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformThemePlugin class provides an abstraction for theme plugins.
 */
QPlatformThemePlugin::QPlatformThemePlugin(QObject *parent)
    : QObject(parent)
{
}

QPlatformThemePlugin::~QPlatformThemePlugin()
{
}

QT_END_NAMESPACE

#include "moc_qplatformthemeplugin.cpp"
