/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY

#include "qwindowsuiagridprovider.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QDebug>
#include <QtCore/QString>

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

#endif // QT_NO_ACCESSIBILITY
