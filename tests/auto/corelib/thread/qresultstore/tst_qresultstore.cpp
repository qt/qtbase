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

#include <QtTest/QtTest>

#include <qresultstore.h>

using namespace QtPrivate;

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
private:
    int int0;
    int int1;
    int int2;
    QVector<int> vec0;
    QVector<int> vec1;
};

void tst_QtConcurrentResultStore::init()
{
    int0 = 0;
    int1 = 1;
    int2 = 2;
    vec0 = QVector<int>() << 2 << 3;
    vec1 = QVector<int>() << 4 << 5;
}

void tst_QtConcurrentResultStore::construction()
{
    ResultStore<int> store;
    QCOMPARE(store.count(), 0);
}

void tst_QtConcurrentResultStore::iterators()
{
    {
        ResultStore<int> store;
        QVERIFY(store.begin() == store.end());
        QVERIFY(store.resultAt(0) == store.end());
        QVERIFY(store.resultAt(1) == store.end());
    }
    {
        ResultStoreBase storebase;
        storebase.addResult(-1, &int0); // note to self: adding a pointer to the stack here is ok since
        storebase.addResult(1, &int1);  // ResultStoreBase does not take ownership, only ResultStore<> does.
        ResultIteratorBase it = storebase.begin();
        QCOMPARE(it.resultIndex(), 0);
        QVERIFY(it == storebase.begin());
        QVERIFY(it != storebase.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != storebase.begin());
        QVERIFY(it != storebase.end());

        ++it;
        QVERIFY(it != storebase.begin());
        QVERIFY(it == storebase.end());
    }
}

void tst_QtConcurrentResultStore::addResult()
{
    {
        // test addResult return value
        ResultStore<int> store;
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

    ResultStoreBase store;
    store.addResults(-1, &vec0, 2, 2);
    store.addResults(-1, &vec1, 2, 2);
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

    ++it;
    QCOMPARE(it.resultIndex(), 3);

    ++it;
    QVERIFY(it == store.end());
}

void tst_QtConcurrentResultStore::resultIndex()
{
    ResultStore<int> store;
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

    QCOMPARE(store.resultAt(0).value(), int0);
    QCOMPARE(store.resultAt(1).value(), vec0[0]);
    QCOMPARE(store.resultAt(2).value(), vec0[1]);
    QCOMPARE(store.resultAt(3).value(), int1);
}

void tst_QtConcurrentResultStore::resultAt()
{
    {
        ResultStore<int> store;
        store.addResult(-1, &int0);
        store.addResults(-1, &vec0);
        store.addResult(200, &int1);

        QCOMPARE(store.resultAt(0).value(), int0);
        QCOMPARE(store.resultAt(1).value(), vec0[0]);
        QCOMPARE(store.resultAt(2).value(), vec0[1]);
        QCOMPARE(store.resultAt(200).value(), int1);
    }
    {
        ResultStore<int> store;
        store.addResult(1, &int1);
        store.addResult(0, &int0);
        store.addResult(-1, &int2);

        QCOMPARE(store.resultAt(0).value(), int0);
        QCOMPARE(store.resultAt(1).value(), int1);
        QCOMPARE(store.resultAt(2).value(), int2);
    }
}

void tst_QtConcurrentResultStore::contains()
{
    {
        ResultStore<int> store;
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
        ResultStore<int> store;
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
        ResultStore<int> store;
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
        ResultStore<int> store;
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
    ResultStore<int> store;
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
}

void tst_QtConcurrentResultStore::addCanceledResult()
{
    // test canceled results
    ResultStore<int> store;
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
        ResultStore<int> store;
        store.addResult(0, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(2, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        ResultStore<int> store;
        store.addResult(2, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        ResultStore<int> store;
        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 4);
    }

    {
        ResultStore<int> store;
        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 4);
    }
    {
        ResultStore<int> store;
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 5);
    }

    {
        ResultStore<int> store;
        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addCanceledResult(2);
        QCOMPARE(store.count(), 4);
    }

    {
        ResultStore<int> store;
        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults(0, 3);
        QCOMPARE(store.count(), 2);
    }

    {
        ResultStore<int> store;
        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults(0, 3);
        QCOMPARE(store.count(), 2);  // results at 3 and 4 become available at index 0, 1

        store.addResult(5, &int0);
        QCOMPARE(store.count(), 3);// result 5 becomes available at index 2
    }
}

QTEST_MAIN(tst_QtConcurrentResultStore)
#include "tst_qresultstore.moc"
