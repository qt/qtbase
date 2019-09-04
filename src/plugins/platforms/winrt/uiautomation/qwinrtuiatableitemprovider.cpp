/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiatableitemprovider.h"
#include "qwinrtuiamainprovider.h"
#include "qwinrtuiametadatacache.h"
#include "qwinrtuiautils.h"

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

using namespace QWinRTUiAutomation;
using namespace ABI::Windows::UI::Xaml::Automation;
using namespace ABI::Windows::UI::Xaml::Automation::Provider;

QWinRTUiaTableItemProvider::QWinRTUiaTableItemProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaTableItemProvider::~QWinRTUiaTableItemProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

// Returns the providers for the column headers associated with the item.
HRESULT STDMETHODCALLTYPE QWinRTUiaTableItemProvider::GetColumnHeaderItems(UINT32 *returnValueSize, IIRawElementProviderSimple ***returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValueSize || !returnValue)
        return E_INVALIDARG;
    *returnValueSize = 0;
    *returnValue = nullptr;

    auto accid = id();
    auto elementIds = std::make_shared<QVarLengthArray<QAccessible::Id>>();

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, elementIds]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface()) {
                QList<QAccessibleInterface *> headers = tableCellInterface->columnHeaderCells();
                for (auto header : qAsConst(headers)) {
                    QAccessible::Id headerId = idForAccessible(header);
                    QWinRTUiaMetadataCache::instance()->load(headerId);
                    elementIds->append(headerId);
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    return QWinRTUiaMainProvider::rawProviderArrayForAccessibleIdList(*elementIds, returnValueSize, returnValue);
}

// Returns the providers for the row headers associated with the item.
HRESULT STDMETHODCALLTYPE QWinRTUiaTableItemProvider::GetRowHeaderItems(UINT32 *returnValueSize, IIRawElementProviderSimple ***returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValueSize || !returnValue)
        return E_INVALIDARG;
    *returnValueSize = 0;
    *returnValue = nullptr;

    auto accid = id();
    auto elementIds = std::make_shared<QVarLengthArray<QAccessible::Id>>();

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, elementIds]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface()) {
                QList<QAccessibleInterface *> headers = tableCellInterface->rowHeaderCells();
                for (auto header : qAsConst(headers)) {
                    QAccessible::Id headerId = idForAccessible(header);
                    QWinRTUiaMetadataCache::instance()->load(headerId);
                    elementIds->append(headerId);
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    return QWinRTUiaMainProvider::rawProviderArrayForAccessibleIdList(*elementIds, returnValueSize, returnValue);
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
