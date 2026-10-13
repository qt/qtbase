// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qnetworkaccesscache_p.h"
#include "QtCore/qpointer.h"
#include "QtCore/qdeadlinetimer.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkreply_p.h"
#include "qnetworkrequest.h"

#include <vector>

//#define DEBUG_ACCESSCACHE

QT_BEGIN_NAMESPACE

enum ExpiryTimeEnum {
    ExpiryTime = 120
};

// idea copied from qcache.h
struct QNetworkAccessCache::Node
{
    QDeadlineTimer timer;
    QByteArray key;

    Node *previous = nullptr; // "previous" nodes expire "previous"ly (before us)
    Node *next = nullptr; // "next" nodes expire "next" (after us)
    CacheableObject *object = nullptr;

    int useCount = 0;
};

QNetworkAccessCache::CacheableObject::CacheableObject(Options options)
    : expires(options & Option::Expires),
      shareable(options & Option::Shareable)
{

}

QNetworkAccessCache::CacheableObject::~CacheableObject()
{
#if 0 //def QT_DEBUG
    if (!key.isEmpty() && Ptr()->hasEntry(key))
        qWarning() << "QNetworkAccessCache: object" << (void*)this << "key" << key
                   << "destroyed without being removed from cache first!";
#endif
}

QNetworkAccessCache::~QNetworkAccessCache()
{
    clear();
}

void QNetworkAccessCache::clear()
{
    NodeHash hashCopy = hash;
    hash.clear();

    // remove all entries
    NodeHash::Iterator it = hashCopy.begin();
    NodeHash::Iterator end = hashCopy.end();
    for ( ; it != end; ++it) {
        (*it)->object->key.clear();
        (*it)->object->dispose();
        delete (*it);
    }

    // now delete:
    hashCopy.clear();

    timer.stop();

    firstExpiringNode = lastExpiringNode = nullptr;
}

/*!
    Appends the entry given by \a key to the end of the linked list.
    (i.e., makes it the newest entry)
 */
void QNetworkAccessCache::linkEntry(const QByteArray &key)
{
    Node * const node = hash.value(key);
    if (!node)
        return;

    Q_ASSERT(node != firstExpiringNode && node != lastExpiringNode);
    Q_ASSERT(node->previous == nullptr && node->next == nullptr);
    Q_ASSERT(node->useCount == 0);


    node->timer.setPreciseRemainingTime(node->object->expiryTimeoutSeconds);
#ifdef DEBUG_ACCESSCACHE
    qDebug() << "QNetworkAccessCache case trying to insert=" << QString::fromUtf8(key)
             << node->timer.remainingTime() << "milliseconds";
    Node *current = lastExpiringNode;
    while (current) {
        qDebug() << "QNetworkAccessCache item=" << QString::fromUtf8(current->key)
                 << current->timer.remainingTime() << "milliseconds"
                 << (current == lastExpiringNode ? "[last to expire]" : "")
                 << (current == firstExpiringNode ? "[first to expire]" : "");
        current = current->previous;
    }
#endif

    if (lastExpiringNode) {
        Q_ASSERT(lastExpiringNode->next == nullptr);
        if (lastExpiringNode->timer < node->timer) {
            // Insert as new last-to-expire node.
            node->previous = lastExpiringNode;
            lastExpiringNode->next = node;
            lastExpiringNode = node;
        } else {
            // Insert in a sorted way, as different nodes might have had different expiryTimeoutSeconds set.
            Node *current = lastExpiringNode;
            while (current->previous != nullptr && current->previous->timer >= node->timer)
                current = current->previous;
            node->previous = current->previous;
            if (node->previous)
                node->previous->next = node;
            node->next = current;
            current->previous = node;
            if (node->previous == nullptr)
                firstExpiringNode = node;
        }
    } else {
        // no current last-to-expire node
        lastExpiringNode = node;
    }
    if (!firstExpiringNode) {
        // there are no entries, so this is the next-to-expire too
        firstExpiringNode = node;
    }
    Q_ASSERT(firstExpiringNode->previous == nullptr);
    Q_ASSERT(lastExpiringNode->next == nullptr);
}

/*!
    Removes the entry pointed by \a key from the linked list.
    Returns \c true if the entry removed was the next to expire.
 */
bool QNetworkAccessCache::unlinkEntry(const QByteArray &key)
{
    Node * const node = hash.value(key);
    if (!node)
        return false;

    bool wasFirst = false;
    if (node == firstExpiringNode) {
        firstExpiringNode = node->next;
        wasFirst = true;
    }
    if (node == lastExpiringNode)
        lastExpiringNode = node->previous;
    if (node->previous)
        node->previous->next = node->next;
    if (node->next)
        node->next->previous = node->previous;

    node->next = node->previous = nullptr;
    return wasFirst;
}

void QNetworkAccessCache::updateTimer()
{
    timer.stop();

    if (!firstExpiringNode)
        return;

    qint64 interval = firstExpiringNode->timer.remainingTime();
    if (interval <= 0) {
        interval = 0;
    }

    // Plus 10 msec so we don't spam timer events if date comparisons are too fuzzy.
    // This code used to do (broken) rounding, but for ConnectionCacheExpiryTimeoutSecondsAttribute
    // to work we cannot do this.
    // See discussion in https://codereview.qt-project.org/c/qt/qtbase/+/337464
    timer.start(interval + 10, this);
}

bool QNetworkAccessCache::emitEntryReady(Node *node, QObject *target, const char *member)
{
    if (!connect(this, SIGNAL(entryReady(QNetworkAccessCache::CacheableObject*)),
                 target, member, Qt::QueuedConnection))
        return false;

    emit entryReady(node->object);
    disconnect(SIGNAL(entryReady(QNetworkAccessCache::CacheableObject*)));

    return true;
}

void QNetworkAccessCache::timerEvent(QTimerEvent *)
{
    while (firstExpiringNode && firstExpiringNode->timer.hasExpired()) {
        Node *next = firstExpiringNode->next;
        firstExpiringNode->object->dispose();
        hash.remove(firstExpiringNode->key); // `firstExpiringNode` gets deleted
        delete firstExpiringNode;
        firstExpiringNode = next;
    }

    // fixup the list
    if (firstExpiringNode)
        firstExpiringNode->previous = nullptr;
    else
        lastExpiringNode = nullptr;

    updateTimer();
}

void QNetworkAccessCache::addEntry(const QByteArray &key, CacheableObject *entry, qint64 connectionCacheExpiryTimeoutSeconds)
{
    Q_ASSERT(!key.isEmpty());

    if (unlinkEntry(key))
        updateTimer();

    Node *node = hash.value(key);
    if (!node) {
        node = new Node;
        hash.insert(key, node);
    }

    if (node->useCount)
        qWarning("QNetworkAccessCache::addEntry: overriding active cache entry '%s'", key.constData());
    if (node->object)
        node->object->dispose();
    node->object = entry;
    node->object->key = key;
    if (connectionCacheExpiryTimeoutSeconds > -1) {
        node->object->expiryTimeoutSeconds = connectionCacheExpiryTimeoutSeconds; // via ConnectionCacheExpiryTimeoutSecondsAttribute
    } else {
        node->object->expiryTimeoutSeconds = ExpiryTime;
    }
    node->key = key;
    node->useCount = 1;

    // It gets only put into the expiry list in linkEntry (from releaseEntry), when it is not used anymore.
}

bool QNetworkAccessCache::hasEntry(const QByteArray &key) const
{
    return hash.contains(key);
}

QNetworkAccessCache::CacheableObject *QNetworkAccessCache::requestEntryNow(const QByteArray &key)
{
    Node *node = hash.value(key);
    if (!node)
        return nullptr;

    if (node->useCount > 0) {
        if (node->object->shareable) {
            ++node->useCount;
            return node->object;
        }

        // object in use and not shareable
        return nullptr;
    }

    // entry not in use, let the caller have it
    bool wasNext = unlinkEntry(key);
    ++node->useCount;

    if (wasNext)
        updateTimer();
    return node->object;
}

void QNetworkAccessCache::releaseEntry(const QByteArray &key)
{
    Node *node = hash.value(key);
    if (!node) {
        qWarning("QNetworkAccessCache::releaseEntry: trying to release key '%s' that is not in cache", key.constData());
        return;
    }

    Q_ASSERT(node->useCount > 0);

    if (!--node->useCount) {
        // no objects waiting; add it back to the expiry list
        if (node->object->expires)
            linkEntry(key);

        if (firstExpiringNode == node)
            updateTimer();
    }
}

void QNetworkAccessCache::removeEntry(const QByteArray &key)
{
    Node *node = hash.value(key);
    if (!node) {
        qWarning("QNetworkAccessCache::removeEntry: trying to remove key '%s' that is not in cache", key.constData());
        return;
    }

    if (unlinkEntry(key))
        updateTimer();
    if (node->useCount > 1)
        qWarning("QNetworkAccessCache::removeEntry: removing active cache entry '%s'",
                 key.constData());

    node->object->key.clear();
    hash.remove(node->key);
    delete node;
}

QT_END_NAMESPACE

#include "moc_qnetworkaccesscache_p.cpp"
