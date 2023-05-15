// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdnslookup.h"
#include "qdnslookup_p.h"

#include <qapplicationstatic.h>
#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qloggingcategory.h>
#include <qrandom.h>
#include <qurl.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(lcDnsLookup, "qt.network.dnslookup", QtCriticalMsg)

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

    \value TXT      text records.
*/

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
    \fn void QDnsLookup::typeChanged(Type type)

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
    : QDnsLookup(type, name, nameserver, DnsPort, parent)
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
    limitations and firewalls. Notably, the Windows API used by QDnsLookup is
    unable to handle alternate port numbers.
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
    Destroys the QDnsLookup object.

    It is safe to delete a QDnsLookup object even if it is not finished, you
    will simply never receive its results.
*/

QDnsLookup::~QDnsLookup()
{
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
    \since 6.6
    Sets the nameserver to \a nameserver and the port to \a port.

    \include qdnslookup.cpp nameserver-port

    \sa QDnsLookup::nameserver, QDnsLookup::nameserverPort
*/
void QDnsLookup::setNameserver(const QHostAddress &nameserver, quint16 port)
{
    Qt::beginPropertyUpdateGroup();
    setNameserver(nameserver);
    setNameserverPort(port);
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
    if (!QCoreApplication::instance()) {
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

    Swaps this domain-name record instance with \a other. This
    function is very fast and never fails.
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

    Swaps this host address record instance with \a other. This
    function is very fast and never fails.
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

    Swaps this mail exchange record with \a other. This function is
    very fast and never fails.
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

    Swaps this service record instance with \a other. This function is
    very fast and never fails.
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

    Swaps this text record instance with \a other. This function is
    very fast and never fails.
*/

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
      port(d->port)
{
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
    if (!r->nameserver.isNull())
        d << " to nameserver " << qUtf16Printable(r->nameserver.toString())
          << " port " << (r->port ? r->port : DnsPort);
    return d;
}

QT_END_NAMESPACE

#include "moc_qdnslookup.cpp"
#include "moc_qdnslookup_p.cpp"
