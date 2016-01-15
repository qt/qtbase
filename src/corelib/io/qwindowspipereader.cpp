/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qwindowspipereader_p.h"
#include "qwinoverlappedionotifier_p.h"
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qeventloop.h>

QT_BEGIN_NAMESPACE

QWindowsPipeReader::QWindowsPipeReader(QObject *parent)
    : QObject(parent),
      handle(INVALID_HANDLE_VALUE),
      readBufferMaxSize(0),
      actualReadBufferSize(0),
      stopped(true),
      readSequenceStarted(false),
      pipeBroken(false),
      readyReadEmitted(false)
{
    dataReadNotifier = new QWinOverlappedIoNotifier(this);
    connect(dataReadNotifier, &QWinOverlappedIoNotifier::notified, this, &QWindowsPipeReader::notified);
}

static bool qt_cancelIo(HANDLE handle, OVERLAPPED *overlapped)
{
    typedef BOOL (WINAPI *PtrCancelIoEx)(HANDLE, LPOVERLAPPED);
    static PtrCancelIoEx ptrCancelIoEx = 0;
    if (!ptrCancelIoEx) {
        HMODULE kernel32 = GetModuleHandleA("kernel32");
        if (kernel32)
            ptrCancelIoEx = PtrCancelIoEx(GetProcAddress(kernel32, "CancelIoEx"));
    }
    if (ptrCancelIoEx)
        return ptrCancelIoEx(handle, overlapped);
    else
        return CancelIo(handle);
}

QWindowsPipeReader::~QWindowsPipeReader()
{
    stop();
}

/*!
    Sets the handle to read from. The handle must be valid.
 */
void QWindowsPipeReader::setHandle(HANDLE hPipeReadEnd)
{
    readBuffer.clear();
    actualReadBufferSize = 0;
    handle = hPipeReadEnd;
    pipeBroken = false;
    readyReadEmitted = false;
    stopped = false;
    if (hPipeReadEnd != INVALID_HANDLE_VALUE) {
        dataReadNotifier->setHandle(hPipeReadEnd);
        dataReadNotifier->setEnabled(true);
    }
}

/*!
    Stops the asynchronous read sequence.
    If the read sequence is running then the I/O operation is canceled.
 */
void QWindowsPipeReader::stop()
{
    stopped = true;
    if (readSequenceStarted) {
        if (qt_cancelIo(handle, &overlapped)) {
            dataReadNotifier->waitForNotified(-1, &overlapped);
        } else {
            const DWORD dwError = GetLastError();
            if (dwError != ERROR_NOT_FOUND) {
                qErrnoWarning(dwError, "QWindowsPipeReader: qt_cancelIo on handle %x failed.",
                              handle);
            }
        }
    }
    readSequenceStarted = false;
    dataReadNotifier->setEnabled(false);
    handle = INVALID_HANDLE_VALUE;
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
    if (pipeBroken && actualReadBufferSize == 0)
        return 0;  // signal EOF

    qint64 readSoFar;
    // If startAsyncRead() has read data, copy it to its destination.
    if (maxlen == 1 && actualReadBufferSize > 0) {
        *data = readBuffer.getChar();
        actualReadBufferSize--;
        readSoFar = 1;
    } else {
        qint64 bytesToRead = qMin(actualReadBufferSize, maxlen);
        readSoFar = 0;
        while (readSoFar < bytesToRead) {
            const char *ptr = readBuffer.readPointer();
            qint64 bytesToReadFromThisBlock = qMin(bytesToRead - readSoFar,
                                                   readBuffer.nextDataBlockSize());
            memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
            readSoFar += bytesToReadFromThisBlock;
            readBuffer.free(bytesToReadFromThisBlock);
            actualReadBufferSize -= bytesToReadFromThisBlock;
        }
    }

    if (!pipeBroken) {
        if (!readSequenceStarted && !stopped)
            startAsyncRead();
        if (readSoFar == 0)
            return -2;      // signal EWOULDBLOCK
    }

    return readSoFar;
}

bool QWindowsPipeReader::canReadLine() const
{
    return readBuffer.indexOf('\n', actualReadBufferSize) >= 0;
}

/*!
    \internal
    Will be called whenever the read operation completes.
 */
void QWindowsPipeReader::notified(quint32 numberOfBytesRead, quint32 errorCode,
                                  OVERLAPPED *notifiedOverlapped)
{
    if (&overlapped != notifiedOverlapped)
        return;

    switch (errorCode) {
    case ERROR_SUCCESS:
        break;
    case ERROR_MORE_DATA:
        // This is not an error. We're connected to a message mode
        // pipe and the message didn't fit into the pipe's system
        // buffer. We will read the remaining data in the next call.
        break;
    case ERROR_BROKEN_PIPE:
    case ERROR_PIPE_NOT_CONNECTED:
        pipeBroken = true;
        break;
    case ERROR_OPERATION_ABORTED:
        if (stopped)
            break;
        // fall through
    default:
        emit winError(errorCode, QLatin1String("QWindowsPipeReader::notified"));
        pipeBroken = true;
        break;
    }

    readSequenceStarted = false;

    // After the reader was stopped, the only reason why this function can be called is the
    // completion of a cancellation. No signals should be emitted, and no new read sequence should
    // be started in this case.
    if (stopped)
        return;

    if (pipeBroken) {
        emit pipeClosed();
        return;
    }

    actualReadBufferSize += numberOfBytesRead;
    readBuffer.truncate(actualReadBufferSize);
    startAsyncRead();
    readyReadEmitted = true;
    emit readyRead();
}

/*!
    \internal
    Reads data from the socket into the readbuffer
 */
void QWindowsPipeReader::startAsyncRead()
{
    const DWORD minReadBufferSize = 4096;
    DWORD bytesToRead = qMax(checkPipeState(), minReadBufferSize);
    if (pipeBroken)
        return;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
        bytesToRead = readBufferMaxSize - readBuffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the pipe.
            return;
        }
    }

    char *ptr = readBuffer.reserve(bytesToRead);

    readSequenceStarted = true;
    ZeroMemory(&overlapped, sizeof(overlapped));
    if (ReadFile(handle, ptr, bytesToRead, NULL, &overlapped)) {
        // We get notified by the QWinOverlappedIoNotifier - even in the synchronous case.
        return;
    } else {
        const DWORD dwError = GetLastError();
        switch (dwError) {
        case ERROR_IO_PENDING:
            // This is not an error. We're getting notified, when data arrives.
            return;
        case ERROR_MORE_DATA:
            // This is not an error. The synchronous read succeeded.
            // We're connected to a message mode pipe and the message
            // didn't fit into the pipe's system buffer.
            // We're getting notified by the QWinOverlappedIoNotifier.
            break;
        case ERROR_BROKEN_PIPE:
        case ERROR_PIPE_NOT_CONNECTED:
            {
                // It may happen, that the other side closes the connection directly
                // after writing data. Then we must set the appropriate socket state.
                readSequenceStarted = false;
                pipeBroken = true;
                emit pipeClosed();
                return;
            }
        default:
            readSequenceStarted = false;
            emit winError(dwError, QLatin1String("QWindowsPipeReader::startAsyncRead"));
            return;
        }
    }
}

/*!
    \internal
    Returns the number of available bytes in the pipe.
    Sets QWindowsPipeReader::pipeBroken to true if the connection is broken.
 */
DWORD QWindowsPipeReader::checkPipeState()
{
    DWORD bytes;
    if (PeekNamedPipe(handle, NULL, 0, NULL, &bytes, NULL)) {
        return bytes;
    } else {
        if (!pipeBroken) {
            pipeBroken = true;
            emit pipeClosed();
        }
    }
    return 0;
}

/*!
    Waits for the completion of the asynchronous read operation.
    Returns \c true, if we've emitted the readyRead signal.
 */
bool QWindowsPipeReader::waitForReadyRead(int msecs)
{
    if (!readSequenceStarted)
        return false;
    readyReadEmitted = false;
    dataReadNotifier->waitForNotified(msecs, &overlapped);
    return readyReadEmitted;
}

/*!
    Waits until the pipe is closed.
 */
bool QWindowsPipeReader::waitForPipeClosed(int msecs)
{
    const int sleepTime = 10;
    QElapsedTimer stopWatch;
    stopWatch.start();
    forever {
        waitForReadyRead(0);
        checkPipeState();
        if (pipeBroken)
            return true;
        if (stopWatch.hasExpired(msecs - sleepTime))
            return false;
        Sleep(sleepTime);
    }
}

QT_END_NAMESPACE
