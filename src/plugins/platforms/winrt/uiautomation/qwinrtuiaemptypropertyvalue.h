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

#ifndef QWINRTUIAEMPTYPROPERTYVALUE_H
#define QWINRTUIAEMPTYPROPERTYVALUE_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <wrl.h>
#include <windows.ui.xaml.h>

QT_BEGIN_NAMESPACE

// Implements an empty property value.
class QWinRTUiaEmptyPropertyValue :
        public Microsoft::WRL::RuntimeClass<ABI::Windows::Foundation::IPropertyValue>
{
    InspectableClass(L"QWinRTUiaEmptyPropertyValue", BaseTrust);
public:

    HRESULT STDMETHODCALLTYPE get_Type(ABI::Windows::Foundation::PropertyType *value)
    {
        *value = ABI::Windows::Foundation::PropertyType_Empty;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_IsNumericScalar(boolean*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt8(BYTE*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetInt16(INT16*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt16(UINT16*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetInt32(INT32*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt32(UINT32*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetInt64(INT64*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt64(UINT64*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetSingle(FLOAT*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetDouble(DOUBLE*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetChar16(WCHAR*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetBoolean(boolean*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetString(HSTRING*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetGuid(GUID*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetDateTime(ABI::Windows::Foundation::DateTime*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetTimeSpan(ABI::Windows::Foundation::TimeSpan*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetPoint(ABI::Windows::Foundation::Point*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetSize(ABI::Windows::Foundation::Size*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetRect(ABI::Windows::Foundation::Rect*) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt8Array(UINT32*, BYTE**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetInt16Array(UINT32*, INT16**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt16Array(UINT32*, UINT16**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetInt32Array(UINT32*, INT32**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt32Array(UINT32*, UINT32**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetInt64Array(UINT32*, INT64**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetUInt64Array(UINT32*, UINT64**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetSingleArray(UINT32*, FLOAT**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetDoubleArray(UINT32*, DOUBLE**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetChar16Array(UINT32*, WCHAR**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetBooleanArray(UINT32*, boolean**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetStringArray(UINT32*, HSTRING**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetInspectableArray(UINT32*, IInspectable***) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetGuidArray(UINT32*, GUID**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetDateTimeArray(UINT32*, ABI::Windows::Foundation::DateTime**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetTimeSpanArray(UINT32*, ABI::Windows::Foundation::TimeSpan**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetPointArray(UINT32*, ABI::Windows::Foundation::Point**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetSizeArray(UINT32*, ABI::Windows::Foundation::Size**) { return E_FAIL; }
    HRESULT STDMETHODCALLTYPE GetRectArray(UINT32*, ABI::Windows::Foundation::Rect**) { return E_FAIL; }
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIAEMPTYPROPERTYVALUE_H
