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

//#define QNATIVESOCKETENGINE_DEBUG
#include "qnativesocketengine_p.h"
#include "private/qnet_unix_p.h"
#include "qiodevice.h"
#include "qhostaddress.h"
#include "qelapsedtimer.h"
#include "qvarlengtharray.h"
#include "qnetworkinterface.h"
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#ifndef QT_NO_IPV6IFNAME
#include <net/if.h>
#endif
#ifdef QT_LINUXBASE
#include <arpa/inet.h>
#endif
#ifdef Q_OS_BSD4
#include <net/if_dl.h>
#endif
#ifdef Q_OS_INTEGRITY
#include <sys/uio.h>
#endif

#if defined QNATIVESOCKETENGINE_DEBUG
#include <qstring.h>
#include <ctype.h>
#endif

#include <netinet/tcp.h>
#ifndef QT_NO_SCTP
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#endif

QT_BEGIN_NAMESPACE

#if defined QNATIVESOCKETENGINE_DEBUG

/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            QString tmp;
            tmp.sprintf("\\%o", c);
            out += tmp.toLatin1();
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}
#endif

/*
    Extracts the port and address from a sockaddr, and stores them in
    \a port and \a addr if they are non-null.
*/
static inline void qt_socket_getPortAndAddress(const qt_sockaddr *s, quint16 *port, QHostAddress *addr)
{
    if (s->a.sa_family == AF_INET6) {
        Q_IPV6ADDR tmp;
        memcpy(&tmp, &s->a6.sin6_addr, sizeof(tmp));
        if (addr) {
            QHostAddress tmpAddress;
            tmpAddress.setAddress(tmp);
            *addr = tmpAddress;
#if QT_CONFIG(networkinterface)
            if (s->a6.sin6_scope_id)
                addr->setScopeId(QNetworkInterface::interfaceNameFromIndex(s->a6.sin6_scope_id));
#endif
        }
        if (port)
            *port = ntohs(s->a6.sin6_port);
        return;
    }

    if (port)
        *port = ntohs(s->a4.sin_port);
    if (addr) {
        QHostAddress tmpAddress;
        tmpAddress.setAddress(ntohl(s->a4.sin_addr.s_addr));
        *addr = tmpAddress;
    }
}

static void convertToLevelAndOption(QNativeSocketEngine::SocketOption opt,
                                    QAbstractSocket::NetworkLayerProtocol socketProtocol, int &level, int &n)
{
    n = -1;
    level = SOL_SOCKET; // default

    switch (opt) {
    case QNativeSocketEngine::NonBlockingSocketOption:  // fcntl, not setsockopt
    case QNativeSocketEngine::BindExclusively:          // not handled on Unix
    case QNativeSocketEngine::MaxStreamsSocketOption:
        Q_UNREACHABLE();

    case QNativeSocketEngine::BroadcastSocketOption:
        n = SO_BROADCAST;
        break;
    case QNativeSocketEngine::ReceiveBufferSocketOption:
        n = SO_RCVBUF;
        break;
    case QNativeSocketEngine::SendBufferSocketOption:
        n = SO_SNDBUF;
        break;
    case QNativeSocketEngine::AddressReusable:
        n = SO_REUSEADDR;
        break;
    case QNativeSocketEngine::ReceiveOutOfBandData:
        n = SO_OOBINLINE;
        break;
    case QNativeSocketEngine::LowDelayOption:
        level = IPPROTO_TCP;
        n = TCP_NODELAY;
        break;
    case QNativeSocketEngine::KeepAliveOption:
        n = SO_KEEPALIVE;
        break;
    case QNativeSocketEngine::MulticastTtlOption:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_HOPS;
        } else
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_TTL;
        }
        break;
    case QNativeSocketEngine::MulticastLoopbackOption:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_LOOP;
        } else
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_LOOP;
        }
        break;
    case QNativeSocketEngine::TypeOfServiceOption:
        if (socketProtocol == QAbstractSocket::IPv4Protocol) {
            level = IPPROTO_IP;
            n = IP_TOS;
        }
        break;
    case QNativeSocketEngine::ReceivePacketInformation:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_RECVPKTINFO;
        } else if (socketProtocol == QAbstractSocket::IPv4Protocol) {
            level = IPPROTO_IP;
#ifdef IP_PKTINFO
            n = IP_PKTINFO;
#elif defined(IP_RECVDSTADDR)
            // variant found in QNX and FreeBSD; it will get us only the
            // destination address, not the interface; we need IP_RECVIF for that.
            n = IP_RECVDSTADDR;
#endif
        }
        break;
    case QNativeSocketEngine::ReceiveHopLimit:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_RECVHOPLIMIT;
        } else if (socketProtocol == QAbstractSocket::IPv4Protocol) {
#ifdef IP_RECVTTL               // IP_RECVTTL is a non-standard extension supported on some OS
            level = IPPROTO_IP;
            n = IP_RECVTTL;
#endif
        }
        break;

    case QNativeSocketEngine::PathMtuInformation:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
#ifdef IPV6_MTU
            level = IPPROTO_IPV6;
            n = IPV6_MTU;
#endif
        } else {
#ifdef IP_MTU
            level = IPPROTO_IP;
            n = IP_MTU;
#endif
        }
        break;
    }
}

/*! \internal

    Creates and returns a new socket descriptor of type \a socketType
    and \a socketProtocol.  Returns -1 on failure.
*/
bool QNativeSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType,
                                         QAbstractSocket::NetworkLayerProtocol &socketProtocol)
{
#ifndef QT_NO_SCTP
    int protocol = (socketType == QAbstractSocket::SctpSocket) ? IPPROTO_SCTP : 0;
#else
    if (socketType == QAbstractSocket::SctpSocket) {
        setError(QAbstractSocket::UnsupportedSocketOperationError,
                 ProtocolUnsupportedErrorString);
#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::createNewSocket(%d, %d): unsupported protocol",
               socketType, socketProtocol);
#endif
        return false;
    }
    int protocol = 0;
#endif // QT_NO_SCTP
    int domain = (socketProtocol == QAbstractSocket::IPv6Protocol
                  || socketProtocol == QAbstractSocket::AnyIPProtocol) ? AF_INET6 : AF_INET;
    int type = (socketType == QAbstractSocket::UdpSocket) ? SOCK_DGRAM : SOCK_STREAM;

    int socket = qt_safe_socket(domain, type, protocol, O_NONBLOCK);
    if (socket < 0 && socketProtocol == QAbstractSocket::AnyIPProtocol && errno == EAFNOSUPPORT) {
        domain = AF_INET;
        socket = qt_safe_socket(domain, type, protocol, O_NONBLOCK);
        socketProtocol = QAbstractSocket::IPv4Protocol;
    }

    if (socket < 0) {
        int ecopy = errno;
        switch (ecopy) {
        case EPROTONOSUPPORT:
        case EAFNOSUPPORT:
        case EINVAL:
            setError(QAbstractSocket::UnsupportedSocketOperationError, ProtocolUnsupportedErrorString);
            break;
        case ENFILE:
        case EMFILE:
        case ENOBUFS:
        case ENOMEM:
            setError(QAbstractSocket::SocketResourceError, ResourceErrorString);
            break;
        case EACCES:
            setError(QAbstractSocket::SocketAccessError, AccessErrorString);
            break;
        default:
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::createNewSocket(%d, %d) == false (%s)",
               socketType, socketProtocol,
               strerror(ecopy));
#endif

        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::createNewSocket(%d, %d) == true",
           socketType, socketProtocol);
#endif

    socketDescriptor = socket;
    if (socket != -1) {
        this->socketProtocol = socketProtocol;
        this->socketType = socketType;
    }
    return true;
}

/*
    Returns the value of the socket option \a opt.
*/
int QNativeSocketEnginePrivate::option(QNativeSocketEngine::SocketOption opt) const
{
    Q_Q(const QNativeSocketEngine);
    if (!q->isValid())
        return -1;

    // handle non-getsockopt and specific cases first
    switch (opt) {
    case QNativeSocketEngine::BindExclusively:
    case QNativeSocketEngine::NonBlockingSocketOption:
    case QNativeSocketEngine::BroadcastSocketOption:
        return -1;
    case QNativeSocketEngine::MaxStreamsSocketOption: {
#ifndef QT_NO_SCTP
        sctp_initmsg sctpInitMsg;
        QT_SOCKOPTLEN_T sctpInitMsgSize = sizeof(sctpInitMsg);
        if (::getsockopt(socketDescriptor, SOL_SCTP, SCTP_INITMSG, &sctpInitMsg,
                         &sctpInitMsgSize) == 0)
            return int(qMin(sctpInitMsg.sinit_num_ostreams, sctpInitMsg.sinit_max_instreams));
#endif
        return -1;
    }

    case QNativeSocketEngine::PathMtuInformation:
#if defined(IPV6_PATHMTU) && !defined(IPV6_MTU)
        // Prefer IPV6_MTU (handled by convertToLevelAndOption), if available
        // (Linux); fall back to IPV6_PATHMTU otherwise (FreeBSD):
        if (socketProtocol == QAbstractSocket::IPv6Protocol) {
            ip6_mtuinfo mtuinfo;
            QT_SOCKOPTLEN_T len = sizeof(mtuinfo);
            if (::getsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_PATHMTU, &mtuinfo, &len) == 0)
                return int(mtuinfo.ip6m_mtu);
            return -1;
        }
#endif
        break;

    default:
        break;
    }

    int n, level;
    int v = -1;
    QT_SOCKOPTLEN_T len = sizeof(v);

    convertToLevelAndOption(opt, socketProtocol, level, n);
    if (n != -1 && ::getsockopt(socketDescriptor, level, n, (char *) &v, &len) != -1)
        return v;

    return -1;
}


/*
    Sets the socket option \a opt to \a v.
*/
bool QNativeSocketEnginePrivate::setOption(QNativeSocketEngine::SocketOption opt, int v)
{
    Q_Q(QNativeSocketEngine);
    if (!q->isValid())
        return false;

    // handle non-setsockopt and specific cases first
    switch (opt) {
    case QNativeSocketEngine::NonBlockingSocketOption: {
        // Make the socket nonblocking.
#if !defined(Q_OS_VXWORKS)
        int flags = ::fcntl(socketDescriptor, F_GETFL, 0);
        if (flags == -1) {
#ifdef QNATIVESOCKETENGINE_DEBUG
            perror("QNativeSocketEnginePrivate::setOption(): fcntl(F_GETFL) failed");
#endif
            return false;
        }
        if (::fcntl(socketDescriptor, F_SETFL, flags | O_NONBLOCK) == -1) {
#ifdef QNATIVESOCKETENGINE_DEBUG
            perror("QNativeSocketEnginePrivate::setOption(): fcntl(F_SETFL) failed");
#endif
            return false;
        }
#else // Q_OS_VXWORKS
        int onoff = 1;

        if (qt_safe_ioctl(socketDescriptor, FIONBIO, &onoff) < 0) {

#ifdef QNATIVESOCKETENGINE_DEBUG
            perror("QNativeSocketEnginePrivate::setOption(): ioctl(FIONBIO, 1) failed");
#endif
            return false;
        }
#endif // Q_OS_VXWORKS
        return true;
    }
    case QNativeSocketEngine::BindExclusively:
        return true;

    case QNativeSocketEngine::MaxStreamsSocketOption: {
#ifndef QT_NO_SCTP
        sctp_initmsg sctpInitMsg;
        QT_SOCKOPTLEN_T sctpInitMsgSize = sizeof(sctpInitMsg);
        if (::getsockopt(socketDescriptor, SOL_SCTP, SCTP_INITMSG, &sctpInitMsg,
                         &sctpInitMsgSize) == 0) {
            sctpInitMsg.sinit_num_ostreams = sctpInitMsg.sinit_max_instreams = uint16_t(v);
            return ::setsockopt(socketDescriptor, SOL_SCTP, SCTP_INITMSG, &sctpInitMsg,
                                sctpInitMsgSize) == 0;
        }
#endif
        return false;
    }

    default:
        break;
    }

    int n, level;
    convertToLevelAndOption(opt, socketProtocol, level, n);
#if defined(SO_REUSEPORT) && !defined(Q_OS_LINUX)
    if (opt == QNativeSocketEngine::AddressReusable) {
        // on OS X, SO_REUSEADDR isn't sufficient to allow multiple binds to the
        // same port (which is useful for multicast UDP). SO_REUSEPORT is, but
        // we most definitely do not want to use this for TCP. See QTBUG-6305.
        if (socketType == QAbstractSocket::UdpSocket)
            n = SO_REUSEPORT;
    }
#endif

    if (n == -1)
        return false;
    return ::setsockopt(socketDescriptor, level, n, (char *) &v, sizeof(v)) == 0;
}

bool QNativeSocketEnginePrivate::nativeConnect(const QHostAddress &addr, quint16 port)
{
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QNativeSocketEnginePrivate::nativeConnect() " << socketDescriptor;
#endif

    qt_sockaddr aa;
    QT_SOCKLEN_T sockAddrSize;
    setPortAndAddress(port, addr, &aa, &sockAddrSize);

    int connectResult = qt_safe_connect(socketDescriptor, &aa.a, sockAddrSize);
#if defined (QNATIVESOCKETENGINE_DEBUG)
    int ecopy = errno;
#endif
    if (connectResult == -1) {
        switch (errno) {
        case EISCONN:
            socketState = QAbstractSocket::ConnectedState;
            break;
        case ECONNREFUSED:
        case EINVAL:
            setError(QAbstractSocket::ConnectionRefusedError, ConnectionRefusedErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case ETIMEDOUT:
            setError(QAbstractSocket::NetworkError, ConnectionTimeOutErrorString);
            break;
        case EHOSTUNREACH:
            setError(QAbstractSocket::NetworkError, HostUnreachableErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case ENETUNREACH:
            setError(QAbstractSocket::NetworkError, NetworkUnreachableErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case EADDRINUSE:
            setError(QAbstractSocket::NetworkError, AddressInuseErrorString);
            break;
        case EINPROGRESS:
        case EALREADY:
            setError(QAbstractSocket::UnfinishedSocketOperationError, InvalidSocketErrorString);
            socketState = QAbstractSocket::ConnectingState;
            break;
        case EAGAIN:
            setError(QAbstractSocket::UnfinishedSocketOperationError, InvalidSocketErrorString);
            break;
        case EACCES:
        case EPERM:
            setError(QAbstractSocket::SocketAccessError, AccessErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case EAFNOSUPPORT:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            socketState = QAbstractSocket::UnconnectedState;
        default:
            break;
        }

        if (socketState != QAbstractSocket::ConnectedState) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
            qDebug("QNativeSocketEnginePrivate::nativeConnect(%s, %i) == false (%s)",
                   addr.toString().toLatin1().constData(), port,
                   socketState == QAbstractSocket::ConnectingState
                   ? "Connection in progress" : strerror(ecopy));
#endif
            return false;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeConnect(%s, %i) == true",
           addr.toString().toLatin1().constData(), port);
#endif

    socketState = QAbstractSocket::ConnectedState;
    return true;
}

bool QNativeSocketEnginePrivate::nativeBind(const QHostAddress &address, quint16 port)
{
    qt_sockaddr aa;
    QT_SOCKLEN_T sockAddrSize;
    setPortAndAddress(port, address, &aa, &sockAddrSize);

#ifdef IPV6_V6ONLY
    if (aa.a.sa_family == AF_INET6) {
        int ipv6only = 0;
        if (address.protocol() == QAbstractSocket::IPv6Protocol)
            ipv6only = 1;
        //default value of this socket option varies depending on unix variant (or system configuration on BSD), so always set it explicitly
        ::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only) );
    }
#endif

    int bindResult = QT_SOCKET_BIND(socketDescriptor, &aa.a, sockAddrSize);
    if (bindResult < 0 && errno == EAFNOSUPPORT && address.protocol() == QAbstractSocket::AnyIPProtocol) {
        // retry with v4
        aa.a4.sin_family = AF_INET;
        aa.a4.sin_port = htons(port);
        aa.a4.sin_addr.s_addr = htonl(address.toIPv4Address());
        sockAddrSize = sizeof(aa.a4);
        bindResult = QT_SOCKET_BIND(socketDescriptor, &aa.a, sockAddrSize);
    }

    if (bindResult < 0) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
        int ecopy = errno;
#endif
        switch(errno) {
        case EADDRINUSE:
            setError(QAbstractSocket::AddressInUseError, AddressInuseErrorString);
            break;
        case EACCES:
            setError(QAbstractSocket::SocketAccessError, AddressProtectedErrorString);
            break;
        case EINVAL:
            setError(QAbstractSocket::UnsupportedSocketOperationError, OperationUnsupportedErrorString);
            break;
        case EADDRNOTAVAIL:
            setError(QAbstractSocket::SocketAddressNotAvailableError, AddressNotAvailableErrorString);
            break;
        default:
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::nativeBind(%s, %i) == false (%s)",
               address.toString().toLatin1().constData(), port, strerror(ecopy));
#endif

        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeBind(%s, %i) == true",
           address.toString().toLatin1().constData(), port);
#endif
    socketState = QAbstractSocket::BoundState;
    return true;
}

bool QNativeSocketEnginePrivate::nativeListen(int backlog)
{
    if (qt_safe_listen(socketDescriptor, backlog) < 0) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
        int ecopy = errno;
#endif
        switch (errno) {
        case EADDRINUSE:
            setError(QAbstractSocket::AddressInUseError,
                     PortInuseErrorString);
            break;
        default:
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::nativeListen(%i) == false (%s)",
               backlog, strerror(ecopy));
#endif
        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeListen(%i) == true", backlog);
#endif

    socketState = QAbstractSocket::ListeningState;
    return true;
}

int QNativeSocketEnginePrivate::nativeAccept()
{
    int acceptedDescriptor = qt_safe_accept(socketDescriptor, 0, 0);
    if (acceptedDescriptor == -1) {
        switch (errno) {
        case EBADF:
        case EOPNOTSUPP:
            setError(QAbstractSocket::UnsupportedSocketOperationError, InvalidSocketErrorString);
            break;
        case ECONNABORTED:
            setError(QAbstractSocket::NetworkError, RemoteHostClosedErrorString);
            break;
        case EFAULT:
        case ENOTSOCK:
            setError(QAbstractSocket::SocketResourceError, NotSocketErrorString);
            break;
        case EPROTONOSUPPORT:
#if !defined(Q_OS_OPENBSD)
        case EPROTO:
#endif
        case EAFNOSUPPORT:
        case EINVAL:
            setError(QAbstractSocket::UnsupportedSocketOperationError, ProtocolUnsupportedErrorString);
            break;
        case ENFILE:
        case EMFILE:
        case ENOBUFS:
        case ENOMEM:
            setError(QAbstractSocket::SocketResourceError, ResourceErrorString);
            break;
        case EACCES:
        case EPERM:
            setError(QAbstractSocket::SocketAccessError, AccessErrorString);
            break;
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
        case EAGAIN:
            setError(QAbstractSocket::TemporaryError, TemporaryErrorString);
            break;
        default:
            setError(QAbstractSocket::UnknownSocketError, UnknownSocketErrorString);
            break;
        }
    }

    return acceptedDescriptor;
}

#ifndef QT_NO_NETWORKINTERFACE

static bool multicastMembershipHelper(QNativeSocketEnginePrivate *d,
                                      int how6,
                                      int how4,
                                      const QHostAddress &groupAddress,
                                      const QNetworkInterface &interface)
{
    int level = 0;
    int sockOpt = 0;
    void *sockArg;
    int sockArgSize;

    ip_mreq mreq4;
    ipv6_mreq mreq6;

    if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol) {
        level = IPPROTO_IPV6;
        sockOpt = how6;
        sockArg = &mreq6;
        sockArgSize = sizeof(mreq6);
        memset(&mreq6, 0, sizeof(mreq6));
        Q_IPV6ADDR ip6 = groupAddress.toIPv6Address();
        memcpy(&mreq6.ipv6mr_multiaddr, &ip6, sizeof(ip6));
        mreq6.ipv6mr_interface = interface.index();
    } else if (groupAddress.protocol() == QAbstractSocket::IPv4Protocol) {
        level = IPPROTO_IP;
        sockOpt = how4;
        sockArg = &mreq4;
        sockArgSize = sizeof(mreq4);
        memset(&mreq4, 0, sizeof(mreq4));
        mreq4.imr_multiaddr.s_addr = htonl(groupAddress.toIPv4Address());

        if (interface.isValid()) {
            const QList<QNetworkAddressEntry> addressEntries = interface.addressEntries();
            bool found = false;
            for (const QNetworkAddressEntry &entry : addressEntries) {
                const QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                    mreq4.imr_interface.s_addr = htonl(ip.toIPv4Address());
                    found = true;
                    break;
                }
            }
            if (!found) {
                d->setError(QAbstractSocket::NetworkError,
                            QNativeSocketEnginePrivate::NetworkUnreachableErrorString);
                return false;
            }
        } else {
            mreq4.imr_interface.s_addr = INADDR_ANY;
        }
    } else {
        // unreachable
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                    QNativeSocketEnginePrivate::ProtocolUnsupportedErrorString);
        return false;
    }

    int res = setsockopt(d->socketDescriptor, level, sockOpt, sockArg, sockArgSize);
    if (res == -1) {
        switch (errno) {
        case ENOPROTOOPT:
            d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                        QNativeSocketEnginePrivate::OperationUnsupportedErrorString);
            break;
        case EADDRNOTAVAIL:
            d->setError(QAbstractSocket::SocketAddressNotAvailableError,
                        QNativeSocketEnginePrivate::AddressNotAvailableErrorString);
            break;
        default:
            d->setError(QAbstractSocket::UnknownSocketError,
                        QNativeSocketEnginePrivate::UnknownSocketErrorString);
            break;
        }
        return false;
    }
    return true;
}

bool QNativeSocketEnginePrivate::nativeJoinMulticastGroup(const QHostAddress &groupAddress,
                                                          const QNetworkInterface &interface)
{
    return multicastMembershipHelper(this,
                                     IPV6_JOIN_GROUP,
                                     IP_ADD_MEMBERSHIP,
                                     groupAddress,
                                     interface);
}

bool QNativeSocketEnginePrivate::nativeLeaveMulticastGroup(const QHostAddress &groupAddress,
                                                           const QNetworkInterface &interface)
{
    return multicastMembershipHelper(this,
                                     IPV6_LEAVE_GROUP,
                                     IP_DROP_MEMBERSHIP,
                                     groupAddress,
                                     interface);
}

QNetworkInterface QNativeSocketEnginePrivate::nativeMulticastInterface() const
{
    if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
        uint v;
        QT_SOCKOPTLEN_T sizeofv = sizeof(v);
        if (::getsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF, &v, &sizeofv) == -1)
            return QNetworkInterface();
        return QNetworkInterface::interfaceFromIndex(v);
    }

    struct in_addr v = { 0 };
    QT_SOCKOPTLEN_T sizeofv = sizeof(v);
    if (::getsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, &v, &sizeofv) == -1)
        return QNetworkInterface();
    if (v.s_addr != 0 && sizeofv >= QT_SOCKOPTLEN_T(sizeof(v))) {
        QHostAddress ipv4(ntohl(v.s_addr));
        QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
        for (int i = 0; i < ifaces.count(); ++i) {
            const QNetworkInterface &iface = ifaces.at(i);
            QList<QNetworkAddressEntry> entries = iface.addressEntries();
            for (int j = 0; j < entries.count(); ++j) {
                const QNetworkAddressEntry &entry = entries.at(j);
                if (entry.ip() == ipv4)
                    return iface;
            }
        }
    }
    return QNetworkInterface();
}

bool QNativeSocketEnginePrivate::nativeSetMulticastInterface(const QNetworkInterface &iface)
{
    if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
        uint v = iface.index();
        return (::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF, &v, sizeof(v)) != -1);
    }

    struct in_addr v;
    if (iface.isValid()) {
        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (int i = 0; i < entries.count(); ++i) {
            const QNetworkAddressEntry &entry = entries.at(i);
            const QHostAddress &ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                v.s_addr = htonl(ip.toIPv4Address());
                int r = ::setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, &v, sizeof(v));
                if (r != -1)
                    return true;
            }
        }
        return false;
    }

    v.s_addr = INADDR_ANY;
    return (::setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, &v, sizeof(v)) != -1);
}

#endif // QT_NO_NETWORKINTERFACE

qint64 QNativeSocketEnginePrivate::nativeBytesAvailable() const
{
    int nbytes = 0;
    // gives shorter than true amounts on Unix domain sockets.
    qint64 available = -1;

#if defined (SO_NREAD)
    if (socketType == QAbstractSocket::UdpSocket) {
        socklen_t sz = sizeof nbytes;
        if (!::getsockopt(socketDescriptor, SOL_SOCKET, SO_NREAD, &nbytes, &sz))
            available = nbytes;
    }
#endif

    if (available == -1 && qt_safe_ioctl(socketDescriptor, FIONREAD, (char *) &nbytes) >= 0)
        available = nbytes;

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeBytesAvailable() == %lli", available);
#endif
    return available > 0 ? available : 0;
}

bool QNativeSocketEnginePrivate::nativeHasPendingDatagrams() const
{
    // Peek 1 bytes into the next message.
    ssize_t readBytes;
    char c;
    EINTR_LOOP(readBytes, ::recv(socketDescriptor, &c, 1, MSG_PEEK));

    // If there's no error, or if our buffer was too small, there must be a
    // pending datagram.
    bool result = (readBytes != -1) || errno == EMSGSIZE;

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeHasPendingDatagrams() == %s",
           result ? "true" : "false");
#endif
    return result;
}

qint64 QNativeSocketEnginePrivate::nativePendingDatagramSize() const
{
    ssize_t recvResult = -1;
#ifdef Q_OS_LINUX
    // Linux can return the actual datagram size if we use MSG_TRUNC
    char c;
    EINTR_LOOP(recvResult, ::recv(socketDescriptor, &c, 1, MSG_PEEK | MSG_TRUNC));
#elif defined(SO_NREAD)
    // macOS can return the actual datagram size if we use SO_NREAD
    int value;
    socklen_t valuelen = sizeof(value);
    recvResult = getsockopt(socketDescriptor, SOL_SOCKET, SO_NREAD, &value, &valuelen);
    if (recvResult != -1)
        recvResult = value;
#else
    // We need to grow the buffer to fit the entire datagram.
    // We start at 1500 bytes (the MTU for Ethernet V2), which should catch
    // almost all uses (effective MTU for UDP under IPv4 is 1468), except
    // for localhost datagrams and those reassembled by the IP layer.
    char udpMessagePeekBuffer[1500];
    struct msghdr msg;
    struct iovec vec;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    vec.iov_base = udpMessagePeekBuffer;
    vec.iov_len = sizeof(udpMessagePeekBuffer);

    for (;;) {
        // the data written to udpMessagePeekBuffer is discarded, so
        // this function is still reentrant although it might not look
        // so.
        recvResult = ::recvmsg(socketDescriptor, &msg, MSG_PEEK);
        if (recvResult == -1 && errno == EINTR)
            continue;

        // was the result truncated?
        if ((msg.msg_flags & MSG_TRUNC) == 0)
            break;

        // grow by 16 times
        msg.msg_iovlen *= 16;
        if (msg.msg_iov != &vec)
            delete[] msg.msg_iov;
        msg.msg_iov = new struct iovec[msg.msg_iovlen];
        std::fill_n(msg.msg_iov, msg.msg_iovlen, vec);
    }

    if (msg.msg_iov != &vec)
        delete[] msg.msg_iov;
#endif

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativePendingDatagramSize() == %zd", recvResult);
#endif

    return qint64(recvResult);
}

qint64 QNativeSocketEnginePrivate::nativeReceiveDatagram(char *data, qint64 maxSize, QIpPacketHeader *header,
                                                         QAbstractSocketEngine::PacketHeaderOptions options)
{
    // we use quintptr to force the alignment
    quintptr cbuf[(CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int))
#if !defined(IP_PKTINFO) && defined(IP_RECVIF) && defined(Q_OS_BSD4)
                   + CMSG_SPACE(sizeof(sockaddr_dl))
#endif
#ifndef QT_NO_SCTP
                   + CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))
#endif
                   + sizeof(quintptr) - 1) / sizeof(quintptr)];

    struct msghdr msg;
    struct iovec vec;
    qt_sockaddr aa;
    char c;
    memset(&msg, 0, sizeof(msg));
    memset(&aa, 0, sizeof(aa));

    // we need to receive at least one byte, even if our user isn't interested in it
    vec.iov_base = maxSize ? data : &c;
    vec.iov_len = maxSize ? maxSize : 1;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    if (options & QAbstractSocketEngine::WantDatagramSender) {
        msg.msg_name = &aa;
        msg.msg_namelen = sizeof(aa);
    }
    if (options & (QAbstractSocketEngine::WantDatagramHopLimit | QAbstractSocketEngine::WantDatagramDestination
                   | QAbstractSocketEngine::WantStreamNumber)) {
        msg.msg_control = cbuf;
        msg.msg_controllen = sizeof(cbuf);
    }

    ssize_t recvResult = 0;
    do {
        recvResult = ::recvmsg(socketDescriptor, &msg, 0);
    } while (recvResult == -1 && errno == EINTR);

    if (recvResult == -1) {
        switch (errno) {
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
        case EAGAIN:
            // No datagram was available for reading
            recvResult = -2;
            break;
        case ECONNREFUSED:
            setError(QAbstractSocket::ConnectionRefusedError, ConnectionRefusedErrorString);
            break;
        default:
            setError(QAbstractSocket::NetworkError, ReceiveDatagramErrorString);
        }
        if (header)
            header->clear();
    } else if (options != QAbstractSocketEngine::WantNone) {
        Q_ASSERT(header);
        qt_socket_getPortAndAddress(&aa, &header->senderPort, &header->senderAddress);
        header->destinationPort = localPort;
        header->endOfRecord = (msg.msg_flags & MSG_EOR) != 0;

        // parse the ancillary data
        struct cmsghdr *cmsgptr;
        for (cmsgptr = CMSG_FIRSTHDR(&msg); cmsgptr != NULL;
             cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {
            if (cmsgptr->cmsg_level == IPPROTO_IPV6 && cmsgptr->cmsg_type == IPV6_PKTINFO
                    && cmsgptr->cmsg_len >= CMSG_LEN(sizeof(in6_pktinfo))) {
                in6_pktinfo *info = reinterpret_cast<in6_pktinfo *>(CMSG_DATA(cmsgptr));

                header->destinationAddress.setAddress(reinterpret_cast<quint8 *>(&info->ipi6_addr));
                header->ifindex = info->ipi6_ifindex;
                if (header->ifindex)
                    header->destinationAddress.setScopeId(QString::number(info->ipi6_ifindex));
            }

#ifdef IP_PKTINFO
            if (cmsgptr->cmsg_level == IPPROTO_IP && cmsgptr->cmsg_type == IP_PKTINFO
                    && cmsgptr->cmsg_len >= CMSG_LEN(sizeof(in_pktinfo))) {
                in_pktinfo *info = reinterpret_cast<in_pktinfo *>(CMSG_DATA(cmsgptr));

                header->destinationAddress.setAddress(ntohl(info->ipi_addr.s_addr));
                header->ifindex = info->ipi_ifindex;
            }
#else
#  ifdef IP_RECVDSTADDR
            if (cmsgptr->cmsg_level == IPPROTO_IP && cmsgptr->cmsg_type == IP_RECVDSTADDR
                    && cmsgptr->cmsg_len >= CMSG_LEN(sizeof(in_addr))) {
                in_addr *addr = reinterpret_cast<in_addr *>(CMSG_DATA(cmsgptr));

                header->destinationAddress.setAddress(ntohl(addr->s_addr));
            }
#  endif
#  if defined(IP_RECVIF) && defined(Q_OS_BSD4)
            if (cmsgptr->cmsg_level == IPPROTO_IP && cmsgptr->cmsg_type == IP_RECVIF
                    && cmsgptr->cmsg_len >= CMSG_LEN(sizeof(sockaddr_dl))) {
                sockaddr_dl *sdl = reinterpret_cast<sockaddr_dl *>(CMSG_DATA(cmsgptr));
                header->ifindex = sdl->sdl_index;
            }
#  endif
#endif

            if (cmsgptr->cmsg_len == CMSG_LEN(sizeof(int))
                    && ((cmsgptr->cmsg_level == IPPROTO_IPV6 && cmsgptr->cmsg_type == IPV6_HOPLIMIT)
                        || (cmsgptr->cmsg_level == IPPROTO_IP && cmsgptr->cmsg_type == IP_TTL))) {
                Q_STATIC_ASSERT(sizeof(header->hopLimit) == sizeof(int));
                memcpy(&header->hopLimit, CMSG_DATA(cmsgptr), sizeof(header->hopLimit));
            }

#ifndef QT_NO_SCTP
            if (cmsgptr->cmsg_level == IPPROTO_SCTP && cmsgptr->cmsg_type == SCTP_SNDRCV
                && cmsgptr->cmsg_len >= CMSG_LEN(sizeof(sctp_sndrcvinfo))) {
                sctp_sndrcvinfo *rcvInfo = reinterpret_cast<sctp_sndrcvinfo *>(CMSG_DATA(cmsgptr));

                header->streamNumber = int(rcvInfo->sinfo_stream);
            }
#endif
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeReceiveDatagram(%p \"%s\", %lli, %s, %i) == %lli",
           data, qt_prettyDebug(data, qMin(recvResult, ssize_t(16)), recvResult).data(), maxSize,
           (recvResult != -1 && options != QAbstractSocketEngine::WantNone)
           ? header->senderAddress.toString().toLatin1().constData() : "(unknown)",
           (recvResult != -1 && options != QAbstractSocketEngine::WantNone)
           ? header->senderPort : 0, (qint64) recvResult);
#endif

    return qint64((maxSize || recvResult < 0) ? recvResult : Q_INT64_C(0));
}

qint64 QNativeSocketEnginePrivate::nativeSendDatagram(const char *data, qint64 len, const QIpPacketHeader &header)
{
    // we use quintptr to force the alignment
    quintptr cbuf[(CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int))
#ifndef QT_NO_SCTP
                   + CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))
#endif
                   + sizeof(quintptr) - 1) / sizeof(quintptr)];

    struct cmsghdr *cmsgptr = reinterpret_cast<struct cmsghdr *>(cbuf);
    struct msghdr msg;
    struct iovec vec;
    qt_sockaddr aa;

    memset(&msg, 0, sizeof(msg));
    memset(&aa, 0, sizeof(aa));
    vec.iov_base = const_cast<char *>(data);
    vec.iov_len = len;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = &cbuf;

    if (header.destinationPort != 0) {
        msg.msg_name = &aa.a;
        setPortAndAddress(header.destinationPort, header.destinationAddress,
                          &aa, &msg.msg_namelen);
    }

    if (msg.msg_namelen == sizeof(aa.a6)) {
        if (header.hopLimit != -1) {
            msg.msg_controllen += CMSG_SPACE(sizeof(int));
            cmsgptr->cmsg_len = CMSG_LEN(sizeof(int));
            cmsgptr->cmsg_level = IPPROTO_IPV6;
            cmsgptr->cmsg_type = IPV6_HOPLIMIT;
            memcpy(CMSG_DATA(cmsgptr), &header.hopLimit, sizeof(int));
            cmsgptr = reinterpret_cast<cmsghdr *>(reinterpret_cast<char *>(cmsgptr) + CMSG_SPACE(sizeof(int)));
        }
        if (header.ifindex != 0 || !header.senderAddress.isNull()) {
            struct in6_pktinfo *data = reinterpret_cast<in6_pktinfo *>(CMSG_DATA(cmsgptr));
            memset(data, 0, sizeof(*data));
            msg.msg_controllen += CMSG_SPACE(sizeof(*data));
            cmsgptr->cmsg_len = CMSG_LEN(sizeof(*data));
            cmsgptr->cmsg_level = IPPROTO_IPV6;
            cmsgptr->cmsg_type = IPV6_PKTINFO;
            data->ipi6_ifindex = header.ifindex;

            QIPv6Address tmp = header.senderAddress.toIPv6Address();
            memcpy(&data->ipi6_addr, &tmp, sizeof(tmp));
            cmsgptr = reinterpret_cast<cmsghdr *>(reinterpret_cast<char *>(cmsgptr) + CMSG_SPACE(sizeof(*data)));
        }
    } else {
        if (header.hopLimit != -1) {
            msg.msg_controllen += CMSG_SPACE(sizeof(int));
            cmsgptr->cmsg_len = CMSG_LEN(sizeof(int));
            cmsgptr->cmsg_level = IPPROTO_IP;
            cmsgptr->cmsg_type = IP_TTL;
            memcpy(CMSG_DATA(cmsgptr), &header.hopLimit, sizeof(int));
            cmsgptr = reinterpret_cast<cmsghdr *>(reinterpret_cast<char *>(cmsgptr) + CMSG_SPACE(sizeof(int)));
        }

#if defined(IP_PKTINFO) || defined(IP_SENDSRCADDR)
        if (header.ifindex != 0 || !header.senderAddress.isNull()) {
#  ifdef IP_PKTINFO
            struct in_pktinfo *data = reinterpret_cast<in_pktinfo *>(CMSG_DATA(cmsgptr));
            memset(data, 0, sizeof(*data));
            cmsgptr->cmsg_type = IP_PKTINFO;
            data->ipi_ifindex = header.ifindex;
            data->ipi_addr.s_addr = htonl(header.senderAddress.toIPv4Address());
#  elif defined(IP_SENDSRCADDR)
            struct in_addr *data = reinterpret_cast<in_addr *>(CMSG_DATA(cmsgptr));
            cmsgptr->cmsg_type = IP_SENDSRCADDR;
            data->s_addr = htonl(header.senderAddress.toIPv4Address());
#  endif
            cmsgptr->cmsg_level = IPPROTO_IP;
            msg.msg_controllen += CMSG_SPACE(sizeof(*data));
            cmsgptr->cmsg_len = CMSG_LEN(sizeof(*data));
            cmsgptr = reinterpret_cast<cmsghdr *>(reinterpret_cast<char *>(cmsgptr) + CMSG_SPACE(sizeof(*data)));
        }
#endif
    }

#ifndef QT_NO_SCTP
    if (header.streamNumber != -1) {
        struct sctp_sndrcvinfo *data = reinterpret_cast<sctp_sndrcvinfo *>(CMSG_DATA(cmsgptr));
        memset(data, 0, sizeof(*data));
        msg.msg_controllen += CMSG_SPACE(sizeof(sctp_sndrcvinfo));
        cmsgptr->cmsg_len = CMSG_LEN(sizeof(sctp_sndrcvinfo));
        cmsgptr->cmsg_level = IPPROTO_SCTP;
        cmsgptr->cmsg_type =  SCTP_SNDRCV;
        data->sinfo_stream = uint16_t(header.streamNumber);
        cmsgptr = reinterpret_cast<cmsghdr *>(reinterpret_cast<char *>(cmsgptr) + CMSG_SPACE(sizeof(*data)));
    }
#endif

    if (msg.msg_controllen == 0)
        msg.msg_control = 0;
    ssize_t sentBytes = qt_safe_sendmsg(socketDescriptor, &msg, 0);

    if (sentBytes < 0) {
        switch (errno) {
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
        case EAGAIN:
            sentBytes = -2;
            break;
        case EMSGSIZE:
            setError(QAbstractSocket::DatagramTooLargeError, DatagramTooLargeErrorString);
            break;
        case ECONNRESET:
            setError(QAbstractSocket::RemoteHostClosedError, RemoteHostClosedErrorString);
            break;
        default:
            setError(QAbstractSocket::NetworkError, SendDatagramErrorString);
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEngine::sendDatagram(%p \"%s\", %lli, \"%s\", %i) == %lli", data,
           qt_prettyDebug(data, qMin<int>(len, 16), len).data(), len,
           header.destinationAddress.toString().toLatin1().constData(),
           header.destinationPort, (qint64) sentBytes);
#endif

    return qint64(sentBytes);
}

bool QNativeSocketEnginePrivate::fetchConnectionParameters()
{
    localPort = 0;
    localAddress.clear();
    peerPort = 0;
    peerAddress.clear();
    inboundStreamCount = outboundStreamCount = 0;

    if (socketDescriptor == -1)
        return false;

    qt_sockaddr sa;
    QT_SOCKLEN_T sockAddrSize = sizeof(sa);

    // Determine local address
    memset(&sa, 0, sizeof(sa));
    if (::getsockname(socketDescriptor, &sa.a, &sockAddrSize) == 0) {
        qt_socket_getPortAndAddress(&sa, &localPort, &localAddress);

        // Determine protocol family
        switch (sa.a.sa_family) {
        case AF_INET:
            socketProtocol = QAbstractSocket::IPv4Protocol;
            break;
        case AF_INET6:
            socketProtocol = QAbstractSocket::IPv6Protocol;
            break;
        default:
            socketProtocol = QAbstractSocket::UnknownNetworkLayerProtocol;
            break;
        }

    } else if (errno == EBADF) {
        setError(QAbstractSocket::UnsupportedSocketOperationError, InvalidSocketErrorString);
        return false;
    }

#if defined (IPV6_V6ONLY)
    // determine if local address is dual mode
    // On linux, these are returned as "::" (==AnyIPv6)
    // On OSX, these are returned as "::FFFF:0.0.0.0" (==AnyIPv4)
    // in either case, the IPV6_V6ONLY option is cleared
    int ipv6only = 0;
    socklen_t optlen = sizeof(ipv6only);
    if (socketProtocol == QAbstractSocket::IPv6Protocol
        && (localAddress == QHostAddress::AnyIPv4 || localAddress == QHostAddress::AnyIPv6)
        && !getsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, &optlen )) {
            if (optlen != sizeof(ipv6only))
                qWarning("unexpected size of IPV6_V6ONLY socket option");
            if (!ipv6only) {
                socketProtocol = QAbstractSocket::AnyIPProtocol;
                localAddress = QHostAddress::Any;
            }
    }
#endif

    // Determine the remote address
    bool connected = ::getpeername(socketDescriptor, &sa.a, &sockAddrSize) == 0;
    if (connected) {
        qt_socket_getPortAndAddress(&sa, &peerPort, &peerAddress);
        inboundStreamCount = outboundStreamCount = 1;
    }

    // Determine the socket type (UDP/TCP/SCTP)
    int value = 0;
    QT_SOCKOPTLEN_T valueSize = sizeof(int);
    if (::getsockopt(socketDescriptor, SOL_SOCKET, SO_TYPE, &value, &valueSize) == 0) {
        if (value == SOCK_STREAM) {
#ifndef QT_NO_SCTP
            if (option(QNativeSocketEngine::MaxStreamsSocketOption) != -1) {
                socketType = QAbstractSocket::SctpSocket;
                if (connected) {
                    sctp_status sctpStatus;
                    QT_SOCKOPTLEN_T sctpStatusSize = sizeof(sctpStatus);
                    sctp_event_subscribe sctpEvents;

                    memset(&sctpEvents, 0, sizeof(sctpEvents));
                    sctpEvents.sctp_data_io_event = 1;
                    if (::getsockopt(socketDescriptor, SOL_SCTP, SCTP_STATUS, &sctpStatus,
                                     &sctpStatusSize) == 0 &&
                        ::setsockopt(socketDescriptor, SOL_SCTP, SCTP_EVENTS, &sctpEvents,
                                     sizeof(sctpEvents)) == 0) {
                        inboundStreamCount = int(sctpStatus.sstat_instrms);
                        outboundStreamCount = int(sctpStatus.sstat_outstrms);
                    } else {
                        setError(QAbstractSocket::UnsupportedSocketOperationError,
                                 InvalidSocketErrorString);
                        return false;
                    }
                }
            } else {
                socketType = QAbstractSocket::TcpSocket;
            }
#else
                socketType = QAbstractSocket::TcpSocket;
#endif
        } else {
            if (value == SOCK_DGRAM)
                socketType = QAbstractSocket::UdpSocket;
            else
                socketType = QAbstractSocket::UnknownSocketType;
        }
    }
#if defined (QNATIVESOCKETENGINE_DEBUG)
    QString socketProtocolStr = QStringLiteral("UnknownProtocol");
    if (socketProtocol == QAbstractSocket::IPv4Protocol) socketProtocolStr = QStringLiteral("IPv4Protocol");
    else if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) socketProtocolStr = QStringLiteral("IPv6Protocol");

    QString socketTypeStr = QStringLiteral("UnknownSocketType");
    if (socketType == QAbstractSocket::TcpSocket) socketTypeStr = QStringLiteral("TcpSocket");
    else if (socketType == QAbstractSocket::UdpSocket) socketTypeStr = QStringLiteral("UdpSocket");
    else if (socketType == QAbstractSocket::SctpSocket) socketTypeStr = QStringLiteral("SctpSocket");

    qDebug("QNativeSocketEnginePrivate::fetchConnectionParameters() local == %s:%i,"
           " peer == %s:%i, socket == %s - %s, inboundStreamCount == %i, outboundStreamCount == %i",
           localAddress.toString().toLatin1().constData(), localPort,
           peerAddress.toString().toLatin1().constData(), peerPort,socketTypeStr.toLatin1().constData(),
           socketProtocolStr.toLatin1().constData(), inboundStreamCount, outboundStreamCount);
#endif
    return true;
}

void QNativeSocketEnginePrivate::nativeClose()
{
#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEngine::nativeClose()");
#endif

    qt_safe_close(socketDescriptor);
}

qint64 QNativeSocketEnginePrivate::nativeWrite(const char *data, qint64 len)
{
    Q_Q(QNativeSocketEngine);

    ssize_t writtenBytes;
    writtenBytes = qt_safe_write_nosignal(socketDescriptor, data, len);

    if (writtenBytes < 0) {
        switch (errno) {
        case EPIPE:
        case ECONNRESET:
            writtenBytes = -1;
            setError(QAbstractSocket::RemoteHostClosedError, RemoteHostClosedErrorString);
            q->close();
            break;
        case EAGAIN:
            writtenBytes = 0;
            break;
        case EMSGSIZE:
            setError(QAbstractSocket::DatagramTooLargeError, DatagramTooLargeErrorString);
            break;
        default:
            break;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeWrite(%p \"%s\", %llu) == %i",
           data, qt_prettyDebug(data, qMin((int) len, 16),
                                (int) len).data(), len, (int) writtenBytes);
#endif

    return qint64(writtenBytes);
}
/*
*/
qint64 QNativeSocketEnginePrivate::nativeRead(char *data, qint64 maxSize)
{
    Q_Q(QNativeSocketEngine);
    if (!q->isValid()) {
        qWarning("QNativeSocketEngine::nativeRead: Invalid socket");
        return -1;
    }

    ssize_t r = 0;
    r = qt_safe_read(socketDescriptor, data, maxSize);

    if (r < 0) {
        r = -1;
        switch (errno) {
#if EWOULDBLOCK-0 && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
        case EAGAIN:
            // No data was available for reading
            r = -2;
            break;
        case EBADF:
        case EINVAL:
        case EIO:
            //error string is now set in read(), not here in nativeRead()
            break;
        case ECONNRESET:
#if defined(Q_OS_VXWORKS)
        case ESHUTDOWN:
#endif
            r = 0;
            break;
        default:
            break;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeRead(%p \"%s\", %llu) == %zd",
           data, qt_prettyDebug(data, qMin(r, ssize_t(16)), r).data(),
           maxSize, r);
#endif

    return qint64(r);
}

int QNativeSocketEnginePrivate::nativeSelect(int timeout, bool selectForRead) const
{
    bool dummy;
    return nativeSelect(timeout, selectForRead, !selectForRead, &dummy, &dummy);
}

int QNativeSocketEnginePrivate::nativeSelect(int timeout, bool checkRead, bool checkWrite,
                       bool *selectForRead, bool *selectForWrite) const
{
    pollfd pfd = qt_make_pollfd(socketDescriptor, 0);

    if (checkRead)
        pfd.events |= POLLIN;

    if (checkWrite)
        pfd.events |= POLLOUT;

    const int ret = qt_poll_msecs(&pfd, 1, timeout);

    if (ret <= 0)
        return ret;

    if (pfd.revents & POLLNVAL) {
        errno = EBADF;
        return -1;
    }

    static const short read_flags = POLLIN | POLLHUP | POLLERR;
    static const short write_flags = POLLOUT | POLLERR;

    *selectForRead = ((pfd.revents & read_flags) != 0);
    *selectForWrite = ((pfd.revents & write_flags) != 0);

    return ret;
}

QT_END_NAMESPACE
