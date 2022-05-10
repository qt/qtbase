// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiatableprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaTableProvider::QWindowsUiaTableProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaTableProvider::~QWindowsUiaTableProvider()
{
}

// Gets the providers for all the row headers in the table.
HRESULT STDMETHODCALLTYPE QWindowsUiaTableProvider::GetRowHeaders(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableInterface *tableInterface = accessible->tableInterface();
    if (!tableInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QList<QAccessibleInterface *> headers;

    for (int i = 0; i < tableInterface->rowCount(); ++i) {
        if (QAccessibleInterface *cell = tableInterface->cellAt(i, 0)) {
            if (QAccessibleTableCellInterface *tableCellInterface = cell->tableCellInterface()) {
                headers.append(tableCellInterface->rowHeaderCells());
            }
        }
    }
    if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, headers.size()))) {
        for (LONG i = 0; i < headers.size(); ++i) {
            if (QWindowsUiaMainProvider *headerProvider = QWindowsUiaMainProvider::providerForAccessible(headers.at(i))) {
                SafeArrayPutElement(*pRetVal, &i, static_cast<IRawElementProviderSimple *>(headerProvider));
                headerProvider->Release();
            }
        }
    }
    return S_OK;
}

// Gets the providers for all the column headers in the table.
HRESULT STDMETHODCALLTYPE QWindowsUiaTableProvider::GetColumnHeaders(SAFEARRAY **pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableInterface *tableInterface = accessible->tableInterface();
    if (!tableInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QList<QAccessibleInterface *> headers;

    for (int i = 0; i < tableInterface->columnCount(); ++i) {
        if (QAccessibleInterface *cell = tableInterface->cellAt(0, i)) {
            if (QAccessibleTableCellInterface *tableCellInterface = cell->tableCellInterface()) {
                headers.append(tableCellInterface->columnHeaderCells());
            }
        }
    }
    if ((*pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, headers.size()))) {
        for (LONG i = 0; i < headers.size(); ++i) {
            if (QWindowsUiaMainProvider *headerProvider = QWindowsUiaMainProvider::providerForAccessible(headers.at(i))) {
                SafeArrayPutElement(*pRetVal, &i, static_cast<IRawElementProviderSimple *>(headerProvider));
                headerProvider->Release();
            }
        }
    }
    return S_OK;
}

// Returns the primary direction of traversal for the table.
HRESULT STDMETHODCALLTYPE QWindowsUiaTableProvider::get_RowOrColumnMajor(enum RowOrColumnMajor *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = RowOrColumnMajor_Indeterminate;
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
