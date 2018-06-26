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
    QString strVal = QString::fromUtf16(reinterpret_cast<const ushort *>(val));
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

    *pRetVal = bStrFromQString(accessible->text(QAccessible::Value));
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
