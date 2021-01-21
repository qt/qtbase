/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
