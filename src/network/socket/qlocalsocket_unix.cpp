// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlocalsocket.h"
#include "qlocalsocket_p.h"
#include "qnet_unix_p.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <qdir.h>
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qstringconverter.h>

#ifdef Q_OS_VXWORKS
#  include <selectLib.h>
#endif

#define QT_CONNECT_TIMEOUT 30000


QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
// determine the full server path
static QString pathNameForConnection(const QString &connectingName,
                                     QLocalSocket::SocketOptions options)
{
    if (options.testFlag(QLocalSocket::AbstractNamespaceOption)
        || connectingName.startsWith(u'/')) {
        return connectingName;
    }

    return QDir::tempPath() + u'/' + connectingName;
}

static QLocalSocket::SocketOptions optionsForPlatform(QLocalSocket::SocketOptions srcOptions)
{
    // For OS that does not support abstract namespace the AbstractNamespaceOption
    // option is cleared.
    if (!PlatformSupportsAbstractNamespace)
        return QLocalSocket::NoOptions;
    return srcOptions;
}
}

QLocalSocketPrivate::QLocalSocketPrivate() : QIODevicePrivate(),
        delayConnect(nullptr),
        connectTimer(nullptr),
        connectingSocket(-1),
        state(QLocalSocket::UnconnectedState),
        socketOptions(QLocalSocket::NoOptions)
{
}

void QLocalSocketPrivate::init()
{
    Q_Q(QLocalSocket);
    // QIODevice signals
    q->connect(&unixSocket, SIGNAL(bytesWritten(qint64)),
               q, SIGNAL(bytesWritten(qint64)));
    q->connect(&unixSocket, SIGNAL(readyRead()), q, SIGNAL(readyRead()));
    // QAbstractSocket signals
    q->connect(&unixSocket, SIGNAL(connected()), q, SIGNAL(connected()));
    q->connect(&unixSocket, SIGNAL(disconnected()), q, SIGNAL(disconnected()));
    q->connect(&unixSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
               q, SLOT(_q_stateChanged(QAbstractSocket::SocketState)));
    q->connect(&unixSocket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
               q, SLOT(_q_errorOccurred(QAbstractSocket::SocketError)));
    q->connect(&unixSocket, SIGNAL(readChannelFinished()), q, SIGNAL(readChannelFinished()));
    unixSocket.setParent(q);
}

void QLocalSocketPrivate::_q_errorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_Q(QLocalSocket);
    QString function = "QLocalSocket"_L1;
    QLocalSocket::LocalSocketError error = (QLocalSocket::LocalSocketError)socketError;
    QString errorString = generateErrorString(error, function);
    q->setErrorString(errorString);
    emit q->errorOccurred(error);
}

void QLocalSocketPrivate::_q_stateChanged(QAbstractSocket::SocketState newState)
{
    Q_Q(QLocalSocket);
    QLocalSocket::LocalSocketState currentState = state;
    switch(newState) {
    case QAbstractSocket::UnconnectedState:
        state = QLocalSocket::UnconnectedState;
        serverName.clear();
        fullServerName.clear();
        break;
    case QAbstractSocket::ConnectingState:
        state = QLocalSocket::ConnectingState;
        break;
    case QAbstractSocket::ConnectedState:
        state = QLocalSocket::ConnectedState;
        break;
    case QAbstractSocket::ClosingState:
        state = QLocalSocket::ClosingState;
        break;
    default:
#if defined QLOCALSOCKET_DEBUG
        qWarning() << "QLocalSocket::Unhandled socket state change:" << newState;
#endif
        return;
    }
    if (currentState != state)
        emit q->stateChanged(state);
}

QString QLocalSocketPrivate::generateErrorString(QLocalSocket::LocalSocketError error, const QString &function) const
{
    QString errorString;
    switch (error) {
    case QLocalSocket::ConnectionRefusedError:
        errorString = QLocalSocket::tr("%1: Connection refused").arg(function);
        break;
    case QLocalSocket::PeerClosedError:
        errorString = QLocalSocket::tr("%1: Remote closed").arg(function);
        break;
    case QLocalSocket::ServerNotFoundError:
        errorString = QLocalSocket::tr("%1: Invalid name").arg(function);
        break;
    case QLocalSocket::SocketAccessError:
        errorString = QLocalSocket::tr("%1: Socket access error").arg(function);
        break;
    case QLocalSocket::SocketResourceError:
        errorString = QLocalSocket::tr("%1: Socket resource error").arg(function);
        break;
    case QLocalSocket::SocketTimeoutError:
        errorString = QLocalSocket::tr("%1: Socket operation timed out").arg(function);
        break;
    case QLocalSocket::DatagramTooLargeError:
        errorString = QLocalSocket::tr("%1: Datagram too large").arg(function);
        break;
    case QLocalSocket::ConnectionError:
        errorString = QLocalSocket::tr("%1: Connection error").arg(function);
        break;
    case QLocalSocket::UnsupportedSocketOperationError:
        errorString = QLocalSocket::tr("%1: The socket operation is not supported").arg(function);
        break;
    case QLocalSocket::OperationError:
        errorString = QLocalSocket::tr("%1: Operation not permitted when socket is in this state").arg(function);
        break;
    case QLocalSocket::UnknownSocketError:
    default:
        errorString = QLocalSocket::tr("%1: Unknown error %2").arg(function).arg(errno);
    }
    return errorString;
}

void QLocalSocketPrivate::setErrorAndEmit(QLocalSocket::LocalSocketError error, const QString &function)
{
    Q_Q(QLocalSocket);
    switch (error) {
    case QLocalSocket::ConnectionRefusedError:
        unixSocket.setSocketError(QAbstractSocket::ConnectionRefusedError);
        break;
    case QLocalSocket::PeerClosedError:
        unixSocket.setSocketError(QAbstractSocket::RemoteHostClosedError);
        break;
    case QLocalSocket::ServerNotFoundError:
        unixSocket.setSocketError(QAbstractSocket::HostNotFoundError);
        break;
    case QLocalSocket::SocketAccessError:
        unixSocket.setSocketError(QAbstractSocket::SocketAccessError);
        break;
    case QLocalSocket::SocketResourceError:
        unixSocket.setSocketError(QAbstractSocket::SocketResourceError);
        break;
    case QLocalSocket::SocketTimeoutError:
        unixSocket.setSocketError(QAbstractSocket::SocketTimeoutError);
        break;
    case QLocalSocket::DatagramTooLargeError:
        unixSocket.setSocketError(QAbstractSocket::DatagramTooLargeError);
        break;
    case QLocalSocket::ConnectionError:
        unixSocket.setSocketError(QAbstractSocket::NetworkError);
        break;
    case QLocalSocket::UnsupportedSocketOperationError:
        unixSocket.setSocketError(QAbstractSocket::UnsupportedSocketOperationError);
        break;
    case QLocalSocket::UnknownSocketError:
    default:
        unixSocket.setSocketError(QAbstractSocket::UnknownSocketError);
    }

    QString errorString = generateErrorString(error, function);
    q->setErrorString(errorString);
    emit q->errorOccurred(error);

    // errors cause a disconnect
    unixSocket.setSocketState(QAbstractSocket::UnconnectedState);
    bool stateChanged = (state != QLocalSocket::UnconnectedState);
    state = QLocalSocket::UnconnectedState;
    q->close();
    if (stateChanged)
        q->emit stateChanged(state);
}

void QLocalSocket::connectToServer(OpenMode openMode)
{
    Q_D(QLocalSocket);
    if (state() == ConnectedState || state() == ConnectingState) {
        QString errorString = d->generateErrorString(QLocalSocket::OperationError, "QLocalSocket::connectToserver"_L1);
        setErrorString(errorString);
        emit errorOccurred(QLocalSocket::OperationError);
        return;
    }

    d->errorString.clear();
    d->unixSocket.setSocketState(QAbstractSocket::ConnectingState);
    d->state = ConnectingState;
    emit stateChanged(d->state);

    if (d->serverName.isEmpty()) {
        d->setErrorAndEmit(ServerNotFoundError, "QLocalSocket::connectToServer"_L1);
        return;
    }

    // create the socket
    if (-1 == (d->connectingSocket = qt_safe_socket(PF_UNIX, SOCK_STREAM, 0, O_NONBLOCK))) {
        d->setErrorAndEmit(UnsupportedSocketOperationError, "QLocalSocket::connectToServer"_L1);
        return;
    }

    // _q_connectToSocket does the actual connecting
    d->connectingName = d->serverName;
    d->connectingOpenMode = openMode;
    d->_q_connectToSocket();
    return;
}

/*!
    \internal

    Tries to connect connectingName and connectingOpenMode

    \sa connectToServer(), waitForConnected()
  */
void QLocalSocketPrivate::_q_connectToSocket()
{
    Q_Q(QLocalSocket);

    QLocalSocket::SocketOptions options = optionsForPlatform(socketOptions);
    const QString connectingPathName = pathNameForConnection(connectingName, options);
    const QByteArray encodedConnectingPathName = QFile::encodeName(connectingPathName);
    struct ::sockaddr_un addr;
    addr.sun_family = PF_UNIX;
    memset(addr.sun_path, 0, sizeof(addr.sun_path));

    // for abstract socket add 2 to length, to take into account trailing AND leading null
    constexpr unsigned int extraCharacters = PlatformSupportsAbstractNamespace ? 2 : 1;

    if (sizeof(addr.sun_path) < static_cast<size_t>(encodedConnectingPathName.size() + extraCharacters)) {
        QString function = "QLocalSocket::connectToServer"_L1;
        setErrorAndEmit(QLocalSocket::ServerNotFoundError, function);
        return;
    }

    QT_SOCKLEN_T addrSize = sizeof(::sockaddr_un);
    if (options.testFlag(QLocalSocket::AbstractNamespaceOption)) {
        ::memcpy(addr.sun_path + 1, encodedConnectingPathName.constData(),
                 encodedConnectingPathName.size() + 1);
        addrSize = offsetof(::sockaddr_un, sun_path) + encodedConnectingPathName.size() + 1;
    } else {
        ::memcpy(addr.sun_path, encodedConnectingPathName.constData(),
                 encodedConnectingPathName.size() + 1);
    }
    if (-1 == qt_safe_connect(connectingSocket, (struct sockaddr *)&addr, addrSize)) {
        QString function = "QLocalSocket::connectToServer"_L1;
        switch (errno)
        {
        case EINVAL:
        case ECONNREFUSED:
            setErrorAndEmit(QLocalSocket::ConnectionRefusedError, function);
            break;
        case ENOENT:
            setErrorAndEmit(QLocalSocket::ServerNotFoundError, function);
            break;
        case EACCES:
        case EPERM:
            setErrorAndEmit(QLocalSocket::SocketAccessError, function);
            break;
        case ETIMEDOUT:
            setErrorAndEmit(QLocalSocket::SocketTimeoutError, function);
            break;
        case EAGAIN:
            // Try again later, all of the sockets listening are full
            if (!delayConnect) {
                delayConnect = new QSocketNotifier(connectingSocket, QSocketNotifier::Write, q);
                q->connect(delayConnect, SIGNAL(activated(QSocketDescriptor)), q, SLOT(_q_connectToSocket()));
            }
            if (!connectTimer) {
                connectTimer = new QTimer(q);
                q->connect(connectTimer, SIGNAL(timeout()),
                                 q, SLOT(_q_abortConnectionAttempt()),
                                 Qt::DirectConnection);
                connectTimer->start(QT_CONNECT_TIMEOUT);
            }
            delayConnect->setEnabled(true);
            break;
        default:
            setErrorAndEmit(QLocalSocket::UnknownSocketError, function);
        }
        return;
    }

    // connected!
    cancelDelayedConnect();

    serverName = connectingName;
    fullServerName = connectingPathName;
    if (unixSocket.setSocketDescriptor(connectingSocket,
        QAbstractSocket::ConnectedState, connectingOpenMode)) {
        q->QIODevice::open(connectingOpenMode);
        q->emit connected();
    } else {
        QString function = "QLocalSocket::connectToServer"_L1;
        setErrorAndEmit(QLocalSocket::UnknownSocketError, function);
    }
    connectingSocket = -1;
    connectingName.clear();
    connectingOpenMode = { };
}

bool QLocalSocket::setSocketDescriptor(qintptr socketDescriptor,
        LocalSocketState socketState, OpenMode openMode)
{
    Q_D(QLocalSocket);
    QAbstractSocket::SocketState newSocketState = QAbstractSocket::UnconnectedState;
    switch (socketState) {
    case ConnectingState:
        newSocketState = QAbstractSocket::ConnectingState;
        break;
    case ConnectedState:
        newSocketState = QAbstractSocket::ConnectedState;
        break;
    case ClosingState:
        newSocketState = QAbstractSocket::ClosingState;
        break;
    case UnconnectedState:
        newSocketState = QAbstractSocket::UnconnectedState;
        break;
    }
    QIODevice::open(openMode);
    d->state = socketState;
    d->describeSocket(socketDescriptor);
    return d->unixSocket.setSocketDescriptor(socketDescriptor,
                                             newSocketState, openMode);
}

void QLocalSocketPrivate::describeSocket(qintptr socketDescriptor)
{
    bool abstractAddress = false;

    struct ::sockaddr_un addr;
    QT_SOCKLEN_T len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    const int getpeernameStatus = ::getpeername(socketDescriptor, (sockaddr *)&addr, &len);
    if (getpeernameStatus != 0 || len == offsetof(sockaddr_un, sun_path)) {
        // this is the case when we call it from QLocalServer, then there is no peername
        len = sizeof(addr);
        if (::getsockname(socketDescriptor, (sockaddr *)&addr, &len) != 0)
            return;
    }
    if (parseSockaddr(addr, static_cast<uint>(len), fullServerName, serverName, abstractAddress)) {
        QLocalSocket::SocketOptions options = socketOptions.value();
        socketOptions = options.setFlag(QLocalSocket::AbstractNamespaceOption, abstractAddress);
    }
}

bool QLocalSocketPrivate::parseSockaddr(const struct ::sockaddr_un &addr,
                                        uint len,
                                        QString &fullServerName,
                                        QString &serverName,
                                        bool &abstractNamespace)
{
    if (len <= offsetof(::sockaddr_un, sun_path))
        return false;
    len -= offsetof(::sockaddr_un, sun_path);
    // check for abstract socket address
    abstractNamespace = PlatformSupportsAbstractNamespace
                                 && (addr.sun_family == PF_UNIX && addr.sun_path[0] == 0);
    QStringDecoder toUtf16(QStringDecoder::System, QStringDecoder::Flag::Stateless);
    // An abstract socket address can be arbitrary binary. To properly handle such a case,
    // we'd have to add new access functions for this very specific case. Instead, we just
    // attempt to decode it according to OS text encoding. If it fails we ignore the result.
    QByteArrayView textData(addr.sun_path + (abstractNamespace ? 1 : 0),
                            len - (abstractNamespace ? 1 : 0));
    QString name = toUtf16(textData);
    if (!name.isEmpty() && !toUtf16.hasError()) {
        //conversion encodes the trailing zeros. So, in case of non-abstract namespace we
        //chop them off as \0 character is not allowed in filenames
        if (!abstractNamespace && (name.at(name.size() - 1) == QChar::fromLatin1('\0'))) {
            int truncPos = name.size() - 1;
            while (truncPos > 0 && name.at(truncPos - 1) == QChar::fromLatin1('\0'))
                truncPos--;
            name.truncate(truncPos);
        }
        fullServerName = name;
        serverName = abstractNamespace
                     ? name
                     : fullServerName.mid(fullServerName.lastIndexOf(u'/') + 1);
        if (serverName.isEmpty())
            serverName = fullServerName;
    }
    return true;
}

void QLocalSocketPrivate::_q_abortConnectionAttempt()
{
    Q_Q(QLocalSocket);
    q->close();
}

void QLocalSocketPrivate::cancelDelayedConnect()
{
    if (delayConnect) {
        delayConnect->setEnabled(false);
        delete delayConnect;
        delayConnect = nullptr;
        connectTimer->stop();
        delete connectTimer;
        connectTimer = nullptr;
    }
}

qintptr QLocalSocket::socketDescriptor() const
{
    Q_D(const QLocalSocket);
    return d->unixSocket.socketDescriptor();
}

qint64 QLocalSocket::readData(char *data, qint64 c)
{
    Q_D(QLocalSocket);
    return d->unixSocket.read(data, c);
}

qint64 QLocalSocket::readLineData(char *data, qint64 maxSize)
{
    if (!maxSize)
        return 0;

    // QIODevice::readLine() reserves space for the trailing '\0' byte,
    // so we must read 'maxSize + 1' bytes.
    return d_func()->unixSocket.readLine(data, maxSize + 1);
}

qint64 QLocalSocket::skipData(qint64 maxSize)
{
    return d_func()->unixSocket.skip(maxSize);
}

qint64 QLocalSocket::writeData(const char *data, qint64 c)
{
    Q_D(QLocalSocket);
    return d->unixSocket.writeData(data, c);
}

void QLocalSocket::abort()
{
    Q_D(QLocalSocket);
    d->unixSocket.abort();
    close();
}

qint64 QLocalSocket::bytesAvailable() const
{
    Q_D(const QLocalSocket);
    return QIODevice::bytesAvailable() + d->unixSocket.bytesAvailable();
}

qint64 QLocalSocket::bytesToWrite() const
{
    Q_D(const QLocalSocket);
    return d->unixSocket.bytesToWrite();
}

bool QLocalSocket::canReadLine() const
{
    Q_D(const QLocalSocket);
    return QIODevice::canReadLine() || d->unixSocket.canReadLine();
}

void QLocalSocket::close()
{
    Q_D(QLocalSocket);

    QIODevice::close();
    d->unixSocket.close();
    d->cancelDelayedConnect();
    if (d->connectingSocket != -1)
        ::close(d->connectingSocket);
    d->connectingSocket = -1;
    d->connectingName.clear();
    d->connectingOpenMode = { };
    d->serverName.clear();
    d->fullServerName.clear();
}

bool QLocalSocket::waitForBytesWritten(int msecs)
{
    Q_D(QLocalSocket);
    return d->unixSocket.waitForBytesWritten(msecs);
}

bool QLocalSocket::flush()
{
    Q_D(QLocalSocket);
    return d->unixSocket.flush();
}

void QLocalSocket::disconnectFromServer()
{
    Q_D(QLocalSocket);
    d->unixSocket.disconnectFromHost();
}

QLocalSocket::LocalSocketError QLocalSocket::error() const
{
    Q_D(const QLocalSocket);
    switch (d->unixSocket.error()) {
    case QAbstractSocket::ConnectionRefusedError:
        return QLocalSocket::ConnectionRefusedError;
    case QAbstractSocket::RemoteHostClosedError:
        return QLocalSocket::PeerClosedError;
    case QAbstractSocket::HostNotFoundError:
        return QLocalSocket::ServerNotFoundError;
    case QAbstractSocket::SocketAccessError:
        return QLocalSocket::SocketAccessError;
    case QAbstractSocket::SocketResourceError:
        return QLocalSocket::SocketResourceError;
    case QAbstractSocket::SocketTimeoutError:
        return QLocalSocket::SocketTimeoutError;
    case QAbstractSocket::DatagramTooLargeError:
        return QLocalSocket::DatagramTooLargeError;
    case QAbstractSocket::NetworkError:
        return QLocalSocket::ConnectionError;
    case QAbstractSocket::UnsupportedSocketOperationError:
        return QLocalSocket::UnsupportedSocketOperationError;
    case QAbstractSocket::UnknownSocketError:
        return QLocalSocket::UnknownSocketError;
    default:
#if defined QLOCALSOCKET_DEBUG
        qWarning() << "QLocalSocket error not handled:" << d->unixSocket.error();
#endif
        break;
    }
    return UnknownSocketError;
}

bool QLocalSocket::isValid() const
{
    Q_D(const QLocalSocket);
    return d->unixSocket.isValid();
}

qint64 QLocalSocket::readBufferSize() const
{
    Q_D(const QLocalSocket);
    return d->unixSocket.readBufferSize();
}

void QLocalSocket::setReadBufferSize(qint64 size)
{
    Q_D(QLocalSocket);
    d->unixSocket.setReadBufferSize(size);
}

bool QLocalSocket::waitForConnected(int msec)
{
    Q_D(QLocalSocket);

    if (state() != ConnectingState)
        return (state() == ConnectedState);

    QElapsedTimer timer;
    timer.start();

    pollfd pfd = qt_make_pollfd(d->connectingSocket, POLLIN);

    do {
        const int timeout = (msec > 0) ? qMax(msec - timer.elapsed(), Q_INT64_C(0)) : msec;
        const int result = qt_poll_msecs(&pfd, 1, timeout);

        if (result == -1)
            d->setErrorAndEmit(QLocalSocket::UnknownSocketError,
                               "QLocalSocket::waitForConnected"_L1);
        else if (result > 0)
            d->_q_connectToSocket();
    } while (state() == ConnectingState && !timer.hasExpired(msec));

    return (state() == ConnectedState);
}

bool QLocalSocket::waitForDisconnected(int msecs)
{
    Q_D(QLocalSocket);
    if (state() == UnconnectedState) {
        qWarning("QLocalSocket::waitForDisconnected() is not allowed in UnconnectedState");
        return false;
    }
    return (d->unixSocket.waitForDisconnected(msecs));
}

bool QLocalSocket::waitForReadyRead(int msecs)
{
    Q_D(QLocalSocket);
    if (state() == QLocalSocket::UnconnectedState)
        return false;
    return (d->unixSocket.waitForReadyRead(msecs));
}

QT_END_NAMESPACE
