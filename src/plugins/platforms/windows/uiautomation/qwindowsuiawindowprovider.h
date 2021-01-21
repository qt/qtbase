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

#ifndef QWINDOWSUIAWINDOWPROVIDER_H
#define QWINDOWSUIAWINDOWPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

class QWindowsUiaWindowProvider : public QWindowsUiaBaseProvider,
                                  public QWindowsComBase<IWindowProvider>
{
    Q_DISABLE_COPY(QWindowsUiaWindowProvider)
public:
    explicit QWindowsUiaWindowProvider(QAccessible::Id id);
    ~QWindowsUiaWindowProvider() override;

    HRESULT STDMETHODCALLTYPE SetVisualState(WindowVisualState state) override;
    HRESULT STDMETHODCALLTYPE Close( void) override;
    HRESULT STDMETHODCALLTYPE WaitForInputIdle(int milliseconds, __RPC__out BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_CanMaximize(__RPC__out BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_CanMinimize(__RPC__out BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_IsModal(__RPC__out BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_WindowVisualState(__RPC__out WindowVisualState *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_WindowInteractionState(__RPC__out WindowInteractionState *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_IsTopmost(__RPC__out BOOL *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAWINDOWPROVIDER_H
