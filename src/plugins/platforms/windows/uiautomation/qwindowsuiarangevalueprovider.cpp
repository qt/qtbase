// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiarangevalueprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaRangeValueProvider::QWindowsUiaRangeValueProvider(QAccessible::Id id) :
    QWindowsUiaBaseProvider(id)
{
}

QWindowsUiaRangeValueProvider::~QWindowsUiaRangeValueProvider()
{
}

HRESULT STDMETHODCALLTYPE QWindowsUiaRangeValueProvider::SetValue(double val)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleValueInterface *valueInterface = accessible->valueInterface();
    if (!valueInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    double minimum = valueInterface->minimumValue().toDouble();
    double maximum = valueInterface->maximumValue().toDouble();
    if ((val < minimum) || (val > maximum))
        return E_INVALIDARG;

    valueInterface->setCurrentValue(QVariant(val));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaRangeValueProvider::get_Value(double *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleValueInterface *valueInterface = accessible->valueInterface();
    if (!valueInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QVariant varValue = valueInterface->currentValue();
    *pRetVal = varValue.toDouble();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaRangeValueProvider::get_IsReadOnly(BOOL *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    *pRetVal = accessible->state().readOnly;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaRangeValueProvider::get_Maximum(double *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleValueInterface *valueInterface = accessible->valueInterface();
    if (!valueInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QVariant varValue = valueInterface->maximumValue();
    *pRetVal = varValue.toDouble();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaRangeValueProvider::get_Minimum(double *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleValueInterface *valueInterface = accessible->valueInterface();
    if (!valueInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QVariant varValue = valueInterface->minimumValue();
    *pRetVal = varValue.toDouble();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsUiaRangeValueProvider::get_LargeChange(double *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;
    return get_SmallChange(pRetVal);
}

HRESULT STDMETHODCALLTYPE QWindowsUiaRangeValueProvider::get_SmallChange(double *pRetVal)
{
    qCDebug(lcQpaUiAutomation) << __FUNCTION__;

    if (!pRetVal)
        return E_INVALIDARG;

    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QAccessibleValueInterface *valueInterface = accessible->valueInterface();
    if (!valueInterface)
        return UIA_E_ELEMENTNOTAVAILABLE;

    QVariant varValue = valueInterface->minimumStepSize();
    *pRetVal = varValue.toDouble();
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
