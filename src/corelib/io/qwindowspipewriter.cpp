// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwindowspipewriter_p.h"
#include <qcoreapplication.h>
#include <QMutexLocker>
#include <QPointer>

QT_BEGIN_NAMESPACE

QWindowsPipeWriter::QWindowsPipeWriter(HANDLE pipeWriteEnd, QObject *parent)
    : QObject(parent),
      handle(pipeWriteEnd),
      eventHandle(CreateEvent(NULL, FALSE, FALSE, NULL)),
      syncHandle(CreateEvent(NULL, TRUE, FALSE, NULL)),
      waitObject(NULL),
      pendingBytesWrittenValue(0),
      lastError(ERROR_SUCCESS),
      completionState(NoError),
      stopped(true),
      writeSequenceStarted(false),
      bytesWrittenPending(false),
      winEventActPosted(false)
{
    ZeroMemory(&overlapped, sizeof(OVERLAPPED));
    overlapped.hEvent = eventHandle;
    waitObject = CreateThreadpoolWait(waitCallback, this, NULL);
    if (waitObject == NULL)
        qErrnoWarning("QWindowsPipeWriter: CreateThreadpollWait failed.");
}

QWindowsPipeWriter::~QWindowsPipeWriter()
{
    stop();
    CloseThreadpoolWait(waitObject);
    CloseHandle(eventHandle);
    CloseHandle(syncHandle);
}

/*!
    Assigns the handle to this writer. The handle must be valid.
    Call this function if data was buffered before getting the handle.
 */
void QWindowsPipeWriter::setHandle(HANDLE hPipeWriteEnd)
{
    Q_ASSERT(!stopped);

    handle = hPipeWriteEnd;
    QMutexLocker locker(&mutex);
    startAsyncWriteHelper(&locker);
}

/*!
    Stops the asynchronous write sequence.
    If the write sequence is running then the I/O operation is canceled.
 */
void QWindowsPipeWriter::stop()
{
    if (stopped)
        return;

    mutex.lock();
    stopped = true;
    if (writeSequenceStarted) {
        // Trying to disable callback before canceling the operation.
        // Callback invocation is unnecessary here.
        SetThreadpoolWait(waitObject, NULL, NULL);
        if (!CancelIoEx(handle, &overlapped)) {
            const DWORD dwError = GetLastError();
            if (dwError != ERROR_NOT_FOUND) {
                qErrnoWarning(dwError, "QWindowsPipeWriter: CancelIoEx on handle %p failed.",
                              handle);
            }
        }
        writeSequenceStarted = false;
    }
    mutex.unlock();

    WaitForThreadpoolWaitCallbacks(waitObject, TRUE);
}

/*!
    Returns the number of bytes that are waiting to be written.
 */
qint64 QWindowsPipeWriter::bytesToWrite() const
{
    QMutexLocker locker(&mutex);
    return writeBuffer.size() + pendingBytesWrittenValue;
}

/*!
    Returns \c true if async operation is in progress.
*/
bool QWindowsPipeWriter::isWriteOperationActive() const
{
    return completionState == NoError && bytesToWrite() != 0;
}

/*!
    Writes a shallow copy of \a ba to the internal buffer.
 */
void QWindowsPipeWriter::write(const QByteArray &ba)
{
    if (completionState != WriteDisabled)
        writeImpl(ba);
}

/*!
    Writes data to the internal buffer.
 */
void QWindowsPipeWriter::write(const char *data, qint64 size)
{
    if (completionState != WriteDisabled)
        writeImpl(data, size);
}

template <typename... Args>
inline void QWindowsPipeWriter::writeImpl(Args... args)
{
    QMutexLocker locker(&mutex);

    writeBuffer.append(args...);

    if (writeSequenceStarted || (lastError != ERROR_SUCCESS))
        return;

    stopped = false;

    // If we don't have an assigned handle yet, defer writing until
    // setHandle() is called.
    if (handle != INVALID_HANDLE_VALUE)
        startAsyncWriteHelper(&locker);
}

void QWindowsPipeWriter::startAsyncWriteHelper(QMutexLocker<QMutex> *locker)
{
    startAsyncWriteLocked();

    // Do not post the event, if the write operation will be completed asynchronously.
    if (!bytesWrittenPending && lastError == ERROR_SUCCESS)
        return;

    notifyCompleted(locker);
}

/*!
    Starts a new write sequence.
 */
void QWindowsPipeWriter::startAsyncWriteLocked()
{
    while (!writeBuffer.isEmpty()) {
        // WriteFile() returns true, if the write operation completes synchronously.
        // We don't need to call GetOverlappedResult() additionally, because
        // 'numberOfBytesWritten' is valid in this case.
        DWORD numberOfBytesWritten;
        DWORD errorCode = ERROR_SUCCESS;
        if (!WriteFile(handle, writeBuffer.readPointer(), writeBuffer.nextDataBlockSize(),
                       &numberOfBytesWritten, &overlapped)) {
            errorCode = GetLastError();
            if (errorCode == ERROR_IO_PENDING) {
                // Operation has been queued and will complete in the future.
                writeSequenceStarted = true;
                SetThreadpoolWait(waitObject, eventHandle, NULL);
                break;
            }
        }

        if (!writeCompleted(errorCode, numberOfBytesWritten))
            break;
    }
}

/*!
    \internal
    Thread pool callback procedure.
 */
void QWindowsPipeWriter::waitCallback(PTP_CALLBACK_INSTANCE instance, PVOID context,
                                      PTP_WAIT wait, TP_WAIT_RESULT waitResult)
{
    Q_UNUSED(instance);
    Q_UNUSED(wait);
    Q_UNUSED(waitResult);
    QWindowsPipeWriter *pipeWriter = reinterpret_cast<QWindowsPipeWriter *>(context);

    // Get the result of the asynchronous operation.
    DWORD numberOfBytesTransfered = 0;
    DWORD errorCode = ERROR_SUCCESS;
    if (!GetOverlappedResult(pipeWriter->handle, &pipeWriter->overlapped,
                             &numberOfBytesTransfered, FALSE))
        errorCode = GetLastError();

    QMutexLocker locker(&pipeWriter->mutex);

    // After the writer was stopped, the only reason why this function can be called is the
    // completion of a cancellation. No signals should be emitted, and no new write sequence
    // should be started in this case.
    if (pipeWriter->stopped)
        return;

    pipeWriter->writeSequenceStarted = false;

    if (pipeWriter->writeCompleted(errorCode, numberOfBytesTransfered))
        pipeWriter->startAsyncWriteLocked();

    // We post the notification even if the write operation failed,
    // to unblock the main thread, in case it is waiting for the event.
    pipeWriter->notifyCompleted(&locker);
}

/*!
    Will be called whenever the write operation completes. Returns \c true if
    no error occurred; otherwise returns \c false.
 */
bool QWindowsPipeWriter::writeCompleted(DWORD errorCode, DWORD numberOfBytesWritten)
{
    switch (errorCode) {
    case ERROR_SUCCESS:
        bytesWrittenPending = true;
        pendingBytesWrittenValue += numberOfBytesWritten;
        writeBuffer.free(numberOfBytesWritten);
        return true;
    case ERROR_PIPE_NOT_CONNECTED: // the other end has closed the pipe
    case ERROR_OPERATION_ABORTED: // the operation was canceled
    case ERROR_NO_DATA: // the pipe is being closed
        break;
    default:
        qErrnoWarning(errorCode, "QWindowsPipeWriter: write failed.");
        break;
    }

    // The buffer is not cleared here, because the write progress
    // should appear on the main thread synchronously.
    lastError = errorCode;
    return false;
}

/*!
    Posts a notification event to the main thread.
 */
void QWindowsPipeWriter::notifyCompleted(QMutexLocker<QMutex> *locker)
{
    if (!winEventActPosted) {
        winEventActPosted = true;
        locker->unlock();
        QCoreApplication::postEvent(this, new QEvent(QEvent::WinEventAct));
    } else {
        locker->unlock();
    }

    // We set the event only after unlocking to avoid additional context
    // switches due to the released thread immediately running into the lock.
    SetEvent(syncHandle);
}

/*!
    Receives notification that the write operation has completed.
 */
bool QWindowsPipeWriter::event(QEvent *e)
{
    if (e->type() == QEvent::WinEventAct) {
        consumePendingAndEmit(true);
        return true;
    }
    return QObject::event(e);
}

/*!
    Updates the state and emits pending signals in the main thread.
    Returns \c true, if bytesWritten() was emitted.
 */
bool QWindowsPipeWriter::consumePendingAndEmit(bool allowWinActPosting)
{
    ResetEvent(syncHandle);
    QMutexLocker locker(&mutex);

    // Enable QEvent::WinEventAct posting.
    if (allowWinActPosting)
        winEventActPosted = false;

    const qint64 numberOfBytesWritten = pendingBytesWrittenValue;
    const bool emitBytesWritten = bytesWrittenPending;
    if (emitBytesWritten) {
        bytesWrittenPending = false;
        pendingBytesWrittenValue = 0;
    }
    const DWORD dwError = lastError;

    locker.unlock();

    // Disable any further processing, if the pipe was stopped.
    if (stopped)
        return false;

    // Trigger 'ErrorDetected' state only once. This state must be set before
    // emitting the bytesWritten() signal. Otherwise, the write sequence will
    // be considered not finished, and we may hang if a slot connected
    // to bytesWritten() calls waitForBytesWritten().
    if (dwError != ERROR_SUCCESS && completionState == NoError) {
        QPointer<QWindowsPipeWriter> alive(this);
        completionState = ErrorDetected;
        if (emitBytesWritten)
            emit bytesWritten(numberOfBytesWritten);
        if (alive) {
            writeBuffer.clear();
            completionState = WriteDisabled;
            emit writeFailed();
        }
    } else if (emitBytesWritten) {
        emit bytesWritten(numberOfBytesWritten);
    }

    return emitBytesWritten;
}

QT_END_NAMESPACE

#include "moc_qwindowspipewriter_p.cpp"
