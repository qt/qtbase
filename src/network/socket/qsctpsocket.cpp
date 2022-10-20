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

//#define QSCTPSOCKET_DEBUG

/*!
    \class QSctpSocket
    \since 5.8

    \brief The QSctpSocket class provides an SCTP socket.

    \ingroup network
    \inmodule QtNetwork

    SCTP (Stream Control Transmission Protocol) is a transport layer
    protocol serving in a similar role as the popular protocols TCP
    and UDP. Like UDP, SCTP is message-oriented, but it ensures reliable,
    in-sequence transport of messages with congestion control like
    TCP.

    SCTP is connection-oriented protocol, which provides the complete
    simultaneous transmission of multiple data streams between
    endpoints. This multi-streaming allows data to be delivered by
    independent channels, so that if there is data loss in one stream,
    delivery will not be affected for the other streams.

    Being message-oriented, SCTP transports a sequence of messages,
    rather than transporting an unbroken stream of bytes as does TCP.
    Like in UDP, in SCTP a sender sends a message in one operation,
    and that exact message is passed to the receiving application
    process in one operation. But unlike UDP, the delivery is
    guaranteed.

    It also supports multi-homing, meaning that a connected endpoint
    can have alternate IP addresses associated with it in order to
    route around network failure or changing conditions.

    QSctpSocket is a convenience subclass of QTcpSocket that allows
    you to emulate TCP data stream over SCTP or establish an SCTP
    connection for reliable datagram service.

    QSctpSocket can operate in one of two possible modes:

    \list
    \li  Continuous byte stream (TCP emulation).
    \li  Multi-streamed datagram mode.
    \endlist

    To set a continuous byte stream mode, instantiate QSctpSocket and
    call setMaximumChannelCount() with a negative value. This gives the
    ability to use QSctpSocket as a regular buffered QTcpSocket. You
    can call connectToHost() to initiate connection with endpoint,
    write() to transmit and read() to receive data from the peer, but
    you cannot distinguish message boundaries.

    By default, QSctpSocket operates in datagram mode. Before
    connecting, call setMaximumChannelCount() to set the maximum number of
    channels that the application is prepared to support. This number
    is a parameter negotiated with the remote endpoint and its value
    can be bounded by the operating system. The default value of 0
    indicates to use the peer's value. If both endpoints have default
    values, then number of connection channels is system-dependent.
    After establishing a connection, you can fetch the actual number
    of channels by calling readChannelCount() and writeChannelCount().

    \snippet code/src_network_socket_qsctpsocket.cpp 0

    In datagram mode, QSctpSocket performs the buffering of datagrams
    independently for each channel. You can queue a datagram to the
    buffer of the current channel by calling writeDatagram() and read
    a pending datagram by calling readDatagram() respectively.

    Using the standard QIODevice functions read(), readLine(), write(),
    etc. is allowed in datagram mode with the same limitations as in
    continuous byte stream mode.

    \note This feature is not supported on the Windows platform.

    \sa QSctpServer, QTcpSocket, QAbstractSocket
*/

#include "qsctpsocket.h"
#include "qsctpsocket_p.h"

#include "qabstractsocketengine_p.h"
#include "private/qbytearray_p.h"

#ifdef QSCTPSOCKET_DEBUG
#include <qdebug.h>
#endif

QT_BEGIN_NAMESPACE

/*! \internal
*/
QSctpSocketPrivate::QSctpSocketPrivate()
    : maximumChannelCount(0)
{
}

/*! \internal
*/
QSctpSocketPrivate::~QSctpSocketPrivate()
{
}

/*! \internal
*/
bool QSctpSocketPrivate::canReadNotification()
{
    Q_Q(QSctpSocket);
#if defined (QSCTPSOCKET_DEBUG)
    qDebug("QSctpSocketPrivate::canReadNotification()");
#endif

    // Handle TCP emulation mode in the base implementation.
    if (!q->isInDatagramMode())
        return QTcpSocketPrivate::canReadNotification();

    const int savedCurrentChannel = currentReadChannel;
    bool currentChannelRead = false;
    do {
        int datagramSize = incomingDatagram.size();
        QIpPacketHeader header;

        do {
            // Determine the size of the pending datagram.
            qint64 bytesToRead = socketEngine->bytesAvailable();
            if (bytesToRead == 0) {
                // As a corner case, if we can't determine the size of the pending datagram,
                // try to read 4K of data from the socket. Subsequent ::recvmsg call either
                // fails or returns the actual length of the datagram.
                bytesToRead = 4096;
            }

            Q_ASSERT((datagramSize + int(bytesToRead)) < MaxByteArraySize);
            incomingDatagram.resize(datagramSize + int(bytesToRead));

#if defined (QSCTPSOCKET_DEBUG)
            qDebug("QSctpSocketPrivate::canReadNotification() about to read %lli bytes",
                   bytesToRead);
#endif
            qint64 readBytes = socketEngine->readDatagram(
                        incomingDatagram.data() + datagramSize, bytesToRead, &header,
                        QAbstractSocketEngine::WantAll);
            if (readBytes <= 0) {
                if (readBytes == -2) { // no data available for reading
                    incomingDatagram.resize(datagramSize);
                    return currentChannelRead;
                }

                socketEngine->close();
                if (readBytes == 0) {
                    setErrorAndEmit(QAbstractSocket::RemoteHostClosedError,
                                    QSctpSocket::tr("The remote host closed the connection"));
                } else {
#if defined (QSCTPSOCKET_DEBUG)
                    qDebug("QSctpSocketPrivate::canReadNotification() read failed: %s",
                           socketEngine->errorString().toLatin1().constData());
#endif
                    setErrorAndEmit(socketEngine->error(), socketEngine->errorString());
                }

#if defined (QSCTPSOCKET_DEBUG)
                qDebug("QSctpSocketPrivate::canReadNotification() disconnecting socket");
#endif
                q->disconnectFromHost();
                return currentChannelRead;
            }
            datagramSize += int(readBytes); // update datagram size
        } while (!header.endOfRecord);

#if defined (QSCTPSOCKET_DEBUG)
        qDebug("QSctpSocketPrivate::canReadNotification() got datagram from channel %i, size = %i",
               header.streamNumber, datagramSize);
#endif

        // Drop the datagram, if opened only for writing
        if (!q->isReadable()) {
            incomingDatagram.clear();
            continue;
        }

        // Store datagram in the channel buffer
        Q_ASSERT(header.streamNumber < readBuffers.size());
        incomingDatagram.resize(datagramSize);
        readBuffers[header.streamNumber].setChunkSize(0); // set packet mode on channel buffer
        readBuffers[header.streamNumber].append(incomingDatagram);
        incomingDatagram = QByteArray();

        if (readHeaders.size() != readBuffers.size())
            readHeaders.resize(readBuffers.size());
        readHeaders[header.streamNumber].push_back(header);

        // Emit notifications.
        if (header.streamNumber == savedCurrentChannel)
            currentChannelRead = true;
        emitReadyRead(header.streamNumber);

    } while (state == QAbstractSocket::ConnectedState);

    return currentChannelRead;
}

/*! \internal
*/
bool QSctpSocketPrivate::writeToSocket()
{
    Q_Q(QSctpSocket);
#if defined (QSCTPSOCKET_DEBUG)
    qDebug("QSctpSocketPrivate::writeToSocket()");
#endif

    // Handle TCP emulation mode in the base implementation.
    if (!q->isInDatagramMode())
        return QTcpSocketPrivate::writeToSocket();

    if (!socketEngine)
        return false;

    QIpPacketHeader defaultHeader;
    const int savedCurrentChannel = currentWriteChannel;
    bool currentChannelWritten = false;
    bool transmitting;
    do {
        transmitting = false;

        for (int channel = 0; channel < writeBuffers.size(); ++channel) {
            QRingBuffer &channelBuffer = writeBuffers[channel];

            if (channelBuffer.isEmpty())
                continue;

            const bool hasHeader = (channel < writeHeaders.size())
                                   && !writeHeaders[channel].empty();
            QIpPacketHeader &header = hasHeader ? writeHeaders[channel].front() : defaultHeader;
            header.streamNumber = channel;
            qint64 sent = socketEngine->writeDatagram(channelBuffer.readPointer(),
                                                      channelBuffer.nextDataBlockSize(),
                                                      header);
            if (sent < 0) {
                if (sent == -2) // temporary error in writeDatagram
                    return currentChannelWritten;

                socketEngine->close();
#if defined (QSCTPSOCKET_DEBUG)
                qDebug("QSctpSocketPrivate::writeToSocket() write error, aborting. %s",
                       socketEngine->errorString().toLatin1().constData());
#endif
                setErrorAndEmit(socketEngine->error(), socketEngine->errorString());
                // An unexpected error so close the socket.
                q->disconnectFromHost();
                return currentChannelWritten;
            }
            Q_ASSERT(sent == channelBuffer.nextDataBlockSize());
#if defined (QSCTPSOCKET_DEBUG)
            qDebug("QSctpSocketPrivate::writeToSocket() sent datagram of size %lli to channel %i",
                   sent, channel);
#endif
            transmitting = true;

            // Remove datagram from the buffer
            channelBuffer.read();
            if (hasHeader)
                writeHeaders[channel].pop_front();

            // Emit notifications.
            if (channel == savedCurrentChannel)
                currentChannelWritten = true;
            emitBytesWritten(sent, channel);

            // If we were closed as a result of the bytesWritten() signal, return.
            if (state == QAbstractSocket::UnconnectedState) {
#if defined (QSCTPSOCKET_DEBUG)
                qDebug("QSctpSocketPrivate::writeToSocket() socket is closing - returning");
#endif
                return currentChannelWritten;
            }
        }
    } while (transmitting);

    // At this point socket is either in Connected or Closing state,
    // write buffers are empty.
    if (state == QAbstractSocket::ClosingState)
        q->disconnectFromHost();
    else
        socketEngine->setWriteNotificationEnabled(false);

    return currentChannelWritten;
}

/*! \internal
*/
void QSctpSocketPrivate::configureCreatedSocket()
{
    if (socketEngine)
        socketEngine->setOption(QAbstractSocketEngine::MaxStreamsSocketOption,
                                maximumChannelCount < 0 ? 1 : maximumChannelCount);
}

/*!
    Creates a QSctpSocket object in state \c UnconnectedState.

    Sets the datagram operation mode. The \a parent argument is passed
    to QObject's constructor.

    \sa socketType(), setMaximumChannelCount()
*/
QSctpSocket::QSctpSocket(QObject *parent)
    : QTcpSocket(SctpSocket, *new QSctpSocketPrivate, parent)
{
#if defined(QSCTPSOCKET_DEBUG)
    qDebug("QSctpSocket::QSctpSocket()");
#endif
    d_func()->isBuffered = true;
}

/*!
    Destroys the socket, closing the connection if necessary.

    \sa close()
*/
QSctpSocket::~QSctpSocket()
{
#if defined(QSCTPSOCKET_DEBUG)
    qDebug("QSctpSocket::~QSctpSocket()");
#endif
}

/*! \reimp
*/
qint64 QSctpSocket::readData(char *data, qint64 maxSize)
{
    Q_D(QSctpSocket);

    // Cleanup headers, if the user calls the standard QIODevice functions
    if (d->currentReadChannel < d->readHeaders.size())
        d->readHeaders[d->currentReadChannel].clear();

    return QTcpSocket::readData(data, maxSize);
}

/*! \reimp
*/
qint64 QSctpSocket::readLineData(char *data, qint64 maxlen)
{
    Q_D(QSctpSocket);

    // Cleanup headers, if the user calls the standard QIODevice functions
    if (d->currentReadChannel < d->readHeaders.size())
        d->readHeaders[d->currentReadChannel].clear();

    return QTcpSocket::readLineData(data, maxlen);
}

/*! \reimp
*/
void QSctpSocket::close()
{
    QTcpSocket::close();
    d_func()->readHeaders.clear();
}

/*! \reimp
*/
void QSctpSocket::disconnectFromHost()
{
    Q_D(QSctpSocket);

    QTcpSocket::disconnectFromHost();
    if (d->state == QAbstractSocket::UnconnectedState) {
        d->incomingDatagram.clear();
        d->writeHeaders.clear();
    }
}

/*!
    Sets the maximum number of channels that the application is
    prepared to support in datagram mode, to \a count. If \a count
    is 0, endpoint's value for maximum number of channels is used.
    Negative \a count sets a continuous byte stream mode.

    Call this method only when QSctpSocket is in UnconnectedState.

    \sa maximumChannelCount(), readChannelCount(), writeChannelCount()
*/
void QSctpSocket::setMaximumChannelCount(int count)
{
    Q_D(QSctpSocket);
    if (d->state != QAbstractSocket::UnconnectedState) {
        qWarning("QSctpSocket::setMaximumChannelCount() is only allowed in UnconnectedState");
        return;
    }
#if defined(QSCTPSOCKET_DEBUG)
    qDebug("QSctpSocket::setMaximumChannelCount(%i)", count);
#endif
    d->maximumChannelCount = qMax(count, -1);
}

/*!
    Returns the maximum number of channels that QSctpSocket is able to
    support.

    A value of 0 (the default) means that the number of connection
    channels would be set by the remote endpoint.

    Returns -1 if QSctpSocket is running in continuous byte stream
    mode.

    \sa setMaximumChannelCount(), readChannelCount(), writeChannelCount()
*/
int QSctpSocket::maximumChannelCount() const
{
    return d_func()->maximumChannelCount;
}

/*!
    Returns \c true if the socket is running in datagram mode.

    \sa setMaximumChannelCount()
*/
bool QSctpSocket::isInDatagramMode() const
{
    Q_D(const QSctpSocket);
    return d->maximumChannelCount != -1 && d->isBuffered;
}

/*!
    Reads a datagram from the buffer of the current read channel, and
    returns it as a QNetworkDatagram object, along with the sender's
    host address and port. If possible, this function will also try to
    determine the datagram's destination address, port, and the number
    of hop counts at reception time.

    On failure, returns a QNetworkDatagram that reports \l
    {QNetworkDatagram::isValid()}{not valid}.

    \sa writeDatagram(), isInDatagramMode(), currentReadChannel()
*/
QNetworkDatagram QSctpSocket::readDatagram()
{
    Q_D(QSctpSocket);

    if (!isReadable() || !isInDatagramMode()) {
        qWarning("QSctpSocket::readDatagram(): operation is not permitted");
        return QNetworkDatagram();
    }

    if (d->currentReadChannel >= d->readHeaders.size()
        || d->readHeaders[d->currentReadChannel].size() == 0) {
        Q_ASSERT(d->buffer.isEmpty());
        return QNetworkDatagram();
    }

    QNetworkDatagram result(*new QNetworkDatagramPrivate(d->buffer.read(),
                                     d->readHeaders[d->currentReadChannel].front()));
    d->readHeaders[d->currentReadChannel].pop_front();

#if defined (QSCTPSOCKET_DEBUG)
    qDebug("QSctpSocket::readDatagram() returning datagram (%p, %i, \"%s\", %i)",
           result.d->data.constData(),
           result.d->data.size(),
           result.senderAddress().toString().toLatin1().constData(),
           result.senderPort());
#endif

    return result;
}

/*!
    Writes a \a datagram to the buffer of the current write channel.
    Returns true on success; otherwise returns false.

    \sa readDatagram(), isInDatagramMode(), currentWriteChannel()
*/
bool QSctpSocket::writeDatagram(const QNetworkDatagram &datagram)
{
    Q_D(QSctpSocket);

    if (!isWritable() || d->state != QAbstractSocket::ConnectedState || !d->socketEngine
        || !d->socketEngine->isValid() || !isInDatagramMode()) {
        qWarning("QSctpSocket::writeDatagram(): operation is not permitted");
        return false;
    }

    if (datagram.d->data.isEmpty()) {
        qWarning("QSctpSocket::writeDatagram() is called with empty datagram");
        return false;
    }


#if defined QSCTPSOCKET_DEBUG
    qDebug("QSctpSocket::writeDatagram(%p, %i, \"%s\", %i)",
           datagram.d->data.constData(),
           datagram.d->data.size(),
           datagram.destinationAddress().toString().toLatin1().constData(),
           datagram.destinationPort());
#endif

    if (d->writeHeaders.size() != d->writeBuffers.size())
        d->writeHeaders.resize(d->writeBuffers.size());
    Q_ASSERT(d->currentWriteChannel < d->writeHeaders.size());
    d->writeHeaders[d->currentWriteChannel].push_back(datagram.d->header);
    d->writeBuffer.setChunkSize(0); // set packet mode on channel buffer
    d->writeBuffer.append(datagram.d->data);

    d->socketEngine->setWriteNotificationEnabled(true);
    return true;
}

QT_END_NAMESPACE
