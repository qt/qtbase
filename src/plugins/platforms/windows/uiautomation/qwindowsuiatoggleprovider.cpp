// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiatoggleprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaToggleProvider::QWindowsUiaToggleProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaToggleProvider::~QWindowsUiaToggleProvider()
{
}

// toggles the state by invoking the toggle action
HRESULT STDMETHODCALLTYPE QWindowsUiaToggleProvider::Toggle()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    actionInterface->doAction(QAccessibleActionInterface::toggleAction());
    return S_OK;
}

// Gets the current toggle state.
HRESULT STDMETHODCALLTYPE QWindowsUiaToggleProvider::get_ToggleState(ToggleState *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = ToggleState_Off;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (accessible->state().checkStateMixed)
        *pRetVal = ToggleState_Indeterminate;
    else if (accessible->state().checked)
        *pRetVal = ToggleState_On;
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
