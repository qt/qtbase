/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QtCore>
#include <QtTest/QtTest>

#include <math.h>


class tst_QWaitCondition : public QObject
{
    Q_OBJECT

public:
    tst_QWaitCondition()
    {
    }

private slots:
    void oscillate_data();
    void oscillate();

    void thrash_data();
    void thrash();

public:
    static QWaitCondition local, remote;
    enum Turn {LocalTurn, RemoteTurn};
    static Turn turn;
};

QWaitCondition tst_QWaitCondition::local;
QWaitCondition tst_QWaitCondition::remote;
tst_QWaitCondition::Turn tst_QWaitCondition::turn = tst_QWaitCondition::LocalTurn;

class OscillateThread : public QThread
{
public:
    bool m_done;
    bool m_useMutex;
    unsigned long m_timeout;
    bool m_wakeOne;
    int count;

    OscillateThread(bool useMutex, unsigned long timeout, bool wakeOne)
    : m_done(false), m_useMutex(useMutex), m_timeout(timeout), m_wakeOne(wakeOne)
        {}
    void run()
    {
        QMutex mtx;
        QReadWriteLock rwl;
        count = 0;

        forever {
            if (m_done)
                break;
            if (m_useMutex) {
                mtx.lock();
                while (tst_QWaitCondition::turn == tst_QWaitCondition::LocalTurn)
                    tst_QWaitCondition::remote.wait(&mtx, m_timeout);
                mtx.unlock();
            } else {
                rwl.lockForWrite();
                while (tst_QWaitCondition::turn == tst_QWaitCondition::LocalTurn)
                    tst_QWaitCondition::remote.wait(&rwl, m_timeout);
                rwl.unlock();
            }
            tst_QWaitCondition::turn = tst_QWaitCondition::LocalTurn;
            if (m_wakeOne)
                tst_QWaitCondition::local.wakeOne();
            else
                tst_QWaitCondition::local.wakeAll();
            count++;
        }
    }
};

void tst_QWaitCondition::oscillate_data()
{
    QTest::addColumn<bool>("useMutex");
    QTest::addColumn<unsigned long>("timeout");
    QTest::addColumn<bool>("wakeOne");

    QTest::newRow("mutex, timeout, one") << true << 1000ul << true;
    QTest::newRow("readWriteLock, timeout, one") << false << 1000ul << true;
    QTest::newRow("mutex, timeout, all") << true << 1000ul << false;
    QTest::newRow("readWriteLock, timeout, all") << false << 1000ul << false;
    QTest::newRow("mutex, forever, one") << true << ULONG_MAX << true;
    QTest::newRow("readWriteLock, forever, one") << false << ULONG_MAX << true;
    QTest::newRow("mutex, forever, all") << true << ULONG_MAX << false;
    QTest::newRow("readWriteLock, forever, all") << false << ULONG_MAX << false;
}

void tst_QWaitCondition::oscillate()
{
    QMutex mtx;
    QReadWriteLock rwl;

    QFETCH(bool, useMutex);
    QFETCH(unsigned long, timeout);
    QFETCH(bool, wakeOne);

    turn = LocalTurn;
    OscillateThread thrd(useMutex, timeout, wakeOne);
    thrd.start();

    QBENCHMARK {
        if (useMutex)
            mtx.lock();
        else
            rwl.lockForWrite();
        turn = RemoteTurn;
        if (wakeOne)
            remote.wakeOne();
        else
            remote.wakeAll();
        if (useMutex) {
            while (turn == RemoteTurn)
                local.wait(&mtx, timeout);
            mtx.unlock();
        } else {
            while (turn == RemoteTurn)
                local.wait(&rwl, timeout);
            rwl.unlock();
        }
    }

    thrd.m_done = true;
    remote.wakeAll();
    thrd.wait();

    QCOMPARE(0, 0);
}

void tst_QWaitCondition::thrash_data()
{
    oscillate_data();
}

void tst_QWaitCondition::thrash()
{
    QMutex mtx;
    mtx.lock();

    QFETCH(bool, useMutex);
    QFETCH(unsigned long, timeout);
    QFETCH(bool, wakeOne);

    turn = LocalTurn;
    OscillateThread thrd(useMutex, timeout, wakeOne);
    thrd.start();
    local.wait(&mtx, 1000ul);
    mtx.unlock();

    QBENCHMARK {
        turn = RemoteTurn;
        if (wakeOne)
            remote.wakeOne();
        else
            remote.wakeAll();
    }

    thrd.m_done = true;
    turn = RemoteTurn;
    remote.wakeAll();
    thrd.wait();

    QCOMPARE(0, 0);
}

QTEST_MAIN(tst_QWaitCondition)
#include "tst_qwaitcondition.moc"
