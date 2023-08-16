// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAWRAPPER_H
#define QWINDOWSUIAWRAPPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

#include "uiatypes_p.h"
#include "uiaattributeids_p.h"
#include "uiacontroltypeids_p.h"
#include "uiaerrorids_p.h"
#include "uiaeventids_p.h"
#include "uiageneralids_p.h"
#include "uiapatternids_p.h"
#include "uiapropertyids_p.h"
#include "uiaserverinterfaces_p.h"
#include "uiaclientinterfaces_p.h"

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QWindowsUiaWrapper
{
    QWindowsUiaWrapper();
    virtual ~QWindowsUiaWrapper();
public:
    static QWindowsUiaWrapper *instance();
    BOOL ready();
    BOOL clientsAreListening();
    LRESULT returnRawElementProvider(HWND hwnd, WPARAM wParam, LPARAM lParam, IRawElementProviderSimple *el);
    HRESULT hostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple **ppProvider);
    HRESULT raiseAutomationPropertyChangedEvent(IRawElementProviderSimple *pProvider, PROPERTYID id, VARIANT oldValue, VARIANT newValue);
    HRESULT raiseAutomationEvent(IRawElementProviderSimple *pProvider, EVENTID id);

private:
    typedef LRESULT (WINAPI *PtrUiaReturnRawElementProvider)(HWND, WPARAM, LPARAM, IRawElementProviderSimple *);
    typedef HRESULT (WINAPI *PtrUiaHostProviderFromHwnd)(HWND, IRawElementProviderSimple **);
    typedef HRESULT (WINAPI *PtrUiaRaiseAutomationPropertyChangedEvent)(IRawElementProviderSimple *, PROPERTYID, VARIANT, VARIANT);
    typedef HRESULT (WINAPI *PtrUiaRaiseAutomationEvent)(IRawElementProviderSimple *, EVENTID);
    typedef BOOL (WINAPI *PtrUiaClientsAreListening)();
    PtrUiaReturnRawElementProvider             m_pUiaReturnRawElementProvider = nullptr;
    PtrUiaHostProviderFromHwnd                 m_pUiaHostProviderFromHwnd = nullptr;
    PtrUiaRaiseAutomationPropertyChangedEvent  m_pUiaRaiseAutomationPropertyChangedEvent = nullptr;
    PtrUiaRaiseAutomationEvent                 m_pUiaRaiseAutomationEvent = nullptr;
    PtrUiaClientsAreListening                  m_pUiaClientsAreListening = nullptr;
};

QT_END_NAMESPACE

#endif //QWINDOWSUIAWRAPPER_H

