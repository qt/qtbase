// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAGRIDPROVIDER_H
#define QWINDOWSUIAGRIDPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Grid control pattern provider. Used by tables/trees.
class QWindowsUiaGridProvider : public QWindowsUiaBaseProvider,
                                public QWindowsComBase<IGridProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaGridProvider)
public:
    explicit QWindowsUiaGridProvider(QAccessible::Id id);
    virtual ~QWindowsUiaGridProvider();

    // IGridProvider
    HRESULT STDMETHODCALLTYPE GetItem(int row, int column, IRawElementProviderSimple **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_RowCount(int *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_ColumnCount(int *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAGRIDPROVIDER_H
