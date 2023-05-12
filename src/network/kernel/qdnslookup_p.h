// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDNSLOOKUP_P_H
#define QDNSLOOKUP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QDnsLookup class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtCore/qmutex.h"
#include "QtCore/qrunnable.h"
#include "QtCore/qsharedpointer.h"
#if QT_CONFIG(thread)
#include "QtCore/qthreadpool.h"
#endif
#include "QtNetwork/qdnslookup.h"
#include "QtNetwork/qhostaddress.h"
#include "private/qobject_p.h"

QT_REQUIRE_CONFIG(dnslookup);

QT_BEGIN_NAMESPACE

//#define QDNSLOOKUP_DEBUG

constexpr qsizetype MaxDomainNameLength = 255;
class QDnsLookupRunnable;

class QDnsLookupReply
{
public:
    QDnsLookupReply()
        : error(QDnsLookup::NoError)
    { }

    QDnsLookup::Error error;
    QString errorString;

    QList<QDnsDomainNameRecord> canonicalNameRecords;
    QList<QDnsHostAddressRecord> hostAddressRecords;
    QList<QDnsMailExchangeRecord> mailExchangeRecords;
    QList<QDnsDomainNameRecord> nameServerRecords;
    QList<QDnsDomainNameRecord> pointerRecords;
    QList<QDnsServiceRecord> serviceRecords;
    QList<QDnsTextRecord> textRecords;
};

class QDnsLookupPrivate : public QObjectPrivate
{
public:
    QDnsLookupPrivate()
        : isFinished(false)
        , type(QDnsLookup::A)
        , runnable(nullptr)
    { }

    void _q_lookupFinished(const QDnsLookupReply &reply);

    static const char *msgNoIpV6NameServerAdresses;

    bool isFinished;

    void nameChanged()
    {
        emit q_func()->nameChanged(name);
    }
    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, QString, name,
                               &QDnsLookupPrivate::nameChanged);

    void typeChanged()
    {
        emit q_func()->typeChanged(type);
    }

    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, QDnsLookup::Type,
                               type, &QDnsLookupPrivate::typeChanged);

    void nameserverChanged()
    {
        emit q_func()->nameserverChanged(nameserver);
    }
    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, QHostAddress, nameserver,
                               &QDnsLookupPrivate::nameserverChanged);

    QDnsLookupReply reply;
    QDnsLookupRunnable *runnable;

    Q_DECLARE_PUBLIC(QDnsLookup)
};

class QDnsLookupRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    QDnsLookupRunnable(QDnsLookup::Type type, const QByteArray &name, const QHostAddress &nameserver)
        : requestType(type)
        , requestName(name)
        , nameserver(nameserver)
    { }
    void run() override;

signals:
    void finished(const QDnsLookupReply &reply);

private:
    static void query(const int requestType, const QByteArray &requestName, const QHostAddress &nameserver, QDnsLookupReply *reply);
    QDnsLookup::Type requestType;
    QByteArray requestName;
    QHostAddress nameserver;
};

#if QT_CONFIG(thread)
class QDnsLookupThreadPool : public QThreadPool
{
    Q_OBJECT

public:
    QDnsLookupThreadPool();
    void start(QRunnable *runnable);

private slots:
    void _q_applicationDestroyed();

private:
    QMutex signalsMutex;
    bool signalsConnected;
};
#endif // QT_CONFIG(thread)

class QDnsRecordPrivate : public QSharedData
{
public:
    QDnsRecordPrivate()
        : timeToLive(0)
    { }

    QString name;
    quint32 timeToLive;
};

class QDnsDomainNameRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsDomainNameRecordPrivate()
    { }

    QString value;
};

class QDnsHostAddressRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsHostAddressRecordPrivate()
    { }

    QHostAddress value;
};

class QDnsMailExchangeRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsMailExchangeRecordPrivate()
        : preference(0)
    { }

    QString exchange;
    quint16 preference;
};

class QDnsServiceRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsServiceRecordPrivate()
        : port(0),
          priority(0),
          weight(0)
    { }

    QString target;
    quint16 port;
    quint16 priority;
    quint16 weight;
};

class QDnsTextRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsTextRecordPrivate()
    { }

    QList<QByteArray> values;
};

QT_END_NAMESPACE

#endif // QDNSLOOKUP_P_H
