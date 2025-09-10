// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAVALUEPROVIDER_H
#define QWINDOWSUIAVALUEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Value control pattern provider.
// Supported for all controls that can return text(QAccessible::Value).
class QWindowsUiaValueProvider : public QWindowsUiaBaseProvider, public QComObject<IValueProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaValueProvider)
public:
    explicit QWindowsUiaValueProvider(QAccessible::Id id);
    virtual ~QWindowsUiaValueProvider();

    // IValueProvider
    HRESULT STDMETHODCALLTYPE SetValue(LPCWSTR val) override;
    HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_Value(BSTR *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAVALUEPROVIDER_H
