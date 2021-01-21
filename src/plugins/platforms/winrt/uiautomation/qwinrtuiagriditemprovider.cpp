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

#include "qwinrtuiagriditemprovider.h"
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

QWinRTUiaGridItemProvider::QWinRTUiaGridItemProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaGridItemProvider::~QWinRTUiaGridItemProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

// Returns the column index of the item.
HRESULT STDMETHODCALLTYPE QWinRTUiaGridItemProvider::get_Column(INT32 *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->columnIndex();
    return S_OK;
}

// Returns the number of columns occupied by the item.
HRESULT STDMETHODCALLTYPE QWinRTUiaGridItemProvider::get_ColumnSpan(INT32 *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->columnCount();
    return S_OK;
}

// Returns the provider for the containing table/tree.
HRESULT STDMETHODCALLTYPE QWinRTUiaGridItemProvider::get_ContainingGrid(IIRawElementProviderSimple **value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;
    *value = nullptr;

    auto accid = id();
    auto elementId = std::make_shared<QAccessible::Id>(0);

    if (!SUCCEEDED(QEventDispatcherWinRT::runOnMainThread([accid, elementId]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface()) {
                if (QAccessibleInterface *table = tableCellInterface->table()) {
                    *elementId = idForAccessible(table);
                    QWinRTUiaMetadataCache::instance()->load(*elementId);
                }
            }
        }
        return S_OK;
    }))) {
        return E_FAIL;
    }

    if (!*elementId)
        return S_OK;

    return QWinRTUiaMainProvider::rawProviderForAccessibleId(*elementId, value);
}

// Returns the row index of the item.
HRESULT STDMETHODCALLTYPE QWinRTUiaGridItemProvider::get_Row(INT32 *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->rowIndex();
    return S_OK;
}

// Returns the number of rows occupied by the item.
HRESULT STDMETHODCALLTYPE QWinRTUiaGridItemProvider::get_RowSpan(INT32 *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->rowCount();
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
