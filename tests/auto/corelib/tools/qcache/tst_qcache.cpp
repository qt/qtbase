/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <qcache.h>

class tst_QCache : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    void cleanupTestCase();
private slots:
    void maxCost();
    void setMaxCost();
    void totalCost();
    void clear();
    void insert();
    void contains();
    void operator_bracket_bracket();
    void remove();
    void take();
    void axioms_on_key_type();
};


struct Foo {
    static int count;
    Foo():c(count) { ++count; }
    Foo(const Foo& o):c(o.c) { ++count; }
    ~Foo() { --count; }
    int c;
    int data[8];
};

int Foo::count = 0;

void tst_QCache::initTestCase()
{
    Foo::count = 0;
}

void tst_QCache::cleanupTestCase()
{
    // always check for memory leaks
    QCOMPARE(Foo::count, 0);
}

void tst_QCache::maxCost()
{
    QCache<QString, int> cache1, cache2(100), cache3(200), cache4(-50);
    QCOMPARE(cache1.maxCost(), 100);
    QCOMPARE(cache2.maxCost(), 100);
    QCOMPARE(cache3.maxCost(), 200);
    QCOMPARE(cache4.maxCost(), -50); // 0 would also make sense

    cache1.setMaxCost(101);
    QCOMPARE(cache1.maxCost(), 101);

    cache1.insert("one", new int(1), 1);
    cache1.insert("two", new int(2), 1);
    QCOMPARE(cache1.totalCost(), 2);
    QCOMPARE(cache1.size(), 2);
    QCOMPARE(cache1.maxCost(), 101);

    cache1.insert("three", new int(3), 98);
    QCOMPARE(cache1.totalCost(), 100);
    QCOMPARE(cache1.size(), 3);
    QCOMPARE(cache1.maxCost(), 101);

    cache1.insert("four", new int(4), 1);
    QCOMPARE(cache1.totalCost(), 101);
    QCOMPARE(cache1.size(), 4);
    QCOMPARE(cache1.maxCost(), 101);

    cache1.insert("five", new int(4), 1);
    QVERIFY(cache1.totalCost() <= 101);
    QVERIFY(cache1.size() == 4);
    QCOMPARE(cache1.maxCost(), 101);

    cache1.setMaxCost(-1);
    QCOMPARE(cache1.totalCost(), 0);
    QCOMPARE(cache1.maxCost(), -1);

    cache2.setMaxCost(202);
    QCOMPARE(cache2.maxCost(), 202);

    cache3.setMaxCost(-50);
    QCOMPARE(cache3.maxCost(), -50);
}

void tst_QCache::setMaxCost()
{
    QCache<int, Foo> cache;
    cache.setMaxCost(2);
    cache.insert(1, new Foo);
    cache.insert(2, new Foo);
    QCOMPARE(cache.totalCost(), 2);
    QCOMPARE(Foo::count, 2);

    cache.insert(3, new Foo);
    QCOMPARE(cache.totalCost(), 2);
    QCOMPARE(Foo::count, 2);

    cache.setMaxCost(3);
    QCOMPARE(cache.totalCost(), 2);
    QCOMPARE(Foo::count, 2);

    cache.setMaxCost(2);
    QCOMPARE(cache.totalCost(), 2);
    QCOMPARE(Foo::count, 2);

    cache.setMaxCost(1);
    QCOMPARE(cache.totalCost(), 1);
    QCOMPARE(Foo::count, 1);

    cache.setMaxCost(0);
    QCOMPARE(cache.totalCost(), 0);
    QCOMPARE(Foo::count, 0);

    cache.setMaxCost(-1);
    QCOMPARE(cache.totalCost(), 0);
    QCOMPARE(Foo::count, 0);
}

void tst_QCache::totalCost()
{
    QCache<QString, int> cache;
    QCOMPARE(cache.totalCost(), 0);

    cache.insert("one", new int(1), 0);
    QCOMPARE(cache.totalCost(), 0);

    cache.insert("two", new int(2), 1);
    QCOMPARE(cache.totalCost(), 1);

    cache.insert("three", new int(3), 2);
    QCOMPARE(cache.totalCost(), 3);

    cache.insert("four", new int(4), 10000);
    QCOMPARE(cache.totalCost(), 3);
    QVERIFY(!cache.contains("four"));

    cache.insert("five", new int(5), -5);
    QCOMPARE(cache.totalCost(), -2);

    cache.insert("six", new int(6), 101);
    QCOMPARE(cache.totalCost(), -2);

    cache.insert("seven", new int(7), 100);
    QCOMPARE(cache.totalCost(), 98);
    QCOMPARE(cache.size(), 5);

    cache.insert("eight", new int(8), 2);
    QCOMPARE(cache.totalCost(), 100);
    QCOMPARE(cache.size(), 6);
}

void tst_QCache::clear()
{
    {
        QCache<QString, Foo> cache(200);
        QCOMPARE(cache.totalCost(), 0);

        for (int i = -3; i < 9; ++i)
            cache.insert(QString::number(i), new Foo, i);
        QCOMPARE(cache.totalCost(), 30);

        QCOMPARE(cache.size(), 12);
        QVERIFY(!cache.isEmpty());
        cache.setMaxCost(300);

        for (int j = 0; j < 3; ++j) {
            cache.clear();
            QCOMPARE(cache.totalCost(), 0);
            QCOMPARE(cache.size(), 0);
            QVERIFY(cache.isEmpty());
            QCOMPARE(Foo::count, 0);
            QCOMPARE(cache.maxCost(), 300);
        }
        cache.insert("10", new Foo, 10);
        QCOMPARE(cache.size(), 1);
        cache.setMaxCost(9);
        QCOMPARE(cache.size(), 0);

        cache.insert("11", new Foo, 9);
        QCOMPARE(cache.size(), 1);
        QCOMPARE(Foo::count, 1);
    }
    QCOMPARE(Foo::count, 0);
}

void tst_QCache::insert()
{
    QCache<QString, Foo> cache;

    Foo *f1 = new Foo;
    cache.insert("one", f1, 1);
    QVERIFY(cache.contains("one"));

    Foo *f2 = new Foo;
    cache.insert("two", f2, 2);
    QVERIFY(cache.contains("two"));
    QCOMPARE(cache.size(), 2);

    Foo *f3 = new Foo;
    cache.insert("two", f3, 2);
    QVERIFY(cache.contains("two"));
    QCOMPARE(cache.size(), 2);

    QVERIFY(cache["two"] == f3);
    QCOMPARE(Foo::count, 2);

    /*
        If the new item is too big, any item with the same name in
        the cache must still be removed, otherwise the application
        might get bad results.
    */
    Foo *f4 = new Foo;
    cache.insert("two", f4, 10000);
    QVERIFY(!cache.contains("two"));
    QCOMPARE(cache.size(), 1);
    QCOMPARE(Foo::count, 1);
}

void tst_QCache::contains()
{
    QCache<int, int> cache;
    QVERIFY(!cache.contains(0));
    QVERIFY(!cache.contains(1));

    cache.insert(1, new int(1), 1);
    QVERIFY(!cache.contains(0));
    QVERIFY(cache.contains(1));

    cache.remove(0);
    cache.remove(1);
    QVERIFY(!cache.contains(0));
    QVERIFY(!cache.contains(1));

    cache.insert(1, new int(1), 1);
    QVERIFY(!cache.contains(0));
    QVERIFY(cache.contains(1));

    cache.clear();
    QVERIFY(!cache.contains(0));
    QVERIFY(!cache.contains(1));
}

void tst_QCache::operator_bracket_bracket()
{
    QCache<int, int> cache;
    cache.insert(1, new int(2));
    QVERIFY(cache[0] == 0);
    QVERIFY(cache[1] != 0);
    QCOMPARE(*cache[1], 2);

    cache.insert(1, new int(4));
    QVERIFY(cache[1] != 0);
    QCOMPARE(*cache[1], 4);

    // check that operator[] doesn't remove the item
    QVERIFY(cache[1] != 0);
    QCOMPARE(*cache[1], 4);

    cache.remove(1);
    QVERIFY(cache[1] == 0);
}

void tst_QCache::remove()
{
    QCache<QString, Foo> cache;
    cache.remove(QString());
    cache.remove("alpha");
    QVERIFY(cache.isEmpty());

    cache.insert("alpha", new Foo, 10);
    QCOMPARE(cache.size(), 1);

    cache.insert("beta", new Foo, 20);
    QCOMPARE(cache.size(), 2);

    for (int i = 0; i < 10; ++i) {
        cache.remove("alpha");
        QCOMPARE(cache.size(), 1);
        QCOMPARE(cache.totalCost(), 20);
    }

    cache.setMaxCost(1);
    QCOMPARE(cache.size(), 0);
    cache.remove("beta");
    QCOMPARE(cache.size(), 0);
}

void tst_QCache::take()
{
    QCache<QString, Foo> cache;
    QCOMPARE(cache.take(QString()), (Foo*)0);
    QCOMPARE(cache.take("alpha"), (Foo*)0);
    QVERIFY(cache.isEmpty());

    Foo *f1 = new Foo;
    cache.insert("alpha", f1, 10);
    QCOMPARE(cache.size(), 1);
    QVERIFY(cache["alpha"] == f1);

    cache.insert("beta", new Foo, 20);
    QCOMPARE(cache.size(), 2);

    QCOMPARE(cache.take("alpha"), f1);
    QCOMPARE(cache.size(), 1);
    QCOMPARE(cache.totalCost(), 20);
    QCOMPARE(Foo::count, 2);
    delete f1;
    QCOMPARE(Foo::count, 1);

    QCOMPARE(cache.take("alpha"), (Foo*)0);
    QCOMPARE(Foo::count, 1);
    QCOMPARE(cache.size(), 1);
    QCOMPARE(cache.totalCost(), 20);

    cache.setMaxCost(1);
    QCOMPARE(cache.size(), 0);
    QCOMPARE(cache.take("beta"), (Foo*)0);
    QCOMPARE(cache.size(), 0);
}

struct KeyType
{
    int foo;

    KeyType(int x) : foo(x) {}

private:
    KeyType &operator=(const KeyType &);
};

struct ValueType
{
    int foo;

    ValueType(int x) : foo(x) {}

private:
    ValueType(const ValueType &);
    ValueType &operator=(const ValueType &);
};

bool operator==(const KeyType &key1, const KeyType &key2)
{
    return key1.foo == key2.foo;
}

uint qHash(const KeyType &key)
{
    return qHash(key.foo);
}

void tst_QCache::axioms_on_key_type()
{
    QCache<KeyType, ValueType> foo;
    foo.setMaxCost(1);
    foo.clear();
    foo.insert(KeyType(123), new ValueType(123));
    foo.object(KeyType(123));
    foo.contains(KeyType(456));
    foo[KeyType(456)];
    foo.remove(KeyType(456));
    foo.remove(KeyType(123));
    foo.take(KeyType(789));
// If this fails, contact the maintaner
    QVERIFY(sizeof(QHash<int, int>) == sizeof(void *));
}

QTEST_APPLESS_MAIN(tst_QCache)
#include "tst_qcache.moc"
