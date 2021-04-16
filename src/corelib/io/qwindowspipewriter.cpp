/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qwindowspipewriter_p.h"
#include <qcoreapplication.h>
#include <QMutexLocker>

QT_BEGIN_NAMESPACE

QWindowsPipeWriter::QWindowsPipeWriter(HANDLE pipeWriteEnd, QObject *parent)
    : QObject(parent),
      handle(pipeWriteEnd),
      eventHandle(CreateEvent(NULL, FALSE, FALSE, NULL)),
      syncHandle(CreateEvent(NULL, TRUE, FALSE, NULL)),
      waitObject(NULL),
      pendingBytesWrittenValue(0),
      lastError(ERROR_SUCCESS),
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
    Returns \c true if async operation is in progress or a bytesWritten
    signal is pending.
 */
bool QWindowsPipeWriter::isWriteOperationActive() const
{
    QMutexLocker locker(&mutex);
    return writeSequenceStarted || bytesWrittenPending;
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
    Writes data to the pipe.
 */
bool QWindowsPipeWriter::write(const QByteArray &ba)
{
    QMutexLocker locker(&mutex);

    if (lastError != ERROR_SUCCESS)
        return false;

    writeBuffer.append(ba);
    if (writeSequenceStarted)
        return true;

    stopped = false;
    startAsyncWriteLocked();

    // Do not post the event, if the write operation will be completed asynchronously.
    if (!bytesWrittenPending)
        return true;

    if (!winEventActPosted) {
        winEventActPosted = true;
        locker.unlock();
        QCoreApplication::postEvent(this, new QEvent(QEvent::WinEventAct));
    } else {
        locker.unlock();
    }

    SetEvent(syncHandle);
    return true;
}

/*!
    Starts a new write sequence. Thread-safety should be ensured by the caller.
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
                return;
            }
        }

        if (!writeCompleted(errorCode, numberOfBytesWritten))
            return;
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

    if (pipeWriter->lastError == ERROR_SUCCESS && !pipeWriter->winEventActPosted) {
        pipeWriter->winEventActPosted = true;
        locker.unlock();
        QCoreApplication::postEvent(pipeWriter, new QEvent(QEvent::WinEventAct));
    } else {
        locker.unlock();
    }

    // We set the event only after unlocking to avoid additional context
    // switches due to the released thread immediately running into the lock.
    SetEvent(pipeWriter->syncHandle);
}

/*!
    Will be called whenever the write operation completes. Returns \c true if
    no error occurred; otherwise returns \c false.
 */
bool QWindowsPipeWriter::writeCompleted(DWORD errorCode, DWORD numberOfBytesWritten)
{
    if (errorCode == ERROR_SUCCESS) {
        Q_ASSERT(numberOfBytesWritten == DWORD(writeBuffer.nextDataBlockSize()));

        bytesWrittenPending = true;
        pendingBytesWrittenValue += numberOfBytesWritten;
        writeBuffer.free(numberOfBytesWritten);
        return true;
    }

    lastError = errorCode;
    writeBuffer.clear();
    // The other end has closed the pipe. This can happen in QLocalSocket. Do not warn.
    if (errorCode != ERROR_OPERATION_ABORTED && errorCode != ERROR_NO_DATA)
        qErrnoWarning(errorCode, "QWindowsPipeWriter: write failed.");
    return false;
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

    if (!bytesWrittenPending)
        return false;

    // Reset the state even if we don't emit bytesWritten().
    // It's a defined behavior to not re-emit this signal recursively.
    bytesWrittenPending = false;
    qint64 numberOfBytesWritten = pendingBytesWrittenValue;
    pendingBytesWrittenValue = 0;

    locker.unlock();

    // Disable any further processing, if the pipe was stopped.
    if (stopped)
        return false;

    emit bytesWritten(numberOfBytesWritten);
    return true;
}

bool QWindowsPipeWriter::waitForNotification(const QDeadlineTimer &deadline)
{
    do {
        DWORD waitRet = WaitForSingleObjectEx(syncHandle, deadline.remainingTime(), TRUE);
        if (waitRet == WAIT_OBJECT_0)
            return true;

        if (waitRet != WAIT_IO_COMPLETION)
            return false;

        // Some I/O completion routine was called. Wait some more.
    } while (!deadline.hasExpired());

    return false;
}

/*!
    Waits for the completion of the asynchronous write operation.
    Returns \c true, if we've emitted the bytesWritten signal.
 */
bool QWindowsPipeWriter::waitForWrite(int msecs)
{
    QDeadlineTimer timer(msecs);

    // Make sure that 'syncHandle' was triggered by the thread pool callback.
    while (isWriteOperationActive() && waitForNotification(timer)) {
        if (consumePendingAndEmit(false))
            return true;
    }

    return false;
}

QT_END_NAMESPACE
