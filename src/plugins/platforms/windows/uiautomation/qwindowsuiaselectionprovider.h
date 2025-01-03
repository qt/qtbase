// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIASELECTIONPROVIDER_H
#define QWINDOWSUIASELECTIONPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Selection control pattern provider. Used for Lists.
class QWindowsUiaSelectionProvider : public QWindowsUiaBaseProvider,
                                     public QWindowsComBase<ISelectionProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaSelectionProvider)
public:
    explicit QWindowsUiaSelectionProvider(QAccessible::Id id);
    virtual ~QWindowsUiaSelectionProvider();

    // ISelectionProvider
    HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIASELECTIONPROVIDER_H
