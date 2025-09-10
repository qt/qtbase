// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
