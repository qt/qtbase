// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qlocalsocket_p.h"
#include <qscopedvaluerollback.h>
#include <qdeadlinetimer.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
struct QSocketPoller
{
    QSocketPoller(const QLocalSocketPrivate &socket);

    qint64 getRemainingTime(const QDeadlineTimer &deadline) const;
    bool poll(const QDeadlineTimer &deadline);

    enum { maxHandles = 2 };
    HANDLE handles[maxHandles];
    DWORD handleCount = 0;
    bool waitForClose = false;
    bool writePending = false;
};

QSocketPoller::QSocketPoller(const QLocalSocketPrivate &socket)
{
    if (socket.pipeWriter && socket.pipeWriter->isWriteOperationActive()) {
        handles[handleCount++] = socket.pipeWriter->syncEvent();
        writePending = true;
    }
    if (socket.pipeReader->isReadOperationActive())
        handles[handleCount++] = socket.pipeReader->syncEvent();
    else
        waitForClose = true;
}

qint64 QSocketPoller::getRemainingTime(const QDeadlineTimer &deadline) const
{
    const qint64 sleepTime = 10;
    qint64 remainingTime = deadline.remainingTime();
    if (waitForClose && (remainingTime > sleepTime || remainingTime == -1))
        return sleepTime;

    return remainingTime;
}

/*!
    \internal

    Waits until new data is available for reading or write operation
    completes. Returns \c true, if we need to check pipe workers;
    otherwise it returns \c false (if an error occurred or the operation
    timed out).

    \note If the read operation is inactive, it succeeds after
    a short wait, allowing the caller to check the state of the socket.
*/
bool QSocketPoller::poll(const QDeadlineTimer &deadline)
{
    Q_ASSERT(handleCount != 0);
    QDeadlineTimer timer(getRemainingTime(deadline));
    DWORD waitRet;

    do {
        waitRet = WaitForMultipleObjectsEx(handleCount, handles, FALSE,
                                           timer.remainingTime(), TRUE);
    } while (waitRet == WAIT_IO_COMPLETION);

    if (waitRet == WAIT_TIMEOUT)
        return waitForClose || !deadline.hasExpired();

    return waitRet - WAIT_OBJECT_0 < handleCount;
}
} // anonymous namespace

void QLocalSocketPrivate::init()
{
    Q_Q(QLocalSocket);
    pipeReader = new QWindowsPipeReader(q);
    QObjectPrivate::connect(pipeReader, &QWindowsPipeReader::readyRead,
                            this, &QLocalSocketPrivate::_q_canRead);
    q->connect(pipeReader, SIGNAL(pipeClosed()), SLOT(_q_pipeClosed()), Qt::QueuedConnection);
    q->connect(pipeReader, SIGNAL(winError(ulong,QString)), SLOT(_q_winError(ulong,QString)));
}

void QLocalSocketPrivate::_q_winError(ulong windowsError, const QString &function)
{
    Q_Q(QLocalSocket);
    QLocalSocket::LocalSocketState currentState = state;

    // If the connectToServer fails due to WaitNamedPipe() time-out, assume ConnectionError
    if (state == QLocalSocket::ConnectingState && windowsError == ERROR_SEM_TIMEOUT)
        windowsError = ERROR_NO_DATA;

    switch (windowsError) {
    case ERROR_PIPE_NOT_CONNECTED:
    case ERROR_BROKEN_PIPE:
    case ERROR_NO_DATA:
        error = QLocalSocket::ConnectionError;
        errorString = QLocalSocket::tr("%1: Connection error").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    case ERROR_FILE_NOT_FOUND:
        error = QLocalSocket::ServerNotFoundError;
        errorString = QLocalSocket::tr("%1: Invalid name").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    case ERROR_ACCESS_DENIED:
        error = QLocalSocket::SocketAccessError;
        errorString = QLocalSocket::tr("%1: Access denied").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    default:
        error = QLocalSocket::UnknownSocketError;
        errorString = QLocalSocket::tr("%1: Unknown error %2").arg(function).arg(windowsError);
#if defined QLOCALSOCKET_DEBUG
        qWarning() << "QLocalSocket error not handled:" << errorString;
#endif
        state = QLocalSocket::UnconnectedState;
    }

    if (currentState != state) {
        emit q->stateChanged(state);
        if (state == QLocalSocket::UnconnectedState && currentState != QLocalSocket::ConnectingState)
            emit q->disconnected();
    }
    emit q->errorOccurred(error);
}

QLocalSocketPrivate::QLocalSocketPrivate() : QIODevicePrivate(),
       handle(INVALID_HANDLE_VALUE),
       pipeWriter(0),
       pipeReader(0),
       error(QLocalSocket::UnknownSocketError),
       state(QLocalSocket::UnconnectedState),
       emittedReadyRead(false),
       emittedBytesWritten(false)
{
}

QLocalSocketPrivate::~QLocalSocketPrivate()
{
    Q_ASSERT(state == QLocalSocket::UnconnectedState);
    Q_ASSERT(handle == INVALID_HANDLE_VALUE);
    Q_ASSERT(pipeWriter == nullptr);
}

void QLocalSocket::connectToServer(OpenMode openMode)
{
    Q_D(QLocalSocket);
    if (state() == ConnectedState || state() == ConnectingState) {
        d->error = OperationError;
        d->errorString = tr("Trying to connect while connection is in progress");
        emit errorOccurred(QLocalSocket::OperationError);
        return;
    }

    d->error = QLocalSocket::UnknownSocketError;
    d->errorString = QString();
    d->state = ConnectingState;
    emit stateChanged(d->state);
    if (d->serverName.isEmpty()) {
        d->error = ServerNotFoundError;
        d->errorString = tr("%1: Invalid name").arg("QLocalSocket::connectToServer"_L1);
        d->state = UnconnectedState;
        emit errorOccurred(d->error);
        emit stateChanged(d->state);
        return;
    }

    const auto pipePath = "\\\\.\\pipe\\"_L1;
    if (d->serverName.startsWith(pipePath))
        d->fullServerName = d->serverName;
    else
        d->fullServerName = pipePath + d->serverName;
    // Try to open a named pipe
    HANDLE localSocket;
    forever {
        DWORD permissions = (openMode & QIODevice::ReadOnly) ? GENERIC_READ : 0;
        permissions |= (openMode & QIODevice::WriteOnly) ? GENERIC_WRITE : 0;
        localSocket = CreateFile(reinterpret_cast<const wchar_t *>(d->fullServerName.utf16()), // pipe name
                                 permissions,
                                 0,              // no sharing
                                 NULL,           // default security attributes
                                 OPEN_EXISTING,  // opens existing pipe
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);          // no template file

        if (localSocket != INVALID_HANDLE_VALUE)
            break;
        DWORD error = GetLastError();
        // It is really an error only if it is not ERROR_PIPE_BUSY
        if (ERROR_PIPE_BUSY != error) {
            break;
        }

        // All pipe instances are busy, so wait until connected or up to 5 seconds.
        if (!WaitNamedPipe((const wchar_t *)d->fullServerName.utf16(), 5000))
            break;
    }

    if (localSocket == INVALID_HANDLE_VALUE) {
        const DWORD winError = GetLastError();
        d->_q_winError(winError, "QLocalSocket::connectToServer"_L1);
        d->fullServerName = QString();
        return;
    }

    // we have a valid handle
    if (setSocketDescriptor(reinterpret_cast<qintptr>(localSocket), ConnectedState, openMode))
        emit connected();
}

static qint64 transformPipeReaderResult(qint64 res)
{
    // QWindowsPipeReader's reading functions return error codes
    // that don't match what we need.
    switch (res) {
    case 0:     // EOF -> transform to error
        return -1;
    case -2:    // EWOULDBLOCK -> no error, just no bytes
        return 0;
    default:
        return res;
    }
}

// This is reading from the buffer
qint64 QLocalSocket::readData(char *data, qint64 maxSize)
{
    Q_D(QLocalSocket);

    if (!maxSize)
        return 0;

    return transformPipeReaderResult(d->pipeReader->read(data, maxSize));
}

qint64 QLocalSocket::readLineData(char *data, qint64 maxSize)
{
    Q_D(QLocalSocket);

    if (!maxSize)
        return 0;

    // QIODevice::readLine() reserves space for the trailing '\0' byte,
    // so we must read 'maxSize + 1' bytes.
    return transformPipeReaderResult(d->pipeReader->readLine(data, maxSize + 1));
}

qint64 QLocalSocket::skipData(qint64 maxSize)
{
    Q_D(QLocalSocket);

    if (!maxSize)
        return 0;

    return transformPipeReaderResult(d->pipeReader->skip(maxSize));
}

qint64 QLocalSocket::writeData(const char *data, qint64 len)
{
    Q_D(QLocalSocket);
    if (!isValid()) {
        d->error = OperationError;
        d->errorString = tr("Socket is not connected");
        return -1;
    }

    if (len == 0)
        return 0;

    if (!d->pipeWriter) {
        d->pipeWriter = new QWindowsPipeWriter(d->handle, this);
        QObjectPrivate::connect(d->pipeWriter, &QWindowsPipeWriter::bytesWritten,
                                d, &QLocalSocketPrivate::_q_bytesWritten);
        QObjectPrivate::connect(d->pipeWriter, &QWindowsPipeWriter::writeFailed,
                                d, &QLocalSocketPrivate::_q_writeFailed);
    }

    if (d->isWriteChunkCached(data, len))
        d->pipeWriter->write(*(d->currentWriteChunk));
    else
        d->pipeWriter->write(data, len);

    return len;
}

void QLocalSocket::abort()
{
    Q_D(QLocalSocket);
    if (d->pipeWriter) {
        delete d->pipeWriter;
        d->pipeWriter = 0;
    }
    close();
}

void QLocalSocketPrivate::_q_canRead()
{
    Q_Q(QLocalSocket);
    if (!emittedReadyRead) {
        QScopedValueRollback<bool> guard(emittedReadyRead, true);
        emit q->readyRead();
    }
}

void QLocalSocketPrivate::_q_pipeClosed()
{
    Q_Q(QLocalSocket);
    if (state == QLocalSocket::UnconnectedState)
        return;

    if (state != QLocalSocket::ClosingState) {
        state = QLocalSocket::ClosingState;
        emit q->stateChanged(state);
        if (state != QLocalSocket::ClosingState)
            return;
    }

    serverName.clear();
    fullServerName.clear();
    pipeReader->stop();
    delete pipeWriter;
    pipeWriter = nullptr;
    if (handle != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(handle);
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }

    state = QLocalSocket::UnconnectedState;
    emit q->stateChanged(state);
    emit q->readChannelFinished();
    emit q->disconnected();
}

qint64 QLocalSocket::bytesAvailable() const
{
    Q_D(const QLocalSocket);
    qint64 available = QIODevice::bytesAvailable();
    available += d->pipeReader->bytesAvailable();
    return available;
}

qint64 QLocalSocket::bytesToWrite() const
{
    Q_D(const QLocalSocket);
    return d->pipeWriterBytesToWrite();
}

bool QLocalSocket::canReadLine() const
{
    Q_D(const QLocalSocket);
    return QIODevice::canReadLine() || d->pipeReader->canReadLine();
}

void QLocalSocket::close()
{
    Q_D(QLocalSocket);

    QIODevice::close();
    d->pipeReader->stopAndClear();
    d->serverName = QString();
    d->fullServerName = QString();
    disconnectFromServer();
}

bool QLocalSocket::flush()
{
    Q_D(QLocalSocket);

    return d->pipeWriter && d->pipeWriter->checkForWrite();
}

void QLocalSocket::disconnectFromServer()
{
    Q_D(QLocalSocket);

    if (bytesToWrite() == 0) {
        d->_q_pipeClosed();
    } else if (d->state != QLocalSocket::ClosingState) {
        d->state = QLocalSocket::ClosingState;
        emit stateChanged(d->state);
    }
}

QLocalSocket::LocalSocketError QLocalSocket::error() const
{
    Q_D(const QLocalSocket);
    return d->error;
}

bool QLocalSocket::setSocketDescriptor(qintptr socketDescriptor,
              LocalSocketState socketState, OpenMode openMode)
{
    Q_D(QLocalSocket);
    d->pipeReader->stop();
    d->handle = reinterpret_cast<HANDLE>(socketDescriptor);
    d->state = socketState;
    d->pipeReader->setHandle(d->handle);
    QIODevice::open(openMode);
    emit stateChanged(d->state);
    if (d->state == ConnectedState && openMode.testFlag(QIODevice::ReadOnly))
        d->pipeReader->startAsyncRead();
    return true;
}

qint64 QLocalSocketPrivate::pipeWriterBytesToWrite() const
{
    return pipeWriter ? pipeWriter->bytesToWrite() : qint64(0);
}

void QLocalSocketPrivate::_q_bytesWritten(qint64 bytes)
{
    Q_Q(QLocalSocket);
    if (!emittedBytesWritten) {
        QScopedValueRollback<bool> guard(emittedBytesWritten, true);
        emit q->bytesWritten(bytes);
    }
    if (state == QLocalSocket::ClosingState)
        q->disconnectFromServer();
}

void QLocalSocketPrivate::_q_writeFailed()
{
    Q_Q(QLocalSocket);
    error = QLocalSocket::PeerClosedError;
    errorString = QLocalSocket::tr("Remote closed");
    emit q->errorOccurred(error);

    _q_pipeClosed();
}

qintptr QLocalSocket::socketDescriptor() const
{
    Q_D(const QLocalSocket);
    return reinterpret_cast<qintptr>(d->handle);
}

qint64 QLocalSocket::readBufferSize() const
{
    Q_D(const QLocalSocket);
    return d->pipeReader->maxReadBufferSize();
}

void QLocalSocket::setReadBufferSize(qint64 size)
{
    Q_D(QLocalSocket);
    d->pipeReader->setMaxReadBufferSize(size);
}

bool QLocalSocket::waitForConnected(int msecs)
{
    Q_UNUSED(msecs);
    return (state() == ConnectedState);
}

bool QLocalSocket::waitForDisconnected(int msecs)
{
    Q_D(QLocalSocket);
    if (state() == UnconnectedState) {
        qWarning("QLocalSocket::waitForDisconnected() is not allowed in UnconnectedState");
        return false;
    }

    QDeadlineTimer deadline(msecs);
    bool wasChecked = false;
    while (!d->pipeReader->isPipeClosed()) {
        if (wasChecked && deadline.hasExpired())
            return false;

        QSocketPoller poller(*d);
        // The first parameter of the WaitForMultipleObjectsEx() call cannot
        // be zero. So we have to call SleepEx() here.
        if (!poller.writePending && poller.waitForClose) {
            // Prevent waiting on the first pass, if both the pipe reader
            // and the pipe writer are inactive.
            if (wasChecked)
                SleepEx(poller.getRemainingTime(deadline), TRUE);
        } else if (!poller.poll(deadline)) {
            return false;
        }

        if (d->pipeWriter)
            d->pipeWriter->checkForWrite();

        // When the read buffer is full, the read sequence is not running,
        // so we need to peek the pipe to detect disconnection.
        if (poller.waitForClose && isValid())
            d->pipeReader->checkPipeState();

        d->pipeReader->checkForReadyRead();
        wasChecked = true;
    }
    d->_q_pipeClosed();
    return true;
}

bool QLocalSocket::isValid() const
{
    Q_D(const QLocalSocket);
    return d->handle != INVALID_HANDLE_VALUE;
}

bool QLocalSocket::waitForReadyRead(int msecs)
{
    Q_D(QLocalSocket);

    if (d->state != QLocalSocket::ConnectedState)
        return false;

    QDeadlineTimer deadline(msecs);
    while (!d->pipeReader->isPipeClosed()) {
        QSocketPoller poller(*d);
        if (poller.waitForClose || !poller.poll(deadline))
            return false;

        if (d->pipeWriter)
            d->pipeWriter->checkForWrite();

        if (d->pipeReader->checkForReadyRead())
            return true;
    }
    d->_q_pipeClosed();
    return false;
}

bool QLocalSocket::waitForBytesWritten(int msecs)
{
    Q_D(QLocalSocket);

    if (d->state == QLocalSocket::UnconnectedState)
        return false;

    QDeadlineTimer deadline(msecs);
    bool wasChecked = false;
    while (!d->pipeReader->isPipeClosed()) {
        if (wasChecked && deadline.hasExpired())
            return false;

        QSocketPoller poller(*d);
        if (!poller.writePending || !poller.poll(deadline))
            return false;

        Q_ASSERT(d->pipeWriter);
        if (d->pipeWriter->checkForWrite())
            return true;

        if (poller.waitForClose && isValid())
            d->pipeReader->checkPipeState();

        d->pipeReader->checkForReadyRead();
        wasChecked = true;
    }
    d->_q_pipeClosed();
    return false;
}

QT_END_NAMESPACE
