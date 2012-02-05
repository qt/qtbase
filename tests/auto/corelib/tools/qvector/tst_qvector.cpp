/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qvector.h>

class tst_QVector : public QObject
{
    Q_OBJECT
private slots:
    void constructors() const;
    void append() const;
    void at() const;
    void capacity() const;
    void clear() const;
    void constData() const;
    void contains() const;
    void count() const;
    void data() const;
    void empty() const;
    void endsWith() const;
    void fill() const;
    void first() const;
    void fromList() const;
    void fromStdVector() const;
    void indexOf() const;
    void insert() const;
    void isEmpty() const;
    void last() const;
    void lastIndexOf() const;
    void mid() const;
    void prepend() const;
    void remove() const;
    void size() const;
    void startsWith() const;
    void swap() const;
    void toList() const;
    void toStdVector() const;
    void value() const;

    void testOperators() const;

    void outOfMemory();
    void reserve();
    void reallocAfterCopy_data();
    void reallocAfterCopy();
    void initializeList();

    void const_shared_null();
    void setSharable_data();
    void setSharable();
};

void tst_QVector::constructors() const
{
    // pre-reserve capacity
    {
        QVector<int> myvec(5);

        QVERIFY(myvec.capacity() == 5);
    }

    // default-initialise items
    {
        QVector<int> myvec(5, 42);

        QVERIFY(myvec.capacity() == 5);

        // make sure all items are initialised ok
        foreach (int meaningoflife, myvec) {
            QCOMPARE(meaningoflife, 42);
        }
    }
}

void tst_QVector::append() const
{
    QVector<int> myvec;
    myvec.append(42);
    myvec.append(43);
    myvec.append(44);

    QVERIFY(myvec.size() == 3);
    QCOMPARE(myvec, QVector<int>() << 42 << 43 << 44);
}

void tst_QVector::at() const
{
    QVector<QString> myvec;
    myvec << "foo" << "bar" << "baz";

    QVERIFY(myvec.size() == 3);
    QCOMPARE(myvec.at(0), QLatin1String("foo"));
    QCOMPARE(myvec.at(1), QLatin1String("bar"));
    QCOMPARE(myvec.at(2), QLatin1String("baz"));

    // append an item
    myvec << "hello";
    QVERIFY(myvec.size() == 4);
    QCOMPARE(myvec.at(0), QLatin1String("foo"));
    QCOMPARE(myvec.at(1), QLatin1String("bar"));
    QCOMPARE(myvec.at(2), QLatin1String("baz"));
    QCOMPARE(myvec.at(3), QLatin1String("hello"));

    // remove an item
    myvec.remove(1);
    QVERIFY(myvec.size() == 3);
    QCOMPARE(myvec.at(0), QLatin1String("foo"));
    QCOMPARE(myvec.at(1), QLatin1String("baz"));
    QCOMPARE(myvec.at(2), QLatin1String("hello"));
}

void tst_QVector::capacity() const
{
    QVector<QString> myvec;

    // TODO: is this guaranteed? seems a safe assumption, but I suppose preallocation of a
    // few items isn't an entirely unforseeable possibility.
    QVERIFY(myvec.capacity() == 0);

    // test it gets a size
    myvec << "aaa" << "bbb" << "ccc";
    QVERIFY(myvec.capacity() >= 3);

    // make sure it grows ok
    myvec << "aaa" << "bbb" << "ccc";
    QVERIFY(myvec.capacity() >= 6);

    // let's try squeeze a bit
    myvec.remove(3);
    myvec.remove(3);
    myvec.remove(3);
    // TODO: is this a safe assumption? presumably it won't release memory until shrink(), but can we asser that is true?
    QVERIFY(myvec.capacity() >= 6);
    myvec.squeeze();
    QVERIFY(myvec.capacity() >= 3);

    myvec.remove(0);
    myvec.remove(0);
    myvec.remove(0);
    // TODO: as above note
    QVERIFY(myvec.capacity() >= 3);
    myvec.squeeze();
    QVERIFY(myvec.capacity() == 0);
}

void tst_QVector::clear() const
{
    QVector<QString> myvec;
    myvec << "aaa" << "bbb" << "ccc";

    QVERIFY(myvec.size() == 3);
    myvec.clear();
    QVERIFY(myvec.size() == 0);
    QVERIFY(myvec.capacity() == 0);
}

void tst_QVector::constData() const
{
    int arr[] = { 42, 43, 44 };
    QVector<int> myvec;
    myvec << 42 << 43 << 44;

    QVERIFY(memcmp(myvec.constData(), reinterpret_cast<const int *>(&arr), sizeof(int) * 3) == 0);
}

void tst_QVector::contains() const
{
    QVector<QString> myvec;
    myvec << "aaa" << "bbb" << "ccc";

    QVERIFY(myvec.contains(QLatin1String("aaa")));
    QVERIFY(myvec.contains(QLatin1String("bbb")));
    QVERIFY(myvec.contains(QLatin1String("ccc")));
    QVERIFY(!myvec.contains(QLatin1String("I don't exist")));

    // add it and make sure it does :)
    myvec.append(QLatin1String("I don't exist"));
    QVERIFY(myvec.contains(QLatin1String("I don't exist")));
}

void tst_QVector::count() const
{
    // total size
    {
        // zero size
        QVector<int> myvec;
        QVERIFY(myvec.count() == 0);

        // grow
        myvec.append(42);
        QVERIFY(myvec.count() == 1);
        myvec.append(42);
        QVERIFY(myvec.count() == 2);

        // shrink
        myvec.remove(0);
        QVERIFY(myvec.count() == 1);
        myvec.remove(0);
        QVERIFY(myvec.count() == 0);
    }

    // count of items
    {
        QVector<QString> myvec;
        myvec << "aaa" << "bbb" << "ccc";

        // initial tests
        QVERIFY(myvec.count(QLatin1String("aaa")) == 1);
        QVERIFY(myvec.count(QLatin1String("pirates")) == 0);

        // grow
        myvec.append(QLatin1String("aaa"));
        QVERIFY(myvec.count(QLatin1String("aaa")) == 2);

        // shrink
        myvec.remove(0);
        QVERIFY(myvec.count(QLatin1String("aaa")) == 1);
    }
}

void tst_QVector::data() const
{
    QVector<int> myvec;
    myvec << 42 << 43 << 44;

    // make sure it starts off ok
    QCOMPARE(*(myvec.data() + 1), 43);

    // alter it
    *(myvec.data() + 1) = 69;

    // check it altered
    QCOMPARE(*(myvec.data() + 1), 69);

    int arr[] = { 42, 69, 44 };
    QVERIFY(memcmp(myvec.data(), reinterpret_cast<int *>(&arr), sizeof(int) * 3) == 0);
}

void tst_QVector::empty() const
{
    QVector<int> myvec;

    // starts empty
    QVERIFY(myvec.empty());

    // not empty
    myvec.append(1);
    QVERIFY(!myvec.empty());

    // empty again
    myvec.remove(0);
    QVERIFY(myvec.empty());
}

void tst_QVector::endsWith() const
{
    QVector<int> myvec;

    // empty vector
    QVERIFY(!myvec.endsWith(1));

    // add the one, should work
    myvec.append(1);
    QVERIFY(myvec.endsWith(1));

    // add something else, fails now
    myvec.append(3);
    QVERIFY(!myvec.endsWith(1));

    // remove it again :)
    myvec.remove(1);
    QVERIFY(myvec.endsWith(1));
}

void tst_QVector::fill() const
{
    QVector<int> myvec;

    // resize
    myvec.resize(5);
    myvec.fill(69);
    QCOMPARE(myvec, QVector<int>() << 69 << 69 << 69 << 69 << 69);

    // make sure it can resize itself too
    myvec.fill(42, 10);
    QCOMPARE(myvec, QVector<int>() << 42 << 42 << 42 << 42 << 42 << 42 << 42 << 42 << 42 << 42);
}

void tst_QVector::first() const
{
    QVector<int> myvec;
    myvec << 69 << 42 << 3;

    // test it starts ok
    QCOMPARE(myvec.first(), 69);

    // test removal changes
    myvec.remove(0);
    QCOMPARE(myvec.first(), 42);

    // test prepend changes
    myvec.prepend(23);
    QCOMPARE(myvec.first(), 23);
}

void tst_QVector::fromList() const
{
    QList<QString> list;
    list << "aaa" << "bbb" << "ninjas" << "pirates";

    QVector<QString> myvec;
    myvec = QVector<QString>::fromList(list);

    // test it worked ok
    QCOMPARE(myvec, QVector<QString>() << "aaa" << "bbb" << "ninjas" << "pirates");
    QCOMPARE(list, QList<QString>() << "aaa" << "bbb" << "ninjas" << "pirates");
}

void tst_QVector::fromStdVector() const
{
    // stl = :(
    std::vector<QString> svec;
    svec.push_back(QLatin1String("aaa"));
    svec.push_back(QLatin1String("bbb"));
    svec.push_back(QLatin1String("ninjas"));
    svec.push_back(QLatin1String("pirates"));
    QVector<QString> myvec = QVector<QString>::fromStdVector(svec);

    // test it converts ok
    QCOMPARE(myvec, QVector<QString>() << "aaa" << "bbb" << "ninjas" << "pirates");
}

void tst_QVector::indexOf() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C" << "B" << "A";

    QVERIFY(myvec.indexOf("B") == 1);
    QVERIFY(myvec.indexOf("B", 1) == 1);
    QVERIFY(myvec.indexOf("B", 2) == 3);
    QVERIFY(myvec.indexOf("X") == -1);
    QVERIFY(myvec.indexOf("X", 2) == -1);

    // add an X
    myvec << "X";
    QVERIFY(myvec.indexOf("X") == 5);
    QVERIFY(myvec.indexOf("X", 5) == 5);
    QVERIFY(myvec.indexOf("X", 6) == -1);

    // remove first A
    myvec.remove(0);
    QVERIFY(myvec.indexOf("A") == 3);
    QVERIFY(myvec.indexOf("A", 3) == 3);
    QVERIFY(myvec.indexOf("A", 4) == -1);
}

void tst_QVector::insert() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";

    // first position
    QCOMPARE(myvec.at(0), QLatin1String("A"));
    myvec.insert(0, QLatin1String("X"));
    QCOMPARE(myvec.at(0), QLatin1String("X"));
    QCOMPARE(myvec.at(1), QLatin1String("A"));

    // middle
    myvec.insert(1, QLatin1String("Z"));
    QCOMPARE(myvec.at(0), QLatin1String("X"));
    QCOMPARE(myvec.at(1), QLatin1String("Z"));
    QCOMPARE(myvec.at(2), QLatin1String("A"));

    // end
    myvec.insert(5, QLatin1String("T"));
    QCOMPARE(myvec.at(5), QLatin1String("T"));
    QCOMPARE(myvec.at(4), QLatin1String("C"));

    // insert a lot of garbage in the middle
    myvec.insert(2, 2, QLatin1String("infinity"));
    QCOMPARE(myvec, QVector<QString>() << "X" << "Z" << "infinity" << "infinity"
             << "A" << "B" << "C" << "T");
}

void tst_QVector::isEmpty() const
{
    QVector<QString> myvec;

    // starts ok
    QVERIFY(myvec.isEmpty());

    // not empty now
    myvec.append(QLatin1String("hello there"));
    QVERIFY(!myvec.isEmpty());

    // empty again
    myvec.remove(0);
    QVERIFY(myvec.isEmpty());
}

void tst_QVector::last() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";

    // test starts ok
    QCOMPARE(myvec.last(), QLatin1String("C"));

    // test it changes ok
    myvec.append(QLatin1String("X"));
    QCOMPARE(myvec.last(), QLatin1String("X"));

    // and remove again
    myvec.remove(3);
    QCOMPARE(myvec.last(), QLatin1String("C"));
}

void tst_QVector::lastIndexOf() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C" << "B" << "A";

    QVERIFY(myvec.lastIndexOf("B") == 3);
    QVERIFY(myvec.lastIndexOf("B", 2) == 1);
    QVERIFY(myvec.lastIndexOf("X") == -1);
    QVERIFY(myvec.lastIndexOf("X", 2) == -1);

    // add an X
    myvec << "X";
    QVERIFY(myvec.lastIndexOf("X") == 5);
    QVERIFY(myvec.lastIndexOf("X", 5) == 5);
    QVERIFY(myvec.lastIndexOf("X", 3) == -1);

    // remove first A
    myvec.remove(0);
    QVERIFY(myvec.lastIndexOf("A") == 3);
    QVERIFY(myvec.lastIndexOf("A", 3) == 3);
    QVERIFY(myvec.lastIndexOf("A", 2) == -1);
}

void tst_QVector::mid() const
{
    QVector<QString> list;
    list << "foo" << "bar" << "baz" << "bak" << "buck" << "hello" << "kitty";

    QCOMPARE(list.mid(3, 3), QVector<QString>() << "bak" << "buck" << "hello");
    QCOMPARE(list.mid(4), QVector<QString>() << "buck" << "hello" << "kitty");
}

void tst_QVector::prepend() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";

    // starts ok
    QVERIFY(myvec.size() == 3);
    QCOMPARE(myvec.at(0), QLatin1String("A"));

    // add something
    myvec.prepend(QLatin1String("X"));
    QCOMPARE(myvec.at(0), QLatin1String("X"));
    QCOMPARE(myvec.at(1), QLatin1String("A"));
    QVERIFY(myvec.size() == 4);

    // something else
    myvec.prepend(QLatin1String("Z"));
    QCOMPARE(myvec.at(0), QLatin1String("Z"));
    QCOMPARE(myvec.at(1), QLatin1String("X"));
    QCOMPARE(myvec.at(2), QLatin1String("A"));
    QVERIFY(myvec.size() == 5);

    // clear and append to an empty vector
    myvec.clear();
    QVERIFY(myvec.size() == 0);
    myvec.prepend(QLatin1String("ninjas"));
    QVERIFY(myvec.size() == 1);
    QCOMPARE(myvec.at(0), QLatin1String("ninjas"));
}

void tst_QVector::remove() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";

    // remove middle
    myvec.remove(1);
    QCOMPARE(myvec, QVector<QString>() << "A" << "C");

    // remove rest
    myvec.remove(0, 2);
    QCOMPARE(myvec, QVector<QString>());
}

// ::reserve() is really hard to think of tests for, so not doing it.
// ::resize() is tested in ::capacity().

void tst_QVector::size() const
{
    // total size
    {
        // zero size
        QVector<int> myvec;
        QVERIFY(myvec.size() == 0);

        // grow
        myvec.append(42);
        QVERIFY(myvec.size() == 1);
        myvec.append(42);
        QVERIFY(myvec.size() == 2);

        // shrink
        myvec.remove(0);
        QVERIFY(myvec.size() == 1);
        myvec.remove(0);
        QVERIFY(myvec.size() == 0);
    }
}

// ::squeeze() is tested in ::capacity().

void tst_QVector::startsWith() const
{
    QVector<int> myvec;

    // empty vector
    QVERIFY(!myvec.startsWith(1));

    // add the one, should work
    myvec.prepend(1);
    QVERIFY(myvec.startsWith(1));

    // add something else, fails now
    myvec.prepend(3);
    QVERIFY(!myvec.startsWith(1));

    // remove it again :)
    myvec.remove(0);
    QVERIFY(myvec.startsWith(1));
}

void tst_QVector::swap() const
{
    QVector<int> v1, v2;
    v1 << 1 << 2 << 3;
    v2 << 4 << 5 << 6;

    v1.swap(v2);
    QCOMPARE(v1,QVector<int>() << 4 << 5 << 6);
    QCOMPARE(v2,QVector<int>() << 1 << 2 << 3);
}

void tst_QVector::toList() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";

    // make sure it converts and doesn't modify the original vector
    QCOMPARE(myvec.toList(), QList<QString>() << "A" << "B" << "C");
    QCOMPARE(myvec, QVector<QString>() << "A" << "B" << "C");
}

void tst_QVector::toStdVector() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";

    std::vector<QString> svec = myvec.toStdVector();
    QCOMPARE(svec.at(0), QLatin1String("A"));
    QCOMPARE(svec.at(1), QLatin1String("B"));
    QCOMPARE(svec.at(2), QLatin1String("C"));

    QCOMPARE(myvec, QVector<QString>() << "A" << "B" << "C");
}

void tst_QVector::value() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";

    // valid calls
    QCOMPARE(myvec.value(0), QLatin1String("A"));
    QCOMPARE(myvec.value(1), QLatin1String("B"));
    QCOMPARE(myvec.value(2), QLatin1String("C"));

    // default calls
    QCOMPARE(myvec.value(-1), QString());
    QCOMPARE(myvec.value(3), QString());

    // test calls with a provided default, valid calls
    QCOMPARE(myvec.value(0, QLatin1String("default")), QLatin1String("A"));
    QCOMPARE(myvec.value(1, QLatin1String("default")), QLatin1String("B"));
    QCOMPARE(myvec.value(2, QLatin1String("default")), QLatin1String("C"));

    // test calls with a provided default that will return the default
    QCOMPARE(myvec.value(-1, QLatin1String("default")), QLatin1String("default"));
    QCOMPARE(myvec.value(3, QLatin1String("default")), QLatin1String("default"));
}

void tst_QVector::testOperators() const
{
    QVector<QString> myvec;
    myvec << "A" << "B" << "C";
    QVector<QString> myvectwo;
    myvectwo << "D" << "E" << "F";
    QVector<QString> combined;
    combined << "A" << "B" << "C" << "D" << "E" << "F";

    // !=
    QVERIFY(myvec != myvectwo);

    // +
    QCOMPARE(myvec + myvectwo, combined);
    QCOMPARE(myvec, QVector<QString>() << "A" << "B" << "C");
    QCOMPARE(myvectwo, QVector<QString>() << "D" << "E" << "F");

    // +=
    myvec += myvectwo;
    QCOMPARE(myvec, combined);

    // ==
    QVERIFY(myvec == combined);

    // []
    QCOMPARE(myvec[0], QLatin1String("A"));
    QCOMPARE(myvec[1], QLatin1String("B"));
    QCOMPARE(myvec[2], QLatin1String("C"));
    QCOMPARE(myvec[3], QLatin1String("D"));
    QCOMPARE(myvec[4], QLatin1String("E"));
    QCOMPARE(myvec[5], QLatin1String("F"));
}


int fooCtor;
int fooDtor;

struct Foo
{
    int *p;

    Foo() { p = new int; ++fooCtor; }
    Foo(const Foo &other) { Q_UNUSED(other); p = new int; ++fooCtor; }

    void operator=(const Foo & /* other */) { }

    ~Foo() { delete p; ++fooDtor; }
};

void tst_QVector::outOfMemory()
{
    fooCtor = 0;
    fooDtor = 0;

    const int N = 0x7fffffff / sizeof(Foo);

    {
        QVector<Foo> a;

        QSKIP("This test crashes on many of our machines.");
        a.resize(N);
        if (a.size() == N) {
            QVERIFY(a.capacity() >= N);
            QCOMPARE(fooCtor, N);
            QCOMPARE(fooDtor, 0);

            for (int i = 0; i < N; i += 35000)
                a[i] = Foo();
        } else {
            // this is the case we're actually testing
            QCOMPARE(a.size(), 0);
            QCOMPARE(a.capacity(), 0);
            QCOMPARE(fooCtor, 0);
            QCOMPARE(fooDtor, 0);

            a.resize(5);
            QCOMPARE(a.size(), 5);
            QVERIFY(a.capacity() >= 5);
            QCOMPARE(fooCtor, 5);
            QCOMPARE(fooDtor, 0);

            const int Prealloc = a.capacity();
            a.resize(Prealloc + 1);
            QCOMPARE(a.size(), Prealloc + 1);
            QVERIFY(a.capacity() >= Prealloc + 1);
            QCOMPARE(fooCtor, Prealloc + 6);
            QCOMPARE(fooDtor, 5);

            a.resize(0x10000000);
            QCOMPARE(a.size(), 0);
            QCOMPARE(a.capacity(), 0);
            QCOMPARE(fooCtor, Prealloc + 6);
            QCOMPARE(fooDtor, Prealloc + 6);
        }
    }

    fooCtor = 0;
    fooDtor = 0;

    {
        QVector<Foo> a(N);
        if (a.size() == N) {
            QVERIFY(a.capacity() >= N);
            QCOMPARE(fooCtor, N);
            QCOMPARE(fooDtor, 0);

            for (int i = 0; i < N; i += 35000)
                a[i] = Foo();
        } else {
            // this is the case we're actually testing
            QCOMPARE(a.size(), 0);
            QCOMPARE(a.capacity(), 0);
            QCOMPARE(fooCtor, 0);
            QCOMPARE(fooDtor, 0);
        }
    }

    Foo foo;

    fooCtor = 0;
    fooDtor = 0;

    {
        QVector<Foo> a(N, foo);
        if (a.size() == N) {
            QVERIFY(a.capacity() >= N);
            QCOMPARE(fooCtor, N);
            QCOMPARE(fooDtor, 0);

            for (int i = 0; i < N; i += 35000)
                a[i] = Foo();
        } else {
            // this is the case we're actually testing
            QCOMPARE(a.size(), 0);
            QCOMPARE(a.capacity(), 0);
            QCOMPARE(fooCtor, 0);
            QCOMPARE(fooDtor, 0);
        }
    }

    fooCtor = 0;
    fooDtor = 0;

    {
        QVector<Foo> a;
        a.resize(10);
        QCOMPARE(fooCtor, 10);
        QCOMPARE(fooDtor, 0);

        QVector<Foo> b(a);
        QCOMPARE(fooCtor, 10);
        QCOMPARE(fooDtor, 0);

        a.resize(N);
        if (a.size() == N) {
            QCOMPARE(fooCtor, N + 10);
        } else {
            QCOMPARE(a.size(), 0);
            QCOMPARE(a.capacity(), 0);
            QCOMPARE(fooCtor, 10);
            QCOMPARE(fooDtor, 0);

            QCOMPARE(b.size(), 10);
            QVERIFY(b.capacity() >= 10);
        }
    }

    {
        QVector<int> a;
        a.resize(10);

        QVector<int> b(a);

        a.resize(N);
        if (a.size() == N) {
            for (int i = 0; i < N; i += 60000)
                a[i] = i;
        } else {
            QCOMPARE(a.size(), 0);
            QCOMPARE(a.capacity(), 0);

            QCOMPARE(b.size(), 10);
            QVERIFY(b.capacity() >= 10);
        }

        b.resize(N - 1);
        if (b.size() == N - 1) {
            for (int i = 0; i < N - 1; i += 60000)
                b[i] = i;
        } else {
            QCOMPARE(b.size(), 0);
            QCOMPARE(b.capacity(), 0);
        }
    }
}

void tst_QVector::reserve()
{
    fooCtor = 0;
    fooDtor = 0;
    {
        QVector<Foo> a;
        a.resize(2);
        QVector<Foo> b(a);
        b.reserve(1);
    }
    QCOMPARE(fooCtor, fooDtor);
}

// This is a regression test for QTBUG-11763, where memory would be reallocated
// soon after copying a QVector.
void tst_QVector::reallocAfterCopy_data()
{
    QTest::addColumn<int>("capacity");
    QTest::addColumn<int>("fill_size");
    QTest::addColumn<int>("func_id");
    QTest::addColumn<int>("result1");
    QTest::addColumn<int>("result2");
    QTest::addColumn<int>("result3");
    QTest::addColumn<int>("result4");

    int result1, result2, result3, result4;
    int fill_size;
    for (int i = 70; i <= 100; i += 10) {
        fill_size = i - 20;
        for (int j = 0; j <= 3; j++) {
            if (j == 0) { // append
                result1 = i;
                result2 = i;
                result3 = i - 19;
                result4 = i - 20;
            } else if (j == 1) { // insert(0)
                result1 = i;
                result2 = i;
                result3 = i - 19;
                result4 = i - 20;
            } else if (j == 2) { // insert(20)
                result1 = i;
                result2 = i;
                result3 = i - 19;
                result4 = i - 20;
            } else if (j == 3) { // insert(0, 10)
                result1 = i;
                result2 = i;
                result3 = i - 10;
                result4 = i - 20;
            }
            QTest::newRow(qPrintable(QString("reallocAfterCopy:%1,%2").arg(i).arg(j))) << i << fill_size << j << result1 << result2 << result3 << result4;
        }
    }
}

void tst_QVector::reallocAfterCopy()
{
    QFETCH(int, capacity);
    QFETCH(int, fill_size);
    QFETCH(int, func_id);
    QFETCH(int, result1);
    QFETCH(int, result2);
    QFETCH(int, result3);
    QFETCH(int, result4);

    QVector<qreal> v1;
    QVector<qreal> v2;

    v1.reserve(capacity);
    v1.resize(0);
    v1.fill(qreal(1.0), fill_size);

    v2 = v1;

    // no need to test begin() and end(), there is a detach() in them
    if (func_id == 0) {
        v1.append(qreal(1.0)); //push_back is same as append
    } else if (func_id == 1) {
        v1.insert(0, qreal(1.0)); //push_front is same as prepend, insert(0)
    } else if (func_id == 2) {
        v1.insert(20, qreal(1.0));
    } else if (func_id == 3) {
        v1.insert(0, 10, qreal(1.0));
    }

    QCOMPARE(v1.capacity(), result1);
    QCOMPARE(v2.capacity(), result2);
    QCOMPARE(v1.size(), result3);
    QCOMPARE(v2.size(), result4);
}

void tst_QVector::initializeList()
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QVector<int> v1{2,3,4};
    QCOMPARE(v1, QVector<int>() << 2 << 3 << 4);
    QCOMPARE(v1, (QVector<int>{2,3,4}));

    QVector<QVector<int>> v2{ v1, {1}, QVector<int>(), {2,3,4}  };
    QVector<QVector<int>> v3;
    v3 << v1 << (QVector<int>() << 1) << QVector<int>() << v1;
    QCOMPARE(v3, v2);
#endif
}

void tst_QVector::const_shared_null()
{
    QVector<int> v1;
    v1.setSharable(false);
    QVERIFY(v1.isDetached());

    QVector<int> v2;
    v2.setSharable(true);
    QVERIFY(!v2.isDetached());
}

Q_DECLARE_METATYPE(QVector<int>);

void tst_QVector::setSharable_data()
{
    QTest::addColumn<QVector<int> >("vector");
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("capacity");
    QTest::addColumn<bool>("isCapacityReserved");

    QVector<int> null;
    QVector<int> empty(0, 5);
    QVector<int> emptyReserved;
    QVector<int> nonEmpty;
    QVector<int> nonEmptyReserved;

    emptyReserved.reserve(10);
    nonEmptyReserved.reserve(15);

    nonEmpty << 0 << 1 << 2 << 3 << 4;
    nonEmptyReserved << 0 << 1 << 2 << 3 << 4 << 5 << 6;

    QVERIFY(emptyReserved.capacity() >= 10);
    QVERIFY(nonEmptyReserved.capacity() >= 15);

    QTest::newRow("null") << null << 0 << 0 << false;
    QTest::newRow("empty") << empty << 0 << 0 << false;
    QTest::newRow("empty, Reserved") << emptyReserved << 0 << 10 << true;
    QTest::newRow("non-empty") << nonEmpty << 5 << 0 << false;
    QTest::newRow("non-empty, Reserved") << nonEmptyReserved << 7 << 15 << true;
}

void tst_QVector::setSharable()
{
    QFETCH(QVector<int>, vector);
    QFETCH(int, size);
    QFETCH(int, capacity);
    QFETCH(bool, isCapacityReserved);

    QVERIFY(!vector.isDetached()); // Shared with QTest

    vector.setSharable(true);

    QCOMPARE(vector.size(), size);
    if (isCapacityReserved)
        QVERIFY2(vector.capacity() >= capacity,
                qPrintable(QString("Capacity is %1, expected at least %2.")
                    .arg(vector.capacity())
                    .arg(capacity)));

    {
        QVector<int> copy(vector);

        QVERIFY(!copy.isDetached());
        QVERIFY(copy.isSharedWith(vector));
    }

    vector.setSharable(false);
    QVERIFY(vector.isDetached() || vector.isSharedWith(QVector<int>()));

    {
        QVector<int> copy(vector);

        QVERIFY(copy.isDetached() || copy.isSharedWith(QVector<int>()));
        QCOMPARE(copy.size(), size);
        if (isCapacityReserved)
            QVERIFY2(copy.capacity() >= capacity,
                    qPrintable(QString("Capacity is %1, expected at least %2.")
                        .arg(vector.capacity())
                        .arg(capacity)));
        QCOMPARE(copy, vector);
    }

    vector.setSharable(true);

    {
        QVector<int> copy(vector);

        QVERIFY(!copy.isDetached());
        QVERIFY(copy.isSharedWith(vector));
    }

    for (int i = 0; i < vector.size(); ++i)
        QCOMPARE(vector[i], i);

    QCOMPARE(vector.size(), size);
    if (isCapacityReserved)
        QVERIFY2(vector.capacity() >= capacity,
                qPrintable(QString("Capacity is %1, expected at least %2.")
                    .arg(vector.capacity())
                    .arg(capacity)));
}

QTEST_APPLESS_MAIN(tst_QVector)
#include "tst_qvector.moc"
