/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qhostaddress.h"
#include "qhostaddress_p.h"
#include "private/qipaddress_p.h"
#include "qdebug.h"
#if defined(Q_OS_WIN)
# include <winsock2.h>
#else
# include <netinet/in.h>
#endif
#include "qplatformdefs.h"
#include "qstringlist.h"
#include "qendian.h"
#ifndef QT_NO_DATASTREAM
#include <qdatastream.h>
#endif
#ifdef __SSE2__
#  include <private/qsimd_p.h>
#endif

#ifdef QT_LINUXBASE
#  include <arpa/inet.h>
#endif

QT_BEGIN_NAMESPACE

#define QT_ENSURE_PARSED(a) \
    do { \
        if (!(a)->d->isParsed) \
            (a)->d->parse(); \
    } while (0)

#ifdef Q_OS_WIN
// sockaddr_in6 size changed between old and new SDK
// Only the new version is the correct one, so always
// use this structure.
#if defined(Q_OS_WINRT)
#  if !defined(u_char)
#    define u_char unsigned char
#  endif
#  if !defined(u_short)
#    define u_short unsigned short
#  endif
#  if !defined(u_long)
#    define u_long unsigned long
#  endif
#endif
struct qt_in6_addr {
    u_char qt_s6_addr[16];
};
typedef struct {
    short   sin6_family;            /* AF_INET6 */
    u_short sin6_port;              /* Transport level port number */
    u_long  sin6_flowinfo;          /* IPv6 flow information */
    struct  qt_in6_addr sin6_addr;  /* IPv6 address */
    u_long  sin6_scope_id;          /* set of interfaces for a scope */
} qt_sockaddr_in6;
#else
#define qt_sockaddr_in6 sockaddr_in6
#define qt_s6_addr s6_addr
#endif


class QHostAddressPrivate
{
public:
    QHostAddressPrivate();

    void setAddress(quint32 a_ = 0);
    void setAddress(const quint8 *a_);
    void setAddress(const Q_IPV6ADDR &a_);

    bool parse();
    void clear();

    QString ipString;
    QString scopeId;

    union {
        Q_IPV6ADDR a6; // IPv6 address
        struct { quint64 c[2]; } a6_64;
        struct { quint32 c[4]; } a6_32;
    };
    quint32 a;    // IPv4 address
    qint8 protocol;
    bool isParsed;

    friend class QHostAddress;
};

QHostAddressPrivate::QHostAddressPrivate()
    : a(0), protocol(QAbstractSocket::UnknownNetworkLayerProtocol), isParsed(true)
{
    memset(&a6, 0, sizeof(a6));
}

void QHostAddressPrivate::setAddress(quint32 a_)
{
    a = a_;
    protocol = QAbstractSocket::IPv4Protocol;
    isParsed = true;

    //create mapped address, except for a_ == 0 (any)
    a6_64.c[0] = 0;
    if (a) {
        a6_32.c[2] = qToBigEndian(0xffff);
        a6_32.c[3] = qToBigEndian(a);
    } else {
        a6_64.c[1] = 0;
    }
}

/// parses v4-mapped addresses or the AnyIPv6 address and stores in \a a;
/// returns true if the address was one of those
static bool convertToIpv4(quint32& a, const Q_IPV6ADDR &a6, const QHostAddress::ConversionMode mode)
{
    if (mode == QHostAddress::StrictConversion)
        return false;

    const uchar *ptr = a6.c;
    if (qFromUnaligned<quint64>(ptr) != 0)
        return false;

    const quint32 mid = qFromBigEndian<quint32>(ptr + 8);
    if ((mid == 0xffff) && (mode & QHostAddress::ConvertV4MappedToIPv4)) {
        a = qFromBigEndian<quint32>(ptr + 12);
        return true;
    }
    if (mid != 0)
        return false;

    const quint32 low = qFromBigEndian<quint32>(ptr + 12);
    if ((low == 0) && (mode & QHostAddress::ConvertUnspecifiedAddress)) {
        a = 0;
        return true;
    }
    if ((low == 1) && (mode & QHostAddress::ConvertLocalHost)) {
        a = INADDR_LOOPBACK;
        return true;
    }
    if ((low != 1) && (mode & QHostAddress::ConvertV4CompatToIPv4)) {
        a = low;
        return true;
    }
    return false;
}

void QHostAddressPrivate::setAddress(const quint8 *a_)
{
    protocol = QAbstractSocket::IPv6Protocol;
    isParsed = true;
    memcpy(a6.c, a_, sizeof(a6));
    a = 0;
    convertToIpv4(a, a6, (QHostAddress::ConvertV4MappedToIPv4
                          | QHostAddress::ConvertUnspecifiedAddress));
}

void QHostAddressPrivate::setAddress(const Q_IPV6ADDR &a_)
{
    setAddress(a_.c);
}

static bool parseIp6(const QString &address, QIPAddressUtils::IPv6Address &addr, QString *scopeId)
{
    QString tmp = address;
    int scopeIdPos = tmp.lastIndexOf(QLatin1Char('%'));
    if (scopeIdPos != -1) {
        *scopeId = tmp.mid(scopeIdPos + 1);
        tmp.chop(tmp.size() - scopeIdPos);
    } else {
        scopeId->clear();
    }
    return QIPAddressUtils::parseIp6(addr, tmp.constBegin(), tmp.constEnd()) == 0;
}

Q_NEVER_INLINE bool QHostAddressPrivate::parse()
{
    isParsed = true;
    protocol = QAbstractSocket::UnknownNetworkLayerProtocol;
    QString a = ipString.simplified();
    if (a.isEmpty())
        return false;

    // All IPv6 addresses contain a ':', and may contain a '.'.
    if (a.contains(QLatin1Char(':'))) {
        quint8 maybeIp6[16];
        if (parseIp6(a, maybeIp6, &scopeId)) {
            setAddress(maybeIp6);
            return true;
        }
    }

    quint32 maybeIp4 = 0;
    if (QIPAddressUtils::parseIp4(maybeIp4, a.constBegin(), a.constEnd())) {
        setAddress(maybeIp4);
        return true;
    }

    return false;
}

void QHostAddressPrivate::clear()
{
    a = 0;
    protocol = QAbstractSocket::UnknownNetworkLayerProtocol;
    isParsed = true;
    memset(&a6, 0, sizeof(a6));
}


bool QNetmaskAddress::setAddress(const QString &address)
{
    length = -1;
    QHostAddress other;
    return other.setAddress(address) && setAddress(other);
}

bool QNetmaskAddress::setAddress(const QHostAddress &address)
{
    static const quint8 zeroes[16] = { 0 };
    union {
        quint32 v4;
        quint8 v6[16];
    } ip;

    int netmask = 0;
    quint8 *ptr = ip.v6;
    quint8 *end;
    length = -1;

    QHostAddress::operator=(address);

    if (d->protocol == QAbstractSocket::IPv4Protocol) {
        ip.v4 = qToBigEndian(d->a);
        end = ptr + 4;
    } else if (d->protocol == QAbstractSocket::IPv6Protocol) {
        memcpy(ip.v6, d->a6.c, 16);
        end = ptr + 16;
    } else {
        d->clear();
        return false;
    }

    while (ptr < end) {
        switch (*ptr) {
        case 255:
            netmask += 8;
            ++ptr;
            continue;

        default:
            d->clear();
            return false;       // invalid IP-style netmask

            // the rest always falls through
        case 254:
            ++netmask;
        case 252:
            ++netmask;
        case 248:
            ++netmask;
        case 240:
            ++netmask;
        case 224:
            ++netmask;
        case 192:
            ++netmask;
        case 128:
            ++netmask;
        case 0:
            break;
        }
        break;
    }

    // confirm that the rest is only zeroes
    if (ptr < end && memcmp(ptr + 1, zeroes, end - ptr - 1) != 0) {
        d->clear();
        return false;
    }

    length = netmask;
    return true;
}

static void clearBits(quint8 *where, int start, int end)
{
    Q_ASSERT(end == 32 || end == 128);
    if (start == end)
        return;

    // for the byte where 'start' is, clear the lower bits only
    quint8 bytemask = 256 - (1 << (8 - (start & 7)));
    where[start / 8] &= bytemask;

    // for the tail part, clear everything
    memset(where + (start + 7) / 8, 0, end / 8 - (start + 7) / 8);
}

int QNetmaskAddress::prefixLength() const
{
    return length;
}

void QNetmaskAddress::setPrefixLength(QAbstractSocket::NetworkLayerProtocol proto, int newLength)
{
    length = newLength;
    if (length < 0 || length > (proto == QAbstractSocket::IPv4Protocol ? 32 :
                                proto == QAbstractSocket::IPv6Protocol ? 128 : -1)) {
        // invalid information, reject
        d->protocol = QAbstractSocket::UnknownNetworkLayerProtocol;
        length = -1;
        return;
    }

    d->protocol = proto;
    if (d->protocol == QAbstractSocket::IPv4Protocol) {
        if (length == 0) {
            d->a = 0;
        } else if (length == 32) {
            d->a = quint32(0xffffffff);
        } else {
            d->a = quint32(0xffffffff) >> (32 - length) << (32 - length);
        }
    } else {
        memset(d->a6.c, 0xFF, sizeof(d->a6));
        clearBits(d->a6.c, length, 128);
    }
}

/*!
    \class QHostAddress
    \brief The QHostAddress class provides an IP address.
    \ingroup network
    \inmodule QtNetwork

    This class holds an IPv4 or IPv6 address in a platform- and
    protocol-independent manner.

    QHostAddress is normally used with the QTcpSocket, QTcpServer,
    and QUdpSocket to connect to a host or to set up a server.

    A host address is set with setAddress(), and retrieved with
    toIPv4Address(), toIPv6Address(), or toString(). You can check the
    type with protocol().

    \note Please note that QHostAddress does not do DNS lookups.
    QHostInfo is needed for that.

    The class also supports common predefined addresses: \l Null, \l
    LocalHost, \l LocalHostIPv6, \l Broadcast, and \l Any.

    \sa QHostInfo, QTcpSocket, QTcpServer, QUdpSocket
*/

/*! \enum QHostAddress::SpecialAddress

    \value Null The null address object. Equivalent to QHostAddress().
    \value LocalHost The IPv4 localhost address. Equivalent to QHostAddress("127.0.0.1").
    \value LocalHostIPv6 The IPv6 localhost address. Equivalent to QHostAddress("::1").
    \value Broadcast The IPv4 broadcast address. Equivalent to QHostAddress("255.255.255.255").
    \value AnyIPv4 The IPv4 any-address. Equivalent to QHostAddress("0.0.0.0"). A socket bound with this address will listen only on IPv4 interaces.
    \value AnyIPv6 The IPv6 any-address. Equivalent to QHostAddress("::"). A socket bound with this address will listen only on IPv6 interaces.
    \value Any The dual stack any-address. A socket bound with this address will listen on both IPv4 and IPv6 interfaces.
*/

/*! \enum QHostAddress::ConversionModeFlag

    \since 5.8

    \value StrictConversion Don't convert IPv6 addresses to IPv4 when comparing two QHostAddress objects of different protocols, so they will always be considered different.
    \value ConvertV4MappedToIPv4 Convert IPv4-mapped IPv6 addresses (RFC 4291 sect. 2.5.5.2) when comparing. Therefore QHostAddress("::ffff:192.168.1.1") will compare equal to QHostAddress("192.168.1.1").
    \value ConvertV4CompatToIPv4 Convert IPv4-compatible IPv6 addresses (RFC 4291 sect. 2.5.5.1) when comparing. Therefore QHostAddress("::192.168.1.1") will compare equal to QHostAddress("192.168.1.1").
    \value ConvertLocalHost Convert the IPv6 loopback addresses to its IPv4 equivalent when comparing. Therefore e.g. QHostAddress("::1") will compare equal to QHostAddress("127.0.0.1").
    \value ConvertUnspecifiedAddress All unspecified addresses will compare equal, namely AnyIPv4, AnyIPv6 and Any.
    \value TolerantConversion Sets all three preceding flags.

    \sa isEqual()
 */

/*!  Constructs a null host address object, i.e. an address which is not valid for any host or interface.

    \sa clear()
*/
QHostAddress::QHostAddress()
    : d(new QHostAddressPrivate)
{
}

/*!
    Constructs a host address object with the IPv4 address \a ip4Addr.
*/
QHostAddress::QHostAddress(quint32 ip4Addr)
    : d(new QHostAddressPrivate)
{
    setAddress(ip4Addr);
}

/*!
    Constructs a host address object with the IPv6 address \a ip6Addr.

    \a ip6Addr must be a 16-byte array in network byte order (big
    endian).
*/
QHostAddress::QHostAddress(quint8 *ip6Addr)
    : d(new QHostAddressPrivate)
{
    setAddress(ip6Addr);
}

/*!
    \since 5.5
    Constructs a host address object with the IPv6 address \a ip6Addr.

    \a ip6Addr must be a 16-byte array in network byte order (big
    endian).
*/
QHostAddress::QHostAddress(const quint8 *ip6Addr)
    : d(new QHostAddressPrivate)
{
    setAddress(ip6Addr);
}

/*!
    Constructs a host address object with the IPv6 address \a ip6Addr.
*/
QHostAddress::QHostAddress(const Q_IPV6ADDR &ip6Addr)
    : d(new QHostAddressPrivate)
{
    setAddress(ip6Addr);
}

/*!
    Constructs an IPv4 or IPv6 address based on the string \a address
    (e.g., "127.0.0.1").

    \sa setAddress()
*/
QHostAddress::QHostAddress(const QString &address)
    : d(new QHostAddressPrivate)
{
    d->ipString = address;
    d->isParsed = false;
}

/*!
    \fn QHostAddress::QHostAddress(const sockaddr *sockaddr)

    Constructs an IPv4 or IPv6 address using the address specified by
    the native structure \a sockaddr.

    \sa setAddress()
*/
QHostAddress::QHostAddress(const struct sockaddr *sockaddr)
    : d(new QHostAddressPrivate)
{
#ifndef Q_OS_WINRT
    if (sockaddr->sa_family == AF_INET)
        setAddress(htonl(((const sockaddr_in *)sockaddr)->sin_addr.s_addr));
    else if (sockaddr->sa_family == AF_INET6)
        setAddress(((const qt_sockaddr_in6 *)sockaddr)->sin6_addr.qt_s6_addr);
#else
    Q_UNUSED(sockaddr)
#endif
}

/*!
    Constructs a copy of the given \a address.
*/
QHostAddress::QHostAddress(const QHostAddress &address)
    : d(new QHostAddressPrivate(*address.d.data()))
{
}

/*!
    Constructs a QHostAddress object for \a address.
*/
QHostAddress::QHostAddress(SpecialAddress address)
    : d(new QHostAddressPrivate)
{
    setAddress(address);
}

/*!
    \overload
    \since 5.8

    Sets the special address specified by \a address.
*/
void QHostAddress::setAddress(SpecialAddress address)
{
    d->clear();

    Q_IPV6ADDR ip6;
    memset(&ip6, 0, sizeof ip6);
    quint32 ip4 = INADDR_ANY;

    switch (address) {
    case Null:
        return;

    case Broadcast:
        ip4 = INADDR_BROADCAST;
        break;
    case LocalHost:
        ip4 = INADDR_LOOPBACK;
        break;
    case AnyIPv4:
        break;

    case LocalHostIPv6:
        ip6[15] = 1;
        Q_FALLTHROUGH();
    case AnyIPv6:
        d->setAddress(ip6);
        return;

    case Any:
        d->protocol = QAbstractSocket::AnyIPProtocol;
        return;
    }

    // common IPv4 part
    d->setAddress(ip4);
}

/*!
    Destroys the host address object.
*/
QHostAddress::~QHostAddress()
{
}

/*!
    Assigns another host \a address to this object, and returns a reference
    to this object.
*/
QHostAddress &QHostAddress::operator=(const QHostAddress &address)
{
    *d.data() = *address.d.data();
    return *this;
}

#if QT_DEPRECATED_SINCE(5, 8)
/*!
    Assigns the host address \a address to this object, and returns a
    reference to this object.

    \sa setAddress()
*/
QHostAddress &QHostAddress::operator=(const QString &address)
{
    setAddress(address);
    return *this;
}
#endif

/*!
    \since 5.8
    Assigns the special address \a address to this object, and returns a
    reference to this object.

    \sa setAddress()
*/
QHostAddress &QHostAddress::operator=(SpecialAddress address)
{
    setAddress(address);
    return *this;
}

/*!
    \fn bool QHostAddress::operator!=(const QHostAddress &other) const
    \since 4.2

    Returns \c true if this host address is not the same as the \a other
    address given; otherwise returns \c false.
*/

/*!
    \fn bool QHostAddress::operator!=(SpecialAddress other) const

    Returns \c true if this host address is not the same as the \a other
    address given; otherwise returns \c false.
*/

/*!
    Sets the host address to 0.0.0.0.
*/
void QHostAddress::clear()
{
    d->clear();
}

/*!
    Set the IPv4 address specified by \a ip4Addr.
*/
void QHostAddress::setAddress(quint32 ip4Addr)
{
    d->setAddress(ip4Addr);
}

/*!
    \overload

    Set the IPv6 address specified by \a ip6Addr.

    \a ip6Addr must be an array of 16 bytes in network byte order
    (high-order byte first).
*/
void QHostAddress::setAddress(quint8 *ip6Addr)
{
    d->setAddress(ip6Addr);
}

/*!
    \overload
    \since 5.5

    Set the IPv6 address specified by \a ip6Addr.

    \a ip6Addr must be an array of 16 bytes in network byte order
    (high-order byte first).
*/
void QHostAddress::setAddress(const quint8 *ip6Addr)
{
    d->setAddress(ip6Addr);
}

/*!
    \overload

    Set the IPv6 address specified by \a ip6Addr.
*/
void QHostAddress::setAddress(const Q_IPV6ADDR &ip6Addr)
{
    d->setAddress(ip6Addr);
}

/*!
    \overload

    Sets the IPv4 or IPv6 address specified by the string
    representation specified by \a address (e.g. "127.0.0.1").
    Returns \c true and sets the address if the address was successfully
    parsed; otherwise returns \c false.
*/
bool QHostAddress::setAddress(const QString &address)
{
    d->ipString = address;
    return d->parse();
}

/*!
    \fn void QHostAddress::setAddress(const sockaddr *sockaddr)
    \overload

    Sets the IPv4 or IPv6 address specified by the native structure \a
    sockaddr.  Returns \c true and sets the address if the address was
    successfully parsed; otherwise returns \c false.
*/
void QHostAddress::setAddress(const struct sockaddr *sockaddr)
{
#ifndef Q_OS_WINRT
    clear();
    if (sockaddr->sa_family == AF_INET)
        setAddress(htonl(((const sockaddr_in *)sockaddr)->sin_addr.s_addr));
    else if (sockaddr->sa_family == AF_INET6)
        setAddress(((const qt_sockaddr_in6 *)sockaddr)->sin6_addr.qt_s6_addr);
#else
    Q_UNUSED(sockaddr)
#endif
}

/*!
    Returns the IPv4 address as a number.

    For example, if the address is 127.0.0.1, the returned value is
    2130706433 (i.e. 0x7f000001).

    This value is valid if the protocol() is
    \l{QAbstractSocket::}{IPv4Protocol},
    or if the protocol is
    \l{QAbstractSocket::}{IPv6Protocol},
    and the IPv6 address is an IPv4 mapped address. (RFC4291)

    \sa toString()
*/
quint32 QHostAddress::toIPv4Address() const
{
    return toIPv4Address(Q_NULLPTR);
}

/*!
    Returns the IPv4 address as a number.

    For example, if the address is 127.0.0.1, the returned value is
    2130706433 (i.e. 0x7f000001).

    This value is valid if the protocol() is
    \l{QAbstractSocket::}{IPv4Protocol},
    or if the protocol is
    \l{QAbstractSocket::}{IPv6Protocol},
    and the IPv6 address is an IPv4 mapped address. (RFC4291). In those
    cases, \a ok will be set to true. Otherwise, it will be set to false.

    \sa toString()
*/
quint32 QHostAddress::toIPv4Address(bool *ok) const
{
    QT_ENSURE_PARSED(this);
    quint32 dummy;
    if (ok)
        *ok = d->protocol == QAbstractSocket::IPv4Protocol || d->protocol == QAbstractSocket::AnyIPProtocol
              || (d->protocol == QAbstractSocket::IPv6Protocol
                  && convertToIpv4(dummy, d->a6, ConversionMode(QHostAddress::ConvertV4MappedToIPv4
                                                                | QHostAddress::ConvertUnspecifiedAddress)));
    return d->a;
}

/*!
    Returns the network layer protocol of the host address.
*/
QAbstractSocket::NetworkLayerProtocol QHostAddress::protocol() const
{
    QT_ENSURE_PARSED(this);
    return QAbstractSocket::NetworkLayerProtocol(d->protocol);
}

/*!
    Returns the IPv6 address as a Q_IPV6ADDR structure. The structure
    consists of 16 unsigned characters.

    \snippet code/src_network_kernel_qhostaddress.cpp 0

    This value is valid if the protocol() is
    \l{QAbstractSocket::}{IPv6Protocol}.
    If the protocol is
    \l{QAbstractSocket::}{IPv4Protocol},
    then the address is returned an an IPv4 mapped IPv6 address. (RFC4291)

    \sa toString()
*/
Q_IPV6ADDR QHostAddress::toIPv6Address() const
{
    QT_ENSURE_PARSED(this);
    return d->a6;
}

/*!
    Returns the address as a string.

    For example, if the address is the IPv4 address 127.0.0.1, the
    returned string is "127.0.0.1". For IPv6 the string format will
    follow the RFC5952 recommendation.
    For QHostAddress::Any, its IPv4 address will be returned ("0.0.0.0")

    \sa toIPv4Address()
*/
QString QHostAddress::toString() const
{
    QT_ENSURE_PARSED(this);
    QString s;
    if (d->protocol == QAbstractSocket::IPv4Protocol
        || d->protocol == QAbstractSocket::AnyIPProtocol) {
        quint32 i = toIPv4Address();
        QIPAddressUtils::toString(s, i);
    } else if (d->protocol == QAbstractSocket::IPv6Protocol) {
        QIPAddressUtils::toString(s, d->a6.c);
        if (!d->scopeId.isEmpty())
            s.append(QLatin1Char('%') + d->scopeId);
    }
    return s;
}

/*!
    \since 4.1

    Returns the scope ID of an IPv6 address. For IPv4 addresses, or if the
    address does not contain a scope ID, an empty QString is returned.

    The IPv6 scope ID specifies the scope of \e reachability for non-global
    IPv6 addresses, limiting the area in which the address can be used. All
    IPv6 addresses are associated with such a reachability scope. The scope ID
    is used to disambiguate addresses that are not guaranteed to be globally
    unique.

    IPv6 specifies the following four levels of reachability:

    \list

    \li Node-local: Addresses that are only used for communicating with
    services on the same interface (e.g., the loopback interface "::1").

    \li Link-local: Addresses that are local to the network interface
    (\e{link}). There is always one link-local address for each IPv6 interface
    on your host. Link-local addresses ("fe80...") are generated from the MAC
    address of the local network adaptor, and are not guaranteed to be unique.

    \li Global: For globally routable addresses, such as public servers on the
    Internet.

    \endlist

    When using a link-local or site-local address for IPv6 connections, you
    must specify the scope ID. The scope ID for a link-local address is
    usually the same as the interface name (e.g., "eth0", "en1") or number
    (e.g., "1", "2").

    \sa setScopeId(), QNetworkInterface, QNetworkInterface::interfaceFromName
*/
QString QHostAddress::scopeId() const
{
    QT_ENSURE_PARSED(this);
    return (d->protocol == QAbstractSocket::IPv6Protocol) ? d->scopeId : QString();
}

/*!
    \since 4.1

    Sets the IPv6 scope ID of the address to \a id. If the address protocol is
    not IPv6, this function does nothing. The scope ID may be set as an
    interface name (such as "eth0" or "en1") or as an integer representing the
    interface index. If \a id is an interface name, QtNetwork will convert to
    an interface index using QNetworkInterface::interfaceIndexFromName() before
    calling the operating system networking functions.

    \sa scopeId(), QNetworkInterface, QNetworkInterface::interfaceFromName
*/
void QHostAddress::setScopeId(const QString &id)
{
    QT_ENSURE_PARSED(this);
    if (d->protocol == QAbstractSocket::IPv6Protocol)
        d->scopeId = id;
}

/*!
    Returns \c true if this host address is the same as the \a other address
    given; otherwise returns \c false. This operator just calls isEqual(other, StrictConversion).

    \sa isEqual()
*/
bool QHostAddress::operator==(const QHostAddress &other) const
{
    return isEqual(other, StrictConversion);
}

/*!
    \since 5.8

    Returns \c true if this host address is the same as the \a other address
    given; otherwise returns \c false.

    The parameter \a mode controls which conversions are preformed between addresses
    of differing protocols. If no \a mode is given, \c TolerantConversion is performed
    by default.

    \sa ConversionMode, operator==()
 */
bool QHostAddress::isEqual(const QHostAddress &other, ConversionMode mode) const
{
    QT_ENSURE_PARSED(this);
    QT_ENSURE_PARSED(&other);

    if (d->protocol == QAbstractSocket::IPv4Protocol) {
        switch (other.d->protocol) {
        case QAbstractSocket::IPv4Protocol:
            return d->a == other.d->a;
        case QAbstractSocket::IPv6Protocol:
            quint32 a4;
            return convertToIpv4(a4, other.d->a6, mode) && (a4 == d->a);
        case QAbstractSocket::AnyIPProtocol:
            return (mode & QHostAddress::ConvertUnspecifiedAddress) && d->a == 0;
        case QAbstractSocket::UnknownNetworkLayerProtocol:
            return false;
        }
    }

    if (d->protocol == QAbstractSocket::IPv6Protocol) {
        switch (other.d->protocol) {
        case QAbstractSocket::IPv4Protocol:
            quint32 a4;
            return convertToIpv4(a4, d->a6, mode) && (a4 == other.d->a);
        case QAbstractSocket::IPv6Protocol:
            return memcmp(&d->a6, &other.d->a6, sizeof(Q_IPV6ADDR)) == 0;
        case QAbstractSocket::AnyIPProtocol:
            return (mode & QHostAddress::ConvertUnspecifiedAddress)
                    && (other.d->a6_64.c[0] == 0) && (other.d->a6_64.c[1] == 0);
        case QAbstractSocket::UnknownNetworkLayerProtocol:
            return false;
        }
    }

    if ((d->protocol == QAbstractSocket::AnyIPProtocol)
            && (mode & QHostAddress::ConvertUnspecifiedAddress)) {
        switch (other.d->protocol) {
        case QAbstractSocket::IPv4Protocol:
            return other.d->a == 0;
        case QAbstractSocket::IPv6Protocol:
            return (other.d->a6_64.c[0] == 0) && (other.d->a6_64.c[1] == 0);
        default:
            break;
        }
    }

    return d->protocol == other.d->protocol;
}

/*!
    Returns \c true if this host address is the same as the \a other
    address given; otherwise returns \c false.
*/
bool QHostAddress::operator ==(SpecialAddress other) const
{
    QT_ENSURE_PARSED(this);
    quint32 ip4 = INADDR_ANY;
    switch (other) {
    case Null:
        return d->protocol == QAbstractSocket::UnknownNetworkLayerProtocol;

    case Broadcast:
        ip4 = INADDR_BROADCAST;
        break;

    case LocalHost:
        ip4 = INADDR_LOOPBACK;
        break;

    case Any:
        return d->protocol == QAbstractSocket::AnyIPProtocol;

    case AnyIPv4:
        break;

    case LocalHostIPv6:
    case AnyIPv6:
        if (d->protocol == QAbstractSocket::IPv6Protocol) {
            quint64 second = quint8(other == LocalHostIPv6);  // 1 for localhost, 0 for any
            return d->a6_64.c[0] == 0 && d->a6_64.c[1] == qToBigEndian(second);
        }
        return false;
    }

    // common IPv4 part
    return d->protocol == QAbstractSocket::IPv4Protocol && d->a == ip4;
}

/*!
    Returns \c true if this host address is null (INADDR_ANY or in6addr_any).
    The default constructor creates a null address, and that address is
    not valid for any host or interface.
*/
bool QHostAddress::isNull() const
{
    QT_ENSURE_PARSED(this);
    return d->protocol == QAbstractSocket::UnknownNetworkLayerProtocol;
}

/*!
    \since 4.5

    Returns \c true if this IP is in the subnet described by the network
    prefix \a subnet and netmask \a netmask.

    An IP is considered to belong to a subnet if it is contained
    between the lowest and the highest address in that subnet. In the
    case of IP version 4, the lowest address is the network address,
    while the highest address is the broadcast address.

    The \a subnet argument does not have to be the actual network
    address (the lowest address in the subnet). It can be any valid IP
    belonging to that subnet. In particular, if it is equal to the IP
    address held by this object, this function will always return true
    (provided the netmask is a valid value).

    \sa parseSubnet()
*/
bool QHostAddress::isInSubnet(const QHostAddress &subnet, int netmask) const
{
    QT_ENSURE_PARSED(this);
    if (subnet.protocol() != d->protocol || netmask < 0)
        return false;

    union {
        quint32 ip;
        quint8 data[4];
    } ip4, net4;
    const quint8 *ip;
    const quint8 *net;
    if (d->protocol == QAbstractSocket::IPv4Protocol) {
        if (netmask > 32)
            netmask = 32;
        ip4.ip = qToBigEndian(d->a);
        net4.ip = qToBigEndian(subnet.d->a);
        ip = ip4.data;
        net = net4.data;
    } else if (d->protocol == QAbstractSocket::IPv6Protocol) {
        if (netmask > 128)
            netmask = 128;
        ip = d->a6.c;
        net = subnet.d->a6.c;
    } else {
        return false;
    }

    if (netmask >= 8 && memcmp(ip, net, netmask / 8) != 0)
        return false;
    if ((netmask & 7) == 0)
        return true;

    // compare the last octet now
    quint8 bytemask = 256 - (1 << (8 - (netmask & 7)));
    quint8 ipbyte = ip[netmask / 8];
    quint8 netbyte = net[netmask / 8];
    return (ipbyte & bytemask) == (netbyte & bytemask);
}

/*!
    \since 4.5
    \overload

    Returns \c true if this IP is in the subnet described by \a
    subnet. The QHostAddress member of \a subnet contains the network
    prefix and the int (second) member contains the netmask (prefix
    length).
*/
bool QHostAddress::isInSubnet(const QPair<QHostAddress, int> &subnet) const
{
    return isInSubnet(subnet.first, subnet.second);
}


/*!
    \since 4.5

    Parses the IP and subnet information contained in \a subnet and
    returns the network prefix for that network and its prefix length.

    The IP address and the netmask must be separated by a slash
    (/).

    This function supports arguments in the form:
    \list
      \li 123.123.123.123/n  where n is any value between 0 and 32
      \li 123.123.123.123/255.255.255.255
      \li <ipv6-address>/n  where n is any value between 0 and 128
    \endlist

    For IP version 4, this function accepts as well missing trailing
    components (i.e., less than 4 octets, like "192.168.1"), followed
    or not by a dot. If the netmask is also missing in that case, it
    is set to the number of octets actually passed (in the example
    above, it would be 24, for 3 octets).

    \sa isInSubnet()
*/
QPair<QHostAddress, int> QHostAddress::parseSubnet(const QString &subnet)
{
    // We support subnets in the form:
    //   ddd.ddd.ddd.ddd/nn
    //   ddd.ddd.ddd/nn
    //   ddd.ddd/nn
    //   ddd/nn
    //   ddd.ddd.ddd.
    //   ddd.ddd.ddd
    //   ddd.ddd.
    //   ddd.ddd
    //   ddd.
    //   ddd
    //   <ipv6-address>/nn
    //
    //  where nn can be an IPv4-style netmask for the IPv4 forms

    const QPair<QHostAddress, int> invalid = qMakePair(QHostAddress(), -1);
    if (subnet.isEmpty())
        return invalid;

    int slash = subnet.indexOf(QLatin1Char('/'));
    QString netStr = subnet;
    if (slash != -1)
        netStr.truncate(slash);

    int netmask = -1;
    bool isIpv6 = netStr.contains(QLatin1Char(':'));

    if (slash != -1) {
        // is the netmask given in IP-form or in bit-count form?
        if (!isIpv6 && subnet.indexOf(QLatin1Char('.'), slash + 1) != -1) {
            // IP-style, convert it to bit-count form
            QNetmaskAddress parser;
            if (!parser.setAddress(subnet.mid(slash + 1)))
                return invalid;
            netmask = parser.prefixLength();
        } else {
            bool ok;
            netmask = subnet.midRef(slash + 1).toUInt(&ok);
            if (!ok)
                return invalid;     // failed to parse the subnet
        }
    }

    if (isIpv6) {
        // looks like it's an IPv6 address
        if (netmask > 128)
            return invalid;     // invalid netmask
        if (netmask < 0)
            netmask = 128;

        QHostAddress net;
        if (!net.setAddress(netStr))
            return invalid;     // failed to parse the IP

        clearBits(net.d->a6.c, netmask, 128);
        return qMakePair(net, netmask);
    }

    if (netmask > 32)
        return invalid;         // invalid netmask

    // parse the address manually
    auto parts = netStr.splitRef(QLatin1Char('.'));
    if (parts.isEmpty() || parts.count() > 4)
        return invalid;         // invalid IPv4 address

    if (parts.constLast().isEmpty())
        parts.removeLast();

    quint32 addr = 0;
    for (int i = 0; i < parts.count(); ++i) {
        bool ok;
        uint byteValue = parts.at(i).toUInt(&ok);
        if (!ok || byteValue > 255)
            return invalid;     // invalid IPv4 address

        addr <<= 8;
        addr += byteValue;
    }
    addr <<= 8 * (4 - parts.count());
    if (netmask == -1) {
        netmask = 8 * parts.count();
    } else if (netmask == 0) {
        // special case here
        // x86's instructions "shr" and "shl" do not operate when
        // their argument is 32, so the code below doesn't work as expected
        addr = 0;
    } else if (netmask != 32) {
        // clear remaining bits
        quint32 mask = quint32(0xffffffff) >> (32 - netmask) << (32 - netmask);
        addr &= mask;
    }

    return qMakePair(QHostAddress(addr), netmask);
}

/*!
    \since 5.0

    returns \c true if the address is the IPv6 loopback address, or any
    of the IPv4 loopback addresses.
*/
bool QHostAddress::isLoopback() const
{
    QT_ENSURE_PARSED(this);
    if ((d->a & 0xFF000000) == 0x7F000000)
        return true; // v4 range (including IPv6 wrapped IPv4 addresses)
    if (d->protocol == QAbstractSocket::IPv6Protocol) {
#ifdef __SSE2__
        const __m128i loopback = _mm_setr_epi8(0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1);
        __m128i ipv6 = _mm_loadu_si128((const __m128i *)d->a6.c);
        __m128i cmp = _mm_cmpeq_epi8(ipv6, loopback);
        return _mm_movemask_epi8(cmp) == 0xffff;
#else
        if (d->a6_64.c[0] != 0 || qFromBigEndian(d->a6_64.c[1]) != 1)
            return false;
#endif
        return true;
    }
    return false;
}

/*!
    \since 5.6

    Returns \c true if the address is an IPv4 or IPv6 multicast address, \c
    false otherwise.
*/
bool QHostAddress::isMulticast() const
{
    QT_ENSURE_PARSED(this);
    if ((d->a & 0xF0000000) == 0xE0000000)
        return true; // 224.0.0.0-239.255.255.255 (including v4-mapped IPv6 addresses)
    if (d->protocol == QAbstractSocket::IPv6Protocol)
        return d->a6.c[0] == 0xff;
    return false;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QHostAddress &address)
{
    QDebugStateSaver saver(d);
    d.resetFormat().nospace();
    if (address == QHostAddress::Any)
        d << "QHostAddress(QHostAddress::Any)";
    else
        d << "QHostAddress(" << address.toString() << ')';
    return d;
}
#endif

uint qHash(const QHostAddress &key, uint seed)
{
    // both lines might throw
    QT_ENSURE_PARSED(&key);
    return qHashBits(key.d->a6.c, 16, seed);
}

#ifndef QT_NO_DATASTREAM

/*! \relates QHostAddress

    Writes host address \a address to the stream \a out and returns a reference
    to the stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QHostAddress &address)
{
    qint8 prot;
    prot = qint8(address.protocol());
    out << prot;
    switch (address.protocol()) {
    case QAbstractSocket::UnknownNetworkLayerProtocol:
    case QAbstractSocket::AnyIPProtocol:
        break;
    case QAbstractSocket::IPv4Protocol:
        out << address.toIPv4Address();
        break;
    case QAbstractSocket::IPv6Protocol:
    {
        Q_IPV6ADDR ipv6 = address.toIPv6Address();
        for (int i = 0; i < 16; ++i)
            out << ipv6[i];
        out << address.scopeId();
    }
        break;
    }
    return out;
}

/*! \relates QHostAddress

    Reads a host address into \a address from the stream \a in and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QHostAddress &address)
{
    qint8 prot;
    in >> prot;
    switch (QAbstractSocket::NetworkLayerProtocol(prot)) {
    case QAbstractSocket::UnknownNetworkLayerProtocol:
        address.clear();
        break;
    case QAbstractSocket::IPv4Protocol:
    {
        quint32 ipv4;
        in >> ipv4;
        address.setAddress(ipv4);
    }
        break;
    case QAbstractSocket::IPv6Protocol:
    {
        Q_IPV6ADDR ipv6;
        for (int i = 0; i < 16; ++i)
            in >> ipv6[i];
        address.setAddress(ipv6);

        QString scope;
        in >> scope;
        address.setScopeId(scope);
    }
        break;
    case QAbstractSocket::AnyIPProtocol:
        address = QHostAddress::Any;
        break;
    default:
        address.clear();
        in.setStatus(QDataStream::ReadCorruptData);
    }
    return in;
}

#endif //QT_NO_DATASTREAM

QT_END_NAMESPACE
