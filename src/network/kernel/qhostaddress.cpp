/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
# include <ws2tcpip.h>
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

QHostAddressPrivate::QHostAddressPrivate()
    : a(0), protocol(QAbstractSocket::UnknownNetworkLayerProtocol)
{
    memset(&a6, 0, sizeof(a6));
}

void QHostAddressPrivate::setAddress(quint32 a_)
{
    a = a_;
    protocol = QAbstractSocket::IPv4Protocol;

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
    QStringRef tmp(&address);
    int scopeIdPos = tmp.lastIndexOf(QLatin1Char('%'));
    if (scopeIdPos != -1) {
        *scopeId = tmp.mid(scopeIdPos + 1).toString();
        tmp.chop(tmp.size() - scopeIdPos);
    } else {
        scopeId->clear();
    }
    return QIPAddressUtils::parseIp6(addr, tmp.constBegin(), tmp.constEnd()) == 0;
}

bool QHostAddressPrivate::parse(const QString &ipString)
{
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
    memset(&a6, 0, sizeof(a6));
}

AddressClassification QHostAddressPrivate::classify() const
{
    if (a) {
        // This is an IPv4 address or an IPv6 v4-mapped address includes all
        // IPv6 v4-compat addresses, except for ::ffff:0.0.0.0 (because `a' is
        // zero). See setAddress(quint8*) below, which calls convertToIpv4(),
        // for details.
        // Source: RFC 5735
        if ((a & 0xff000000U) == 0x7f000000U)   // 127.0.0.0/8
            return LoopbackAddress;
        if ((a & 0xf0000000U) == 0xe0000000U)   // 224.0.0.0/4
            return MulticastAddress;
        if ((a & 0xffff0000U) == 0xa9fe0000U)   // 169.254.0.0/16
            return LinkLocalAddress;
        if ((a & 0xff000000U) == 0)             // 0.0.0.0/8 except 0.0.0.0 (handled below)
            return LocalNetAddress;
        if ((a & 0xf0000000U) == 0xf0000000U) { // 240.0.0.0/4
            if (a == 0xffffffffU)               // 255.255.255.255
                return BroadcastAddress;
            return UnknownAddress;
        }

        // Not testing for PrivateNetworkAddress and TestNetworkAddress
        // since we don't need them yet.
        return GlobalAddress;
    }

    // As `a' is zero, this address is either ::ffff:0.0.0.0 or a non-v4-mapped IPv6 address.
    // Source: https://www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xhtml
    if (a6_64.c[0]) {
        quint32 high16 = qFromBigEndian(a6_32.c[0]) >> 16;
        switch (high16 >> 8) {
        case 0xff:                          // ff00::/8
            return MulticastAddress;
        case 0xfe:
            switch (high16 & 0xffc0) {
            case 0xfec0:                    // fec0::/10
                return SiteLocalAddress;

            case 0xfe80:                    // fe80::/10
                return LinkLocalAddress;

            default:                        // fe00::/9
                return UnknownAddress;
            }
        case 0xfd:                          // fc00::/7
        case 0xfc:
            return UniqueLocalAddress;
        default:
            return GlobalAddress;
        }
    }

    quint64 low64 = qFromBigEndian(a6_64.c[1]);
    if (low64 == 1)                             // ::1
        return LoopbackAddress;
    if (low64 >> 32 == 0xffff) {                // ::ffff:0.0.0.0/96
        Q_ASSERT(quint32(low64) == 0);
        return LocalNetAddress;
    }
    if (low64)                                  // not ::
        return GlobalAddress;

    if (protocol == QAbstractSocket::UnknownNetworkLayerProtocol)
        return UnknownAddress;

    // only :: and 0.0.0.0 remain now
    return LocalNetAddress;
}

bool QNetmask::setAddress(const QHostAddress &address)
{
    static const quint8 zeroes[16] = { 0 };
    union {
        quint32 v4;
        quint8 v6[16];
    } ip;

    int netmask = 0;
    quint8 *ptr = ip.v6;
    quint8 *end;
    length = 255;

    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        ip.v4 = qToBigEndian(address.toIPv4Address());
        end = ptr + 4;
    } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        memcpy(ip.v6, address.toIPv6Address().c, 16);
        end = ptr + 16;
    } else {
        return false;
    }

    while (ptr < end) {
        switch (*ptr) {
        case 255:
            netmask += 8;
            ++ptr;
            continue;

        default:
            return false;       // invalid IP-style netmask

        case 254:
            ++netmask;
            Q_FALLTHROUGH();
        case 252:
            ++netmask;
            Q_FALLTHROUGH();
        case 248:
            ++netmask;
            Q_FALLTHROUGH();
        case 240:
            ++netmask;
            Q_FALLTHROUGH();
        case 224:
            ++netmask;
            Q_FALLTHROUGH();
        case 192:
            ++netmask;
            Q_FALLTHROUGH();
        case 128:
            ++netmask;
            Q_FALLTHROUGH();
        case 0:
            break;
        }
        break;
    }

    // confirm that the rest is only zeroes
    if (ptr < end && memcmp(ptr + 1, zeroes, end - ptr - 1) != 0)
        return false;

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

QHostAddress QNetmask::address(QAbstractSocket::NetworkLayerProtocol protocol) const
{
    if (length == 255 || protocol == QAbstractSocket::AnyIPProtocol ||
            protocol == QAbstractSocket::UnknownNetworkLayerProtocol) {
        return QHostAddress();
    } else if (protocol == QAbstractSocket::IPv4Protocol) {
        quint32 a;
        if (length == 0)
            a = 0;
        else if (length == 32)
            a = quint32(0xffffffff);
        else
            a = quint32(0xffffffff) >> (32 - length) << (32 - length);
        return QHostAddress(a);
    } else {
        Q_IPV6ADDR a6;
        memset(a6.c, 0xFF, sizeof(a6));
        clearBits(a6.c, length, 128);
        return QHostAddress(a6);
    }
}

/*!
    \class QHostAddress
    \brief The QHostAddress class provides an IP address.
    \ingroup network
    \ingroup shared
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

    \value Null The null address object. Equivalent to QHostAddress(). See also QHostAddress::isNull().
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
    d->parse(address);
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
        setAddress(((const sockaddr_in6 *)sockaddr)->sin6_addr.s6_addr);
#else
    Q_UNUSED(sockaddr)
#endif
}

/*!
    Constructs a copy of the given \a address.
*/
QHostAddress::QHostAddress(const QHostAddress &address)
    : d(address.d)
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
    d = address.d;
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
    \fn void QHostAddress::swap(QHostAddress &other)
    \since 5.6

    Swaps this host address with \a other. This operation is very fast
    and never fails.
*/

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
    Sets the host address to null.

    \sa QHostAddress::Null
*/
void QHostAddress::clear()
{
    d.detach();
    d->clear();
}

/*!
    Set the IPv4 address specified by \a ip4Addr.
*/
void QHostAddress::setAddress(quint32 ip4Addr)
{
    d.detach();
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
    d.detach();
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
    d.detach();
    d->setAddress(ip6Addr);
}

/*!
    \overload

    Set the IPv6 address specified by \a ip6Addr.
*/
void QHostAddress::setAddress(const Q_IPV6ADDR &ip6Addr)
{
    d.detach();
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
    d.detach();
    return d->parse(address);
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
    d.detach();
#ifndef Q_OS_WINRT
    clear();
    if (sockaddr->sa_family == AF_INET)
        setAddress(htonl(((const sockaddr_in *)sockaddr)->sin_addr.s_addr));
    else if (sockaddr->sa_family == AF_INET6)
        setAddress(((const sockaddr_in6 *)sockaddr)->sin6_addr.s6_addr);
#else
    Q_UNUSED(sockaddr)
#endif
}

/*!
    \overload
    \since 5.8

    Sets the special address specified by \a address.
*/
void QHostAddress::setAddress(SpecialAddress address)
{
    clear();

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
    return toIPv4Address(nullptr);
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
    d.detach();
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
    return d == other.d || isEqual(other, StrictConversion);
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
    if (d == other.d)
        return true;

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
    Returns \c true if this host address is not valid for any host or interface.

    The default constructor creates a null address.

    \sa QHostAddress::Null
*/
bool QHostAddress::isNull() const
{
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
    QStringRef netStr(&subnet);
    if (slash != -1)
        netStr.truncate(slash);

    int netmask = -1;
    bool isIpv6 = netStr.contains(QLatin1Char(':'));

    if (slash != -1) {
        // is the netmask given in IP-form or in bit-count form?
        if (!isIpv6 && subnet.indexOf(QLatin1Char('.'), slash + 1) != -1) {
            // IP-style, convert it to bit-count form
            QHostAddress mask;
            QNetmask parser;
            if (!mask.setAddress(subnet.mid(slash + 1)))
                return invalid;
            if (!parser.setAddress(mask))
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
        if (!net.setAddress(netStr.toString()))
            return invalid;     // failed to parse the IP

        clearBits(net.d->a6.c, netmask, 128);
        return qMakePair(net, netmask);
    }

    if (netmask > 32)
        return invalid;         // invalid netmask

    // parse the address manually
    auto parts = netStr.split(QLatin1Char('.'));
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
    return d->classify() == LoopbackAddress;
}

/*!
    \since 5.11

    Returns \c true if the address is an IPv4 or IPv6 global address, \c false
    otherwise. A global address is an address that is not reserved for
    special purposes (like loopback or multicast) or future purposes.

    Note that IPv6 unique local unicast addresses are considered global
    addresses (see isUniqueLocalUnicast()), as are IPv4 addresses reserved for
    local networks by \l {https://tools.ietf.org/html/rfc1918}{RFC 1918}.

    Also note that IPv6 site-local addresses are deprecated and should be
    considered as global in new applications. This function returns true for
    site-local addresses too.

    \sa isLoopback(), isSiteLocal(), isUniqueLocalUnicast()
*/
bool QHostAddress::isGlobal() const
{
    return d->classify() & GlobalAddress;   // GlobalAddress is a bit
}

/*!
    \since 5.11

    Returns \c true if the address is an IPv4 or IPv6 link-local address, \c
    false otherwise.

    An IPv4 link-local address is an address in the network 169.254.0.0/16. An
    IPv6 link-local address is one in the network fe80::/10. See the
    \l{https://www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xhtml}{IANA
    IPv6 Address Space} registry for more information.

    \sa isLoopback(), isGlobal(), isMulticast(), isSiteLocal(), isUniqueLocalUnicast()
*/
bool QHostAddress::isLinkLocal() const
{
    return d->classify() == LinkLocalAddress;
}

/*!
    \since 5.11

    Returns \c true if the address is an IPv6 site-local address, \c
    false otherwise.

    An IPv6 site-local address is one in the network fec0::/10. See the
    \l{https://www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xhtml}{IANA
    IPv6 Address Space} registry for more information.

    IPv6 site-local addresses are deprecated and should not be depended upon in
    new applications. New applications should not depend on this function and
    should consider site-local addresses the same as global (which is why
    isGlobal() also returns true). Site-local addresses were replaced by Unique
    Local Addresses (ULA).

    \sa isLoopback(), isGlobal(), isMulticast(), isLinkLocal(), isUniqueLocalUnicast()
*/
bool QHostAddress::isSiteLocal() const
{
    return d->classify() == SiteLocalAddress;
}

/*!
    \since 5.11

    Returns \c true if the address is an IPv6 unique local unicast address, \c
    false otherwise.

    An IPv6 unique local unicast address is one in the network fc00::/7. See the
    \l{https://www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xhtml}
    {IANA IPv6 Address Space} registry for more information.

    Note that Unique local unicast addresses count as global addresses too. RFC
    4193 says that, in practice, "applications may treat these addresses like
    global scoped addresses." Only routers need care about the distinction.

    \sa isLoopback(), isGlobal(), isMulticast(), isLinkLocal(), isUniqueLocalUnicast()
*/
bool QHostAddress::isUniqueLocalUnicast() const
{
    return d->classify() == UniqueLocalAddress;
}

/*!
    \since 5.6

    Returns \c true if the address is an IPv4 or IPv6 multicast address, \c
    false otherwise.

    \sa isLoopback(), isGlobal(), isLinkLocal(), isSiteLocal(), isUniqueLocalUnicast()
*/
bool QHostAddress::isMulticast() const
{
    return d->classify() == MulticastAddress;
}

/*!
    \since 5.11

    Returns \c true if the address is the IPv4 broadcast address, \c false
    otherwise. The IPv4 broadcast address is 255.255.255.255.

    Note that this function does not return true for an IPv4 network's local
    broadcast address. For that, please use \l QNetworkInterface to obtain the
    broadcast addresses of the local machine.

    \sa isLoopback(), isGlobal(), isMulticast(), isLinkLocal(), isUniqueLocalUnicast()
*/
bool QHostAddress::isBroadcast() const
{
    return d->classify() == BroadcastAddress;
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

/*!
    \since 5.0
    \relates QHostAddress
    Returns a hash of the host address \a key, using \a seed to seed the calculation.
*/
uint qHash(const QHostAddress &key, uint seed) Q_DECL_NOTHROW
{
    return qHashBits(key.d->a6.c, 16, seed);
}

/*!
    \fn bool operator==(QHostAddress::SpecialAddress lhs, const QHostAddress &rhs)
    \relates QHostAddress

    Returns \c true if special address \a lhs is the same as host address \a rhs;
    otherwise returns \c false.

    \sa isEqual()
*/

/*!
    \fn bool operator!=(QHostAddress::SpecialAddress lhs, const QHostAddress &rhs)
    \relates QHostAddress
    \since 5.9

    Returns \c false if special address \a lhs is the same as host address \a rhs;
    otherwise returns \c true.

    \sa isEqual()
*/

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
