// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <qresultstore.h>

using namespace QtPrivate;

class IntResultsCleaner
{
public:
    IntResultsCleaner(QtPrivate::ResultStoreBase &s) : store(s) { }
    ~IntResultsCleaner() { store.clear<int>(); }

private:
    QtPrivate::ResultStoreBase &store;
};

class tst_QtConcurrentResultStore : public QObject
{
    Q_OBJECT
public slots:
    void init();
private slots:
    void construction();
    void iterators();
    void addResult();
    void addResults();
    void resultIndex();
    void resultAt();
    void contains();
    void filterMode();
    void addCanceledResult();
    void count();
    void pendingResultsDoNotLeak_data();
    void pendingResultsDoNotLeak();
private:
    int int0;
    int int1;
    int int2;
    QList<int> vec0;
    QList<int> vec1;
};

void tst_QtConcurrentResultStore::init()
{
    int0 = 0;
    int1 = 1;
    int2 = 2;
    vec0 = QList<int> { 2, 3 };
    vec1 = QList<int> { 4, 5 };
}

void tst_QtConcurrentResultStore::construction()
{
    ResultStoreBase store;
    QCOMPARE(store.count(), 0);
}

void tst_QtConcurrentResultStore::iterators()
{
    {
        ResultStoreBase store;
        QCOMPARE(store.begin(), store.end());
        QCOMPARE(store.resultAt(0), store.end());
        QCOMPARE(store.resultAt(1), store.end());
    }
    {
        QtPrivate::ResultStoreBase storebase;
        IntResultsCleaner cleanGuard(storebase);

        storebase.addResult(-1, &int0); // note to self: adding a pointer to the stack here is ok since
        storebase.addResult(1, &int1);  // ResultStoreBase does not take ownership, only ResultStore<> does.
        ResultIteratorBase it = storebase.begin();
        QCOMPARE(it.resultIndex(), 0);
        QCOMPARE(it, storebase.begin());
        QVERIFY(it != storebase.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != storebase.begin());
        QVERIFY(it != storebase.end());

        ++it;
        QVERIFY(it != storebase.begin());
        QCOMPARE(it, storebase.end());
    }
}

void tst_QtConcurrentResultStore::addResult()
{
    {
        // test addResult return value
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);

        QCOMPARE(store.addResult(0, &int0), 0);
        QCOMPARE(store.count(), 1); // result 0 becomes available
        QCOMPARE(store.contains(0), true);

        QCOMPARE(store.addResult(2, &int0), 2);
        QCOMPARE(store.count(), 1);
        QCOMPARE(store.contains(2), false);

        QCOMPARE(store.addCanceledResult(1), 1);
        QCOMPARE(store.count(), 2); // result 2 is renamed to 1 and becomes available

        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), false);

        QCOMPARE(store.addResult(3, &int0), 3);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.contains(2), true);

        QCOMPARE(store.addResult(6, &int0), 6);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.addResult(7, &int0), 7);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.contains(3), false);

        QCOMPARE(store.addCanceledResult(4), 4);
        QCOMPARE(store.addCanceledResult(5), 5);
        QCOMPARE(store.count(), 5); // 6 and 7 is renamed to 3 and 4 and becomes available

        QCOMPARE(store.contains(3), true);
        QCOMPARE(store.contains(4), true);

        QCOMPARE(store.addResult(8, &int0), 8);
        QCOMPARE(store.contains(5), true);
        QCOMPARE(store.count(), 6);

        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);
    }
}

void tst_QtConcurrentResultStore::addResults()
{
    QtPrivate::ResultStoreBase store;
    IntResultsCleaner cleanGuard(store);

    store.addResults(-1, &vec0);
    store.addResults(-1, &vec1);
    ResultIteratorBase it = store.begin();
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

    QList<int> empty;
    const auto countBefore = store.count();
    QCOMPARE(store.addResults(countBefore, &empty), -1);
    QCOMPARE(store.count(), countBefore);

    QCOMPARE(store.addResults(countBefore, &vec1), countBefore);
    QCOMPARE(store.count(), countBefore + vec1.size());
}

void tst_QtConcurrentResultStore::resultIndex()
{
    QtPrivate::ResultStoreBase store;
    IntResultsCleaner cleanGuard(store);

    store.addResult(-1, &int0);
    store.addResults(-1, &vec0);
    store.addResult(-1, &int1);

    ResultIteratorBase it = store.begin();
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

    QCOMPARE(store.resultAt(0).value<int>(), int0);
    QCOMPARE(store.resultAt(1).value<int>(), vec0[0]);
    QCOMPARE(store.resultAt(2).value<int>(), vec0[1]);
    QCOMPARE(store.resultAt(3).value<int>(), int1);
}

void tst_QtConcurrentResultStore::resultAt()
{
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(-1, &int0);
        store.addResults(-1, &vec0);
        store.addResult(200, &int1);

        QCOMPARE(store.resultAt(0).value<int>(), int0);
        QCOMPARE(store.resultAt(1).value<int>(), vec0[0]);
        QCOMPARE(store.resultAt(2).value<int>(), vec0[1]);
        QCOMPARE(store.resultAt(200).value<int>(), int1);
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(1, &int1);
        store.addResult(0, &int0);
        store.addResult(-1, &int2);

        QCOMPARE(store.resultAt(0).value<int>(), int0);
        QCOMPARE(store.resultAt(1).value<int>(), int1);
        QCOMPARE(store.resultAt(2).value<int>(), int2);
    }
}

void tst_QtConcurrentResultStore::contains()
{
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        QCOMPARE(store.contains(0), false);
        QCOMPARE(store.contains(1), false);
        QCOMPARE(store.contains(INT_MAX), false);
        store.addResult(1, &int1);
        QVERIFY(store.contains(int1));
        store.addResult(0, &int0);
        QVERIFY(store.contains(int0));
        store.addResult(-1, &int2);
        QVERIFY(store.contains(int2));
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

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
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

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
        store.addCanceledResults<int>(4, 2);

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
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addCanceledResult(0);
        QCOMPARE(store.contains(0), false);

        store.addResult(1, &int0);
        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), false);
    }
}

void tst_QtConcurrentResultStore::filterMode()
{
    // Test filter mode, where "gaps" in the result array aren't allowed.
    QtPrivate::ResultStoreBase store;
    IntResultsCleaner cleanGuard(store);

    QCOMPARE(store.filterMode(), false);
    store.setFilterMode(true);
    QVERIFY(store.filterMode());

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

    QList<int> empty;
    const auto countBefore = store.count();
    QCOMPARE(store.addResults(countBefore, &empty), -1);
    QCOMPARE(store.count(), countBefore);

    QCOMPARE(store.addResult(countBefore, &int2), countBefore);
    QCOMPARE(store.count(), countBefore + 1);
}

void tst_QtConcurrentResultStore::addCanceledResult()
{
    // test canceled results
    QtPrivate::ResultStoreBase store;
    IntResultsCleaner cleanGuard(store);

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

void tst_QtConcurrentResultStore::count()
{
    {
        // test resultCount in non-filtered mode. It should always be possible
        // to iterate through the results 0 to resultCount.
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(0, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(2, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 4);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 4);
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 5);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addCanceledResult(2);
        QCOMPARE(store.count(), 4);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults<int>(0, 3);
        QCOMPARE(store.count(), 2);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults<int>(0, 3);
        QCOMPARE(store.count(), 2);  // results at 3 and 4 become available at index 0, 1

        store.addResult(5, &int0);
        QCOMPARE(store.count(), 3);// result 5 becomes available at index 2
    }
}

// simplified version of CountedObject from tst_qarraydata.cpp
struct CountedObject
{
    CountedObject() : id(liveCount++)
    { }

    CountedObject(const CountedObject &other) : id(other.id)
    {
        ++liveCount;
    }

    ~CountedObject()
    {
        --liveCount;
    }

    CountedObject &operator=(const CountedObject &) = default;

    struct LeakChecker
    {
        LeakChecker()
            : previousLiveCount(liveCount)
        {
        }

        ~LeakChecker()
        {
            QCOMPARE(liveCount, previousLiveCount);
        }

    private:
        const size_t previousLiveCount;
    };

    int id = 0;
    static size_t liveCount;
};

size_t CountedObject::liveCount = 0;

void tst_QtConcurrentResultStore::pendingResultsDoNotLeak_data()
{
    QTest::addColumn<bool>("filterMode");

    QTest::addRow("filter-mode-off") << false;
    QTest::addRow("filter-mode-on") << true;
}

void tst_QtConcurrentResultStore::pendingResultsDoNotLeak()
{
    QFETCH(bool, filterMode);
    CountedObject::LeakChecker leakChecker; Q_UNUSED(leakChecker)

    QtPrivate::ResultStoreBase store;
    auto cleanGaurd = qScopeGuard([&] { store.clear<CountedObject>(); });

    store.setFilterMode(filterMode);

    // lvalue
    auto lvalueObj = CountedObject();
    store.addResult(42, &lvalueObj);

    // rvalue
    store.moveResult(43, CountedObject());

    // array
    auto lvalueListOfObj = QList<CountedObject>({CountedObject(), CountedObject()});
    store.addResults(44, &lvalueListOfObj);
}

QTEST_MAIN(tst_QtConcurrentResultStore)
#include "tst_qresultstore.moc"
