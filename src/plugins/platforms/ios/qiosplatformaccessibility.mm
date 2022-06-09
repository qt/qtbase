// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosplatformaccessibility.h"

#if QT_CONFIG(accessibility)

#include <QtGui/QtGui>
#include "qioswindow.h"

QIOSPlatformAccessibility::QIOSPlatformAccessibility()
{}

QIOSPlatformAccessibility::~QIOSPlatformAccessibility()
{}


void invalidateCache(QAccessibleInterface *iface)
{
    if (!iface || !iface->isValid()) {
        qWarning() << "invalid accessible interface: " << iface;
        return;
    }

    // This will invalidate everything regardless of what window the
    // interface belonged to. We might want to revisit this strategy later.
    // (Therefore this function still takes the interface as argument)
    // It is also responsible for the bug that focus gets temporary lost
    // when items get added or removed from the screen
    foreach (QWindow *win, QGuiApplication::topLevelWindows()) {
        if (win && win->handle()) {
            QT_PREPEND_NAMESPACE(QIOSWindow) *window = static_cast<QT_PREPEND_NAMESPACE(QIOSWindow) *>(win->handle());
            window->clearAccessibleCache();
        }
    }
}


void QIOSPlatformAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!isActive() || !event->accessibleInterface())
        return;
    switch (event->type()) {
    case QAccessible::ObjectCreated:
    case QAccessible::ObjectShow:
    case QAccessible::ObjectHide:
    case QAccessible::ObjectDestroyed:
        invalidateCache(event->accessibleInterface());
        break;
    default:
        break;
    }
}

#endif
