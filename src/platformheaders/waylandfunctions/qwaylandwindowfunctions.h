/****************************************************************************
**
** Copyright (C) 2016 LG Electronics Ltd, author: mikko.levonmaa@lge.com
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

#ifndef QWAYLANDWINDOWFUNCTIONS_H
#define QWAYLANDWINDOWFUNCTIONS_H

#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

class QWindow;

class QWaylandWindowFunctions {
public:

    typedef void (*SetWindowSync)(QWindow *window);
    typedef void (*SetWindowDeSync)(QWindow *window);
    typedef bool (*IsWindowSync)(QWindow *window);
    static const QByteArray setSyncIdentifier() { return QByteArrayLiteral("WaylandSubSurfaceSetSync"); }
    static const QByteArray setDeSyncIdentifier() { return QByteArrayLiteral("WaylandSubSurfaceSetDeSync"); }
    static const QByteArray isSyncIdentifier() { return QByteArrayLiteral("WaylandSubSurfaceIsSync"); }

    static void setSync(QWindow *window)
    {
        static SetWindowSync func = reinterpret_cast<SetWindowSync>(QGuiApplication::platformFunction(setSyncIdentifier()));
        Q_ASSERT(func);
        func(window);
    }

    static void setDeSync(QWindow *window)
    {
        static SetWindowDeSync func = reinterpret_cast<SetWindowDeSync>(QGuiApplication::platformFunction(setDeSyncIdentifier()));
        Q_ASSERT(func);
        func(window);
    }

    static bool isSync(QWindow *window)
    {
        static IsWindowSync func = reinterpret_cast<IsWindowSync>(QGuiApplication::platformFunction(isSyncIdentifier()));
        Q_ASSERT(func);
        return func(window);
    }

};

QT_END_NAMESPACE

#endif // QWAYLANDWINDOWFUNCTIONS_H

