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

#ifndef QWINRTUIAPROVIDERCACHE_H
#define QWINRTUIAPROVIDERCACHE_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiabaseprovider.h"

#include <QtCore/QHash>
#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>

QT_BEGIN_NAMESPACE

// Singleton used to cache provider instances using the accessibility ID as the key.
class QWinRTUiaProviderCache : public QObject
{
    QWinRTUiaProviderCache();
    Q_OBJECT
public:
    static QWinRTUiaProviderCache *instance();
    QWinRTUiaBaseProvider *providerForId(QAccessible::Id id) const;
    void insert(QAccessible::Id id, QWinRTUiaBaseProvider *provider);
    void remove(QAccessible::Id id);

private Q_SLOTS:
    void objectDestroyed(QObject *obj);

private:
    QHash<QAccessible::Id, QWinRTUiaBaseProvider *> m_providerTable;
    QHash<QObject *, QAccessible::Id> m_inverseTable;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIAPROVIDERCACHE_H
