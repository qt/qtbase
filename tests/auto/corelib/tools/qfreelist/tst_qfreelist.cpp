// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QThread>
#include <private/qfreelist_p.h>
#include <QTest>

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
        QCOMPARE_NE(next, 1); // With generation counter
        QCOMPARE(next & QFreeListDefaultConstants::IndexMask, 1);
        voidFreeList[next];
        voidFreeList.at(next);
        voidFreeList.release(next);
        QCOMPARE(voidFreeList.next() & QFreeListDefaultConstants::IndexMask, 1);
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
        QCOMPARE_NE(next, 1); // With generation counter
        QCOMPARE(next & QFreeListDefaultConstants::IndexMask, 1);
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
    inline void run() override
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

        for (int x : std::as_const(needToRelease))
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
