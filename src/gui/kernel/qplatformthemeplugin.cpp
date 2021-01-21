/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
