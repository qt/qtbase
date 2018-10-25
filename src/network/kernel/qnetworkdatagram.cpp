/****************************************************************************
**
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

#include "qnetworkdatagram.h"
#include "qnetworkdatagram_p.h"

#ifndef QT_NO_UDPSOCKET

QT_BEGIN_NAMESPACE

/*!
    \class QNetworkDatagram
    \brief The QNetworkDatagram class provides the data and metadata of a UDP datagram.
    \since 5.8
    \ingroup network
    \inmodule QtNetwork
    \reentrant

    QNetworkDatagram can be used with the \l QUdpSocket class to represent the full
    information contained in a UDP (User Datagram Protocol) datagram.
    QNetworkDatagram encapsulates the following information of a datagram:
    \list
      \li the payload data;
      \li the sender address and port number;
      \li the destination address and port number;
      \li the remaining hop count limit (on IPv4, this field is usually called "time to live" - TTL);
      \li the network interface index the datagram was received on or to be sent on.
    \endlist

    QUdpSocket will try to match a common behavior as much as possible on all
    operating systems, but not all of the metadata above can be obtained in
    some operating systems. Metadata that cannot be set on the datagram when
    sending with QUdpSocket::writeDatagram() will be silently discarded.

    Upon reception, the senderAddress() and senderPort() properties contain the
    address and port of the peer that sent the datagram, while
    destinationAddress() and destinationPort() contain the target that was
    contained in the datagram. That is usually an address local to the current
    machine, but it can also be an IPv4 broadcast address (such as
    "255.255.255.255") or an IPv4 or IPv6 multicast address. Applications may
    find it useful to determine if the datagram was sent specifically to this
    machine via unicast addressing or whether it was sent to multiple destinations.

    When sending, the senderAddress() and senderPort() should contain the local
    address to be used when sending. The sender address must be an address that
    is assigned to this machine, which can be obtained using
    \l{QNetworkInterface}, and the port number must be the port number that the
    socket is bound to. Either field can be left unset and will be filled in by
    the operating system with default values. The destinationAddress() and
    destinationPort() fields may be set to a target address different from the
    one the UDP socket is currently associated with.

    Usually, when sending a datagram in reply to a datagram previously
    received, one will set the destinationAddress() to be the senderAddress()
    of the incoming datagram and similarly for the port numbers. To facilitate
    this common process, QNetworkDatagram provides the function makeReply().

    The hopCount() function contains, for a received datagram, the remaining
    hop count limit for the packet. When sending, it contains the hop count
    limit to be set. Most protocols will leave this value set to the default
    and let the operating system decide on the best value to be used.
    Multicasting over IPv4 often uses this field to indicate the scope of the
    multicast group (link-local, local to an organization or global).

    The interfaceIndex() function contains the index of the operating system's
    interface that received the packet. This value is the same one that can be
    set on a QHostAddress::scopeId() property and matches the
    QNetworkInterface::index() property. When sending packets to global
    addresses, it is not necessary to set the interface index as the operating
    system will choose the correct one using the system routing table. This
    property is important when sending datagrams to link-local destinations,
    whether unicast or multicast.

    \section1 Feature support

    Some features of QNetworkDatagram are not supported in all operating systems.
    Only the address and ports of the remote host (sender in received packets
    and destination for outgoing packets) are supported in all systems. On most
    operating systems, the other features are supported only for IPv6. Software
    should check at runtime whether the rest could be determined for IPv4
    addresses.

    The current feature support is as follows:

    \table
      \header   \li Operating system    \li Local address   \li Hop count       \li Interface index
      \row      \li FreeBSD             \li Supported       \li Supported       \li Only for IPv6
      \row      \li Linux               \li Supported       \li Supported       \li Supported
      \row      \li OS X                \li Supported       \li Supported       \li Only for IPv6
      \row      \li Other Unix supporting RFC 3542 \li Only for IPv6 \li Only for IPv6 \li Only for IPv6
      \row      \li Windows (desktop)   \li Supported       \li Supported       \li Supported
      \row      \li Windows RT          \li Not supported   \li Not supported   \li Not supported
    \endtable

    \sa QUdpSocket, QNetworkInterface
 */

/*!
    Creates a QNetworkDatagram object with no payload data and undefined destination address.

    The payload can be modified by using setData() and the destination address
    can be set with setDestination().

    If the destination address is left undefined, QUdpSocket::writeDatagram()
    will attempt to send the datagram to the address last associated with, by
    using QUdpSocket::connectToHost().
 */
QNetworkDatagram::QNetworkDatagram()
    : d(new QNetworkDatagramPrivate)
{
}

/*!
    Creates a QNetworkDatagram object and sets \a data as the payload data, along with
    \a destinationAddress and \a port as the destination address of the datagram.
 */
QNetworkDatagram::QNetworkDatagram(const QByteArray &data, const QHostAddress &destinationAddress, quint16 port)
    : d(new QNetworkDatagramPrivate(data, destinationAddress, port))
{
}

/*!
    Creates a copy of the \a other datagram, including the payload and metadata.

    To create a datagram suitable for sending in a reply, use QNetworkDatagram::makeReply();
 */
QNetworkDatagram::QNetworkDatagram(const QNetworkDatagram &other)
    : d(new QNetworkDatagramPrivate(*other.d))
{
}

/*! \internal */
QNetworkDatagram::QNetworkDatagram(QNetworkDatagramPrivate &dd)
    : d(&dd)
{
}

/*!
    Copies the \a other datagram, including the payload and metadata.

    To create a datagram suitable for sending in a reply, use QNetworkDatagram::makeReply();
 */
QNetworkDatagram &QNetworkDatagram::operator=(const QNetworkDatagram &other)
{
    *d = *other.d;
    return *this;
}

/*!
    Clears the payload data and metadata in this QNetworkDatagram object, resetting
    them to their default values.
 */
void QNetworkDatagram::clear()
{
    d->data.clear();
    d->header.senderAddress.clear();
    d->header.destinationAddress.clear();
    d->header.hopLimit = -1;
    d->header.ifindex = 0;
}

/*!
    \fn QNetworkDatagram::isNull() const
    Returns true if this QNetworkDatagram object is null. This function is the
    opposite of isValid().
 */

/*!
    Returns true if this QNetworkDatagram object is valid. A valid QNetworkDatagram
    object contains at least one sender or receiver address. Valid datagrams
    can contain empty payloads.
 */
bool QNetworkDatagram::isValid() const
{
    return d->header.senderAddress.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol ||
           d->header.destinationAddress.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol;
}

/*!
    Returns the sender address associated with this datagram. For a datagram
    received from the network, it is the address of the peer node that sent the
    datagram. For an outgoing datagrams, it is the local address to be used
    when sending.

    If no sender address was set on this datagram, the returned object will
    report true to QHostAddress::isNull().

    \sa destinationAddress(), senderPort(), setSender()
*/
QHostAddress QNetworkDatagram::senderAddress() const
{
    return d->header.senderAddress;
}

/*!
    Returns the destination address associated with this datagram. For a
    datagram received from the network, it is the address the peer node sent
    the datagram to, which can either be a local address of this machine or a
    multicast or broadcast address. For an outgoing datagrams, it is the
    address the datagram should be sent to.

    If no destination address was set on this datagram, the returned object
    will report true to QHostAddress::isNull().

    \sa senderAddress(), destinationPort(), setDestination()
*/
QHostAddress QNetworkDatagram::destinationAddress() const
{
    return d->header.destinationAddress;
}

/*!
    Returns the port number of the sender associated with this datagram. For a
    datagram received from the network, it is the port number that the peer
    node sent the datagram from. For an outgoing datagram, it is the local port
    the datagram should be sent from.

    If no sender address was associated with this datagram, this function
    returns -1.

    \sa senderAddress(), destinationPort(), setSender()
*/
int QNetworkDatagram::senderPort() const
{
    return d->header.senderAddress.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol
            ? -1 : d->header.senderPort;
}

/*!
    Returns the port number of the destination associated with this datagram.
    For a datagram received from the network, it is the local port number that
    the peer node sent the datagram to. For an outgoing datagram, it is the
    peer port the datagram should be sent to.

    If no destination address was associated with this datagram, this function
    returns -1.

    \sa destinationAddress(), senderPort(), setDestination()
*/
int QNetworkDatagram::destinationPort() const
{
    return d->header.destinationAddress.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol
            ? -1 : d->header.destinationPort;
}

/*!
    Sets the sender address associated with this datagram to be the address \a
    address and port number \a port. The sender address and port numbers are
    usually set by \l QUdpSocket upon reception, so there's no need to call
    this function on a received datagram.

    For outgoing datagrams, this function can be used to set the address the
    datagram should carry. The address \a address must usually be one of the
    local addresses assigned to this machine, which can be obtained using \l
    QNetworkInterface. If left unset, the operating system will choose the most
    appropriate address to use given the destination in question.

    The port number \a port must be the port number associated with the socket,
    if there is one. The value of 0 can be used to indicate that the operating
    system should choose the port number.

    \sa QUdpSocket::writeDatagram(), senderAddress(), senderPort(), setDestination()
 */
void QNetworkDatagram::setSender(const QHostAddress &address, quint16 port)
{
    d->header.senderAddress = address;
    d->header.senderPort = port;
}

/*!
    Sets the destination address associated with this datagram to be the
    address \a address and port number \a port. The destination address and
    port numbers are usually set by \l QUdpSocket upon reception, so there's no
    need to call this function on a received datagram.

    For outgoing datagrams, this function can be used to set the address the
    datagram should be sent to. It can be the unicast address used to
    communicate with the peer or a broadcast or multicast address to send to a
    group of devices.

    \sa QUdpSocket::writeDatagram(), destinationAddress(), destinationPort(), setSender()
 */
void QNetworkDatagram::setDestination(const QHostAddress &address, quint16 port)
{
    d->header.destinationAddress = address;
    d->header.destinationPort = port;
}

/*!
    Returns the hop count limit associated with this datagram. The hop count
    limit is the number of nodes that are allowed to forward the IP packet
    before it expires and an error is sent back to the sender of the datagram.
    In IPv4, this value is usually known as "time to live" (TTL).

    If this datagram was received from the network, this is the remaining hop
    count of the datagram after reception and was decremented by 1 by each node
    that forwarded the packet. A value of -1 indicates that the hop limit count
    not be obtained.

    If this is an outgoing datagram, this is the value to be set in the IP header
    upon sending. A value of -1 indicates the operating system should choose
    the value.

    \sa setHopLimit()
 */
int QNetworkDatagram::hopLimit() const
{
    return d->header.hopLimit;
}

/*!
    Sets the hop count limit associated with this datagram to \a count. The hop
    count limit is the number of nodes that are allowed to forward the IP
    packet before it expires and an error is sent back to the sender of the
    datagram. In IPv4, this value is usually known as "time to live" (TTL).

    It is usually not necessary to call this function on datagrams received
    from the network.

    If this is an outgoing packet, this is the value to be set in the IP header
    upon sending. The valid range for the value is 1 to 255. This function also
    accepts a value of -1 to indicate that the operating system should choose
    the value.

    \sa hopLimit()
 */
void QNetworkDatagram::setHopLimit(int count)
{
    d->header.hopLimit = count;
}

/*!
    Returns the interface index this datagram is associated with. The interface
    index is a positive number that uniquely identifies the network interface
    in the operating system. This number matches the value returned by
    QNetworkInterface::index() for the interface.

    If this datagram was received from the network, this is the index of the
    interface that the packet was received from. If this is an outgoing
    datagram, this is the index of the interface that the datagram should be
    sent on.

    A value of 0 indicates that the interface index is unknown.

    \sa setInterfaceIndex()
 */
uint QNetworkDatagram::interfaceIndex() const
{
    return d->header.ifindex;
}

/*!
    Sets the interface index this datagram is associated with to \a index. The
    interface index is a positive number that uniquely identifies the network
    interface in the operating system. This number matches the value returned
    by QNetworkInterface::index() for the interface.

    It is usually not necessary to call this function on datagrams received
    from the network.

    If this is an outgoing packet, this is the index of the interface the
    datagram should be sent on. A value of 0 indicates that the operating
    system should choose the interface based on other factors.

    Note that the interface index can also be set with
    QHostAddress::setScopeId() for IPv6 destination addresses and then with
    setDestination(). If the scope ID set in the destination address and \a
    index are different and neither is zero, it is undefined which interface
    the operating system will send the datagram on.

    \sa setInterfaceIndex()
 */
void QNetworkDatagram::setInterfaceIndex(uint index)
{
    d->header.ifindex = index;
}

/*!
    Returns the data payload of this datagram. For a datagram received from the
    network, it contains the payload of the datagram. For an outgoing datagram,
    it is the datagram to be sent.

    Note that datagrams can be transmitted with no data, so the returned
    QByteArray may be empty.

    \sa setData()
 */
QByteArray QNetworkDatagram::data() const
{
    return d->data;
}

/*!
    Sets the data payload of this datagram to \a data. It is usually not
    necessary to call this function on received datagrams. For outgoing
    datagrams, this function sets the data to be sent on the network.

    Since datagrams can empty, an empty QByteArray is a valid value for \a
    data.

    \sa data()
 */
void QNetworkDatagram::setData(const QByteArray &data)
{
    d->data = data;
}

/*!
    \fn QNetworkDatagram QNetworkDatagram::makeReply(const QByteArray &payload) const &
    \fn QNetworkDatagram QNetworkDatagram::makeReply(const QByteArray &payload) &&

    Creates a new QNetworkDatagram representing a reply to this incoming datagram
    and sets the payload data to \a payload. This function is a very convenient
    way of responding to a datagram back to the original sender.

    Example:
    \snippet code/src_network_kernel_qnetworkdatagram.cpp 0

    This function is especially convenient since it will automatically copy
    parameters from this datagram to the new datagram as appropriate:

    \list
      \li this datagram's sender address and port are copied to the new
          datagram's destination address and port;
      \li this datagram's interface index, if any, is copied to the new
          datagram's interface index;
      \li this datagram's destination address and port are copied to the new
          datagram's sender address and port only if the address is IPv6
          global (non-multicast) address;
      \li the hop count limit on the new datagram is reset to the default (-1);
    \endlist

    If QNetworkDatagram is modified in a future version of Qt to carry further
    metadata, this function will copy that metadata as appropriate.

    This datagram's destination address is not copied if it is an IPv4 address
    because it is not possible to tell an IPv4 broadcast address apart from a
    regular IPv4 address without an exhaustive search of all addresses assigned
    to this machine. Attempting to send a datagram with the sender address
    equal to the broadcast address is likely to fail. However, this should not
    affect the communication as network interfaces with multiple IPv4 addresses
    are uncommon, so the address the operating system will select will likely
    be one the peer will understand.

    \note This function comes with both rvalue- and lvalue-reference qualifier
    overloads, so it is a good idea to make sure this object is rvalue, if
    possible, before calling makeReply, so as to make better use of move
    semantics. To achieve that, the example above would use:
    \snippet code/src_network_kernel_qnetworkdatagram.cpp 1
 */


static bool isNonMulticast(const QHostAddress &addr)
{
    // is it a multicast address?
    return !addr.isMulticast();
}

QNetworkDatagram QNetworkDatagram::makeReply_helper(const QByteArray &data) const
{
    QNetworkDatagramPrivate *x = new QNetworkDatagramPrivate(data, d->header.senderAddress, d->header.senderPort);
    x->header.ifindex = d->header.ifindex;
    if (isNonMulticast(d->header.destinationAddress)) {
        x->header.senderAddress = d->header.destinationAddress;
        x->header.senderPort = d->header.destinationPort;
    }
    return QNetworkDatagram(*x);
}

void QNetworkDatagram::makeReply_helper_inplace(const QByteArray &data)
{
    d->data = data;
    d->header.hopLimit = -1;
    qSwap(d->header.destinationPort, d->header.senderPort);
    qSwap(d->header.destinationAddress, d->header.senderAddress);
    if (!isNonMulticast(d->header.senderAddress))
        d->header.senderAddress.clear();
}

void QNetworkDatagram::destroy(QNetworkDatagramPrivate *d)
{
    Q_ASSUME(d);
    delete d;
}

/*! \fn  void QNetworkDatagram::swap(QNetworkDatagram &other)
  Swaps this instance with \a other.
*/


QT_END_NAMESPACE

#endif // QT_NO_UDPSOCKET
