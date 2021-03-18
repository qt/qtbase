/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiaaccessibility.h"
#include "qwinrtuiamainprovider.h"

#include <QtGui/QAccessible>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qt_windows.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

QWinRTUiaAccessibility::QWinRTUiaAccessibility()
{
}

QWinRTUiaAccessibility::~QWinRTUiaAccessibility()
{
}

// Handles UI Automation window messages.
void QWinRTUiaAccessibility::activate()
{
    // Start handling accessibility internally
    QGuiApplicationPrivate::platformIntegration()->accessibility()->setActive(true);
}

// Handles accessibility update notifications.
void QWinRTUiaAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!event)
        return;

    QAccessibleInterface *accessible = event->accessibleInterface();
    if (!isActive() || !accessible || !accessible->isValid())
        return;

    switch (event->type()) {
    case QAccessible::Focus:
        QWinRTUiaMainProvider::notifyFocusChange(event);
        break;
    case QAccessible::ObjectCreated:
    case QAccessible::ObjectDestroyed:
    case QAccessible::ObjectShow:
    case QAccessible::ObjectHide:
    case QAccessible::ObjectReorder:
        QWinRTUiaMainProvider::notifyVisibilityChange(event);
        break;
    case QAccessible::StateChanged:
        QWinRTUiaMainProvider::notifyStateChange(static_cast<QAccessibleStateChangeEvent *>(event));
        break;
    case QAccessible::ValueChanged:
        QWinRTUiaMainProvider::notifyValueChange(static_cast<QAccessibleValueChangeEvent *>(event));
        break;
    case QAccessible::TextAttributeChanged:
    case QAccessible::TextColumnChanged:
    case QAccessible::TextInserted:
    case QAccessible::TextRemoved:
    case QAccessible::TextUpdated:
    case QAccessible::TextSelectionChanged:
    case QAccessible::TextCaretMoved:
        QWinRTUiaMainProvider::notifyTextChange(event);
        break;
    default:
        break;
    }
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
