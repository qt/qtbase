// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qdnslookup.h"
#include "qdnslookup_p.h"

#include <qapplicationstatic.h>
#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qendian.h>
#include <qloggingcategory.h>
#include <qrandom.h>
#include <qspan.h>
#include <qurl.h>

#if QT_CONFIG(ssl)
#  include <qsslsocket.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcDnsLookup, "qt.network.dnslookup", QtCriticalMsg)

namespace {
struct QDnsLookupThreadPool : QThreadPool
{
    QDnsLookupThreadPool()
    {
        // Run up to 5 lookups in parallel.
        setMaxThreadCount(5);
    }
};
}

Q_APPLICATION_STATIC(QDnsLookupThreadPool, theDnsLookupThreadPool);

static bool qt_qdnsmailexchangerecord_less_than(const QDnsMailExchangeRecord &r1, const QDnsMailExchangeRecord &r2)
{
    // Lower numbers are more preferred than higher ones.
    return r1.preference() < r2.preference();
}

/*
    Sorts a list of QDnsMailExchangeRecord objects according to RFC 5321.
*/

static void qt_qdnsmailexchangerecord_sort(QList<QDnsMailExchangeRecord> &records)
{
    // If we have no more than one result, we are done.
    if (records.size() <= 1)
        return;

    // Order the records by preference.
    std::sort(records.begin(), records.end(), qt_qdnsmailexchangerecord_less_than);

    int i = 0;
    while (i < records.size()) {

        // Determine the slice of records with the current preference.
        QList<QDnsMailExchangeRecord> slice;
        const quint16 slicePreference = records.at(i).preference();
        for (int j = i; j < records.size(); ++j) {
            if (records.at(j).preference() != slicePreference)
                break;
            slice << records.at(j);
        }

        // Randomize the slice of records.
        while (!slice.isEmpty()) {
            const unsigned int pos = QRandomGenerator::global()->bounded(slice.size());
            records[i++] = slice.takeAt(pos);
        }
    }
}

static bool qt_qdnsservicerecord_less_than(const QDnsServiceRecord &r1, const QDnsServiceRecord &r2)
{
    // Order by priority, or if the priorities are equal,
    // put zero weight records first.
    return r1.priority() < r2.priority()
       || (r1.priority() == r2.priority()
        && r1.weight() == 0 && r2.weight() > 0);
}

/*
    Sorts a list of QDnsServiceRecord objects according to RFC 2782.
*/

static void qt_qdnsservicerecord_sort(QList<QDnsServiceRecord> &records)
{
    // If we have no more than one result, we are done.
    if (records.size() <= 1)
        return;

    // Order the records by priority, and for records with an equal
    // priority, put records with a zero weight first.
    std::sort(records.begin(), records.end(), qt_qdnsservicerecord_less_than);

    int i = 0;
    while (i < records.size()) {

        // Determine the slice of records with the current priority.
        QList<QDnsServiceRecord> slice;
        const quint16 slicePriority = records.at(i).priority();
        unsigned int sliceWeight = 0;
        for (int j = i; j < records.size(); ++j) {
            if (records.at(j).priority() != slicePriority)
                break;
            sliceWeight += records.at(j).weight();
            slice << records.at(j);
        }
#ifdef QDNSLOOKUP_DEBUG
        qDebug("qt_qdnsservicerecord_sort() : priority %i (size: %i, total weight: %i)",
               slicePriority, slice.size(), sliceWeight);
#endif

        // Order the slice of records.
        while (!slice.isEmpty()) {
            const unsigned int weightThreshold = QRandomGenerator::global()->bounded(sliceWeight + 1);
            unsigned int summedWeight = 0;
            for (int j = 0; j < slice.size(); ++j) {
                summedWeight += slice.at(j).weight();
                if (summedWeight >= weightThreshold) {
#ifdef QDNSLOOKUP_DEBUG
                    qDebug("qt_qdnsservicerecord_sort() : adding %s %i (weight: %i)",
                           qPrintable(slice.at(j).target()), slice.at(j).port(),
                           slice.at(j).weight());
#endif
                    // Adjust the slice weight and take the current record.
                    sliceWeight -= slice.at(j).weight();
                    records[i++] = slice.takeAt(j);
                    break;
                }
            }
        }
    }
}

/*!
    \class QDnsLookup
    \brief The QDnsLookup class represents a DNS lookup.
    \since 5.0

    \inmodule QtNetwork
    \ingroup network

    QDnsLookup uses the mechanisms provided by the operating system to perform
    DNS lookups. To perform a lookup you need to specify a \l name and \l type
    then invoke the \l{QDnsLookup::lookup()}{lookup()} slot. The
    \l{QDnsLookup::finished()}{finished()} signal will be emitted upon
    completion.

    For example, you can determine which servers an XMPP chat client should
    connect to for a given domain with:

    \snippet code/src_network_kernel_qdnslookup.cpp 0

    Once the request finishes you can handle the results with:

    \snippet code/src_network_kernel_qdnslookup.cpp 1

    \note If you simply want to find the IP address(es) associated with a host
    name, or the host name associated with an IP address you should use
    QHostInfo instead.

    \section1 DNS-over-TLS and Authentic Data

    QDnsLookup supports DNS-over-TLS (DoT, as specified by \l{RFC 7858}) on
    some platforms. That currently includes all Unix platforms where regular
    queries are supported, if \l QSslSocket support is present in Qt. To query
    if support is present at runtime, use isProtocolSupported().

    When using DNS-over-TLS, QDnsLookup only implements the "Opportunistic
    Privacy Profile" method of authentication, as described in \l{RFC 7858}
    section 4.1. In this mode, QDnsLookup (through \l QSslSocket) only
    validates that the server presents a certificate that is valid for the
    server being connected to. Clients may use setSslConfiguration() to impose
    additional restrictions and sslConfiguration() to obtain information after
    the query is complete.

    QDnsLookup will request DNS servers queried over TLS to perform
    authentication on the data they return. If they confirm the data is valid,
    the \l authenticData property will be set to true. QDnsLookup does not
    verify the integrity of the data by itself, so applications should only
    trust this property on servers they have confirmed through other means to
    be trustworthy.

    \section2 Authentic Data without TLS

    QDnsLookup request Authentic Data for any server set with setNameserver(),
    even if TLS encryption is not required. This is useful when querying a
    caching nameserver on the same host as the application or on a trusted
    network. Though similar to the TLS case, the application is responsible for
    determining if the server it chose to use is trustworthy, and if the
    unencrypted connection cannot be tampered with.

    QDnsLookup obeys the system configuration to request Authentic Data on the
    default nameserver (that is, if setNameserver() is not called). This is
    currently only supported on Linux systems using glibc 2.31 or later. On any
    other systems, QDnsLookup will ignore the AD bit in the query header.
*/

/*!
    \enum QDnsLookup::Error

    Indicates all possible error conditions found during the
    processing of the DNS lookup.

    \value NoError              no error condition.

    \value ResolverError        there was an error initializing the system's
    DNS resolver.

    \value OperationCancelledError  the lookup was aborted using the abort()
    method.

    \value InvalidRequestError  the requested DNS lookup was invalid.

    \value InvalidReplyError    the reply returned by the server was invalid.

    \value ServerFailureError   the server encountered an internal failure
    while processing the request (SERVFAIL).

    \value ServerRefusedError   the server refused to process the request for
    security or policy reasons (REFUSED).

    \value NotFoundError        the requested domain name does not exist
    (NXDOMAIN).

    \value TimeoutError         the server was not reached or did not reply
    in time (since 6.6).
*/

/*!
    \enum QDnsLookup::Type

    Indicates the type of DNS lookup that was performed.

    \value A        IPv4 address records.

    \value AAAA     IPv6 address records.

    \value ANY      any records.

    \value CNAME    canonical name records.

    \value MX       mail exchange records.

    \value NS       name server records.

    \value PTR      pointer records.

    \value SRV      service records.

    \value[since 6.8] TLSA     TLS association records.

    \value TXT      text records.
*/

/*!
    \enum QDnsLookup::Protocol

    Indicates the type of DNS server that is being queried.

    \value Standard
        Regular, unencrypted DNS, using UDP and falling back to TCP as necessary
        (default port: 53)

    \value DnsOverTls
        Encrypted DNS over TLS (DoT, as specified by \l{RFC 7858}), over TCP
        (default port: 853)

    \sa isProtocolSupported(), nameserverProtocol, setNameserver()
*/

/*!
    \since 6.8

    Returns true if DNS queries using \a protocol are supported with QDnsLookup.

    \sa nameserverProtocol
*/
bool QDnsLookup::isProtocolSupported(Protocol protocol)
{
#if QT_CONFIG(libresolv) || defined(Q_OS_WIN)
    switch (protocol) {
    case QDnsLookup::Standard:
        return true;
    case QDnsLookup::DnsOverTls:
#  if QT_CONFIG(ssl)
        if (QSslSocket::supportsSsl())
            return true;
#  endif
        return false;
    }
#else
    Q_UNUSED(protocol)
#endif
    return false;
}

/*!
    \since 6.8

    Returns the standard (default) port number for the protocol \a protocol.

    \sa isProtocolSupported()
*/
quint16 QDnsLookup::defaultPortForProtocol(Protocol protocol) noexcept
{
    switch (protocol) {
    case QDnsLookup::Standard:
        return DnsPort;
    case QDnsLookup::DnsOverTls:
        return DnsOverTlsPort;
    }
    return 0;       // will probably fail somewhere
}

/*!
    \fn void QDnsLookup::finished()

    This signal is emitted when the reply has finished processing.
*/

/*!
    \fn void QDnsLookup::nameChanged(const QString &name)

    This signal is emitted when the lookup \l name changes.
    \a name is the new lookup name.
*/

/*!
    \fn void QDnsLookup::typeChanged(QDnsLookup::Type type)

    This signal is emitted when the lookup \l type changes.
    \a type is the new lookup type.
*/

/*!
    Constructs a QDnsLookup object and sets \a parent as the parent object.

    The \l type property will default to QDnsLookup::A.
*/

QDnsLookup::QDnsLookup(QObject *parent)
    : QObject(*new QDnsLookupPrivate, parent)
{
}

/*!
    Constructs a QDnsLookup object for the given \a type and \a name and sets
    \a parent as the parent object.
*/

QDnsLookup::QDnsLookup(Type type, const QString &name, QObject *parent)
    : QObject(*new QDnsLookupPrivate, parent)
{
    Q_D(QDnsLookup);
    d->name = name;
    d->type = type;
}

/*!
    \fn QDnsLookup::QDnsLookup(Type type, const QString &name, const QHostAddress &nameserver, QObject *parent)
    \since 5.4

    Constructs a QDnsLookup object to issue a query for \a name of record type
    \a type, using the DNS server \a nameserver running on the default DNS port,
    and sets \a parent as the parent object.
*/

QDnsLookup::QDnsLookup(Type type, const QString &name, const QHostAddress &nameserver, QObject *parent)
    : QDnsLookup(type, name, nameserver, 0, parent)
{
}

/*!
    \fn QDnsLookup::QDnsLookup(Type type, const QString &name, const QHostAddress &nameserver, quint16 port, QObject *parent)
    \since 6.6

    Constructs a QDnsLookup object to issue a query for \a name of record type
    \a type, using the DNS server \a nameserver running on port \a port, and
    sets \a parent as the parent object.

//! [nameserver-port]
    \note Setting the port number to any value other than the default (53) can
    cause the name resolution to fail, depending on the operating system
    limitations and firewalls, if the nameserverProtocol() to be used
    QDnsLookup::Standard. Notably, the Windows API used by QDnsLookup is unable
    to handle alternate port numbers.
//! [nameserver-port]
*/
QDnsLookup::QDnsLookup(Type type, const QString &name, const QHostAddress &nameserver, quint16 port, QObject *parent)
    : QObject(*new QDnsLookupPrivate, parent)
{
    Q_D(QDnsLookup);
    d->name = name;
    d->type = type;
    d->port = port;
    d->nameserver = nameserver;
}

/*!
    \since 6.8

    Constructs a QDnsLookup object to issue a query for \a name of record type
    \a type, using the DNS server \a nameserver running on port \a port, and
    sets \a parent as the parent object.

    The query will be sent using \a protocol, if supported. Use
    isProtocolSupported() to check if it is supported.

    \include qdnslookup.cpp nameserver-port
*/
QDnsLookup::QDnsLookup(Type type, const QString &name, Protocol protocol,
                       const QHostAddress &nameserver, quint16 port, QObject *parent)
    : QObject(*new QDnsLookupPrivate, parent)
{
    Q_D(QDnsLookup);
    d->name = name;
    d->type = type;
    d->nameserver = nameserver;
    d->port = port;
    d->protocol = protocol;
}

/*!
    Destroys the QDnsLookup object.

    It is safe to delete a QDnsLookup object even if it is not finished, you
    will simply never receive its results.
*/

QDnsLookup::~QDnsLookup()
{
}

/*!
    \since 6.8
    \property QDnsLookup::authenticData
    \brief whether the reply was authenticated by the resolver.

    QDnsLookup does not perform the authentication itself. Instead, it trusts
    the name server that was queried to perform the authentication and report
    it. The application is responsible for determining if any servers it
    configured with setNameserver() are trustworthy; if no server was set,
    QDnsLookup obeys system configuration on whether responses should be
    trusted.

    This property may be set even if error() indicates a resolver error
    occurred.

    \sa setNameserver(), nameserverProtocol()
*/
bool QDnsLookup::isAuthenticData() const
{
    return d_func()->reply.authenticData;
}

/*!
    \property QDnsLookup::error
    \brief the type of error that occurred if the DNS lookup failed, or NoError.
*/

QDnsLookup::Error QDnsLookup::error() const
{
    return d_func()->reply.error;
}

/*!
    \property QDnsLookup::errorString
    \brief a human-readable description of the error if the DNS lookup failed.
*/

QString QDnsLookup::errorString() const
{
    return d_func()->reply.errorString;
}

/*!
    Returns whether the reply has finished or was aborted.
*/

bool QDnsLookup::isFinished() const
{
    return d_func()->isFinished;
}

/*!
    \property QDnsLookup::name
    \brief the name to lookup.

    If the name to look up is empty, QDnsLookup will attempt to resolve the
    root domain of DNS. That query is usually performed with QDnsLookup::type
    set to \l{QDnsLookup::Type}{NS}.

    \note The name will be encoded using IDNA, which means it's unsuitable for
    querying SRV records compatible with the DNS-SD specification.
*/

QString QDnsLookup::name() const
{
    return d_func()->name;
}

void QDnsLookup::setName(const QString &name)
{
    Q_D(QDnsLookup);
    d->name = name;
}

QBindable<QString> QDnsLookup::bindableName()
{
    Q_D(QDnsLookup);
    return &d->name;
}

/*!
    \property QDnsLookup::type
    \brief the type of DNS lookup.
*/

QDnsLookup::Type QDnsLookup::type() const
{
    return d_func()->type;
}

void QDnsLookup::setType(Type type)
{
    Q_D(QDnsLookup);
    d->type = type;
}

QBindable<QDnsLookup::Type> QDnsLookup::bindableType()
{
    Q_D(QDnsLookup);
    return &d->type;
}

/*!
    \property QDnsLookup::nameserver
    \brief the nameserver to use for DNS lookup.
*/

QHostAddress QDnsLookup::nameserver() const
{
    return d_func()->nameserver;
}

void QDnsLookup::setNameserver(const QHostAddress &nameserver)
{
    Q_D(QDnsLookup);
    d->nameserver = nameserver;
}

QBindable<QHostAddress> QDnsLookup::bindableNameserver()
{
    Q_D(QDnsLookup);
    return &d->nameserver;
}

/*!
    \property QDnsLookup::nameserverPort
    \since 6.6
    \brief the port number of nameserver to use for DNS lookup.

    The value of 0 indicates that QDnsLookup should use the default port for
    the nameserverProtocol().

    \include qdnslookup.cpp nameserver-port
*/

quint16 QDnsLookup::nameserverPort() const
{
    return d_func()->port;
}

void QDnsLookup::setNameserverPort(quint16 nameserverPort)
{
    Q_D(QDnsLookup);
    d->port = nameserverPort;
}

QBindable<quint16> QDnsLookup::bindableNameserverPort()
{
    Q_D(QDnsLookup);
    return &d->port;
}

/*!
    \property QDnsLookup::nameserverProtocol
    \since 6.8
    \brief the protocol to use when sending the DNS query

    \sa isProtocolSupported()
*/
QDnsLookup::Protocol QDnsLookup::nameserverProtocol() const
{
    return d_func()->protocol;
}

void QDnsLookup::setNameserverProtocol(Protocol protocol)
{
    d_func()->protocol = protocol;
}

QBindable<QDnsLookup::Protocol> QDnsLookup::bindableNameserverProtocol()
{
    return &d_func()->protocol;
}

/*!
    \fn void QDnsLookup::setNameserver(const QHostAddress &nameserver, quint16 port)
    \since 6.6

    Sets the nameserver to \a nameserver and the port to \a port.

    \include qdnslookup.cpp nameserver-port

    \sa QDnsLookup::nameserver, QDnsLookup::nameserverPort
*/

void QDnsLookup::setNameserver(Protocol protocol, const QHostAddress &nameserver, quint16 port)
{
    Qt::beginPropertyUpdateGroup();
    setNameserver(nameserver);
    setNameserverPort(port);
    setNameserverProtocol(protocol);
    Qt::endPropertyUpdateGroup();
}

/*!
    Returns the list of canonical name records associated with this lookup.
*/

QList<QDnsDomainNameRecord> QDnsLookup::canonicalNameRecords() const
{
    return d_func()->reply.canonicalNameRecords;
}

/*!
    Returns the list of host address records associated with this lookup.
*/

QList<QDnsHostAddressRecord> QDnsLookup::hostAddressRecords() const
{
    return d_func()->reply.hostAddressRecords;
}

/*!
    Returns the list of mail exchange records associated with this lookup.

    The records are sorted according to
    \l{http://www.rfc-editor.org/rfc/rfc5321.txt}{RFC 5321}, so if you use them
    to connect to servers, you should try them in the order they are listed.
*/

QList<QDnsMailExchangeRecord> QDnsLookup::mailExchangeRecords() const
{
    return d_func()->reply.mailExchangeRecords;
}

/*!
    Returns the list of name server records associated with this lookup.
*/

QList<QDnsDomainNameRecord> QDnsLookup::nameServerRecords() const
{
    return d_func()->reply.nameServerRecords;
}

/*!
    Returns the list of pointer records associated with this lookup.
*/

QList<QDnsDomainNameRecord> QDnsLookup::pointerRecords() const
{
    return d_func()->reply.pointerRecords;
}

/*!
    Returns the list of service records associated with this lookup.

    The records are sorted according to
    \l{http://www.rfc-editor.org/rfc/rfc2782.txt}{RFC 2782}, so if you use them
    to connect to servers, you should try them in the order they are listed.
*/

QList<QDnsServiceRecord> QDnsLookup::serviceRecords() const
{
    return d_func()->reply.serviceRecords;
}

/*!
    Returns the list of text records associated with this lookup.
*/

QList<QDnsTextRecord> QDnsLookup::textRecords() const
{
    return d_func()->reply.textRecords;
}

/*!
    \since 6.8
    Returns the list of TLS association records associated with this lookup.

    According to the standards relating to DNS-based Authentication of Named
    Entities (DANE), this field should be ignored and must not be used for
    verifying the authentity of a given server if the authenticity of the DNS
    reply cannot itself be confirmed. See isAuthenticData() for more
    information.
 */
QList<QDnsTlsAssociationRecord> QDnsLookup::tlsAssociationRecords() const
{
    return d_func()->reply.tlsAssociationRecords;
}

#if QT_CONFIG(ssl)
/*!
    \since 6.8
    Sets the \a sslConfiguration to use for outgoing DNS-over-TLS connections.

    \sa sslConfiguration(), QSslSocket::setSslConfiguration()
*/
void QDnsLookup::setSslConfiguration(const QSslConfiguration &sslConfiguration)
{
    Q_D(QDnsLookup);
    d->sslConfiguration.emplace(sslConfiguration);
}

/*!
    Returns the current SSL configuration.

    \sa setSslConfiguration()
*/
QSslConfiguration QDnsLookup::sslConfiguration() const
{
    const Q_D(QDnsLookup);
    return d->sslConfiguration.value_or(QSslConfiguration::defaultConfiguration());
}
#endif

/*!
    Aborts the DNS lookup operation.

    If the lookup is already finished, does nothing.
*/

void QDnsLookup::abort()
{
    Q_D(QDnsLookup);
    if (d->runnable) {
        d->runnable = nullptr;
        d->reply = QDnsLookupReply();
        d->reply.error = QDnsLookup::OperationCancelledError;
        d->reply.errorString = tr("Operation cancelled");
        d->isFinished = true;
        emit finished();
    }
}

/*!
    Performs the DNS lookup.

    The \l{QDnsLookup::finished()}{finished()} signal is emitted upon completion.
*/

void QDnsLookup::lookup()
{
    Q_D(QDnsLookup);
    d->isFinished = false;
    d->reply = QDnsLookupReply();
    if (!QCoreApplication::instanceExists()) {
        // NOT qCWarning because this isn't a result of the lookup
        qWarning("QDnsLookup requires a QCoreApplication");
        return;
    }

    auto l = [this](const QDnsLookupReply &reply) {
        Q_D(QDnsLookup);
        if (d->runnable == sender()) {
#ifdef QDNSLOOKUP_DEBUG
            qDebug("DNS reply for %s: %i (%s)", qPrintable(d->name), reply.error, qPrintable(reply.errorString));
#endif
#if QT_CONFIG(ssl)
            d->sslConfiguration = std::move(reply.sslConfiguration);
#endif
            d->reply = reply;
            d->runnable = nullptr;
            d->isFinished = true;
            emit finished();
        }
    };

    d->runnable = new QDnsLookupRunnable(d);
    connect(d->runnable, &QDnsLookupRunnable::finished, this, l,
            Qt::BlockingQueuedConnection);
    theDnsLookupThreadPool->start(d->runnable);
}

/*!
    \class QDnsDomainNameRecord
    \brief The QDnsDomainNameRecord class stores information about a domain
    name record.

    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    When performing a name server lookup, zero or more records will be returned.
    Each record is represented by a QDnsDomainNameRecord instance.

    \sa QDnsLookup
*/

/*!
    Constructs an empty domain name record object.
*/

QDnsDomainNameRecord::QDnsDomainNameRecord()
    : d(new QDnsDomainNameRecordPrivate)
{
}

/*!
    Constructs a copy of \a other.
*/

QDnsDomainNameRecord::QDnsDomainNameRecord(const QDnsDomainNameRecord &other)
    : d(other.d)
{
}

/*!
    Destroys a domain name record.
*/

QDnsDomainNameRecord::~QDnsDomainNameRecord()
{
}

/*!
    Returns the name for this record.
*/

QString QDnsDomainNameRecord::name() const
{
    return d->name;
}

/*!
    Returns the duration in seconds for which this record is valid.
*/

quint32 QDnsDomainNameRecord::timeToLive() const
{
    return d->timeToLive;
}

/*!
    Returns the value for this domain name record.
*/

QString QDnsDomainNameRecord::value() const
{
    return d->value;
}

/*!
    Assigns the data of the \a other object to this record object,
    and returns a reference to it.
*/

QDnsDomainNameRecord &QDnsDomainNameRecord::operator=(const QDnsDomainNameRecord &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QDnsDomainNameRecord::swap(QDnsDomainNameRecord &other)
    \memberswap{domain-name record instance}
*/

/*!
    \class QDnsHostAddressRecord
    \brief The QDnsHostAddressRecord class stores information about a host
    address record.

    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    When performing an address lookup, zero or more records will be
    returned. Each record is represented by a QDnsHostAddressRecord instance.

    \sa QDnsLookup
*/

/*!
    Constructs an empty host address record object.
*/

QDnsHostAddressRecord::QDnsHostAddressRecord()
    : d(new QDnsHostAddressRecordPrivate)
{
}

/*!
    Constructs a copy of \a other.
*/

QDnsHostAddressRecord::QDnsHostAddressRecord(const QDnsHostAddressRecord &other)
    : d(other.d)
{
}

/*!
    Destroys a host address record.
*/

QDnsHostAddressRecord::~QDnsHostAddressRecord()
{
}

/*!
    Returns the name for this record.
*/

QString QDnsHostAddressRecord::name() const
{
    return d->name;
}

/*!
    Returns the duration in seconds for which this record is valid.
*/

quint32 QDnsHostAddressRecord::timeToLive() const
{
    return d->timeToLive;
}

/*!
    Returns the value for this host address record.
*/

QHostAddress QDnsHostAddressRecord::value() const
{
    return d->value;
}

/*!
    Assigns the data of the \a other object to this record object,
    and returns a reference to it.
*/

QDnsHostAddressRecord &QDnsHostAddressRecord::operator=(const QDnsHostAddressRecord &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QDnsHostAddressRecord::swap(QDnsHostAddressRecord &other)
    \memberswap{host address record instance}
*/

/*!
    \class QDnsMailExchangeRecord
    \brief The QDnsMailExchangeRecord class stores information about a DNS MX record.

    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    When performing a lookup on a service, zero or more records will be
    returned. Each record is represented by a QDnsMailExchangeRecord instance.

    The meaning of the fields is defined in
    \l{http://www.rfc-editor.org/rfc/rfc1035.txt}{RFC 1035}.

    \sa QDnsLookup
*/

/*!
    Constructs an empty mail exchange record object.
*/

QDnsMailExchangeRecord::QDnsMailExchangeRecord()
    : d(new QDnsMailExchangeRecordPrivate)
{
}

/*!
    Constructs a copy of \a other.
*/

QDnsMailExchangeRecord::QDnsMailExchangeRecord(const QDnsMailExchangeRecord &other)
    : d(other.d)
{
}

/*!
    Destroys a mail exchange record.
*/

QDnsMailExchangeRecord::~QDnsMailExchangeRecord()
{
}

/*!
    Returns the domain name of the mail exchange for this record.
*/

QString QDnsMailExchangeRecord::exchange() const
{
    return d->exchange;
}

/*!
    Returns the name for this record.
*/

QString QDnsMailExchangeRecord::name() const
{
    return d->name;
}

/*!
    Returns the preference for this record.
*/

quint16 QDnsMailExchangeRecord::preference() const
{
    return d->preference;
}

/*!
    Returns the duration in seconds for which this record is valid.
*/

quint32 QDnsMailExchangeRecord::timeToLive() const
{
    return d->timeToLive;
}

/*!
    Assigns the data of the \a other object to this record object,
    and returns a reference to it.
*/

QDnsMailExchangeRecord &QDnsMailExchangeRecord::operator=(const QDnsMailExchangeRecord &other)
{
    d = other.d;
    return *this;
}
/*!
    \fn void QDnsMailExchangeRecord::swap(QDnsMailExchangeRecord &other)
    \memberswap{mail exchange record}
*/

/*!
    \class QDnsServiceRecord
    \brief The QDnsServiceRecord class stores information about a DNS SRV record.

    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    When performing a lookup on a service, zero or more records will be
    returned. Each record is represented by a QDnsServiceRecord instance.

    The meaning of the fields is defined in
    \l{http://www.rfc-editor.org/rfc/rfc2782.txt}{RFC 2782}.

    \sa QDnsLookup
*/

/*!
    Constructs an empty service record object.
*/

QDnsServiceRecord::QDnsServiceRecord()
    : d(new QDnsServiceRecordPrivate)
{
}

/*!
    Constructs a copy of \a other.
*/

QDnsServiceRecord::QDnsServiceRecord(const QDnsServiceRecord &other)
    : d(other.d)
{
}

/*!
    Destroys a service record.
*/

QDnsServiceRecord::~QDnsServiceRecord()
{
}

/*!
    Returns the name for this record.
*/

QString QDnsServiceRecord::name() const
{
    return d->name;
}

/*!
    Returns the port on the target host for this service record.
*/

quint16 QDnsServiceRecord::port() const
{
    return d->port;
}

/*!
    Returns the priority for this service record.

    A client must attempt to contact the target host with the lowest-numbered
    priority.
*/

quint16 QDnsServiceRecord::priority() const
{
    return d->priority;
}

/*!
    Returns the domain name of the target host for this service record.
*/

QString QDnsServiceRecord::target() const
{
    return d->target;
}

/*!
    Returns the duration in seconds for which this record is valid.
*/

quint32 QDnsServiceRecord::timeToLive() const
{
    return d->timeToLive;
}

/*!
    Returns the weight for this service record.

    The weight field specifies a relative weight for entries with the same
    priority. Entries with higher weights should be selected with a higher
    probability.
*/

quint16 QDnsServiceRecord::weight() const
{
    return d->weight;
}

/*!
    Assigns the data of the \a other object to this record object,
    and returns a reference to it.
*/

QDnsServiceRecord &QDnsServiceRecord::operator=(const QDnsServiceRecord &other)
{
    d = other.d;
    return *this;
}
/*!
    \fn void QDnsServiceRecord::swap(QDnsServiceRecord &other)
    \memberswap{service record instance}
*/

/*!
    \class QDnsTextRecord
    \brief The QDnsTextRecord class stores information about a DNS TXT record.

    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    When performing a text lookup, zero or more records will be
    returned. Each record is represented by a QDnsTextRecord instance.

    The meaning of the fields is defined in
    \l{http://www.rfc-editor.org/rfc/rfc1035.txt}{RFC 1035}.

    \sa QDnsLookup
*/

/*!
    Constructs an empty text record object.
*/

QDnsTextRecord::QDnsTextRecord()
    : d(new QDnsTextRecordPrivate)
{
}

/*!
    Constructs a copy of \a other.
*/

QDnsTextRecord::QDnsTextRecord(const QDnsTextRecord &other)
    : d(other.d)
{
}

/*!
    Destroys a text record.
*/

QDnsTextRecord::~QDnsTextRecord()
{
}

/*!
    Returns the name for this text record.
*/

QString QDnsTextRecord::name() const
{
    return d->name;
}

/*!
    Returns the duration in seconds for which this record is valid.
*/

quint32 QDnsTextRecord::timeToLive() const
{
    return d->timeToLive;
}

/*!
    Returns the values for this text record.
*/

QList<QByteArray> QDnsTextRecord::values() const
{
    return d->values;
}

/*!
    Assigns the data of the \a other object to this record object,
    and returns a reference to it.
*/

QDnsTextRecord &QDnsTextRecord::operator=(const QDnsTextRecord &other)
{
    d = other.d;
    return *this;
}
/*!
    \fn void QDnsTextRecord::swap(QDnsTextRecord &other)
    \memberswap{text record instance}
*/

/*!
    \class QDnsTlsAssociationRecord
    \since 6.8
    \brief The QDnsTlsAssociationRecord class stores information about a DNS TLSA record.

    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    When performing a text lookup, zero or more records will be returned. Each
    record is represented by a QDnsTlsAssociationRecord instance.

    The meaning of the fields is defined in \l{RFC 6698}.

    \sa QDnsLookup
*/

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QDnsTlsAssociationRecordPrivate)

/*!
    \enum QDnsTlsAssociationRecord::CertificateUsage

    This enumeration contains valid values for the certificate usage field of
    TLS Association queries. The following list is up-to-date with \l{RFC 6698}
    section 2.1.1 and RFC 7218 section 2.1. Please refer to those documents for
    authoritative instructions on interpreting this enumeration.

    \value CertificateAuthorityConstrait
        Indicates the record includes an association to a specific Certificate
        Authority that must be found in the TLS server's certificate chain and
        must pass PKIX validation.

    \value ServiceCertificateConstraint
        Indicates the record includes an association to a certificate that must
        match the end entity certificate provided by the TLS server and must
        pass PKIX validation.

    \value TrustAnchorAssertion
        Indicates the record includes an association to a certificate that MUST
        be used as the ultimate trust anchor to validate the TLS server's
        certificate and must pass PKIX validation.

    \value DomainIssuedCertificate
        Indicates the record includes an association to a certificate that must
        match the end entity certificate provided by the TLS server. PKIX
        validation is not tested.

    \value PrivateUse
        No standard meaning applied.

    \value PKIX_TA
        Alias; mnemonic for Public Key Infrastructure Trust Anchor

    \value PKIX_EE
        Alias; mnemonic for Public Key Infrastructure End Entity

    \value DANE_TA
        Alias; mnemonic for DNS-based Authentication of Named Entities Trust Anchor

    \value DANE_EE
        Alias; mnemonic for DNS-based Authentication of Named Entities End Entity

    \value PrivCert
        Alias

    Other values are currently reserved, but may be unreserved by future
    standards. This enumeration can be used for those values even if no
    enumerator is provided.

    \sa usage()
*/

/*!
    \enum QDnsTlsAssociationRecord::Selector

    This enumeration contains valid values for the selector field of TLS
    Association queries. The following list is up-to-date with \l{RFC 6698}
    section 2.1.2 and RFC 7218 section 2.2. Please refer to those documents for
    authoritative instructions on interpreting this enumeration.

    \value FullCertificate
        Indicates this record refers to the full certificate in its binary
        structure form.

    \value SubjectPublicKeyInfo
        Indicates the record refers to the certificate's subject and public
        key information, in DER-encoded binary structure form.

    \value PrivateUse
        No standard meaning applied.

    \value Cert
        Alias

    \value SPKI
        Alias

    \value PrivSel
        Alias

    Other values are currently reserved, but may be unreserved by future
    standards. This enumeration can be used for those values even if no
    enumerator is provided.

    \sa selector()
*/

/*!
    \enum QDnsTlsAssociationRecord::MatchingType

    This enumeration contains valid values for the matching type field of TLS
    Association queries. The following list is up-to-date with \l{RFC 6698}
    section 2.1.3 and RFC 7218 section 2.3. Please refer to those documents for
    authoritative instructions on interpreting this enumeration.

    \value Exact
        Indicates this the certificate or SPKI data is stored verbatim in this
        record.

    \value Sha256
        Indicates this a SHA-256 checksum of the the certificate or SPKI data
        present in this record.

    \value Sha512
        Indicates this a SHA-512 checksum of the the certificate or SPKI data
        present in this record.

    \value PrivateUse
        No standard meaning applied.

    \value PrivMatch
        Alias

    Other values are currently reserved, but may be unreserved by future
    standards. This enumeration can be used for those values even if no
    enumerator is provided.

    \sa matchType()
*/

/*!
    Constructs an empty TLS Association record.
 */
QDnsTlsAssociationRecord::QDnsTlsAssociationRecord()
    : d(new QDnsTlsAssociationRecordPrivate)
{
}

/*!
    Constructs a copy of \a other.
 */
QDnsTlsAssociationRecord::QDnsTlsAssociationRecord(const QDnsTlsAssociationRecord &other) = default;

/*!
    Moves the content of \a other into this object.
 */
QDnsTlsAssociationRecord &
QDnsTlsAssociationRecord::operator=(const QDnsTlsAssociationRecord &other) = default;

/*!
    Destroys this TLS Association record object.
 */
QDnsTlsAssociationRecord::~QDnsTlsAssociationRecord() = default;

/*!
    Returns the name of this record.
*/
QString QDnsTlsAssociationRecord::name() const
{
    return d->name;
}

/*!
    Returns the duration in seconds for which this record is valid.
*/
quint32 QDnsTlsAssociationRecord::timeToLive() const
{
    return d->timeToLive;
}

/*!
    Returns the certificate usage field for this record.
 */
QDnsTlsAssociationRecord::CertificateUsage QDnsTlsAssociationRecord::usage() const
{
    return d->usage;
}

/*!
    Returns the selector field for this record.
 */
QDnsTlsAssociationRecord::Selector QDnsTlsAssociationRecord::selector() const
{
    return d->selector;
}

/*!
    Returns the match type field for this record.
 */
QDnsTlsAssociationRecord::MatchingType QDnsTlsAssociationRecord::matchType() const
{
    return d->matchType;
}

/*!
    Returns the binary data field for this record. The interpretation of this
    binary data depends on the three numeric fields provided by
    certificateUsage(), selector(), and matchType().

    Do note this is a binary field, even for the checksums, similar to what
    QCyrptographicHash::result() returns.
 */
QByteArray QDnsTlsAssociationRecord::value() const
{
    return d->value;
}

static QDnsLookupRunnable::EncodedLabel encodeLabel(const QString &label)
{
    QDnsLookupRunnable::EncodedLabel::value_type rootDomain = u'.';
    if (label.isEmpty())
        return QDnsLookupRunnable::EncodedLabel(1, rootDomain);

    QString encodedLabel = qt_ACE_do(label, ToAceOnly, ForbidLeadingDot);
#ifdef Q_OS_WIN
    return encodedLabel;
#else
    return std::move(encodedLabel).toLatin1();
#endif
}

inline QDnsLookupRunnable::QDnsLookupRunnable(const QDnsLookupPrivate *d)
    : requestName(encodeLabel(d->name)),
      nameserver(d->nameserver),
      requestType(d->type),
      port(d->port),
      protocol(d->protocol)
{
    if (port == 0)
        port = QDnsLookup::defaultPortForProtocol(protocol);
#if QT_CONFIG(ssl)
    sslConfiguration = d->sslConfiguration;
#endif
}

void QDnsLookupRunnable::run()
{
    QDnsLookupReply reply;

    // Validate input.
    if (qsizetype n = requestName.size(); n > MaxDomainNameLength || n == 0) {
        reply.error = QDnsLookup::InvalidRequestError;
        reply.errorString = QDnsLookup::tr("Invalid domain name");
    } else {
        // Perform request.
        query(&reply);

        // Sort results.
        qt_qdnsmailexchangerecord_sort(reply.mailExchangeRecords);
        qt_qdnsservicerecord_sort(reply.serviceRecords);
    }

    emit finished(reply);

    // maybe print the lookup error as warning
    switch (reply.error) {
    case QDnsLookup::NoError:
    case QDnsLookup::OperationCancelledError:
    case QDnsLookup::NotFoundError:
    case QDnsLookup::ServerFailureError:
    case QDnsLookup::ServerRefusedError:
    case QDnsLookup::TimeoutError:
        break;      // no warning for these

    case QDnsLookup::ResolverError:
    case QDnsLookup::InvalidRequestError:
    case QDnsLookup::InvalidReplyError:
        qCWarning(lcDnsLookup()).nospace()
                << "DNS lookup failed (" << reply.error << "): "
              << qUtf16Printable(reply.errorString)
              << "; request was " << this;  // continues below
    }
}

inline QDebug operator<<(QDebug &d, QDnsLookupRunnable *r)
{
    // continued: print the information about the request
    d << r->requestName.left(MaxDomainNameLength);
    if (r->requestName.size() > MaxDomainNameLength)
        d << "... (truncated)";
    d << " type " << r->requestType;
    if (!r->nameserver.isNull()) {
        d << " to nameserver " << qUtf16Printable(r->nameserver.toString())
          << " port " << (r->port ? r->port : QDnsLookup::defaultPortForProtocol(r->protocol));
        switch (r->protocol) {
        case QDnsLookup::Standard:
            break;
        case QDnsLookup::DnsOverTls:
            d << " (TLS)";
        }
    }
    return d;
}

#if QT_CONFIG(ssl)
static constexpr std::chrono::milliseconds DnsOverTlsConnectTimeout(15'000);
static constexpr std::chrono::milliseconds DnsOverTlsTimeout(120'000);
static constexpr quint8 DnsAuthenticDataBit = 0x20;

static int makeReplyErrorFromSocket(QDnsLookupReply *reply, const QAbstractSocket *socket)
{
    QDnsLookup::Error error = [&] {
        switch (socket->error()) {
        case QAbstractSocket::SocketTimeoutError:
        case QAbstractSocket::ProxyConnectionTimeoutError:
            return QDnsLookup::TimeoutError;
        default:
            return QDnsLookup::ResolverError;
        }
    }();
    reply->setError(error, socket->errorString());
    return false;
}

bool QDnsLookupRunnable::sendDnsOverTls(QDnsLookupReply *reply, QSpan<unsigned char> query,
                                        ReplyBuffer &response)
{
    QSslSocket socket;
    socket.setSslConfiguration(sslConfiguration.value_or(QSslConfiguration::defaultConfiguration()));

#  if QT_CONFIG(networkproxy)
    socket.setProtocolTag("domain-s"_L1);
#  endif

    // Request the name server attempt to authenticate the reply.
    query[3] |= DnsAuthenticDataBit;

    do {
        quint16 size = qToBigEndian<quint16>(query.size());
        QDeadlineTimer timeout(DnsOverTlsTimeout);

        socket.connectToHostEncrypted(nameserver.toString(), port);
        socket.write(reinterpret_cast<const char *>(&size), sizeof(size));
        socket.write(reinterpret_cast<const char *>(query.data()), query.size());
        if (!socket.waitForEncrypted(DnsOverTlsConnectTimeout.count()))
            break;

        reply->sslConfiguration = socket.sslConfiguration();

        // accumulate reply
        auto waitForBytes = [&](void *buffer, int count) {
            int remaining = timeout.remainingTime();
            while (remaining >= 0 && socket.bytesAvailable() < count) {
                if (!socket.waitForReadyRead(remaining))
                    return false;
            }
            return socket.read(static_cast<char *>(buffer), count) == count;
        };
        if (!waitForBytes(&size, sizeof(size)))
            break;

        // note: strictly speaking, we're allocating memory based on untrusted data
        // but in practice, due to limited range of the data type (16 bits),
        // the maximum allocation is small.
        size = qFromBigEndian(size);
        response.resize(size);
        if (waitForBytes(response.data(), size)) {
            // check if the AD bit is set; we'll trust it over TLS requests
            if (size >= 4)
                reply->authenticData = response[3] & DnsAuthenticDataBit;
            return true;
        }
    } while (false);

    // handle errors
    return makeReplyErrorFromSocket(reply, &socket);
}
#else
bool QDnsLookupRunnable::sendDnsOverTls(QDnsLookupReply *reply, QSpan<unsigned char> query,
                                        ReplyBuffer &response)
{
    Q_UNUSED(query)
    Q_UNUSED(response)
    reply->setError(QDnsLookup::ResolverError, QDnsLookup::tr("SSL/TLS support not present"));
    return false;
}
#endif

QT_END_NAMESPACE

#include "moc_qdnslookup.cpp"
#include "moc_qdnslookup_p.cpp"
