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
#include <QCoreApplication>
#include <QDebug>

#define QFUTURE_TEST

#include <QtTest/QtTest>
#include <qfuture.h>
#include <qfuturewatcher.h>
#include <qresultstore.h>
#include <qthreadpool.h>
#include <qexception.h>
#include <private/qfutureinterface_p.h>

// COM interface macro.
#if defined(Q_OS_WIN) && defined(interface)
#  undef interface
#endif

class tst_QFuture: public QObject
{
    Q_OBJECT
private slots:
    void resultStore();
    void future();
    void futureInterface();
    void refcounting();
    void cancel();
    void statePropagation();
    void multipleResults();
    void indexedResults();
    void progress();
    void progressText();
    void resultsAfterFinished();
    void resultsAsList();
    void implicitConversions();
    void iterators();
    void pause();
    void throttling();
    void voidConversions();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
    void nestedExceptions();
#endif
    void nonGlobalThreadPool();
};

void tst_QFuture::resultStore()
{
    int int0 = 0;
    int int1 = 1;
    int int2 = 2;

    {
        QtPrivate::ResultStore<int> store;
        QCOMPARE(store.begin(), store.end());
        QCOMPARE(store.resultAt(0), store.end());
        QCOMPARE(store.resultAt(1), store.end());
    }


    {
        QtPrivate::ResultStoreBase store;
        store.addResult(-1, &int0); // note to self: adding a pointer to the stack here is ok since
        store.addResult(1, &int1);  // ResultStoreBase does not take ownership, only ResultStore<> does.
        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QVERIFY(it == store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QVERIFY(it != store.begin());
        QVERIFY(it == store.end());
    }

    QVector<int> vec0 = QVector<int>() << 2 << 3;
    QVector<int> vec1 = QVector<int>() << 4 << 5;

    {
        QtPrivate::ResultStoreBase store;
        store.addResults(-1, &vec0, 2, 2);
        store.addResults(-1, &vec1, 2, 2);
        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QCOMPARE(it, store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 2);

        ++it;
        QCOMPARE(it.resultIndex(), 3);

        ++it;
        QCOMPARE(it, store.end());
    }
    {
        QtPrivate::ResultStoreBase store;
        store.addResult(-1, &int0);
        store.addResults(-1, &vec1, 2, 2);
        store.addResult(-1, &int1);

        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QVERIFY(it == store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 2);
        QVERIFY(it != store.end());
        ++it;
        QCOMPARE(it.resultIndex(), 3);
        QVERIFY(it != store.end());
        ++it;
        QVERIFY(it == store.end());

        QCOMPARE(store.resultAt(0).resultIndex(), 0);
        QCOMPARE(store.resultAt(1).resultIndex(), 1);
        QCOMPARE(store.resultAt(2).resultIndex(), 2);
        QCOMPARE(store.resultAt(3).resultIndex(), 3);
        QCOMPARE(store.resultAt(4), store.end());
    }
    {
        QtPrivate::ResultStore<int> store;
        store.addResult(-1, &int0);
        store.addResults(-1, &vec0);
        store.addResult(-1, &int1);

        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QVERIFY(it == store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 2);
        QVERIFY(it != store.end());
        ++it;
        QCOMPARE(it.resultIndex(), 3);
        QVERIFY(it != store.end());
        ++it;
        QVERIFY(it == store.end());

        QCOMPARE(store.resultAt(0).value(), int0);
        QCOMPARE(store.resultAt(1).value(), vec0[0]);
        QCOMPARE(store.resultAt(2).value(), vec0[1]);
        QCOMPARE(store.resultAt(3).value(), int1);
    }
    {
        QtPrivate::ResultStore<int> store;
        store.addResult(-1, &int0);
        store.addResults(-1, &vec0);
        store.addResult(200, &int1);

        QCOMPARE(store.resultAt(0).value(), int0);
        QCOMPARE(store.resultAt(1).value(), vec0[0]);
        QCOMPARE(store.resultAt(2).value(), vec0[1]);
        QCOMPARE(store.resultAt(200).value(), int1);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.addResult(1, &int1);
        store.addResult(0, &int0);
        store.addResult(-1, &int2);

        QCOMPARE(store.resultAt(0).value(), int0);
        QCOMPARE(store.resultAt(1).value(), int1);
        QCOMPARE(store.resultAt(2).value(), int2);
    }

    {
        QtPrivate::ResultStore<int> store;
        QCOMPARE(store.contains(0), false);
        QCOMPARE(store.contains(1), false);
        QCOMPARE(store.contains(INT_MAX), false);
    }

    {
        // Test filter mode, where "gaps" in the result array aren't allowed.
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);

        store.addResult(0, &int0);
        QCOMPARE(store.contains(0), true);

        store.addResult(2, &int2); // add result at index 2
        QCOMPARE(store.contains(2), false); // but 1 is missing, so this 2 won't be reported yet.

        store.addResult(1, &int1);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), true); // 2 should be visible now.

        store.addResult(4, &int0);
        store.addResult(5, &int0);
        store.addResult(7, &int0);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(7), false);

        store.addResult(3, &int0);  // adding 3 makes 4 and 5 visible
        QCOMPARE(store.contains(4), true);
        QCOMPARE(store.contains(5), true);
        QCOMPARE(store.contains(7), false);

        store.addResult(6, &int0);  // adding 6 makes 7 visible

        QCOMPARE(store.contains(6), true);
        QCOMPARE(store.contains(7), true);
        QCOMPARE(store.contains(8), false);
    }

    {
        // test canceled results
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);

        store.addResult(0, &int0);
        QCOMPARE(store.contains(0), true);

        store.addResult(2, &int0);
        QCOMPARE(store.contains(2), false);

        store.addCanceledResult(1); // report no result at 1

        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), true); // 2 gets renamed to 1
        QCOMPARE(store.contains(2), false);

        store.addResult(3, &int0);
        QCOMPARE(store.contains(2), true); //3 gets renamed to 2

        store.addResult(6, &int0);
        store.addResult(7, &int0);
        QCOMPARE(store.contains(3), false);

        store.addCanceledResult(4);
        store.addCanceledResult(5);

        QCOMPARE(store.contains(3), true); //6 gets renamed to 3
        QCOMPARE(store.contains(4), true); //7 gets renamed to 4

        store.addResult(8, &int0);
        QCOMPARE(store.contains(5), true); //8 gets renamed to 4

        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);
    }

    {
        // test addResult return value
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 1); // result 0 becomes available
        QCOMPARE(store.contains(0), true);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 1);
        QCOMPARE(store.contains(2), false);

        store.addCanceledResult(1);
        QCOMPARE(store.count(), 2); // result 2 is renamed to 1 and becomes available

        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), false);

        store.addResult(3, &int0);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.contains(2), true);

        store.addResult(6, &int0);
        QCOMPARE(store.count(), 3);
        store.addResult(7, &int0);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.contains(3), false);

        store.addCanceledResult(4);
        store.addCanceledResult(5);
        QCOMPARE(store.count(), 5); // 6 and 7 is renamed to 3 and 4 and becomes available

        QCOMPARE(store.contains(3), true);
        QCOMPARE(store.contains(4), true);

        store.addResult(8, &int0);
        QCOMPARE(store.contains(5), true);
        QCOMPARE(store.count(), 6);

        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);
    }

    {
        // test resultCount in non-filtered mode. It should always be possible
        // to iterate through the results 0 to resultCount.
        QtPrivate::ResultStore<int> store;
        store.addResult(0, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(2, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.addResult(2, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 4);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 4);
    }
    {
        QtPrivate::ResultStore<int> store;
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 5);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addCanceledResult(2);
        QCOMPARE(store.count(), 4);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults(0, 3);
        QCOMPARE(store.count(), 2);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults(0, 3);
        QCOMPARE(store.count(), 2);  // results at 3 and 4 become available at index 0, 1

        store.addResult(5, &int0);
        QCOMPARE(store.count(), 3);// result 5 becomes available at index 2
    }

    {
        QtPrivate::ResultStore<int> store;
        store.addResult(1, &int0);
        store.addResult(3, &int0);
        store.addResults(6, &vec0);
        QCOMPARE(store.contains(0), false);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), false);
        QCOMPARE(store.contains(3), true);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(6), true);
        QCOMPARE(store.contains(7), true);
    }

    {
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);
        store.addResult(1, &int0);
        store.addResult(3, &int0);
        store.addResults(6, &vec0);
        QCOMPARE(store.contains(0), false);
        QCOMPARE(store.contains(1), false);
        QCOMPARE(store.contains(2), false);
        QCOMPARE(store.contains(3), false);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);

        store.addCanceledResult(0);
        store.addCanceledResult(2);
        store.addCanceledResults(4, 2);

        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), true);
        QCOMPARE(store.contains(3), true);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);
    }
    {
        QtPrivate::ResultStore<int> store;
        store.setFilterMode(true);
        store.addCanceledResult(0);
        QCOMPARE(store.contains(0), false);

        store.addResult(1, &int0);
        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), false);
    }
}

void tst_QFuture::future()
{
    // default constructors
    QFuture<int> intFuture;
    intFuture.waitForFinished();
    QFuture<QString> stringFuture;
    stringFuture.waitForFinished();
    QFuture<void> voidFuture;
    voidFuture.waitForFinished();
    QFuture<void> defaultVoidFuture;
    defaultVoidFuture.waitForFinished();

    // copy constructor
    QFuture<int> intFuture2(intFuture);
    QFuture<void> voidFuture2(defaultVoidFuture);

    // assigmnent operator
    intFuture2 = QFuture<int>();
    voidFuture2 = QFuture<void>();

    // state
    QCOMPARE(intFuture2.isStarted(), true);
    QCOMPARE(intFuture2.isFinished(), true);
}

class IntResult : public QFutureInterface<int>
{
public:
    QFuture<int> run()
    {
        this->reportStarted();
        QFuture<int> future = QFuture<int>(this);

        int res = 10;
        reportFinished(&res);
        return future;
    }
};

int value = 10;

class VoidResult : public QFutureInterfaceBase
{
public:
    QFuture<void> run()
    {
        this->reportStarted();
        QFuture<void> future = QFuture<void>(this);
        reportFinished();
        return future;
    }
};

void tst_QFuture::futureInterface()
{
    {
        QFuture<void> future;
        {
            QFutureInterface<void> i;
            i.reportStarted();
            future = i.future();
            i.reportFinished();
        }
    }
    {
        QFuture<int> future;
        {
            QFutureInterface<int> i;
            i.reportStarted();
            i.reportResult(10);
            future = i.future();
            i.reportFinished();
        }
        QCOMPARE(future.resultAt(0), 10);
    }

    {
        QFuture<int> intFuture;

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), true);

        IntResult result;

        result.reportStarted();
        intFuture = result.future();

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), false);

        result.reportFinished(&value);

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), true);

        int e = intFuture.result();

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), true);
        QCOMPARE(intFuture.isCanceled(), false);

        QCOMPARE(e, value);
        intFuture.waitForFinished();

        IntResult intAlgo;
        intFuture = intAlgo.run();
        QFuture<int> intFuture2(intFuture);
        QCOMPARE(intFuture.result(), value);
        QCOMPARE(intFuture2.result(), value);
        intFuture.waitForFinished();

        VoidResult a;
        a.run().waitForFinished();
    }
}

template <typename T>
void testRefCounting()
{
    QFutureInterface<T> interface;
    QCOMPARE(interface.d->refCount.load(), 1);

    {
        interface.reportStarted();

        QFuture<T> f = interface.future();
        QCOMPARE(interface.d->refCount.load(), 2);

        QFuture<T> f2(f);
        QCOMPARE(interface.d->refCount.load(), 3);

        QFuture<T> f3;
        f3 = f2;
        QCOMPARE(interface.d->refCount.load(), 4);

        interface.reportFinished(0);
        QCOMPARE(interface.d->refCount.load(), 4);
    }

    QCOMPARE(interface.d->refCount.load(), 1);
}

void tst_QFuture::refcounting()
{
    testRefCounting<int>();
}

void tst_QFuture::cancel()
{
    {
        QFuture<void> f;
        QFutureInterface<void> result;

        result.reportStarted();
        f = result.future();
        QVERIFY(!f.isCanceled());
        result.reportCanceled();
        QVERIFY(f.isCanceled());
        result.reportFinished();
        QVERIFY(f.isCanceled());
        f.waitForFinished();
        QVERIFY(f.isCanceled());
    }

    // Cancel from the QFuture side and test if the result
    // interface detects it.
    {
        QFutureInterface<void> result;

        QFuture<void> f;
        QVERIFY(f.isStarted());

        result.reportStarted();
        f = result.future();

        QVERIFY(f.isStarted());

        QVERIFY(!result.isCanceled());
        f.cancel();

        QVERIFY(result.isCanceled());

        result.reportFinished();
    }

    // Test that finished futures can be canceled.
    {
        QFutureInterface<void> result;

        QFuture<void> f;
        QVERIFY(f.isStarted());

        result.reportStarted();
        f = result.future();

        QVERIFY(f.isStarted());

        result.reportFinished();

        f.cancel();

        QVERIFY(result.isCanceled());
        QVERIFY(f.isCanceled());
    }

    // Results reported after canceled is called should not be propagated.
    {

        QFutureInterface<int> futureInterface;
        futureInterface.reportStarted();
        QFuture<int> f = futureInterface.future();

        int result = 0;
        futureInterface.reportResult(&result);
        result = 1;
        futureInterface.reportResult(&result);
        f.cancel();
        result = 2;
        futureInterface.reportResult(&result);
        result = 3;
        futureInterface.reportResult(&result);
        futureInterface.reportFinished();
        QCOMPARE(f.results(), QList<int>());
    }
}

void tst_QFuture::statePropagation()
{
    QFuture<void> f1;
    QFuture<void> f2;

    QCOMPARE(f1.isStarted(), true);

    QFutureInterface<void> result;
    result.reportStarted();
    f1 = result.future();

    f2 = f1;

    QCOMPARE(f2.isStarted(), true);

    result.reportCanceled();

    QCOMPARE(f2.isStarted(), true);
    QCOMPARE(f2.isCanceled(), true);

    QFuture<void> f3 = f2;

    QCOMPARE(f3.isStarted(), true);
    QCOMPARE(f3.isCanceled(), true);

    result.reportFinished();

    QCOMPARE(f2.isStarted(), true);
    QCOMPARE(f2.isCanceled(), true);

    QCOMPARE(f3.isStarted(), true);
    QCOMPARE(f3.isCanceled(), true);
}

/*
    Tests that a QFuture can return multiple results.
*/
void tst_QFuture::multipleResults()
{
    IntResult a;
    a.reportStarted();
    QFuture<int> f = a.future();

    QFuture<int> copy = f;
    int result;

    result = 1;
    a.reportResult(&result);
    QCOMPARE(f.resultAt(0), 1);

    result = 2;
    a.reportResult(&result);
    QCOMPARE(f.resultAt(1), 2);

    result = 3;
    a.reportResult(&result);

    result = 4;
    a.reportFinished(&result);

    QCOMPARE(f.results(), QList<int>() << 1 << 2 << 3 << 4);

    // test foreach
    QList<int> fasit = QList<int>() << 1 << 2 << 3 << 4;
    {
        QList<int> results;
        foreach(int result, f)
            results.append(result);
        QCOMPARE(results, fasit);
    }
    {
        QList<int> results;
        foreach(int result, copy)
            results.append(result);
        QCOMPARE(results, fasit);
    }
}

/*
    Test out-of-order result reporting using indexes
*/
void tst_QFuture::indexedResults()
{
    {
        QFutureInterface<QChar> Interface;
        QFuture<QChar> f;
        QVERIFY(f.isStarted());

        Interface.reportStarted();
        f = Interface.future();

        QVERIFY(f.isStarted());

        QChar result;

        result = 'B';
        Interface.reportResult(&result, 1);

        QCOMPARE(f.resultAt(1), result);

        result = 'A';
        Interface.reportResult(&result, 0);
        QCOMPARE(f.resultAt(0), result);

        result = 'C';
        Interface.reportResult(&result); // no index
        QCOMPARE(f.resultAt(2), result);

        Interface.reportFinished();

        QCOMPARE(f.results(), QList<QChar>() << 'A' << 'B' << 'C');
    }

    {
        // Test result reporting with a missing result in the middle
        QFutureInterface<int> Interface;
        Interface.reportStarted();
        QFuture<int> f = Interface.future();
        int result;

        result = 0;
        Interface.reportResult(&result, 0);
        QVERIFY(f.isResultReadyAt(0));
        QCOMPARE(f.resultAt(0), 0);

        result = 3;
        Interface.reportResult(&result, 3);
        QVERIFY(f.isResultReadyAt(3));
        QCOMPARE(f.resultAt(3), 3);

        result = 2;
        Interface.reportResult(&result, 2);
        QVERIFY(f.isResultReadyAt(2));
        QCOMPARE(f.resultAt(2), 2);

        result = 4;
        Interface.reportResult(&result); // no index
        QVERIFY(f.isResultReadyAt(4));
        QCOMPARE(f.resultAt(4), 4);

        Interface.reportFinished();

        QCOMPARE(f.results(), QList<int>() << 0 << 2 << 3 << 4);
    }
}

void tst_QFuture::progress()
{
    QFutureInterface<QChar> result;
    QFuture<QChar> f;

    QCOMPARE (f.progressValue(), 0);

    result.reportStarted();
    f = result.future();

    QCOMPARE (f.progressValue(), 0);

    result.setProgressValue(50);

    QCOMPARE (f.progressValue(), 50);

    result.reportFinished();

    QCOMPARE (f.progressValue(), 50);
}

void tst_QFuture::progressText()
{
    QFutureInterface<void> i;
    i.reportStarted();
    QFuture<void> f = i.future();

    QCOMPARE(f.progressText(), QLatin1String(""));
    i.setProgressValueAndText(1, QLatin1String("foo"));
    QCOMPARE(f.progressText(), QLatin1String("foo"));
    i.reportFinished();
}

/*
    Test that results reported after finished are ignored.
*/
void tst_QFuture::resultsAfterFinished()
{
    {
        IntResult a;
        a.reportStarted();
        QFuture<int> f =  a.future();
        int result;

        QCOMPARE(f.resultCount(), 0);

        result = 1;
        a.reportResult(&result);
        QCOMPARE(f.resultAt(0), 1);

        a.reportFinished();

        QCOMPARE(f.resultAt(0), 1);
        QCOMPARE(f.resultCount(), 1);
        result = 2;
        a.reportResult(&result);
        QCOMPARE(f.resultCount(), 1);
    }
    // cancel it
    {
        IntResult a;
        a.reportStarted();
        QFuture<int> f =  a.future();
        int result;

        QCOMPARE(f.resultCount(), 0);

        result = 1;
        a.reportResult(&result);
        QCOMPARE(f.resultAt(0), 1);
        QCOMPARE(f.resultCount(), 1);

        a.reportCanceled();

        QCOMPARE(f.resultAt(0), 1);
        QCOMPARE(f.resultCount(), 1);

        result = 2;
        a.reportResult(&result);
        a.reportFinished();
    }
}

void tst_QFuture::resultsAsList()
{
    IntResult a;
    a.reportStarted();
    QFuture<int> f = a.future();

    int result;
    result = 1;
    a.reportResult(&result);
    result = 2;
    a.reportResult(&result);

    a.reportFinished();

    QList<int> results = f.results();
    QCOMPARE(results, QList<int>() << 1 << 2);
}

/*
    Test that QFuture<T> can be implicitly converted to T
*/
void tst_QFuture::implicitConversions()
{
    QFutureInterface<QString> iface;
    iface.reportStarted();

    QFuture<QString> f(&iface);

    const QString input("FooBar 2000");
    iface.reportFinished(&input);

    const QString result = f;
    QCOMPARE(result, input);
    QCOMPARE(QString(f), input);
    QCOMPARE(static_cast<QString>(f), input);
}

void tst_QFuture::iterators()
{
    {
        QFutureInterface<int> e;
        e.reportStarted();
        QFuture<int> f = e.future();

        int result;
        result = 1;
        e.reportResult(&result);
        result = 2;
        e.reportResult(&result);
        result = 3;
        e.reportResult(&result);
        e.reportFinished();

        QList<int> results;
        QFutureIterator<int> i(f);
        while (i.hasNext()) {
            results.append(i.next());
        }

        QCOMPARE(results, f.results());

        QFuture<int>::const_iterator i1 = f.begin(), i2 = i1 + 1;
        QFuture<int>::const_iterator c1 = i1, c2 = c1 + 1;

        QCOMPARE(i1, i1);
        QCOMPARE(i1, c1);
        QCOMPARE(c1, i1);
        QCOMPARE(c1, c1);
        QCOMPARE(i2, i2);
        QCOMPARE(i2, c2);
        QCOMPARE(c2, i2);
        QCOMPARE(c2, c2);

        QVERIFY(i1 != i2);
        QVERIFY(i1 != c2);
        QVERIFY(c1 != i2);
        QVERIFY(c1 != c2);
        QVERIFY(i2 != i1);
        QVERIFY(i2 != c1);
        QVERIFY(c2 != i1);
        QVERIFY(c2 != c1);

        int x1 = *i1;
        Q_UNUSED(x1);
        int x2 = *i2;
        Q_UNUSED(x2);
        int y1 = *c1;
        Q_UNUSED(y1);
        int y2 = *c2;
        Q_UNUSED(y2);
    }

    {
        QFutureInterface<QString> e;
        e.reportStarted();
        QFuture<QString> f =  e.future();

        e.reportResult(QString("one"));
        e.reportResult(QString("two"));
        e.reportResult(QString("three"));
        e.reportFinished();

        QList<QString> results;
        QFutureIterator<QString> i(f);
        while (i.hasNext()) {
            results.append(i.next());
        }

        QCOMPARE(results, f.results());

        QFuture<QString>::const_iterator i1 = f.begin(), i2 = i1 + 1;
        QFuture<QString>::const_iterator c1 = i1, c2 = c1 + 1;

        QCOMPARE(i1, i1);
        QCOMPARE(i1, c1);
        QCOMPARE(c1, i1);
        QCOMPARE(c1, c1);
        QCOMPARE(i2, i2);
        QCOMPARE(i2, c2);
        QCOMPARE(c2, i2);
        QCOMPARE(c2, c2);

        QVERIFY(i1 != i2);
        QVERIFY(i1 != c2);
        QVERIFY(c1 != i2);
        QVERIFY(c1 != c2);
        QVERIFY(i2 != i1);
        QVERIFY(i2 != c1);
        QVERIFY(c2 != i1);
        QVERIFY(c2 != c1);

        QString x1 = *i1;
        QString x2 = *i2;
        QString y1 = *c1;
        QString y2 = *c2;

        QCOMPARE(x1, y1);
        QCOMPARE(x2, y2);

        int i1Size = i1->size();
        int i2Size = i2->size();
        int c1Size = c1->size();
        int c2Size = c2->size();

        QCOMPARE(i1Size, c1Size);
        QCOMPARE(i2Size, c2Size);
    }

    {
        const int resultCount = 20;

        QFutureInterface<int> e;
        e.reportStarted();
        QFuture<int> f =  e.future();

        for (int i = 0; i < resultCount; ++i) {
            e.reportResult(i);
        }

        e.reportFinished();

        {
            QFutureIterator<int> it(f);
            QFutureIterator<int> it2(it);
        }

        {
            QFutureIterator<int> it(f);

            for (int i = 0; i < resultCount - 1; ++i) {
                QVERIFY(it.hasNext());
                QCOMPARE(it.peekNext(), i);
                QCOMPARE(it.next(), i);
            }

            QVERIFY(it.hasNext());
            QCOMPARE(it.peekNext(), resultCount - 1);
            QCOMPARE(it.next(), resultCount - 1);
            QVERIFY(!it.hasNext());
        }

        {
            QFutureIterator<int> it(f);
            QVERIFY(it.hasNext());
            it.toBack();
            QVERIFY(!it.hasNext());
            it.toFront();
            QVERIFY(it.hasNext());
        }
    }
}

class SignalSlotObject : public QObject
{
Q_OBJECT
public:
    SignalSlotObject()
    : finishedCalled(false),
      canceledCalled(false),
      rangeBegin(0),
      rangeEnd(0) { }

public slots:
    void finished()
    {
        finishedCalled = true;
    }

    void canceled()
    {
        canceledCalled = true;
    }

    void resultReady(int index)
    {
        results.insert(index);
    }

    void progressRange(int begin, int end)
    {
        rangeBegin = begin;
        rangeEnd = end;
    }

    void progress(int progress)
    {
        reportedProgress.insert(progress);
    }
public:
    bool finishedCalled;
    bool canceledCalled;
    QSet<int> results;
    int rangeBegin;
    int rangeEnd;
    QSet<int> reportedProgress;
};

void tst_QFuture::pause()
{
    QFutureInterface<void> Interface;

    Interface.reportStarted();
    QFuture<void> f = Interface.future();

    QVERIFY(!Interface.isPaused());
    f.pause();
    QVERIFY(Interface.isPaused());
    f.resume();
    QVERIFY(!Interface.isPaused());
    f.togglePaused();
    QVERIFY(Interface.isPaused());
    f.togglePaused();
    QVERIFY(!Interface.isPaused());

    Interface.reportFinished();
}

const int resultCount = 1000;

class ResultObject : public QObject
{
Q_OBJECT
public slots:
    void resultReady(int)
    {

    }
public:
};

// Test that that the isPaused() on future result interface returns true
// if we report a lot of results that are not handled.
void tst_QFuture::throttling()
{
    {
        QFutureInterface<void> i;

        i.reportStarted();
        QFuture<void> f = i.future();

        QVERIFY(!i.isThrottled());

        i.setThrottled(true);
        QVERIFY(i.isThrottled());

        i.setThrottled(false);
        QVERIFY(!i.isThrottled());

        i.setThrottled(true);
        QVERIFY(i.isThrottled());

        i.reportFinished();
    }
}

void tst_QFuture::voidConversions()
{
    {
        QFutureInterface<int> iface;
        iface.reportStarted();

        QFuture<int> intFuture(&iface);
        int value = 10;
        iface.reportFinished(&value);

        QFuture<void> voidFuture(intFuture);
        voidFuture = intFuture;

        QVERIFY(voidFuture == intFuture);
    }

    {
        QFuture<void> voidFuture;
        {
            QFutureInterface<QList<int> > iface;
            iface.reportStarted();

            QFuture<QList<int> > listFuture(&iface);
            iface.reportResult(QList<int>() << 1 << 2 << 3);
            voidFuture = listFuture;
        }
        QCOMPARE(voidFuture.resultCount(), 0);
    }
}


#ifndef QT_NO_EXCEPTIONS

QFuture<void> createExceptionFuture()
{
    QFutureInterface<void> i;
    i.reportStarted();
    QFuture<void> f = i.future();

    QException e;
    i.reportException(e);
    i.reportFinished();
    return f;
}

QFuture<int> createExceptionResultFuture()
{
    QFutureInterface<int> i;
    i.reportStarted();
    QFuture<int> f = i.future();
    int r = 0;
    i.reportResult(r);

    QException e;
    i.reportException(e);
    i.reportFinished();
    return f;
}

class DerivedException : public QException
{
public:
    void raise() const Q_DECL_OVERRIDE { throw *this; }
    DerivedException *clone() const Q_DECL_OVERRIDE { return new DerivedException(*this); }
};

QFuture<void> createDerivedExceptionFuture()
{
    QFutureInterface<void> i;
    i.reportStarted();
    QFuture<void> f = i.future();

    DerivedException e;
    i.reportException(e);
    i.reportFinished();
    return f;
}

void tst_QFuture::exceptions()
{
    // test throwing from waitForFinished
    {
        QFuture<void> f = createExceptionFuture();
        bool caught = false;
        try {
            f.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test result()
    {
        QFuture<int> f = createExceptionResultFuture();
        bool caught = false;
        try {
            f.result();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test result() and destroy
    {
        bool caught = false;
        try {
            createExceptionResultFuture().result();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test results()
    {
        QFuture<int> f = createExceptionResultFuture();
        bool caught = false;
        try {
            f.results();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test foreach
    {
        QFuture<int> f = createExceptionResultFuture();
        bool caught = false;
        try {
            foreach (int e, f.results()) {
                Q_UNUSED(e);
                QFAIL("did not get exception");
            }
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // catch derived exceptions
    {
        bool caught = false;
        try {
            createDerivedExceptionFuture().waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    {
        bool caught = false;
        try {
            createDerivedExceptionFuture().waitForFinished();
        } catch (DerivedException &) {
            caught = true;
        }
        QVERIFY(caught);
    }
}

class MyClass
{
public:
    ~MyClass()
    {
        QFuture<void> f = createExceptionFuture();
        try {
            f.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
    }
    static bool caught;
};

bool MyClass::caught = false;

// This is a regression test for QTBUG-18149. where QFuture did not throw
// exceptions if called from destructors when the stack was already unwinding
// due to an exception having been thrown.
void tst_QFuture::nestedExceptions()
{
    try {
        MyClass m;
        Q_UNUSED(m);
        throw 0;
    } catch (int) {}

    QVERIFY(MyClass::caught);
}

#endif // QT_NO_EXCEPTIONS

void tst_QFuture::nonGlobalThreadPool()
{
    static Q_CONSTEXPR int Answer = 42;

    struct UselessTask : QRunnable, QFutureInterface<int>
    {
        QFuture<int> start(QThreadPool *pool)
        {
            setRunnable(this);
            setThreadPool(pool);
            reportStarted();
            QFuture<int> f = future();
            pool->start(this);
            return f;
        }

        void run() Q_DECL_OVERRIDE
        {
            const int ms = 100 + (qrand() % 100 - 100/2);
            QThread::msleep(ms);
            reportResult(Answer);
            reportFinished();
        }
    };

    QThreadPool pool;

    const int numTasks = QThread::idealThreadCount();

    QVector<QFuture<int> > futures;
    futures.reserve(numTasks);

    for (int i = 0; i < numTasks; ++i)
        futures.push_back((new UselessTask)->start(&pool));

    QVERIFY(!pool.waitForDone(0)); // pool is busy (meaning our tasks did end up executing there)

    QVERIFY(pool.waitForDone(10000)); // max sleep time in UselessTask::run is 150ms, so 10s should be enough
                                      // (and the call returns as soon as all tasks finished anyway, so the
                                      // maximum wait time only matters when the test fails)

    Q_FOREACH (const QFuture<int> &future, futures) {
        QVERIFY(future.isFinished());
        QCOMPARE(future.result(), Answer);
    }
}

QTEST_MAIN(tst_QFuture)
#include "tst_qfuture.moc"
