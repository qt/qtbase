/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QXCBSCREENFUNCTIONS_H
#define QXCBSCREENFUNCTIONS_H

#include <QtPlatformHeaders/QPlatformHeaderHelper>

QT_BEGIN_NAMESPACE

class QScreen;

class QXcbScreenFunctions
{
public:
    typedef bool (*VirtualDesktopNumber)(const QScreen *screen);
    static const QByteArray virtualDesktopNumberIdentifier() { return QByteArrayLiteral("XcbVirtualDesktopNumber"); }
    static int virtualDesktopNumber(const QScreen *screen)
    {
        return QPlatformHeaderHelper::callPlatformFunction<int, VirtualDesktopNumber, const QScreen *>(virtualDesktopNumberIdentifier(), screen);
    }
};

QT_END_NAMESPACE

#endif  /*QXCBSCREENFUNCTIONS_H*/
