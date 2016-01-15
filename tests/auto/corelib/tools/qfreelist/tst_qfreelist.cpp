/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QThread>
#include <private/qfreelist_p.h>
#include <QtTest/QtTest>

class tst_QFreeList : public QObject
{
    Q_OBJECT

private slots:
    void basicTest();
    void customized();
    void threadedTest();
};

void tst_QFreeList::basicTest()
{
    {
        QFreeList<void> voidFreeList;
        int zero = voidFreeList.next();
        int one  = voidFreeList.next();
        int two = voidFreeList.next();
        QCOMPARE(zero, 0);
        QCOMPARE(one, 1);
        QCOMPARE(two, 2);
        voidFreeList[zero];
        voidFreeList[one];
        voidFreeList[two];
        voidFreeList.at(zero);
        voidFreeList.at(one);
        voidFreeList.at(two);
        voidFreeList.release(one);
        int next = voidFreeList.next();
        QCOMPARE(next, 1);
        voidFreeList[next];
        voidFreeList.at(next);
    }

    {
        QFreeList<int> intFreeList;
        int zero = intFreeList.next();
        int one = intFreeList.next();
        int two = intFreeList.next();
        QCOMPARE(zero, 0);
        QCOMPARE(one, 1);
        QCOMPARE(two, 2);
        intFreeList[zero] = zero;
        intFreeList[one] = one;
        intFreeList[two] = two;
        QCOMPARE(intFreeList.at(zero), zero);
        QCOMPARE(intFreeList.at(one), one);
        QCOMPARE(intFreeList.at(two), two);
        intFreeList.release(one);
        int next = intFreeList.next();
        QCOMPARE(next, 1);
        QCOMPARE(intFreeList.at(next), one);
        intFreeList[next] = -one;
        QCOMPARE(intFreeList.at(next), -one);
    }
}

struct CustomFreeListConstants : public QFreeListDefaultConstants
{
    enum {
        InitialNextValue = 50,
        BlockCount = 10
    };

    static const int Sizes[10];
};

const int CustomFreeListConstants::Sizes[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 16777216 - 1 - 2 - 3 - 4 - 5 - 6 - 7 - 8 - 9 };

void tst_QFreeList::customized()
{
    QFreeList<void, CustomFreeListConstants> customFreeList;
    int next = customFreeList.next();
    QCOMPARE(next, int(CustomFreeListConstants::InitialNextValue));
    customFreeList[next];
    customFreeList.at(next);
    customFreeList.release(next);
}

enum { TimeLimit = 3000 };

class FreeListThread : public QThread
{
    static QFreeList<void> freelist;

public:
    inline FreeListThread() : QThread() { }
    inline void run()
    {
        QElapsedTimer t;
        t.start();
        QList<int> needToRelease;
        do {
            int i = freelist.next();
            int j = freelist.next();
            int k = freelist.next();
            int l = freelist.next();
            freelist.release(k);
            int n = freelist.next();
            int m = freelist.next();
            freelist.release(l);
            freelist.release(m);
            freelist.release(n);
            freelist.release(j);
            // freelist.release(i);
            needToRelease << i;
        } while (t.elapsed() < TimeLimit);

        foreach (int x, needToRelease)
            freelist.release(x);
    }
};

QFreeList<void> FreeListThread::freelist;

void tst_QFreeList::threadedTest()
{
    const int ThreadCount = QThread::idealThreadCount();
    FreeListThread *threads = new FreeListThread[ThreadCount];
    for (int i = 0; i < ThreadCount; ++i)
        threads[i].start();
    for (int i = 0; i < ThreadCount; ++i)
        threads[i].wait();
    delete [] threads;
}

QTEST_MAIN(tst_QFreeList)
#include "tst_qfreelist.moc"
