/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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
    typedef QHash<QByteArray, Node> NodeHash;

    class CacheableObject
    {
        friend class QNetworkAccessCache;
        QByteArray key;
        bool expires;
        bool shareable;
    public:
        CacheableObject();
        virtual ~CacheableObject();
        virtual void dispose() = 0;
        inline QByteArray cacheKey() const { return key; }

    protected:
        void setExpires(bool enable);
        void setShareable(bool enable);
    };

    QNetworkAccessCache();
    ~QNetworkAccessCache();

    void clear();

    void addEntry(const QByteArray &key, CacheableObject *entry);
    bool hasEntry(const QByteArray &key) const;
    bool requestEntry(const QByteArray &key, QObject *target, const char *member);
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
    Node *oldest;
    Node *newest;

    QBasicTimer timer;

    void linkEntry(const QByteArray &key);
    bool unlinkEntry(const QByteArray &key);
    void updateTimer();
    bool emitEntryReady(Node *node, QObject *target, const char *member);
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QNetworkAccessCache::CacheableObject*)

#endif
