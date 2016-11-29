/****************************************************************************
**
** Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
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

//#define QSCTPSERVER_DEBUG

/*!
    \class QSctpServer
    \since 5.8

    \brief The QSctpServer class provides an SCTP-based server.

    \ingroup network
    \inmodule QtNetwork

    SCTP (Stream Control Transmission Protocol) is a transport layer
    protocol serving in a similar role as the popular protocols TCP
    and UDP. Like UDP, SCTP is message-oriented, but it ensures reliable,
    in-sequence transport of messages with congestion control like
    TCP. See the QSctpSocket documentation for more protocol details.

    QSctpServer is a convenience subclass of QTcpServer that allows
    you to accept incoming SCTP socket connections either in TCP
    emulation or in datagram mode.

    The most common way to use QSctpServer is to construct an object
    and set the maximum number of channels that the server is
    prepared to support, by calling setMaximumChannelCount(). You can set
    the TCP emulation mode by passing a negative argument in this
    call. Also, a special value of 0 (the default) indicates to use
    the peer's value as the actual number of channels. The new incoming
    connection inherits this number from the server socket descriptor
    and adjusts it according to the remote endpoint settings.

    In TCP emulation mode, accepted clients use a single continuous
    byte stream for data transmission, and QSctpServer acts like a
    plain QTcpServer. Call nextPendingConnection() to accept the
    pending connection as a connected QTcpSocket. The function returns
    a pointer to a QTcpSocket in QAbstractSocket::ConnectedState that
    you can use for communicating with the client. This mode gives
    access only to basic SCTP protocol features. The socket transmits SCTP
    packets over IP at system level and interacts through the
    QTcpSocket interface with the application.

    In contrast, datagram mode is message-oriented and provides a
    complete simultaneous transmission of multiple data streams
    between endpoints. Call nextPendingDatagramConnection() to accept
    the pending datagram-mode connection as a connected QSctpSocket.

    \note This feature is not supported on the Windows platform.

    \sa QTcpServer, QSctpSocket, QAbstractSocket
*/

#include "qsctpserver.h"
#include "qsctpserver_p.h"

#include "qsctpsocket.h"
#include "qabstractsocketengine_p.h"

#ifdef QSCTPSERVER_DEBUG
#include <qdebug.h>
#endif

QT_BEGIN_NAMESPACE

/*! \internal
*/
QSctpServerPrivate::QSctpServerPrivate()
    : maximumChannelCount(0)
{
}

/*! \internal
*/
QSctpServerPrivate::~QSctpServerPrivate()
{
}

/*! \internal
*/
void QSctpServerPrivate::configureCreatedSocket()
{
    QTcpServerPrivate::configureCreatedSocket();
    if (socketEngine)
        socketEngine->setOption(QAbstractSocketEngine::MaxStreamsSocketOption,
                                maximumChannelCount == -1 ? 1 : maximumChannelCount);
}

/*!
    Constructs a QSctpServer object.

    Sets the datagram operation mode. The \a parent argument is passed
    to QObject's constructor.

    \sa setMaximumChannelCount(), listen(), setSocketDescriptor()
*/
QSctpServer::QSctpServer(QObject *parent)
    : QTcpServer(QAbstractSocket::SctpSocket, *new QSctpServerPrivate, parent)
{
#if defined(QSCTPSERVER_DEBUG)
    qDebug("QSctpServer::QSctpServer()");
#endif
}

/*!
    Destroys the QSctpServer object. If the server is listening for
    connections, the socket is automatically closed.

    \sa close()
*/
QSctpServer::~QSctpServer()
{
#if defined(QSCTPSERVER_DEBUG)
    qDebug("QSctpServer::~QSctpServer()");
#endif
}

/*!
    Sets the maximum number of channels that the server is prepared to
    support in datagram mode, to \a count. If \a count is 0, endpoint
    maximum number of channels value would be used. Negative \a count
    sets a TCP emulation mode.

    Call this method only when QSctpServer is in UnconnectedState.

    \sa maximumChannelCount(), QSctpSocket
*/
void QSctpServer::setMaximumChannelCount(int count)
{
    Q_D(QSctpServer);
    if (d->state != QAbstractSocket::UnconnectedState) {
        qWarning("QSctpServer::setMaximumChannelCount() is only allowed in UnconnectedState");
        return;
    }
#if defined(QSCTPSERVER_DEBUG)
    qDebug("QSctpServer::setMaximumChannelCount(%i)", count);
#endif
    d->maximumChannelCount = count;
}

/*!
    Returns the maximum number of channels that the accepted sockets are
    able to support.

    A value of 0 (the default) means that the number of connection
    channels would be set by the remote endpoint.

    Returns -1, if QSctpServer running in TCP emulation mode.

    \sa setMaximumChannelCount()
*/
int QSctpServer::maximumChannelCount() const
{
    return d_func()->maximumChannelCount;
}

/*! \reimp
*/
void QSctpServer::incomingConnection(qintptr socketDescriptor)
{
#if defined (QSCTPSERVER_DEBUG)
    qDebug("QSctpServer::incomingConnection(%i)", socketDescriptor);
#endif

    QSctpSocket *socket = new QSctpSocket(this);
    socket->setMaximumChannelCount(d_func()->maximumChannelCount);
    socket->setSocketDescriptor(socketDescriptor);
    addPendingConnection(socket);
}

/*!
    Returns the next pending datagram-mode connection as a connected
    QSctpSocket object.

    Datagram-mode connection provides a message-oriented, multi-stream
    communication.

    The socket is created as a child of the server, which means that
    it is automatically deleted when the QSctpServer object is
    destroyed. It is still a good idea to delete the object
    explicitly when you are done with it, to avoid wasting memory.

    This function returns null if there are no pending datagram-mode
    connections.

    \note The returned QSctpSocket object cannot be used from another
    thread. If you want to use an incoming connection from another
    thread, you need to override incomingConnection().

    \sa hasPendingConnections(), nextPendingConnection(), QSctpSocket
*/
QSctpSocket *QSctpServer::nextPendingDatagramConnection()
{
    Q_D(QSctpServer);

    QMutableListIterator<QTcpSocket *> i(d->pendingConnections);
    while (i.hasNext()) {
        QSctpSocket *socket = qobject_cast<QSctpSocket *>(i.next());
        Q_ASSERT(socket);

        if (socket->isInDatagramMode()) {
            i.remove();
            Q_ASSERT(d->socketEngine);
            d->socketEngine->setReadNotificationEnabled(true);
            return socket;
        }
    }

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qsctpserver.cpp"
