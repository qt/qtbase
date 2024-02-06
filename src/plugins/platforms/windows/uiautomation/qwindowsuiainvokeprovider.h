// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAINVOKEPROVIDER_H
#define QWINDOWSUIAINVOKEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Invoke control pattern provider.
class QWindowsUiaInvokeProvider : public QWindowsUiaBaseProvider, public QComObject<IInvokeProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaInvokeProvider)
public:
    explicit QWindowsUiaInvokeProvider(QAccessible::Id id);
    virtual ~QWindowsUiaInvokeProvider();

    // IInvokeProvider
    HRESULT STDMETHODCALLTYPE Invoke() override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAINVOKEPROVIDER_H
