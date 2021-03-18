/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <initguid.h>

#include "qwindowsuiawrapper_p.h"
#include <QtCore/private/qsystemlibrary_p.h>

QT_BEGIN_NAMESPACE

// private constructor
QWindowsUiaWrapper::QWindowsUiaWrapper()
{
    QSystemLibrary uiaLib(QStringLiteral("UIAutomationCore"));
    if (uiaLib.load()) {
        m_pUiaReturnRawElementProvider = reinterpret_cast<PtrUiaReturnRawElementProvider>(uiaLib.resolve("UiaReturnRawElementProvider"));
        m_pUiaHostProviderFromHwnd = reinterpret_cast<PtrUiaHostProviderFromHwnd>(uiaLib.resolve("UiaHostProviderFromHwnd"));
        m_pUiaRaiseAutomationPropertyChangedEvent = reinterpret_cast<PtrUiaRaiseAutomationPropertyChangedEvent>(uiaLib.resolve("UiaRaiseAutomationPropertyChangedEvent"));
        m_pUiaRaiseAutomationEvent = reinterpret_cast<PtrUiaRaiseAutomationEvent>(uiaLib.resolve("UiaRaiseAutomationEvent"));
        m_pUiaRaiseNotificationEvent = reinterpret_cast<PtrUiaRaiseNotificationEvent>(uiaLib.resolve("UiaRaiseNotificationEvent"));
        m_pUiaClientsAreListening = reinterpret_cast<PtrUiaClientsAreListening>(uiaLib.resolve("UiaClientsAreListening"));
    }
}

QWindowsUiaWrapper::~QWindowsUiaWrapper()
{
}

// shared instance
QWindowsUiaWrapper *QWindowsUiaWrapper::instance()
{
    static QWindowsUiaWrapper wrapper;
    return &wrapper;
}

// True if most symbols resolved (UiaRaiseNotificationEvent is optional).
BOOL QWindowsUiaWrapper::ready()
{
    return m_pUiaReturnRawElementProvider
        && m_pUiaHostProviderFromHwnd
        && m_pUiaRaiseAutomationPropertyChangedEvent
        && m_pUiaRaiseAutomationEvent
        && m_pUiaClientsAreListening;
}

BOOL QWindowsUiaWrapper::clientsAreListening()
{
    if (!m_pUiaClientsAreListening)
        return FALSE;
    return m_pUiaClientsAreListening();
}

LRESULT QWindowsUiaWrapper::returnRawElementProvider(HWND hwnd, WPARAM wParam, LPARAM lParam, IRawElementProviderSimple *el)
{
    if (!m_pUiaReturnRawElementProvider)
        return static_cast<LRESULT>(NULL);
    return m_pUiaReturnRawElementProvider(hwnd, wParam, lParam, el);
}

HRESULT QWindowsUiaWrapper::hostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple **ppProvider)
{
    if (!m_pUiaHostProviderFromHwnd)
        return UIA_E_NOTSUPPORTED;
    return m_pUiaHostProviderFromHwnd(hwnd, ppProvider);
}

HRESULT QWindowsUiaWrapper::raiseAutomationPropertyChangedEvent(IRawElementProviderSimple *pProvider, PROPERTYID id, VARIANT oldValue, VARIANT newValue)
{
    if (!m_pUiaRaiseAutomationPropertyChangedEvent)
        return UIA_E_NOTSUPPORTED;
    return m_pUiaRaiseAutomationPropertyChangedEvent(pProvider, id, oldValue, newValue);
}

HRESULT QWindowsUiaWrapper::raiseAutomationEvent(IRawElementProviderSimple *pProvider, EVENTID id)
{
    if (!m_pUiaRaiseAutomationEvent)
        return UIA_E_NOTSUPPORTED;
    return m_pUiaRaiseAutomationEvent(pProvider, id);
}

HRESULT QWindowsUiaWrapper::raiseNotificationEvent(IRawElementProviderSimple *provider, NotificationKind notificationKind, NotificationProcessing notificationProcessing, BSTR displayString, BSTR activityId)
{
    if (!m_pUiaRaiseNotificationEvent)
        return UIA_E_NOTSUPPORTED;
    return m_pUiaRaiseNotificationEvent(provider, notificationKind, notificationProcessing, displayString, activityId);
}

QT_END_NAMESPACE

