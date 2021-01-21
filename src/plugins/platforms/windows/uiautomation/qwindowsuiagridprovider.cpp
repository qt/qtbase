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

#include "qwindowsuiagridprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaGridProvider::QWindowsUiaGridProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaGridProvider::~QWindowsUiaGridProvider()
{
}

// Returns the provider for an item within a table/tree.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridProvider::GetItem(int row, int column, IRawElementProviderSimple **pRetVal)
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

    if ((row >= 0) && (row < tableInterface->rowCount()) && (column >= 0) && (column < tableInterface->columnCount())) {
        if (QAccessibleInterface *cell = tableInterface->cellAt(row, column)) {
            *pRetVal = QWindowsUiaMainProvider::providerForAccessible(cell);
        }
    }
    return S_OK;
}

// Returns the number of rows.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridProvider::get_RowCount(int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableInterface *tableInterface = accessible->tableInterface();
    if (!tableInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = tableInterface->rowCount();
    return S_OK;
}

// Returns the number of columns.
HRESULT STDMETHODCALLTYPE QWindowsUiaGridProvider::get_ColumnCount(int *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = 0;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleTableInterface *tableInterface = accessible->tableInterface();
    if (!tableInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = tableInterface->columnCount();
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
