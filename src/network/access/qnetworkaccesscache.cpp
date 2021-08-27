/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qnetworkaccesscache_p.h"
#include "QtCore/qpointer.h"
#include "QtCore/qdatetime.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkreply_p.h"
#include "qnetworkrequest.h"

#include <vector>

QT_BEGIN_NAMESPACE

enum ExpiryTimeEnum {
    ExpiryTime = 120
};

namespace {
    struct Receiver
    {
        QPointer<QObject> object;
        const char *member;
    };
}

// idea copied from qcache.h
struct QNetworkAccessCache::Node
{
    QDateTime timestamp;
    QByteArray key;

    Node *older, *newer;
    CacheableObject *object;

    int useCount;

    Node()
        : older(nullptr), newer(nullptr), object(nullptr), useCount(0)
    { }
};

QNetworkAccessCache::CacheableObject::CacheableObject()
{
    // leave the members uninitialized
    // they must be initialized by the derived class's constructor
}

QNetworkAccessCache::CacheableObject::~CacheableObject()
{
#if 0 //def QT_DEBUG
    if (!key.isEmpty() && Ptr()->hasEntry(key))
        qWarning() << "QNetworkAccessCache: object" << (void*)this << "key" << key
                   << "destroyed without being removed from cache first!";
#endif
}

void QNetworkAccessCache::CacheableObject::setExpires(bool enable)
{
    expires = enable;
}

void QNetworkAccessCache::CacheableObject::setShareable(bool enable)
{
    shareable = enable;
}

QNetworkAccessCache::QNetworkAccessCache()
    : oldest(nullptr), newest(nullptr)
{
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

    oldest = newest = nullptr;
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

    Q_ASSERT(node != oldest && node != newest);
    Q_ASSERT(node->older == nullptr && node->newer == nullptr);
    Q_ASSERT(node->useCount == 0);


    node->timestamp = QDateTime::currentDateTimeUtc().addSecs(node->object->expiryTimeoutSeconds);
#ifdef QT_DEBUG
    qDebug() << "QNetworkAccessCache case trying to insert=" <<QString::fromUtf8(key) << node->timestamp;
    Node *current = newest;
    while (current) {
        qDebug() << "QNetworkAccessCache item=" << QString::fromUtf8(current->key) << current->timestamp << (current==newest? "newest":"") <<  (current==oldest? "oldest":"");
        current = current->older;
    }
#endif

    if (newest) {
        Q_ASSERT(newest->newer == nullptr);
        if (newest->timestamp < node->timestamp) {
            // Insert as new newest.
            node->older = newest;
            newest->newer = node;
            newest = node;
            Q_ASSERT(newest->newer == nullptr);
        } else {
            // Insert in a sorted way, as different nodes might have had different expiryTimeoutSeconds set.
            Node *current = newest;
            while (current->older != nullptr && current->older->timestamp >= node->timestamp) {
                current = current->older;
            }
            node->older = current->older;
            node->newer = current;
            current->older = node;
            if (node->older == nullptr) {
                oldest = node;
                Q_ASSERT(oldest->older == nullptr);
            }
        }
    } else {
        // no newest yet
        newest = node;
    }
    if (!oldest) {
        // there are no entries, so this is the oldest one too
        oldest = node;
    }
}

/*!
    Removes the entry pointed by \a key from the linked list.
    Returns \c true if the entry removed was the oldest one.
 */
bool QNetworkAccessCache::unlinkEntry(const QByteArray &key)
{
    Node * const node = hash.value(key);
    if (!node)
        return false;

    bool wasOldest = false;
    if (node == oldest) {
        oldest = node->newer;
        wasOldest = true;
    }
    if (node == newest)
        newest = node->older;
    if (node->older)
        node->older->newer = node->newer;
    if (node->newer)
        node->newer->older = node->older;

    node->newer = node->older = nullptr;
    return wasOldest;
}

void QNetworkAccessCache::updateTimer()
{
    timer.stop();

    if (!oldest)
        return;

    qint64 interval = QDateTime::currentDateTimeUtc().msecsTo(oldest->timestamp);
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
    // expire old items
    const QDateTime now = QDateTime::currentDateTimeUtc();

    while (oldest && oldest->timestamp < now) {
        Node *next = oldest->newer;
        oldest->object->dispose();
        hash.remove(oldest->key); // oldest gets deleted
        delete oldest;
        oldest = next;
    }

    // fixup the list
    if (oldest)
        oldest->older = nullptr;
    else
        newest = nullptr;

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
    bool wasOldest = unlinkEntry(key);
    ++node->useCount;

    if (wasOldest)
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

        if (oldest == node)
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
