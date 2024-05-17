// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAGRIDITEMPROVIDER_H
#define QWINDOWSUIAGRIDITEMPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Grid Item control pattern provider. Used by items within a table/tree.
class QWindowsUiaGridItemProvider : public QWindowsUiaBaseProvider,
                                    public QComObject<IGridItemProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaGridItemProvider)
public:
    explicit QWindowsUiaGridItemProvider(QAccessible::Id id);
    virtual ~QWindowsUiaGridItemProvider();

    // IGridItemProvider
    HRESULT STDMETHODCALLTYPE get_Row(int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_Column(int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_RowSpan(int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_ColumnSpan(int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_ContainingGrid(IRawElementProviderSimple **pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAGRIDITEMPROVIDER_H
