// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIARANGEVALUEPROVIDER_H
#define QWINDOWSUIARANGEVALUEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Range Value control pattern provider.
class QWindowsUiaRangeValueProvider : public QWindowsUiaBaseProvider,
                                      public QComObject<IRangeValueProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaRangeValueProvider)
public:
    explicit QWindowsUiaRangeValueProvider(QAccessible::Id id);
    virtual ~QWindowsUiaRangeValueProvider();

    // IRangeValueProvider
    HRESULT STDMETHODCALLTYPE SetValue(double val) override;
    HRESULT STDMETHODCALLTYPE get_Value(double *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_Maximum(double *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_Minimum(double *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_LargeChange(double *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_SmallChange(double *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIARANGEVALUEPROVIDER_H
