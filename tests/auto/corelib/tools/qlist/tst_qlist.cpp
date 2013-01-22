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
#include <QList>

struct Movable {
    Movable(char input = 'j')
        : i(input)
        , state(Constructed)
    {
        ++liveCount;
    }
    Movable(const Movable &other)
        : i(other.i)
        , state(Constructed)
    {
        check(other.state, Constructed);
        ++liveCount;
    }

    ~Movable()
    {
        check(state, Constructed);
        i = 0;
        --liveCount;
        state = Destructed;
    }

    bool operator ==(const Movable &other) const
    {
        check(state, Constructed);
        check(other.state, Constructed);
        return i == other.i;
    }

    Movable &operator=(const Movable &other)
    {
        check(state, Constructed);
        check(other.state, Constructed);
        i = other.i;
        return *this;
    }
    char i;

    static int getLiveCount() { return liveCount; }
private:
    static int liveCount;

    enum State { Constructed = 106, Destructed = 110 };
    State state;

    static void check(const State state1, const State state2)
    {
        QCOMPARE(state1, state2);
    }
};

int Movable::liveCount = 0;

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Movable, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Movable);

int qHash(const Movable& movable)
{
    return qHash(movable.i);
}

struct Complex
{
    Complex(int val = 0)
        : value(val)
        , checkSum(this)
    {
        ++liveCount;
    }

    Complex(Complex const &other)
        : value(other.value)
        , checkSum(this)
    {
        ++liveCount;
    }

    Complex &operator=(Complex const &other)
    {
        check(); other.check();

        value = other.value;
        return *this;
    }

    ~Complex()
    {
        --liveCount;
        check();
    }

    operator int() const { return value; }

    bool operator==(Complex const &other) const
    {
        check(); other.check();
        return value == other.value;
    }

    void check() const
    {
        QVERIFY(this == checkSum);
    }

    static int getLiveCount() { return liveCount; }
private:
    static int liveCount;

    int value;
    void *checkSum;
};

int Complex::liveCount = 0;

Q_DECLARE_METATYPE(Complex);

// Tests depend on the fact that:
Q_STATIC_ASSERT(!QTypeInfo<int>::isStatic);
Q_STATIC_ASSERT(!QTypeInfo<int>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<Movable>::isStatic);
Q_STATIC_ASSERT(QTypeInfo<Movable>::isComplex);
Q_STATIC_ASSERT(QTypeInfo<Complex>::isStatic);
Q_STATIC_ASSERT(QTypeInfo<Complex>::isComplex);

class tst_QList : public QObject
{
    Q_OBJECT

private slots:
    void lengthInt() const;
    void lengthMovable() const;
    void lengthComplex() const;
    void lengthSignature() const;
    void appendInt() const;
    void appendMovable() const;
    void appendComplex() const;
    void prepend() const;
    void midInt() const;
    void midMovable() const;
    void midComplex() const;
    void atInt() const;
    void atMovable() const;
    void atComplex() const;
    void firstInt() const;
    void firstMovable() const;
    void firstComplex() const;
    void lastInt() const;
    void lastMovable() const;
    void lastComplex() const;
    void beginInt() const;
    void beginMovable() const;
    void beginComplex() const;
    void endInt() const;
    void endMovable() const;
    void endComplex() const;
    void containsInt() const;
    void containsMovable() const;
    void containsComplex() const;
    void countInt() const;
    void countMovable() const;
    void countComplex() const;
    void emptyInt() const;
    void emptyMovable() const;
    void emptyComplex() const;
    void endsWithInt() const;
    void endsWithMovable() const;
    void endsWithComplex() const;
    void lastIndexOfInt() const;
    void lastIndexOfMovable() const;
    void lastIndexOfComplex() const;
    void moveInt() const;
    void moveMovable() const;
    void moveComplex() const;
    void removeAllInt() const;
    void removeAllMovable() const;
    void removeAllComplex() const;
    void removeAtInt() const;
    void removeAtMovable() const;
    void removeAtComplex() const;
    void removeOneInt() const;
    void removeOneMovable() const;
    void removeOneComplex() const;
    void replaceInt() const;
    void replaceMovable() const;
    void replaceComplex() const;
    void startsWithInt() const;
    void startsWithMovable() const;
    void startsWithComplex() const;
    void swapInt() const;
    void swapMovable() const;
    void swapComplex() const;
    void takeAtInt() const;
    void takeAtMovable() const;
    void takeAtComplex() const;
    void takeFirstInt() const;
    void takeFirstMovable() const;
    void takeFirstComplex() const;
    void takeLastInt() const;
    void takeLastMovable() const;
    void takeLastComplex() const;
    void toSetInt() const;
    void toSetMovable() const;
    void toSetComplex() const;
    void toStdListInt() const;
    void toStdListMovable() const;
    void toStdListComplex() const;
    void toVectorInt() const;
    void toVectorMovable() const;
    void toVectorComplex() const;
    void valueInt() const;
    void valueMovable() const;
    void valueComplex() const;

    void testOperatorsInt() const;
    void testOperatorsMovable() const;
    void testOperatorsComplex() const;
    void testSTLIteratorsInt() const;
    void testSTLIteratorsMovable() const;
    void testSTLIteratorsComplex() const;

    void initializeList() const;

    void constSharedNullInt() const;
    void constSharedNullMovable() const;
    void constSharedNullComplex() const;
    void setSharableInt_data() const;
    void setSharableInt() const;
    void setSharableComplex_data() const;
    void setSharableComplex() const;
private:
    template<typename T> void length() const;
    template<typename T> void append() const;
    template<typename T> void mid() const;
    template<typename T> void at() const;
    template<typename T> void first() const;
    template<typename T> void last() const;
    template<typename T> void begin() const;
    template<typename T> void end() const;
    template<typename T> void contains() const;
    template<typename T> void count() const;
    template<typename T> void empty() const;
    template<typename T> void endsWith() const;
    template<typename T> void lastIndexOf() const;
    template<typename T> void move() const;
    template<typename T> void removeAll() const;
    template<typename T> void removeAt() const;
    template<typename T> void removeOne() const;
    template<typename T> void replace() const;
    template<typename T> void startsWith() const;
    template<typename T> void swap() const;
    template<typename T> void takeAt() const;
    template<typename T> void takeFirst() const;
    template<typename T> void takeLast() const;
    template<typename T> void toSet() const;
    template<typename T> void toStdList() const;
    template<typename T> void toVector() const;
    template<typename T> void value() const;

    template<typename T> void testOperators() const;
    template<typename T> void testSTLIterators() const;

    template<typename T> void constSharedNull() const;

    int dummyForGuard;
};

template<typename T> struct SimpleValue
{
    static T at(int index)
    {
        return values[index % maxSize];
    }
    static const uint maxSize = 7;
    static const T values[maxSize];
};

template<>
const int SimpleValue<int>::values[] = { 10, 20, 30, 40, 100, 101, 102 };
template<>
const Movable SimpleValue<Movable>::values[] = { 10, 20, 30, 40, 100, 101, 102 };
template<>
const Complex SimpleValue<Complex>::values[] = { 10, 20, 30, 40, 100, 101, 102 };

// Make some macros for the tests to use in order to be slightly more readable...
#define T_FOO SimpleValue<T>::at(0)
#define T_BAR SimpleValue<T>::at(1)
#define T_BAZ SimpleValue<T>::at(2)
#define T_CAT SimpleValue<T>::at(3)
#define T_DOG SimpleValue<T>::at(4)
#define T_BLAH SimpleValue<T>::at(5)
#define T_WEEE SimpleValue<T>::at(6)

template<typename T>
void tst_QList::length() const
{
    /* Empty list. */
    {
        const QList<T> list;
        QCOMPARE(list.length(), 0);
    }

    /* One entry. */
    {
        QList<T> list;
        list.append(T_FOO);
        QCOMPARE(list.length(), 1);
    }

    /* Two entries. */
    {
        QList<T> list;
        list.append(T_FOO);
        list.append(T_BAR);
        QCOMPARE(list.length(), 2);
    }

    /* Three entries. */
    {
        QList<T> list;
        list.append(T_FOO);
        list.append(T_BAR);
        list.append(T_BAZ);
        QCOMPARE(list.length(), 3);
    }
}

void tst_QList::lengthInt() const
{
    length<int>();
}

void tst_QList::lengthMovable() const
{
    const int liveCount = Movable::getLiveCount();
    length<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::lengthComplex() const
{
    const int liveCount = Complex::getLiveCount();
    length<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QList::lengthSignature() const
{
    /* Constness. */
    {
        const QList<int> list;
        /* The function should be const. */
        list.length();
    }
}

template<typename T>
void tst_QList::append() const
{
    /* test append(const QList<T> &) function */
    T one(T_FOO);
    T two(T_BAR);
    T three(T_BAZ);
    T four(T_CAT);
    QList<T> list1;
    QList<T> list2;
    QList<T> listTotal;
    list1.append(one);
    list1.append(two);
    list2.append(three);
    list2.append(four);
    list1.append(list2);
    listTotal.append(one);
    listTotal.append(two);
    listTotal.append(three);
    listTotal.append(four);
    QCOMPARE(list1, listTotal);
}

void tst_QList::appendInt() const
{
    append<int>();
}

void tst_QList::appendMovable() const
{
    const int liveCount = Movable::getLiveCount();
    append<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::appendComplex() const
{
    const int liveCount = Complex::getLiveCount();
    append<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QList::prepend() const
{
    QList<int *> list;
    int *t1 = new int(0);
    list.prepend(t1);
    QVERIFY(list.size() == 1);
    QVERIFY(list.at(0) == t1);
    int *t2 = new int(0);
    list.prepend(t2);
    QVERIFY(list.size() == 2);
    QVERIFY(list.at(0) == t2);
    QVERIFY(list.at(1) == t1);
    int *t3 = new int(0);
    list.prepend(t3);
    QVERIFY(list.size() == 3);
    QVERIFY(list.at(0) == t3);
    QVERIFY(list.at(1) == t2);
    QVERIFY(list.at(2) == t1);
    list.removeAll(t2);
    delete t2;
    QVERIFY(list.size() == 2);
    QVERIFY(list.at(0) == t3);
    QVERIFY(list.at(1) == t1);
    int *t4 = new int(0);
    list.prepend(t4);
    QVERIFY(list.size() == 3);
    QVERIFY(list.at(0) == t4);
    QVERIFY(list.at(1) == t3);
    QVERIFY(list.at(2) == t1);
    qDeleteAll(list);
    list.clear();
}

template<typename T>
void tst_QList::mid() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ << T_CAT << T_DOG << T_BLAH << T_WEEE;

    QCOMPARE(list.mid(3, 3),
        QList<T>() << T_CAT << T_DOG << T_BLAH);

    QList<T> list1;
    QCOMPARE(list1.mid(1, 1).length(), 0);
}

void tst_QList::midInt() const
{
    mid<int>();
}

void tst_QList::midMovable() const
{
    const int liveCount = Movable::getLiveCount();
    mid<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::midComplex() const
{
    const int liveCount = Complex::getLiveCount();
    mid<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::at() const
{
    // test at() and make sure it functions correctly with some simple list manipulation.
    QList<T> list;

    // create a list
    list << T_FOO << T_BAR << T_BAZ;
    QVERIFY(list.size() == 3);
    QCOMPARE(list.at(0), T_FOO);
    QCOMPARE(list.at(1), T_BAR);
    QCOMPARE(list.at(2), T_BAZ);

    // append an item
    list << T_CAT;
    QVERIFY(list.size() == 4);
    QCOMPARE(list.at(0), T_FOO);
    QCOMPARE(list.at(1), T_BAR);
    QCOMPARE(list.at(2), T_BAZ);
    QCOMPARE(list.at(3), T_CAT);

    // remove an item
    list.removeAt(1);
    QVERIFY(list.size() == 3);
    QCOMPARE(list.at(0), T_FOO);
    QCOMPARE(list.at(1), T_BAZ);
    QCOMPARE(list.at(2), T_CAT);
}

void tst_QList::atInt() const
{
    at<int>();
}

void tst_QList::atMovable() const
{
    const int liveCount = Movable::getLiveCount();
    at<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::atComplex() const
{
    const int liveCount = Complex::getLiveCount();
    at<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::first() const
{
    QList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(list.first(), T_FOO);

    // remove an item, make sure it still works
    list.pop_front();
    QVERIFY(list.size() == 1);
    QCOMPARE(list.first(), T_BAR);
}

void tst_QList::firstInt() const
{
    first<int>();
}

void tst_QList::firstMovable() const
{
    const int liveCount = Movable::getLiveCount();
    first<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::firstComplex() const
{
    const int liveCount = Complex::getLiveCount();
    first<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::last() const
{
    QList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(list.last(), T_BAR);

    // remove an item, make sure it still works
    list.pop_back();
    QVERIFY(list.size() == 1);
    QCOMPARE(list.last(), T_FOO);
}

void tst_QList::lastInt() const
{
    last<int>();
}

void tst_QList::lastMovable() const
{
    const int liveCount = Movable::getLiveCount();
    last<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::lastComplex() const
{
    const int liveCount = Complex::getLiveCount();
    last<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::begin() const
{
    QList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(*list.begin(), T_FOO);

    // remove an item, make sure it still works
    list.pop_front();
    QVERIFY(list.size() == 1);
    QCOMPARE(*list.begin(), T_BAR);
}

void tst_QList::beginInt() const
{
    begin<int>();
}

void tst_QList::beginMovable() const
{
    const int liveCount = Movable::getLiveCount();
    begin<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::beginComplex() const
{
    const int liveCount = Complex::getLiveCount();
    begin<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::end() const
{
    QList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(*--list.end(), T_BAR);

    // remove an item, make sure it still works
    list.pop_back();
    QVERIFY(list.size() == 1);
    QCOMPARE(*--list.end(), T_FOO);
}

void tst_QList::endInt() const
{
    end<int>();
}

void tst_QList::endMovable() const
{
    const int liveCount = Movable::getLiveCount();
    end<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::endComplex() const
{
    const int liveCount = Complex::getLiveCount();
    end<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::contains() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QVERIFY(list.contains(T_FOO) == true);
    QVERIFY(list.contains(T_BLAH) != true);

    // add it and make sure it matches
    list.append(T_BLAH);
    QVERIFY(list.contains(T_BLAH) == true);
}

void tst_QList::containsInt() const
{
    contains<int>();
}

void tst_QList::containsMovable() const
{
    const int liveCount = Movable::getLiveCount();
    contains<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::containsComplex() const
{
    const int liveCount = Complex::getLiveCount();
    contains<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::count() const
{
    QList<T> list;

    // starts empty
    QVERIFY(list.count() == 0);

    // goes up
    list.append(T_FOO);
    QVERIFY(list.count() == 1);

    // and up
    list.append(T_BAR);
    QVERIFY(list.count() == 2);

    // and down
    list.pop_back();
    QVERIFY(list.count() == 1);

    // and empty. :)
    list.pop_back();
    QVERIFY(list.count() == 0);
}

void tst_QList::countInt() const
{
    count<int>();
}

void tst_QList::countMovable() const
{
    const int liveCount = Movable::getLiveCount();
    count<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::countComplex() const
{
    const int liveCount = Complex::getLiveCount();
    count<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::empty() const
{
    QList<T> list;

    // make sure it starts empty
    QVERIFY(list.empty());

    // and doesn't stay empty
    list.append(T_FOO);
    QVERIFY(!list.empty());

    // and goes back to being empty
    list.pop_back();
    QVERIFY(list.empty());
}

void tst_QList::emptyInt() const
{
    empty<int>();
}

void tst_QList::emptyMovable() const
{
    const int liveCount = Movable::getLiveCount();
    empty<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::emptyComplex() const
{
    const int liveCount = Complex::getLiveCount();
    empty<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::endsWith() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // test it returns correctly in both cases
    QVERIFY(list.endsWith(T_BAZ));
    QVERIFY(!list.endsWith(T_BAR));

    // remove an item and make sure the end item changes
    list.pop_back();
    QVERIFY(list.endsWith(T_BAR));
}

void tst_QList::endsWithInt() const
{
    endsWith<int>();
}

void tst_QList::endsWithMovable() const
{
    const int liveCount = Movable::getLiveCount();
    endsWith<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::endsWithComplex() const
{
    const int liveCount = Complex::getLiveCount();
    endsWith<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::lastIndexOf() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // one instance of the target item
    QVERIFY(list.lastIndexOf(T_BAZ) == 2);

    // shouldn't find this
    QVERIFY(list.lastIndexOf(T_WEEE) == -1);

    // multiple instances
    list.append(T_BAZ);
    list.append(T_BAZ);
    QVERIFY(list.lastIndexOf(T_BAZ) == 4);

    // search from the middle to find the last one
    QVERIFY(list.lastIndexOf(T_BAZ, 3) == 3);

    // try to find none
    QVERIFY(list.lastIndexOf(T_BAZ, 1) == -1);
}

void tst_QList::lastIndexOfInt() const
{
    lastIndexOf<int>();
}

void tst_QList::lastIndexOfMovable() const
{
    const int liveCount = Movable::getLiveCount();
    lastIndexOf<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::lastIndexOfComplex() const
{
    const int liveCount = Complex::getLiveCount();
    lastIndexOf<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::move() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // move an item
    list.move(0, list.count() - 1);
    QCOMPARE(list, QList<T>() << T_BAR << T_BAZ << T_FOO);

    // move it back
    list.move(list.count() - 1, 0);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAR << T_BAZ);

    // move an item in the middle
    list.move(1, 0);
    QCOMPARE(list, QList<T>() << T_BAR << T_FOO << T_BAZ);
}

void tst_QList::moveInt() const
{
    move<int>();
}

void tst_QList::moveMovable() const
{
    const int liveCount = Movable::getLiveCount();
    move<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::moveComplex() const
{
    const int liveCount = Complex::getLiveCount();
    move<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::removeAll() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // remove one instance
    list.removeAll(T_BAR);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAZ);

    // many instances
    list << T_FOO << T_BAR << T_BAZ << T_FOO << T_BAR << T_BAZ << T_FOO << T_BAR << T_BAZ;
    list.removeAll(T_BAR);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO << T_BAZ);

    // try remove something that doesn't exist
    list.removeAll(T_WEEE);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO << T_BAZ);
}

void tst_QList::removeAllInt() const
{
    removeAll<int>();
}

void tst_QList::removeAllMovable() const
{
    const int liveCount = Movable::getLiveCount();
    removeAll<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::removeAllComplex() const
{
    const int liveCount = Complex::getLiveCount();
    removeAll<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::removeAt() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // middle
    list.removeAt(1);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAZ);

    // start
    list.removeAt(0);
    QCOMPARE(list, QList<T>() << T_BAZ);

    // final
    list.removeAt(0);
    QCOMPARE(list, QList<T>());
}

void tst_QList::removeAtInt() const
{
    removeAt<int>();
}

void tst_QList::removeAtMovable() const
{
    const int liveCount = Movable::getLiveCount();
    removeAt<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::removeAtComplex() const
{
    const int liveCount = Complex::getLiveCount();
    removeAt<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::removeOne() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // middle
    list.removeOne(T_BAR);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAZ);

    // start
    list.removeOne(T_FOO);
    QCOMPARE(list, QList<T>() << T_BAZ);

    // last
    list.removeOne(T_BAZ);
    QCOMPARE(list, QList<T>());

    // make sure it really only removes one :)
    list << T_FOO << T_FOO;
    list.removeOne(T_FOO);
    QCOMPARE(list, QList<T>() << T_FOO);

    // try remove something that doesn't exist
    list.removeOne(T_WEEE);
    QCOMPARE(list, QList<T>() << T_FOO);
}

void tst_QList::removeOneInt() const
{
    removeOne<int>();
}

void tst_QList::removeOneMovable() const
{
    const int liveCount = Movable::getLiveCount();
    removeOne<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::removeOneComplex() const
{
    const int liveCount = Complex::getLiveCount();
    removeOne<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::replace() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // start
    list.replace(0, T_CAT);
    QCOMPARE(list, QList<T>() << T_CAT
        << T_BAR << T_BAZ);

    // middle
    list.replace(1, T_DOG);
    QCOMPARE(list, QList<T>() << T_CAT
        << T_DOG << T_BAZ);

    // end
    list.replace(2, T_BLAH);
    QCOMPARE(list, QList<T>() << T_CAT
        << T_DOG << T_BLAH);
}

void tst_QList::replaceInt() const
{
    replace<int>();
}

void tst_QList::replaceMovable() const
{
    const int liveCount = Movable::getLiveCount();
    replace<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::replaceComplex() const
{
    const int liveCount = Complex::getLiveCount();
    replace<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::startsWith() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // make sure it starts ok
    QVERIFY(list.startsWith(T_FOO));

    // remove an item
    list.removeFirst();
    QVERIFY(list.startsWith(T_BAR));
}

void tst_QList::startsWithInt() const
{
    startsWith<int>();
}

void tst_QList::startsWithMovable() const
{
    const int liveCount = Movable::getLiveCount();
    startsWith<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::startsWithComplex() const
{
    const int liveCount = Complex::getLiveCount();
    startsWith<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::swap() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // swap
    list.swap(0, 2);
    QCOMPARE(list, QList<T>() << T_BAZ << T_BAR << T_FOO);

    // swap again
    list.swap(1, 2);
    QCOMPARE(list, QList<T>() << T_BAZ << T_FOO << T_BAR);

    QList<T> list2;
    list2 << T_DOG << T_BLAH;

    list.swap(list2);
    QCOMPARE(list,  QList<T>() << T_DOG << T_BLAH);
    QCOMPARE(list2, QList<T>() << T_BAZ << T_FOO << T_BAR);
}

void tst_QList::swapInt() const
{
    swap<int>();
}

void tst_QList::swapMovable() const
{
    const int liveCount = Movable::getLiveCount();
    swap<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::swapComplex() const
{
    const int liveCount = Complex::getLiveCount();
    swap<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::takeAt() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QCOMPARE(list.takeAt(0), T_FOO);
    QVERIFY(list.size() == 2);
    QCOMPARE(list.takeAt(1), T_BAZ);
    QVERIFY(list.size() == 1);
    QCOMPARE(list.takeAt(0), T_BAR);
    QVERIFY(list.size() == 0);
}

void tst_QList::takeAtInt() const
{
    takeAt<int>();
}

void tst_QList::takeAtMovable() const
{
    const int liveCount = Movable::getLiveCount();
    takeAt<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::takeAtComplex() const
{
    const int liveCount = Complex::getLiveCount();
    takeAt<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::takeFirst() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QCOMPARE(list.takeFirst(), T_FOO);
    QVERIFY(list.size() == 2);
    QCOMPARE(list.takeFirst(), T_BAR);
    QVERIFY(list.size() == 1);
    QCOMPARE(list.takeFirst(), T_BAZ);
    QVERIFY(list.size() == 0);
}

void tst_QList::takeFirstInt() const
{
    takeFirst<int>();
}

void tst_QList::takeFirstMovable() const
{
    const int liveCount = Movable::getLiveCount();
    takeFirst<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::takeFirstComplex() const
{
    const int liveCount = Complex::getLiveCount();
    takeFirst<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::takeLast() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QCOMPARE(list.takeLast(), T_BAZ);
    QCOMPARE(list.takeLast(), T_BAR);
    QCOMPARE(list.takeLast(), T_FOO);
}

void tst_QList::takeLastInt() const
{
    takeLast<int>();
}

void tst_QList::takeLastMovable() const
{
    const int liveCount = Movable::getLiveCount();
    takeLast<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::takeLastComplex() const
{
    const int liveCount = Complex::getLiveCount();
    takeLast<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::toSet() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // no duplicates
    QCOMPARE(list.toSet(), QSet<T>() << T_FOO << T_BAR << T_BAZ);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAR << T_BAZ);

    // duplicates (is this more of a QSet test?)
    list << T_FOO << T_BAR << T_BAZ;
    QCOMPARE(list.toSet(), QSet<T>() << T_FOO << T_BAR << T_BAZ);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAR << T_BAZ
        << T_FOO << T_BAR << T_BAZ);
}

void tst_QList::toSetInt() const
{
    toSet<int>();
}

void tst_QList::toSetMovable() const
{
    const int liveCount = Movable::getLiveCount();
    toSet<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::toSetComplex() const
{
    const int liveCount = Complex::getLiveCount();
    toSet<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::toStdList() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // yuck.
    std::list<T> slist;
    slist.push_back(T_FOO);
    slist.push_back(T_BAR);
    slist.push_back(T_BAZ);

    QCOMPARE(list.toStdList(), slist);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAR << T_BAZ);
}

void tst_QList::toStdListInt() const
{
    toStdList<int>();
}

void tst_QList::toStdListMovable() const
{
    const int liveCount = Movable::getLiveCount();
    toStdList<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::toStdListComplex() const
{
    const int liveCount = Complex::getLiveCount();
    toStdList<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::toVector() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QCOMPARE(list.toVector(), QVector<T>() << T_FOO << T_BAR << T_BAZ);
}

void tst_QList::toVectorInt() const
{
    toVector<int>();
}

void tst_QList::toVectorMovable() const
{
    const int liveCount = Movable::getLiveCount();
    toVector<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::toVectorComplex() const
{
    const int liveCount = Complex::getLiveCount();
    toVector<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::value() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // test real values
    QCOMPARE(list.value(0), T_FOO);
    QCOMPARE(list.value(2), T_BAZ);

    // test empty default
    QCOMPARE(list.value(3), T());
    QCOMPARE(list.value(-1), T());

    // test defaults
    T defaultT(T_WEEE);
    QCOMPARE(list.value(-1, defaultT), defaultT);
    QCOMPARE(list.value(3, defaultT), defaultT);
}

void tst_QList::valueInt() const
{
    value<int>();
}

void tst_QList::valueMovable() const
{
    const int liveCount = Movable::getLiveCount();
    value<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::valueComplex() const
{
    const int liveCount = Complex::getLiveCount();
    value<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::testOperators() const
{
    QList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QList<T> listtwo;
    listtwo << T_FOO << T_BAR << T_BAZ;

    // test equal
    QVERIFY(list == listtwo);

    // not equal
    listtwo.append(T_CAT);
    QVERIFY(list != listtwo);

    // +=
    list += listtwo;
    QVERIFY(list.size() == 7);
    QVERIFY(listtwo.size() == 4);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAR << T_BAZ
        << T_FOO << T_BAR << T_BAZ << T_CAT);

    // =
    list = listtwo;
    QCOMPARE(list, listtwo);
    QCOMPARE(list, QList<T>() << T_FOO << T_BAR << T_BAZ << T_CAT);

    // []
    QCOMPARE(list[0], T_FOO);
    QCOMPARE(list[list.size() - 1], T_CAT);
}

void tst_QList::testOperatorsInt() const
{
    testOperators<int>();
}

void tst_QList::testOperatorsMovable() const
{
    const int liveCount = Movable::getLiveCount();
    testOperators<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::testOperatorsComplex() const
{
    const int liveCount = Complex::getLiveCount();
    testOperators<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QList::testSTLIterators() const
{
    QList<T> list;

    // create a list
    list << T_FOO << T_BAR << T_BAZ;
    typename QList<T>::iterator it = list.begin();
    QCOMPARE(*it, T_FOO); it++;
    QCOMPARE(*it, T_BAR); it++;
    QCOMPARE(*it, T_BAZ); it++;
    QCOMPARE(it, list.end()); it--;

    // walk backwards
    QCOMPARE(*it, T_BAZ); it--;
    QCOMPARE(*it, T_BAR); it--;
    QCOMPARE(*it, T_FOO);

    // test erase
    it = list.erase(it);
    QVERIFY(list.size() == 2);
    QCOMPARE(*it, T_BAR);

    // test multiple erase
    it = list.erase(it, it + 2);
    QVERIFY(list.size() == 0);
    QCOMPARE(it, list.end());

    // insert again
    it = list.insert(it, T_FOO);
    QVERIFY(list.size() == 1);
    QCOMPARE(*it, T_FOO);

    // insert again
    it = list.insert(it, T_BAR);
    QVERIFY(list.size() == 2);
    QCOMPARE(*it++, T_BAR);
    QCOMPARE(*it, T_FOO);
}

void tst_QList::testSTLIteratorsInt() const
{
    testSTLIterators<int>();
}

void tst_QList::testSTLIteratorsMovable() const
{
    const int liveCount = Movable::getLiveCount();
    testSTLIterators<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::testSTLIteratorsComplex() const
{
    const int liveCount = Complex::getLiveCount();
    testSTLIterators<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QList::initializeList() const
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QList<int> v1{2,3,4};
    QCOMPARE(v1, QList<int>() << 2 << 3 << 4);
    QCOMPARE(v1, (QList<int>{2,3,4}));

    QList<QList<int>> v2{ v1, {1}, QList<int>(), {2,3,4}  };
    QList<QList<int>> v3;
    v3 << v1 << (QList<int>() << 1) << QList<int>() << v1;
    QCOMPARE(v3, v2);
#endif
}

template<typename T>
void tst_QList::constSharedNull() const
{
    QList<T> list1;
    list1.setSharable(false);
    QVERIFY(list1.isDetached());

    QList<T> list2;
    list2.setSharable(true);
    QVERIFY(!list2.isDetached());
}

void tst_QList::constSharedNullInt() const
{
    constSharedNull<int>();
}

void tst_QList::constSharedNullMovable() const
{
    const int liveCount = Movable::getLiveCount();
    constSharedNull<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QList::constSharedNullComplex() const
{
    const int liveCount = Complex::getLiveCount();
    constSharedNull<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template <class T>
void generateSetSharableData()
{
    QTest::addColumn<QList<T> >("list");
    QTest::addColumn<int>("size");

    QTest::newRow("null") << QList<T>() << 0;
    QTest::newRow("non-empty") << (QList<T>() << T(0) << T(1) << T(2) << T(3) << T(4)) << 5;
}

template <class T>
void runSetSharableTest()
{
    QFETCH(QList<T>, list);
    QFETCH(int, size);

    QVERIFY(!list.isDetached()); // Shared with QTest

    list.setSharable(true);

    QCOMPARE(list.size(), size);

    {
        QList<T> copy(list);
        QVERIFY(!copy.isDetached());
        QVERIFY(copy.isSharedWith(list));
    }

    list.setSharable(false);
    QVERIFY(list.isDetached() || list.isSharedWith(QList<T>()));

    {
        QList<T> copy(list);

        QVERIFY(copy.isDetached() || copy.isSharedWith(QList<T>()));
        QCOMPARE(copy.size(), size);
        QCOMPARE(copy, list);
    }

    list.setSharable(true);

    {
        QList<T> copy(list);

        QVERIFY(!copy.isDetached());
        QVERIFY(copy.isSharedWith(list));
    }

    for (int i = 0; i < list.size(); ++i)
        QCOMPARE(int(list[i]), i);

    QCOMPARE(list.size(), size);
}

void tst_QList::setSharableInt_data() const
{
    generateSetSharableData<int>();
}

void tst_QList::setSharableComplex_data() const
{
    generateSetSharableData<Complex>();
}

void tst_QList::setSharableInt() const
{
    runSetSharableTest<int>();
}

void tst_QList::setSharableComplex() const
{
    runSetSharableTest<Complex>();
}

QTEST_APPLESS_MAIN(tst_QList)
#include "tst_qlist.moc"
