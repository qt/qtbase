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

#include "qwinrtuiarangevalueprovider.h"
#include "qwinrtuiametadatacache.h"
#include "qwinrtuiautils.h"

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

QT_BEGIN_NAMESPACE

using namespace QWinRTUiAutomation;
using namespace ABI::Windows::UI::Xaml::Automation;
using namespace ABI::Windows::UI::Xaml::Automation::Provider;

QWinRTUiaRangeValueProvider::QWinRTUiaRangeValueProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaRangeValueProvider::~QWinRTUiaRangeValueProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaRangeValueProvider::get_IsReadOnly(boolean *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = (metadata->state().readOnly != 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaRangeValueProvider::get_LargeChange(DOUBLE *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->minimumStepSize();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaRangeValueProvider::get_Maximum(DOUBLE *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->maximumValue();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaRangeValueProvider::get_Minimum(DOUBLE *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->minimumValue();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaRangeValueProvider::get_SmallChange(DOUBLE *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->minimumStepSize();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaRangeValueProvider::get_Value(DOUBLE *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;

    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = metadata->currentValue();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWinRTUiaRangeValueProvider::SetValue(DOUBLE value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();

    QEventDispatcherWinRT::runOnMainThread([accid, value]() {
        if (QAccessibleInterface *accessible = accessibleForId(accid)) {
            if (QAccessibleValueInterface *valueInterface = accessible->valueInterface()) {
                double minimum = valueInterface->minimumValue().toDouble();
                double maximum = valueInterface->maximumValue().toDouble();
                if ((value >= minimum) && (value <= maximum)) {
                    valueInterface->setCurrentValue(QVariant(value));
                }
            }
        }
        QWinRTUiaMetadataCache::instance()->load(accid);
        return S_OK;
    }, 0);

    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
