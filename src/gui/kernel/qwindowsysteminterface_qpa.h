/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QWINDOWSYSTEMINTERFACE_H
#define QWINDOWSYSTEMINTERFACE_H

#include <QtCore/QTime>
#include <QtGui/qwindowdefs.h>
#include <QtCore/QEvent>
#include <QtGui/QWindow>
#include <QtCore/QWeakPointer>
#include <QtCore/QMutex>
#include <QtGui/QTouchEvent>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_GUI_EXPORT QWindowSystemInterface
{
public:
    static void handleMouseEvent(QWindow *w, const QPoint & local, const QPoint & global, Qt::MouseButtons b);
    static void handleMouseEvent(QWindow *w, ulong timestamp, const QPoint & local, const QPoint & global, Qt::MouseButtons b);

    static void handleKeyEvent(QWindow *w, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1);
    static void handleKeyEvent(QWindow *w, ulong timestamp, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1);

    static void handleExtendedKeyEvent(QWindow *w, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                       quint32 nativeScanCode, quint32 nativeVirtualKey,
                                       quint32 nativeModifiers,
                                       const QString& text = QString(), bool autorep = false,
                                       ushort count = 1);
    static void handleExtendedKeyEvent(QWindow *w, ulong timestamp, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                       quint32 nativeScanCode, quint32 nativeVirtualKey,
                                       quint32 nativeModifiers,
                                       const QString& text = QString(), bool autorep = false,
                                       ushort count = 1);

    static void handleWheelEvent(QWindow *w, const QPoint & local, const QPoint & global, int d, Qt::Orientation o);
    static void handleWheelEvent(QWindow *w, ulong timestamp, const QPoint & local, const QPoint & global, int d, Qt::Orientation o);

    struct TouchPoint {
        int id;                 // for application use
        bool isPrimary;         // for application use
        QPointF normalPosition; // touch device coordinates, (0 to 1, 0 to 1)
        QRectF area;            // the touched area, centered at position in screen coordinates
        qreal pressure;         // 0 to 1
        Qt::TouchPointState state; //Qt::TouchPoint{Pressed|Moved|Stationary|Released}
    };

    static void handleTouchEvent(QWindow *w, QEvent::Type type, QTouchEvent::DeviceType devType, const QList<struct TouchPoint> &points);
    static void handleTouchEvent(QWindow *w, ulong timestamp, QEvent::Type type, QTouchEvent::DeviceType devType, const QList<struct TouchPoint> &points);

    static void handleGeometryChange(QWindow *w, const QRect &newRect);
    static void handleCloseEvent(QWindow *w);
    static void handleEnterEvent(QWindow *w);
    static void handleLeaveEvent(QWindow *w);
    static void handleWindowActivated(QWindow *w);

    static void handleMapEvent(QWindow *w);
    static void handleUnmapEvent(QWindow *w);

    static void handleExposeEvent(QWindow *w, const QRegion &region);

    // Changes to the screen
    static void handleScreenGeometryChange(int screenIndex);
    static void handleScreenAvailableGeometryChange(int screenIndex);
    static void handleScreenCountChange(int count);
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // QWINDOWSYSTEMINTERFACE_H
