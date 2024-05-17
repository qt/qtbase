// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIASELECTIONITEMPROVIDER_H
#define QWINDOWSUIASELECTIONITEMPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Selection Item control pattern provider. Used for List items and radio buttons.
class QWindowsUiaSelectionItemProvider : public QWindowsUiaBaseProvider,
                                         public QComObject<ISelectionItemProvider>
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
