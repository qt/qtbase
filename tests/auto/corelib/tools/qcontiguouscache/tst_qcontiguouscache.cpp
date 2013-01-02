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

#include <QObject>
#include <QTest>
#include <QCache>
#include <QContiguousCache>

#include <QDebug>
#include <stdio.h>

class tst_QContiguousCache : public QObject
{
    Q_OBJECT
private slots:
    void empty();
    void swap();

    void append_data();
    void append();

    void prepend_data();
    void prepend();

    void complexType();

    void operatorAt();

    void setCapacity();

    void zeroCapacity();
    void modifyZeroCapacityCache();
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

    int refCount() const { return d->ref.load(); }
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

void tst_QContiguousCache::modifyZeroCapacityCache()
{
    {
        QContiguousCache<int> contiguousCache;
        contiguousCache.insert(0, 42);
    }
    {
        QContiguousCache<int> contiguousCache;
        contiguousCache.insert(1, 42);
    }
    {
        QContiguousCache<int> contiguousCache;
        contiguousCache.append(42);
    }
    {
        QContiguousCache<int> contiguousCache;
        contiguousCache.prepend(42);
    }
}

#include "tst_qcontiguouscache.moc"
