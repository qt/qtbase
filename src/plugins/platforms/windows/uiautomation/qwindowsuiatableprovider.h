// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIATABLEPROVIDER_H
#define QWINDOWSUIATABLEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Table control pattern provider. Used by tables/trees.
class QWindowsUiaTableProvider : public QWindowsUiaBaseProvider, public QComObject<ITableProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaTableProvider)
public:
    explicit QWindowsUiaTableProvider(QAccessible::Id id);
    virtual ~QWindowsUiaTableProvider();

    // ITableProvider
    HRESULT STDMETHODCALLTYPE GetRowHeaders(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetColumnHeaders(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_RowOrColumnMajor(enum RowOrColumnMajor *pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIATABLEPROVIDER_H
