// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiavalueprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaValueProvider::QWindowsUiaValueProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaValueProvider::~QWindowsUiaValueProvider()
{
}

// Sets the value associated with the control.
HRESULT STDMETHODCALLTYPE QWindowsUiaValueProvider::SetValue(LPCWSTR val)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    // First sets the value as a text.
    QString strVal = QString::fromUtf16(reinterpret_cast<const char16_t *>(val));
    accessible->setText(QAccessible::Value, strVal);

    // Then, if the control supports the value interface (range value)
    // and the supplied text can be converted to a number, and that number
    // lies within the min/max limits, sets it as the control's current (numeric) value.
    if (QAccessibleValueInterface *valueInterface = accessible->valueInterface()) {
        bool ok = false;
        double numval = strVal.toDouble(&ok);
        if (ok) {
            double minimum = valueInterface->minimumValue().toDouble();
            double maximum = valueInterface->maximumValue().toDouble();
            if ((numval >= minimum) && (numval <= maximum)) {
                valueInterface->setCurrentValue(QVariant(numval));
            }
        }
    }
    return S_OK;
}

// True for read-only controls.
HRESULT STDMETHODCALLTYPE QWindowsUiaValueProvider::get_IsReadOnly(BOOL *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = FALSE;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = accessible->state().readOnly;
    return S_OK;
}

// Returns the value in text form.
HRESULT STDMETHODCALLTYPE QWindowsUiaValueProvider::get_Value(BSTR *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;
    *pRetVal = nullptr;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = QBStr(accessible->text(QAccessible::Value)).release();
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
