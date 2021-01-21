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

#ifndef QWINDOWSUIAPROVIDERCACHE_H
#define QWINDOWSUIAPROVIDERCACHE_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

#include <QtCore/qhash.h>
#include <QtGui/qaccessible.h>

QT_BEGIN_NAMESPACE

// Singleton used to cache provider instances using the accessibility ID as the key.
class QWindowsUiaProviderCache : public QObject
{
    QWindowsUiaProviderCache();
    Q_OBJECT
public:
    static QWindowsUiaProviderCache *instance();
    QWindowsUiaBaseProvider *providerForId(QAccessible::Id id) const;
    void insert(QAccessible::Id id, QWindowsUiaBaseProvider *provider);
    void remove(QAccessible::Id id);

private Q_SLOTS:
    void objectDestroyed(QObject *obj);

private:
    QHash<QAccessible::Id, QWindowsUiaBaseProvider *> m_providerTable;
    QHash<QObject *, QAccessible::Id> m_inverseTable;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAPROVIDERCACHE_H
