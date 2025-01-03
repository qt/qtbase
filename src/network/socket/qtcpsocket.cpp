// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//#define QTCPSOCKET_DEBUG

/*!
    \class QTcpSocket

    \brief The QTcpSocket class provides a TCP socket.

    \reentrant
    \ingroup network
    \inmodule QtNetwork

    TCP (Transmission Control Protocol) is a reliable,
    stream-oriented, connection-oriented transport protocol. It is
    especially well suited for continuous transmission of data.

    QTcpSocket is a convenience subclass of QAbstractSocket that
    allows you to establish a TCP connection and transfer streams of
    data. See the QAbstractSocket documentation for details.

    \note TCP sockets cannot be opened in QIODevice::Unbuffered mode.

    \sa QTcpServer, QUdpSocket, QNetworkAccessManager,
    {Fortune Server}, {Fortune Client},
    {Threaded Fortune Server}, {Blocking Fortune Client},
    {Torrent Example}
*/

#include "qtcpsocket.h"
#include "qtcpsocket_p.h"
#include "qlist.h"
#include "qhostaddress.h"

QT_BEGIN_NAMESPACE

/*!
    Creates a QTcpSocket object in state \c UnconnectedState.

    \a parent is passed on to the QObject constructor.

    \sa socketType()
*/
QTcpSocket::QTcpSocket(QObject *parent)
    : QAbstractSocket(TcpSocket, *new QTcpSocketPrivate, parent)
{
#if defined(QTCPSOCKET_DEBUG)
    qDebug("QTcpSocket::QTcpSocket()");
#endif
    d_func()->isBuffered = true;
}

/*!
    Destroys the socket, closing the connection if necessary.

    \sa close()
*/

QTcpSocket::~QTcpSocket()
{
#if defined(QTCPSOCKET_DEBUG)
    qDebug("QTcpSocket::~QTcpSocket()");
#endif
}

/*!
    \internal
*/
QTcpSocket::QTcpSocket(QTcpSocketPrivate &dd, QObject *parent)
    : QAbstractSocket(TcpSocket, dd, parent)
{
    d_func()->isBuffered = true;
}

/*!
    \internal
*/
QTcpSocket::QTcpSocket(QAbstractSocket::SocketType socketType,
                       QTcpSocketPrivate &dd, QObject *parent)
    : QAbstractSocket(socketType, dd, parent)
{
}

QT_END_NAMESPACE

#include "moc_qtcpsocket.cpp"
