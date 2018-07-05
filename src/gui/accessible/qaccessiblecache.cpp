/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessiblecache_p.h"
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcAccessibilityCache, "qt.accessibility.cache");

/*!
    \class QAccessibleCache
    \internal
    \brief Maintains a cache of accessible interfaces.
*/

static QAccessibleCache *accessibleCache = nullptr;

static void cleanupAccessibleCache()
{
    delete accessibleCache;
    accessibleCache = nullptr;
}

QAccessibleCache::~QAccessibleCache()
{
    for (QAccessible::Id id: idToInterface.keys())
        deleteInterface(id);
}

QAccessibleCache *QAccessibleCache::instance()
{
    if (!accessibleCache) {
        accessibleCache = new QAccessibleCache;
        qAddPostRoutine(cleanupAccessibleCache);
    }
    return accessibleCache;
}

/*
  The ID is always in the range [INT_MAX+1, UINT_MAX].
  This makes it easy on windows to reserve the positive integer range
  for the index of a child and not clash with the unique ids.
*/
QAccessible::Id QAccessibleCache::acquireId() const
{
    static const QAccessible::Id FirstId = QAccessible::Id(INT_MAX) + 1;
    static QAccessible::Id lastUsedId = FirstId;

    while (idToInterface.contains(lastUsedId)) {
        // (wrap back when when we reach UINT_MAX - 1)
        // -1 because on Android -1 is taken for the "View" so just avoid it completely for consistency
        if (lastUsedId == UINT_MAX - 1)
            lastUsedId = FirstId;
        else
            ++lastUsedId;
    }

    return lastUsedId;
}

QAccessibleInterface *QAccessibleCache::interfaceForId(QAccessible::Id id) const
{
    return idToInterface.value(id);
}

QAccessible::Id QAccessibleCache::idForInterface(QAccessibleInterface *iface) const
{
    return interfaceToId.value(iface);
}

QAccessible::Id QAccessibleCache::insert(QObject *object, QAccessibleInterface *iface) const
{
    Q_ASSERT(iface);
    Q_UNUSED(object)

    // object might be 0
    Q_ASSERT(!objectToId.contains(object));
    Q_ASSERT_X(!interfaceToId.contains(iface), "", "Accessible interface inserted into cache twice!");

    QAccessible::Id id = acquireId();
    QObject *obj = iface->object();
    Q_ASSERT(object == obj);
    if (obj) {
        objectToId.insert(obj, id);
        connect(obj, &QObject::destroyed, this, &QAccessibleCache::objectDestroyed);
    }
    idToInterface.insert(id, iface);
    interfaceToId.insert(iface, id);
    qCDebug(lcAccessibilityCache) << "insert - id:" << id << " iface:" << iface;
    return id;
}

void QAccessibleCache::objectDestroyed(QObject* obj)
{
    QAccessible::Id id = objectToId.value(obj);
    if (id) {
        Q_ASSERT_X(idToInterface.contains(id), "", "QObject with accessible interface deleted, where interface not in cache!");
        deleteInterface(id, obj);
    }
}

void QAccessibleCache::deleteInterface(QAccessible::Id id, QObject *obj)
{
    QAccessibleInterface *iface = idToInterface.take(id);
    qCDebug(lcAccessibilityCache) << "delete - id:" << id << " iface:" << iface;
    if (!iface) // the interface may be deleted already
        return;
    interfaceToId.take(iface);
    if (!obj)
        obj = iface->object();
    if (obj)
        objectToId.remove(obj);
    delete iface;

#ifdef Q_OS_MAC
    removeCocoaElement(id);
#endif
}

QT_END_NAMESPACE

#endif
