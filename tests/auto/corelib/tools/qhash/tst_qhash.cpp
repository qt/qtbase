// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qdebug.h>
#include <qhash.h>
#include <qmap.h>
#include <qscopeguard.h>
#include <qset.h>

#include <algorithm>
#include <vector>
#include <unordered_set>
#include <string>
#include <tuple>

#include <qsemaphore.h>

#include <private/qcomparisontesthelper_p.h>

using namespace Qt::StringLiterals;

class tst_QHash : public QObject
{
    Q_OBJECT
private slots:
    void insert1();
    void erase();
    void erase_edge_case();
    void key();
    void keys();

    void swap();
    void count(); // copied from tst_QMap
    void clear(); // copied from tst_QMap
    void empty(); // copied from tst_QMap
    void find(); // copied from tst_QMap
    void constFind(); // copied from tst_QMap
    void contains(); // copied from tst_QMap
    void qhash();
    void take(); // copied from tst_QMap
    void comparisonCompiles();
    void operator_eq(); // slightly modified from tst_QMap
    void heterogeneousSearch();
    void heterogeneousSearchConstKey();
    void heterogeneousSearchByteArray();
    void heterogeneousSearchString();
    void heterogeneousSearchLatin1String();

    void rehash_isnt_quadratic();
    void dont_need_default_constructor();
    void qmultihash_specific();
    void qmultihash_qhash_rvalue_ref_ctor();
    void qmultihash_qhash_rvalue_ref_unite();
    void qmultihashUnite();
    void qmultihashSize();
    void qmultihashHeterogeneousSearch();
    void qmultihashHeterogeneousSearchConstKey();
    void qmultihashHeterogeneousSearchByteArray();
    void qmultihashHeterogeneousSearchString();
    void qmultihashHeterogeneousSearchLatin1String();

    void compare();
    void compare2();
    void iterators(); // sligthly modified from tst_QMap
    void multihashIterators();
    void iteratorsInEmptyHash();
    void keyIterator();
    void multihashKeyIterator();
    void keyValueIterator();
    void multihashKeyValueIterator();
    void keyValueIteratorInEmptyHash();
    void keys_values_uniqueKeys(); // slightly modified from tst_QMap

    void const_shared_null();
    void twoArguments_qHash();
    void initializerList();
    void eraseValidIteratorOnSharedHash();
    void equal_range();
    void insert_hash();
    void multiHashStoresInReverseInsertionOrder();

    void emplace();
    void tryEmplace();

    void insertOrAssign();

    void badHashFunction();
    void hashOfHash();

    void stdHash();

    void countInEmptyHash();
    void removeInEmptyHash();
    void valueInEmptyHash();
    void fineTuningInEmptyHash();

    void reserveShared();
    void reserveLessThanCurrentAmount();
    void reserveKeepCapacity_data();
    void reserveKeepCapacity();

    void QTBUG98265();

    void detachAndReferences();

    void lookupUsingKeyIterator();

    void squeeze();
    void squeezeShared();
};

struct IdentityTracker {
    int value, id;
};

inline size_t qHash(IdentityTracker key) { return qHash(key.value); }
inline bool operator==(IdentityTracker lhs, IdentityTracker rhs) { return lhs.value == rhs.value; }


struct Foo {
    static int count;
    Foo():c(count) { ++count; }
    Foo(const Foo& o):c(o.c) { ++count; }
    ~Foo() { --count; }
    constexpr Foo &operator=(const Foo &o) noexcept { c = o.c; return *this; }

    int c;
    int data[8];
};

int Foo::count = 0;

//copied from tst_QMap.cpp
class MyClass
{
public:
    MyClass()
    {
        ++count;
    }
    MyClass( const QString& c)
    {
        count++;
        str = c;
    }
    MyClass(const QString &a, const QString &b)
    {
        count++;
        str = a + b;
    }
    ~MyClass() {
        count--;
    }
    MyClass( const MyClass& c ) {
        count++;
        ++copies;
        str = c.str;
    }
    MyClass &operator =(const MyClass &o) {
        str = o.str;
        ++copies;
        return *this;
    }
    MyClass(MyClass &&c) {
        count++;
        ++moves;
        str = c.str;
    }
    MyClass &operator =(MyClass &&o) {
        str = o.str;
        ++moves;
        return *this;
    }

    QString str;
    static int count;
    static int copies;
    static int moves;
};

int MyClass::count  = 0;
int MyClass::copies = 0;
int MyClass::moves  = 0;

typedef QHash<QString, MyClass> MyMap;

//void tst_QMap::count()
void tst_QHash::count()
{
    {
        MyMap map;
        MyMap map2( map );
        QCOMPARE( map.size(), 0 );
        QCOMPARE( map2.size(), 0 );
        QCOMPARE( MyClass::count, 0 );
        // detach
        map2["Hallo"] = MyClass( "Fritz" );
        QCOMPARE( map.size(), 0 );
        QCOMPARE( map2.size(), 1 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 1 );
#endif
    }
    QCOMPARE( MyClass::count, 0 );

    {
        typedef QHash<QString, MyClass> Map;
        Map map;
        QCOMPARE( map.size(), 0);
        map.insert( "Torben", MyClass("Weis") );
        QCOMPARE( map.size(), 1 );
        map.insert( "Claudia", MyClass("Sorg") );
        QCOMPARE( map.size(), 2 );
        map.insert( "Lars", MyClass("Linzbach") );
        map.insert( "Matthias", MyClass("Ettrich") );
        map.insert( "Sue", MyClass("Paludo") );
        map.insert( "Eirik", MyClass("Eng") );
        map.insert( "Haavard", MyClass("Nord") );
        map.insert( "Arnt", MyClass("Gulbrandsen") );
        map.insert( "Paul", MyClass("Tvete") );
        QCOMPARE( map.size(), 9 );
        map.insert( "Paul", MyClass("Tvete 1") );
        map.insert( "Paul", MyClass("Tvete 2") );
        map.insert( "Paul", MyClass("Tvete 3") );
        map.insert( "Paul", MyClass("Tvete 4") );
        map.insert( "Paul", MyClass("Tvete 5") );
        map.insert( "Paul", MyClass("Tvete 6") );

        QCOMPARE( map.size(), 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        Map map2( map );
        QVERIFY( map2.size() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2.insert( "Kay", MyClass("Roemer") );
        QVERIFY( map2.size() == 10 );
        QVERIFY( map.size() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 19 );
#endif

        map2 = map;
        QVERIFY( map.size() == 9 );
        QVERIFY( map2.size() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2.insert( "Kay", MyClass("Roemer") );
        QVERIFY( map2.size() == 10 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 19 );
#endif

        map2.clear();
        QVERIFY( map.size() == 9 );
        QVERIFY( map2.size() == 0 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2 = map;
        QVERIFY( map.size() == 9 );
        QVERIFY( map2.size() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2.clear();
        QVERIFY( map.size() == 9 );
        QVERIFY( map2.size() == 0 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map.remove( "Lars" );
        QVERIFY( map.size() == 8 );
        QVERIFY( map2.size() == 0 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 8 );
#endif

        map.remove( "Mist" );
        QVERIFY( map.size() == 8 );
        QVERIFY( map2.size() == 0 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 8 );
#endif
    }
    QVERIFY( MyClass::count == 0 );

    {
         typedef QHash<QString,MyClass> Map;
         Map map;
         map["Torben"] = MyClass("Weis");
#ifndef Q_CC_SUN
         QVERIFY( MyClass::count == 1 );
#endif
         QVERIFY( map.size() == 1 );

         (void)map["Torben"].str;
         (void)map["Lars"].str;
#ifndef Q_CC_SUN
         QVERIFY( MyClass::count == 2 );
#endif
         QVERIFY( map.size() == 2 );

         const Map& cmap = map;
         (void)cmap["Depp"].str;
#ifndef Q_CC_SUN
         QVERIFY( MyClass::count == 2 );
#endif
         QVERIFY( map.size() == 2 );
         QVERIFY( cmap.size() == 2 );
    }
    QCOMPARE( MyClass::count, 0 );
    {
        for ( int i = 0; i < 100; ++i )
        {
            QHash<int, MyClass> map;
            for (int j = 0; j < i; ++j)
                map.insert(j, MyClass(QString::number(j)));
        }
        QCOMPARE( MyClass::count, 0 );
    }
    QCOMPARE( MyClass::count, 0 );
}
void tst_QHash::insert1()
{
    const char *hello = "hello";
    const char *world = "world";
    const char *allo = "allo";
    const char *monde = "monde";

    {
        typedef QHash<QString, QString> Hash;
        Hash hash;
        QString key = QLatin1String("  ");
        for (int i = 0; i < 10; ++i) {
            key[0] = QChar(i + '0');
            for (int j = 0; j < 10; ++j) {
                key[1] = QChar(j + '0');
                hash.insert(key, "V" + key);
            }
        }

        for (int i = 0; i < 10; ++i) {
            key[0] = QChar(i + '0');
            for (int j = 0; j < 10; ++j) {
                key[1] = QChar(j + '0');
                hash.remove(key);
            }
        }
    }

    {
        typedef QHash<int, const char *> Hash;
        Hash hash;
        hash.insert(1, hello);
        hash.insert(2, world);

        QVERIFY(hash.size() == 2);
        QVERIFY(!hash.isEmpty());

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wself-assign-overloaded")
        {
            Hash hash2 = hash;
            hash2 = hash;
            hash = hash2;
            hash2 = hash2;
            hash = hash;
            hash2.clear();
            hash2 = hash2;
            QVERIFY(hash2.size() == 0);
            QVERIFY(hash2.isEmpty());
        }
        QVERIFY(hash.size() == 2);
QT_WARNING_POP

        {
            Hash hash2 = hash;
            hash2[1] = allo;
            hash2[2] = monde;

            QVERIFY(hash2[1] == allo);
            QVERIFY(hash2[2] == monde);
            QVERIFY(hash[1] == hello);
            QVERIFY(hash[2] == world);

            hash2[1] = hash[1];
            hash2[2] = hash[2];

            QVERIFY(hash2[1] == hello);
            QVERIFY(hash2[2] == world);

            hash[1] = hash[1];
            QVERIFY(hash[1] == hello);
        }
        {
            Hash hash2 = hash;
            hash2.detach();
            hash2.remove(1);
            QVERIFY(hash2.size() == 1);
            hash2.remove(1);
            QVERIFY(hash2.size() == 1);
            hash2.remove(0);
            QVERIFY(hash2.size() == 1);
            hash2.remove(2);
            QVERIFY(hash2.size() == 0);
            QVERIFY(hash.size() == 2);
        }

        hash.detach();

        {
            Hash::iterator it1 = hash.find(1);
            QVERIFY(it1 != hash.end());

            Hash::iterator it2 = hash.find(0);
            QVERIFY(it2 != hash.begin());
            QVERIFY(it2 == hash.end());

            *it1 = monde;
            QVERIFY(*it1 == monde);
            QVERIFY(hash[1] == monde);

            *it1 = hello;
            QVERIFY(*it1 == hello);
            QVERIFY(hash[1] == hello);

            hash[1] = monde;
            QVERIFY(it1.key() == 1);
            QVERIFY(it1.value() == monde);
            QVERIFY(*it1 == monde);
            QVERIFY(hash[1] == monde);

            hash[1] = hello;
            QVERIFY(*it1 == hello);
            QVERIFY(hash[1] == hello);
        }

        {
            const Hash hash2 = hash;

            Hash::const_iterator it1 = hash2.find(1);
            QVERIFY(it1 != hash2.end());
            QVERIFY(it1.key() == 1);
            QVERIFY(it1.value() == hello);
            QVERIFY(*it1 == hello);

            Hash::const_iterator it2 = hash2.find(2);
            QVERIFY(it1 != it2);
            QVERIFY(it1 != hash2.end());
            QVERIFY(it2 != hash2.end());

            int count = 0;
            it1 = hash2.begin();
            while (it1 != hash2.end()) {
                count++;
                ++it1;
            }
            QVERIFY(count == 2);

            count = 0;
            it1 = hash.constBegin();
            while (it1 != hash.constEnd()) {
                count++;
                ++it1;
            }
            QVERIFY(count == 2);
        }

        {
            QVERIFY(hash.contains(1));
            QVERIFY(hash.contains(2));
            QVERIFY(!hash.contains(0));
            QVERIFY(!hash.contains(3));
        }

        {
            QVERIFY(hash.value(1) == hello);
            QVERIFY(hash.value(2) == world);
            QVERIFY(hash.value(3) == 0);
            QVERIFY(hash.value(1, allo) == hello);
            QVERIFY(hash.value(2, allo) == world);
            QVERIFY(hash.value(3, allo) == allo);
            QVERIFY(hash.value(0, monde) == monde);
        }

        {
            QHash<int,Foo> hash;
            for (int i = 0; i < 10; i++)
                hash.insert(i, Foo());
            QVERIFY(Foo::count == 10);
            hash.remove(7);
            QVERIFY(Foo::count == 9);

        }
        QVERIFY(Foo::count == 0);
        {
            QHash<int, int*> hash;
            QVERIFY(((const QHash<int,int*>*) &hash)->operator[](7) == 0);
        }
    }
    {
        QHash<IdentityTracker, int> hash;
        QCOMPARE(hash.size(), 0);
        QVERIFY(!hash.isDetached());
        const int dummy = -1;
        IdentityTracker id00 = {0, 0}, id01 = {0, 1}, searchKey = {0, dummy};
        QCOMPARE(hash.insert(id00, id00.id).key().id, id00.id);
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash.insert(id01, id01.id).key().id, id00.id); // first key inserted is kept
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash.find(searchKey).value(), id01.id);  // last-inserted value
        QCOMPARE(hash.find(searchKey).key().id, id00.id); // but first-inserted key
    }
}

void tst_QHash::erase()
{
    QHash<int, int> h1;
    h1.insert(1, 2);
    h1.erase(h1.begin());
    QVERIFY(h1.size() == 0);
    QVERIFY(h1.begin() == h1.end());
    h1.insert(3, 4);
    QVERIFY(*h1.begin() == 4);
    h1.insert(5, 6);
    QVERIFY(h1.size() == 2);
    QHash<int, int>::iterator it1 = h1.begin();
    ++it1;
    it1 = h1.erase(it1);
    QVERIFY(it1 == h1.end());
    h1.insert(7, 8);
    h1.insert(9, 10);
    it1 = h1.begin();
    int n = 0;
    while (it1 != h1.end()) {
        it1 = h1.erase(it1);
        ++n;
    }
    QVERIFY(n == 3);

    QMultiHash<int, int> h2;
    h2.insert(20, 41);
    h2.insert(20, 42);
    QVERIFY(h2.size() == 2);
    auto bit = h2.begin();
    auto mit = h2.erase(bit);
    mit = h2.erase(h2.begin());
    QVERIFY(mit == h2.end());

    h2 = QMultiHash<int, int>();
    h2.emplace(1, 1);
    h2.emplace(1, 2);
    h2.emplace(3, 1);
    h2.emplace(3, 4);
    QMultiHash<int, int> h3 = h2;
    auto it = h3.constFind(3);
    ++it;
    QVERIFY(h3.isSharedWith(h2));
    it = h3.erase(it);
    QVERIFY(!h3.isSharedWith(h2));
    if (it != h3.cend()) {
        auto it2 = h3.constFind(it.key());
        QCOMPARE(it, it2);
    }
}

/*
    With a specific seed we could end up in a situation where, upon deleting the
    last entry in a QHash, the returned iterator would not point to the end()
    iterator.
*/
void tst_QHash::erase_edge_case()
{
    QHashSeed::setDeterministicGlobalSeed();
    auto resetSeed = qScopeGuard([&]() {
        QHashSeed::resetRandomGlobalSeed();
    });

    QHash<int, int> h1;
    h1.reserve(2);
    qsizetype capacity = h1.capacity();
    // Beholden to QHash internals:
    qsizetype numBuckets = capacity << 1;

    // Find some keys which will both be slotted into the last bucket:
    int keys[2];
    int index = 0;
    for (qsizetype i = 0; i < numBuckets * 4 && index < 2; ++i) {
        const size_t hash = qHash(i, QHashSeed::globalSeed());
        const size_t bucketForHash = QHashPrivate::GrowthPolicy::bucketForHash(numBuckets, hash);
        if (qsizetype(bucketForHash) == numBuckets - 1)
            keys[index++] = i;
    }
    QCOMPARE(index, 2); // Sanity check. If this fails then the test needs an update!

    // As mentioned earlier these are both calculated to be in the last bucket:
    h1.insert(keys[0], 4);
    h1.insert(keys[1], 6);
    // As a sanity-check, make sure that the key we inserted last is the first one (because its
    // allocation to the last bucket would make it wrap around):
    // NOTE: If this fails this then this test may need an update!!!
    QCOMPARE(h1.constBegin().key(), keys[1]);
    // Then we delete the last entry:
    QHash<int, int>::iterator it1 = h1.begin();
    ++it1;
    it1 = h1.erase(it1);
    // Now, since we deleted the last entry, the iterator should be at the end():
    QVERIFY(it1 == h1.end());
}

void tst_QHash::key()
{
    {
        QString def("default value");

        QHash<QString, int> hash1;
        QCOMPARE(hash1.key(1), QString());
        QCOMPARE(hash1.key(1, def), def);
        QVERIFY(!hash1.isDetached());

        hash1.insert("one", 1);
        QCOMPARE(hash1.key(1), QLatin1String("one"));
        QCOMPARE(hash1.key(1, def), QLatin1String("one"));
        QCOMPARE(hash1.key(2), QString());
        QCOMPARE(hash1.key(2, def), def);

        hash1.insert("two", 2);
        QCOMPARE(hash1.key(1), QLatin1String("one"));
        QCOMPARE(hash1.key(1, def), QLatin1String("one"));
        QCOMPARE(hash1.key(2), QLatin1String("two"));
        QCOMPARE(hash1.key(2, def), QLatin1String("two"));
        QCOMPARE(hash1.key(3), QString());
        QCOMPARE(hash1.key(3, def), def);

        hash1.insert("deux", 2);
        QCOMPARE(hash1.key(1), QLatin1String("one"));
        QCOMPARE(hash1.key(1, def), QLatin1String("one"));
        QVERIFY(hash1.key(2) == QLatin1String("deux") || hash1.key(2) == QLatin1String("two"));
        QVERIFY(hash1.key(2, def) == QLatin1String("deux") || hash1.key(2, def) == QLatin1String("two"));
        QCOMPARE(hash1.key(3), QString());
        QCOMPARE(hash1.key(3, def), def);
    }

    {
        int def = 666;

        QHash<int, QString> hash2;
        QCOMPARE(hash2.key("one"), 0);
        QCOMPARE(hash2.key("one", def), def);
        QVERIFY(!hash2.isDetached());

        hash2.insert(1, "one");
        QCOMPARE(hash2.key("one"), 1);
        QCOMPARE(hash2.key("one", def), 1);
        QCOMPARE(hash2.key("two"), 0);
        QCOMPARE(hash2.key("two", def), def);

        hash2.insert(2, "two");
        QCOMPARE(hash2.key("one"), 1);
        QCOMPARE(hash2.key("one", def), 1);
        QCOMPARE(hash2.key("two"), 2);
        QCOMPARE(hash2.key("two", def), 2);
        QCOMPARE(hash2.key("three"), 0);
        QCOMPARE(hash2.key("three", def), def);

        hash2.insert(3, "two");
        QCOMPARE(hash2.key("one"), 1);
        QCOMPARE(hash2.key("one", def), 1);
        QVERIFY(hash2.key("two") == 2 || hash2.key("two") == 3);
        QVERIFY(hash2.key("two", def) == 2 || hash2.key("two", def) == 3);
        QCOMPARE(hash2.key("three"), 0);
        QCOMPARE(hash2.key("three", def), def);

        hash2.insert(-1, "two");
        QVERIFY(hash2.key("two") == 2 || hash2.key("two") == 3 || hash2.key("two") == -1);
        QVERIFY(hash2.key("two", def) == 2 || hash2.key("two", def) == 3 || hash2.key("two", def) == -1);

        hash2.insert(0, "zero");
        QCOMPARE(hash2.key("zero"), 0);
        QCOMPARE(hash2.key("zero", def), 0);
    }

    {
        const int def = -1;
        QMultiHash<int, QString> hash;
        QCOMPARE(hash.key("val"), 0);
        QCOMPARE(hash.key("val", def), def);
        QVERIFY(!hash.isDetached());

        hash.insert(1, "value1");
        hash.insert(1, "value2");
        hash.insert(2, "value1");

        QCOMPARE(hash.key("value2"), 1);
        const auto key = hash.key("value1");
        QVERIFY(key == 1 || key == 2);
        QCOMPARE(hash.key("value"), 0);
        QCOMPARE(hash.key("value", def), def);
    }
}

template <typename T>
QList<T> sorted(const QList<T> &list)
{
    QList<T> res = list;
    std::sort(res.begin(), res.end());
    return res;
}

void tst_QHash::keys()
{
    {
        QHash<QString, int> hash;
        QVERIFY(hash.keys().isEmpty());
        QVERIFY(hash.keys(1).isEmpty());
        QVERIFY(!hash.isDetached());

        hash.insert("key1", 1);
        hash.insert("key2", 2);
        hash.insert("key3", 1);

        QCOMPARE(sorted(hash.keys()), QStringList({ "key1", "key2", "key3" }));
        QCOMPARE(sorted(hash.keys(1)), QStringList({ "key1", "key3" }));
    }
    {
        QMultiHash<QString, int> hash;
        QVERIFY(hash.keys().isEmpty());
        QVERIFY(hash.keys(1).isEmpty());
        QVERIFY(!hash.isDetached());

        hash.insert("key1", 1);
        hash.insert("key2", 1);
        hash.insert("key1", 2);

        QCOMPARE(sorted(hash.keys()), QStringList({ "key1", "key1", "key2" }));
        QCOMPARE(sorted(hash.keys(1)), QStringList({ "key1", "key2" }));
    }
}

void tst_QHash::swap()
{
    QHash<int,QString> h1, h2;
    h1[0] = "h1[0]";
    h2[1] = "h2[1]";
    h1.swap(h2);
    QCOMPARE(h1.value(1),QLatin1String("h2[1]"));
    QCOMPARE(h2.value(0),QLatin1String("h1[0]"));
}

// copied from tst_QMap
void tst_QHash::clear()
{
    {
        MyMap map;
        map.clear();
        QVERIFY( map.isEmpty() );
        map.insert( "key", MyClass( "value" ) );
        map.clear();
        QVERIFY( map.isEmpty() );
        map.insert( "key0", MyClass( "value0" ) );
        map.insert( "key0", MyClass( "value1" ) );
        map.insert( "key1", MyClass( "value2" ) );
        map.clear();
        QVERIFY( map.isEmpty() );
    }
    QCOMPARE( MyClass::count, int(0) );

    {
        QMultiHash<QString, MyClass> multiHash;
        multiHash.clear();
        QVERIFY(multiHash.isEmpty());

        multiHash.insert("key", MyClass("value0"));
        QVERIFY(!multiHash.isEmpty());
        multiHash.clear();
        QVERIFY(multiHash.isEmpty());

        multiHash.insert("key0", MyClass("value0"));
        multiHash.insert("key0", MyClass("value1"));
        multiHash.insert("key1", MyClass("value2"));
        QVERIFY(!multiHash.isEmpty());
        multiHash.clear();
        QVERIFY(multiHash.isEmpty());
    }
    QCOMPARE(MyClass::count, int(0));
}
//copied from tst_QMap
void tst_QHash::empty()
{
    QHash<int, QString> map1;

    QVERIFY(map1.isEmpty());
    QVERIFY(map1.empty());

    map1.insert(1, "one");
    QVERIFY(!map1.isEmpty());
    QVERIFY(!map1.empty());

    map1.clear();
    QVERIFY(map1.isEmpty());
    QVERIFY(map1.empty());
}

//copied from tst_QMap
void tst_QHash::find()
{
    const QHash<int, QString> constEmptyHash;
    QVERIFY(constEmptyHash.find(1) == constEmptyHash.end());
    QVERIFY(!constEmptyHash.isDetached());

    QHash<int, QString> map1;
    QString testString="Teststring %0";
    QString compareString;
    int i,count=0;

    QVERIFY(map1.find(1) == map1.end());
    QVERIFY(!map1.isDetached());

    map1.insert(1,"Mensch");
    map1.insert(1,"Mayer");
    map1.insert(2,"Hej");

    QCOMPARE(map1.find(1).value(), QLatin1String("Mayer"));
    QCOMPARE(map1.find(2).value(), QLatin1String("Hej"));

    const QMultiHash<int, QString> constEmptyMultiHash;
    QVERIFY(constEmptyMultiHash.find(1) == constEmptyMultiHash.cend());
    QVERIFY(constEmptyMultiHash.find(1, "value") == constEmptyMultiHash.cend());
    QVERIFY(!constEmptyMultiHash.isDetached());

    QMultiHash<int, QString> emptyMultiHash;
    QVERIFY(emptyMultiHash.find(1) == emptyMultiHash.end());
    QVERIFY(emptyMultiHash.find(1, "value") == emptyMultiHash.end());
    QVERIFY(!emptyMultiHash.isDetached());

    QMultiHash<int, QString> multiMap(map1);
    for (i = 3; i < 10; ++i) {
        compareString = testString.arg(i);
        multiMap.insert(4, compareString);
    }

    auto it = multiMap.constFind(4);

    for (i = 9; i > 2 && it != multiMap.constEnd() && it.key() == 4; --i) {
        compareString = testString.arg(i);
        QVERIFY(it.value() == compareString);
        ++it;
        ++count;
    }
    QCOMPARE(count, 7);
}

// copied from tst_QMap
void tst_QHash::constFind()
{
    QHash<int, QString> map1;
    QString testString="Teststring %0";
    QString compareString;
    int i,count=0;

    QVERIFY(map1.constFind(1) == map1.constEnd());
    QVERIFY(!map1.isDetached());

    map1.insert(1,"Mensch");
    map1.insert(1,"Mayer");
    map1.insert(2,"Hej");

    QCOMPARE(map1.constFind(1).value(), QLatin1String("Mayer"));
    QCOMPARE(map1.constFind(2).value(), QLatin1String("Hej"));

    QMultiHash<int, QString> emptyMultiHash;
    QVERIFY(emptyMultiHash.constFind(1) == emptyMultiHash.constEnd());
    QVERIFY(!emptyMultiHash.isDetached());

    QMultiHash<int, QString> multiMap(map1);
    for (i = 3; i < 10; ++i) {
        compareString = testString.arg(i);
        multiMap.insert(4, compareString);
    }

    auto it = multiMap.constFind(4);

    for (i = 9; i > 2 && it != multiMap.constEnd() && it.key() == 4; --i) {
        compareString = testString.arg(i);
        QVERIFY(it.value() == compareString);
        ++it;
        ++count;
    }
    QCOMPARE(count, 7);
}

// copied from tst_QMap
void tst_QHash::contains()
{
    QHash<int, QString> map1;
    int i;

    QVERIFY(!map1.contains(1));
    QVERIFY(!map1.isDetached());

    map1.insert(1, "one");
    QVERIFY(map1.contains(1));

    for (i=2; i < 100; ++i)
        map1.insert(i, "teststring");
    for (i=99; i > 1; --i)
        QVERIFY(map1.contains(i));

    map1.remove(43);
    QVERIFY(!map1.contains(43));
}

namespace {
class QGlobalQHashSeedResetter
{
    int oldSeed;
public:
    // not entirely correct (may lost changes made by another thread between the query
    // of the old and the setting of the new seed), but setHashSeed() can't
    // return the old value, so this is the best we can do:
    explicit QGlobalQHashSeedResetter(int newSeed)
        : oldSeed(getHashSeed())
    {
        setHashSeed(newSeed);
    }
    ~QGlobalQHashSeedResetter()
    {
        setHashSeed(oldSeed);
    }

private:
    // The functions are implemented to replace the deprecated
    // qGlobalQHashSeed() and qSetGlobalQHashSeed()
    static int getHashSeed()
    {
        return int(QHashSeed::globalSeed() & INT_MAX);
    }
    static void setHashSeed(int seed)
    {
        if (seed == 0)
            QHashSeed::setDeterministicGlobalSeed();
        else
            QHashSeed::resetRandomGlobalSeed();
    }
};

template <typename Key, typename T>
QHash<T, Key> inverted(const QHash<Key, T> &in)
{
    QHash<T, Key> result;
    for (auto it = in.begin(), end = in.end(); it != end; ++it)
        result[it.value()] = it.key();
    return result;
}

template <typename AssociativeContainer>
void make_test_data(AssociativeContainer &c)
{
    c["one"] = "1";
    c["two"] = "2";
}

}

void tst_QHash::qhash()
{
    const QGlobalQHashSeedResetter seed1(0);

    QHash<QString, QString> hash1;
    make_test_data(hash1);
    const QHash<QString, QString> hsah1 = inverted(hash1);

    const QGlobalQHashSeedResetter seed2(1);

    QHash<QString, QString> hash2;
    make_test_data(hash2);
    const QHash<QString, QString> hsah2 = inverted(hash2);

    QCOMPARE(hash1, hash2);
    QCOMPARE(hsah1, hsah2);
    QCOMPARE(qHash(hash1), qHash(hash2));
    QCOMPARE(qHash(hsah1), qHash(hsah2));

    // by construction this is almost impossible to cause false collisions:
    QVERIFY(hash1 != hsah1);
    QVERIFY(hash2 != hsah2);
    QVERIFY(qHash(hash1) != qHash(hsah1));
    QVERIFY(qHash(hash2) != qHash(hsah2));
}

//copied from tst_QMap
void tst_QHash::take()
{
    {
        QHash<int, QString> map;
        QCOMPARE(map.take(1), QString());
        QVERIFY(!map.isDetached());

        map.insert(2, "zwei");
        map.insert(3, "drei");

        QCOMPARE(map.take(3), QLatin1String("drei"));
        QVERIFY(!map.contains(3));
    }
    {
        QMultiHash<int, QString> hash;
        QCOMPARE(hash.take(1), QString());
        QVERIFY(!hash.isDetached());

        hash.insert(1, "value1");
        hash.insert(2, "value2");
        hash.insert(1, "value3");

        // The docs tell that if there are multiple values for a key, then the
        // most recent is returned.
        QCOMPARE(hash.take(1), "value3");
        QCOMPARE(hash.take(1), "value1");
        QCOMPARE(hash.take(1), QString());
        QCOMPARE(hash.take(2), "value2");
    }
}

void tst_QHash::comparisonCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QHash<int, int>>();
    QTestPrivate::testEqualityOperatorsCompile<QHash<QString, QString>>();
    QTestPrivate::testEqualityOperatorsCompile<QHash<QString, int>>();
    QTestPrivate::testEqualityOperatorsCompile<QMultiHash<int, int>>();
    QTestPrivate::testEqualityOperatorsCompile<QMultiHash<QString, QString>>();
    QTestPrivate::testEqualityOperatorsCompile<QMultiHash<QString, int>>();
}

// slightly modified from tst_QMap
void tst_QHash::operator_eq()
{
    {
        // compare for equality:
        QHash<int, int> a;
        QHash<int, int> b;

        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);

        a.insert(1,1);
        b.insert(1,1);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);

        a.insert(0,1);
        b.insert(0,1);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);

        // compare for inequality:
        a.insert(42,0);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
        QT_TEST_EQUALITY_OPS(a, b, false);

        a.insert(65, -1);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
        QT_TEST_EQUALITY_OPS(a, b, false);

        b.insert(-1, -1);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
        QT_TEST_EQUALITY_OPS(a, b, false);
    }

    {
        // a more complex map
        QHash<QString, QString> a;
        QHash<QString, QString> b;

        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);

        a.insert("Hello", "World");
        QVERIFY(a != b);
        QVERIFY(!(a == b));
        QT_TEST_EQUALITY_OPS(a, b, false);

        b.insert("Hello", "World");
        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);

        a.insert("Goodbye", "cruel world");
        QVERIFY(a != b);
        QVERIFY(!(a == b));
        QT_TEST_EQUALITY_OPS(a, b, false);

        b.insert("Goodbye", "cruel world");

        // what happens if we insert nulls?
        a.insert(QString(), QString());
        QVERIFY(a != b);
        QVERIFY(!(a == b));
        QT_TEST_EQUALITY_OPS(a, b, false);

        // empty keys and null keys match:
        b.insert(QString(""), QString());
        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);
    }

    {
        QHash<QString, int> a;
        QHash<QString, int> b;

        a.insert("otto", 1);
        b.insert("willy", 1);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
        QT_TEST_EQUALITY_OPS(a, b, false);
    }

    // unlike multi-maps, multi-hashes should be equal iff their contents are equal,
    // regardless of insertion or iteration order

    {
        QMultiHash<int, int> a;
        QMultiHash<int, int> b;

        a.insert(0, 0);
        a.insert(0, 1);

        b.insert(0, 1);
        b.insert(0, 0);

        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);
    }

    {
        QMultiHash<int, int> a;
        QMultiHash<int, int> b;

        enum { Count = 100 };

        for (int key = 0; key < Count; ++key) {
            for (int value = 0; value < Count; ++value)
                a.insert(key, value);
        }

        for (int key = Count - 1; key >= 0; --key) {
            for (int value = 0; value < Count; ++value)
                b.insert(key, value);
        }

        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);
    }

    {
        QMultiHash<int, int> a;
        QMultiHash<int, int> b;

        enum {
            Count = 100,
            KeyStep = 17,   // coprime with Count
            ValueStep = 23, // coprime with Count
        };

        for (int key = 0; key < Count; ++key) {
            for (int value = 0; value < Count; ++value)
                a.insert(key, value);
        }

        // Generates two permutations of [0, Count) for the keys and values,
        // so that b will be identical to a, just built in a very different order.

        for (int k = 0; k < Count; ++k) {
           const int key = (k * KeyStep) % Count;
           for (int v = 0; v < Count; ++v)
               b.insert(key, (v * ValueStep) % Count);
        }

        QVERIFY(a == b);
        QVERIFY(!(a != b));
        QT_TEST_EQUALITY_OPS(a, b, true);
    }
}

struct HeterogeneousHashingType
{
#ifndef __cpp_aggregate_paren_init
    HeterogeneousHashingType() = default;
    HeterogeneousHashingType(const QString &string) : s(string) {}
#endif
    inline static int conversionCount = 0;
    QString s;

    Q_IMPLICIT operator QString() const
    {
        ++conversionCount;
        return s;
    }

    // std::equality_comparable_with requires we be self-comparable too
    friend bool operator==(const HeterogeneousHashingType &t1, const HeterogeneousHashingType &t2) { return t1.s == t2.s; };

    friend bool operator==(const QString &string, const HeterogeneousHashingType &tester)
    { return tester.s == string; }
#ifndef __cpp_impl_three_way_compare // full set required for detail::is_equality_comparable_with<QString>
    friend bool operator!=(const QString &string, const HeterogeneousHashingType &tester)
    { return !operator==(string, tester); }
    friend bool operator==(const HeterogeneousHashingType &tester, const QString &string)
    { return operator==(string, tester); }
    friend bool operator!=(const HeterogeneousHashingType &tester, const QString &string)
    { return !operator==(string, tester); }
#endif

    friend size_t qHash(const HeterogeneousHashingType &tester, size_t seed)
    { return qHash(tester.s, seed); }
};
QT_BEGIN_NAMESPACE
template <> struct QHashHeterogeneousSearch<QString, HeterogeneousHashingType> : std::true_type {};
template <> struct QHashHeterogeneousSearch<HeterogeneousHashingType, QString> : std::true_type {};
QT_END_NAMESPACE
static_assert(QHashPrivate::detail::is_equality_comparable_with<QString, HeterogeneousHashingType>::value);
static_assert(QHashPrivate::HeterogeneouslySearchableWith<QString, HeterogeneousHashingType>::value);
static_assert(QHashPrivate::HeterogeneouslySearchableWith<HeterogeneousHashingType, QString>::value);

template <typename T> struct HeterogeneousSearchTestHelper
{
    static void resetCounter() {}
    static void checkCounter() {}
};
template <> struct HeterogeneousSearchTestHelper<HeterogeneousHashingType>
{
    static void resetCounter()
    {
        HeterogeneousHashingType::conversionCount = 0;
    }
    static void checkCounter()
    {
        QTest::setThrowOnFail(true);
        auto scopeExit = qScopeGuard([] { QTest::setThrowOnFail(false); });
        QCOMPARE(HeterogeneousHashingType::conversionCount, 0);
    }
};

template <template <typename, typename> class Hash, typename String, typename View, typename Converter>
static void heterogeneousSearchTest(const QList<std::remove_const_t<String>> &keys, Converter conv)
{
    using Helper = HeterogeneousSearchTestHelper<View>;
    String key = keys.last();
    String otherKey = keys.first();
    auto keyHolder = conv(key);
    auto otherKeyHolder = conv(otherKey);
    View keyView(keyHolder);
    View otherKeyView(otherKeyHolder);

    Hash<String, qsizetype> hash;
    static constexpr bool IsMultiHash = !std::is_same_v<decltype(hash.remove(String())), bool>;
    hash[key] = keys.size();

    Helper::resetCounter();
    QVERIFY(hash.contains(keyView));
    QCOMPARE_EQ(hash.count(keyView), 1);
    QCOMPARE_EQ(hash.value(keyView), keys.size());
    QCOMPARE_EQ(hash.value(keyView, -1), keys.size());
    QCOMPARE_EQ(std::as_const(hash)[keyView], keys.size());
    QCOMPARE_EQ(hash.find(keyView), hash.begin());
    QCOMPARE_EQ(std::as_const(hash).find(keyView), hash.constBegin());
    QCOMPARE_EQ(hash.constFind(keyView), hash.constBegin());
    QCOMPARE_EQ(hash.equal_range(keyView), std::make_pair(hash.begin(), hash.end()));
    QCOMPARE_EQ(std::as_const(hash).equal_range(keyView),
                std::make_pair(hash.constBegin(), hash.constEnd()));
    Helper::checkCounter();

    QVERIFY(!hash.contains(otherKeyView));
    QCOMPARE_EQ(hash.count(otherKeyView), 0);
    QCOMPARE_EQ(hash.value(otherKeyView), 0);
    QCOMPARE_EQ(hash.value(otherKeyView, -1), -1);
    QCOMPARE_EQ(std::as_const(hash)[otherKeyView], 0);
    QCOMPARE_EQ(hash.find(otherKeyView), hash.end());
    QCOMPARE_EQ(std::as_const(hash).find(otherKeyView), hash.constEnd());
    QCOMPARE_EQ(hash.constFind(otherKeyView), hash.constEnd());
    QCOMPARE_EQ(hash.equal_range(otherKeyView), std::make_pair(hash.end(), hash.end()));
    QCOMPARE_EQ(std::as_const(hash).equal_range(otherKeyView),
                std::make_pair(hash.constEnd(), hash.constEnd()));
    Helper::checkCounter();

    // non-const versions
    QCOMPARE_EQ(hash[keyView], keys.size());    // already there
    Helper::checkCounter();

    QCOMPARE_EQ(hash[otherKeyView], 0);         // inserts
    Helper::resetCounter();
    hash[otherKeyView] = INT_MAX;
    Helper::checkCounter();

    if constexpr (IsMultiHash) {
        hash.insert(key, keys.size());
        QCOMPARE_EQ(hash.count(keyView), 2);

        // not depending on which of the two the current implementation finds
        QCOMPARE_NE(hash.value(keyView), 0);
        QCOMPARE_NE(hash.value(keyView, -1000), -1000);
        QCOMPARE_NE(std::as_const(hash)[keyView], 0);
        QCOMPARE_NE(hash.find(keyView), hash.end());
        QCOMPARE_NE(std::as_const(hash).find(keyView), hash.constEnd());
        QCOMPARE_NE(hash.constFind(keyView), hash.constEnd());
        QCOMPARE_NE(hash.equal_range(keyView), std::make_pair(hash.end(), hash.end()));
        QCOMPARE_NE(std::as_const(hash).equal_range(keyView),
                    std::make_pair(hash.constEnd(), hash.constEnd()));

        // QMultiHash-specific functions
        QVERIFY(hash.contains(keyView, keys.size()));
        QCOMPARE_EQ(hash.count(keyView, 0), 0);
        QCOMPARE_EQ(hash.count(keyView, keys.size()), 2);
        QCOMPARE_EQ(hash.values(keyView), QList<qsizetype>({ keys.size(), keys.size() }));

        hash.insert(key, -keys.size());
        QCOMPARE_EQ(hash.count(keyView), 3);
        QCOMPARE_EQ(hash.find(keyView, 0), hash.end());
        QCOMPARE_NE(hash.find(keyView, keys.size()), hash.end());
        QCOMPARE_NE(hash.find(keyView, -keys.size()), hash.end());
        QCOMPARE_EQ(std::as_const(hash).find(keyView, 0), hash.constEnd());
        QCOMPARE_NE(std::as_const(hash).find(keyView, keys.size()), hash.constEnd());
        QCOMPARE_NE(std::as_const(hash).find(keyView, -keys.size()), hash.constEnd());
        QCOMPARE_EQ(hash.constFind(keyView, 0), hash.constEnd());
        QCOMPARE_NE(hash.constFind(keyView, keys.size()), hash.constEnd());
        QCOMPARE_NE(hash.constFind(keyView, -keys.size()), hash.constEnd());

        // removals
        QCOMPARE_EQ(hash.remove(keyView, -keys.size()), 1);
        QCOMPARE_EQ(hash.remove(keyView), 2);
    } else {
        // removals
        QCOMPARE_EQ(hash.remove(keyView), true);
    }

    QCOMPARE_EQ(hash.take(otherKeyView), INT_MAX);
    QVERIFY(hash.isEmpty());
    Helper::checkCounter();

    // repeat with more keys
    for (qsizetype i = 0; i < keys.size() - 1; ++i) {
        hash.insert(keys[i], -(i + 1));
        hash.insert(keys[i], i + 1);
    }

    QVERIFY(!hash.contains(keyView));
    QCOMPARE_EQ(hash.count(keyView), 0);
    QCOMPARE_EQ(hash.value(keyView), 0);
    QCOMPARE_EQ(hash.value(keyView, -1), -1);
    QCOMPARE_EQ(std::as_const(hash)[keyView], 0);
    QCOMPARE_EQ(hash.find(keyView), hash.end());
    QCOMPARE_EQ(hash.constFind(keyView), hash.constEnd());
    Helper::checkCounter();
}

template <template <typename, typename> class Hash, typename String, typename View>
static void heterogeneousSearchTest(const QList<std::remove_const_t<String>> &keys)
{
    heterogeneousSearchTest<Hash, String, View>(keys, [](const String &s) { return View(s); });
}

template <template <typename, typename> class Hash, typename T>
static void heterogeneousSearchLatin1String(T)
{
    if constexpr (!T::value) {
        QSKIP("QLatin1StringView and QString do not have the same hash on this platform");
    } else {
        // similar to the above
        auto toLatin1 = [](const QString &s) { return s.toLatin1(); };
        heterogeneousSearchTest<Hash, QString, QLatin1StringView>({ "Hello", {}, "World" }, toLatin1);
    }
}

void tst_QHash::heterogeneousSearch()
{
    heterogeneousSearchTest<QHash, QString, HeterogeneousHashingType>({ "Hello", {}, "World" });
}

void tst_QHash::heterogeneousSearchConstKey()
{
    // QHash<const QString, X> seen in the wild (e.g. Qt Creator)
    heterogeneousSearchTest<QHash, const QString, HeterogeneousHashingType>({ "Hello", {}, "World" });
}

void tst_QHash::heterogeneousSearchByteArray()
{
    heterogeneousSearchTest<QHash, QByteArray, QByteArrayView>({ "Hello", {}, "World" });
}

void tst_QHash::heterogeneousSearchString()
{
    heterogeneousSearchTest<QHash, QString, QStringView>({ "Hello", {}, "World" });
}

void tst_QHash::heterogeneousSearchLatin1String()
{
    ::heterogeneousSearchLatin1String<QHash>(QHashHeterogeneousSearch<QString, QLatin1StringView>{});
}

void tst_QHash::compare()
{
    QHash<int, QString> hash1,hash2;
    QString testString = "Teststring %1";
    int i;

    for (i = 0; i < 1000; ++i)
        hash1.insert(i,testString.arg(i));

    for (--i; i >= 0; --i)
        hash2.insert(i,testString.arg(i));

    hash1.squeeze();
    hash2.squeeze();

    QVERIFY(hash1 == hash2);
    QVERIFY(!(hash1 != hash2));

    hash1.take(234);
    hash2.take(234);
    QVERIFY(hash1 == hash2);
    QVERIFY(!(hash1 != hash2));

    hash2.take(261);
    QVERIFY(!(hash1 == hash2));
    QVERIFY(hash1 != hash2);
}

void tst_QHash::compare2()
{
    QMultiHash<int, int> a;
    QMultiHash<int, int> b;

    a.insert(17, 1);
    a.insert(17 * 2, 1);
    b.insert(17 * 2, 1);
    b.insert(17, 1);
    QVERIFY(a == b);
    QVERIFY(b == a);

    a.insert(17, 2);
    a.insert(17 * 2, 3);
    b.insert(17 * 2, 3);
    b.insert(17, 2);
    QVERIFY(a == b);
    QVERIFY(b == a);

    a.insert(17, 4);
    a.insert(17 * 2, 5);
    b.insert(17 * 2, 4);
    b.insert(17, 5);
    QVERIFY(!(a == b));
    QVERIFY(!(b == a));

    a.clear();
    b.clear();
    a.insert(1, 1);
    a.insert(1, 2);
    a.insert(1, 3);
    b.insert(1, 1);
    b.insert(1, 2);
    b.insert(1, 3);
    b.insert(1, 4);
    QVERIFY(!(a == b));
    QVERIFY(!(b == a));
}

//sligthly modified from tst_QMap
void tst_QHash::iterators()
{
    QHash<int, QString> hash;
    QMap<int, QString> testMap;
    QString testString="Teststring %1";
    QString testString1;
    int i;

    for (i = 1; i < 100; ++i)
        hash.insert(i, testString.arg(i));

    //to get some chaos in the hash
    hash.squeeze();

    //STL-Style iterators

    QHash<int, QString>::iterator stlIt = hash.begin();
    for (stlIt = hash.begin(), i = 1; stlIt != hash.end() && i < 100; ++stlIt, ++i) {
            testMap.insert(i,stlIt.value());
            //QVERIFY(stlIt.value() == hash.value(
    }
    stlIt = hash.begin();

    QVERIFY(stlIt.value() == testMap.value(1));

    for (int i = 0; i < 5; ++i)
        ++stlIt;
    QVERIFY(stlIt.value() == testMap.value(6));

    stlIt++;
    QVERIFY(stlIt.value() == testMap.value(7));

    testMap.clear();

    //STL-Style const-iterators

    QHash<int, QString>::const_iterator cstlIt = hash.constBegin();
    for (cstlIt = hash.constBegin(), i = 1; cstlIt != hash.constEnd() && i < 100; ++cstlIt, ++i) {
            testMap.insert(i,cstlIt.value());
            //QVERIFY(stlIt.value() == hash.value(
    }
    cstlIt = hash.constBegin();

    QVERIFY(cstlIt.value() == testMap.value(1));

    for (int i = 0; i < 5; ++i)
        ++cstlIt;
    QVERIFY(cstlIt.value() == testMap.value(6));

    cstlIt++;
    QVERIFY(cstlIt.value() == testMap.value(7));

    testMap.clear();

    //Java-Style iterators

    QHashIterator<int, QString> javaIt(hash);

    //walk through
    i = 0;
    while(javaIt.hasNext()) {
        ++i;
        javaIt.next();
        testMap.insert(i,javaIt.value());
    }
    javaIt.toFront();
    i = 0;
    while(javaIt.hasNext()) {
        ++i;
        javaIt.next();
        QVERIFY(javaIt.value() == testMap.value(i));
    }

    //peekNext()
    javaIt.toFront();
    javaIt.next();
    while(javaIt.hasNext()) {
        testString = javaIt.value();
        testString1 = javaIt.peekNext().value();
        javaIt.next();
        QVERIFY(javaIt.value() == testString1);
    }
}

void tst_QHash::multihashIterators()
{
    QMultiHash<int, QString> hash;
    QMap<int, QString> referenceMap;
    QString testString = "Teststring %1-%2";
    int i = 0;

    // Add 5 elements for each key
    for (i = 0; i < 10; ++i) {
        for (int j = 0; j < 5; ++j)
            hash.insert(i, testString.arg(i, j));
    }

    hash.squeeze();

    // Verify that iteration is reproducible.

    // STL iterator
    QMultiHash<int, QString>::iterator stlIt;

    for (stlIt = hash.begin(), i = 1; stlIt != hash.end(); ++stlIt, ++i)
        referenceMap.insert(i, *stlIt);

    stlIt = hash.begin();
    QCOMPARE(*stlIt, referenceMap[1]);

    for (i = 0; i < 5; ++i)
        stlIt++;
    QCOMPARE(*stlIt, referenceMap[6]);

    for (i = 0; i < 44; ++i)
        stlIt++;
    QCOMPARE(*stlIt, referenceMap[50]);

    // const STL iterator
    referenceMap.clear();
    QMultiHash<int, QString>::const_iterator cstlIt;

    for (cstlIt = hash.cbegin(), i = 1; cstlIt != hash.cend(); ++cstlIt, ++i)
        referenceMap.insert(i, *cstlIt);

    cstlIt = hash.cbegin();
    QCOMPARE(*cstlIt, referenceMap[1]);

    for (i = 0; i < 5; ++i)
        cstlIt++;
    QCOMPARE(*cstlIt, referenceMap[6]);

    for (i = 0; i < 44; ++i)
        cstlIt++;
    QCOMPARE(*cstlIt, referenceMap[50]);

    // Java-Style iterator
    referenceMap.clear();
    QMultiHashIterator<int, QString> javaIt(hash);

    // walk through
    i = 0;
    while (javaIt.hasNext()) {
        ++i;
        javaIt.next();
        referenceMap.insert(i, javaIt.value());
    }
    javaIt.toFront();
    i = 0;
    while (javaIt.hasNext()) {
        ++i;
        javaIt.next();
        QCOMPARE(javaIt.value(), referenceMap.value(i));
    }

    // peekNext()
    javaIt.toFront();
    javaIt.next();
    QString nextValue;
    while (javaIt.hasNext()) {
        nextValue = javaIt.peekNext().value();
        javaIt.next();
        QCOMPARE(javaIt.value(), nextValue);
    }
}

template<typename T>
void iteratorsInEmptyHashTestMethod()
{
    T hash;
    using ConstIter = typename T::const_iterator;
    ConstIter it1 = hash.cbegin();
    ConstIter it2 = hash.constBegin();
    QVERIFY(it1 == it2 && it2 == ConstIter());
    QVERIFY(!hash.isDetached());

    ConstIter it3 = hash.cend();
    ConstIter it4 = hash.constEnd();
    QVERIFY(it3 == it4 && it4 == ConstIter());
    QVERIFY(!hash.isDetached());

    // to call const overloads of begin() and end()
    const T hash2;
    ConstIter it5 = hash2.begin();
    ConstIter it6 = hash2.end();
    QVERIFY(it5 == it6 && it6 == ConstIter());
    QVERIFY(!hash2.isDetached());

    T hash3;
    using Iter = typename T::iterator;
    Iter it7 = hash3.end();
    QVERIFY(it7 == Iter());
    QVERIFY(!hash3.isDetached());

    Iter it8 = hash3.begin();
    QVERIFY(it8 == Iter());
    QVERIFY(!hash3.isDetached()); // No detach from empty hash just for iteration!
}

void tst_QHash::iteratorsInEmptyHash()
{
    iteratorsInEmptyHashTestMethod<QHash<int, QString>>();
    if (QTest::currentTestFailed())
        return;

    iteratorsInEmptyHashTestMethod<QMultiHash<int, QString>>();
}

void tst_QHash::keyIterator()
{
    QHash<int, int> hash;

    using KeyIterator = QHash<int, int>::key_iterator;
    KeyIterator it1 = hash.keyBegin();
    KeyIterator it2 = hash.keyEnd();
    QVERIFY(it1 == it2 && it2 == KeyIterator());
    QVERIFY(!hash.isDetached());

    for (int i = 0; i < 100; ++i)
        hash.insert(i, i*100);

    KeyIterator key_it = hash.keyBegin();
    QHash<int, int>::const_iterator it = hash.cbegin();
    for (int i = 0; i < 100; ++i) {
        QCOMPARE(*key_it, it.key());
        key_it++;
        it++;
    }

    key_it = std::find(hash.keyBegin(), hash.keyEnd(), 50);
    it = std::find(hash.cbegin(), hash.cend(), 50 * 100);

    QVERIFY(key_it != hash.keyEnd());
    QCOMPARE(*key_it, it.key());
    QCOMPARE(*(key_it++), (it++).key());
    if (key_it != hash.keyEnd()) {
        QVERIFY(it != hash.cend());
        ++key_it;
        ++it;
        if (key_it != hash.keyEnd())
            QCOMPARE(*key_it, it.key());
        else
            QVERIFY(it == hash.cend());
    }

    QCOMPARE(std::count(hash.keyBegin(), hash.keyEnd(), 99), 1);

    // DefaultConstructible test
    static_assert(std::is_default_constructible<KeyIterator>::value);
}

void tst_QHash::multihashKeyIterator()
{
    QMultiHash<int, int> hash;

    using KeyIterator = QMultiHash<int, int>::key_iterator;
    KeyIterator it1 = hash.keyBegin();
    KeyIterator it2 = hash.keyEnd();
    QVERIFY(it1 == it2 && it2 == KeyIterator());
    QVERIFY(!hash.isDetached());

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 5; ++j)
            hash.insert(i, i * 100 + j);
    }

    KeyIterator keyIt = hash.keyBegin();
    QMultiHash<int, int>::const_iterator it = hash.cbegin();
    while (keyIt != hash.keyEnd() && it != hash.cend()) {
        QCOMPARE(*keyIt, it.key());
        keyIt++;
        it++;
    }

    keyIt = std::find(hash.keyBegin(), hash.keyEnd(), 5);
    it = std::find(hash.cbegin(), hash.cend(), 5 * 100 + 2);

    QVERIFY(keyIt != hash.keyEnd());
    QCOMPARE(*keyIt, it.key());

    QCOMPARE(std::count(hash.keyBegin(), hash.keyEnd(), 9), 5);

    // DefaultConstructible test
    static_assert(std::is_default_constructible<KeyIterator>::value);
}

void tst_QHash::keyValueIterator()
{
    QHash<int, int> hash;
    typedef QHash<int, int>::const_key_value_iterator::value_type entry_type;

    for (int i = 0; i < 100; ++i)
        hash.insert(i, i * 100);

    auto key_value_it = hash.constKeyValueBegin();
    auto it = hash.cbegin();


    for (int i = 0; i < hash.size(); ++i) {
        QVERIFY(key_value_it != hash.constKeyValueEnd());
        QVERIFY(it != hash.cend());

        entry_type pair(it.key(), it.value());
        QCOMPARE(*key_value_it, pair);
        QCOMPARE(key_value_it->first, pair.first);
        QCOMPARE(key_value_it->second, pair.second);
        QCOMPARE(&(*key_value_it).first, &it.key());
        QCOMPARE(&key_value_it->first,   &it.key());
        QCOMPARE(&(*key_value_it).second, &it.value());
        QCOMPARE(&key_value_it->second,   &it.value());
        ++key_value_it;
        ++it;
    }

    QVERIFY(key_value_it == hash.constKeyValueEnd());
    QVERIFY(it == hash.cend());

    int key = 50;
    int value = 50 * 100;
    entry_type pair(key, value);
    key_value_it = std::find(hash.constKeyValueBegin(), hash.constKeyValueEnd(), pair);
    it = std::find(hash.cbegin(), hash.cend(), value);

    QVERIFY(key_value_it != hash.constKeyValueEnd());
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));

    ++it;
    ++key_value_it;
    if (it != hash.cend())
        QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));
    else
        QVERIFY(key_value_it == hash.constKeyValueEnd());

    key = 99;
    value = 99 * 100;
    QCOMPARE(std::count(hash.constKeyValueBegin(), hash.constKeyValueEnd(), entry_type(key, value)), 1);
}

void tst_QHash::multihashKeyValueIterator()
{
    QMultiHash<int, int> hash;
    using EntryType = QHash<int, int>::const_key_value_iterator::value_type;

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 5; j++)
            hash.insert(i, i * 100 + j);
    }

    auto keyValueIt = hash.constKeyValueBegin();
    auto it = hash.cbegin();

    for (int i = 0; i < hash.size(); ++i) {
        QVERIFY(keyValueIt != hash.constKeyValueEnd());
        QVERIFY(it != hash.cend());

        EntryType pair(it.key(), it.value());
        QCOMPARE(*keyValueIt, pair);
        QCOMPARE(keyValueIt->first, pair.first);
        QCOMPARE(keyValueIt->second, pair.second);
        ++keyValueIt;
        ++it;
    }

    QVERIFY(keyValueIt == hash.constKeyValueEnd());
    QVERIFY(it == hash.cend());

    int key = 5;
    int value = key * 100 + 3;
    EntryType pair(key, value);
    keyValueIt = std::find(hash.constKeyValueBegin(), hash.constKeyValueEnd(), pair);
    it = std::find(hash.cbegin(), hash.cend(), value);

    QVERIFY(keyValueIt != hash.constKeyValueEnd());
    QCOMPARE(*keyValueIt, EntryType(it.key(), it.value()));

    key = 9;
    value = key * 100 + 4;
    const auto numItems =
            std::count(hash.constKeyValueBegin(), hash.constKeyValueEnd(), EntryType(key, value));
    QCOMPARE(numItems, 1);
}

template<typename T>
void keyValueIteratorInEmptyHashTestMethod()
{
    T hash;
    using ConstKeyValueIter = typename T::const_key_value_iterator;

    ConstKeyValueIter it1 = hash.constKeyValueBegin();
    ConstKeyValueIter it2 = hash.constKeyValueEnd();
    QVERIFY(it1 == it2 && it2 == ConstKeyValueIter());
    QVERIFY(!hash.isDetached());

    const T hash2;
    ConstKeyValueIter it3 = hash2.keyValueBegin();
    ConstKeyValueIter it4 = hash2.keyValueEnd();
    QVERIFY(it3 == it4 && it4 == ConstKeyValueIter());
    QVERIFY(!hash.isDetached());

    T hash3;
    using KeyValueIter = typename T::key_value_iterator;

    KeyValueIter it5 = hash3.keyValueEnd();
    QVERIFY(it5 == KeyValueIter());
    QVERIFY(!hash3.isDetached());

    KeyValueIter it6 = hash3.keyValueBegin();
    QVERIFY(it6 == KeyValueIter());
    QVERIFY(!hash3.isDetached()); // No detach in empty hash just for iteration
}

void tst_QHash::keyValueIteratorInEmptyHash()
{
    keyValueIteratorInEmptyHashTestMethod<QHash<int, int>>();
    if (QTest::currentTestFailed())
        return;

    keyValueIteratorInEmptyHashTestMethod<QMultiHash<int, int>>();
}

void tst_QHash::rehash_isnt_quadratic()
{
    // this test should be incredibly slow if rehash() is quadratic
    for (int j = 0; j < 5; ++j) {
        QHash<int, int> testHash;
        for (int i = 0; i < 500000; ++i)
            testHash.insert(i, 1);
    }
}

class Bar
{
public:
    Bar(int i) : j(i) {}

    int j;
};

void tst_QHash::dont_need_default_constructor()
{
    QHash<int, Bar> hash1;
    for (int i = 0; i < 100; ++i) {
        hash1.insert(i, Bar(2 * i));
        QVERIFY(hash1.value(i, Bar(-1)).j == 2 * i);
        QVERIFY(hash1.size() == i + 1);
    }

    QHash<QString, Bar> hash2;
    for (int i = 0; i < 100; ++i) {
        hash2.insert(QString::number(i), Bar(2 * i));
        QVERIFY(hash2.value(QString::number(i), Bar(-1)).j == 2 * i);
        QVERIFY(hash2.size() == i + 1);
    }

    QHash<int, Bar> hash3;
    for (int i = 0; i < 100; ++i) {
        hash3.tryEmplace(i, Bar(2 * i));
        QVERIFY(hash3.value(i, Bar(-1)).j == 2 * i);
        QVERIFY(hash3.size() == i + 1);
    }

    QHash<int, Bar> hash4;
    for (int i = 0; i < 100; ++i) {
        hash4.tryInsert(i, Bar(2 * i));
        QVERIFY(hash4.value(i, Bar(-1)).j == 2 * i);
        QVERIFY(hash4.size() == i + 1);
    }

    QHash<int, Bar> hash5;
    for (int i = 0; i < 100; ++i) {
        hash5.insertOrAssign(i, Bar(2 * i));
        QVERIFY(hash5.value(i, Bar(-1)).j == 2 * i);
        QVERIFY(hash5.size() == i + 1);
    }
}

void tst_QHash::qmultihash_specific()
{
    QMultiHash<int, int> hash1;

    QVERIFY(!hash1.contains(1));
    QVERIFY(!hash1.contains(1, 2));
    QVERIFY(!hash1.isDetached());

    for (int i = 1; i <= 9; ++i) {
        QVERIFY(!hash1.contains(i));
        for (int j = 1; j <= i; ++j) {
            int k = i * 10 + j;
            QVERIFY(!hash1.contains(i, k));
            hash1.insert(i, k);
            QVERIFY(hash1.contains(i, k));
        }
        QVERIFY(hash1.contains(i));
    }

    for (int i = 1; i <= 9; ++i) {
        QVERIFY(hash1.contains(i));
        for (int j = 1; j <= i; ++j) {
            int k = i * 10 + j;
            QVERIFY(hash1.contains(i, k));
        }
    }

    QVERIFY(hash1.contains(9, 99));
    QCOMPARE(hash1.size(), 45);
    hash1.remove(9, 99);
    QVERIFY(!hash1.contains(9, 99));
    QCOMPARE(hash1.size(), 44);

    hash1.remove(9, 99);
    QVERIFY(!hash1.contains(9, 99));
    QCOMPARE(hash1.size(), 44);

    hash1.remove(1, 99);
    QCOMPARE(hash1.size(), 44);

    hash1.insert(1, 99);
    hash1.insert(1, 99);

    QCOMPARE(hash1.size(), 46);
    hash1.remove(1, 99);
    QCOMPARE(hash1.size(), 44);
    hash1.remove(1, 99);
    QCOMPARE(hash1.size(), 44);

    {
    QMultiHash<int, int>::const_iterator i = hash1.constFind(1, 11);
    QVERIFY(i.key() == 1);
    QVERIFY(i.value() == 11);

    i = hash1.constFind(2, 22);
    QVERIFY(i.key() == 2);
    QVERIFY(i.value() == 22);

    i = hash1.constFind(9, 98);
    QVERIFY(i.key() == 9);
    QVERIFY(i.value() == 98);
    }

    {
    const QMultiHash<int, int> hash2(hash1);
    QMultiHash<int, int>::const_iterator i = hash2.find(1, 11);
    QVERIFY(i.key() == 1);
    QVERIFY(i.value() == 11);

    i = hash2.find(2, 22);
    QVERIFY(i.key() == 2);
    QVERIFY(i.value() == 22);

    i = hash2.find(9, 98);
    QVERIFY(i.key() == 9);
    QVERIFY(i.value() == 98);
    }

    {
    QMultiHash<int, int>::iterator i = hash1.find(1, 11);
    QVERIFY(i.key() == 1);
    QVERIFY(i.value() == 11);

    i = hash1.find(2, 22);
    QVERIFY(i.key() == 2);
    QVERIFY(i.value() == 22);

    i = hash1.find(9, 98);
    QVERIFY(i.key() == 9);
    QVERIFY(i.value() == 98);
    }

    QCOMPARE(hash1.count(9), 8);
    QCOMPARE(hash1.size(), 44);
    hash1.remove(9);
    QCOMPARE(hash1.count(9), 0);
    QCOMPARE(hash1.size(), 36);

    {
    QMultiHash<int, int> map1;
    map1.insert(42, 1);
    map1.insert(10, 2);
    map1.insert(48, 3);
    QMultiHash<int, int> map2;
    map2.insert(8, 4);
    map2.insert(42, 5);
    map2.insert(95, 12);

    map1+=map2;
    map2.insert(42, 1);
    map2.insert(10, 2);
    map2.insert(48, 3);
    QCOMPARE(map1.size(), map2.size());
    QVERIFY(map1.remove(42,5));
    QVERIFY(map1 != map2);
    QVERIFY(map2.remove(42,5));
    QVERIFY(map1 == map2);

    QHash<int, int> hash;
    hash.insert(-1, -1);
    map2.unite(hash);
    QCOMPARE(map2.size(), 6);
    QCOMPARE(map2[-1], -1);
    }
}

void tst_QHash::qmultihash_qhash_rvalue_ref_ctor()
{
    // QHash is empty
    {
    QHash<int, MyClass> hash;
    QMultiHash<int, MyClass> multiHash(std::move(hash));
    QVERIFY(multiHash.isEmpty());
    }

    // QHash is detached
    {
    MyClass::copies = 0;
    MyClass::moves = 0;
    QHash<int, MyClass> hash;
    hash.emplace(0, "a");
    hash.emplace(1, "b");
    QMultiHash<int, MyClass> multiHash(std::move(hash));
    QCOMPARE(multiHash.size(), 2);
    QCOMPARE(multiHash[0].str, QString("a"));
    QCOMPARE(multiHash[1].str, QString("b"));
    QCOMPARE(MyClass::copies, 0);
    QCOMPARE(MyClass::moves, 2);
    QCOMPARE(MyClass::count, 2);
    }

    // QHash is shared
    {
    MyClass::copies = 0;
    MyClass::moves = 0;
    QHash<int, MyClass> hash;
    hash.emplace(0, "a");
    hash.emplace(1, "b");
    QHash<int, MyClass> hash2(hash);
    QMultiHash<int, MyClass> multiHash(std::move(hash));
    QCOMPARE(multiHash.size(), 2);
    QCOMPARE(multiHash[0].str, QString("a"));
    QCOMPARE(multiHash[1].str, QString("b"));
    QCOMPARE(MyClass::copies, 2);
    QCOMPARE(MyClass::moves, 0);
    QCOMPARE(MyClass::count, 4);
    }
}

void tst_QHash::qmultihash_qhash_rvalue_ref_unite()
{
    // QHash is empty
    {
    QHash<int, MyClass> hash;
    QMultiHash<int, MyClass> multiHash;
    multiHash.unite(std::move(hash));
    QVERIFY(multiHash.isEmpty());
    }

    // QHash is detached
    {
    MyClass::copies = 0;
    MyClass::moves = 0;
    QHash<int, MyClass> hash;
    hash.emplace(0, "a");
    hash.emplace(1, "b");
    QMultiHash<int, MyClass> multiHash;
    multiHash.unite(std::move(hash));
    QCOMPARE(multiHash.size(), 2);
    QCOMPARE(multiHash[0].str, QString("a"));
    QCOMPARE(multiHash[1].str, QString("b"));
    QCOMPARE(MyClass::copies, 0);
    QCOMPARE(MyClass::moves, 2);
    QCOMPARE(MyClass::count, 2);
    }

    // QHash is shared
    {
    MyClass::copies = 0;
    MyClass::moves = 0;
    QHash<int, MyClass> hash;
    hash.emplace(0, "a");
    hash.emplace(1, "b");
    QHash<int, MyClass> hash2(hash);
    QMultiHash<int, MyClass> multiHash;
    multiHash.unite(std::move(hash));
    QCOMPARE(multiHash.size(), 2);
    QCOMPARE(multiHash[0].str, QString("a"));
    QCOMPARE(multiHash[1].str, QString("b"));
    QCOMPARE(MyClass::copies, 2);
    QCOMPARE(MyClass::moves, 0);
    QCOMPARE(MyClass::count, 4);
    }

    // QMultiHash already contains an item with the same key
    {
    MyClass::copies = 0;
    MyClass::moves = 0;
    QHash<int, MyClass> hash;
    hash.emplace(0, "a");
    hash.emplace(1, "b");
    QMultiHash<int, MyClass> multiHash;
    multiHash.emplace(0, "c");
    multiHash.unite(std::move(hash));
    QCOMPARE(multiHash.size(), 3);
    const auto aRange = multiHash.equal_range(0);
    QCOMPARE(std::distance(aRange.first, aRange.second), 2);
    auto it = aRange.first;
    QCOMPARE(it->str, QString("a"));
    QCOMPARE((++it)->str, QString("c"));
    QCOMPARE(multiHash[1].str, QString("b"));
    QCOMPARE(MyClass::copies, 0);
    QCOMPARE(MyClass::moves, 2);
    QCOMPARE(MyClass::count, 3);
    }
}

void tst_QHash::qmultihashUnite()
{
    // Joining two multi hashes, first is empty
    {
        MyClass::copies = 0;
        MyClass::moves = 0;
        QMultiHash<int, MyClass> hash1;
        QMultiHash<int, MyClass> hash2;
        hash2.emplace(0, "a");
        hash2.emplace(1, "b");

        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);

        hash1.unite(hash2);
        // hash1 is empty, so we just share the data between hash1 and hash2
        QCOMPARE(hash1.size(), 2);
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);
    }
    // Joining two multi hashes, second is empty
    {
        MyClass::copies = 0;
        MyClass::moves = 0;
        QMultiHash<int, MyClass> hash1;
        QMultiHash<int, MyClass> hash2;
        hash1.emplace(0, "a");
        hash1.emplace(1, "b");

        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);

        hash1.unite(hash2);
        // hash2 is empty, so nothing happens
        QVERIFY(hash2.isEmpty());
        QVERIFY(!hash2.isDetached());
        QCOMPARE(hash1.size(), 2);
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);
    }
    // Joining two multi hashes
    {
        MyClass::copies = 0;
        MyClass::moves = 0;
        QMultiHash<int, MyClass> hash1;
        QMultiHash<int, MyClass> hash2;
        hash1.emplace(0, "a");
        hash1.emplace(1, "b");
        hash2.emplace(0, "c");
        hash2.emplace(1, "d");

        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 4);

        hash1.unite(hash2);
        QCOMPARE(hash1.size(), 4);
        QCOMPARE(MyClass::copies, 2);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 6);
    }

    // operator+() uses unite() internally.

    // using operator+(), hash1 is empty
    {
        MyClass::copies = 0;
        MyClass::moves = 0;
        QMultiHash<int, MyClass> hash1;
        QMultiHash<int, MyClass> hash2;
        hash2.emplace(0, "a");
        hash2.emplace(1, "b");

        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);

        auto hash3 = hash1 + hash2;
        // hash1 is empty, so we just share the data between hash3 and hash2
        QCOMPARE(hash1.size(), 0);
        QCOMPARE(hash2.size(), 2);
        QCOMPARE(hash3.size(), 2);
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);
    }
    // using operator+(), hash2 is empty
    {
        MyClass::copies = 0;
        MyClass::moves = 0;
        QMultiHash<int, MyClass> hash1;
        QMultiHash<int, MyClass> hash2;
        hash1.emplace(0, "a");
        hash1.emplace(1, "b");

        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);

        auto hash3 = hash1 + hash2;
        // hash2 is empty, so we just share the data between hash3 and hash1
        QCOMPARE(hash1.size(), 2);
        QCOMPARE(hash2.size(), 0);
        QCOMPARE(hash3.size(), 2);
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 2);
    }
    // using operator+()
    {
        MyClass::copies = 0;
        MyClass::moves = 0;
        QMultiHash<int, MyClass> hash1;
        QMultiHash<int, MyClass> hash2;
        hash1.emplace(0, "a");
        hash1.emplace(1, "b");
        hash2.emplace(0, "c");
        hash2.emplace(1, "d");

        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 4);

        auto hash3 = hash1 + hash2;
        QCOMPARE(hash1.size(), 2);
        QCOMPARE(hash2.size(), 2);
        QCOMPARE(hash3.size(), 4);
        QCOMPARE(MyClass::copies, 4);
        QCOMPARE(MyClass::moves, 0);
        QCOMPARE(MyClass::count, 8);
    }
}

void tst_QHash::qmultihashSize()
{
    // QMultiHash has an extra m_size member that counts the number of values,
    // while d->size (shared with QHash) counts the number of distinct keys.
    {
        QMultiHash<int, int> hash;
        QCOMPARE(hash.size(), 0);
        QVERIFY(hash.isEmpty());

        hash.insert(0, 42);
        QCOMPARE(hash.size(), 1);
        QVERIFY(!hash.isEmpty());

        hash.insert(0, 42);
        QCOMPARE(hash.size(), 2);
        QVERIFY(!hash.isEmpty());

        hash.emplace(0, 42);
        QCOMPARE(hash.size(), 3);
        QVERIFY(!hash.isEmpty());

        QCOMPARE(hash.take(0), 42);
        QCOMPARE(hash.size(), 2);
        QVERIFY(!hash.isEmpty());

        QCOMPARE(hash.remove(0), 2);
        QCOMPARE(hash.size(), 0);
        QVERIFY(hash.isEmpty());
    }

    {
        QMultiHash<int, int> hash;
        hash.emplace(0, 0);
        hash.emplace(0, 0);
        QCOMPARE(hash.size(), 2);
        QVERIFY(!hash.isEmpty());

        hash.emplace(0, 1);
        QCOMPARE(hash.size(), 3);
        QVERIFY(!hash.isEmpty());

        QCOMPARE(hash.remove(0, 0), 2);
        QCOMPARE(hash.size(), 1);
        QVERIFY(!hash.isEmpty());

        hash.remove(0);
        QCOMPARE(hash.size(), 0);
        QVERIFY(hash.isEmpty());
    }

    {
        QMultiHash<int, int> hash;

        hash[0] = 0;
        QCOMPARE(hash.size(), 1);
        QVERIFY(!hash.isEmpty());

        hash.replace(0, 1);
        QCOMPARE(hash.size(), 1);
        QVERIFY(!hash.isEmpty());

        hash.insert(0, 1);
        hash.erase(hash.cbegin());
        QCOMPARE(hash.size(), 1);
        QVERIFY(!hash.isEmpty());

        hash.erase(hash.cbegin());
        QCOMPARE(hash.size(), 0);
        QVERIFY(hash.isEmpty());
    }
}

void tst_QHash::qmultihashHeterogeneousSearch()
{
    heterogeneousSearchTest<QMultiHash, QString, HeterogeneousHashingType>({ "Hello", {}, "World" });
}

void tst_QHash::qmultihashHeterogeneousSearchConstKey()
{
    heterogeneousSearchTest<QMultiHash, const QString, HeterogeneousHashingType>({ "Hello", {}, "World" });
}

void tst_QHash::qmultihashHeterogeneousSearchByteArray()
{
    heterogeneousSearchTest<QMultiHash, QByteArray, QByteArrayView>({ "Hello", {}, "World" });
}

void tst_QHash::qmultihashHeterogeneousSearchString()
{
    heterogeneousSearchTest<QMultiHash, QString, QStringView>({ "Hello", {}, "World" });
}

void tst_QHash::qmultihashHeterogeneousSearchLatin1String()
{
    ::heterogeneousSearchLatin1String<QMultiHash>(QHashHeterogeneousSearch<QString, QLatin1StringView>{});
}

void tst_QHash::keys_values_uniqueKeys()
{
    QMultiHash<QString, int> hash;
    QVERIFY(hash.uniqueKeys().isEmpty());
    QVERIFY(hash.keys().isEmpty());
    QVERIFY(hash.values().isEmpty());
    QVERIFY(!hash.isDetached());

    hash.insert("alpha", 1);
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha"));
    QVERIFY(hash.keys() == hash.uniqueKeys());
    QVERIFY(hash.values() == (QList<int>() << 1));

    hash.insert("beta", -2);
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(hash.keys() == hash.uniqueKeys());
    QVERIFY(sorted(hash.values()) == sorted(QList<int>() << 1 << -2));

    hash.insert("alpha", 2);
    QVERIFY(sorted(hash.uniqueKeys()) == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha" << "alpha" << "beta"));
    QVERIFY(sorted(hash.values()) == sorted(QList<int>() << 2 << 1 << -2));

    hash.insert("beta", 4);
    QVERIFY(sorted(hash.uniqueKeys()) == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha" << "alpha" << "beta" << "beta"));
    QVERIFY(sorted(hash.values()) == sorted(QList<int>() << 2 << 1 << 4 << -2));
}

void tst_QHash::const_shared_null()
{
    QHash<int, QString> hash2;
    QVERIFY(!hash2.isDetached());
}

// This gets set to != 0 in wrong qHash overloads
static int wrongqHashOverload = 0;

struct OneArgumentQHashStruct1 {};
bool operator==(const OneArgumentQHashStruct1 &, const OneArgumentQHashStruct1 &) { return false; }
size_t qHash(OneArgumentQHashStruct1) { return 0; }

struct OneArgumentQHashStruct2 {};
bool operator==(const OneArgumentQHashStruct2 &, const OneArgumentQHashStruct2 &) { return false; }
size_t qHash(const OneArgumentQHashStruct2 &) { return 0; }

struct OneArgumentQHashStruct3 {};
bool operator==(const OneArgumentQHashStruct3 &, const OneArgumentQHashStruct3 &) { return false; }
size_t qHash(OneArgumentQHashStruct3) { return 0; }
size_t qHash(OneArgumentQHashStruct3 &, size_t) { wrongqHashOverload = 1; return 0; }

struct OneArgumentQHashStruct4 {};
bool operator==(const OneArgumentQHashStruct4 &, const OneArgumentQHashStruct4 &) { return false; }
size_t qHash(const OneArgumentQHashStruct4 &) { return 0; }
size_t qHash(OneArgumentQHashStruct4 &, size_t) { wrongqHashOverload = 1; return 0; }


struct TwoArgumentsQHashStruct1 {};
bool operator==(const TwoArgumentsQHashStruct1 &, const TwoArgumentsQHashStruct1 &) { return false; }
size_t qHash(const TwoArgumentsQHashStruct1 &) { wrongqHashOverload = 1; return 0; }
size_t qHash(const TwoArgumentsQHashStruct1 &, size_t) { return 0; }

struct TwoArgumentsQHashStruct2 {};
bool operator==(const TwoArgumentsQHashStruct2 &, const TwoArgumentsQHashStruct2 &) { return false; }
size_t qHash(TwoArgumentsQHashStruct2) { wrongqHashOverload = 1; return 0; }
size_t qHash(const TwoArgumentsQHashStruct2 &, size_t) { return 0; }

struct TwoArgumentsQHashStruct3 {};
bool operator==(const TwoArgumentsQHashStruct3 &, const TwoArgumentsQHashStruct3 &) { return false; }
size_t qHash(const TwoArgumentsQHashStruct3 &) { wrongqHashOverload = 1; return 0; }
size_t qHash(TwoArgumentsQHashStruct3, size_t) { return 0; }

struct TwoArgumentsQHashStruct4 {};
bool operator==(const TwoArgumentsQHashStruct4 &, const TwoArgumentsQHashStruct4 &) { return false; }
size_t qHash(TwoArgumentsQHashStruct4) { wrongqHashOverload = 1; return 0; }
size_t qHash(TwoArgumentsQHashStruct4, size_t) { return 0; }

/*!
    \internal

    Check that QHash picks up the right overload.
    The best one, for a type T, is the two-args version of qHash:
    either size_t qHash(T, size_t) or size_t qHash(const T &, size_t).

    If neither of these exists, then one between
    size_t qHash(T) or size_t qHash(const T &) must exist
    (and it gets selected instead).
*/
void tst_QHash::twoArguments_qHash()
{
    QHash<OneArgumentQHashStruct1, int> oneArgHash1;
    OneArgumentQHashStruct1 oneArgObject1;
    oneArgHash1[oneArgObject1] = 1;
    QCOMPARE(wrongqHashOverload, 0);

    QHash<OneArgumentQHashStruct2, int> oneArgHash2;
    OneArgumentQHashStruct2 oneArgObject2;
    oneArgHash2[oneArgObject2] = 1;
    QCOMPARE(wrongqHashOverload, 0);

    QHash<OneArgumentQHashStruct3, int> oneArgHash3;
    OneArgumentQHashStruct3 oneArgObject3;
    oneArgHash3[oneArgObject3] = 1;
    QCOMPARE(wrongqHashOverload, 0);

    QHash<OneArgumentQHashStruct4, int> oneArgHash4;
    OneArgumentQHashStruct4 oneArgObject4;
    oneArgHash4[oneArgObject4] = 1;
    QCOMPARE(wrongqHashOverload, 0);

    QHash<TwoArgumentsQHashStruct1, int> twoArgsHash1;
    TwoArgumentsQHashStruct1 twoArgsObject1;
    twoArgsHash1[twoArgsObject1] = 1;
    QCOMPARE(wrongqHashOverload, 0);

    QHash<TwoArgumentsQHashStruct2, int> twoArgsHash2;
    TwoArgumentsQHashStruct2 twoArgsObject2;
    twoArgsHash2[twoArgsObject2] = 1;
    QCOMPARE(wrongqHashOverload, 0);

    QHash<TwoArgumentsQHashStruct3, int> twoArgsHash3;
    TwoArgumentsQHashStruct3 twoArgsObject3;
    twoArgsHash3[twoArgsObject3] = 1;
    QCOMPARE(wrongqHashOverload, 0);

    QHash<TwoArgumentsQHashStruct4, int> twoArgsHash4;
    TwoArgumentsQHashStruct4 twoArgsObject4;
    twoArgsHash4[twoArgsObject4] = 1;
    QCOMPARE(wrongqHashOverload, 0);
}

void tst_QHash::initializerList()
{
    QHash<int, QString> hash = {{1, "bar"}, {1, "hello"}, {2, "initializer_list"}};
    QCOMPARE(hash.size(), 2);
    QCOMPARE(hash[1], QString("hello"));
    QCOMPARE(hash[2], QString("initializer_list"));

    // note the difference to std::unordered_map:
    // std::unordered_map<int, QString> stdh = {{1, "bar"}, {1, "hello"}, {2, "initializer_list"}};
    // QCOMPARE(stdh.size(), 2UL);
    // QCOMPARE(stdh[1], QString("bar"));

    QMultiHash<QString, int> multiHash{{"il", 1}, {"il", 2}, {"il", 3}};
    QCOMPARE(multiHash.size(), 3);
    QList<int> values = multiHash.values("il");
    QCOMPARE(values.size(), 3);

    QHash<int, int> emptyHash{};
    QVERIFY(emptyHash.isEmpty());

    QHash<int, char> emptyPairs{{}, {}};
    QVERIFY(!emptyPairs.isEmpty());

    QMultiHash<QString, double> emptyMultiHash{};
    QVERIFY(emptyMultiHash.isEmpty());

    QMultiHash<int, float> emptyPairs2{{}, {}};
    QVERIFY(!emptyPairs2.isEmpty());
}

void tst_QHash::eraseValidIteratorOnSharedHash()
{
    QMultiHash<int, int> a, b;
    a.insert(10, 10);
    a.insert(10, 25);
    a.insert(10, 30);
    a.insert(20, 20);
    a.insert(40, 40);

    auto i = a.begin();
    while (i.value() != 25)
        ++i;

    b = a;
    a.erase(i);

    QCOMPARE(b.size(), 5);
    QCOMPARE(a.size(), 4);

    for (i = a.begin(); i != a.end(); ++i)
        QVERIFY(i.value() != 25);

    int itemsWith10 = 0;
    for (i = b.begin(); i != b.end(); ++i)
        itemsWith10 += (i.key() == 10);

    QCOMPARE(itemsWith10, 3);
}

void tst_QHash::equal_range()
{
    QMultiHash<int, QString> hash;

    auto result = hash.equal_range(0);
    QCOMPARE(result.first, hash.end());
    QCOMPARE(result.second, hash.end());

    hash.insert(1, "one");

    result = hash.equal_range(1);

    QCOMPARE(result.first, hash.find(1));
    QVERIFY(std::distance(result.first, result.second) == 1);

    QMultiHash<int, int> h1;
    {
        auto p = h1.equal_range(0);
        QVERIFY(p.first == p.second);
        QVERIFY(p.first == h1.end());
    }

    h1.insert(1, 2);
    {
        auto p1 = h1.equal_range(9);
        QVERIFY(p1.first == p1.second);
        QVERIFY(p1.first == h1.end());
    }
    {
        auto p2 = h1.equal_range(1);
        QVERIFY(p2.first != p2.second);
        QVERIFY(p2.first == h1.begin());
        QVERIFY(p2.second == h1.end());
    }

    QMultiHash<int, int> m1 = h1;
    m1.insert(1, 0);
    QCOMPARE(m1.size(), 2);
    {
        auto p1 = m1.equal_range(9);
        QVERIFY(p1.first == p1.second);
        QVERIFY(p1.first == m1.end());
    }
    {
        auto p2 = m1.equal_range(1);
        QVERIFY(p2.first != p2.second);
        QVERIFY(p2.first == m1.begin());
        QVERIFY(p2.second == m1.end());
        QCOMPARE(std::distance(p2.first, p2.second), 2);
    }

    m1.insert(0, 0);
    QCOMPARE(m1.size(), 3);
    {
        auto p1 = m1.equal_range(9);
        QVERIFY(p1.first == p1.second);
        QVERIFY(p1.first == m1.end());
    }
    {
        const auto p2 = m1.equal_range(1);
        QVERIFY(p2.first != p2.second);
        QCOMPARE(p2.first.key(), 1);
        QCOMPARE(std::distance(p2.first, p2.second), 2);
        QVERIFY(p2.first == m1.begin() || p2.second == m1.end());
    }

    const QMultiHash<int, int> ch1 = h1;
    {
        auto p1 = ch1.equal_range(9);
        QVERIFY(p1.first == p1.second);
        QVERIFY(p1.first == ch1.end());
    }
    {
        auto p2 = ch1.equal_range(1);
        QVERIFY(p2.first != p2.second);
        QVERIFY(p2.first == ch1.begin());
        QVERIFY(p2.second == ch1.end());
    }

    const QMultiHash<int, int> cm1 = m1;
    {
        auto p1 = cm1.equal_range(9);
        QVERIFY(p1.first == p1.second);
        QVERIFY(p1.first == cm1.end());
    }
    {
        auto p2 = cm1.equal_range(1);
        QVERIFY(p2.first != p2.second);
        QCOMPARE(std::distance(p2.first, p2.second), 2);
        QVERIFY(p2.first == cm1.cbegin() || p2.second == cm1.cend());
    }

    {
        const QMultiHash<int, int> cm2;
        auto p1 = cm2.equal_range(0);
        QVERIFY(p1.first == cm2.end());
        QVERIFY(p1.second == cm2.end());
    }

    QMultiHash<int, int> h2;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            h2.insert(i, i*j);

    for (int i = 0; i < 8; ++i) {
        auto pair = h2.equal_range(i);
        std::vector<int> vec(pair.first, pair.second);
        std::sort(vec.begin(), vec.end());
        for (int j = 0; j < 8; ++j)
            QCOMPARE(i*j, vec[j]);
    }
}

void tst_QHash::insert_hash()
{
    {
        QHash<int, int> hash;
        hash.insert(1, 1);
        hash.insert(2, 2);
        hash.insert(0, -1);

        QHash<int, int> hash2;
        hash2.insert(0, 0);
        hash2.insert(3, 3);
        hash2.insert(4, 4);

        hash.insert(hash2);

        QCOMPARE(hash.size(), 5);
        for (int i = 0; i < 5; ++i)
            QCOMPARE(hash[i], i);
    }
    {
        QHash<int, int> hash;
        hash.insert(0, 5);

        QHash<int, int> hash2;

        hash.insert(hash2);

        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 5);
    }
    {
        QHash<int, int> hash;
        QHash<int, int> hash2;
        hash2.insert(0, 5);

        hash.insert(hash2);

        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 5);
        QCOMPARE(hash, hash2);
    }
    {
        QHash<int, int> hash;
        hash.insert(0, 7);
        hash.insert(2, 5);
        hash.insert(7, 55);

        // insert into ourself, nothing should happen
        hash.insert(hash);

        QCOMPARE(hash.size(), 3);
        QCOMPARE(hash[0], 7);
        QCOMPARE(hash[2], 5);
        QCOMPARE(hash[7], 55);
    }
}

void tst_QHash::multiHashStoresInReverseInsertionOrder()
{
    const QString strings[] = {
        u"zero"_s,
        u"null"_s,
        u"nada"_s,
    };
    {
        QMultiHash<int, QString> hash;
        for (const QString &string : strings)
            hash.insert(0, string);
        auto printOnFailure = qScopeGuard([&] { qDebug() << hash; });
        QVERIFY(std::equal(hash.begin(), hash.end(),
                           std::rbegin(strings), std::rend(strings)));
        printOnFailure.dismiss();
    }
}

void tst_QHash::emplace()
{
    {
        QHash<QString, MyClass> hash;
        MyClass::copies = 0;
        MyClass::moves = 0;

        hash.emplace(QString("a"), QString("a"));
        QCOMPARE(hash["a"].str, "a");
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        hash.emplace(QString("ab"), QString("ab"));
        QCOMPARE(hash["ab"].str, "ab");
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        hash.emplace(QString("ab"), QString("abc"));
        QCOMPARE(hash["ab"].str, "abc");
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 1);
    }
    {
        QMultiHash<QString, MyClass> hash;
        MyClass::copies = 0;
        MyClass::moves = 0;

        hash.emplace(QString("a"), QString("a"));
        QCOMPARE(hash["a"].str, "a");
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        hash.emplace(QString("ab"), QString("ab"));
        QCOMPARE(hash["ab"].str, "ab");
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        hash.emplace(QString("ab"), QString("abc"));
        QCOMPARE(hash["ab"].str, "abc");
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 0);
        hash.emplaceReplace(QString("ab"), QString("abcd"));
        QCOMPARE(hash["ab"].str, "abcd");
        QCOMPARE(MyClass::copies, 0);
        QCOMPARE(MyClass::moves, 1);
    }
}

void tst_QHash::tryEmplace()
{
    {
        QHash<int, int> hash;
        QHash<int, int>::TryEmplaceResult r = hash.tryEmplace(0, 0);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 0);
        QVERIFY(r.inserted);
        QCOMPARE(*r.iterator, 0);
        int cref = 0;
        r = hash.tryEmplace(cref, 1);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 0);
        QVERIFY(!r.inserted);
        QCOMPARE(*r.iterator, 0);

        auto [iterator, inserted] = hash.tryEmplace(1, 1);
        QCOMPARE_NE(iterator, hash.end());
        QCOMPARE(hash.size(), 2);
        QCOMPARE(hash[1], 1);
        QVERIFY(inserted);
        QCOMPARE(*iterator, 1);

        auto [it, inserted2] = hash.try_emplace(cref, 1);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(hash.size(), 2);
        QCOMPARE(hash[0], 0);
        QVERIFY(!inserted2);
        QCOMPARE(it->second, 0);

        using KVIterator = decltype(hash)::key_value_iterator;
        using PairReturn = std::pair<KVIterator, bool>; // Return-value of try_emplace

        // Handle conversion _from_ the try_emplace return type.
        QHash<int, int>::TryEmplaceResult t = hash.try_emplace(2, 2);
        QCOMPARE(t.inserted, true);
        QCOMPARE(t.iterator.value(), 2);
        QCOMPARE(hash.size(), 3);

        t = hash.try_emplace(2, -1);
        QCOMPARE(t.inserted, false);
        QCOMPARE(t.iterator.value(), 2);
        QCOMPARE(hash.size(), 3);

        // Handle conversion _to_ the try_emplace return type.
        PairReturn p = hash.tryEmplace(1, -1);
        QCOMPARE_NE(p.first, hash.keyValueEnd());
        QCOMPARE(hash.size(), 3);

        QVERIFY(!p.second);
        QCOMPARE(p.first->second, 1);

        p = hash.try_emplace(cref, -1);
        QCOMPARE_NE(p.first, hash.keyValueEnd());
        QCOMPARE(hash.size(), 3);
        QVERIFY(!p.second);
        QCOMPARE(p.first->second, 0);
    }
    {
        // Make sure any kind of resize is properly handled
        QHash<int, int> hash;
        hash.reserve(1);
        const qsizetype end = hash.capacity() + 2;
        auto [it, inserted] = hash.try_emplace(0, 0);
        for (int i = 1; i < end; ++i) {
            QHash<int, int> copy = hash;

            // Test pure detach, no insert, works on any capacity:
            std::tie(it, inserted) = hash.try_emplace(i - 1, i);
            QCOMPARE_NE(it, hash.keyValueEnd());
            QVERIFY(!inserted);
            QCOMPARE(it->second, i - 1);
            QVERIFY(!hash.isSharedWith(copy));

            copy = hash;
            // And when an insertion _does_ happen:
            std::tie(it, inserted) = hash.try_emplace(i, i);
            QVERIFY(inserted);
            QCOMPARE(it->second, i);
        }
        QCOMPARE(hash.size(), end);
        QCOMPARE_GT(hash.capacity(), end);
    }
    {
        // Heterogeneous key
        QHash<QString, int> hash;
        auto [it, inserted] = hash.try_emplace(HeterogeneousHashingType{u"Hello World"_s}, 242);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QVERIFY(inserted);
        QCOMPARE(it->second, 242);
        QCOMPARE(hash.size(), 1);
        QCOMPARE(it->first, u"Hello World"_s);

        auto r = hash.tryEmplace(HeterogeneousHashingType{u"Uniquely"_s}, 1);
        QCOMPARE_NE(r.iterator, hash.end());
        QVERIFY(r.inserted);
        QCOMPARE(*r.iterator, 1);
        QCOMPARE(hash.size(), 2);
        QCOMPARE(r.iterator.key(), u"Uniquely"_s);
    }
    // Overloads taking hint:
    {
        QHash<QString, int> hash;
        auto it = hash.try_emplace(hash.end(), HeterogeneousHashingType{u"Hello World"_s}, 242);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(it->second, 242);
        QCOMPARE(hash.size(), 1);
        QCOMPARE(it->first, u"Hello World"_s);

        it = hash.try_emplace(hash.begin(), u"Uniquely"_s, 1);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(it->second, 1);
        QCOMPARE(hash.size(), 2);
        QCOMPARE(it->first, u"Uniquely"_s);

        QString cref = u"Uniquely"_s;
        it = hash.try_emplace(hash.end(), cref, 16);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(it->second, 1);
        QCOMPARE(hash.size(), 2);
        QCOMPARE(it->first, u"Uniquely"_s);
    }
    // tryInsert (it just calls tryEmplace)
    {
        QHash<int, int> hash;
        QHash<int, int>::TryEmplaceResult r = hash.tryInsert(0, 0);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 0);
        QVERIFY(r.inserted);
        QCOMPARE(*r.iterator, 0);
        r = hash.tryInsert(0, 1);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 0);
        QVERIFY(!r.inserted);
        QCOMPARE(*r.iterator, 0);
    }
}

void tst_QHash::insertOrAssign()
{
    {
        QHash<int, int> hash;
        QHash<int, int>::TryEmplaceResult r = hash.insertOrAssign(0, 0);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 0);
        QVERIFY(r.inserted);
        QCOMPARE(*r.iterator, 0);

        int cref = 0;
        r = hash.insertOrAssign(cref, 1);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 1);
        QVERIFY(!r.inserted);
        QCOMPARE(*r.iterator, 1);
    }
    // insert_or_assign (stdlib compat)
    {
        QHash<int, int> hash;
        QHash<int, int>::TryEmplaceResult r = hash.insert_or_assign(0, 0);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 0);
        QVERIFY(r.inserted);
        QCOMPARE(*r.iterator, 0);

        int cref = 0;
        r = hash.insert_or_assign(cref, 1);
        QCOMPARE_NE(r.iterator, hash.end());
        QCOMPARE(hash.size(), 1);
        QCOMPARE(hash[0], 1);
        QVERIFY(!r.inserted);
        QCOMPARE(*r.iterator, 1);

        auto [it, inserted] = hash.insert_or_assign(1, 1);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(hash.size(), 2);
        QCOMPARE(hash[1], 1);
        QVERIFY(inserted);
        QCOMPARE(it->second, 1);

        cref = 1;
        std::tie(it, inserted) = hash.insert_or_assign(cref, -1);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(hash.size(), 2);
        QCOMPARE(hash[1], -1);
        QVERIFY(!inserted);
        QCOMPARE(it->second, -1);

        // And with iterator hint:
        it = hash.insert_or_assign(hash.end(), 0, -1);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(hash.size(), 2);
        QCOMPARE(hash[1], -1);
        QVERIFY(!inserted);
        QCOMPARE(it->second, -1);

        it = hash.insert_or_assign(hash.end(), cref, -2);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QCOMPARE(hash.size(), 2);
        QCOMPARE(hash[1], -2);
        QVERIFY(!inserted);
        QCOMPARE(it->second, -2);
    }
    {
        // Heterogenous key
        QHash<QString, int> hash;
        auto r = hash.insertOrAssign(HeterogeneousHashingType{u"Hello World"_s}, 242);
        QCOMPARE_NE(r.iterator, hash.end());
        QVERIFY(r.inserted);
        QCOMPARE(r.iterator.value(), 242);
        QCOMPARE(hash.size(), 1);
        QCOMPARE(r.iterator.key(), u"Hello World"_s);

        HeterogeneousHashingType ctor{u"Uniquely"_s};
        auto [it, inserted] = hash.insert_or_assign(ctor, 1);
        QCOMPARE_NE(it, hash.keyValueEnd());
        QVERIFY(inserted);
        QCOMPARE(it->second, 1);
        QCOMPARE(hash.size(), 2);
        QCOMPARE(it->first, u"Uniquely"_s);
    }
}

struct BadKey {
    int k;
    BadKey(int i) : k(i) {}
    bool operator==(const BadKey &other) const
    {
        return k == other.k;
    }
};

size_t qHash(BadKey, size_t seed)
{
    return seed;
}

void tst_QHash::badHashFunction()
{
    QHash<BadKey, int> hash;
    for (int i = 0; i < 10000; ++i)
        hash.insert(i, i);

    for (int i = 0; i < 10000; ++i)
        QCOMPARE(hash.value(i), i);

    for (int i = 10000; i < 20000; ++i)
        QVERIFY(!hash.contains(i));

}

void tst_QHash::hashOfHash()
{
    QHash<int, int> hash;
    (void)qHash(hash);

    QMultiHash<int, int> multiHash;
    (void)qHash(multiHash);
}

template <bool HasQHash_>
struct StdHashKeyType {
    static inline constexpr bool HasQHash = HasQHash_;
    static bool StdHashUsed;

    int i;
    friend bool operator==(const StdHashKeyType &lhs, const StdHashKeyType &rhs)
    { return lhs.i == rhs.i; }
};

template <bool HasQHash>
bool StdHashKeyType<HasQHash>::StdHashUsed = false;

namespace std {
template <bool HasQHash> struct hash<StdHashKeyType<HasQHash>>
{
    size_t operator()(const StdHashKeyType<HasQHash> &s, size_t seed = 0) const {
        StdHashKeyType<HasQHash>::StdHashUsed = true;
        return hash<int>()(s.i) ^ seed;
    }
};
}

template <bool HasQHash>
std::enable_if_t<HasQHash, size_t>
qHash(const StdHashKeyType<HasQHash> &s, size_t seed)
{
    return qHash(s.i, seed);
}

template <typename T>
void stdHashImpl()
{
    QHash<T, int> hash;
    for (int i = 0; i < 1000; ++i)
        hash.insert(T{i}, i);

    QCOMPARE(hash.size(), 1000);
    for (int i = 0; i < 1000; ++i)
        QCOMPARE(hash.value(T{i}, -1), i);

    for (int i = 500; i < 1500; ++i)
        hash.insert(T{i}, i);

    QCOMPARE(hash.size(), 1500);
    for (int i = 0; i < 1500; ++i)
        QCOMPARE(hash.value(T{i}, -1), i);

    qsizetype count = 0;
    for (int i = -2000; i < 2000; ++i) {
        if (hash.contains(T{i}))
            ++count;
    }
    QCOMPARE(count, 1500);
    QCOMPARE(T::StdHashUsed, !T::HasQHash);


    std::unordered_set<T> set;
    for (int i = 0; i < 1000; ++i)
        set.insert(T{i});

    for (int i = 500; i < 1500; ++i)
        set.insert(T{i});

    QCOMPARE(set.size(), size_t(1500));
    count = 0;
    for (int i = -2000; i < 2000; ++i)
        count += qsizetype(set.count(T{i}));
    QCOMPARE(count, 1500);
    QVERIFY(T::StdHashUsed);
}

void tst_QHash::stdHash()
{
    stdHashImpl<StdHashKeyType<false>>();
    stdHashImpl<StdHashKeyType<true>>();

    QSet<std::string> strings{ "a", "b", "c" };
    QVERIFY(strings.contains("a"));
    QVERIFY(!strings.contains("z"));
}

void tst_QHash::countInEmptyHash()
{
    {
        QHash<int, int> hash;
        QCOMPARE(hash.size(), 0);
        QCOMPARE(hash.count(42), 0);
    }

    {
        QMultiHash<int, int> hash;
        QCOMPARE(hash.size(), 0);
        QCOMPARE(hash.count(42), 0);
        QCOMPARE(hash.count(42, 1), 0);
    }
}

void tst_QHash::removeInEmptyHash()
{
    {
        QHash<QString, int> hash;
        QCOMPARE(hash.remove("test"), false);
        QVERIFY(!hash.isDetached());

        using Iter = QHash<QString, int>::iterator;
        const auto removed = hash.removeIf([](Iter) { return true; });
        QCOMPARE(removed, 0);
    }
    {
        QMultiHash<QString, int> hash;
        QCOMPARE(hash.remove("key"), 0);
        QCOMPARE(hash.remove("key", 1), 0);
        QVERIFY(!hash.isDetached());

        using Iter = QMultiHash<QString, int>::iterator;
        const auto removed = hash.removeIf([](Iter) { return true; });
        QCOMPARE(removed, 0);
    }
}

template<typename T>
void valueInEmptyHashTestFunction()
{
    T hash;
    QCOMPARE(hash.value("key"), 0);
    QCOMPARE(hash.value("key", -1), -1);
    QVERIFY(hash.values().isEmpty());
    QVERIFY(!hash.isDetached());

    const T constHash;
    QCOMPARE(constHash["key"], 0);
}

void tst_QHash::valueInEmptyHash()
{
    valueInEmptyHashTestFunction<QHash<QString, int>>();
    if (QTest::currentTestFailed())
        return;

    valueInEmptyHashTestFunction<QMultiHash<QString, int>>();
}

void tst_QHash::fineTuningInEmptyHash()
{
    QHash<QString, int> hash;
    QCOMPARE(hash.capacity(), 0);
    hash.squeeze();
    QCOMPARE(hash.capacity(), 0);
    QVERIFY(qFuzzyIsNull(hash.load_factor()));
    QVERIFY(!hash.isDetached());

    hash.reserve(10);
    QVERIFY(hash.capacity() >= 10);
    hash.squeeze();
    QVERIFY(hash.capacity() > 0);
}

void tst_QHash::reserveShared()
{
    QHash<char, char> hash;
    hash.insert('c', 'c');
    auto hash2 = hash;

    QCOMPARE(hash2.capacity(), hash.capacity());
    auto oldCap = hash.capacity();

    hash2.reserve(100); // This shouldn't crash

    QVERIFY(hash2.capacity() >= 100);
    QCOMPARE(hash.capacity(), oldCap);
}

void tst_QHash::reserveLessThanCurrentAmount()
{
    {
        QHash<int, int> hash;
        for (int i = 0; i < 1000; ++i)
            hash.insert(i, i * 10);

        // This used to hang in an infinite loop: QTBUG-102067
        hash.reserve(1);

        // Make sure that hash still has all elements
        for (int i = 0; i < 1000; ++i)
            QCOMPARE(hash.value(i), i * 10);
    }
    {
        QMultiHash<int, int> hash;
        for (int i = 0; i < 1000; ++i) {
            hash.insert(i, i * 10);
            hash.insert(i, i * 10 + 1);
        }

        // This used to hang in infinite loop: QTBUG-102067
        hash.reserve(1);

        // Make sure that hash still has all elements
        for (int i = 0; i < 1000; ++i)
            QCOMPARE(hash.values(i), QList<int>({ i * 10 + 1, i * 10 }));
    }
}

void tst_QHash::reserveKeepCapacity_data()
{
    QTest::addColumn<qsizetype>("requested");
    auto addRow = [](qsizetype requested) {
        QTest::addRow("%td", ptrdiff_t(requested)) << requested;
    };

    QHash<int, int> testHash = {{1, 1}};
    qsizetype minCapacity = testHash.capacity();
    addRow(minCapacity - 1);
    addRow(minCapacity + 0);
    addRow(minCapacity + 1);
    addRow(2 * minCapacity - 1);
    addRow(2 * minCapacity + 0);
    addRow(2 * minCapacity + 1);
}

void tst_QHash::reserveKeepCapacity()
{
    QFETCH(qsizetype, requested);

    QHash<qsizetype, qsizetype> hash;
    hash.reserve(requested);
    qsizetype initialCapacity = hash.capacity();
    QCOMPARE_GE(initialCapacity, requested);

    // insert this many elements into the hash
    for (qsizetype i = 0; i < requested; ++i)
        hash.insert(i, i);

    // it mustn't have increased capacity after inserting the elements
    QCOMPARE(hash.capacity(), initialCapacity);
}

void tst_QHash::QTBUG98265()
{
    QMultiHash<QUuid, QByteArray> a;
    QMultiHash<QUuid, QByteArray> b;
    a.insert(QUuid("3e0dfb4d-90eb-43a4-bd54-88f5b69832c1"), QByteArray());
    b.insert(QUuid("1b710ada-3dd7-432e-b7c8-e852e59f46a0"), QByteArray());

    QVERIFY(a != b);
}

/*
    Calling functions which take a const-ref argument for a key with a reference
    to a key inside the hash itself should keep the key valid as long as it is
    needed. If not users may get hard-to-debug races where CoW should've
    shielded them.
*/
void tst_QHash::detachAndReferences()
{
    // Repeat a few times because it's not a guarantee
    for (int i = 0; i < 50; ++i) {
        QHash<char, char> hash;
        hash.insert('a', 'a');
        hash.insert('b', 'a');
        hash.insert('c', 'a');
        hash.insert('d', 'a');
        hash.insert('e', 'a');
        hash.insert('f', 'a');
        hash.insert('g', 'a');

        QSemaphore sem;
        QSemaphore sem2;
        std::thread th([&sem, &sem2, hash]() mutable {
            sem.release();
            sem2.acquire();
            hash.reserve(100); // [2]: ...then this rehashes directly, without detaching
        });

        // The key is a reference to an entry in the hash. If we were already
        // detached then no problem occurs! The problem happens because _after_
        // we detach but before using the key the other thread resizes and
        // rehashes, leaving our const-ref dangling.
        auto it = hash.constBegin();
        const auto &key = it.key(); // [3]: leaving our const-refs dangling
        auto kCopy = key;
        const auto &value = it.value();
        auto vCopy = value;
        sem2.release();
        sem.acquire();
        hash.insert(key, value); // [1]: this detaches first...

        th.join();
        QCOMPARE(hash.size(), 7);
        QVERIFY(hash.contains(kCopy));
        QCOMPARE(hash.value(kCopy), vCopy);
    }
}

void tst_QHash::lookupUsingKeyIterator()
{
    QHash<QString, QString> hash;
    hash.reserve(1);
    qsizetype minCapacity = hash.capacity();
    // Beholden to internal implementation details:
    qsizetype rehashLimit = minCapacity == 64 ? 63 : 8;

    for (char16_t c = u'a'; c <= u'a' + rehashLimit; ++c)
        hash.insert(QString(QChar(c)), u"h"_s);

    for (auto it = hash.keyBegin(), end = hash.keyEnd(); it != end; ++it)
        QVERIFY(!hash[*it].isEmpty());
}

void tst_QHash::squeeze()
{
    {
        QHash<int, int> hash;
        hash.reserve(1000);
        for (int i = 0; i < 10; ++i)
            hash.insert(i, i * 10);
        QVERIFY(hash.isDetached());
        const size_t buckets = hash.bucket_count();
        const qsizetype size = hash.size();

        hash.squeeze();

        QVERIFY(hash.bucket_count() < buckets);
        QCOMPARE(hash.size(), size);
        for (int i = 0; i < size; ++i)
            QCOMPARE(hash.value(i), i * 10);
    }
    {
        QMultiHash<int, int> hash;
        hash.reserve(1000);
        for (int i = 0; i < 10; ++i) {
            hash.insert(i, i * 10);
            hash.insert(i, i * 10 + 1);
        }
        QVERIFY(hash.isDetached());
        const size_t buckets = hash.bucket_count();
        const qsizetype size = hash.size();

        hash.squeeze();

        QVERIFY(hash.bucket_count() < buckets);
        QCOMPARE(hash.size(), size);
        for (int i = 0; i < (size / 2); ++i)
            QCOMPARE(hash.values(i), QList<int>({ i * 10 + 1, i * 10 }));
    }
}

void tst_QHash::squeezeShared()
{
    {
        QHash<int, int> hash;
        hash.reserve(1000);
        for (int i = 0; i < 10; ++i)
            hash.insert(i, i * 10);

        QHash<int, int> other = hash;

        // Check that when squeezing a hash with shared d_ptr, the number of
        // buckets actually decreases.
        QVERIFY(!other.isDetached());
        const size_t buckets = other.bucket_count();
        const qsizetype size = other.size();

        other.squeeze();

        QCOMPARE(hash.bucket_count(), buckets);
        QVERIFY(other.bucket_count() < buckets);

        QCOMPARE(other.size(), size);
        for (int i = 0; i < size; ++i)
            QCOMPARE(other.value(i), i * 10);
    }
    {
        QMultiHash<int, int> hash;
        hash.reserve(1000);
        for (int i = 0; i < 10; ++i) {
            hash.insert(i, i * 10);
            hash.insert(i, i * 10 + 1);
        }

        QMultiHash<int, int> other = hash;

        // Check that when squeezing a hash with shared d_ptr, the number of
        // buckets actually decreases.
        QVERIFY(!other.isDetached());
        const size_t buckets = other.bucket_count();
        const qsizetype size = other.size();

        other.squeeze();

        QCOMPARE(hash.bucket_count(), buckets);
        QVERIFY(other.bucket_count() < buckets);

        QCOMPARE(other.size(), size);
        for (int i = 0; i < (size / 2); ++i)
            QCOMPARE(other.values(i), QList<int>({ i * 10 + 1, i * 10 }));
    }
}

QTEST_APPLESS_MAIN(tst_QHash)
#include "tst_qhash.moc"
