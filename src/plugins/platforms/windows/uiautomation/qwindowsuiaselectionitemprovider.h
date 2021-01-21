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

#ifndef QWINDOWSUIASELECTIONITEMPROVIDER_H
#define QWINDOWSUIASELECTIONITEMPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Selection Item control pattern provider. Used for List items and radio buttons.
class QWindowsUiaSelectionItemProvider : public QWindowsUiaBaseProvider,
                                         public QWindowsComBase<ISelectionItemProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaSelectionItemProvider)
public:
    explicit QWindowsUiaSelectionItemProvider(QAccessible::Id id);
    virtual ~QWindowsUiaSelectionItemProvider();

    // ISelectionItemProvider
    HRESULT STDMETHODCALLTYPE Select() override;
    HRESULT STDMETHODCALLTYPE AddToSelection() override;
    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
    HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple **pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIASELECTIONITEMPROVIDER_H
