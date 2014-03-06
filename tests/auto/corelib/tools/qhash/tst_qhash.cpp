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

#include <qhash.h>
#include <qmap.h>

#include <algorithm>

class tst_QHash : public QObject
{
    Q_OBJECT
private slots:
    void insert1();
    void erase();
    void key();

    void swap();
    void count(); // copied from tst_QMap
    void clear(); // copied from tst_QMap
    void empty(); // copied from tst_QMap
    void find(); // copied from tst_QMap
    void constFind(); // copied from tst_QMap
    void contains(); // copied from tst_QMap
    void take(); // copied from tst_QMap
    void operator_eq(); // copied from tst_QMap
    void rehash_isnt_quadratic();
    void dont_need_default_constructor();
    void qhash();
    void fp_qhash_of_zero_is_zero();
    void qmultihash_specific();

    void compare();
    void compare2();
    void iterators(); // sligthly modified from tst_QMap
    void keys_values_uniqueKeys(); // slightly modified from tst_QMap
    void noNeedlessRehashes();

    void const_shared_null();
    void twoArguments_qHash();
    void initializerList();

    void qthash_data();
    void qthash();
    void eraseValidIteratorOnSharedHash();
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

//copied from tst_QMap.cpp
class MyClass
{
public:
    MyClass() { ++count;
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

typedef QHash<QString, MyClass> MyMap;

//void tst_QMap::count()
void tst_QHash::count()
{
    {
        MyMap map;
        MyMap map2( map );
        QCOMPARE( map.count(), 0 );
        QCOMPARE( map2.count(), 0 );
        QCOMPARE( MyClass::count, 0 );
        // detach
        map2["Hallo"] = MyClass( "Fritz" );
        QCOMPARE( map.count(), 0 );
        QCOMPARE( map2.count(), 1 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 1 );
#endif
    }
    QCOMPARE( MyClass::count, 0 );

    {
        typedef QHash<QString, MyClass> Map;
        Map map;
        QCOMPARE( map.count(), 0);
        map.insert( "Torben", MyClass("Weis") );
        QCOMPARE( map.count(), 1 );
        map.insert( "Claudia", MyClass("Sorg") );
        QCOMPARE( map.count(), 2 );
        map.insert( "Lars", MyClass("Linzbach") );
        map.insert( "Matthias", MyClass("Ettrich") );
        map.insert( "Sue", MyClass("Paludo") );
        map.insert( "Eirik", MyClass("Eng") );
        map.insert( "Haavard", MyClass("Nord") );
        map.insert( "Arnt", MyClass("Gulbrandsen") );
        map.insert( "Paul", MyClass("Tvete") );
        QCOMPARE( map.count(), 9 );
        map.insert( "Paul", MyClass("Tvete 1") );
        map.insert( "Paul", MyClass("Tvete 2") );
        map.insert( "Paul", MyClass("Tvete 3") );
        map.insert( "Paul", MyClass("Tvete 4") );
        map.insert( "Paul", MyClass("Tvete 5") );
        map.insert( "Paul", MyClass("Tvete 6") );

        QCOMPARE( map.count(), 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        Map map2( map );
        QVERIFY( map2.count() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2.insert( "Kay", MyClass("Roemer") );
        QVERIFY( map2.count() == 10 );
        QVERIFY( map.count() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 19 );
#endif

        map2 = map;
        QVERIFY( map.count() == 9 );
        QVERIFY( map2.count() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2.insert( "Kay", MyClass("Roemer") );
        QVERIFY( map2.count() == 10 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 19 );
#endif

        map2.clear();
        QVERIFY( map.count() == 9 );
        QVERIFY( map2.count() == 0 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2 = map;
        QVERIFY( map.count() == 9 );
        QVERIFY( map2.count() == 9 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map2.clear();
        QVERIFY( map.count() == 9 );
        QVERIFY( map2.count() == 0 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 9 );
#endif

        map.remove( "Lars" );
        QVERIFY( map.count() == 8 );
        QVERIFY( map2.count() == 0 );
#ifndef Q_CC_SUN
        QCOMPARE( MyClass::count, 8 );
#endif

        map.remove( "Mist" );
        QVERIFY( map.count() == 8 );
        QVERIFY( map2.count() == 0 );
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
         QVERIFY( map.count() == 1 );

         (void)map["Torben"].str;
         (void)map["Lars"].str;
#ifndef Q_CC_SUN
         QVERIFY( MyClass::count == 2 );
#endif
         QVERIFY( map.count() == 2 );

         const Map& cmap = map;
         (void)cmap["Depp"].str;
#ifndef Q_CC_SUN
         QVERIFY( MyClass::count == 2 );
#endif
         QVERIFY( map.count() == 2 );
         QVERIFY( cmap.count() == 2 );
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
        QString key;
        for (int i = 0; i < 10; ++i) {
            key[0] = i + '0';
            for (int j = 0; j < 10; ++j) {
                key[1] = j + '0';
                hash.insert(key, "V" + key);
            }
        }

        for (int i = 0; i < 10; ++i) {
            key[0] = i + '0';
            for (int j = 0; j < 10; ++j) {
                key[1] = j + '0';
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
    QHash<int, int> h2;
    h2.insertMulti(20, 41);
    h2.insertMulti(20, 42);
    QVERIFY(h2.size() == 2);
    it1 = h2.erase(h2.begin());
    it1 = h2.erase(h2.begin());
    QVERIFY(it1 == h2.end());
}

void tst_QHash::key()
{
    {
        QString def("default value");

        QHash<QString, int> hash1;
        QCOMPARE(hash1.key(1), QString());
        QCOMPARE(hash1.key(1, def), def);

        hash1.insert("one", 1);
        QCOMPARE(hash1.key(1), QString("one"));
        QCOMPARE(hash1.key(1, def), QString("one"));
        QCOMPARE(hash1.key(2), QString());
        QCOMPARE(hash1.key(2, def), def);

        hash1.insert("two", 2);
        QCOMPARE(hash1.key(1), QString("one"));
        QCOMPARE(hash1.key(1, def), QString("one"));
        QCOMPARE(hash1.key(2), QString("two"));
        QCOMPARE(hash1.key(2, def), QString("two"));
        QCOMPARE(hash1.key(3), QString());
        QCOMPARE(hash1.key(3, def), def);

        hash1.insert("deux", 2);
        QCOMPARE(hash1.key(1), QString("one"));
        QCOMPARE(hash1.key(1, def), QString("one"));
        QVERIFY(hash1.key(2) == "deux" || hash1.key(2) == "two");
        QVERIFY(hash1.key(2, def) == "deux" || hash1.key(2, def) == "two");
        QCOMPARE(hash1.key(3), QString());
        QCOMPARE(hash1.key(3, def), def);
    }

    {
        int def = 666;

        QHash<int, QString> hash2;
        QCOMPARE(hash2.key("one"), 0);
        QCOMPARE(hash2.key("one", def), def);

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
}
//copied from tst_QMap
void tst_QHash::empty()
{
    QHash<int, QString> map1;

    QVERIFY(map1.isEmpty());

    map1.insert(1, "one");
    QVERIFY(!map1.isEmpty());

    map1.clear();
    QVERIFY(map1.isEmpty());

}

//copied from tst_QMap
void tst_QHash::find()
{
    QHash<int, QString> map1;
    QString testString="Teststring %0";
    QString compareString;
    int i,count=0;

    QVERIFY(map1.find(1) == map1.end());

    map1.insert(1,"Mensch");
    map1.insert(1,"Mayer");
    map1.insert(2,"Hej");

    QVERIFY(map1.find(1).value() == "Mayer");
    QVERIFY(map1.find(2).value() == "Hej");

    for(i = 3; i < 10; ++i) {
        compareString = testString.arg(i);
        map1.insertMulti(4, compareString);
    }

    QHash<int, QString>::const_iterator it=map1.constFind(4);

    for(i = 9; i > 2 && it != map1.constEnd() && it.key() == 4; --i) {
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

    map1.insert(1,"Mensch");
    map1.insert(1,"Mayer");
    map1.insert(2,"Hej");

    QVERIFY(map1.constFind(1).value() == "Mayer");
    QVERIFY(map1.constFind(2).value() == "Hej");

    for(i = 3; i < 10; ++i) {
        compareString = testString.arg(i);
        map1.insertMulti(4, compareString);
    }

    QHash<int, QString>::const_iterator it=map1.constFind(4);

    for(i = 9; i > 2 && it != map1.constEnd() && it.key() == 4; --i) {
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

    map1.insert(1, "one");
    QVERIFY(map1.contains(1));

    for(i=2; i < 100; ++i)
        map1.insert(i, "teststring");
    for(i=99; i > 1; --i)
        QVERIFY(map1.contains(i));

    map1.remove(43);
    QVERIFY(!map1.contains(43));
}

//copied from tst_QMap
void tst_QHash::take()
{
    QHash<int, QString> map;

    map.insert(2, "zwei");
    map.insert(3, "drei");

    QVERIFY(map.take(3) == "drei");
    QVERIFY(!map.contains(3));
}

//copied from tst_QMap
void tst_QHash::operator_eq()
{
    {
        // compare for equality:
        QHash<int, int> a;
        QHash<int, int> b;

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
        QHash<QString, QString> a;
        QHash<QString, QString> b;

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
        QHash<QString, int> a;
        QHash<QString, int> b;

        a.insert("otto", 1);
        b.insert("willy", 1);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }
}

void tst_QHash::compare()
{
    QHash<int, QString> hash1,hash2;
    QString testString = "Teststring %1";
    int i;

    for(i = 0; i < 1000; ++i)
        hash1.insert(i,testString.arg(i));

    for(--i; i >= 0; --i)
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
    QHash<int, int> a;
    QHash<int, int> b;

    a.insertMulti(17, 1);
    a.insertMulti(17 * 2, 1);
    b.insertMulti(17 * 2, 1);
    b.insertMulti(17, 1);
    QVERIFY(a == b);
    QVERIFY(b == a);

    a.insertMulti(17, 2);
    a.insertMulti(17 * 2, 3);
    b.insertMulti(17 * 2, 3);
    b.insertMulti(17, 2);
    QVERIFY(a == b);
    QVERIFY(b == a);

    a.insertMulti(17, 4);
    a.insertMulti(17 * 2, 5);
    b.insertMulti(17 * 2, 4);
    b.insertMulti(17, 5);
    QVERIFY(!(a == b));
    QVERIFY(!(b == a));

    a.clear();
    b.clear();
    a.insertMulti(1, 1);
    a.insertMulti(1, 2);
    a.insertMulti(1, 3);
    b.insertMulti(1, 1);
    b.insertMulti(1, 2);
    b.insertMulti(1, 3);
    b.insertMulti(1, 4);
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

    for(i = 1; i < 100; ++i)
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

    stlIt+=5;
    QVERIFY(stlIt.value() == testMap.value(6));

    stlIt++;
    QVERIFY(stlIt.value() == testMap.value(7));

    stlIt-=3;
    QVERIFY(stlIt.value() == testMap.value(4));

    stlIt--;
    QVERIFY(stlIt.value() == testMap.value(3));

    testMap.clear();

    //STL-Style const-iterators

    QHash<int, QString>::const_iterator cstlIt = hash.constBegin();
    for (cstlIt = hash.constBegin(), i = 1; cstlIt != hash.constEnd() && i < 100; ++cstlIt, ++i) {
            testMap.insert(i,cstlIt.value());
            //QVERIFY(stlIt.value() == hash.value(
    }
    cstlIt = hash.constBegin();

    QVERIFY(cstlIt.value() == testMap.value(1));

    cstlIt+=5;
    QVERIFY(cstlIt.value() == testMap.value(6));

    cstlIt++;
    QVERIFY(cstlIt.value() == testMap.value(7));

    cstlIt-=3;
    QVERIFY(cstlIt.value() == testMap.value(4));

    cstlIt--;
    QVERIFY(cstlIt.value() == testMap.value(3));

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

    ++i;
    while(javaIt.hasPrevious()) {
        --i;
        javaIt.previous();
        QVERIFY(javaIt.value() == testMap.value(i));
    }

    //peekNext()  peekPrevious()
    javaIt.toFront();
    javaIt.next();
    while(javaIt.hasNext()) {
        testString = javaIt.value();
        testString1 = javaIt.peekNext().value();
        javaIt.next();
        QVERIFY(javaIt.value() == testString1);
        QCOMPARE(javaIt.peekPrevious().value(), testString1);
    }
    while(javaIt.hasPrevious()) {
        testString = javaIt.value();
        testString1 = javaIt.peekPrevious().value();
        javaIt.previous();
        QVERIFY(javaIt.value() == testString1);
        QCOMPARE(javaIt.peekNext().value(), testString1);
    }
}

void tst_QHash::rehash_isnt_quadratic()
{
    // this test should be incredibly slow if rehash() is quadratic
    for (int j = 0; j < 5; ++j) {
        QHash<int, int> testHash;
#if defined(Q_OS_WINCE) // mobiles do not have infinite mem...
        for (int i = 0; i < 50000; ++i)
#else
        for (int i = 0; i < 500000; ++i)
#endif
            testHash.insertMulti(1, 1);
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
}

void tst_QHash::qhash()
{
    {
        QBitArray a1;
        QBitArray a2;
        QVERIFY(qHash(a1) == 0);

        a1.resize(1);
        a1.setBit(0, true);

        a2.resize(1);
        a2.setBit(0, false);

        uint h1 = qHash(a1);
        uint h2 = qHash(a2);

        QVERIFY(h1 != h2);

        a2.setBit(0, true);
        QVERIFY(h1 == qHash(a2));

        a1.fill(true, 8);
        a1.resize(7);

        h1 = qHash(a1);

        a2.fill(true, 7);
        h2 = qHash(a2);

        QVERIFY(h1 == h2);

        a2.setBit(0, false);
        uint h3 = qHash(a2);
        QVERIFY(h2 != h3);

        a2.setBit(0, true);
        QVERIFY(h2 == qHash(a2));

        a2.setBit(6, false);
        uint h4 = qHash(a2);
        QVERIFY(h2 != h4);

        a2.setBit(6, true);
        QVERIFY(h2 == qHash(a2));

        QVERIFY(h3 != h4);
    }

    {
        QPair<int, int> p12(1, 2);
        QPair<int, int> p21(2, 1);

        QVERIFY(qHash(p12) == qHash(p12));
        QVERIFY(qHash(p21) == qHash(p21));
        QVERIFY(qHash(p12) != qHash(p21));

        QPair<int, int> pA(0x12345678, 0x12345678);
        QPair<int, int> pB(0x12345675, 0x12345675);

        QVERIFY(qHash(pA) != qHash(pB));
    }
}

void tst_QHash::fp_qhash_of_zero_is_zero()
{
    QCOMPARE(qHash(-0.0f), 0U);
    QCOMPARE(qHash( 0.0f), 0U);

    QCOMPARE(qHash(-0.0 ), 0U);
    QCOMPARE(qHash( 0.0 ), 0U);

#ifndef Q_OS_DARWIN
    QCOMPARE(qHash(-0.0L), 0U);
    QCOMPARE(qHash( 0.0L), 0U);
#endif
}

void tst_QHash::qmultihash_specific()
{
    QMultiHash<int, int> hash1;
    for (int i = 1; i <= 9; ++i) {
        for (int j = 1; j <= i; ++j) {
            int k = i * 10 + j;
            QVERIFY(!hash1.contains(i, k));
            hash1.insert(i, k);
            QVERIFY(hash1.contains(i, k));
        }
    }

    for (int i = 1; i <= 9; ++i) {
        for (int j = 1; j <= i; ++j) {
            int k = i * 10 + j;
            QVERIFY(hash1.contains(i, k));
        }
    }

    QVERIFY(hash1.contains(9, 99));
    QCOMPARE(hash1.count(), 45);
    hash1.remove(9, 99);
    QVERIFY(!hash1.contains(9, 99));
    QCOMPARE(hash1.count(), 44);

    hash1.remove(9, 99);
    QVERIFY(!hash1.contains(9, 99));
    QCOMPARE(hash1.count(), 44);

    hash1.remove(1, 99);
    QCOMPARE(hash1.count(), 44);

    hash1.insert(1, 99);
    hash1.insert(1, 99);

    QCOMPARE(hash1.count(), 46);
    hash1.remove(1, 99);
    QCOMPARE(hash1.count(), 44);
    hash1.remove(1, 99);
    QCOMPARE(hash1.count(), 44);

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
    QCOMPARE(map1.count(), map2.count());
    QVERIFY(map1.remove(42,5));
    QVERIFY(map2.remove(42,5));
    QVERIFY(map1 == map2);
    }
}

template <typename T>
QList<T> sorted(const QList<T> &list)
{
    QList<T> res = list;
    std::sort(res.begin(), res.end());
    return res;
}

void tst_QHash::keys_values_uniqueKeys()
{
    QHash<QString, int> hash;
    QVERIFY(hash.uniqueKeys().isEmpty());
    QVERIFY(hash.keys().isEmpty());
    QVERIFY(hash.values().isEmpty());

    hash.insertMulti("alpha", 1);
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha"));
    QVERIFY(hash.keys() == hash.uniqueKeys());
    QVERIFY(hash.values() == (QList<int>() << 1));

    hash.insertMulti("beta", -2);
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(hash.keys() == hash.uniqueKeys());
    QVERIFY(sorted(hash.values()) == sorted(QList<int>() << 1 << -2));

    hash.insertMulti("alpha", 2);
    QVERIFY(sorted(hash.uniqueKeys()) == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha" << "alpha" << "beta"));
    QVERIFY(sorted(hash.values()) == sorted(QList<int>() << 2 << 1 << -2));

    hash.insertMulti("beta", 4);
    QVERIFY(sorted(hash.uniqueKeys()) == (QList<QString>() << "alpha" << "beta"));
    QVERIFY(sorted(hash.keys()) == (QList<QString>() << "alpha" << "alpha" << "beta" << "beta"));
    QVERIFY(sorted(hash.values()) == sorted(QList<int>() << 2 << 1 << 4 << -2));
}

void tst_QHash::noNeedlessRehashes()
{
    QHash<int, int> hash;
    for (int i = 0; i < 512; ++i) {
        int j = (i * 345) % 512;
        hash.insert(j, j);
        int oldCapacity = hash.capacity();
        hash[j] = j + 1;
        QCOMPARE(oldCapacity, hash.capacity());
        hash.insert(j, j + 1);
        QCOMPARE(oldCapacity, hash.capacity());
    }
}

void tst_QHash::const_shared_null()
{
    QHash<int, QString> hash2;
#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
    QHash<int, QString> hash1;
    hash1.setSharable(false);
    QVERIFY(hash1.isDetached());

    hash2.setSharable(true);
#endif
    QVERIFY(!hash2.isDetached());
}

// This gets set to != 0 in wrong qHash overloads
static int wrongqHashOverload = 0;

struct OneArgumentQHashStruct1 {};
bool operator==(const OneArgumentQHashStruct1 &, const OneArgumentQHashStruct1 &) { return false; }
uint qHash(OneArgumentQHashStruct1) { return 0; }

struct OneArgumentQHashStruct2 {};
bool operator==(const OneArgumentQHashStruct2 &, const OneArgumentQHashStruct2 &) { return false; }
uint qHash(const OneArgumentQHashStruct2 &) { return 0; }

struct OneArgumentQHashStruct3 {};
bool operator==(const OneArgumentQHashStruct3 &, const OneArgumentQHashStruct3 &) { return false; }
uint qHash(OneArgumentQHashStruct3) { return 0; }
uint qHash(OneArgumentQHashStruct3 &, uint) { wrongqHashOverload = 1; return 0; }

struct OneArgumentQHashStruct4 {};
bool operator==(const OneArgumentQHashStruct4 &, const OneArgumentQHashStruct4 &) { return false; }
uint qHash(const OneArgumentQHashStruct4 &) { return 0; }
uint qHash(OneArgumentQHashStruct4 &, uint) { wrongqHashOverload = 1; return 0; }


struct TwoArgumentsQHashStruct1 {};
bool operator==(const TwoArgumentsQHashStruct1 &, const TwoArgumentsQHashStruct1 &) { return false; }
uint qHash(const TwoArgumentsQHashStruct1 &) { wrongqHashOverload = 1; return 0; }
uint qHash(const TwoArgumentsQHashStruct1 &, uint) { return 0; }

struct TwoArgumentsQHashStruct2 {};
bool operator==(const TwoArgumentsQHashStruct2 &, const TwoArgumentsQHashStruct2 &) { return false; }
uint qHash(TwoArgumentsQHashStruct2) { wrongqHashOverload = 1; return 0; }
uint qHash(const TwoArgumentsQHashStruct2 &, uint) { return 0; }

struct TwoArgumentsQHashStruct3 {};
bool operator==(const TwoArgumentsQHashStruct3 &, const TwoArgumentsQHashStruct3 &) { return false; }
uint qHash(const TwoArgumentsQHashStruct3 &) { wrongqHashOverload = 1; return 0; }
uint qHash(TwoArgumentsQHashStruct3, uint) { return 0; }

struct TwoArgumentsQHashStruct4 {};
bool operator==(const TwoArgumentsQHashStruct4 &, const TwoArgumentsQHashStruct4 &) { return false; }
uint qHash(TwoArgumentsQHashStruct4) { wrongqHashOverload = 1; return 0; }
uint qHash(TwoArgumentsQHashStruct4, uint) { return 0; }

/*!
    \internal

    Check that QHash picks up the right overload.
    The best one, for a type T, is the two-args version of qHash:
    either uint qHash(T, uint) or uint qHash(const T &, uint).

    If neither of these exists, then one between
    uint qHash(T) or uint qHash(const T &) must exist
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
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QHash<int, QString> hash{{1, "hello"}, {2, "initializer_list"}};
    QCOMPARE(hash.count(), 2);
    QVERIFY(hash[1] == "hello");
    QVERIFY(hash[2] == "initializer_list");

    QMultiHash<QString, int> multiHash{{"il", 1}, {"il", 2}, {"il", 3}};
    QCOMPARE(multiHash.count(), 3);
    QList<int> values = multiHash.values("il");
    QCOMPARE(values.count(), 3);

    QHash<int, int> emptyHash{};
    QVERIFY(emptyHash.isEmpty());

    QHash<int, char> emptyPairs{{}, {}};
    QVERIFY(!emptyPairs.isEmpty());

    QMultiHash<QString, double> emptyMultiHash{};
    QVERIFY(emptyMultiHash.isEmpty());

    QMultiHash<int, float> emptyPairs2{{}, {}};
    QVERIFY(!emptyPairs2.isEmpty());
#else
    QSKIP("Compiler doesn't support initializer lists");
#endif
}

void tst_QHash::qthash_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<uint>("hash");

    QTest::newRow("null") << QString() << 0u;
    QTest::newRow("empty") << QStringLiteral("") << 0u;
    QTest::newRow("abcdef") << QStringLiteral("abcdef") << 108567222u;
    QTest::newRow("tqbfjotld") << QStringLiteral("The quick brown fox jumps over the lazy dog") << 140865879u;
    QTest::newRow("42") << QStringLiteral("42") << 882u;
}

void tst_QHash::qthash()
{
    QFETCH(QString, key);
    const uint result = qt_hash(key);
    QTEST(result, "hash");
}

void tst_QHash::eraseValidIteratorOnSharedHash()
{
    QHash<int, int> a, b;
    a.insert(10, 10);
    a.insertMulti(10, 25);
    a.insertMulti(10, 30);
    a.insert(20, 20);
    a.insert(40, 40);

    QHash<int, int>::iterator i = a.begin();
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

QTEST_APPLESS_MAIN(tst_QHash)
#include "tst_qhash.moc"
