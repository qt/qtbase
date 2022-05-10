/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
        QtAndroidAccessibility::notifyLocationChange(event->uniqueId());
    } else if (event->type() == QAccessible::ObjectHide) {
        QtAndroidAccessibility::notifyObjectHide(event->uniqueId());
    } else if (event->type() == QAccessible::Focus) {
        QtAndroidAccessibility::notifyObjectFocus(event->uniqueId());
    } else if (event->type() == QAccessible::ValueChanged) {
        QtAndroidAccessibility::notifyValueChanged(event->uniqueId());
    }
}

void QAndroidPlatformAccessibility::setRootObject(QObject *obj)
{
    QPlatformAccessibility::setRootObject(obj);
    QtAndroidAccessibility::createAccessibilityContextObject(obj);
}

QT_END_NAMESPACE
