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
        QtAndroidAccessibility::notifyLocationChange();
    } else if (event->type() == QAccessible::ObjectHide) {
        QtAndroidAccessibility::notifyObjectHide(event->uniqueId());
    } else if (event->type() == QAccessible::Focus) {
        QtAndroidAccessibility::notifyObjectFocus(event->uniqueId());
    }
}

QT_END_NAMESPACE
