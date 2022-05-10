// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiagriditemprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaGridItemProvider::QWindowsUiaGridItemProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaGridItemProvider::~QWindowsUiaGridItemProvider()
{
}

// Returns the row index of the item.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridItemProvider::get_Row(int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface();
    if (!tableCellInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = tableCellInterface->rowIndex();
    return S_OK;
}

// Returns the column index of the item.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridItemProvider::get_Column(int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface();
    if (!tableCellInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = tableCellInterface->columnIndex();
    return S_OK;
}

// Returns the number of rows occupied by the item.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridItemProvider::get_RowSpan(int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface();
    if (!tableCellInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = tableCellInterface->rowExtent();
    return S_OK;
}

// Returns the number of columns occupied by the item.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridItemProvider::get_ColumnSpan(int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface();
    if (!tableCellInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = tableCellInterface->columnExtent();
    return S_OK;
}

// Returns the provider for the containing table/tree.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridItemProvider::get_ContainingGrid(IRawElementProviderSimple **pRetVal)
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

    if (QAccessibleInterface *table = tableCellInterface->table()) {
        *pRetVal = QWindowsUiaMainProvider::providerForAccessible(table);
    }
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
