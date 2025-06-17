// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qwindowspipereader_p.h"
#include <qcoreapplication.h>
#include <QMutexLocker>
#include <QPointer>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const DWORD minReadBufferSize = 4096;

QWindowsPipeReader::QWindowsPipeReader(QObject *parent)
    : QObject(parent),
      handle(INVALID_HANDLE_VALUE),
      eventHandle(CreateEvent(NULL, FALSE, FALSE, NULL)),
      syncHandle(CreateEvent(NULL, TRUE, FALSE, NULL)),
      waitObject(NULL),
      readBufferMaxSize(0),
      actualReadBufferSize(0),
      pendingReadBytes(0),
      lastError(ERROR_SUCCESS),
      state(Stopped),
      readSequenceStarted(false),
      pipeBroken(true),
      readyReadPending(false),
      winEventActPosted(false)
{
    ZeroMemory(&overlapped, sizeof(OVERLAPPED));
    overlapped.hEvent = eventHandle;
    waitObject = CreateThreadpoolWait(waitCallback, this, NULL);
    if (waitObject == NULL)
        qErrnoWarning("QWindowsPipeReader: CreateThreadpollWait failed.");
}

QWindowsPipeReader::~QWindowsPipeReader()
{
    stop();

    // Wait for thread pool callback to complete, as it can be still
    // executing some completion code.
    WaitForThreadpoolWaitCallbacks(waitObject, FALSE);
    CloseThreadpoolWait(waitObject);
    CloseHandle(eventHandle);
    CloseHandle(syncHandle);
}

/*!
    Sets the handle to read from. The handle must be valid.
    Do not call this function while the pipe is running.
 */
void QWindowsPipeReader::setHandle(HANDLE hPipeReadEnd)
{
    readBuffer.clear();
    actualReadBufferSize = 0;
    readyReadPending = false;
    pendingReadBytes = 0;
    handle = hPipeReadEnd;
    pipeBroken = false;
    lastError = ERROR_SUCCESS;
}

/*!
    Stops the asynchronous read sequence.
    If the read sequence is running then the I/O operation is canceled.
 */
void QWindowsPipeReader::stop()
{
    cancelAsyncRead(Stopped);
    pipeBroken = true;
}

/*!
    Stops the asynchronous read sequence.
    Reads all pending bytes into the internal buffer.
 */
void QWindowsPipeReader::drainAndStop()
{
    cancelAsyncRead(Draining);
    pipeBroken = true;
}

/*!
    Stops the asynchronous read sequence.
    Clears the internal buffer and discards any pending data.
 */
void QWindowsPipeReader::stopAndClear()
{
    cancelAsyncRead(Stopped);
    readBuffer.clear();
    actualReadBufferSize = 0;
    // QLocalSocket is supposed to write data in the 'Closing'
    // state, so we don't set 'pipeBroken' flag here. Also, avoid
    // setting this flag in checkForReadyRead().
    lastError = ERROR_SUCCESS;
}

/*!
    Stops the asynchronous read sequence.
 */
void QWindowsPipeReader::cancelAsyncRead(State newState)
{
    if (state != Running)
        return;

    mutex.lock();
    state = newState;
    if (readSequenceStarted) {
        // This can legitimately fail due to the GetOverlappedResult()
        // in the callback not being locked. We ignore ERROR_NOT_FOUND
        // in this case.
        if (!CancelIoEx(handle, &overlapped)) {
            const DWORD dwError = GetLastError();
            if (dwError != ERROR_NOT_FOUND) {
                qErrnoWarning(dwError, "QWindowsPipeReader: CancelIoEx on handle %p failed.",
                              handle);
            }
        }

        // Wait for callback to complete.
        do {
            mutex.unlock();
            waitForNotification();
            mutex.lock();
        } while (readSequenceStarted);
    }
    mutex.unlock();

    // Finish reading to keep the class state consistent. Note that
    // signals are not emitted in the call below, as the caller is
    // expected to do that synchronously.
    consumePending();
}

/*!
    Sets the size of internal read buffer.
 */
void QWindowsPipeReader::setMaxReadBufferSize(qint64 size)
{
    QMutexLocker locker(&mutex);
    readBufferMaxSize = size;
}

/*!
    Returns \c true if async operation is in progress, there is
    pending data to read, or a read error is pending.
*/
bool QWindowsPipeReader::isReadOperationActive() const
{
    QMutexLocker locker(&mutex);
    return readSequenceStarted || readyReadPending
           || (lastError != ERROR_SUCCESS && !pipeBroken);
}

/*!
    Returns the number of bytes we've read so far.
 */
qint64 QWindowsPipeReader::bytesAvailable() const
{
    return actualReadBufferSize;
}

/*!
    Copies at most \c{maxlen} bytes from the internal read buffer to \c{data}.
 */
qint64 QWindowsPipeReader::read(char *data, qint64 maxlen)
{
    QMutexLocker locker(&mutex);
    qint64 readSoFar;

    // If startAsyncRead() has read data, copy it to its destination.
    if (maxlen == 1 && actualReadBufferSize > 0) {
        *data = readBuffer.getChar();
        actualReadBufferSize--;
        readSoFar = 1;
    } else {
        readSoFar = readBuffer.read(data, qMin(actualReadBufferSize, maxlen));
        actualReadBufferSize -= readSoFar;
    }

    if (!pipeBroken) {
        startAsyncReadHelper(&locker);
        if (readSoFar == 0)
            return -2;      // signal EWOULDBLOCK
    }

    return readSoFar;
}

/*!
    Reads a line from the internal buffer, but no more than \c{maxlen}
    characters. A terminating '\0' byte is always appended to \c{data},
    so \c{maxlen} must be larger than 1.
 */
qint64 QWindowsPipeReader::readLine(char *data, qint64 maxlen)
{
    QMutexLocker locker(&mutex);
    qint64 readSoFar = 0;

    if (actualReadBufferSize > 0) {
        readSoFar = readBuffer.readLine(data, qMin(actualReadBufferSize + 1, maxlen));
        actualReadBufferSize -= readSoFar;
    }

    if (!pipeBroken) {
        startAsyncReadHelper(&locker);
        if (readSoFar == 0)
            return -2;      // signal EWOULDBLOCK
    }

    return readSoFar;
}

/*!
    Skips up to \c{maxlen} bytes from the internal read buffer.
 */
qint64 QWindowsPipeReader::skip(qint64 maxlen)
{
    QMutexLocker locker(&mutex);

    const qint64 skippedSoFar = readBuffer.skip(qMin(actualReadBufferSize, maxlen));
    actualReadBufferSize -= skippedSoFar;

    if (!pipeBroken) {
        startAsyncReadHelper(&locker);
        if (skippedSoFar == 0)
            return -2;      // signal EWOULDBLOCK
    }

    return skippedSoFar;
}

/*!
    Returns \c true if a complete line of data can be read from the buffer.
 */
bool QWindowsPipeReader::canReadLine() const
{
    QMutexLocker locker(&mutex);
    return readBuffer.indexOf('\n', actualReadBufferSize) >= 0;
}

/*!
    Starts an asynchronous read sequence on the pipe.
 */
void QWindowsPipeReader::startAsyncRead()
{
    QMutexLocker locker(&mutex);
    startAsyncReadHelper(&locker);
}

void QWindowsPipeReader::startAsyncReadHelper(QMutexLocker<QMutex> *locker)
{
    if (readSequenceStarted || lastError != ERROR_SUCCESS)
        return;

    state = Running;
    startAsyncReadLocked();

    // Do not post the event, if the read operation will be completed asynchronously.
    if (!readyReadPending && lastError == ERROR_SUCCESS)
        return;

    if (!winEventActPosted) {
        winEventActPosted = true;
        locker->unlock();
        QCoreApplication::postEvent(this, new QEvent(QEvent::WinEventAct));
    } else {
        locker->unlock();
    }

    SetEvent(syncHandle);
}

/*!
    Starts a new read sequence. Thread-safety should be ensured
    by the caller.
 */
void QWindowsPipeReader::startAsyncReadLocked()
{
    // Determine the number of bytes to read.
    qint64 bytesToRead = qMax(checkPipeState(), state == Running ? minReadBufferSize : 0);

    // This can happen only while draining; just do nothing in this case.
    if (bytesToRead == 0)
        return;

    while (lastError == ERROR_SUCCESS) {
        if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
            bytesToRead = readBufferMaxSize - readBuffer.size();
            if (bytesToRead <= 0) {
                // Buffer is full. User must read data from the buffer
                // before we can read more from the pipe.
                return;
            }
        }

        char *ptr = readBuffer.reserve(bytesToRead);

        // ReadFile() returns true, if the read operation completes synchronously.
        // We don't need to call GetOverlappedResult() additionally, because
        // 'numberOfBytesRead' is valid in this case.
        DWORD numberOfBytesRead;
        DWORD errorCode = ERROR_SUCCESS;
        if (!ReadFile(handle, ptr, bytesToRead, &numberOfBytesRead, &overlapped)) {
            errorCode = GetLastError();
            if (errorCode == ERROR_IO_PENDING) {
                Q_ASSERT(state == Running);
                // Operation has been queued and will complete in the future.
                readSequenceStarted = true;
                SetThreadpoolWait(waitObject, eventHandle, NULL);
                return;
            }
        }

        if (!readCompleted(errorCode, numberOfBytesRead))
            return;

        // In the 'Draining' state, we have to get all the data with one call
        // to ReadFile(). Note that message mode pipes are not supported here.
        if (state == Draining) {
            Q_ASSERT(bytesToRead == qint64(numberOfBytesRead));
            return;
        }

        // We need to loop until all pending data has been read and an
        // operation is queued for asynchronous completion.
        // If the pipe is configured to work in message mode, we read
        // the data in chunks.
        bytesToRead = qMax(checkPipeState(), minReadBufferSize);
    }
}

/*!
    \internal

    Thread pool callback procedure.
 */
void QWindowsPipeReader::waitCallback(PTP_CALLBACK_INSTANCE instance, PVOID context,
                                      PTP_WAIT wait, TP_WAIT_RESULT waitResult)
{
    Q_UNUSED(instance);
    Q_UNUSED(wait);
    Q_UNUSED(waitResult);
    QWindowsPipeReader *pipeReader = reinterpret_cast<QWindowsPipeReader *>(context);

    // Get the result of the asynchronous operation.
    DWORD numberOfBytesTransfered = 0;
    DWORD errorCode = ERROR_SUCCESS;
    if (!GetOverlappedResult(pipeReader->handle, &pipeReader->overlapped,
                             &numberOfBytesTransfered, FALSE))
        errorCode = GetLastError();

    pipeReader->mutex.lock();

    pipeReader->readSequenceStarted = false;

    // Do not overwrite error code, if error has been detected by
    // checkPipeState() in waitForPipeClosed(). Also, if the reader was
    // stopped, the only reason why this function can be called is the
    // completion of a cancellation. No signals should be emitted, and
    // no new read sequence should be started in this case.
    if (pipeReader->lastError == ERROR_SUCCESS && pipeReader->state != Stopped) {
        // Ignore ERROR_OPERATION_ABORTED. We have canceled the I/O operation
        // specifically for flushing the pipe.
        if (pipeReader->state == Draining && errorCode == ERROR_OPERATION_ABORTED)
            errorCode = ERROR_SUCCESS;

        if (pipeReader->readCompleted(errorCode, numberOfBytesTransfered))
            pipeReader->startAsyncReadLocked();

        if (pipeReader->state == Running && !pipeReader->winEventActPosted) {
            pipeReader->winEventActPosted = true;
            pipeReader->mutex.unlock();
            QCoreApplication::postEvent(pipeReader, new QEvent(QEvent::WinEventAct));
        } else {
            pipeReader->mutex.unlock();
        }
    } else {
        pipeReader->mutex.unlock();
    }

    // We set the event only after unlocking to avoid additional context
    // switches due to the released thread immediately running into the lock.
    SetEvent(pipeReader->syncHandle);
}

/*!
    Will be called whenever the read operation completes. Returns \c true if
    no error occurred; otherwise returns \c false.
 */
bool QWindowsPipeReader::readCompleted(DWORD errorCode, DWORD numberOfBytesRead)
{
    // ERROR_MORE_DATA is not an error. We're connected to a message mode
    // pipe and the message didn't fit into the pipe's system
    // buffer. We will read the remaining data in the next call.
    if (errorCode == ERROR_SUCCESS || errorCode == ERROR_MORE_DATA) {
        readyReadPending = true;
        pendingReadBytes += numberOfBytesRead;
        readBuffer.truncate(actualReadBufferSize + pendingReadBytes);
        return true;
    }

    lastError = errorCode;
    return false;
}

/*!
    Receives notification that the read operation has completed.
 */
bool QWindowsPipeReader::event(QEvent *e)
{
    if (e->type() == QEvent::WinEventAct) {
        consumePendingAndEmit(true);
        return true;
    }
    return QObject::event(e);
}

/*!
    Updates the read buffer size and emits pending signals in the main thread.
    Returns \c true, if readyRead() was emitted.
 */
bool QWindowsPipeReader::consumePendingAndEmit(bool allowWinActPosting)
{
    ResetEvent(syncHandle);
    mutex.lock();

    // Enable QEvent::WinEventAct posting.
    if (allowWinActPosting)
        winEventActPosted = false;

    const bool emitReadyRead = consumePending();
    const DWORD dwError = lastError;

    mutex.unlock();

    // Trigger 'pipeBroken' only once. This flag must be updated before
    // emitting the readyRead() signal. Otherwise, the read sequence will
    // be considered not finished, and we may hang if a slot connected
    // to readyRead() calls waitForReadyRead().
    const bool emitPipeClosed = (dwError != ERROR_SUCCESS && !pipeBroken);
    if (emitPipeClosed)
        pipeBroken = true;

    // Disable any further processing, if the pipe was stopped.
    // We are not allowed to emit signals in either 'Stopped'
    // or 'Draining' state.
    if (state != Running)
        return false;

    if (!emitPipeClosed) {
        if (emitReadyRead)
            emit readyRead();
    } else {
        QPointer<QWindowsPipeReader> alive(this);
        if (emitReadyRead)
            emit readyRead();
        if (alive && dwError != ERROR_BROKEN_PIPE && dwError != ERROR_PIPE_NOT_CONNECTED)
            emit winError(dwError, "QWindowsPipeReader::consumePendingAndEmit"_L1);
        if (alive)
            emit pipeClosed();
    }

    return emitReadyRead;
}

/*!
    Updates the read buffer size. Returns \c true, if readyRead()
    should be emitted. Thread-safety should be ensured by the caller.
 */
bool QWindowsPipeReader::consumePending()
{
    if (readyReadPending) {
        readyReadPending = false;
        actualReadBufferSize += pendingReadBytes;
        pendingReadBytes = 0;
        return true;
    }

    return false;
}

/*!
    Returns the number of available bytes in the pipe.
 */
DWORD QWindowsPipeReader::checkPipeState()
{
    DWORD bytes;
    if (PeekNamedPipe(handle, nullptr, 0, nullptr, &bytes, nullptr))
        return bytes;

    lastError = GetLastError();
    return 0;
}

bool QWindowsPipeReader::waitForNotification()
{
    DWORD waitRet;
    do {
        waitRet = WaitForSingleObjectEx(syncHandle, INFINITE, TRUE);
        if (waitRet == WAIT_OBJECT_0)
            return true;

        // Some I/O completion routine was called. Wait some more.
    } while (waitRet == WAIT_IO_COMPLETION);

    return false;
}

QT_END_NAMESPACE

#include "moc_qwindowspipereader_p.cpp"
