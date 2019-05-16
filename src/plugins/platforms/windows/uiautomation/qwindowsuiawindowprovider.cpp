/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qwindowsuiawindowprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtGui/private/qwindow_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaWindowProvider::QWindowsUiaWindowProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaWindowProvider::~QWindowsUiaWindowProvider()
{
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::SetVisualState(WindowVisualState state) {
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible || !accessible->window())
        return UIA_E_ELEMENTNOTAVAILABLE;
    auto window = accessible->window();
    switch (state) {
    case WindowVisualState_Normal:
        window->showNormal();
        break;
    case WindowVisualState_Maximized:
        window->showMaximized();
        break;
    case WindowVisualState_Minimized:
        window->showMinimized();
        break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::Close() {
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible || !accessible->window())
        return UIA_E_ELEMENTNOTAVAILABLE;
    accessible->window()->close();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::WaitForInputIdle(int milliseconds, __RPC__out BOOL *pRetVal) {
    Q_UNUSED(milliseconds);
    Q_UNUSED(pRetVal);
    return UIA_E_NOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::get_CanMaximize(__RPC__out BOOL *pRetVal) {
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible || !accessible->window())
        return UIA_E_ELEMENTNOTAVAILABLE;

    auto window = accessible->window();
    auto flags = window->flags();

    *pRetVal = (!(flags & Qt::MSWindowsFixedSizeDialogHint)
                && (flags & Qt::WindowMaximizeButtonHint)
                && ((flags & Qt::CustomizeWindowHint)
                    || window->maximumSize() == QSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX)));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::get_CanMinimize(__RPC__out BOOL *pRetVal) {
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible || !accessible->window())
        return UIA_E_ELEMENTNOTAVAILABLE;
    *pRetVal = accessible->window()->flags() & Qt::WindowMinimizeButtonHint;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::get_IsModal(__RPC__out BOOL *pRetVal) {
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible || !accessible->window())
        return UIA_E_ELEMENTNOTAVAILABLE;
    *pRetVal = accessible->window()->isModal();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::get_WindowVisualState(__RPC__out enum WindowVisualState *pRetVal) {
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible || !accessible->window())
        return UIA_E_ELEMENTNOTAVAILABLE;
    auto visibility = accessible->window()->visibility();
    switch (visibility) {
    case QWindow::FullScreen:
    case QWindow::Maximized:
        *pRetVal = WindowVisualState_Maximized;
        break;
    case QWindow::Minimized:
        *pRetVal = WindowVisualState_Minimized;
        break;
    default:
        *pRetVal = WindowVisualState_Normal;
        break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::get_WindowInteractionState(__RPC__out enum WindowInteractionState *pRetVal) {
    Q_UNUSED(pRetVal);
    return UIA_E_NOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaWindowProvider::get_IsTopmost(__RPC__out BOOL *pRetVal) {
    Q_UNUSED(pRetVal);
    return UIA_E_NOTSUPPORTED;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
