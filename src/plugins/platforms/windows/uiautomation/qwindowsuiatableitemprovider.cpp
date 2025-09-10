// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiatableitemprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaTableItemProvider::QWindowsUiaTableItemProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaTableItemProvider::~QWindowsUiaTableItemProvider()
{
}

// Returns the providers for the row headers associated with the item.
HRESULT STDMETHODCALLTYPE QWindowsUiaTableItemProvider::GetRowHeaderItems(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface();
    if (!tableCellInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    const auto headers = tableCellInterface->rowHeaderCells();

    if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, headers.size()))) {
        for (LONG i = 0; i < headers.size(); ++i) {
            if (ComPtr<IRawElementProviderSimple> provider =
                        QWindowsUiaMainProvider::providerForAccessible(headers.at(i))) {
                SafeArrayPutElement(*pRetVal, &i, provider.Get());
            }
        }
    }
    return S_OK;
}

// Returns the providers for the column headers associated with the item.
HRESULT STDMETHODCALLTYPE QWindowsUiaTableItemProvider::GetColumnHeaderItems(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface();
    if (!tableCellInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    const auto headers = tableCellInterface->columnHeaderCells();

    if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, headers.size()))) {
        for (LONG i = 0; i < headers.size(); ++i) {
            if (ComPtr<IRawElementProviderSimple> provider =
                        QWindowsUiaMainProvider::providerForAccessible(headers.at(i))) {
                SafeArrayPutElement(*pRetVal, &i, provider.Get());
            }
        }
    }
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
