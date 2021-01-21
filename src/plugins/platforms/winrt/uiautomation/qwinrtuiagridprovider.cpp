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

#include "qwinrtuiagridprovider.h"
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
using namespace ABI::Windows::UI::Xaml::Automation::Peers;

QWinRTUiaGridProvider::QWinRTUiaGridProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaGridProvider::~QWinRTUiaGridProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

// Returns the number of columns.
HRESULT STDMETHODCALLTYPE QWinRTUiaGridProvider::get_ColumnCount(INT32 *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->columnCount();
    return S_OK;
}

// Returns the number of rows.
HRESULT STDMETHODCALLTYPE QWinRTUiaGridProvider::get_RowCount(INT32 *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->rowCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaGridProvider::GetItem(INT32 row, INT32 column, IIRawElementProviderSimple **returnValue)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!returnValue)
        return E_INVALIDARG;
    *returnValue = nullptr;

    auto accid = id();
    auto elementId = std::make_shared<QAccessible::Id>(0);

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, row, column, elementId]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleTableInterface *tableInterface = accessible->tableInterface()) {
                if ((row >= 0) && (row < tableInterface->rowCount()) && (column >= 0) && (column < tableInterface->columnCount())) {
                    if (QAccessibleInterface *cell = tableInterface->cellAt(row, column)) {
                        *elementId = idForAccessible(cell);
                        QWinRTUiaMetadataCache::instance()->load(*elementId);
                    }
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    if (!*elementId)
        return E_FAIL;

    return QWinRTUiaMainProvider::rawProviderForAccessibleId(*elementId, returnValue);
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
