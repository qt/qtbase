// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//#define QHOSTINFO_DEBUG

#include "qhostinfo.h"
#include "qhostinfo_p.h"
#include <qplatformdefs.h>

#include "QtCore/qapplicationstatic.h"
#include "QtCore/qscopedpointer.h"
#include <qabstracteventdispatcher.h>
#include <qcoreapplication.h>
#include <qmetaobject.h>
#include <qscopeguard.h>
#include <qstringlist.h>
#include <qthread.h>
#include <qurl.h>

#include <algorithm>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  if defined(AI_ADDRCONFIG) && !defined(Q_OS_WASM)
#    define Q_ADDRCONFIG          AI_ADDRCONFIG
#  endif
#elif defined Q_OS_WIN
#  include <ws2tcpip.h>

#  define QT_SOCKLEN_T int
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

//#define QHOSTINFO_DEBUG

QT_IMPL_METATYPE_EXTERN(QHostInfo)

namespace {
struct ToBeLookedUpEquals {
    typedef bool result_type;
    explicit ToBeLookedUpEquals(const QString &toBeLookedUp) noexcept : m_toBeLookedUp(toBeLookedUp) {}
    result_type operator()(QHostInfoRunnable* lookup) const noexcept
    {
        return m_toBeLookedUp == lookup->toBeLookedUp;
    }
private:
    QString m_toBeLookedUp;
};

template <typename InputIt, typename OutputIt1, typename OutputIt2, typename UnaryPredicate>
std::pair<OutputIt1, OutputIt2> separate_if(InputIt first, InputIt last, OutputIt1 dest1, OutputIt2 dest2, UnaryPredicate p)
{
    while (first != last) {
        if (p(*first)) {
            *dest1 = *first;
            ++dest1;
        } else {
            *dest2 = *first;
            ++dest2;
        }
        ++first;
    }
    return std::make_pair(dest1, dest2);
}

Q_APPLICATION_STATIC(QHostInfoLookupManager, theHostInfoLookupManager)

}

QHostInfoResult::QHostInfoResult(const QObject *receiver, QtPrivate::SlotObjUniquePtr slot)
    : receiver{receiver}, slotObj{std::move(slot)}, withContextObject{slotObj && receiver}
{
    if (receiver)
        moveToThread(receiver->thread());
}

QHostInfoResult::~QHostInfoResult()
    = default;

/*
    The calling thread is likely the one that executes the lookup via
    QHostInfoRunnable. Unless we operate with a queued connection already,
    posts the QHostInfo to a dedicated QHostInfoResult object that lives in
    the same thread as the user-provided receiver, or (if there is none) in
    the thread that made the call to lookupHost. That QHostInfoResult object
    then calls the user code in the correct thread.

    The 'result' object deletes itself (via deleteLater) when the metacall
    event is received.
*/
void QHostInfoResult::postResultsReady(const QHostInfo &info)
{
    // queued connection will take care of dispatching to right thread
    if (!slotObj) {
        emit resultsReady(info);
        return;
    }
    // we used to have a context object, but it's already destroyed
    if (withContextObject && !receiver)
        return;

    static const int signal_index = []() -> int {
        auto senderMetaObject = &QHostInfoResult::staticMetaObject;
        auto signal = &QHostInfoResult::resultsReady;
        int signal_index = -1;
        void *args[] = { &signal_index, &signal };
        senderMetaObject->static_metacall(QMetaObject::IndexOfMethod, 0, args);
        return signal_index + QMetaObjectPrivate::signalOffset(senderMetaObject);
    }();

    // a long-living version of this
    auto result = new QHostInfoResult(this);
    Q_CHECK_PTR(result);

    auto metaCallEvent = QMetaCallEvent::create(std::move(slotObj), nullptr, signal_index, info);
    Q_CHECK_PTR(metaCallEvent);
    qApp->postEvent(result, metaCallEvent);
}

/*
    Receives the event posted by postResultsReady, and calls the functor.
*/
bool QHostInfoResult::event(QEvent *event)
{
    if (event->type() == QEvent::MetaCall) {
        Q_ASSERT(slotObj);
        auto metaCallEvent = static_cast<QMetaCallEvent *>(event);
        auto args = metaCallEvent->args();
        // we didn't have a context object, or it's still alive
        if (!withContextObject || receiver)
            slotObj->call(const_cast<QObject*>(receiver.data()), args);

        deleteLater();
        return true;
    }
    return QObject::event(event);
}

/*!
    \class QHostInfo
    \brief The QHostInfo class provides static functions for host name lookups.

    \reentrant
    \inmodule QtNetwork
    \ingroup network

    QHostInfo finds the IP address(es) associated with a host name,
    or the host name associated with an IP address.
    The class provides two static convenience functions: one that
    works asynchronously and emits a signal once the host is found,
    and one that blocks and returns a QHostInfo object.

    To look up a host's IP addresses asynchronously, call lookupHost(),
    which takes the host name or IP address, a receiver object, and a slot
    signature as arguments and returns an ID. You can abort the
    lookup by calling abortHostLookup() with the lookup ID.

    Example:

    \snippet code/src_network_kernel_qhostinfo.cpp 0


    The slot is invoked when the results are ready. The results are
    stored in a QHostInfo object. Call
    addresses() to get the list of IP addresses for the host, and
    hostName() to get the host name that was looked up.

    If the lookup failed, error() returns the type of error that
    occurred. errorString() gives a human-readable description of the
    lookup error.

    If you want a blocking lookup, use the QHostInfo::fromName() function:

    \snippet code/src_network_kernel_qhostinfo.cpp 1

    QHostInfo supports Internationalized Domain Names (IDNs) through the
    IDNA and Punycode standards.

    To retrieve the name of the local host, use the static
    QHostInfo::localHostName() function.

    QHostInfo uses the mechanisms provided by the operating system
    to perform the lookup. As per \l {RFC 6724}
    there is no guarantee that all IP addresses registered for a domain or
    host will be returned.

    \note Since Qt 4.6.1 QHostInfo is using multiple threads for DNS lookup
    instead of one dedicated DNS thread. This improves performance,
    but also changes the order of signal emissions when using lookupHost()
    compared to previous versions of Qt.
    \note Since Qt 4.6.3 QHostInfo is using a small internal 60 second DNS cache
    for performance improvements.

    \sa QAbstractSocket, {RFC 3492}, {RFC 6724}
*/

static int nextId()
{
    Q_CONSTINIT static QBasicAtomicInt counter = Q_BASIC_ATOMIC_INITIALIZER(0);
    return 1 + counter.fetchAndAddRelaxed(1);
}

/*!
    Looks up the IP address(es) associated with host name \a name, and
    returns an ID for the lookup. When the result of the lookup is
    ready, the slot or signal \a member in \a receiver is called with
    a QHostInfo argument. The QHostInfo object can then be inspected
    to get the results of the lookup.

    The lookup is performed by a single function call, for example:

    \snippet code/src_network_kernel_qhostinfo.cpp 2

    The implementation of the slot prints basic information about the
    addresses returned by the lookup, or reports an error if it failed:

    \snippet code/src_network_kernel_qhostinfo.cpp 3

    If you pass a literal IP address to \a name instead of a host name,
    QHostInfo will search for the domain name for the IP (i.e., QHostInfo will
    perform a \e reverse lookup). On success, the resulting QHostInfo will
    contain both the resolved domain name and IP addresses for the host
    name. Example:

    \snippet code/src_network_kernel_qhostinfo.cpp 4

    \note There is no guarantee on the order the signals will be emitted
    if you start multiple requests with lookupHost().

    \sa abortHostLookup(), addresses(), error(), fromName()
*/
int QHostInfo::lookupHost(const QString &name, QObject *receiver, const char *member)
{
    return QHostInfo::lookupHostImpl(name, receiver, nullptr, member);
}

/*!
    \fn QHostInfo &QHostInfo::operator=(QHostInfo &&other)

    Move-assigns \a other to this QHostInfo instance.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 5.10
*/

/*!
    \fn void QHostInfo::swap(QHostInfo &other)

    Swaps host-info \a other with this host-info. This operation is
    very fast and never fails.

    \since 5.10
*/

/*!
    \fn template<typename Functor> int QHostInfo::lookupHost(const QString &name, Functor &&functor)

    \since 5.9

    \overload

    Looks up the IP address(es) associated with host name \a name, and
    returns an ID for the lookup. When the result of the lookup is
    ready, the \a functor is called with a QHostInfo argument. The
    QHostInfo object can then be inspected to get the results of the
    lookup.

    The \a functor will be run in the thread that makes the call to lookupHost;
    that thread must have a running Qt event loop.

    \note There is no guarantee on the order the signals will be emitted
    if you start multiple requests with lookupHost().

    \sa abortHostLookup(), addresses(), error(), fromName()
*/

/*!
    \fn template<typename Functor> int QHostInfo::lookupHost(const QString &name, const QObject *context, Functor functor)

    \since 5.9

    \overload

    Looks up the IP address(es) associated with host name \a name, and
    returns an ID for the lookup. When the result of the lookup is
    ready, the \a functor is called with a QHostInfo argument. The
    QHostInfo object can then be inspected to get the results of the
    lookup.

    If \a context is destroyed before the lookup completes, the
    \a functor will not be called. The \a functor will be run in the
    thread of \a context. The context's thread must have a running Qt
    event loop.

    Here is an alternative signature for the function:
    \code
    lookupHost(const QString &name, const QObject *receiver, PointerToMemberFunction function)
    \endcode

    In this case, when the result of the lookup is ready, the slot or
    signal \c{function} in \c{receiver} is called with a QHostInfo
    argument. The QHostInfo object can then be inspected to get the
    results of the lookup.

    \note There is no guarantee on the order the signals will be emitted
    if you start multiple requests with lookupHost().

    \sa abortHostLookup(), addresses(), error(), fromName()
*/

/*!
    Aborts the host lookup with the ID \a id, as returned by lookupHost().

    \sa lookupHost(), lookupId()
*/
void QHostInfo::abortHostLookup(int id)
{
    theHostInfoLookupManager()->abortLookup(id);
}

/*!
    Looks up the IP address(es) for the given host \a name. The
    function blocks during the lookup which means that execution of
    the program is suspended until the results of the lookup are
    ready. Returns the result of the lookup in a QHostInfo object.

    If you pass a literal IP address to \a name instead of a host name,
    QHostInfo will search for the domain name for the IP (i.e., QHostInfo will
    perform a \e reverse lookup). On success, the returned QHostInfo will
    contain both the resolved domain name and IP addresses for the host name.

    \sa lookupHost()
*/
QHostInfo QHostInfo::fromName(const QString &name)
{
#if defined QHOSTINFO_DEBUG
    qDebug("QHostInfo::fromName(\"%s\")",name.toLatin1().constData());
#endif

#ifdef Q_OS_WASM
    return QHostInfoAgent::lookup(name);
#else
    QHostInfo hostInfo = QHostInfoAgent::fromName(name);
    QHostInfoLookupManager* manager = theHostInfoLookupManager();
    manager->cache.put(name, hostInfo);
    return hostInfo;
#endif
}


QHostInfo QHostInfoAgent::reverseLookup(const QHostAddress &address)
{
    QHostInfo results;
    // Reverse lookup
    sockaddr_in sa4;
    sockaddr_in6 sa6;
    sockaddr *sa = nullptr;
    QT_SOCKLEN_T saSize;
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        sa = reinterpret_cast<sockaddr *>(&sa4);
        saSize = sizeof(sa4);
        memset(&sa4, 0, sizeof(sa4));
        sa4.sin_family = AF_INET;
        sa4.sin_addr.s_addr = htonl(address.toIPv4Address());
    } else {
        sa = reinterpret_cast<sockaddr *>(&sa6);
        saSize = sizeof(sa6);
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        memcpy(&sa6.sin6_addr, address.toIPv6Address().c, sizeof(sa6.sin6_addr));
    }

    char hbuf[NI_MAXHOST];
    if (sa && getnameinfo(sa, saSize, hbuf, sizeof(hbuf), nullptr, 0, 0) == 0)
        results.setHostName(QString::fromLatin1(hbuf));

    if (results.hostName().isEmpty())
        results.setHostName(address.toString());
    results.setAddresses(QList<QHostAddress>() << address);

    return results;
}

/*
    Call getaddrinfo, and returns the results as QHostInfo::addresses
*/
QHostInfo QHostInfoAgent::lookup(const QString &hostName)
{
    QHostInfo results;

    // IDN support
    QByteArray aceHostname = QUrl::toAce(hostName);
    results.setHostName(hostName);
    if (aceHostname.isEmpty()) {
        results.setError(QHostInfo::HostNotFound);
        results.setErrorString(hostName.isEmpty() ?
                               QCoreApplication::translate("QHostInfoAgent", "No host name given") :
                               QCoreApplication::translate("QHostInfoAgent", "Invalid hostname"));
        return results;
    }

    addrinfo *res = nullptr;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
#ifdef Q_ADDRCONFIG
    hints.ai_flags = Q_ADDRCONFIG;
#endif

    int result = getaddrinfo(aceHostname.constData(), nullptr, &hints, &res);
# ifdef Q_ADDRCONFIG
    if (result == EAI_BADFLAGS) {
        // if the lookup failed with AI_ADDRCONFIG set, try again without it
        hints.ai_flags = 0;
        result = getaddrinfo(aceHostname.constData(), nullptr, &hints, &res);
    }
# endif

    if (result == 0) {
        addrinfo *node = res;
        QList<QHostAddress> addresses;
        while (node) {
#ifdef QHOSTINFO_DEBUG
            qDebug() << "getaddrinfo node: flags:" << node->ai_flags << "family:" << node->ai_family
                     << "ai_socktype:" << node->ai_socktype << "ai_protocol:" << node->ai_protocol
                     << "ai_addrlen:" << node->ai_addrlen;
#endif
            switch (node->ai_family) {
            case AF_INET: {
                QHostAddress addr;
                addr.setAddress(ntohl(((sockaddr_in *) node->ai_addr)->sin_addr.s_addr));
                if (!addresses.contains(addr))
                    addresses.append(addr);
                break;
            }
            case AF_INET6: {
                QHostAddress addr;
                sockaddr_in6 *sa6 = (sockaddr_in6 *) node->ai_addr;
                addr.setAddress(sa6->sin6_addr.s6_addr);
                if (sa6->sin6_scope_id)
                    addr.setScopeId(QString::number(sa6->sin6_scope_id));
                if (!addresses.contains(addr))
                    addresses.append(addr);
                break;
            }
            default:
                results.setError(QHostInfo::UnknownError);
                results.setErrorString(QCoreApplication::translate("QHostInfoAgent", "Unknown address type"));
            }
            node = node->ai_next;
        }
        if (addresses.isEmpty()) {
            // Reached the end of the list, but no addresses were found; this
            // means the list contains one or more unknown address types.
            results.setError(QHostInfo::UnknownError);
            results.setErrorString(QCoreApplication::translate("QHostInfoAgent", "Unknown address type"));
        }

        results.setAddresses(addresses);
        freeaddrinfo(res);
    } else {
        switch (result) {
#ifdef Q_OS_WIN
        case WSAHOST_NOT_FOUND: //authoritative not found
        case WSATRY_AGAIN: //non authoritative not found
        case WSANO_DATA: //valid name, no associated address
#else
        case EAI_NONAME:
        case EAI_FAIL:
#  ifdef EAI_NODATA // EAI_NODATA is deprecated in RFC 3493
        case EAI_NODATA:
#  endif
#endif
            results.setError(QHostInfo::HostNotFound);
            results.setErrorString(QCoreApplication::translate("QHostInfoAgent", "Host not found"));
            break;
        default:
            results.setError(QHostInfo::UnknownError);
#ifdef Q_OS_WIN
            results.setErrorString(QString::fromWCharArray(gai_strerror(result)));
#else
            results.setErrorString(QString::fromLocal8Bit(gai_strerror(result)));
#endif
            break;
        }
    }

#if defined(QHOSTINFO_DEBUG)
    if (results.error() != QHostInfo::NoError) {
        qDebug("QHostInfoAgent::fromName(): error #%d %s",
               h_errno, results.errorString().toLatin1().constData());
    } else {
        QString tmp;
        QList<QHostAddress> addresses = results.addresses();
        for (int i = 0; i < addresses.count(); ++i) {
            if (i != 0) tmp += ", "_L1;
            tmp += addresses.at(i).toString();
        }
        qDebug("QHostInfoAgent::fromName(): found %i entries for \"%s\": {%s}",
               addresses.count(), aceHostname.constData(),
               tmp.toLatin1().constData());
    }
#endif

    return results;
}

/*!
    \enum QHostInfo::HostInfoError

    This enum describes the various errors that can occur when trying
    to resolve a host name.

    \value NoError The lookup was successful.
    \value HostNotFound No IP addresses were found for the host.
    \value UnknownError An unknown error occurred.

    \sa error(), setError()
*/

/*!
    Constructs an empty host info object with lookup ID \a id.

    \sa lookupId()
*/
QHostInfo::QHostInfo(int id)
    : d_ptr(new QHostInfoPrivate)
{
    Q_D(QHostInfo);
    d->lookupId = id;
}

/*!
    Constructs a copy of \a other.
*/
QHostInfo::QHostInfo(const QHostInfo &other)
    : d_ptr(new QHostInfoPrivate(*other.d_ptr))
{
}

/*!
    \fn QHostInfo::QHostInfo(QHostInfo &&other)

    Move-constructs a new QHostInfo from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 5.14
*/

/*!
    Assigns the data of the \a other object to this host info object,
    and returns a reference to it.
*/
QHostInfo &QHostInfo::operator=(const QHostInfo &other)
{
    if (this == &other)
      return *this;

    Q_ASSERT(d_ptr && other.d_ptr);
    *d_ptr = *other.d_ptr;
    return *this;
}

/*!
    Destroys the host info object.
*/
QHostInfo::~QHostInfo()
{
    delete d_ptr;
}

/*!
    Returns the list of IP addresses associated with hostName(). This
    list may be empty.

    Example:

    \snippet code/src_network_kernel_qhostinfo.cpp 5

    \sa hostName(), error()
*/
QList<QHostAddress> QHostInfo::addresses() const
{
    Q_D(const QHostInfo);
    return d->addrs;
}

/*!
    Sets the list of addresses in this QHostInfo to \a addresses.

    \sa addresses()
*/
void QHostInfo::setAddresses(const QList<QHostAddress> &addresses)
{
    Q_D(QHostInfo);
    d->addrs = addresses;
}

/*!
    Returns the name of the host whose IP addresses were looked up.

    \sa localHostName()
*/
QString QHostInfo::hostName() const
{
    Q_D(const QHostInfo);
    return d->hostName;
}

/*!
    Sets the host name of this QHostInfo to \a hostName.

    \sa hostName()
*/
void QHostInfo::setHostName(const QString &hostName)
{
    Q_D(QHostInfo);
    d->hostName = hostName;
}

/*!
    Returns the type of error that occurred if the host name lookup
    failed; otherwise returns NoError.

    \sa setError(), errorString()
*/
QHostInfo::HostInfoError QHostInfo::error() const
{
    Q_D(const QHostInfo);
    return d->err;
}

/*!
    Sets the error type of this QHostInfo to \a error.

    \sa error(), errorString()
*/
void QHostInfo::setError(HostInfoError error)
{
    Q_D(QHostInfo);
    d->err = error;
}

/*!
    Returns the ID of this lookup.

    \sa setLookupId(), abortHostLookup(), hostName()
*/
int QHostInfo::lookupId() const
{
    Q_D(const QHostInfo);
    return d->lookupId;
}

/*!
    Sets the ID of this lookup to \a id.

    \sa lookupId(), lookupHost()
*/
void QHostInfo::setLookupId(int id)
{
    Q_D(QHostInfo);
    d->lookupId = id;
}

/*!
    If the lookup failed, this function returns a human readable
    description of the error; otherwise "Unknown error" is returned.

    \sa setErrorString(), error()
*/
QString QHostInfo::errorString() const
{
    Q_D(const QHostInfo);
    return d->errorStr;
}

/*!
    Sets the human readable description of the error that occurred to \a str
    if the lookup failed.

    \sa errorString(), setError()
*/
void QHostInfo::setErrorString(const QString &str)
{
    Q_D(QHostInfo);
    d->errorStr = str;
}

/*!
    \fn QString QHostInfo::localHostName()

    Returns this machine's host name, if one is configured. Note that hostnames
    are not guaranteed to be globally unique, especially if they were
    configured automatically.

    This function does not guarantee the returned host name is a Fully
    Qualified Domain Name (FQDN). For that, use fromName() to resolve the
    returned name to an FQDN.

    This function returns the same as QSysInfo::machineHostName().

    \sa hostName(), localDomainName()
*/
QString QHostInfo::localHostName()
{
    return QSysInfo::machineHostName();
}

/*!
    \fn QString QHostInfo::localDomainName()

    Returns the DNS domain of this machine.

    \note DNS domains are not related to domain names found in
    Windows networks.

    \sa hostName()
*/

/*!
    \internal
    Called by the various lookupHost overloads to perform the lookup.

    Signals either the functor encapuslated in the \a slotObjRaw in the context
    of \a receiver, or the \a member slot of the \a receiver.

    \a receiver might be the nullptr, but only if a \a slotObjRaw is provided.
*/
int QHostInfo::lookupHostImpl(const QString &name,
                              const QObject *receiver,
                              QtPrivate::QSlotObjectBase *slotObjRaw,
                              const char *member)
{
    QtPrivate::SlotObjUniquePtr slotObj{slotObjRaw};
#if defined QHOSTINFO_DEBUG
    qDebug("QHostInfo::lookupHostImpl(\"%s\", %p, %p, %s)",
           name.toLatin1().constData(), receiver, slotObj.get(), member ? member + 1 : 0);
#endif
    Q_ASSERT(!member != !slotObj); // one of these must be set, but not both
    Q_ASSERT(receiver || slotObj);

    if (!QAbstractEventDispatcher::instance(QThread::currentThread())) {
        qWarning("QHostInfo::lookupHost() called with no event dispatcher");
        return -1;
    }

    qRegisterMetaType<QHostInfo>();

    int id = nextId(); // generate unique ID

    if (Q_UNLIKELY(name.isEmpty())) {
        QHostInfo hostInfo(id);
        hostInfo.setError(QHostInfo::HostNotFound);
        hostInfo.setErrorString(QCoreApplication::translate("QHostInfo", "No host name given"));

        QHostInfoResult result(receiver, std::move(slotObj));
        if (receiver && member)
            QObject::connect(&result, SIGNAL(resultsReady(QHostInfo)),
                            receiver, member, Qt::QueuedConnection);
        result.postResultsReady(hostInfo);

        return id;
    }

#ifdef Q_OS_WASM
    // Resolve the host name directly without using a thread or cache,
    // since Emscripten's host lookup is fast. Emscripten maintains an internal
    // mapping of hosts and addresses for the purposes of WebSocket socket
    // tunnelling, and does not perform an actual host lookup.
    QHostInfo hostInfo = QHostInfoAgent::lookup(name);
    hostInfo.setLookupId(id);

    QHostInfoResult result(receiver, std::move(slotObj));
    if (receiver && member)
        QObject::connect(&result, SIGNAL(resultsReady(QHostInfo)),
                        receiver, member, Qt::QueuedConnection);
    result.postResultsReady(hostInfo);
#else
    QHostInfoLookupManager *manager = theHostInfoLookupManager();

    if (Q_LIKELY(manager)) {
        // the application is still alive
        if (manager->cache.isEnabled()) {
            // check cache first
            bool valid = false;
            QHostInfo info = manager->cache.get(name, &valid);
            if (valid) {
                info.setLookupId(id);
                QHostInfoResult result(receiver, std::move(slotObj));
                if (receiver && member)
                    QObject::connect(&result, SIGNAL(resultsReady(QHostInfo)),
                                    receiver, member, Qt::QueuedConnection);
                result.postResultsReady(info);
                return id;
            }
        }

        // cache is not enabled or it was not in the cache, do normal lookup
        QHostInfoRunnable *runnable = new QHostInfoRunnable(name, id, receiver, std::move(slotObj));
        if (receiver && member)
            QObject::connect(&runnable->resultEmitter, SIGNAL(resultsReady(QHostInfo)),
                                receiver, member, Qt::QueuedConnection);
        manager->scheduleLookup(runnable);
    }
#endif // Q_OS_WASM
    return id;
}

QHostInfoRunnable::QHostInfoRunnable(const QString &hn, int i, const QObject *receiver,
                                     QtPrivate::SlotObjUniquePtr slotObj)
    : toBeLookedUp{hn}, id{i}, resultEmitter{receiver, std::move(slotObj)}
{
    setAutoDelete(true);
}

QHostInfoRunnable::~QHostInfoRunnable()
    = default;

// the QHostInfoLookupManager will at some point call this via a QThreadPool
void QHostInfoRunnable::run()
{
    QHostInfoLookupManager *manager = theHostInfoLookupManager();
    const auto sg = qScopeGuard([&] { manager->lookupFinished(this); });
    // check aborted
    if (manager->wasAborted(id))
        return;

    QHostInfo hostInfo;

    // QHostInfo::lookupHost already checks the cache. However we need to check
    // it here too because it might have been cache saved by another QHostInfoRunnable
    // in the meanwhile while this QHostInfoRunnable was scheduled but not running
    if (manager->cache.isEnabled()) {
        // check the cache first
        bool valid = false;
        hostInfo = manager->cache.get(toBeLookedUp, &valid);
        if (!valid) {
            // not in cache, we need to do the lookup and store the result in the cache
            hostInfo = QHostInfoAgent::fromName(toBeLookedUp);
            manager->cache.put(toBeLookedUp, hostInfo);
        }
    } else {
        // cache is not enabled, just do the lookup and continue
        hostInfo = QHostInfoAgent::fromName(toBeLookedUp);
    }

    // check aborted again
    if (manager->wasAborted(id))
        return;

    // signal emission
    hostInfo.setLookupId(id);
    resultEmitter.postResultsReady(hostInfo);

#if QT_CONFIG(thread)
    // now also iterate through the postponed ones
    {
        QMutexLocker locker(&manager->mutex);
        const auto partitionBegin = std::stable_partition(manager->postponedLookups.rbegin(), manager->postponedLookups.rend(),
                                                          ToBeLookedUpEquals(toBeLookedUp)).base();
        const auto partitionEnd = manager->postponedLookups.end();
        for (auto it = partitionBegin; it != partitionEnd; ++it) {
            QHostInfoRunnable* postponed = *it;
            // we can now emit
            hostInfo.setLookupId(postponed->id);
            postponed->resultEmitter.postResultsReady(hostInfo);
            delete postponed;
        }
        manager->postponedLookups.erase(partitionBegin, partitionEnd);
    }

#endif
    // thread goes back to QThreadPool
}

QHostInfoLookupManager::QHostInfoLookupManager() : wasDeleted(false)
{
#if QT_CONFIG(thread)
    QObject::connect(QCoreApplication::instance(), &QObject::destroyed,
                     &threadPool, [&](QObject *) { threadPool.waitForDone(); },
                     Qt::DirectConnection);
    threadPool.setMaxThreadCount(20); // do up to 20 DNS lookups in parallel
#endif
}

QHostInfoLookupManager::~QHostInfoLookupManager()
{
    QMutexLocker locker(&mutex);
    wasDeleted = true;
    locker.unlock();

    // don't qDeleteAll currentLookups, the QThreadPool has ownership
    clear();
}

void QHostInfoLookupManager::clear()
{
    {
        QMutexLocker locker(&mutex);
        qDeleteAll(scheduledLookups);
        qDeleteAll(finishedLookups);
#if QT_CONFIG(thread)
        qDeleteAll(postponedLookups);
        postponedLookups.clear();
#endif
        scheduledLookups.clear();
        finishedLookups.clear();
    }

#if QT_CONFIG(thread)
    threadPool.waitForDone();
#endif
    cache.clear();
}

// assumes mutex is locked by caller
void QHostInfoLookupManager::rescheduleWithMutexHeld()
{
    if (wasDeleted)
        return;

    // goals of this function:
    //  - launch new lookups via the thread pool
    //  - make sure only one lookup per host/IP is in progress

    if (!finishedLookups.isEmpty()) {
        // remove ID from aborted if it is in there
        for (int i = 0; i < finishedLookups.size(); i++) {
           abortedLookups.removeAll(finishedLookups.at(i)->id);
        }

        finishedLookups.clear();
    }

#if QT_CONFIG(thread)
    auto isAlreadyRunning = [this](QHostInfoRunnable *lookup) {
        return std::any_of(currentLookups.cbegin(), currentLookups.cend(), ToBeLookedUpEquals(lookup->toBeLookedUp));
    };

    // Transfer any postponed lookups that aren't currently running to the scheduled list, keeping already-running lookups:
    postponedLookups.erase(separate_if(postponedLookups.begin(),
                                       postponedLookups.end(),
                                       postponedLookups.begin(),
                                       std::front_inserter(scheduledLookups), // prepend! we want to finish it ASAP
                                       isAlreadyRunning).first,
                           postponedLookups.end());

    // Unschedule and postpone any that are currently running:
    scheduledLookups.erase(separate_if(scheduledLookups.begin(),
                                       scheduledLookups.end(),
                                       std::back_inserter(postponedLookups),
                                       scheduledLookups.begin(),
                                       isAlreadyRunning).second,
                           scheduledLookups.end());

    const int availableThreads = threadPool.maxThreadCount() - currentLookups.size();
    if (availableThreads > 0) {
        int readyToStartCount = qMin(availableThreads, scheduledLookups.size());
        auto it = scheduledLookups.begin();
        while (readyToStartCount--) {
            // runnable now running in new thread, track this in currentLookups
            threadPool.start(*it);
            currentLookups.push_back(std::move(*it));
            ++it;
        }
        scheduledLookups.erase(scheduledLookups.begin(), it);
    }
#else
    if (!scheduledLookups.isEmpty())
        scheduledLookups.takeFirst()->run();
#endif
}

// called by QHostInfo
void QHostInfoLookupManager::scheduleLookup(QHostInfoRunnable *r)
{
    QMutexLocker locker(&this->mutex);

    if (wasDeleted)
        return;

    scheduledLookups.enqueue(r);
    rescheduleWithMutexHeld();
}

// called by QHostInfo
void QHostInfoLookupManager::abortLookup(int id)
{
    QMutexLocker locker(&this->mutex);

    if (wasDeleted)
        return;

#if QT_CONFIG(thread)
    // is postponed? delete and return
    for (int i = 0; i < postponedLookups.size(); i++) {
        if (postponedLookups.at(i)->id == id) {
            delete postponedLookups.takeAt(i);
            return;
        }
    }
#endif

    // is scheduled? delete and return
    for (int i = 0; i < scheduledLookups.size(); i++) {
        if (scheduledLookups.at(i)->id == id) {
            delete scheduledLookups.takeAt(i);
            return;
        }
    }

    if (!abortedLookups.contains(id))
        abortedLookups.append(id);
}

// called from QHostInfoRunnable
bool QHostInfoLookupManager::wasAborted(int id)
{
    QMutexLocker locker(&this->mutex);

    if (wasDeleted)
        return true;

    return abortedLookups.contains(id);
}

// called from QHostInfoRunnable
void QHostInfoLookupManager::lookupFinished(QHostInfoRunnable *r)
{
    QMutexLocker locker(&this->mutex);

    if (wasDeleted)
        return;

#if QT_CONFIG(thread)
    currentLookups.removeOne(r);
#endif
    finishedLookups.append(r);
    rescheduleWithMutexHeld();
}

// This function returns immediately when we had a result in the cache, else it will later emit a signal
QHostInfo qt_qhostinfo_lookup(const QString &name, QObject *receiver, const char *member, bool *valid, int *id)
{
    *valid = false;
    *id = -1;

    // check cache
    QHostInfoLookupManager* manager = theHostInfoLookupManager();
    if (manager && manager->cache.isEnabled()) {
        QHostInfo info = manager->cache.get(name, valid);
        if (*valid) {
            return info;
        }
    }

    // was not in cache, trigger lookup
    *id = QHostInfo::lookupHostImpl(name, receiver, nullptr, member);

    // return empty response, valid==false
    return QHostInfo();
}

void qt_qhostinfo_clear_cache()
{
    QHostInfoLookupManager* manager = theHostInfoLookupManager();
    if (manager) {
        manager->clear();
    }
}

#ifdef QT_BUILD_INTERNAL
void Q_AUTOTEST_EXPORT qt_qhostinfo_enable_cache(bool e)
{
    QHostInfoLookupManager* manager = theHostInfoLookupManager();
    if (manager) {
        manager->cache.setEnabled(e);
    }
}

void qt_qhostinfo_cache_inject(const QString &hostname, const QHostInfo &resolution)
{
    QHostInfoLookupManager* manager = theHostInfoLookupManager();
    if (!manager || !manager->cache.isEnabled())
        return;

    manager->cache.put(hostname, resolution);
}
#endif

// cache for 60 seconds
// cache 128 items
QHostInfoCache::QHostInfoCache() : max_age(60), enabled(true), cache(128)
{
#ifdef QT_QHOSTINFO_CACHE_DISABLED_BY_DEFAULT
    enabled.store(false, std::memory_order_relaxed);
#endif
}

QHostInfo QHostInfoCache::get(const QString &name, bool *valid)
{
    QMutexLocker locker(&this->mutex);

    *valid = false;
    if (QHostInfoCacheElement *element = cache.object(name)) {
        if (element->age.elapsed() < max_age*1000)
            *valid = true;
        return element->info;

        // FIXME idea:
        // if too old but not expired, trigger a new lookup
        // to freshen our cache
    }

    return QHostInfo();
}

void QHostInfoCache::put(const QString &name, const QHostInfo &info)
{
    // if the lookup failed, don't cache
    if (info.error() != QHostInfo::NoError)
        return;

    QHostInfoCacheElement* element = new QHostInfoCacheElement();
    element->info = info;
    element->age = QElapsedTimer();
    element->age.start();

    QMutexLocker locker(&this->mutex);
    cache.insert(name, element); // cache will take ownership
}

void QHostInfoCache::clear()
{
    QMutexLocker locker(&this->mutex);
    cache.clear();
}

QT_END_NAMESPACE

#include "moc_qhostinfo_p.cpp"
