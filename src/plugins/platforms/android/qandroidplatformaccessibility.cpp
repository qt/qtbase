// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qandroidplatformaccessibility.h"
#include "androidjniaccessibility.h"

QT_BEGIN_NAMESPACE
QAndroidPlatformAccessibility::QAndroidPlatformAccessibility()
{
    QtAndroidAccessibility::initialize();
}

QAndroidPlatformAccessibility::~QAndroidPlatformAccessibility()
{}

void QAndroidPlatformAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!isActive() || event == nullptr || !event->accessibleInterface())
        return;

    // We do not need implementation of all events, as current statues are polled
    // by QtAccessibilityDelegate.java on every accessibility interaction.
    // Currently we only send notification about the element's position change,
    // so that the element can be moved on the screen if it's focused.

    if (event->type() == QAccessible::LocationChanged) {
        QtAndroidAccessibility::notifyLocationChange(event->uniqueId());
    } else if (event->type() == QAccessible::ObjectHide) {
        QtAndroidAccessibility::notifyObjectHide(event->uniqueId());
    } else if (event->type() == QAccessible::Focus) {
        QtAndroidAccessibility::notifyObjectFocus(event->uniqueId());
    } else if (event->type() == QAccessible::ValueChanged) {
        QtAndroidAccessibility::notifyValueChanged(event->uniqueId());
    } else if (event->type() == QAccessible::ScrollingEnd) {
        QtAndroidAccessibility::notifyScrolledEvent(event->uniqueId());
    }
}

void QAndroidPlatformAccessibility::setRootObject(QObject *obj)
{
    QPlatformAccessibility::setRootObject(obj);
    QtAndroidAccessibility::createAccessibilityContextObject(obj);
}

QT_END_NAMESPACE
