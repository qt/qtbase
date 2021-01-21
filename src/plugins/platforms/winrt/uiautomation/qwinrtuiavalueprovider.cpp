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

#include "qwinrtuiavalueprovider.h"
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

QWinRTUiaValueProvider::QWinRTUiaValueProvider(QAccessible::Id id) :
    QWinRTUiaBaseProvider(id)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

QWinRTUiaValueProvider::~QWinRTUiaValueProvider()
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
}

// True for read-only controls.
HRESULT STDMETHODCALLTYPE QWinRTUiaValueProvider::get_IsReadOnly(boolean *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    *value = (metadata->state().readOnly != 0);
    return S_OK;
}

// Returns the value in text form.
HRESULT STDMETHODCALLTYPE QWinRTUiaValueProvider::get_Value(HSTRING *value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!value)
        return E_INVALIDARG;
    QSharedPointer<QWinRTUiaControlMetadata> metadata = QWinRTUiaMetadataCache::instance()->metadataForId(id());
    return qHString(metadata->value(), value);
}

// Sets the value associated with the control.
HRESULT STDMETHODCALLTYPE QWinRTUiaValueProvider::SetValue(HSTRING value)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    auto accid = id();

    QString tmpValue = hStrToQStr(value);

    QEventDispatcherWinRT::runOnMainThread([accid, tmpValue]() {

        if (QAccessibleInterface *accessible = accessibleForId(accid)) {

            // First sets the value as a text.
            accessible->setText(QAccessible::Value, tmpValue);

            // Then, if the control supports the value interface (range value)
            // and the supplied text can be converted to a number, and that number
            // lies within the min/max limits, sets it as the control's current (numeric) value.
            if (QAccessibleValueInterface *valueInterface = accessible->valueInterface()) {
                bool ok = false;
                double numval = tmpValue.toDouble(&ok);
                if (ok) {
                    double minimum = valueInterface->minimumValue().toDouble();
                    double maximum = valueInterface->maximumValue().toDouble();
                    if ((numval >= minimum) && (numval <= maximum)) {
                        valueInterface->setCurrentValue(QVariant(numval));
                    }
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
