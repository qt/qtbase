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
