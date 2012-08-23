/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinoverlappedionotifier_p.h"
#include <qdebug.h>
#include <qmutex.h>
#include <qpointer.h>
#include <qset.h>
#include <qthread.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWinOverlappedIoNotifier
    \inmodule QtCore
    \brief The QWinOverlappedIoNotifier class provides support for overlapped I/O notifications on Windows.
    \since 5.0
    \internal

    The QWinOverlappedIoNotifier class makes it possible to use efficient
    overlapped (asynchronous) I/O notifications on Windows by using an
    I/O completion port.

    Once you have obtained a file handle, you can use setHandle() to get
    notifications for I/O operations. Whenever an I/O operation completes,
    the notified() signal is emitted which will pass the number of transferred
    bytes and the operation's error code to the receiver.

    Every handle that supports overlapped I/O can be used by
    QWinOverlappedIoNotifier. That includes file handles, TCP sockets
    and named pipes.

    Note that you must not use ReadFileEx() and WriteFileEx() together
    with QWinOverlappedIoNotifier. They are not supported as they use a
    different I/O notification mechanism.

    The hEvent member in the OVERLAPPED structure passed to ReadFile()
    or WriteFile() is ignored and can be used for other purposes.

    \warning This class is only available on Windows.
*/

class QWinIoCompletionPort : protected QThread
{
public:
    QWinIoCompletionPort()
        : hPort(INVALID_HANDLE_VALUE)
    {
        setObjectName(QLatin1String("I/O completion port thread"));
        HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (!hIOCP) {
            qErrnoWarning("CreateIoCompletionPort failed.");
            return;
        }
        hPort = hIOCP;
    }

    ~QWinIoCompletionPort()
    {
        PostQueuedCompletionStatus(hPort, 0, 0, NULL);
        QThread::wait();
        CloseHandle(hPort);
    }

    void registerNotifier(QWinOverlappedIoNotifier *notifier)
    {
        HANDLE hIOCP = CreateIoCompletionPort(notifier->hHandle, hPort, reinterpret_cast<ULONG_PTR>(notifier), 0);
        if (!hIOCP) {
            qErrnoWarning("Can't associate file handle %x with I/O completion port.", notifier->hHandle);
            return;
        }
        mutex.lock();
        notifiers += notifier;
        mutex.unlock();
        if (!QThread::isRunning())
            QThread::start();
    }

    void unregisterNotifier(QWinOverlappedIoNotifier *notifier)
    {
        mutex.lock();
        notifiers.remove(notifier);
        mutex.unlock();
    }

protected:
    void run()
    {
        DWORD dwBytesRead;
        ULONG_PTR pulCompletionKey;
        OVERLAPPED *overlapped;

        forever {
            BOOL success = GetQueuedCompletionStatus(hPort,
                                                     &dwBytesRead,
                                                     &pulCompletionKey,
                                                     &overlapped,
                                                     INFINITE);

            DWORD errorCode = success ? ERROR_SUCCESS : GetLastError();
            if (!success && !overlapped) {
                qErrnoWarning(errorCode, "GetQueuedCompletionStatus failed.");
                return;
            }

            if (success && !(dwBytesRead || pulCompletionKey || overlapped)) {
                // We've posted null values via PostQueuedCompletionStatus to end this thread.
                return;
            }

            QWinOverlappedIoNotifier *notifier = reinterpret_cast<QWinOverlappedIoNotifier *>(pulCompletionKey);
            mutex.lock();
            if (notifiers.contains(notifier))
                notifier->notify(dwBytesRead, errorCode);
            mutex.unlock();
        }
    }

private:
    HANDLE hPort;
    QSet<QWinOverlappedIoNotifier *> notifiers;
    QMutex mutex;
};

Q_GLOBAL_STATIC(QWinIoCompletionPort, iocp)

QWinOverlappedIoNotifier::QWinOverlappedIoNotifier(QObject *parent)
    : QObject(parent),
      hHandle(INVALID_HANDLE_VALUE),
      lastNumberOfBytes(0),
      lastErrorCode(ERROR_SUCCESS)
{
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    connect(this, &QWinOverlappedIoNotifier::_q_notify,
            this, &QWinOverlappedIoNotifier::_q_notified, Qt::QueuedConnection);
}

QWinOverlappedIoNotifier::~QWinOverlappedIoNotifier()
{
    setEnabled(false);
    CloseHandle(hEvent);
}

void QWinOverlappedIoNotifier::setHandle(HANDLE h)
{
    hHandle = h;
}

void QWinOverlappedIoNotifier::setEnabled(bool enabled)
{
    if (enabled)
        iocp()->registerNotifier(this);
    else
        iocp()->unregisterNotifier(this);
}

bool QWinOverlappedIoNotifier::waitForNotified(int msecs)
{
    DWORD result = WaitForSingleObject(hEvent, msecs == -1 ? INFINITE : DWORD(msecs));
    switch (result) {
    case WAIT_OBJECT_0:
        _q_notified();
        return true;
    case WAIT_TIMEOUT:
        return false;
    }

    qErrnoWarning("QWinOverlappedIoNotifier::waitForNotified: WaitForSingleObject failed.");
    return false;
}

/*!
  * Note: This function runs in the I/O completion port thread.
  */
void QWinOverlappedIoNotifier::notify(DWORD numberOfBytes, DWORD errorCode)
{
    lastNumberOfBytes = numberOfBytes;
    lastErrorCode = errorCode;
    SetEvent(hEvent);
    emit _q_notify();
}

void QWinOverlappedIoNotifier::_q_notified()
{
    if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
        ResetEvent(hEvent);
        emit notified(lastNumberOfBytes, lastErrorCode);
    }
}

QT_END_NAMESPACE
