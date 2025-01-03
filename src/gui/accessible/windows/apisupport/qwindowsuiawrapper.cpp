// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

// True if all symbols resolved.
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

QT_END_NAMESPACE

