// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIATABLEITEMPROVIDER_H
#define QWINDOWSUIATABLEITEMPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Table Item control pattern provider. Used by items within a table/tree.
class QWindowsUiaTableItemProvider : public QWindowsUiaBaseProvider,
                                     public QComObject<ITableItemProvider>
{
    Q_DISABLE_COPY_MOVE(QWindowsUiaTableItemProvider)
public:
    explicit QWindowsUiaTableItemProvider(QAccessible::Id id);
    virtual ~QWindowsUiaTableItemProvider();

    // ITableItemProvider
    HRESULT STDMETHODCALLTYPE GetRowHeaderItems(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetColumnHeaderItems(SAFEARRAY **pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIATABLEITEMPROVIDER_H
