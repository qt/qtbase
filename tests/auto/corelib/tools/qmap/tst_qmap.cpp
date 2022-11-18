// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmap.h>
#include <QTest>
#include <QDebug>

QT_WARNING_DISABLE_DEPRECATED

class tst_QMap : public QObject
{
    Q_OBJECT
protected:
    template <class Map>
    void sanityCheckTree(const Map &m, int calledFromLine);
public slots:
    void init();
private slots:
    void ctor();
    void count();
    void clear();
    void beginEnd();
    void firstLast();
    void key();
    void value();

    void swap();

    void operator_eq();

    void empty();
    void contains();
    void find();
    void constFind();
    void lowerUpperBound();
    void mergeCompare();
    void take();

    void iterators();
    void multimapIterators();
    void iteratorsInEmptyMap();
    void keyIterator();
    void multimapKeyIterator();
    void keyValueIterator();
    void multimapKeyValueIterator();
    void keyValueIteratorInEmptyMap();
    void keys_values_uniqueKeys();
    void qmultimap_specific();

    void const_shared_null();

    void equal_range();

    void insert();
    void insertMap();
    void checkMostLeftNode();
    void initializerList();
    void testInsertWithHint();
    void testInsertMultiWithHint();
    void eraseValidIteratorOnSharedMap();
    void removeElementsInMap();
    void toStdMap();

    // Tests for deprecated APIs.
#if QT_DEPRECATED_SINCE(6, 0)
    void deprecatedInsertMulti();
    void deprecatedIteratorApis();
    void deprecatedInsert();
#endif // QT_DEPRECATED_SINCE(6, 0)
};

struct IdentityTracker {
    int value, id;
};

inline bool operator<(IdentityTracker lhs, IdentityTracker rhs) { return lhs.value < rhs.value; }

typedef QMap<QString, QString> StringMap;

class MyClass
{
public:
    MyClass() {
        ++count;
    }
    MyClass( const QString& c) {
        count++; str = c;
    }
    ~MyClass() {
        count--;
    }
    MyClass( const MyClass& c ) {
        count++; str = c.str;
    }
    MyClass &operator =(const MyClass &o) {
        str = o.str; return *this;
    }

    QString str;
    static int count;
};

int MyClass::count = 0;

typedef QMap<QString, MyClass> MyMap;
typedef QMultiMap<QString, MyClass> MyMultiMap;

QDebug operator << (QDebug d, const MyClass &c) {
    d << c.str;
    return d;
}

template <class Map>
void tst_QMap::sanityCheckTree(const Map &m, int calledFromLine)
{
    QString possibleFrom;
    possibleFrom.setNum(calledFromLine);
    possibleFrom = "Called from line: " + possibleFrom;
    int count = 0;
    typename Map::const_iterator oldite = m.constBegin();
    for (typename Map::const_iterator i = m.constBegin(); i != m.constEnd(); ++i) {
        count++;
        bool oldIteratorIsLarger = i.key() < oldite.key();
        QVERIFY2(!oldIteratorIsLarger, possibleFrom.toUtf8());
        oldite = i;
    }
    if (m.size() != count) { // Fail
        qDebug() << possibleFrom;
        QCOMPARE(m.size(), count);
    }
    if (m.size() == 0)
        QVERIFY(m.constBegin() == m.constEnd());
}

void tst_QMap::init()
{
    MyClass::count = 0;
}

void tst_QMap::ctor()
{
    std::map<int, int> map;
    for (int i = 0; i < 100000; ++i)
        map.insert(std::pair<int, int>(i * 3, i * 7));

    QMap<int, int> qmap(map); // ctor.

    // Check that we have the same
    std::map<int, int>::iterator j = map.begin();
    QMap<int, int>::const_iterator i = qmap.constBegin();

    while (i != qmap.constEnd()) {
        QCOMPARE( (*j).first, i.key());
        QCOMPARE( (*j).second, i.value());
        ++i;
        ++j;
    }

    QCOMPARE( (int) map.size(), qmap.size());
}



void tst_QMap::count()
{
    {
        MyMap map;
        MyMap map2( map );
        QCOMPARE( map.size(), 0 );
        QCOMPARE( map2.size(), 0 );
        QCOMPARE( MyClass::count, int(0) );
        QCOMPARE(map.count("key"), 0);
        QCOMPARE(map.size(), 0);
        QCOMPARE(map2.size(), 0);
        QVERIFY(!map.isDetached());
        QVERIFY(!map2.isDetached());
        // detach
        map2["Hallo"] = MyClass( "Fritz" );
        QCOMPARE( map.size(), 0 );
        QCOMPARE( map.size(), 0 );
        QCOMPARE( map2.size(), 1 );
        QCOMPARE( map2.size(), 1 );
        QVERIFY(!map.isDetached());
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 1 );
#endif
    }
    QCOMPARE( MyClass::count, int(0) );

    {
        typedef QMap<QString, MyClass> Map;
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
        QCOMPARE( map.count("Paul"), 1 );
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
        typedef QMap<QString,MyClass> Map;
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
            QMap<int, MyClass> map;
            for (int j = 0; j < i; ++j)
                map.insert(j, MyClass(QString::number(j)));
        }
        QCOMPARE( MyClass::count, 0 );
    }
    QCOMPARE( MyClass::count, 0 );

    {
        QMultiMap<int, MyClass> map;
        QMultiMap<int, MyClass> map2(map);
        QCOMPARE(map.size(), 0);
        QCOMPARE(map2.size(), 0);
        QCOMPARE(MyClass::count, 0);
        QCOMPARE(map.count(1), 0);
        QCOMPARE(map.size(), 0);
        QCOMPARE(map2.size(), 0);
        QVERIFY(!map.isDetached());
        QVERIFY(!map2.isDetached());

        // detach
        map2.insert(0, MyClass("value0"));
        QCOMPARE(map.size(), 0);
        QCOMPARE(map.size(), 0);
        QCOMPARE(map2.size(), 1);
        QCOMPARE(map2.size(), 1);
        QVERIFY(!map.isDetached());
        QCOMPARE(MyClass::count, 1);

        map2.insert(1, MyClass("value1"));
        map2.insert(2, MyClass("value2"));
        QCOMPARE(map2.size(), 3);
        QCOMPARE(MyClass::count, 3);

        map2.insert(0, MyClass("value0_1"));
        map2.insert(0, MyClass("value0_2"));
        QCOMPARE(map2.size(), 5);
        QCOMPARE(map2.count(0), 3);
        QCOMPARE(MyClass::count, 5);

        map2.clear();
        QCOMPARE(map2.size(), 0);
        QCOMPARE(MyClass::count, 0);

    }
    QCOMPARE(MyClass::count, 0);
}

void tst_QMap::clear()
{
    {
        MyMap map;
        map.clear();
        QVERIFY(map.isEmpty());
        QVERIFY(!map.isDetached());
        map.insert( "key", MyClass( "value" ) );
        QVERIFY(!map.isEmpty());
        map.clear();
        QVERIFY(map.isEmpty());
        map.insert( "key0", MyClass( "value0" ) );
        map.insert( "key0", MyClass( "value1" ) );
        map.insert( "key1", MyClass( "value2" ) );
        QVERIFY(!map.isEmpty());
        map.clear();
        sanityCheckTree(map, __LINE__);
        QVERIFY(map.isEmpty());
    }
    QCOMPARE(MyClass::count, int(0));

    {
        MyMultiMap map;
        map.clear();
        QVERIFY(map.isEmpty());
        QVERIFY(!map.isDetached());
        map.insert( "key", MyClass( "value" ) );
        QVERIFY(!map.isEmpty());
        map.clear();
        QVERIFY(map.isEmpty());
        map.insert( "key0", MyClass( "value0" ) );
        map.insert( "key0", MyClass( "value1" ) );
        map.insert( "key1", MyClass( "value2" ) );
        QVERIFY(!map.isEmpty());
        map.clear();
        sanityCheckTree(map, __LINE__);
        QVERIFY(map.isEmpty());
    }
    QCOMPARE(MyClass::count, int(0));
}

void tst_QMap::beginEnd()
{
    StringMap m0;
    QVERIFY( m0.begin() == m0.end() );
    QVERIFY( m0.begin() == m0.begin() );

    // sample string->string map
    StringMap map;
    QVERIFY( map.constBegin() == map.constEnd() );
    map.insert( "0", "a" );
    map.insert( "1", "b" );
    QVERIFY( map.constBegin() == map.cbegin() );
    QVERIFY( map.constEnd()   == map.cend() );

    // make a copy. const function shouldn't detach
    StringMap map2 = map;
    QVERIFY( map.constBegin() == map2.constBegin() );
    QVERIFY( map.constEnd() == map2.constEnd() );

    // test iteration
    QString result;
    for ( StringMap::ConstIterator it = map.constBegin();
          it != map.constEnd(); ++it )
        result += *it;
    QCOMPARE( result, QString( "ab" ) );

    // maps should still be identical
    QVERIFY( map.constBegin() == map2.constBegin() );
    QVERIFY( map.constEnd() == map2.constEnd() );

    // detach
    map2.insert( "2", "c" );
    QVERIFY( map.constBegin() == map.constBegin() );

    // comparing iterators between two different std::map is UB (and raises an
    // assertion failure with MSVC debug-mode iterators), so we compare the
    // elements' addresses.
    QVERIFY(&map.constBegin().key() != &map2.constBegin().key());
    QVERIFY(&map.constBegin().value() != &map2.constBegin().value());
}

void tst_QMap::firstLast()
{
    // sample string->string map
    StringMap map;
    map.insert("0", "a");
    map.insert("1", "b");
    map.insert("5", "e");

    QCOMPARE(map.firstKey(), QStringLiteral("0"));
    QCOMPARE(map.lastKey(), QStringLiteral("5"));
    QCOMPARE(map.first(), QStringLiteral("a"));
    QCOMPARE(map.last(), QStringLiteral("e"));

    // const map
    const StringMap const_map = map;
    QCOMPARE(map.firstKey(), const_map.firstKey());
    QCOMPARE(map.lastKey(), const_map.lastKey());
    QCOMPARE(map.first(), const_map.first());
    QCOMPARE(map.last(), const_map.last());

    map.take(map.firstKey());
    QCOMPARE(map.firstKey(), QStringLiteral("1"));
    QCOMPARE(map.lastKey(), QStringLiteral("5"));

    map.take(map.lastKey());
    QCOMPARE(map.lastKey(), map.lastKey());
}

void tst_QMap::key()
{
    {
        QString def("default value");

        QMap<QString, int> map1;
        QCOMPARE(map1.key(1), QString());
        QCOMPARE(map1.key(1, def), def);
        QVERIFY(!map1.isDetached());

        map1.insert("one", 1);
        QCOMPARE(map1.key(1), QLatin1String("one"));
        QCOMPARE(map1.key(1, def), QLatin1String("one"));
        QCOMPARE(map1.key(2), QString());
        QCOMPARE(map1.key(2, def), def);

        map1.insert("two", 2);
        QCOMPARE(map1.key(1), QLatin1String("one"));
        QCOMPARE(map1.key(1, def), QLatin1String("one"));
        QCOMPARE(map1.key(2), QLatin1String("two"));
        QCOMPARE(map1.key(2, def), QLatin1String("two"));
        QCOMPARE(map1.key(3), QString());
        QCOMPARE(map1.key(3, def), def);

        map1.insert("deux", 2);
        QCOMPARE(map1.key(1), QLatin1String("one"));
        QCOMPARE(map1.key(1, def), QLatin1String("one"));
        QVERIFY(map1.key(2) == QLatin1String("deux") || map1.key(2) == QLatin1String("two"));
        QVERIFY(map1.key(2, def) == QLatin1String("deux") || map1.key(2, def) == QLatin1String("two"));
        QCOMPARE(map1.key(3), QString());
        QCOMPARE(map1.key(3, def), def);
    }

    {
        int def = 666;

        QMap<int, QString> map2;
        QCOMPARE(map2.key("one"), 0);
        QCOMPARE(map2.key("one", def), def);
        QVERIFY(!map2.isDetached());

        map2.insert(1, "one");
        QCOMPARE(map2.key("one"), 1);
        QCOMPARE(map2.key("one", def), 1);
        QCOMPARE(map2.key("two"), 0);
        QCOMPARE(map2.key("two", def), def);

        map2.insert(2, "two");
        QCOMPARE(map2.key("one"), 1);
        QCOMPARE(map2.key("one", def), 1);
        QCOMPARE(map2.key("two"), 2);
        QCOMPARE(map2.key("two", def), 2);
        QCOMPARE(map2.key("three"), 0);
        QCOMPARE(map2.key("three", def), def);

        map2.insert(3, "two");
        QCOMPARE(map2.key("one"), 1);
        QCOMPARE(map2.key("one", def), 1);
        QCOMPARE(map2.key("two"), 2);
        QCOMPARE(map2.key("two", def), 2);
        QCOMPARE(map2.key("three"), 0);
        QCOMPARE(map2.key("three", def), def);

        map2.insert(-1, "two");
        QCOMPARE(map2.key("two"), -1);
        QCOMPARE(map2.key("two", def), -1);

        map2.insert(0, "zero");
        QCOMPARE(map2.key("zero"), 0);
        QCOMPARE(map2.key("zero", def), 0);
    }

    {
        int def = -1;
        QMultiMap<int, QString> multiMap;
        QCOMPARE(multiMap.key("value0"), 0);
        QCOMPARE(multiMap.key("value0", def), def);
        QVERIFY(!multiMap.isDetached());

        multiMap.insert(1, "value1");
        multiMap.insert(2, "value2");
        multiMap.insert(1, "value1_1");

        QCOMPARE(multiMap.key("value1"), 1);
        QCOMPARE(multiMap.key("value1", def), 1);
        QCOMPARE(multiMap.key("value1_1"), 1);
        QCOMPARE(multiMap.key("value2"), 2);
        QCOMPARE(multiMap.key("value3"), 0);
        QCOMPARE(multiMap.key("value3", def), def);
    }
}

void tst_QMap::value()
{
    const QString def = "default value";
    {
        QMap<int, QString> map;
        QCOMPARE(map.value(1), QString());
        QCOMPARE(map.value(1, def), def);
        QVERIFY(!map.isDetached());

        map.insert(1, "value1");
        QCOMPARE(map.value(1), "value1");
        QCOMPARE(map[1], "value1");
        QCOMPARE(map.value(2), QString());
        QCOMPARE(map.value(2, def), def);
        QCOMPARE(map[2], QString());
        QCOMPARE(map.size(), 2);

        map.insert(2, "value2");
        QCOMPARE(map.value(2), "value2");
        QCOMPARE(map[2], "value2");

        map.insert(1, "value3");
        QCOMPARE(map.value(1), "value3");
        QCOMPARE(map.value(1, def), "value3");
        QCOMPARE(map[1], "value3");

        const QMap<int, QString> constMap;
        QVERIFY(!constMap.isDetached());
        QCOMPARE(constMap.value(1, def), def);
        QCOMPARE(constMap[1], QString());
        QCOMPARE(constMap.size(), 0);
        QVERIFY(!constMap.isDetached());
    }
    {
        QMultiMap<int, QString> map;
        QCOMPARE(map.value(1), QString());
        QCOMPARE(map.value(1, def), def);
        QVERIFY(!map.isDetached());

        map.insert(1, "value1");
        QCOMPARE(map.value(1), "value1");
        QCOMPARE(map.value(2), QString());
        QCOMPARE(map.value(2, def), def);

        map.insert(2, "value2");
        QCOMPARE(map.value(2), "value2");

        map.insert(1, "value3");
        // If multiple values exist, the most recently added is returned.
        QCOMPARE(map.value(1), "value3");
        QCOMPARE(map.value(1, def), "value3");

        map.remove(1, "value3");
        QCOMPARE(map.value(1), "value1");
    }
}

void tst_QMap::swap()
{
    QMap<int,QString> m1, m2;
    m1[0] = "m1[0]";
    m2[1] = "m2[1]";
    m1.swap(m2);
    QCOMPARE(m1.value(1),QLatin1String("m2[1]"));
    QCOMPARE(m2.value(0),QLatin1String("m1[0]"));
    sanityCheckTree(m1, __LINE__);
    sanityCheckTree(m2, __LINE__);
}

void tst_QMap::operator_eq()
{
    {
        // compare for equality:
        QMap<int, int> a;
        QMap<int, int> b;

        QVERIFY(a == b);
        QVERIFY(!(a != b));

        a.insert(1,1);
        b.insert(1,1);
        QVERIFY(a == b);
        QVERIFY(!(a != b));

        a.insert(0,1);
        b.insert(0,1);
        QVERIFY(a == b);
        QVERIFY(!(a != b));

        // compare for inequality:
        a.insert(42,0);
        QVERIFY(a != b);
        QVERIFY(!(a == b));

        a.insert(65, -1);
        QVERIFY(a != b);
        QVERIFY(!(a == b));

        b.insert(-1, -1);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        // a more complex map
        QMap<QString, QString> a;
        QMap<QString, QString> b;

        QVERIFY(a == b);
        QVERIFY(!(a != b));

        a.insert("Hello", "World");
        QVERIFY(a != b);
        QVERIFY(!(a == b));

        b.insert("Hello", "World");
        QVERIFY(a == b);
        QVERIFY(!(a != b));

        a.insert("Goodbye", "cruel world");
        QVERIFY(a != b);
        QVERIFY(!(a == b));

        b.insert("Goodbye", "cruel world");

        // what happens if we insert nulls?
        a.insert(QString(), QString());
        QVERIFY(a != b);
        QVERIFY(!(a == b));

        // empty keys and null keys match:
        b.insert(QString(""), QString());
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    {
        QMap<QString, int> a;
        QMap<QString, int> b;

        a.insert("otto", 1);
        b.insert("willy", 1);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }
}

template <typename T>
void emptyTestMethod()
{
    T map;

    QVERIFY(map.isEmpty());
    QVERIFY(map.empty());
    QVERIFY(!map.isDetached());

    map.insert(1, "one");
    QVERIFY(!map.isEmpty());
    QVERIFY(!map.empty());

    map.clear();
    QVERIFY(map.isEmpty());
    QVERIFY(map.empty());
}

void tst_QMap::empty()
{
    emptyTestMethod<QMap<int, QString>>();
    if (QTest::currentTestFailed())
        return;

    emptyTestMethod<QMultiMap<int, QString>>();
}

void tst_QMap::contains()
{
    {
        QMap<int, QString> map1;
        int i;

        QVERIFY(!map1.contains(1));
        QVERIFY(!map1.isDetached());

        map1.insert(1, "one");
        QVERIFY(map1.contains(1));

        for (i = 2; i < 100; ++i)
            map1.insert(i, "teststring");
        for (i = 99; i > 1; --i)
            QVERIFY(map1.contains(i));

        map1.remove(43);
        QVERIFY(!map1.contains(43));
    }

    {
        QMultiMap<int, QString> multiMap;
        QVERIFY(!multiMap.contains(1));
        QVERIFY(!multiMap.contains(1, "value1"));
        QVERIFY(!multiMap.isDetached());

        multiMap.insert(1, "value1");
        multiMap.insert(2, "value2");
        multiMap.insert(1, "value1_1");

        QVERIFY(multiMap.contains(1));
        QVERIFY(multiMap.contains(1, "value1"));
        QVERIFY(multiMap.contains(1, "value1_1"));
        QVERIFY(multiMap.contains(2));
        QVERIFY(multiMap.contains(2, "value2"));
        QVERIFY(!multiMap.contains(2, "invalid_value"));

        QVERIFY(!multiMap.contains(3));
        multiMap.insert(3, "value3");
        QVERIFY(multiMap.contains(3));

        multiMap.remove(3);
        QVERIFY(!multiMap.contains(3));
    }
}

void tst_QMap::find()
{
    {
        const QMap<int, QString> constMap;
        QCOMPARE(constMap.find(1), constMap.end());
        QVERIFY(!constMap.isDetached());

        QMap<int, QString> map;
        QCOMPARE(map.find(1), map.end());

        map.insert(1, "value1");
        map.insert(2, "value2");
        map.insert(5, "value5");
        map.insert(1, "value0");

        QCOMPARE(map.find(1).value(), u"value0");
        QCOMPARE(map.find(5).value(), u"value5");
        QCOMPARE(map.find(2).value(), u"value2");
        QCOMPARE(map.find(4), map.end());
    }

    const QMultiMap<int, QString> constMap;
    QCOMPARE(constMap.find(1), constMap.end());
    QCOMPARE(constMap.find(1, "value"), constMap.end());
    QVERIFY(!constMap.isDetached());

    QMultiMap<int, QString> map;
    QString testString="Teststring %0";
    QString compareString;
    int i,count=0;

    QCOMPARE(map.find(1), map.end());
    QCOMPARE(map.find(1, "value1"), map.end());

    map.insert(1,"Mensch");
    map.insert(1,"Mayer");
    map.insert(2,"Hej");

    QCOMPARE(map.find(1).value(), QLatin1String("Mayer"));
    QCOMPARE(map.find(2).value(), QLatin1String("Hej"));
    QCOMPARE(map.find(1, "Mensch").value(), QLatin1String("Mensch"));
    QCOMPARE(map.find(1, "Unknown Value"), map.end());

    for (i = 3; i < 10; ++i) {
        compareString = testString.arg(i);
        map.insert(4, compareString);
        QCOMPARE(map.count(4), i - 2);
    }

    QMultiMap<int, QString>::iterator it = map.find(4);

    for (i = 9; i > 2 && it != map.end() && it.key() == 4; --i) {
        compareString = testString.arg(i);
        QVERIFY(it.value() == compareString);
        QCOMPARE(map.find(4, compareString), it);
        ++it;
        ++count;
    }
    QCOMPARE(count, 7);
}

void tst_QMap::constFind()
{
    {
        QMap<int, QString> map;
        QCOMPARE(map.constFind(1), map.constEnd());
        QVERIFY(!map.isDetached());

        map.insert(1, "value1");
        map.insert(2, "value2");
        map.insert(5, "value5");
        map.insert(1, "value0");

        QCOMPARE(map.constFind(1).value(), QLatin1String("value0"));
        QCOMPARE(map.constFind(5).value(), QLatin1String("value5"));
        QCOMPARE(map.constFind(2).value(), QLatin1String("value2"));
        QCOMPARE(map.constFind(4), map.constEnd());
    }

    QMultiMap<int, QString> map;
    QString testString="Teststring %0";
    QString compareString;
    int i,count=0;

    QCOMPARE(map.constFind(1), map.constEnd());
    QCOMPARE(map.constFind(1, "value"), map.constEnd());
    QVERIFY(!map.isDetached());

    map.insert(1,"Mensch");
    map.insert(1,"Mayer");
    map.insert(2,"Hej");

    QVERIFY(map.constFind(4) == map.constEnd());

    QCOMPARE(map.constFind(1).value(), QLatin1String("Mayer"));
    QCOMPARE(map.constFind(2).value(), QLatin1String("Hej"));
    QCOMPARE(map.constFind(1, "Mensch").value(), QLatin1String("Mensch"));
    QCOMPARE(map.constFind(1, "Invalid Value"), map.constEnd());

    for (i = 3; i < 10; ++i) {
        compareString = testString.arg(i);
        map.insert(4, compareString);
    }

    QMultiMap<int, QString>::const_iterator it = map.constFind(4);

    for (i = 9; i > 2 && it != map.constEnd() && it.key() == 4; --i) {
        compareString = testString.arg(i);
        QVERIFY(it.value() == compareString);
        QCOMPARE(map.constFind(4, compareString), it);
        ++it;
        ++count;
    }
    QCOMPARE(count, 7);
}

void tst_QMap::lowerUpperBound()
{
    {
        const QMap<int, QString> emptyConstMap;
        QCOMPARE(emptyConstMap.lowerBound(1), emptyConstMap.constEnd());
        QCOMPARE(emptyConstMap.upperBound(1), emptyConstMap.constEnd());
        QVERIFY(!emptyConstMap.isDetached());

        const QMap<int, QString> constMap { qMakePair(1, "one"),
                                            qMakePair(5, "five"),
                                            qMakePair(10, "ten") };

        QCOMPARE(constMap.lowerBound(-1).key(), 1);
        QCOMPARE(constMap.lowerBound(1).key(), 1);
        QCOMPARE(constMap.lowerBound(3).key(), 5);
        QCOMPARE(constMap.lowerBound(12), constMap.constEnd());

        QCOMPARE(constMap.upperBound(-1).key(), 1);
        QCOMPARE(constMap.upperBound(1).key(), 5);
        QCOMPARE(constMap.upperBound(3).key(), 5);
        QCOMPARE(constMap.upperBound(12), constMap.constEnd());

        QMap<int, QString> map;

        map.insert(1, "one");
        map.insert(5, "five");
        map.insert(10, "ten");
        map.insert(3, "three");
        map.insert(7, "seven");

        QCOMPARE(map.lowerBound(0).key(), 1);
        QCOMPARE(map.lowerBound(1).key(), 1);
        QCOMPARE(map.lowerBound(2).key(), 3);
        QCOMPARE(map.lowerBound(3).key(), 3);
        QCOMPARE(map.lowerBound(4).key(), 5);
        QCOMPARE(map.lowerBound(5).key(), 5);
        QCOMPARE(map.lowerBound(6).key(), 7);
        QCOMPARE(map.lowerBound(7).key(), 7);
        QCOMPARE(map.lowerBound(10).key(), 10);
        QCOMPARE(map.lowerBound(999), map.end());

        QCOMPARE(map.upperBound(0).key(), 1);
        QCOMPARE(map.upperBound(1).key(), 3);
        QCOMPARE(map.upperBound(2).key(), 3);
        QCOMPARE(map.upperBound(3).key(), 5);
        QCOMPARE(map.upperBound(7).key(), 10);
        QCOMPARE(map.upperBound(10), map.end());
        QCOMPARE(map.upperBound(999), map.end());
    }

    const QMultiMap<int, QString> emptyConstMap;
    QCOMPARE(emptyConstMap.lowerBound(1), emptyConstMap.constEnd());
    QCOMPARE(emptyConstMap.upperBound(1), emptyConstMap.constEnd());
    QVERIFY(!emptyConstMap.isDetached());

    const QMultiMap<int, QString> constMap { qMakePair(1, "one"),
                                             qMakePair(5, "five"),
                                             qMakePair(10, "ten") };

    QCOMPARE(constMap.lowerBound(-1).key(), 1);
    QCOMPARE(constMap.lowerBound(1).key(), 1);
    QCOMPARE(constMap.lowerBound(3).key(), 5);
    QCOMPARE(constMap.lowerBound(12), constMap.constEnd());

    QCOMPARE(constMap.upperBound(-1).key(), 1);
    QCOMPARE(constMap.upperBound(1).key(), 5);
    QCOMPARE(constMap.upperBound(3).key(), 5);
    QCOMPARE(constMap.upperBound(12), constMap.constEnd());

    QMultiMap<int, QString> map;

    map.insert(1, "one");
    map.insert(5, "five");
    map.insert(10, "ten");

    //Copied from documentation

    QCOMPARE(map.upperBound(0).key(), 1);      // returns iterator to (1, "one")
    QCOMPARE(map.upperBound(1).key(), 5);      // returns iterator to (5, "five")
    QCOMPARE(map.upperBound(2).key(), 5);      // returns iterator to (5, "five")
    QVERIFY(map.upperBound(10) == map.end());     // returns end()
    QVERIFY(map.upperBound(999) == map.end());    // returns end()

    QCOMPARE(map.lowerBound(0).key(), 1);      // returns iterator to (1, "one")
    QCOMPARE(map.lowerBound(1).key(), 1);      // returns iterator to (1, "one")
    QCOMPARE(map.lowerBound(2).key(), 5);      // returns iterator to (5, "five")
    QCOMPARE(map.lowerBound(10).key(), 10);     // returns iterator to (10, "ten")
    QVERIFY(map.lowerBound(999) == map.end());    // returns end()

    map.insert(3, "three");
    map.insert(7, "seven");
    map.insert(7, "seven_2");

    QCOMPARE(map.upperBound(0).key(), 1);
    QCOMPARE(map.upperBound(1).key(), 3);
    QCOMPARE(map.upperBound(2).key(), 3);
    QCOMPARE(map.upperBound(3).key(), 5);
    QCOMPARE(map.upperBound(7).key(), 10);
    QVERIFY(map.upperBound(10) == map.end());
    QVERIFY(map.upperBound(999) == map.end());

    QCOMPARE(map.lowerBound(0).key(), 1);
    QCOMPARE(map.lowerBound(1).key(), 1);
    QCOMPARE(map.lowerBound(2).key(), 3);
    QCOMPARE(map.lowerBound(3).key(), 3);
    QCOMPARE(map.lowerBound(4).key(), 5);
    QCOMPARE(map.lowerBound(5).key(), 5);
    QCOMPARE(map.lowerBound(6).key(), 7);
    QCOMPARE(map.lowerBound(7).key(), 7);
    QCOMPARE(map.lowerBound(6).value(), QLatin1String("seven_2"));
    QCOMPARE(map.lowerBound(7).value(), QLatin1String("seven_2"));
    QCOMPARE((++map.lowerBound(6)).value(), QLatin1String("seven"));
    QCOMPARE((++map.lowerBound(7)).value(), QLatin1String("seven"));
    QCOMPARE(map.lowerBound(10).key(), 10);
    QVERIFY(map.lowerBound(999) == map.end());
}

void tst_QMap::mergeCompare()
{
    QMultiMap<int, QString> map1, map2, map3, map1b, map2b, map4;

    // unite with an empty map does nothing
    map1.unite(map2);
    QVERIFY(!map1.isDetached());

    map1.insert(1,"ett");
    map1.insert(3,"tre");
    map1.insert(5,"fem");

    map2.insert(2,"tvo");
    map2.insert(4,"fyra");

    map1.unite(map2);
    sanityCheckTree(map1, __LINE__);

    map1b = map1;
    map2b = map2;
    map2b.insert(0, "nul");
    map1b.unite(map2b);
    sanityCheckTree(map1b, __LINE__);

    QCOMPARE(map1.value(1), QLatin1String("ett"));
    QCOMPARE(map1.value(2), QLatin1String("tvo"));
    QCOMPARE(map1.value(3), QLatin1String("tre"));
    QCOMPARE(map1.value(4), QLatin1String("fyra"));
    QCOMPARE(map1.value(5), QLatin1String("fem"));

    map3.insert(1, "ett");
    map3.insert(2, "tvo");
    map3.insert(3, "tre");
    map3.insert(4, "fyra");
    map3.insert(5, "fem");

    QCOMPARE(map1, map3);

    map4.unite(map3);
    QCOMPARE(map4, map3);
}

void tst_QMap::take()
{
    {
        QMap<int, QString> map;
        QCOMPARE(map.take(1), QString());
        QVERIFY(!map.isDetached());

        map.insert(2, "zwei");
        map.insert(3, "drei");

        QCOMPARE(map.take(3), QLatin1String("drei"));
        QVERIFY(!map.contains(3));
    }

    {
        QMultiMap<int, QString> multiMap;
        QCOMPARE(multiMap.take(1), QString());
        QVERIFY(!multiMap.isDetached());

        multiMap.insert(0, "value0");
        multiMap.insert(1, "value1");
        multiMap.insert(0, "value0_1");
        multiMap.insert(0, "value0_2");

        // The most recently inserted value is returned
        QCOMPARE(multiMap.take(0), u"value0_2");
        QCOMPARE(multiMap.take(0), u"value0_1");
        QCOMPARE(multiMap.take(0), u"value0");
        QCOMPARE(multiMap.take(0), QString());
        QVERIFY(!multiMap.contains(0));
    }
}

void tst_QMap::iterators()
{
    QMap<int, QString> map;
    QString testString="Teststring %1";
    int i;

    for(i = 1; i < 100; ++i)
        map.insert(i, testString.arg(i));

    //STL-Style iterators

    QMap<int, QString>::iterator stlIt = map.begin();
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 1"));

    std::advance(stlIt, 5);
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 6"));

    stlIt++;
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 7"));

    std::advance(stlIt, -3);
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 4"));

    stlIt--;
    QCOMPARE(stlIt.value(), QLatin1String("Teststring 3"));

    for(stlIt = map.begin(), i = 1; stlIt != map.end(); ++stlIt, ++i)
            QVERIFY(stlIt.value() == testString.arg(i));
    QCOMPARE(i, 100);

    //STL-Style const-iterators

    QMap<int, QString>::const_iterator cstlIt = map.constBegin();
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 1"));

    std::advance(cstlIt, 5);
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 6"));

    cstlIt++;
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 7"));

    std::advance(cstlIt, -3);
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 4"));

    cstlIt--;
    QCOMPARE(cstlIt.value(), QLatin1String("Teststring 3"));

    for(cstlIt = map.constBegin(), i = 1; cstlIt != map.constEnd(); ++cstlIt, ++i)
            QVERIFY(cstlIt.value() == testString.arg(i));
    QCOMPARE(i, 100);

    //Java-Style iterators

    QMapIterator<int, QString> javaIt(map);

    i = 0;
    while(javaIt.hasNext()) {
        ++i;
        javaIt.next();
        QVERIFY(javaIt.value() == testString.arg(i));
    }

    ++i;
    while(javaIt.hasPrevious()) {
        --i;
        javaIt.previous();
        QVERIFY(javaIt.value() == testString.arg(i));
    }

    i = 51;
    while(javaIt.hasPrevious()) {
        --i;
        javaIt.previous();
        QVERIFY(javaIt.value() == testString.arg(i));
    }
}

void tst_QMap::multimapIterators()
{
    QMultiMap<int, QString> map;
    const QString testString = "Teststring %1-%2";

    for (int i = 0; i < 5; ++i) {
        // reverse order, because the last added is returned first.
        for (int j = 4; j >= 0; --j)
            map.insert(i, testString.arg(i).arg(j));
    }

    // STL-style iterators
    auto stlIt = map.begin();
    QCOMPARE(stlIt.value(), u"Teststring 0-0");

    stlIt++;
    QCOMPARE(stlIt.value(), u"Teststring 0-1");

    std::advance(stlIt, 10);
    QCOMPARE(stlIt.value(), u"Teststring 2-1");

    std::advance(stlIt, -4);
    QCOMPARE(stlIt.value(), u"Teststring 1-2");

    stlIt--;
    QCOMPARE(stlIt.value(), u"Teststring 1-1");

    // STL-style const iterators
    auto cstlIt = map.cbegin();
    QCOMPARE(cstlIt.value(), u"Teststring 0-0");

    cstlIt++;
    QCOMPARE(cstlIt.value(), u"Teststring 0-1");

    std::advance(cstlIt, 16);
    QCOMPARE(cstlIt.value(), u"Teststring 3-2");

    std::advance(cstlIt, -6);
    QCOMPARE(cstlIt.value(), u"Teststring 2-1");

    cstlIt--;
    QCOMPARE(cstlIt.value(), u"Teststring 2-0");

    // Java-style iterator
    QMultiMapIterator javaIt(map);
    int i = 0;
    int j = 0;
    while (javaIt.hasNext()) {
        javaIt.next();
        QCOMPARE(javaIt.value(), testString.arg(i).arg(j));
        if (++j == 5) {
            j = 0;
            i++;
        }
    }

    i = 4;
    j = 4;
    while (javaIt.hasPrevious()) {
        javaIt.previous();
        QCOMPARE(javaIt.value(), testString.arg(i).arg(j));
        if (--j < 0) {
            j = 4;
            i--;
        }
    }
}

template <typename T>
void iteratorsInEmptyMapTestMethod()
{
    T map;
    using ConstIter = typename T::const_iterator;
    ConstIter it1 = map.cbegin();
    ConstIter it2 = map.constBegin();
    QVERIFY(it1 == it2 && it2 == ConstIter());
    QVERIFY(!map.isDetached());

    ConstIter it3 = map.cend();
    ConstIter it4 = map.constEnd();
    QVERIFY(it3 == it4 && it4 == ConstIter());
    QVERIFY(!map.isDetached());

    // to call const overloads of begin() and end()
    const T map2;
    ConstIter it5 = map2.begin();
    ConstIter it6 = map2.end();
    QVERIFY(it5 == it6 && it6 == ConstIter());
    QVERIFY(!map2.isDetached());

    using Iter = typename T::iterator;
    Iter it7 = map.begin();
    Iter it8 = map.end();
    QVERIFY(it7 == it8);
}

void tst_QMap::iteratorsInEmptyMap()
{
    iteratorsInEmptyMapTestMethod<QMap<int, int>>();
    if (QTest::currentTestFailed())
        return;

    iteratorsInEmptyMapTestMethod<QMultiMap<int, int>>();
}

void tst_QMap::keyIterator()
{
    QMap<int, int> map;

    using KeyIterator = QMap<int, int>::key_iterator;
    KeyIterator it1 = map.keyBegin();
    KeyIterator it2 = map.keyEnd();
    QVERIFY(it1 == it2 && it2 == KeyIterator());
    QVERIFY(!map.isDetached());

    for (int i = 0; i < 100; ++i)
        map.insert(i, i*100);

    KeyIterator key_it = map.keyBegin();
    QMap<int, int>::const_iterator it = map.cbegin();
    for (int i = 0; i < 100; ++i) {
        QCOMPARE(*key_it, it.key());
        ++key_it;
        ++it;
    }

    key_it = std::find(map.keyBegin(), map.keyEnd(), 50);
    it = std::find(map.cbegin(), map.cend(), 50 * 100);

    QVERIFY(key_it != map.keyEnd());
    QCOMPARE(*key_it, it.key());
    QCOMPARE(*(key_it++), (it++).key());
    QCOMPARE(*(key_it--), (it--).key());
    QCOMPARE(*(++key_it), (++it).key());
    QCOMPARE(*(--key_it), (--it).key());

    QCOMPARE(std::count(map.keyBegin(), map.keyEnd(), 99), 1);

    // DefaultConstructible test
    static_assert(std::is_default_constructible<KeyIterator>::value);
}

void tst_QMap::multimapKeyIterator()
{
    QMultiMap<int, int> map;

    using KeyIterator = QMultiMap<int, int>::key_iterator;
    KeyIterator it1 = map.keyBegin();
    KeyIterator it2 = map.keyEnd();
    QVERIFY(it1 == it2 && it2 == KeyIterator());
    QVERIFY(!map.isDetached());

    for (int i = 0; i < 5; ++i) {
        for (int j = 4; j >= 0; --j)
            map.insert(i, 100 * i + j);
    }

    KeyIterator keyIt = map.keyBegin();
    QMultiMap<int, int>::const_iterator it = map.cbegin();
    for (int i = 0; i < 5; ++i) {
        for (int j = 4; j >= 0; --j) {
            QCOMPARE(*keyIt, it.key());
            ++keyIt;
            ++it;
        }
    }

    keyIt = std::find(map.keyBegin(), map.keyEnd(), 3);
    it = std::find(map.cbegin(), map.cend(), 3 * 100);

    QVERIFY(keyIt != map.keyEnd());
    QCOMPARE(*keyIt, it.key());
    QCOMPARE(*(keyIt++), (it++).key());
    QCOMPARE(*(keyIt--), (it--).key());
    QCOMPARE(*(++keyIt), (++it).key());
    QCOMPARE(*(--keyIt), (--it).key());

    QCOMPARE(std::count(map.keyBegin(), map.keyEnd(), 2), 5);

    // DefaultConstructible test
    static_assert(std::is_default_constructible<KeyIterator>::value);
}

void tst_QMap::keyValueIterator()
{
    QMap<int, int> map;
    typedef QMap<int, int>::const_key_value_iterator::value_type entry_type;

    for (int i = 0; i < 100; ++i)
        map.insert(i, i * 100);

    auto key_value_it = map.constKeyValueBegin();
    auto it = map.cbegin();

    for (int i = 0; i < map.size(); ++i) {
        QVERIFY(key_value_it != map.constKeyValueEnd());
        QVERIFY(it != map.cend());

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

    QVERIFY(key_value_it == map.constKeyValueEnd());
    QVERIFY(it == map.cend());

    int key = 50;
    int value = 50 * 100;
    entry_type pair(key, value);
    key_value_it = std::find(map.constKeyValueBegin(), map.constKeyValueEnd(), pair);
    it = std::find(map.cbegin(), map.cend(), value);

    QVERIFY(key_value_it != map.constKeyValueEnd());
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));

    ++it;
    ++key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));

    --it;
    --key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));

    ++it;
    ++key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));

    --it;
    --key_value_it;
    QCOMPARE(*key_value_it, entry_type(it.key(), it.value()));
    key = 99;
    value = 99 * 100;
    QCOMPARE(std::count(map.constKeyValueBegin(), map.constKeyValueEnd(), entry_type(key, value)), 1);
}

void tst_QMap::multimapKeyValueIterator()
{
    QMultiMap<int, int> map;
    using EntryType = QMultiMap<int, int>::const_key_value_iterator::value_type;

    for (int i = 0; i < 5; ++i) {
        for (int j = 4; j >= 0; --j)
            map.insert(i, 100 * i + j);
    }

    auto keyValueIt = map.constKeyValueBegin();
    auto it = map.cbegin();

    for (int i = 0; i < map.size(); ++i) {
        QVERIFY(keyValueIt != map.constKeyValueEnd());
        QVERIFY(it != map.cend());

        EntryType pair(it.key(), it.value());
        QCOMPARE(*keyValueIt, pair);
        QCOMPARE(keyValueIt->first, pair.first);
        QCOMPARE(keyValueIt->second, pair.second);
        ++keyValueIt;
        ++it;
    }

    QVERIFY(keyValueIt == map.constKeyValueEnd());
    QVERIFY(it == map.cend());

    int key = 3;
    int value = 100 * 3;
    keyValueIt = std::find(map.constKeyValueBegin(), map.constKeyValueEnd(), EntryType(key, value));
    it = std::find(map.cbegin(), map.cend(), value);

    QVERIFY(keyValueIt != map.constKeyValueEnd());
    QCOMPARE(*keyValueIt, EntryType(it.key(), it.value()));

    ++it;
    ++keyValueIt;
    QCOMPARE(*keyValueIt, EntryType(it.key(), it.value()));

    --it;
    --keyValueIt;
    QCOMPARE(*keyValueIt, EntryType(it.key(), it.value()));

    std::advance(it, 5);
    std::advance(keyValueIt, 5);
    QCOMPARE(*keyValueIt, EntryType(it.key(), it.value()));

    std::advance(it, -5);
    std::advance(keyValueIt, -5);
    QCOMPARE(*keyValueIt, EntryType(it.key(), it.value()));

    key = 2;
    value = 100 * 2 + 2;
    auto cnt = std::count(map.constKeyValueBegin(), map.constKeyValueEnd(), EntryType(key, value));
    QCOMPARE(cnt, 1);
}

template <typename T>
void keyValueIteratorInEmptyMapTestMethod()
{
    T map;
    using ConstKeyValueIter = typename T::const_key_value_iterator;

    ConstKeyValueIter it1 = map.constKeyValueBegin();
    ConstKeyValueIter it2 = map.constKeyValueEnd();
    QVERIFY(it1 == it2 && it2 == ConstKeyValueIter());
    QVERIFY(!map.isDetached());

    const T map2;
    ConstKeyValueIter it3 = map2.keyValueBegin();
    ConstKeyValueIter it4 = map2.keyValueEnd();
    QVERIFY(it3 == it4 && it4 == ConstKeyValueIter());
    QVERIFY(!map2.isDetached());

    using KeyValueIter = typename T::key_value_iterator;

    KeyValueIter it5 = map.keyValueBegin();
    KeyValueIter it6 = map.keyValueEnd();
    QVERIFY(it5 == it6);
}

void tst_QMap::keyValueIteratorInEmptyMap()
{
    keyValueIteratorInEmptyMapTestMethod<QMap<int, int>>();
    if (QTest::currentTestFailed())
        return;

    keyValueIteratorInEmptyMapTestMethod<QMultiMap<int, int>>();
}

void tst_QMap::keys_values_uniqueKeys()
{
    {
        QMap<QString, int> map;
        QVERIFY(map.keys().isEmpty());
        QVERIFY(map.keys(1).isEmpty());
        QVERIFY(map.values().isEmpty());
        QVERIFY(!map.isDetached());

        map.insert("one", 1);
        QCOMPARE(map.keys(), QStringList({ "one" }));
        QCOMPARE(map.keys(1), QStringList({ "one" }));
        QCOMPARE(map.values(), QList<int>({ 1 }));

        map.insert("two", 2);
        QCOMPARE(map.keys(), QStringList({ "one", "two" }));
        QCOMPARE(map.keys(1), QStringList({ "one" }));
        QCOMPARE(map.values(), QList<int>({ 1, 2 }));

        map.insert("three", 2);
        QCOMPARE(map.keys(), QStringList({ "one", "three", "two" }));
        QCOMPARE(map.keys(2), QStringList({ "three", "two" }));
        QCOMPARE(map.values(), QList<int>({ 1, 2, 2 }));

        map.insert("one", 0);
        QCOMPARE(map.keys(), QStringList({ "one", "three", "two" }));
        QCOMPARE(map.keys(1), QStringList());
        QCOMPARE(map.keys(0), QStringList({ "one" }));
        QCOMPARE(map.keys(2), QStringList({ "three", "two" }));
        QCOMPARE(map.values(), QList<int>({ 0, 2, 2 }));
    }

    QMultiMap<QString, int> map;
    QVERIFY(map.keys().isEmpty());
    QVERIFY(map.keys(1).isEmpty());
    QVERIFY(map.uniqueKeys().isEmpty());
    QVERIFY(map.values().isEmpty());
    QVERIFY(map.values("key").isEmpty());
    QVERIFY(!map.isDetached());

    map.insert("alpha", 1);
    QVERIFY(map.keys() == (QList<QString>() << "alpha"));
    QVERIFY(map.values() == (QList<int>() << 1));
    QVERIFY(map.uniqueKeys() == QList<QString>({ "alpha" }));

    map.insert("beta", -2);
    QVERIFY(map.keys() == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(map.values() == (QList<int>() << 1 << -2));
    QVERIFY(map.uniqueKeys() == QList<QString>({ "alpha", "beta" }));

    map.insert("alpha", 2);
    QVERIFY(map.keys() == (QList<QString>() << "alpha" << "alpha" << "beta"));
    QVERIFY(map.values() == (QList<int>() << 2 << 1 << -2));
    QVERIFY(map.uniqueKeys() == QList<QString>({ "alpha", "beta" }));
    QVERIFY(map.values("alpha") == QList<int>({ 2, 1 }));

    map.insert("beta", 4);
    QVERIFY(map.keys() == (QList<QString>() << "alpha" << "alpha" << "beta" << "beta"));
    QVERIFY(map.values() == (QList<int>() << 2 << 1 << 4 << -2));
    QVERIFY(map.uniqueKeys() == QList<QString>({ "alpha", "beta" }));
    QVERIFY(map.values("alpha") == QList<int>({ 2, 1 }));
    QVERIFY(map.values("beta") == QList<int>({ 4, -2 }));

    map.insert("gamma", 2);
    QVERIFY(map.keys() == QList<QString>({ "alpha", "alpha", "beta", "beta", "gamma" }));
    QVERIFY(map.values() == QList<int>({ 2, 1, 4, -2, 2 }));
    QVERIFY(map.uniqueKeys() == QList<QString>({ "alpha", "beta", "gamma" }));
    QVERIFY(map.values("alpha") == QList<int>({ 2, 1 }));
    QVERIFY(map.values("beta") == QList<int>({ 4, -2 }));
    QVERIFY(map.values("gamma") == QList<int>({ 2 }));
    QVERIFY(map.keys(2) == QList<QString>({ "alpha", "gamma" }));
}

void tst_QMap::qmultimap_specific()
{
    QMultiMap<int, int> map1;
    for (int i = 1; i <= 9; ++i) {
        for (int j = 1; j <= i; ++j) {
            int k = i * 10 + j;
            QVERIFY(!map1.contains(i, k));
            map1.insert(i, k);
            QVERIFY(map1.contains(i, k));
        }
    }

    for (int i = 1; i <= 9; ++i) {
        for (int j = 1; j <= i; ++j) {
            int k = i * 10 + j;
            QVERIFY(map1.contains(i, k));
        }
    }

    QVERIFY(map1.contains(9, 99));
    QCOMPARE(map1.size(), 45);
    map1.remove(9, 99);
    QVERIFY(!map1.contains(9, 99));
    QCOMPARE(map1.size(), 44);

    map1.remove(9, 99);
    QVERIFY(!map1.contains(9, 99));
    QCOMPARE(map1.size(), 44);

    map1.remove(1, 99);
    QCOMPARE(map1.size(), 44);

    map1.insert(1, 99);
    map1.insert(1, 99);

    QCOMPARE(map1.size(), 46);
    map1.remove(1, 99);
    QCOMPARE(map1.size(), 44);
    map1.remove(1, 99);
    QCOMPARE(map1.size(), 44);

    {
    QMultiMap<int, int>::const_iterator i = map1.constFind(1, 11);
    QVERIFY(i.key() == 1);
    QVERIFY(i.value() == 11);

    i = map1.constFind(2, 22);
    QVERIFY(i.key() == 2);
    QVERIFY(i.value() == 22);

    i = map1.constFind(9, 98);
    QVERIFY(i.key() == 9);
    QVERIFY(i.value() == 98);
    }

    {
    const QMultiMap<int, int> map2(map1);
    QMultiMap<int, int>::const_iterator i = map2.find(1, 11);
    QVERIFY(i.key() == 1);
    QVERIFY(i.value() == 11);

    i = map2.find(2, 22);
    QVERIFY(i.key() == 2);
    QVERIFY(i.value() == 22);

    i = map2.find(9, 98);
    QVERIFY(i.key() == 9);
    QVERIFY(i.value() == 98);
    }

    {
    QMultiMap<int, int>::iterator i = map1.find(1, 11);
    QVERIFY(i.key() == 1);
    QVERIFY(i.value() == 11);

    i = map1.find(2, 22);
    QVERIFY(i.key() == 2);
    QVERIFY(i.value() == 22);

    i = map1.find(9, 98);
    QVERIFY(i.key() == 9);
    QVERIFY(i.value() == 98);
    }

    {
    QMultiMap<int, int> map1;
    map1.insert(42, 1);
    map1.insert(10, 2);
    map1.insert(48, 3);
    QMultiMap<int, int> map2;
    map2.insert(8, 4);
    map2.insert(42, 5);
    map2.insert(95, 12);

    map1+=map2;
    map2.insert(42, 1);
    map2.insert(10, 2);
    map2.insert(48, 3);
    QCOMPARE(map1.size(), map2.size());
    QVERIFY(map1.remove(42,5));
    QVERIFY(map2.remove(42,5));
    QVERIFY(map1 == map2);
    }

    map1.insert(map1.constBegin(), -1, -1);
    QCOMPARE(map1.size(), 45);
    map1.insert(map1.constBegin(), -1, -1);
    QCOMPARE(map1.size(), 46);
    map1.insert(map1.constBegin(), -2, -2);
    QCOMPARE(map1.size(), 47);
    map1.insert(map1.constBegin(), 5, 5); // Invald hint
    QCOMPARE(map1.size(), 48);
    map1.insert(map1.constBegin(), 5, 5); // Invald hint
    QCOMPARE(map1.size(), 49);
    sanityCheckTree(map1, __LINE__);
}

void tst_QMap::const_shared_null()
{
    QMap<int, QString> map2;
    QVERIFY(!map2.isDetached());
}

void tst_QMap::equal_range()
{
    {
        const QMap<int, QString> constMap;
        QCOMPARE(constMap.equal_range(1), qMakePair(constMap.constEnd(), constMap.constEnd()));
        QVERIFY(!constMap.isDetached());

        QMap<int, QString> map;
        QCOMPARE(map.equal_range(1), qMakePair(map.end(), map.end()));

        map.insert(1, "value1");
        map.insert(5, "value5");
        map.insert(1, "value0");

        auto pair = map.equal_range(1);
        QCOMPARE(pair.first.value(), "value0");
        QCOMPARE(pair.second.value(), "value5");
        auto b = map.find(1);
        auto e = map.find(5);
        QCOMPARE(pair, qMakePair(b, e));

        pair = map.equal_range(3);
        QCOMPARE(pair.first.value(), "value5");
        QCOMPARE(pair.second.value(), "value5");
        QCOMPARE(pair, qMakePair(e, e));

        QCOMPARE(map.equal_range(10), qMakePair(map.end(), map.end()));
    }

    const QMultiMap<int, QString> constMap;
    QCOMPARE(constMap.equal_range(1), qMakePair(constMap.constEnd(), constMap.constEnd()));
    QVERIFY(!constMap.isDetached());

    QMultiMap<int, QString> map;
    const QMultiMap<int, QString> &cmap = map;

    QPair<QMultiMap<int, QString>::iterator, QMultiMap<int, QString>::iterator> result = map.equal_range(0);
    QCOMPARE(result.first, map.end());
    QCOMPARE(result.second, map.end());

    QPair<QMultiMap<int, QString>::const_iterator, QMultiMap<int, QString>::const_iterator> cresult = cmap.equal_range(0);
    QCOMPARE(cresult.first, cmap.cend());
    QCOMPARE(cresult.second, cmap.cend());

    map.insert(1, "one");

    result = map.equal_range(0);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.find(1));

    result = map.equal_range(1);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.end());

    result = map.equal_range(2);
    QCOMPARE(result.first, map.end());
    QCOMPARE(result.second, map.end());

    cresult = cmap.equal_range(0);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.find(1));

    cresult = cmap.equal_range(1);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.cend());

    cresult = cmap.equal_range(2);
    QCOMPARE(cresult.first, cmap.cend());
    QCOMPARE(cresult.second, cmap.cend());

    for (int i = -10; i < 10; i += 2)
        map.insert(i, QString::number(i));

    result = map.equal_range(0);
    QCOMPARE(result.first, map.find(0));
    QCOMPARE(result.second, map.find(1));

    result = map.equal_range(1);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.find(2));

    result = map.equal_range(2);
    QCOMPARE(result.first, map.find(2));
    QCOMPARE(result.second, map.find(4));

    cresult = cmap.equal_range(0);
    QCOMPARE(cresult.first, cmap.find(0));
    QCOMPARE(cresult.second, cmap.find(1));

    cresult = cmap.equal_range(1);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.find(2));

    cresult = cmap.equal_range(2);
    QCOMPARE(cresult.first, cmap.find(2));
    QCOMPARE(cresult.second, cmap.find(4));

    map.insert(1, "another one");

    result = map.equal_range(1);
    QCOMPARE(result.first, map.find(1));
    QCOMPARE(result.second, map.find(2));

    cresult = cmap.equal_range(1);
    QCOMPARE(cresult.first, cmap.find(1));
    QCOMPARE(cresult.second, cmap.find(2));

    QCOMPARE(map.count(1), 2);
}

void tst_QMap::insert()
{
    QMap<QString, float> map;
    map.insert("cs/key1", 1);
    map.insert("cs/key2", 2);
    map.insert("cs/key1", 3);
    QCOMPARE(map.size(), 2);

    QMap<int, int> intMap;
    for (int i = 0; i < 1000; ++i) {
        intMap.insert(i, i);
    }

    QCOMPARE(intMap.size(), 1000);

    for (int i = 0; i < 1000; ++i) {
        QCOMPARE(intMap.value(i), i);
        intMap.insert(i, -1);
        QCOMPARE(intMap.size(), 1000);
        QCOMPARE(intMap.value(i), -1);
    }

    {
        QMap<IdentityTracker, int> map;
        QCOMPARE(map.size(), 0);
        const int dummy = -1;
        IdentityTracker id00 = {0, 0}, id01 = {0, 1}, searchKey = {0, dummy};
        QCOMPARE(map.insert(id00, id00.id).key().id, id00.id);
        QCOMPARE(map.size(), 1);
        QCOMPARE(map.insert(id01, id01.id).key().id, id00.id); // first key inserted is kept
        QCOMPARE(map.size(), 1);
        QCOMPARE(map.find(searchKey).value(), id01.id);  // last-inserted value
        QCOMPARE(map.find(searchKey).key().id, id00.id); // but first-inserted key
    }
    {
        QMultiMap<IdentityTracker, int> map;
        QCOMPARE(map.size(), 0);
        const int dummy = -1;
        IdentityTracker id00 = {0, 0}, id01 = {0, 1}, searchKey = {0, dummy};
        QCOMPARE(map.insert(id00, id00.id).key().id, id00.id);
        QCOMPARE(map.size(), 1);
        QCOMPARE(map.insert(id01, id01.id).key().id, id01.id);
        QCOMPARE(map.size(), 2);
        QMultiMap<IdentityTracker, int>::const_iterator pos = map.constFind(searchKey);
        QCOMPARE(pos.value(), pos.key().id); // key fits to value it was inserted with
        ++pos;
        QCOMPARE(pos.value(), pos.key().id); // key fits to value it was inserted with
    }
}

template <template <typename K, typename T> typename Map>
void testDetachWhenInsert()
{
    const Map<int, int> referenceSource = {
        { 0, 0 },
        { 1, 1 },
        { 2, 2 }
    };

    const Map<int, int> referenceDestination = {
        { 0, 0 },
        { 1, 1 },
        { 2, 2 },
        { 3, 3 }
    };

    // copy insertion of non-shared map
    {
        Map<int, int> source;
        source.insert(0, 0);
        source.insert(1, 1);
        source.insert(2, 2);

        Map<int, int> dest;
        dest.insert(3, 3);
        Map<int, int> destCopy = dest;

        if constexpr (std::is_same_v<decltype(dest), QMap<int, int>>)
            dest.insert(source); // QMap
        else
            dest.unite(source); // QMultiMap

        QCOMPARE(source, referenceSource);
        QCOMPARE(dest, referenceDestination);

        QCOMPARE(destCopy.size(), 1); // unchanged
    }

    // copy insertion of shared map
    {
        Map<int, int> source;
        source.insert(0, 0);
        source.insert(1, 1);
        source.insert(2, 2);
        Map<int, int> sourceCopy = source;

        Map<int, int> dest;
        dest.insert(3, 3);
        Map<int, int> destCopy = dest;

        if constexpr (std::is_same_v<decltype(dest), QMap<int, int>>)
            dest.insert(source); // QMap
        else
            dest.unite(source); // QMultiMap

        QCOMPARE(source, referenceSource);
        QCOMPARE(sourceCopy, referenceSource);

        QCOMPARE(dest, referenceDestination);
        QCOMPARE(destCopy.size(), 1); // unchanged
    }

    // move insertion of non-shared map
    {
        Map<int, int> source;
        source.insert(0, 0);
        source.insert(1, 1);
        source.insert(2, 2);

        Map<int, int> dest;
        dest.insert(3, 3);
        Map<int, int> destCopy = dest;

        if constexpr (std::is_same_v<decltype(dest), QMap<int, int>>)
            dest.insert(source); // QMap
        else
            dest.unite(source); // QMultiMap

        QCOMPARE(dest, referenceDestination);
        QCOMPARE(destCopy.size(), 1); // unchanged
    }

    // move insertion of shared map
    {
        Map<int, int> source;
        source.insert(0, 0);
        source.insert(1, 1);
        source.insert(2, 2);
        Map<int, int> sourceCopy = source;

        Map<int, int> dest;
        dest.insert(3, 3);
        Map<int, int> destCopy = dest;

        if constexpr (std::is_same_v<decltype(dest), QMap<int, int>>)
            dest.insert(std::move(source)); // QMap
        else
            dest.unite(std::move(source)); // QMultiMap

        QCOMPARE(sourceCopy, referenceSource);

        QCOMPARE(dest, referenceDestination);
        QCOMPARE(destCopy.size(), 1); // unchanged
    }
};

void tst_QMap::insertMap()
{
    {
        QMap<int, int> map1;
        QMap<int, int> map2;
        QVERIFY(map1.isEmpty());
        QVERIFY(map2.isEmpty());

        map1.insert(map2);
        QVERIFY(map1.isEmpty());
        QVERIFY(map2.isEmpty());
        QVERIFY(!map1.isDetached());
        QVERIFY(!map2.isDetached());
    }
    {
        QMap<int, int> map;
        map.insert(1, 1);
        map.insert(2, 2);
        map.insert(0, -1);

        QMap<int, int> map2;
        map2.insert(0, 0);
        map2.insert(3, 3);
        map2.insert(4, 4);

        map.insert(map2);

        QCOMPARE(map.size(), 5);
        for (int i = 0; i < 5; ++i)
            QCOMPARE(map[i], i);
    }
    {
        QMap<int, int> map;
        for (int i = 0; i < 10; ++i)
            map.insert(i * 3, i);

        QMap<int, int> map2;
        for (int i = 0; i < 10; ++i)
            map2.insert(i * 4, i);

        map.insert(map2);

        QCOMPARE(map.size(), 17);
        for (int i = 0; i < 10; ++i) {
            // i * 3 == i except for i = 4, 8
            QCOMPARE(map[i * 3], (i && i % 4 == 0) ? i - (i / 4) : i);
            QCOMPARE(map[i * 4], i);
        }

        auto it = map.cbegin();
        int prev = it.key();
        ++it;
        for (auto end = map.cend(); it != end; ++it) {
            QVERIFY(prev < it.key());
            prev = it.key();
        }
    }
    {
        QMap<int, int> map;
        map.insert(1, 1);

        QMap<int, int> map2;

        map.insert(map2);
        QCOMPARE(map.size(), 1);
        QCOMPARE(map[1], 1);
    }
    {
        QMap<int, int> map;
        QMap<int, int> map2;
        map2.insert(1, 1);

        map.insert(map2);
        QCOMPARE(map.size(), 1);
        QCOMPARE(map[1], 1);

        QMap<int, int> map3;
        map3.insert(std::move(map2));
        QCOMPARE(map3, map);
    }
    {
        QMap<int, int> map;
        map.insert(0, 0);
        map.insert(1, 1);
        map.insert(2, 2);

        // Test inserting into self, nothing should happen
        map.insert(map);

        QCOMPARE(map.size(), 3);
        for (int i = 0; i < 3; ++i)
            QCOMPARE(map[i], i);
    }
    {
        // Here we use a QMultiMap and insert that into QMap,
        // since it has multiple values with the same key the
        // ordering is undefined so we won't test that, but
        // make sure this isn't adding multiple entries with the
        // same key to the QMap.
        QMap<int, int> map;
        map.insert(0, 0);

        QMap<int, int> map2;
        map2.insert(0, 1);

        map.insert(map2);

        QCOMPARE(map.size(), 1);
    }

    testDetachWhenInsert<QMap>();
    testDetachWhenInsert<QMultiMap>();
}

void tst_QMap::checkMostLeftNode()
{
    QMap<int, int> map;

    map.insert(100, 1);
    sanityCheckTree(map, __LINE__);

    // insert
    map.insert(99, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(98, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(97, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(96, 1);
    sanityCheckTree(map, __LINE__);
    map.insert(95, 1);

    // remove
    sanityCheckTree(map, __LINE__);
    map.take(95);
    sanityCheckTree(map, __LINE__);
    map.remove(96);
    sanityCheckTree(map, __LINE__);
    map.erase(map.begin());
    sanityCheckTree(map, __LINE__);
    map.remove(97);
    sanityCheckTree(map, __LINE__);
    map.remove(98);
    sanityCheckTree(map, __LINE__);
    map.remove(99);
    sanityCheckTree(map, __LINE__);
    map.remove(100);
    sanityCheckTree(map, __LINE__);
    map.insert(200, 1);
    QCOMPARE(map.constBegin().key(), 200);
    sanityCheckTree(map, __LINE__);
    // remove the non left most node
    map.insert(202, 2);
    map.insert(203, 3);
    map.insert(204, 4);
    map.remove(202);
    sanityCheckTree(map, __LINE__);
    map.remove(203);
    sanityCheckTree(map, __LINE__);
    map.remove(204);
    sanityCheckTree(map, __LINE__);
    // erase last item
    map.erase(map.begin());
    sanityCheckTree(map, __LINE__);
}

void tst_QMap::initializerList()
{
    QMap<int, QString> map = {{1, "bar"}, {1, "hello"}, {2, "initializer_list"}};
    QCOMPARE(map.size(), 2);
    QCOMPARE(map[1], QString("hello"));
    QCOMPARE(map[2], QString("initializer_list"));

    // note the difference to std::map:
    // std::map<int, QString> stdm = {{1, "bar"}, {1, "hello"}, {2, "initializer_list"}};
    // QCOMPARE(stdm.size(), 2UL);
    // QCOMPARE(stdm[1], QString("bar"));

    QMultiMap<QString, int> multiMap{{"il", 1}, {"il", 2}, {"il", 3}};
    QCOMPARE(multiMap.size(), 3);
    QList<int> values = multiMap.values("il");
    QCOMPARE(values.size(), 3);

    QMap<int, int> emptyMap{};
    QVERIFY(emptyMap.isEmpty());

    QMap<char, char> emptyPairs{{}, {}};
    QVERIFY(!emptyPairs.isEmpty());

    QMultiMap<double, double> emptyMultiMap{};
    QVERIFY(emptyMultiMap.isEmpty());

    QMultiMap<float, float> emptyPairs2{{}, {}};
    QVERIFY(!emptyPairs2.isEmpty());
}

void tst_QMap::testInsertWithHint()
{
    QMap<int, int> map;

    // Check with end hint();
    map.insert(map.constEnd(), 3, 1);     // size == 1
    sanityCheckTree(map, __LINE__);
    map.insert(map.constEnd(), 5, 1);     // size = 2
    sanityCheckTree(map, __LINE__);
    map.insert(map.constEnd(), 50, 1);    // size = 3
    sanityCheckTree(map, __LINE__);
    QMap<int, int>::const_iterator key75(map.insert(map.constEnd(), 75, 1)); // size = 4
    sanityCheckTree(map, __LINE__);
    map.insert(map.constEnd(), 100, 1);   // size = 5
    sanityCheckTree(map, __LINE__);
    map.insert(map.constEnd(), 105, 1);   // size = 6
    sanityCheckTree(map, __LINE__);
    map.insert(map.constEnd(), 10, 5); // invalid hint and size = 7
    sanityCheckTree(map, __LINE__);
    QMap<int, int>::iterator lastkey = map.insert(map.constEnd(), 105, 12); // overwrite
    sanityCheckTree(map, __LINE__);
    QCOMPARE(lastkey.value(), 12);
    QCOMPARE(lastkey.key(), 105);
    QCOMPARE(map.size(), 7);

    // With regular hint
    map.insert(key75, 75, 100); // overwrite current key
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 7);
    QCOMPARE(key75.key(), 75);
    QCOMPARE(key75.value(), 100);

    map.insert(key75, 50, 101); // overwrite previous value
    QMap<int, int>::const_iterator key50(key75);
    --key50;
    QCOMPARE(map.size(), 7);
    QCOMPARE(key50.key(), 50);
    QCOMPARE(key50.value(), 101);

    map.insert(key75, 17, 125);     // invalid hint - size 8
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 8);

    // begin
    map.insert(map.constBegin(), 1, 1);  // size 9
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 9);

    map.insert(map.constBegin(), 1, 10);  // overwrite existing (leftmost) value
    QCOMPARE(map.constBegin().value(), 10);

    map.insert(map.constBegin(), 47, 47); // wrong hint  - size 10
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 10);

    // insert with right == 0
    QMap<int, int>::const_iterator i1 (map.insert(key75, 70, 12)); // overwrite
    map.insert(i1, 69, 12);  // size 12

    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 12);
}

void tst_QMap::testInsertMultiWithHint()
{
    QMultiMap<int, int> map;

    map.insert(map.end(), 64, 65);
    map.insert(128, 129);
    map.insert(256, 257);
    sanityCheckTree(map, __LINE__);

    map.insert(map.end(), 512, 513);
    map.insert(map.end(), 512, 513 * 2);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 5);
    map.insert(map.end(), 256, 258); // wrong hint
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 6);

    QMultiMap<int, int>::iterator i = map.insert(map.constBegin(), 256, 259); // wrong hint
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 7);

    QMultiMap<int, int>::iterator j = map.insert(map.constBegin(), 69, 66);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 8);

    j = map.insert(j, 68, 259);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 9);

    j = map.insert(j, 67, 67);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 10);

    i = map.insert(i, 256, 259);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 11);

    i = map.insert(i, 256, 260);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 12);

    map.insert(i, 64, 67);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 13);

    map.insert(map.constBegin(), 20, 20);
    sanityCheckTree(map, __LINE__);
    QCOMPARE(map.size(), 14);
}

void tst_QMap::eraseValidIteratorOnSharedMap()
{
    QMultiMap<int, int> a, b;
    a.insert(10, 10);
    a.insert(10, 40);
    a.insert(10, 25);
    a.insert(10, 30);
    a.insert(20, 20);

    QMultiMap<int, int>::iterator i = a.begin();
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

    QCOMPARE(itemsWith10, 4);

    // Border cases
    QMultiMap <QString, QString> ms1, ms2, ms3;
    ms1.insert("foo", "bar");
    ms1.insert("foo", "quux");
    ms1.insert("foo", "bar");

    QMultiMap <QString, QString>::iterator si = ms1.begin();
    ms2 = ms1;
    ms1.erase(si);
    si = ms1.begin();
    QCOMPARE(si.value(), QString("quux"));
    ++si;
    QCOMPARE(si.value(), QString("bar"));

    si = ms2.begin();
    ++si;
    ++si;
    ms3 = ms2;
    ms2.erase(si);
    si = ms2.begin();
    QCOMPARE(si.value(), QString("bar"));
    ++si;
    QCOMPARE(si.value(), QString("quux"));

    QCOMPARE(ms1.size(), 2);
    QCOMPARE(ms2.size(), 2);
    QCOMPARE(ms3.size(), 3);
}

void tst_QMap::removeElementsInMap()
{
    // A class that causes an almost certain crash if its operator< is
    // called on a destroyed object
    struct SharedInt {
        QSharedPointer<int> m_int;
        explicit SharedInt(int i) : m_int(QSharedPointer<int>::create(i)) {}
        bool operator<(const SharedInt &other) const { return *m_int < *other.m_int; }
    };

    {
        QMap<int, int> map;
        QCOMPARE(map.remove(1), 0);
        QVERIFY(!map.isDetached());

        auto cnt = map.removeIf([](QMap<int, int>::iterator) { return true; });
        QCOMPARE(cnt, 0);
    }
    {
        QMap<SharedInt, int> map {
            { SharedInt(1), 1 },
            { SharedInt(2), 2 },
            { SharedInt(3), 3 },
            { SharedInt(4), 4 },
            { SharedInt(5), 5 },
        };
        QCOMPARE(map.size(), 5);

        map.remove(SharedInt(1));
        QCOMPARE(map.size(), 4);

        map.remove(SharedInt(-1));
        QCOMPARE(map.size(), 4);

        QMap<SharedInt, int> map2 = map;
        QCOMPARE(map.size(), 4);
        QCOMPARE(map2.size(), 4);

        map.remove(SharedInt(3));
        QCOMPARE(map.size(), 3);
        QCOMPARE(map2.size(), 4);

        map.remove(SharedInt(-1));
        QCOMPARE(map.size(), 3);
        QCOMPARE(map2.size(), 4);

        map = map2;
        QCOMPARE(map.size(), 4);
        QCOMPARE(map2.size(), 4);

        map.remove(SharedInt(-1));
        QCOMPARE(map.size(), 4);
        QCOMPARE(map2.size(), 4);

        map.remove(map.firstKey());
        QCOMPARE(map.size(), 3);
        QCOMPARE(map2.size(), 4);

        map.remove(map.lastKey());
        QCOMPARE(map.size(), 2);
        QCOMPARE(map2.size(), 4);

        map = map2;
        QCOMPARE(map.size(), 4);
        QCOMPARE(map2.size(), 4);

        auto size = map.size();
        for (auto it = map.begin(); it != map.end(); ) {
            const auto oldIt = it++;
            size -= map.remove(oldIt.key());
            QCOMPARE(map.size(), size);
            QCOMPARE(map2.size(), 4);
        }

        QCOMPARE(map.size(), 0);
        QCOMPARE(map2.size(), 4);

        auto cnt = map2.removeIf([](auto it) { return (*it % 2) == 0; });
        QCOMPARE(cnt, 2);
        QCOMPARE(map2.size(), 2);
    }

    {
        QMultiMap<int, int> map;
        QCOMPARE(map.remove(1), 0);
        QVERIFY(!map.isDetached());

        auto cnt = map.removeIf([](QMultiMap<int, int>::iterator) { return true; });
        QCOMPARE(cnt, 0);
    }
    {
        QMultiMap<SharedInt, int> multimap {
            { SharedInt(1), 10 },
            { SharedInt(1), 11 },
            { SharedInt(2), 2 },
            { SharedInt(3), 30 },
            { SharedInt(3), 31 },
            { SharedInt(3), 32 },
            { SharedInt(4), 4 },
            { SharedInt(5), 5 },
            { SharedInt(6), 60 },
            { SharedInt(6), 61 },
            { SharedInt(6), 60 },
            { SharedInt(6), 62 },
            { SharedInt(6), 60 },
            { SharedInt(7), 7 },
        };

        QCOMPARE(multimap.size(), 14);

        multimap.remove(SharedInt(1));
        QCOMPARE(multimap.size(), 12);

        multimap.remove(SharedInt(-1));
        QCOMPARE(multimap.size(), 12);

        QMultiMap<SharedInt, int> multimap2 = multimap;
        QCOMPARE(multimap.size(), 12);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(SharedInt(3));
        QCOMPARE(multimap.size(), 9);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(SharedInt(4));
        QCOMPARE(multimap.size(), 8);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(SharedInt(-1));
        QCOMPARE(multimap.size(), 8);
        QCOMPARE(multimap2.size(), 12);

        multimap = multimap2;
        QCOMPARE(multimap.size(), 12);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(SharedInt(-1));
        QCOMPARE(multimap.size(), 12);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(SharedInt(6), 60);
        QCOMPARE(multimap.size(), 9);
        QCOMPARE(multimap2.size(), 12);

        multimap = multimap2;
        QCOMPARE(multimap.size(), 12);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(SharedInt(6), 62);
        QCOMPARE(multimap.size(), 11);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(multimap.firstKey());
        QCOMPARE(multimap.size(), 10);
        QCOMPARE(multimap2.size(), 12);

        multimap.remove(multimap.lastKey());
        QCOMPARE(multimap.size(), 9);
        QCOMPARE(multimap2.size(), 12);

        multimap = multimap2;
        QCOMPARE(multimap.size(), 12);
        QCOMPARE(multimap2.size(), 12);

        auto itFor6 = multimap.find(SharedInt(6));
        QVERIFY(itFor6 != multimap.end());
        QCOMPARE(itFor6.value(), 60);
        multimap.remove(itFor6.key(), itFor6.value());
        QCOMPARE(multimap.size(), 9);
        QCOMPARE(multimap2.size(), 12);

        multimap = multimap2;
        QCOMPARE(multimap.size(), 12);
        QCOMPARE(multimap2.size(), 12);

        auto size = multimap.size();
        for (auto it = multimap.begin(); it != multimap.end();) {
            const auto range = multimap.equal_range(it.key());
            const auto oldIt = it;
            it = range.second;
            size -= multimap.remove(oldIt.key());
            QCOMPARE(multimap.size(), size);
            QCOMPARE(multimap2.size(), 12);
        }

        QCOMPARE(multimap.size(), 0);
        QCOMPARE(multimap2.size(), 12);

        auto cnt = multimap2.removeIf([](auto it) { return (*it % 2) == 0; });
        QCOMPARE(cnt, 8);
        QCOMPARE(multimap2.size(), 4);
    }
}

template <typename QtMap, typename StdMap>
void toStdMapTestMethod(const StdMap &expectedMap)
{
    QtMap map;
    QVERIFY(map.isEmpty());
    auto stdMap = map.toStdMap();
    QVERIFY(stdMap.empty());
    QVERIFY(!map.isDetached());

    map.insert(1, "value1");
    map.insert(2, "value2");
    map.insert(3, "value3");
    map.insert(1, "value0");

    stdMap = map.toStdMap();
    QCOMPARE(stdMap, expectedMap);
}

void tst_QMap::toStdMap()
{
    const std::map<int, QString> expectedMap { {1, "value0"}, {2, "value2"}, {3, "value3"} };
    toStdMapTestMethod<QMap<int, QString>>(expectedMap);
    if (QTest::currentTestFailed())
        return;

    const std::multimap<int, QString> expectedMultiMap {
        {1, "value0"}, {1, "value1"}, {2, "value2"}, {3, "value3"} };
    toStdMapTestMethod<QMultiMap<int, QString>>(expectedMultiMap);
}

#if QT_DEPRECATED_SINCE(6, 0)
void tst_QMap::deprecatedInsertMulti()
{
    QMultiMap<int, QString> referenceMap;
    referenceMap.insert(1, "value1");
    referenceMap.insert(2, "value2");
    referenceMap.insert(3, "value3");
    referenceMap.insert(1, "value1_2");
    referenceMap.insert(referenceMap.find(2), 2, "value2_2");
    referenceMap.insert(referenceMap.end(), 1, "value1_3");

    QMultiMap<int, QString> deprecatedMap;
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    deprecatedMap.insertMulti(1, "value1");
    deprecatedMap.insertMulti(2, "value2");
    deprecatedMap.insertMulti(3, "value3");
    deprecatedMap.insertMulti(1, "value1_2");
    deprecatedMap.insertMulti(deprecatedMap.find(2), 2, "value2_2");
    deprecatedMap.insertMulti(deprecatedMap.end(), 1, "value1_3");
QT_WARNING_POP

    QCOMPARE(deprecatedMap, referenceMap);
}

void tst_QMap::deprecatedIteratorApis()
{
    QMap<int, QString> map;
    QString testString = "Teststring %1";
    for (int i = 1; i < 100; ++i)
        map.insert(i, testString.arg(i));

    auto it = map.begin();
    QCOMPARE(it.value(), QLatin1String("Teststring 1"));
    QT_IGNORE_DEPRECATIONS(it += 5;)
    QCOMPARE(it.value(), QLatin1String("Teststring 6"));
    QT_IGNORE_DEPRECATIONS(it = it - 3;)
    QCOMPARE(it.value(), QLatin1String("Teststring 3"));

    auto cit = map.constBegin();
    QCOMPARE(cit.value(), QLatin1String("Teststring 1"));
    QT_IGNORE_DEPRECATIONS(cit += 5;)
    QCOMPARE(cit.value(), QLatin1String("Teststring 6"));
    QT_IGNORE_DEPRECATIONS(cit = cit - 3;)
    QCOMPARE(cit.value(), QLatin1String("Teststring 3"));
}

void tst_QMap::deprecatedInsert()
{
    QMultiMap<int, QString> refMap;
    refMap.insert(1, "value1");
    refMap.insert(2, "value2");
    refMap.insert(3, "value3");

    QMultiMap<int, QString> depMap = refMap;

    QMultiMap<int, QString> otherMap;
    otherMap.insert(1, "value1_2");
    otherMap.insert(3, "value3_2");
    otherMap.insert(4, "value4");

    refMap.unite(otherMap);
    QT_IGNORE_DEPRECATIONS(depMap.insert(otherMap);)

    QCOMPARE(refMap, depMap);
}
#endif // QT_DEPRECATED_SINCE(6, 0)

QTEST_APPLESS_MAIN(tst_QMap)
#include "tst_qmap.moc"
