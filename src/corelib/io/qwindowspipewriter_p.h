/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSPIPEWRITER_P_H
#define QWINDOWSPIPEWRITER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qelapsedtimer.h>
#include <qobject.h>
#include <qbytearray.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

#define SLEEPMIN 10
#define SLEEPMAX 500

class QIncrementalSleepTimer
{

public:
    QIncrementalSleepTimer(int msecs)
        : totalTimeOut(msecs)
        , nextSleep(qMin(SLEEPMIN, totalTimeOut))
    {
        if (totalTimeOut == -1)
            nextSleep = SLEEPMIN;
        timer.start();
    }

    int nextSleepTime()
    {
        int tmp = nextSleep;
        nextSleep = qMin(nextSleep * 2, qMin(SLEEPMAX, timeLeft()));
        return tmp;
    }

    int timeLeft() const
    {
        if (totalTimeOut == -1)
            return SLEEPMAX;
        return qMax(int(totalTimeOut - timer.elapsed()), 0);
    }

    bool hasTimedOut() const
    {
        if (totalTimeOut == -1)
            return false;
        return timer.elapsed() >= totalTimeOut;
    }

    void resetIncrements()
    {
        nextSleep = qMin(SLEEPMIN, timeLeft());
    }

private:
    QElapsedTimer timer;
    int totalTimeOut;
    int nextSleep;
};

class Q_CORE_EXPORT QWindowsPipeWriter : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsPipeWriter(HANDLE pipeWriteEnd, QObject *parent = 0);
    ~QWindowsPipeWriter();

    bool write(const QByteArray &ba);
    void stop();
    bool waitForWrite(int msecs);
    bool isWriteOperationActive() const { return writeSequenceStarted; }
    qint64 bytesToWrite() const;

Q_SIGNALS:
    void canWrite();
    void bytesWritten(qint64 bytes);
    void _q_queueBytesWritten(QPrivateSignal);

private:
    static void CALLBACK writeFileCompleted(DWORD errorCode, DWORD numberOfBytesTransfered,
                                            OVERLAPPED *overlappedBase);
    void notified(DWORD errorCode, DWORD numberOfBytesWritten);
    bool waitForNotification(int timeout);
    void emitPendingBytesWrittenValue();

    class Overlapped : public OVERLAPPED
    {
        Q_DISABLE_COPY(Overlapped)
    public:
        explicit Overlapped(QWindowsPipeWriter *pipeWriter);
        void clear();

        QWindowsPipeWriter *pipeWriter;
    };

    HANDLE handle;
    Overlapped overlapped;
    QByteArray buffer;
    qint64 numberOfBytesToWrite;
    qint64 pendingBytesWrittenValue;
    bool stopped;
    bool writeSequenceStarted;
    bool notifiedCalled;
    bool bytesWrittenPending;
    bool inBytesWritten;
};

QT_END_NAMESPACE

#endif // QT_NO_PROCESS
