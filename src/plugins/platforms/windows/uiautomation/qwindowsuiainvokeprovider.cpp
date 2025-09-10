// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiainvokeprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaInvokeProvider::QWindowsUiaInvokeProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaInvokeProvider::~QWindowsUiaInvokeProvider()
{
}

HRESULT STDMETHODCALLTYPE QWindowsUiaInvokeProvider::Invoke()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleActionInterface *actionInterface = accessible->actionInterface();
    if (!actionInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    actionInterface->doAction(QAccessibleActionInterface::pressAction());
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
