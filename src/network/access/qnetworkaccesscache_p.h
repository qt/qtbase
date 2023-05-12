// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNETWORKACCESSCACHE_P_H
#define QNETWORKACCESSCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtCore/qobject.h"
#include "QtCore/qbasictimer.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qhash.h"
#include "QtCore/qmetatype.h"

QT_BEGIN_NAMESPACE

class QNetworkRequest;
class QUrl;

// this class is not about caching files but about
// caching objects used by QNetworkAccessManager, e.g. existing TCP connections
// or credentials.
class QNetworkAccessCache: public QObject
{
    Q_OBJECT
public:
    struct Node;
    typedef QHash<QByteArray, Node *> NodeHash;

    class CacheableObject
    {
        friend class QNetworkAccessCache;
        QByteArray key;
        bool expires;
        bool shareable;
        qint64 expiryTimeoutSeconds;
    public:
        CacheableObject();
        virtual ~CacheableObject();
        virtual void dispose() = 0;
        inline QByteArray cacheKey() const { return key; }

    protected:
        void setExpires(bool enable);
        void setShareable(bool enable);
    };

    ~QNetworkAccessCache();

    void clear();

    void addEntry(const QByteArray &key, CacheableObject *entry, qint64 connectionCacheExpiryTimeoutSeconds = -1);
    bool hasEntry(const QByteArray &key) const;
    CacheableObject *requestEntryNow(const QByteArray &key);
    void releaseEntry(const QByteArray &key);
    void removeEntry(const QByteArray &key);

signals:
    void entryReady(QNetworkAccessCache::CacheableObject *);

protected:
    void timerEvent(QTimerEvent *) override;

private:
    // idea copied from qcache.h
    NodeHash hash;
    Node *firstExpiringNode = nullptr;
    Node *lastExpiringNode = nullptr;

    QBasicTimer timer;

    void linkEntry(const QByteArray &key);
    bool unlinkEntry(const QByteArray &key);
    void updateTimer();
    bool emitEntryReady(Node *node, QObject *target, const char *member);
};

QT_END_NAMESPACE

#endif
