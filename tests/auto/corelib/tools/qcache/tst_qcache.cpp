// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qcache.h>

class tst_QCache : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    void cleanupTestCase();
private slots:
    void empty();
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
    void largeCache();
    void internalChainOrderAfterEntryUpdate();
    void emplaceLowerCost();
    void trimWithMovingAcrossSpans();
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

void tst_QCache::empty()
{
    QCache<int, int> cache;
    QCOMPARE(cache.size(), 0);
    QCOMPARE(cache.count(), 0);
    QVERIFY(cache.isEmpty());
    QVERIFY(!cache.contains(1));
    QCOMPARE(cache.keys().size(), 0);
    QCOMPARE(cache.take(1), nullptr);
    QVERIFY(!cache.remove(1));
    QCOMPARE(cache.object(1), nullptr);
    QCOMPARE(cache[1], nullptr);
    QCOMPARE(cache.totalCost(), 0);
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
    constexpr KeyType(const KeyType &o) noexcept : foo(o.foo) {}

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

size_t qHash(const KeyType &key)
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

void tst_QCache::largeCache()
{
    QCache<int, int> cache;
    cache.setMaxCost(500);
    for (int i = 0; i < 1000; ++i) {
        for (int j = 0; j < qMax(0, i - 500); ++j)
            QVERIFY(!cache.contains(j));
        for (int j = qMax(0, i - 500); j < i; ++j)
            QVERIFY(cache.contains(j));
        cache.insert(i, new int);
    }
    cache.clear();
    QVERIFY(cache.size() == 0);
}

// The internal chain could lose track of some objects.
// Make sure it doesn't happen again.
void tst_QCache::internalChainOrderAfterEntryUpdate()
{
    QCache<QString, int> cache;
    cache.setMaxCost(20);
    cache.insert(QString::number(1), new int, 1);
    cache.insert(QString::number(2), new int, 1);
    cache.insert(QString::number(1), new int, 1);
    // If the chain is still 'in order' then setting maxCost == 0 should
    // a. not crash, and
    // b. remove all the elements in the QHash
    cache.setMaxCost(0);
    QCOMPARE(cache.size(), 0);
}

void tst_QCache::emplaceLowerCost()
{
    QCache<QString, int> cache;
    cache.setMaxCost(5);
    cache.insert("a", new int, 3); // insert high cost
    cache.insert("a", new int, 1); // and then exchange it with a lower-cost object
    QCOMPARE(cache.totalCost(), 1);
    cache.remove("a"); // then remove the object
    // The cache should now have a cost == 0 and be empty.
    QCOMPARE(cache.totalCost(), 0);
    QVERIFY(cache.isEmpty());
}

struct TrivialHashType {
    int i = -1;
    size_t hash = 0;

    TrivialHashType(int i, size_t hash) : i(i), hash(hash) {}
    TrivialHashType(const TrivialHashType &o) noexcept = default;
    TrivialHashType &operator=(const TrivialHashType &o) noexcept = default;
    TrivialHashType(TrivialHashType &&o) noexcept : i(o.i), hash(o.hash) {
        o.i = -1;
        o.hash = 0;
    }
    TrivialHashType &operator=(TrivialHashType &&o) noexcept {
        i = o.i;
        hash = o.hash;
        o.i = -1;
        o.hash = 0;
        return *this;
    }


    friend bool operator==(const TrivialHashType &lhs, const TrivialHashType &rhs)
    {
        return lhs.i == rhs.i;
    }
};
quint64 qHash(TrivialHashType t, size_t seed = 0)
{
    Q_UNUSED(seed);
    return t.hash;
}

// During trim(), if the Node we have a pointer to in the function is moved
// to another span in the hash table, our pointer would end up pointing to
// garbage memory. Test that this no longer happens
void tst_QCache::trimWithMovingAcrossSpans()
{
    qsizetype numBuckets = [](){
        QHash<int, int> h;
        h.reserve(1);
        // Beholden to QHash internals:
        return h.capacity() << 1;
    }();

    QCache<TrivialHashType, int> cache;
    cache.setMaxCost(1000);

    auto lastBucketInSpan = size_t(numBuckets - 1);
    // If this fails then the test is no longer valid
    QCOMPARE(QHashPrivate::GrowthPolicy::bucketForHash(numBuckets, lastBucketInSpan),
             lastBucketInSpan);

    // Pad some space so we have two spans:
    for (int i = 2; i < numBuckets; ++i)
        cache.insert({i, 0}, nullptr);

    // These two are vying for the last bucket in the first span,
    // when '0' is deleted, '1' is moved across the span boundary,
    // invalidating any pointer to its Node.
    cache.insert({0, lastBucketInSpan}, nullptr);
    cache.insert({1, lastBucketInSpan}, nullptr);

    QCOMPARE(cache.size(), numBuckets);
    cache.setMaxCost(0);
    QCOMPARE(cache.size(), 0);
}

QTEST_APPLESS_MAIN(tst_QCache)
#include "tst_qcache.moc"
