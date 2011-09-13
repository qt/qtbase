/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QTest>
#include <QCache>
#include <QContiguousCache>

#include <QDebug>
#include <stdio.h>

class tst_QContiguousCache : public QObject
{
    Q_OBJECT
public:
    tst_QContiguousCache() {}
    virtual ~tst_QContiguousCache() {}
private slots:
    void empty();
    void swap();

    void append_data();
    void append();

    void prepend_data();
    void prepend();

    void asScrollingList();

    void complexType();

    void operatorAt();

    void cacheBenchmark();
    void contiguousCacheBenchmark();

    void setCapacity();

    void zeroCapacity();
};

QTEST_MAIN(tst_QContiguousCache)

void tst_QContiguousCache::empty()
{
    QContiguousCache<int> c(10);
    QCOMPARE(c.capacity(), 10);
    QCOMPARE(c.count(), 0);
    QVERIFY(c.isEmpty());
    c.append(1);
    QCOMPARE(c.count(), 1);
    QVERIFY(!c.isEmpty());
    c.clear();
    QCOMPARE(c.capacity(), 10);
    QCOMPARE(c.count(), 0);
    QVERIFY(c.isEmpty());
    c.prepend(1);
    QCOMPARE(c.count(), 1);
    QVERIFY(!c.isEmpty());
    c.clear();
    QCOMPARE(c.count(), 0);
    QVERIFY(c.isEmpty());
    QCOMPARE(c.capacity(), 10);
}

void tst_QContiguousCache::swap()
{
    QContiguousCache<int> c1(10), c2(100);
    c1.append(1);
    c1.swap(c2);
    QCOMPARE(c1.capacity(), 100);
    QCOMPARE(c1.count(),    0  );
    QCOMPARE(c2.capacity(), 10 );
    QCOMPARE(c2.count(),    1  );
}

void tst_QContiguousCache::append_data()
{
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("cacheSize");
    QTest::addColumn<bool>("invalidIndexes");

    QTest::newRow("0+30[10]") << 0 << 30 << 10 << false;
    QTest::newRow("300+30[10]") << 300 << 30 << 10 << false;
    QTest::newRow("MAX-10+30[10]") << INT_MAX-10 << 30 << 10 << true;
}

void tst_QContiguousCache::append()
{
    QFETCH(int, start);
    QFETCH(int, count);
    QFETCH(int, cacheSize);
    QFETCH(bool, invalidIndexes);

    int i, j;
    QContiguousCache<int> c(cacheSize);

    i = 1;
    QCOMPARE(c.available(), cacheSize);
    if (start == 0)
        c.append(i++);
    else
        c.insert(start, i++);
    while (i < count) {
        c.append(i);
        QCOMPARE(c.available(), qMax(0, cacheSize - i));
        QCOMPARE(c.first(), qMax(1, i-cacheSize+1));
        QCOMPARE(c.last(), i);
        QCOMPARE(c.count(), qMin(i, cacheSize));
        QCOMPARE(c.isFull(), i >= cacheSize);
        i++;
    }

    QCOMPARE(c.areIndexesValid(), !invalidIndexes);
    if (invalidIndexes)
        c.normalizeIndexes();
    QVERIFY(c.areIndexesValid());

    // test taking from end until empty.
    for (j = 0; j < cacheSize; j++, i--) {
        QCOMPARE(c.takeLast(), i-1);
        QCOMPARE(c.count(), cacheSize-j-1);
        QCOMPARE(c.available(), j+1);
        QVERIFY(!c.isFull());
        QCOMPARE(c.isEmpty(), j==cacheSize-1);
    }

}

void tst_QContiguousCache::prepend_data()
{
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("cacheSize");
    QTest::addColumn<bool>("invalidIndexes");

    QTest::newRow("30-30[10]") << 30 << 30 << 10 << false;
    QTest::newRow("300-30[10]") << 300 << 30 << 10 << false;
    QTest::newRow("10-30[10]") << 10 << 30 << 10 << true;
}

void tst_QContiguousCache::prepend()
{
    QFETCH(int, start);
    QFETCH(int, count);
    QFETCH(int, cacheSize);
    QFETCH(bool, invalidIndexes);

    int i, j;
    QContiguousCache<int> c(cacheSize);

    i = 1;
    QCOMPARE(c.available(), cacheSize);
    c.insert(start, i++);
    while(i < count) {
        c.prepend(i);
        QCOMPARE(c.available(), qMax(0, cacheSize - i));
        QCOMPARE(c.last(), qMax(1, i-cacheSize+1));
        QCOMPARE(c.first(), i);
        QCOMPARE(c.count(), qMin(i, cacheSize));
        QCOMPARE(c.isFull(), i >= cacheSize);
        i++;
    }

    QCOMPARE(c.areIndexesValid(), !invalidIndexes);
    if (invalidIndexes)
        c.normalizeIndexes();
    QVERIFY(c.areIndexesValid());

    // test taking from start until empty.
    for (j = 0; j < cacheSize; j++, i--) {
        QCOMPARE(c.takeFirst(), i-1);
        QCOMPARE(c.count(), cacheSize-j-1);
        QCOMPARE(c.available(), j+1);
        QVERIFY(!c.isFull());
        QCOMPARE(c.isEmpty(), j==cacheSize-1);
    }
}

void tst_QContiguousCache::asScrollingList()
{
    int i;
    QContiguousCache<int> c(10);

    // Once allocated QContiguousCache should not
    // allocate any additional memory for non
    // complex data types.
    QBENCHMARK {
        // simulate scrolling in a list of items;
        for(i = 0; i < 10; ++i) {
            QCOMPARE(c.available(), 10-i);
            c.append(i);
        }

        QCOMPARE(c.firstIndex(), 0);
        QCOMPARE(c.lastIndex(), 9);
        QCOMPARE(c.first(), 0);
        QCOMPARE(c.last(), 9);
        QVERIFY(!c.containsIndex(-1));
        QVERIFY(!c.containsIndex(10));
        QCOMPARE(c.available(), 0);

        for (i = 0; i < 10; ++i) {
            QVERIFY(c.containsIndex(i));
            QCOMPARE(c.at(i), i);
            QCOMPARE(c[i], i);
            QCOMPARE(((const QContiguousCache<int>)c)[i], i);
        }

        for (i = 10; i < 30; ++i)
            c.append(i);

        QCOMPARE(c.firstIndex(), 20);
        QCOMPARE(c.lastIndex(), 29);
        QCOMPARE(c.first(), 20);
        QCOMPARE(c.last(), 29);
        QVERIFY(!c.containsIndex(19));
        QVERIFY(!c.containsIndex(30));
        QCOMPARE(c.available(), 0);

        for (i = 20; i < 30; ++i) {
            QVERIFY(c.containsIndex(i));
            QCOMPARE(c.at(i), i);
            QCOMPARE(c[i], i);
            QCOMPARE(((const QContiguousCache<int> )c)[i], i);
        }

        for (i = 19; i >= 10; --i)
            c.prepend(i);

        QCOMPARE(c.firstIndex(), 10);
        QCOMPARE(c.lastIndex(), 19);
        QCOMPARE(c.first(), 10);
        QCOMPARE(c.last(), 19);
        QVERIFY(!c.containsIndex(9));
        QVERIFY(!c.containsIndex(20));
        QCOMPARE(c.available(), 0);

        for (i = 10; i < 20; ++i) {
            QVERIFY(c.containsIndex(i));
            QCOMPARE(c.at(i), i);
            QCOMPARE(c[i], i);
            QCOMPARE(((const QContiguousCache<int> )c)[i], i);
        }

        for (i = 200; i < 220; ++i)
            c.insert(i, i);

        QCOMPARE(c.firstIndex(), 210);
        QCOMPARE(c.lastIndex(), 219);
        QCOMPARE(c.first(), 210);
        QCOMPARE(c.last(), 219);
        QVERIFY(!c.containsIndex(209));
        QVERIFY(!c.containsIndex(300));
        QCOMPARE(c.available(), 0);

        for (i = 210; i < 220; ++i) {
            QVERIFY(c.containsIndex(i));
            QCOMPARE(c.at(i), i);
            QCOMPARE(c[i], i);
            QCOMPARE(((const QContiguousCache<int> )c)[i], i);
        }
        c.clear(); // needed to reset benchmark
    }

    // from a specific bug that was encountered.  100 to 299 cached, attempted to cache 250 - 205 via insert, failed.
    // bug was that item at 150 would instead be item that should have been inserted at 250
    c.setCapacity(200);
    for(i = 100; i < 300; ++i)
        c.insert(i, i);
    for (i = 250; i <= 306; ++i)
        c.insert(i, 1000+i);
    for (i = 107; i <= 306; ++i) {
        QVERIFY(c.containsIndex(i));
        QCOMPARE(c.at(i), i < 250 ? i : 1000+i);
    }
}

struct RefCountingClassData
{
    QBasicAtomicInt ref;
    static RefCountingClassData shared_null;
};

RefCountingClassData RefCountingClassData::shared_null = {
    Q_BASIC_ATOMIC_INITIALIZER(1)
};

class RefCountingClass
{
public:
    RefCountingClass() : d(&RefCountingClassData::shared_null) { d->ref.ref(); }

    RefCountingClass(const RefCountingClass &other)
    {
        d = other.d;
        d->ref.ref();
    }

    ~RefCountingClass()
    {
        if (!d->ref.deref())
            delete d;
    }

    RefCountingClass &operator=(const RefCountingClass &other)
    {
        if (!d->ref.deref())
            delete d;
        d = other.d;
        d->ref.ref();
        return *this;
    }

    int refCount() const { return d->ref; }
private:
    RefCountingClassData *d;
};

void tst_QContiguousCache::complexType()
{
    RefCountingClass original;

    QContiguousCache<RefCountingClass> contiguousCache(10);
    contiguousCache.append(original);
    QCOMPARE(original.refCount(), 3);
    contiguousCache.removeFirst();
    QCOMPARE(original.refCount(), 2); // shared null, 'original'.
    contiguousCache.append(original);
    QCOMPARE(original.refCount(), 3);
    contiguousCache.clear();
    QCOMPARE(original.refCount(), 2);

    for(int i = 0; i < 100; ++i)
        contiguousCache.insert(i, original);

    QCOMPARE(original.refCount(), 12); // shared null, 'original', + 10 in contiguousCache.

    contiguousCache.clear();
    QCOMPARE(original.refCount(), 2);
    for (int i = 0; i < 100; i++)
        contiguousCache.append(original);

    QCOMPARE(original.refCount(), 12); // shared null, 'original', + 10 in contiguousCache.
    contiguousCache.clear();
    QCOMPARE(original.refCount(), 2);

    for (int i = 0; i < 100; i++)
        contiguousCache.prepend(original);

    QCOMPARE(original.refCount(), 12); // shared null, 'original', + 10 in contiguousCache.
    contiguousCache.clear();
    QCOMPARE(original.refCount(), 2);

    for (int i = 0; i < 100; i++)
        contiguousCache.append(original);

    contiguousCache.takeLast();
    QCOMPARE(original.refCount(), 11);

    contiguousCache.takeFirst();
    QCOMPARE(original.refCount(), 10);
}

void tst_QContiguousCache::operatorAt()
{
    RefCountingClass original;
    QContiguousCache<RefCountingClass> contiguousCache(10);

    for (int i = 25; i < 35; ++i)
        contiguousCache[i] = original;

    QCOMPARE(original.refCount(), 12); // shared null, orig, items in cache

    // verify const access does not copy items.
    const QContiguousCache<RefCountingClass> copy(contiguousCache);
    for (int i = 25; i < 35; ++i)
        QCOMPARE(copy[i].refCount(), 12);

    // verify modifying the original increments ref count (e.g. does a detach)
    contiguousCache[25] = original;
    QCOMPARE(original.refCount(), 22);
}

/*
    Benchmarks must be near identical in tasks to be fair.
    QCache uses pointers to ints as its a requirement of QCache,
    whereas QContiguousCache doesn't support pointers (won't free them).
    Given the ability to use simple data types is a benefit, its
    fair.  Although this obviously must take into account we are
    testing QContiguousCache use cases here, QCache has its own
    areas where it is the more sensible class to use.
*/
void tst_QContiguousCache::cacheBenchmark()
{
    QBENCHMARK {
        QCache<int, int> cache;
        cache.setMaxCost(100);

        for (int i = 0; i < 1000; i++)
            cache.insert(i, new int(i));
    }
}

void tst_QContiguousCache::contiguousCacheBenchmark()
{
    QBENCHMARK {
        QContiguousCache<int> contiguousCache(100);
        for (int i = 0; i < 1000; i++)
            contiguousCache.insert(i, i);
    }
}

void tst_QContiguousCache::setCapacity()
{
    int i;
    QContiguousCache<int> contiguousCache(100);
    for (i = 280; i < 310; ++i)
        contiguousCache.insert(i, i);
    QCOMPARE(contiguousCache.capacity(), 100);
    QCOMPARE(contiguousCache.count(), 30);
    QCOMPARE(contiguousCache.firstIndex(), 280);
    QCOMPARE(contiguousCache.lastIndex(), 309);

    for (i = contiguousCache.firstIndex(); i <= contiguousCache.lastIndex(); ++i) {
        QVERIFY(contiguousCache.containsIndex(i));
        QCOMPARE(contiguousCache.at(i), i);
    }

    contiguousCache.setCapacity(150);

    QCOMPARE(contiguousCache.capacity(), 150);
    QCOMPARE(contiguousCache.count(), 30);
    QCOMPARE(contiguousCache.firstIndex(), 280);
    QCOMPARE(contiguousCache.lastIndex(), 309);

    for (i = contiguousCache.firstIndex(); i <= contiguousCache.lastIndex(); ++i) {
        QVERIFY(contiguousCache.containsIndex(i));
        QCOMPARE(contiguousCache.at(i), i);
    }

    contiguousCache.setCapacity(20);

    QCOMPARE(contiguousCache.capacity(), 20);
    QCOMPARE(contiguousCache.count(), 20);
    QCOMPARE(contiguousCache.firstIndex(), 290);
    QCOMPARE(contiguousCache.lastIndex(), 309);

    for (i = contiguousCache.firstIndex(); i <= contiguousCache.lastIndex(); ++i) {
        QVERIFY(contiguousCache.containsIndex(i));
        QCOMPARE(contiguousCache.at(i), i);
    }
}

void tst_QContiguousCache::zeroCapacity()
{
    QContiguousCache<int> contiguousCache;
    QCOMPARE(contiguousCache.capacity(),0);
    contiguousCache.setCapacity(10);
    QCOMPARE(contiguousCache.capacity(),10);
    contiguousCache.setCapacity(0);
    QCOMPARE(contiguousCache.capacity(),0);
}

#include "tst_qcontiguouscache.moc"
