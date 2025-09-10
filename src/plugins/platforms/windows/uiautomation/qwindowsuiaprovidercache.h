// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAPROVIDERCACHE_H
#define QWINDOWSUIAPROVIDERCACHE_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiamainprovider.h"

#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
#include <QtGui/qaccessible.h>
#include <QtCore/private/qcomptr_p.h>

QT_BEGIN_NAMESPACE

// Singleton used to cache provider instances using the accessibility ID as the key.
class QWindowsUiaProviderCache : public QObject
{
    QWindowsUiaProviderCache();
    Q_OBJECT
public:
    static QWindowsUiaProviderCache *instance();
    ComPtr<QWindowsUiaMainProvider> providerForId(QAccessible::Id id) const;
    void insert(QAccessible::Id id, QWindowsUiaMainProvider *provider);

private Q_SLOTS:
    void remove(QObject *obj);

private:
    mutable QMutex m_tableMutex; // TODO: Can tables be accessed concurrently?
    QHash<QAccessible::Id, QWindowsUiaMainProvider *> m_providerTable;
    QHash<QObject *, QAccessible::Id> m_inverseTable;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAPROVIDERCACHE_H
