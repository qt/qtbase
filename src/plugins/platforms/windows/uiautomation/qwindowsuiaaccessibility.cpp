/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY

#include "qwindowsuiaaccessibility.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"

#include <QtGui/QAccessible>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qt_windows.h>
#include <qpa/qplatformintegration.h>
#include <QtWindowsUIAutomationSupport/private/qwindowsuiawrapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaAccessibility::QWindowsUiaAccessibility()
{
}

QWindowsUiaAccessibility::~QWindowsUiaAccessibility()
{
}

// Handles UI Automation window messages.
bool QWindowsUiaAccessibility::handleWmGetObject(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    // Start handling accessibility internally
    QGuiApplicationPrivate::platformIntegration()->accessibility()->setActive(true);

    // Ignoring all requests while starting up / shutting down
    if (QCoreApplication::startingUp() || QCoreApplication::closingDown())
        return false;

    if (QWindow *window = QWindowsContext::instance()->findWindow(hwnd)) {
        if (QAccessibleInterface *accessible = window->accessibleRoot()) {
            QWindowsUiaMainProvider *provider = QWindowsUiaMainProvider::providerForAccessible(accessible);
            *lResult = QWindowsUiaWrapper::instance()->returnRawElementProvider(hwnd, wParam, lParam, provider);
            return true;
        }
    }
    return false;
}

// Handles accessibility update notifications.
void QWindowsUiaAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!event)
        return;

    QAccessibleInterface *accessible = event->accessibleInterface();
    if (!isActive() || !accessible || !accessible->isValid())
        return;

    // Ensures QWindowsUiaWrapper is properly initialized.
    if (!QWindowsUiaWrapper::instance()->ready())
        return;

    // No need to do anything when nobody is listening.
    if (!QWindowsUiaWrapper::instance()->clientsAreListening())
        return;

    switch (event->type()) {

    case QAccessible::Focus:
        QWindowsUiaMainProvider::notifyFocusChange(event);
        break;

    case QAccessible::StateChanged:
        QWindowsUiaMainProvider::notifyStateChange(static_cast<QAccessibleStateChangeEvent *>(event));
        break;

    case QAccessible::ValueChanged:
        QWindowsUiaMainProvider::notifyValueChange(static_cast<QAccessibleValueChangeEvent *>(event));
        break;

    case QAccessible::TextAttributeChanged:
    case QAccessible::TextColumnChanged:
    case QAccessible::TextInserted:
    case QAccessible::TextRemoved:
    case QAccessible::TextUpdated:
    case QAccessible::TextSelectionChanged:
    case QAccessible::TextCaretMoved:
        QWindowsUiaMainProvider::notifyTextChange(event);
        break;

    default:
        break;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
