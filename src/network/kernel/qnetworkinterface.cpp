/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2017 Intel Corporation.
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

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"

#include "qdebug.h"
#include "qendian.h"
#include "private/qtools_p.h"

#ifndef QT_NO_NETWORKINTERFACE

QT_BEGIN_NAMESPACE

static QList<QNetworkInterfacePrivate *> postProcess(QList<QNetworkInterfacePrivate *> list)
{
    // Some platforms report a netmask but don't report a broadcast address
    // Go through all available addresses and calculate the broadcast address
    // from the IP and the netmask
    //
    // This is an IPv4-only thing -- IPv6 has no concept of broadcasts
    // The math is:
    //    broadcast = IP | ~netmask

    for (QNetworkInterfacePrivate *interface : list) {
        for (QNetworkAddressEntry &address : interface->addressEntries) {
            if (address.ip().protocol() != QAbstractSocket::IPv4Protocol)
                continue;

            if (!address.netmask().isNull() && address.broadcast().isNull()) {
                QHostAddress bcast = address.ip();
                bcast = QHostAddress(bcast.toIPv4Address() | ~address.netmask().toIPv4Address());
                address.setBroadcast(bcast);
            }
        }
    }

    return list;
}

Q_GLOBAL_STATIC(QNetworkInterfaceManager, manager)

QNetworkInterfaceManager::QNetworkInterfaceManager()
{
}

QNetworkInterfaceManager::~QNetworkInterfaceManager()
{
}

QSharedDataPointer<QNetworkInterfacePrivate> QNetworkInterfaceManager::interfaceFromName(const QString &name)
{
    const auto interfaceList = allInterfaces();

    bool ok;
    uint index = name.toUInt(&ok);

    for (const auto &interface : interfaceList) {
        if (ok && interface->index == int(index))
            return interface;
        else if (interface->name == name)
            return interface;
    }

    return empty;
}

QSharedDataPointer<QNetworkInterfacePrivate> QNetworkInterfaceManager::interfaceFromIndex(int index)
{
    const auto interfaceList = allInterfaces();
    for (const auto &interface : interfaceList) {
        if (interface->index == index)
            return interface;
    }

    return empty;
}

QList<QSharedDataPointer<QNetworkInterfacePrivate> > QNetworkInterfaceManager::allInterfaces()
{
    const QList<QNetworkInterfacePrivate *> list = postProcess(scan());
    QList<QSharedDataPointer<QNetworkInterfacePrivate> > result;
    result.reserve(list.size());

    for (QNetworkInterfacePrivate *ptr : list) {
        if ((ptr->flags & QNetworkInterface::IsUp) == 0) {
            // if the network interface isn't UP, the addresses are ineligible for DNS
            for (auto &addr : ptr->addressEntries)
                addr.setDnsEligibility(QNetworkAddressEntry::DnsIneligible);
        }

        result << QSharedDataPointer<QNetworkInterfacePrivate>(ptr);
    }

    return result;
}

QString QNetworkInterfacePrivate::makeHwAddress(int len, uchar *data)
{
    const int outLen = qMax(len * 2 + (len - 1) * 1, 0);
    QString result(outLen, Qt::Uninitialized);
    QChar *out = result.data();
    for (int i = 0; i < len; ++i) {
        if (i)
            *out++ = QLatin1Char(':');
        *out++ = QLatin1Char(QtMiscUtils::toHexUpper(data[i] / 16));
        *out++ = QLatin1Char(QtMiscUtils::toHexUpper(data[i] % 16));
    }
    return result;
}

/*!
    \class QNetworkAddressEntry
    \brief The QNetworkAddressEntry class stores one IP address
    supported by a network interface, along with its associated
    netmask and broadcast address.

    \since 4.2
    \reentrant
    \ingroup network
    \ingroup shared
    \inmodule QtNetwork

    Each network interface can contain zero or more IP addresses, which
    in turn can be associated with a netmask and/or a broadcast
    address (depending on support from the operating system).

    This class represents one such group.
*/

/*!
    \enum QNetworkAddressEntry::DnsEligibilityStatus
    \since 5.11

    This enum indicates whether a given host address is eligible to be
    published in the Domain Name System (DNS) or other similar name resolution
    mechanisms. In general, an address is suitable for publication if it is an
    address this machine will be reached at for an indeterminate amount of
    time, though it need not be permanent. For example, addresses obtained via
    DHCP are often eligible, but cryptographically-generated temporary IPv6
    addresses are not.

    \value DnsEligibilityUnknown    Qt and the operating system could not determine
                                    whether this address should be published or not.
                                    The application may need to apply further
                                    heuristics if it cannot find any eligible
                                    addresses.
    \value DnsEligible              This address is eligible for publication in DNS.
    \value DnsIneligible            This address should not be published in DNS and
                                    should not be transmitted to other parties,
                                    except maybe as the source address of an outgoing
                                    packet.

    \sa dnsEligibility(), setDnsEligibility()
*/

/*!
    Constructs an empty QNetworkAddressEntry object.
*/
QNetworkAddressEntry::QNetworkAddressEntry()
    : d(new QNetworkAddressEntryPrivate)
{
}

/*!
    Constructs a QNetworkAddressEntry object that is a copy of the
    object \a other.
*/
QNetworkAddressEntry::QNetworkAddressEntry(const QNetworkAddressEntry &other)
    : d(new QNetworkAddressEntryPrivate(*other.d.data()))
{
}

/*!
    Makes a copy of the QNetworkAddressEntry object \a other.
*/
QNetworkAddressEntry &QNetworkAddressEntry::operator=(const QNetworkAddressEntry &other)
{
    *d.data() = *other.d.data();
    return *this;
}

/*!
    \fn void QNetworkAddressEntry::swap(QNetworkAddressEntry &other)
    \since 5.0

    Swaps this network address entry instance with \a other. This
    function is very fast and never fails.
*/

/*!
    Destroys this QNetworkAddressEntry object.
*/
QNetworkAddressEntry::~QNetworkAddressEntry()
{
}

/*!
    Returns \c true if this network address entry is the same as \a
    other.
*/
bool QNetworkAddressEntry::operator==(const QNetworkAddressEntry &other) const
{
    if (d == other.d) return true;
    if (!d || !other.d) return false;
    return d->address == other.d->address &&
        d->netmask == other.d->netmask &&
        d->broadcast == other.d->broadcast;
}

/*!
    \since 5.11

    Returns whether this address is eligible for publication in the Domain Name
    System (DNS) or similar name resolution mechanisms.

    In general, an address is suitable for publication if it is an address this
    machine will be reached at for an indeterminate amount of time, though it
    need not be permanent. For example, addresses obtained via DHCP are often
    eligible, but cryptographically-generated temporary IPv6 addresses are not.

    On some systems, QNetworkInterface will need to heuristically determine
    which addresses are eligible.

    \sa isLifetimeKnown(), isPermanent(), setDnsEligibility()
*/
QNetworkAddressEntry::DnsEligibilityStatus QNetworkAddressEntry::dnsEligibility() const
{
    return d->dnsEligibility;
}

/*!
    \since 5.11

    Sets the DNS eligibility flag for this address to \a status.

    \sa dnsEligibility()
*/
void QNetworkAddressEntry::setDnsEligibility(DnsEligibilityStatus status)
{
    d->dnsEligibility = status;
}

/*!
    \fn bool QNetworkAddressEntry::operator!=(const QNetworkAddressEntry &other) const

    Returns \c true if this network address entry is different from \a
    other.
*/

/*!
    This function returns one IPv4 or IPv6 address found, that was
    found in a network interface.
*/
QHostAddress QNetworkAddressEntry::ip() const
{
    return d->address;
}

/*!
    Sets the IP address the QNetworkAddressEntry object contains to \a
    newIp.
*/
void QNetworkAddressEntry::setIp(const QHostAddress &newIp)
{
    d->address = newIp;
}

/*!
    Returns the netmask associated with the IP address. The
    netmask is expressed in the form of an IP address, such as
    255.255.0.0.

    For IPv6 addresses, the prefix length is converted to an address
    where the number of bits set to 1 is equal to the prefix
    length. For a prefix length of 64 bits (the most common value),
    the netmask will be expressed as a QHostAddress holding the
    address FFFF:FFFF:FFFF:FFFF::

    \sa prefixLength()
*/
QHostAddress QNetworkAddressEntry::netmask() const
{
    return d->netmask.address(d->address.protocol());
}

/*!
    Sets the netmask that this QNetworkAddressEntry object contains to
    \a newNetmask. Setting the netmask also sets the prefix length to
    match the new netmask.

    \sa setPrefixLength()
*/
void QNetworkAddressEntry::setNetmask(const QHostAddress &newNetmask)
{
    if (newNetmask.protocol() != ip().protocol()) {
        d->netmask = QNetmask();
        return;
    }

    d->netmask.setAddress(newNetmask);
}

/*!
    \since 4.5
    Returns the prefix length of this IP address. The prefix length
    matches the number of bits set to 1 in the netmask (see
    netmask()). For IPv4 addresses, the value is between 0 and 32. For
    IPv6 addresses, it's contained between 0 and 128 and is the
    preferred form of representing addresses.

    This function returns -1 if the prefix length could not be
    determined (i.e., netmask() returns a null QHostAddress()).

    \sa netmask()
*/
int QNetworkAddressEntry::prefixLength() const
{
    return d->netmask.prefixLength();
}

/*!
    \since 4.5
    Sets the prefix length of this IP address to \a length. The value
    of \a length must be valid for this type of IP address: between 0
    and 32 for IPv4 addresses, between 0 and 128 for IPv6
    addresses. Setting to any invalid value is equivalent to setting
    to -1, which means "no prefix length".

    Setting the prefix length also sets the netmask (see netmask()).

    \sa setNetmask()
*/
void QNetworkAddressEntry::setPrefixLength(int length)
{
    d->netmask.setPrefixLength(d->address.protocol(), length);
}

/*!
    Returns the broadcast address associated with the IPv4
    address and netmask. It can usually be derived from those two by
    setting to 1 the bits of the IP address where the netmask contains
    a 0. (In other words, by bitwise-OR'ing the IP address with the
    inverse of the netmask)

    This member is always empty for IPv6 addresses, since the concept
    of broadcast has been abandoned in that system in favor of
    multicast. In particular, the group of hosts corresponding to all
    the nodes in the local network can be reached by the "all-nodes"
    special multicast group (address FF02::1).
*/
QHostAddress QNetworkAddressEntry::broadcast() const
{
    return d->broadcast;
}

/*!
    Sets the broadcast IP address of this QNetworkAddressEntry object
    to \a newBroadcast.
*/
void QNetworkAddressEntry::setBroadcast(const QHostAddress &newBroadcast)
{
    d->broadcast = newBroadcast;
}

/*!
    \since 5.11

    Returns \c true if the address lifetime is known, \c false if not. If the
    lifetime is not known, both preferredLifetime() and validityLifetime() will
    return QDeadlineTimer::Forever.

    \sa preferredLifetime(), validityLifetime(), setAddressLifetime(), clearAddressLifetime()
*/
bool QNetworkAddressEntry::isLifetimeKnown() const
{
    return d->lifetimeKnown;
}

/*!
    \since 5.11

    Returns the deadline when this address becomes deprecated (no longer
    preferred), if known. If the address lifetime is not known (see
    isLifetimeKnown()), this function always returns QDeadlineTimer::Forever.

    While an address is preferred, it may be used by the operating system as
    the source address for new, outgoing packets. After it becomes deprecated,
    it will remain valid for incoming packets for a while longer until finally
    removed (see validityLifetime()).

    \sa validityLifetime(), isLifetimeKnown(), setAddressLifetime(), clearAddressLifetime()
*/
QDeadlineTimer QNetworkAddressEntry::preferredLifetime() const
{
    return d->preferredLifetime;
}

/*!
    \since 5.11

    Returns the deadline when this address becomes invalid and will be removed
    from the networking stack, if known. If the address lifetime is not known
    (see isLifetimeKnown()), this function always returns
    QDeadlineTimer::Forever.

    While an address is valid, it will be accepted by the operating system as a
    valid destination address for this machine. Whether it is used as a source
    address for new, outgoing packets is controlled by, among other rules, the
    preferred lifetime (see preferredLifetime()).

    \sa preferredLifetime(), isLifetimeKnown(), setAddressLifetime(), clearAddressLifetime()
*/
QDeadlineTimer QNetworkAddressEntry::validityLifetime() const
{
    return d->validityLifetime;
}

/*!
    \since 5.11

    Sets both the preferred and valid lifetimes for this address to the \a
    preferred and \a validity deadlines, respectively. After this call,
    isLifetimeKnown() will return \c true, even if both parameters are
    QDeadlineTimer::Forever.

    \sa preferredLifetime(), validityLifetime(), isLifetimeKnown(), clearAddressLifetime()
*/
void QNetworkAddressEntry::setAddressLifetime(QDeadlineTimer preferred, QDeadlineTimer validity)
{
    d->preferredLifetime = preferred;
    d->validityLifetime = validity;
    d->lifetimeKnown = true;
}

/*!
    \since 5.11

    Resets both the preferred and valid lifetimes for this address. After this
    call, isLifetimeKnown() will return \c false.

    \sa preferredLifetime(), validityLifetime(), isLifetimeKnown(), setAddressLifetime()
*/
void QNetworkAddressEntry::clearAddressLifetime()
{
    d->preferredLifetime = QDeadlineTimer::Forever;
    d->validityLifetime = QDeadlineTimer::Forever;
    d->lifetimeKnown = false;
}

/*!
    \since 5.11

    Returns \c true if this address is permanent on this interface, \c false if
    it's temporary. A permenant address is one which has no expiration time and
    is often static (manually configured).

    If this information could not be determined, this function returns \c true.

    \note Depending on the operating system and the networking configuration
    tool, it is possible for a temporary address to be interpreted as
    permanent, if the tool did not inform the details correctly to the
    operating system.

    \sa isLifetimeKnown(), validityLifetime(), isTemporary()
*/
bool QNetworkAddressEntry::isPermanent() const
{
    return d->validityLifetime.isForever();
}

/*!
    \fn bool QNetworkAddressEntry::isTemporary() const
    \since 5.11

    Returns \c true if this address is temporary on this interface, \c false if
    it's permanent.

    \sa isLifetimeKnown(), validityLifetime(), isPermanent()
*/

/*!
    \class QNetworkInterface
    \brief The QNetworkInterface class provides a listing of the host's IP
    addresses and network interfaces.

    \since 4.2
    \reentrant
    \ingroup network
    \ingroup shared
    \inmodule QtNetwork

    QNetworkInterface represents one network interface attached to the
    host where the program is being run. Each network interface may
    contain zero or more IP addresses, each of which is optionally
    associated with a netmask and/or a broadcast address. The list of
    such trios can be obtained with addressEntries(). Alternatively,
    when the netmask or the broadcast addresses or other information aren't
    necessary, use the allAddresses() convenience function to obtain just the
    IP addresses of the active interfaces.

    QNetworkInterface also reports the interface's hardware address with
    hardwareAddress().

    Not all operating systems support reporting all features. Only the
    IPv4 addresses are guaranteed to be listed by this class in all
    platforms. In particular, IPv6 address listing is only supported
    on Windows, Linux, \macos and the BSDs.

    \sa QNetworkAddressEntry
*/

/*!
    \enum QNetworkInterface::InterfaceFlag
    Specifies the flags associated with this network interface. The
    possible values are:

    \value IsUp                 the network interface is active
    \value IsRunning            the network interface has resources
                                allocated
    \value CanBroadcast         the network interface works in
                                broadcast mode
    \value IsLoopBack           the network interface is a loopback
                                interface: that is, it's a virtual
                                interface whose destination is the
                                host computer itself
    \value IsPointToPoint       the network interface is a
                                point-to-point interface: that is,
                                there is one, single other address
                                that can be directly reached by it.
    \value CanMulticast         the network interface supports
                                multicasting

    Note that one network interface cannot be both broadcast-based and
    point-to-point.
*/

/*!
    \enum QNetworkInterface::InterfaceType

    Specifies the type of hardware (PHY layer, OSI level 1) this interface is,
    if it could be determined. Interface types that are not among those listed
    below will generally be listed as Unknown, though future versions of Qt may
    add new enumeration values.

    The possible values are:

    \value Unknown              The interface type could not be determined or is not
                                one of the other listed types.
    \value Loopback             The virtual loopback interface, which is assigned
                                the loopback IP addresses (127.0.0.1, ::1).
    \value Virtual              A type of interface determined to be virtual, but
                                not any of the other possible types. For example,
                                tunnel interfaces are (currently) detected as
                                virtual ones.
    \value Ethernet             IEEE 802.3 Ethernet interfaces, though on many
                                systems other types of IEEE 802 interfaces may also
                                be detected as Ethernet (especially Wi-Fi).
    \value Wifi                 IEEE 802.11 Wi-Fi interfaces. Note that on some
                                systems, QNetworkInterface may be unable to
                                distinguish regular Ethernet from Wi-Fi and will
                                not return this enum value.
    \value Ieee80211            An alias for WiFi.
    \value CanBus               ISO 11898 Controller Area Network bus interfaces,
                                usually found on automotive systems.
    \value Fddi                 ANSI X3T12 Fiber Distributed Data Interface, a local area
                                network over optical fibers.
    \value Ppp                  Point-to-Point Protocol interfaces, establishing a
                                direct connection between two nodes over a lower
                                transport layer (often serial over radio or physical
                                line).
    \value Slip                 Serial Line Internet Protocol interfaces.
    \value Phonet               Interfaces using the Linux Phonet socket family, for
                                communication with cellular modems. See the
                                \l {https://www.kernel.org/doc/Documentation/networking/phonet.txt}{Linux kernel documentation}
                                for more information.
    \value Ieee802154           IEEE 802.15.4 Personal Area Network interfaces, other
                                than 6LoWPAN (see below).
    \value SixLoWPAN            6LoWPAN (IPv6 over Low-power Wireless Personal Area
                                Networks) interfaces, which operate on IEEE 802.15.4
                                PHY, but have specific header compression schemes
                                for IPv6 and UDP. This type of interface is often
                                used for mesh networking.
    \value Ieee80216            IEEE 802.16 Wireless Metropolitan Area Network, also
                                known under the commercial name "WiMAX".
    \value Ieee1394             IEEE 1394 interfaces (a.k.a. "FireWire").
*/

/*!
    Constructs an empty network interface object.
*/
QNetworkInterface::QNetworkInterface()
    : d(nullptr)
{
}

/*!
    Frees the resources associated with the QNetworkInterface object.
*/
QNetworkInterface::~QNetworkInterface()
{
}

/*!
    Creates a copy of the QNetworkInterface object contained in \a
    other.
*/
QNetworkInterface::QNetworkInterface(const QNetworkInterface &other)
    : d(other.d)
{
}

/*!
    Copies the contents of the QNetworkInterface object contained in \a
    other into this one.
*/
QNetworkInterface &QNetworkInterface::operator=(const QNetworkInterface &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QNetworkInterface::swap(QNetworkInterface &other)
    \since 5.0

    Swaps this network interface instance with \a other. This function
    is very fast and never fails.
*/

/*!
    Returns \c true if this QNetworkInterface object contains valid
    information about a network interface.
*/
bool QNetworkInterface::isValid() const
{
    return !name().isEmpty();
}

/*!
    \since 4.5
    Returns the interface system index, if known. This is an integer
    assigned by the operating system to identify this interface and it
    generally doesn't change. It matches the scope ID field in IPv6
    addresses.

    If the index isn't known, this function returns 0.
*/
int QNetworkInterface::index() const
{
    return d ? d->index : 0;
}

/*!
    \since 5.11

    Returns the maximum transmission unit on this interface, if known, or 0
    otherwise.

    The maximum transmission unit is the largest packet that may be sent on
    this interface without incurring link-level fragmentation. Applications may
    use this value to calculate the size of the payload that will fit an
    unfragmented UDP datagram. Remember to subtract the sizes of headers used
    in your communication over the interface, e.g. TCP (20 bytes) or UDP (12),
    IPv4 (20) or IPv6 (40, absent some form of header compression), when
    computing how big a payload you can transmit. Also note that the MTU along
    the full path (the Path MTU) to the destination may be smaller than the
    interface's MTU.

    \sa QUdpSocket
*/
int QNetworkInterface::maximumTransmissionUnit() const
{
    return d ? d->mtu : 0;
}

/*!
    Returns the name of this network interface. On Unix systems, this
    is a string containing the type of the interface and optionally a
    sequence number, such as "eth0", "lo" or "pcn0". On Windows, it's
    an internal ID that cannot be changed by the user.
*/
QString QNetworkInterface::name() const
{
    return d ? d->name : QString();
}

/*!
    \since 4.5

    Returns the human-readable name of this network interface on
    Windows, such as "Local Area Connection", if the name could be
    determined. If it couldn't, this function returns the same as
    name(). The human-readable name is a name that the user can modify
    in the Windows Control Panel, so it may change during the
    execution of the program.

    On Unix, this function currently always returns the same as
    name(), since Unix systems don't store a configuration for
    human-readable names.
*/
QString QNetworkInterface::humanReadableName() const
{
    return d ? !d->friendlyName.isEmpty() ? d->friendlyName : name() : QString();
}

/*!
    Returns the flags associated with this network interface.
*/
QNetworkInterface::InterfaceFlags QNetworkInterface::flags() const
{
    return d ? d->flags : InterfaceFlags{};
}

/*!
    \since 5.11

    Returns the type of this interface, if it could be determined. If it could
    not be determined, this function returns QNetworkInterface::Unknown.

    \sa hardwareAddress()
*/
QNetworkInterface::InterfaceType QNetworkInterface::type() const
{
    return d ? d->type : Unknown;
}

/*!
    Returns the low-level hardware address for this interface. On
    Ethernet interfaces, this will be a MAC address in string
    representation, separated by colons.

    Other interface types may have other types of hardware
    addresses. Implementations should not depend on this function
    returning a valid MAC address.

    \sa type()
*/
QString QNetworkInterface::hardwareAddress() const
{
    return d ? d->hardwareAddress : QString();
}

/*!
    Returns the list of IP addresses that this interface possesses
    along with their associated netmasks and broadcast addresses.

    If the netmask or broadcast address or other information is not necessary,
    you can call the allAddresses() function to obtain just the IP addresses of
    the active interfaces.
*/
QList<QNetworkAddressEntry> QNetworkInterface::addressEntries() const
{
    return d ? d->addressEntries : QList<QNetworkAddressEntry>();
}

/*!
    \since 5.7

    Returns the index of the interface whose name is \a name or 0 if there is
    no interface with that name. This function should produce the same result
    as the following code, but will probably execute faster.

    \snippet code/src_network_kernel_qnetworkinterface.cpp 0

    \sa interfaceFromName(), interfaceNameFromIndex(), QNetworkDatagram::interfaceIndex()
*/
int QNetworkInterface::interfaceIndexFromName(const QString &name)
{
    if (name.isEmpty())
        return 0;

    bool ok;
    uint id = name.toUInt(&ok);
    if (!ok)
        id = QNetworkInterfaceManager::interfaceIndexFromName(name);
    return int(id);
}

/*!
    Returns a QNetworkInterface object for the interface named \a
    name. If no such interface exists, this function returns an
    invalid QNetworkInterface object.

    The string \a name may be either an actual interface name (such as "eth0"
    or "en1") or an interface index in string form ("1", "2", etc.).

    \sa name(), isValid()
*/
QNetworkInterface QNetworkInterface::interfaceFromName(const QString &name)
{
    QNetworkInterface result;
    result.d = manager()->interfaceFromName(name);
    return result;
}

/*!
    Returns a QNetworkInterface object for the interface whose internal
    ID is \a index. Network interfaces have a unique identifier called
    the "interface index" to distinguish it from other interfaces on
    the system. Often, this value is assigned progressively and
    interfaces being removed and then added again get a different
    value every time.

    This index is also found in the IPv6 address' scope ID field.
*/
QNetworkInterface QNetworkInterface::interfaceFromIndex(int index)
{
    QNetworkInterface result;
    result.d = manager()->interfaceFromIndex(index);
    return result;
}

/*!
    \since 5.7

    Returns the name of the interface whose index is \a index or an empty
    string if there is no interface with that index. This function should
    produce the same result as the following code, but will probably execute
    faster.

    \snippet code/src_network_kernel_qnetworkinterface.cpp 1

    \sa interfaceFromIndex(), interfaceIndexFromName(), QNetworkDatagram::interfaceIndex()
*/
QString QNetworkInterface::interfaceNameFromIndex(int index)
{
    if (!index)
        return QString();
    return QNetworkInterfaceManager::interfaceNameFromIndex(index);
}

/*!
    Returns a listing of all the network interfaces found on the host
    machine.  In case of failure it returns a list with zero elements.
*/
QList<QNetworkInterface> QNetworkInterface::allInterfaces()
{
    const QList<QSharedDataPointer<QNetworkInterfacePrivate> > privs = manager()->allInterfaces();
    QList<QNetworkInterface> result;
    result.reserve(privs.size());
    for (const auto &p : privs) {
        QNetworkInterface item;
        item.d = p;
        result << item;
    }

    return result;
}

/*!
    This convenience function returns all IP addresses found on the host
    machine. It is equivalent to calling addressEntries() on all the objects
    returned by allInterfaces() that are in the QNetworkInterface::IsUp state
    to obtain lists of QNetworkAddressEntry objects then calling
    QNetworkAddressEntry::ip() on each of these.
*/
QList<QHostAddress> QNetworkInterface::allAddresses()
{
    const QList<QSharedDataPointer<QNetworkInterfacePrivate> > privs = manager()->allInterfaces();
    QList<QHostAddress> result;
    for (const auto &p : privs) {
        // skip addresses if the interface isn't up
        if ((p->flags & QNetworkInterface::IsUp) == 0)
            continue;

        for (const QNetworkAddressEntry &entry : qAsConst(p->addressEntries))
            result += entry.ip();
    }

    return result;
}

#ifndef QT_NO_DEBUG_STREAM
static inline QDebug flagsDebug(QDebug debug, QNetworkInterface::InterfaceFlags flags)
{
    if (flags & QNetworkInterface::IsUp)
        debug << "IsUp ";
    if (flags & QNetworkInterface::IsRunning)
        debug << "IsRunning ";
    if (flags & QNetworkInterface::CanBroadcast)
        debug << "CanBroadcast ";
    if (flags & QNetworkInterface::IsLoopBack)
        debug << "IsLoopBack ";
    if (flags & QNetworkInterface::IsPointToPoint)
        debug << "IsPointToPoint ";
    if (flags & QNetworkInterface::CanMulticast)
        debug << "CanMulticast ";
    return debug;
}

static inline QDebug operator<<(QDebug debug, const QNetworkAddressEntry &entry)
{
    debug << "(address = " << entry.ip();
    if (!entry.netmask().isNull())
        debug << ", netmask = " << entry.netmask();
    if (!entry.broadcast().isNull())
        debug << ", broadcast = " << entry.broadcast();
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const QNetworkInterface &networkInterface)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << "QNetworkInterface(name = " << networkInterface.name()
          << ", hardware address = " << networkInterface.hardwareAddress()
          << ", flags = ";
    flagsDebug(debug, networkInterface.flags());
    debug << ", entries = " << networkInterface.addressEntries()
          << ")\n";
    return debug;
}
#endif

QT_END_NAMESPACE

#include "moc_qnetworkinterface.cpp"

#endif // QT_NO_NETWORKINTERFACE
