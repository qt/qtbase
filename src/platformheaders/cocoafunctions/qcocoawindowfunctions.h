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

#ifndef QCOCOAWINDOWFUNCTIONS_H
#define QCOCOAWINDOWFUNCTIONS_H

#include <QtPlatformHeaders/QPlatformHeaderHelper>

QT_BEGIN_NAMESPACE

class QWindow;

class QCocoaWindowFunctions {
public:
    typedef QPoint (*BottomLeftClippedByNSWindowOffset)(QWindow *window);
    static const QByteArray bottomLeftClippedByNSWindowOffsetIdentifier() { return QByteArrayLiteral("CocoaBottomLeftClippedByNSWindowOffset"); }

    static QPoint bottomLeftClippedByNSWindowOffset(QWindow *window)
    {
        return QPlatformHeaderHelper::callPlatformFunction<QPoint, BottomLeftClippedByNSWindowOffset>(bottomLeftClippedByNSWindowOffsetIdentifier(),window);
    }
};


QT_END_NAMESPACE

#endif // QCOCOAWINDOWFUNCTIONS_H
