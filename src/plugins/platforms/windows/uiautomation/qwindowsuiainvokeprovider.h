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

#ifndef QWINDOWSUIAINVOKEPROVIDER_H
#define QWINDOWSUIAINVOKEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Invoke control pattern provider.
class QWindowsUiaInvokeProvider : public QWindowsUiaBaseProvider,
                                  public QWindowsComBase<IInvokeProvider>
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
