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
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_THREAD

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

class Q_CORE_EXPORT QWindowsPipeWriter : public QThread
{
    Q_OBJECT

Q_SIGNALS:
    void canWrite();
    void bytesWritten(qint64 bytes);

public:
    explicit QWindowsPipeWriter(HANDLE writePipe, QObject * parent = 0);
    ~QWindowsPipeWriter();

    bool waitForWrite(int msecs);
    qint64 write(const char *data, qint64 maxlen);

    qint64 bytesToWrite() const
    {
        QMutexLocker locker(&lock);
        return data.size();
    }

    bool hadWritten() const
    {
        return hasWritten;
    }

protected:
   void run();

private:
    QByteArray data;
    QWaitCondition waitCondition;
    mutable QMutex lock;
    HANDLE writePipe;
    volatile bool quitNow;
    bool hasWritten;
};

#endif //QT_NO_THREAD

QT_END_NAMESPACE

#endif // QT_NO_PROCESS
