// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "qiosplatformaccessibility.h"

#if QT_CONFIG(accessibility)

#include <QtGui/QtGui>
#include "qioswindow.h"
#include "quiaccessibilityelement.h"

QIOSPlatformAccessibility::QIOSPlatformAccessibility()
{
    m_focusObserver = QMacNotificationObserver(
            nil, UIAccessibilityElementFocusedNotification, [&](NSNotification *notification) {
                id element = notification.userInfo[UIAccessibilityFocusedElementKey];
                m_focusElement = static_cast<QMacAccessibilityElement *>(element);
            });
}

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
    foreach (QWindow *win, QGuiApplication::topLevelWindows()) {
        if (win && win->handle()) {
            QT_PREPEND_NAMESPACE(QIOSWindow) *window = static_cast<QT_PREPEND_NAMESPACE(QIOSWindow) *>(win->handle());
            window->clearAccessibleCache();
        }
    }
}


void QIOSPlatformAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    auto *accessibleInterface = event->accessibleInterface();
    if (!isActive() || !accessibleInterface)
        return;
    switch (event->type()) {
    case QAccessible::Announcement: {
        auto *announcementEvent = static_cast<QAccessibleAnnouncementEvent *>(event);
        UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification,
                                        announcementEvent->message().toNSString());
        break;
    }
    case QAccessible::Focus: {
        auto *element = [QMacAccessibilityElement elementWithId:event->uniqueId()];
        Q_ASSERT(element);
        // There's no NSAccessibilityFocusedUIElementChangedNotification, like we have on
        // macOS. Instead, the documentation for UIAccessibilityLayoutChangedNotification
        // specifies that the optional argument to UIAccessibilityPostNotification is the
        // accessibility element for VoiceOver to move to after processing the notification.
        UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, element);
        break;
    }
    case QAccessible::DescriptionChanged:
    case QAccessible::NameChanged: {
        auto *element = [QMacAccessibilityElement elementWithId:event->uniqueId()];
        if (element == m_focusElement)
            UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, element);
        break;
    }
    case QAccessible::ObjectCreated:
    case QAccessible::ObjectShow:
    case QAccessible::ObjectHide:
    case QAccessible::ObjectDestroyed:
        invalidateCache(accessibleInterface);
        switch (accessibleInterface->role()) {
        case QAccessible::Window:
        case QAccessible::Dialog:
            // Bigger changes to the UI require a full reset of VoiceOver
            UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification, nil);
            break;
        default:
            // While smaller changes can be handled by re-reading the layout
            UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, nil);
        }
        break;
    default:
        break;
    }
}

#endif
