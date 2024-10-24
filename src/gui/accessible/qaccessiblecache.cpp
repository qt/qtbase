// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblecache_p.h"
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcAccessibilityCache, "qt.accessibility.cache");

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
    static QAccessible::Id nextId = FirstId;

    while (idToInterface.contains(nextId)) {
        // (wrap back when when we reach UINT_MAX - 1)
        // -1 because on Android -1 is taken for the "View" so just avoid it completely for consistency
        if (nextId == UINT_MAX - 1)
            nextId = FirstId;
        else
            ++nextId;
    }

    return nextId++;
}

QAccessibleInterface *QAccessibleCache::interfaceForId(QAccessible::Id id) const
{
    return idToInterface.value(id);
}

QAccessible::Id QAccessibleCache::idForInterface(QAccessibleInterface *iface) const
{
    return interfaceToId.value(iface);
}

QAccessible::Id QAccessibleCache::idForObject(QObject *obj) const
{
    if (obj) {
        const QMetaObject *mo = obj->metaObject();
        for (auto pair : objectToId.values(obj)) {
            if (pair.second == mo) {
                return pair.first;
            }
        }
    }
    return 0;
}

/*!
 * \internal
 *
 * returns true if the cache has an interface for the object and its corresponding QMetaObject
 */
bool QAccessibleCache::containsObject(QObject *obj) const
{
    if (obj) {
        const QMetaObject *mo = obj->metaObject();
        for (auto pair : objectToId.values(obj)) {
            if (pair.second == mo) {
                return true;
            }
        }
    }
    return false;
}

QAccessible::Id QAccessibleCache::insert(QObject *object, QAccessibleInterface *iface) const
{
    Q_ASSERT(iface);
    Q_UNUSED(object);

    // object might be 0
    Q_ASSERT(!containsObject(object));
    Q_ASSERT_X(!interfaceToId.contains(iface), "", "Accessible interface inserted into cache twice!");

    QAccessible::Id id = acquireId();
    QObject *obj = iface->object();
    Q_ASSERT(object == obj);
    if (obj) {
        objectToId.insert(obj, std::pair(id, obj->metaObject()));
        connect(obj, &QObject::destroyed, this, &QAccessibleCache::objectDestroyed);
    }
    idToInterface.insert(id, iface);
    interfaceToId.insert(iface, id);
    qCDebug(lcAccessibilityCache) << "insert - id:" << id << " iface:" << iface;
    return id;
}

void QAccessibleCache::objectDestroyed(QObject* obj)
{
    /*
    In some cases we might add a not fully-constructed object to the cache. This might happen with
    for instance QWidget subclasses that are in the construction phase. If updateAccessibility() is
    called in the constructor of QWidget (directly or indirectly), it will end up asking for the
    classname of that widget in order to know which accessibility interface subclass the
    accessibility factory should instantiate and return. However, since that requires a virtual
    call to metaObject(), it will return the metaObject() of QWidget (not for the subclass), and so
    the factory will ultimately return a rather generic QAccessibleWidget instead of a more
    specialized interface. Even though it is a "incomplete" interface it will be put in the cache
    and it will be usable as if the object is a widget. In order for the cache to not just return
    the same generic QAccessibleWidget for that object, we have to check if the cache matches
    the objects QMetaObject. We therefore use a QMultiHash and also store the QMetaObject * in
    the value. We therefore might potentially store several values for the corresponding object
    (in theory one for each level in the class inheritance chain)

    This means that after the object have been fully constructed, we will at some point again query
    for the interface for the same object, but now its metaObject() returns the correct
    QMetaObject, so it won't return the QAccessibleWidget that is associated with the object in the
    cache. Instead it will go to the factory and create the _correct_ specialized interface for the
    object. If that succeeded, it will also put that entry in the cache. We will therefore in those
    cases insert *two* cache entries for the same object (using QMultiHash). They both must live
    until the object is destroyed.

    So when the object is destroyed we might have to delete two entries from the cache.
    */
    for (auto pair : objectToId.values(obj)) {
        QAccessible::Id id = pair.first;
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

#ifdef Q_OS_APPLE
    removeAccessibleElement(id);
#endif
}

QT_END_NAMESPACE

#include "moc_qaccessiblecache_p.cpp"

#endif
