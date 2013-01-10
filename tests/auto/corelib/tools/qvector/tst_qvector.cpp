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
#include <QAtomicInt>
#include <QThread>
#include <QSemaphore>
#include <qvector.h>

struct Movable {
    Movable(char input = 'j')
        : i(input)
        , state(Constructed)
    {
        counter.fetchAndAddRelaxed(1);
    }
    Movable(const Movable &other)
        : i(other.i)
        , state(Constructed)
    {
        check(other.state, Constructed);
        counter.fetchAndAddRelaxed(1);
    }

    ~Movable()
    {
        check(state, Constructed);
        i = 0;
        counter.fetchAndAddRelaxed(-1);
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
    static QAtomicInt counter;
private:
    enum State { Constructed = 106, Destructed = 110 };
    State state;

    static void check(const State state1, const State state2)
    {
        QCOMPARE(state1, state2);
    }
};

QAtomicInt Movable::counter = 0;
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Movable, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(Movable);

struct Custom {
    Custom(char input = 'j')
        : i(input)
        , that(this)
        , state(Constructed)
    {
        counter.fetchAndAddRelaxed(1);
    }
    Custom(const Custom &other)
        : that(this)
        , state(Constructed)
    {
        check(&other);
        counter.fetchAndAddRelaxed(1);
        this->i = other.i;
    }
    ~Custom()
    {
        check(this);
        i = 0;
        counter.fetchAndAddRelaxed(-1);
        state = Destructed;
    }

    bool operator ==(const Custom &other) const
    {
        check(&other);
        check(this);
        return i == other.i;
    }

    Custom &operator=(const Custom &other)
    {
        check(&other);
        check(this);
        i = other.i;
        return *this;
    }
    static QAtomicInt counter;

    char i;             // used to identify orgin of an instance
private:
    Custom *that;       // used to check if an instance was moved

    enum State { Constructed = 106, Destructed = 110 };
    State state;

    static void check(const Custom *c)
    {
        // check if c object has been moved
        QCOMPARE(c, c->that);
        QCOMPARE(c->state, Constructed);
    }
};
QAtomicInt Custom::counter = 0;

Q_DECLARE_METATYPE(Custom);

// tests depends on the fact that:
Q_STATIC_ASSERT(!QTypeInfo<int>::isStatic);
Q_STATIC_ASSERT(!QTypeInfo<int>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<Movable>::isStatic);
Q_STATIC_ASSERT(QTypeInfo<Movable>::isComplex);
Q_STATIC_ASSERT(QTypeInfo<Custom>::isStatic);
Q_STATIC_ASSERT(QTypeInfo<Custom>::isComplex);


class tst_QVector : public QObject
{
    Q_OBJECT
private slots:
    void constructors_empty() const;
    void constructors_emptyReserveZero() const;
    void constructors_emptyReserve() const;
    void constructors_reserveAndInitialize() const;
    void copyConstructorInt() const;
    void copyConstructorMovable() const;
    void copyConstructorCustom() const;
    void addInt() const;
    void addMovable() const;
    void addCustom() const;
    void appendInt() const;
    void appendMovable() const;
    void appendCustom() const;
    void at() const;
    void capacityInt() const;
    void capacityMovable() const;
    void capacityCustom() const;
    void clearInt() const;
    void clearMovable() const;
    void clearCustom() const;
    void constData() const;
    void contains() const;
    void countInt() const;
    void countMovable() const;
    void countCustom() const;
    void data() const;
    void emptyInt() const;
    void emptyMovable() const;
    void emptyCustom() const;
    void endsWith() const;
    void eraseEmptyInt() const;
    void eraseEmptyMovable() const;
    void eraseEmptyCustom() const;
    void eraseEmptyReservedInt() const;
    void eraseEmptyReservedMovable() const;
    void eraseEmptyReservedCustom() const;
    void eraseInt() const;
    void eraseIntShared() const;
    void eraseMovable() const;
    void eraseMovableShared() const;
    void eraseCustom() const;
    void eraseCustomShared() const;
    void eraseReservedInt() const;
    void eraseReservedMovable() const;
    void eraseReservedCustom() const;
    void fillInt() const;
    void fillMovable() const;
    void fillCustom() const;
    void first() const;
    void fromListInt() const;
    void fromListMovable() const;
    void fromListCustom() const;
    void fromStdVector() const;
    void indexOf() const;
    void insert() const;
    void isEmpty() const;
    void last() const;
    void lastIndexOf() const;
    void mid() const;
    void prependInt() const;
    void prependMovable() const;
    void prependCustom() const;
    void removeInt() const;
    void removeMovable() const;
    void removeCustom() const;
    void removeFirstLast() const;
    void resizePOD_data() const;
    void resizePOD() const;
    void resizeComplexMovable_data() const;
    void resizeComplexMovable() const;
    void resizeComplex_data() const;
    void resizeComplex() const;
    void resizeCtorAndDtor() const;
    void sizeInt() const;
    void sizeMovable() const;
    void sizeCustom() const;
    void startsWith() const;
    void swapInt() const;
    void swapMovable() const;
    void swapCustom() const;
    void toList() const;
    void toStdVector() const;
    void value() const;

    void testOperators() const;

    void reserve();
    void reallocAfterCopy_data();
    void reallocAfterCopy();
    void initializeListInt();
    void initializeListMovable();
    void initializeListCustom();

    void const_shared_null();
    void setSharableInt_data();
    void setSharableInt();
    void setSharableMovable_data();
    void setSharableMovable();
    void setSharableCustom_data();
    void setSharableCustom();

    void detachInt() const;
    void detachMovable() const;
    void detachCustom() const;
    void detachThreadSafetyInt() const;
    void detachThreadSafetyMovable() const;
    void detachThreadSafetyCustom() const;

private:
    template<typename T> void copyConstructor() const;
    template<typename T> void add() const;
    template<typename T> void append() const;
    template<typename T> void capacity() const;
    template<typename T> void clear() const;
    template<typename T> void count() const;
    template<typename T> void empty() const;
    template<typename T> void eraseEmpty() const;
    template<typename T> void eraseEmptyReserved() const;
    template<typename T> void erase(bool shared) const;
    template<typename T> void eraseReserved() const;
    template<typename T> void fill() const;
    template<typename T> void fromList() const;
    template<typename T> void prepend() const;
    template<typename T> void remove() const;
    template<typename T> void size() const;
    template<typename T> void swap() const;
    template<typename T> void initializeList();
    template<typename T> void setSharable_data() const;
    template<typename T> void setSharable() const;
    template<typename T> void detach() const;
    template<typename T> void detachThreadSafety() const;
};


template<typename T> struct SimpleValue
{
    static T at(int index)
    {
        return Values[index % MaxIndex];
    }

    static QVector<T> vector(int size)
    {
        QVector<T> ret;
        for (int i = 0; i < size; i++)
            ret.append(at(i));
        return ret;
    }

    static const uint MaxIndex = 6;
    static const T Values[MaxIndex];
};

template<>
const int SimpleValue<int>::Values[] = { 110, 105, 101, 114, 111, 98 };
template<>
const Movable SimpleValue<Movable>::Values[] = { 110, 105, 101, 114, 111, 98 };
template<>
const Custom SimpleValue<Custom>::Values[] = { 110, 105, 101, 114, 111, 98 };

void tst_QVector::constructors_empty() const
{
    QVector<int> emptyInt;
    QVector<Movable> emptyMovable;
    QVector<Custom> emptyCustom;
}

void tst_QVector::constructors_emptyReserveZero() const
{
    QVector<int> emptyInt(0);
    QVector<Movable> emptyMovable(0);
    QVector<Custom> emptyCustom(0);
}

void tst_QVector::constructors_emptyReserve() const
{
    // pre-reserve capacity
    QVector<int> myInt(5);
    QVERIFY(myInt.capacity() == 5);
    QVector<Movable> myMovable(5);
    QVERIFY(myMovable.capacity() == 5);
    QVector<Custom> myCustom(4);
    QVERIFY(myCustom.capacity() == 4);
}

void tst_QVector::constructors_reserveAndInitialize() const
{
    // default-initialise items

    QVector<int> myInt(5, 42);
    QVERIFY(myInt.capacity() == 5);
    foreach (int meaningoflife, myInt) {
        QCOMPARE(meaningoflife, 42);
    }

    QVector<QString> myString(5, QString::fromLatin1("c++"));
    QVERIFY(myString.capacity() == 5);
    // make sure all items are initialised ok
    foreach (QString meaningoflife, myString) {
        QCOMPARE(meaningoflife, QString::fromLatin1("c++"));
    }

    QVector<Custom> myCustom(5, Custom('n'));
    QVERIFY(myCustom.capacity() == 5);
    // make sure all items are initialised ok
    foreach (Custom meaningoflife, myCustom) {
        QCOMPARE(meaningoflife.i, 'n');
    }
}

template<typename T>
void tst_QVector::copyConstructor() const
{
    T value1(SimpleValue<T>::at(0));
    T value2(SimpleValue<T>::at(1));
    T value3(SimpleValue<T>::at(2));
    T value4(SimpleValue<T>::at(3));
    {
        QVector<T> v1;
        QVector<T> v2(v1);
        QCOMPARE(v1, v2);
    }
    {
        QVector<T> v1;
        v1.setSharable(false);
        QVector<T> v2(v1);
        QVERIFY(!v1.isSharedWith(v2));
        QCOMPARE(v1, v2);
    }
    {
        QVector<T> v1;
        v1 << value1 << value2 << value3 << value4;
        QVector<T> v2(v1);
        QCOMPARE(v1, v2);
    }
    {
        QVector<T> v1;
        v1 << value1 << value2 << value3 << value4;
        v1.setSharable(false);
        QVector<T> v2(v1);
        QVERIFY(!v1.isSharedWith(v2));
        QCOMPARE(v1, v2);
    }
}

void tst_QVector::copyConstructorInt() const
{
    copyConstructor<int>();
}

void tst_QVector::copyConstructorMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    copyConstructor<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::copyConstructorCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    copyConstructor<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QVector::add() const
{
    {
        QVector<T> empty1;
        QVector<T> empty2;
        QVERIFY((empty1 + empty2).isEmpty());
        empty1 += empty2;
        QVERIFY(empty1.isEmpty());
        QVERIFY(empty2.isEmpty());
    }
    {
        QVector<T> v(12);
        QVector<T> empty;
        QCOMPARE((v + empty), v);
        v += empty;
        QVERIFY(!v.isEmpty());
        QCOMPARE(v.size(), 12);
        QVERIFY(empty.isEmpty());
    }
    {
        QVector<T> v1(12);
        QVector<T> v2;
        v2 += v1;
        QVERIFY(!v1.isEmpty());
        QCOMPARE(v1.size(), 12);
        QVERIFY(!v2.isEmpty());
        QCOMPARE(v2.size(), 12);
    }
}

void tst_QVector::addInt() const
{
    add<int>();
}

void tst_QVector::addMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    add<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::addCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    add<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QVector::append() const
{
    {
        QVector<T> myvec;
        myvec.append(SimpleValue<T>::at(0));
        QVERIFY(myvec.size() == 1);
        myvec.append(SimpleValue<T>::at(1));
        QVERIFY(myvec.size() == 2);
        myvec.append(SimpleValue<T>::at(2));
        QVERIFY(myvec.size() == 3);

        QCOMPARE(myvec, QVector<T>() << SimpleValue<T>::at(0)
                                     << SimpleValue<T>::at(1)
                                     << SimpleValue<T>::at(2));
    }
    {
        QVector<T> v(2);
        v.append(SimpleValue<T>::at(0));
        QVERIFY(v.size() == 3);
        QCOMPARE(v.at(v.size() - 1), SimpleValue<T>::at(0));
    }
    {
        QVector<T> v(2);
        v.reserve(12);
        v.append(SimpleValue<T>::at(0));
        QVERIFY(v.size() == 3);
        QCOMPARE(v.at(v.size() - 1), SimpleValue<T>::at(0));
    }
    {
        QVector<T> v(2);
        v.reserve(12);
        v.setSharable(false);
        v.append(SimpleValue<T>::at(0));
        QVERIFY(v.size() == 3);
        QCOMPARE(v.last(), SimpleValue<T>::at(0));
    }
}

void tst_QVector::appendInt() const
{
    append<int>();
}

void tst_QVector::appendMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    append<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::appendCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    append<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::capacity() const
{
    QVector<T> myvec;

    // TODO: is this guaranteed? seems a safe assumption, but I suppose preallocation of a
    // few items isn't an entirely unforseeable possibility.
    QVERIFY(myvec.capacity() == 0);

    // test it gets a size
    myvec << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2);
    QVERIFY(myvec.capacity() >= 3);

    // make sure it grows ok
    myvec << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2);
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

void tst_QVector::capacityInt() const
{
    capacity<int>();
}

void tst_QVector::capacityMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    capacity<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::capacityCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    capacity<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QVector::clear() const
{
    QVector<T> myvec;
    myvec << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2);

    QVERIFY(myvec.size() == 3);
    myvec.clear();
    QVERIFY(myvec.size() == 0);
    QVERIFY(myvec.capacity() == 0);
}

void tst_QVector::clearInt() const
{
    clear<int>();
}

void tst_QVector::clearMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    clear<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::clearCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    clear<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::count() const
{
    // total size
    {
        // zero size
        QVector<T> myvec;
        QVERIFY(myvec.count() == 0);

        // grow
        myvec.append(SimpleValue<T>::at(0));
        QVERIFY(myvec.count() == 1);
        myvec.append(SimpleValue<T>::at(1));
        QVERIFY(myvec.count() == 2);

        // shrink
        myvec.remove(0);
        QVERIFY(myvec.count() == 1);
        myvec.remove(0);
        QVERIFY(myvec.count() == 0);
    }

    // count of items
    {
        QVector<T> myvec;
        myvec << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2);

        // initial tests
        QVERIFY(myvec.count(SimpleValue<T>::at(0)) == 1);
        QVERIFY(myvec.count(SimpleValue<T>::at(3)) == 0);

        // grow
        myvec.append(SimpleValue<T>::at(0));
        QVERIFY(myvec.count(SimpleValue<T>::at(0)) == 2);

        // shrink
        myvec.remove(0);
        QVERIFY(myvec.count(SimpleValue<T>::at(0)) == 1);
    }
}

void tst_QVector::countInt() const
{
    count<int>();
}

void tst_QVector::countMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    count<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::countCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    count<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::empty() const
{
    QVector<T> myvec;

    // starts empty
    QVERIFY(myvec.empty());

    // not empty
    myvec.append(SimpleValue<T>::at(2));
    QVERIFY(!myvec.empty());

    // empty again
    myvec.remove(0);
    QVERIFY(myvec.empty());
}

void tst_QVector::emptyInt() const
{
    empty<int>();
}

void tst_QVector::emptyMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    empty<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::emptyCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    empty<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::eraseEmpty() const
{
    {
        QVector<T> v;
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
    {
        QVector<T> v;
        v.setSharable(false);
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
}

void tst_QVector::eraseEmptyInt() const
{
    eraseEmpty<int>();
}

void tst_QVector::eraseEmptyMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    eraseEmpty<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::eraseEmptyCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    eraseEmpty<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QVector::eraseEmptyReserved() const
{
    {
        QVector<T> v;
        v.reserve(10);
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
    {
        QVector<T> v;
        v.reserve(10);
        v.setSharable(false);
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
}

void tst_QVector::eraseEmptyReservedInt() const
{
    eraseEmptyReserved<int>();
}

void tst_QVector::eraseEmptyReservedMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    eraseEmptyReserved<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::eraseEmptyReservedCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    eraseEmptyReserved<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
struct SharedVectorChecker
{
    SharedVectorChecker(const QVector<T> &original, bool doCopyVector)
        : originalSize(-1),
          copy(0)
    {
        if (doCopyVector) {
            originalSize = original.size();
            copy = new QVector<T>(original);
            // this is unlikely to fail, but if the check in the destructor fails it's good to know that
            // we were still alright here.
            QCOMPARE(originalSize, copy->size());
        }
    }

    ~SharedVectorChecker()
    {
        if (copy)
            QCOMPARE(copy->size(), originalSize);
        delete copy;
    }

    int originalSize;
    QVector<T> *copy;
};

template<typename T>
void tst_QVector::erase(bool shared) const
{
    // note: remove() is actually more efficient, and more dangerous, because it uses the non-detaching
    // begin() / end() internally. you can also use constBegin() and constEnd() with erase(), but only
    // using reinterpret_cast... because both iterator types are really just pointers.
    // so we use a mix of erase() and remove() to cover more cases.
    {
        QVector<T> v = SimpleValue<T>::vector(12);
        SharedVectorChecker<T> svc(v, shared);
        v.erase(v.begin());
        QCOMPARE(v.size(), 11);
        for (int i = 0; i < 11; i++)
            QCOMPARE(v.at(i), SimpleValue<T>::at(i + 1));
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
        if (shared)
            QCOMPARE(SimpleValue<T>::vector(12), *svc.copy);
    }
    {
        QVector<T> v = SimpleValue<T>::vector(12);
        SharedVectorChecker<T> svc(v, shared);
        v.remove(1);
        QCOMPARE(v.size(), 11);
        QCOMPARE(v.at(0), SimpleValue<T>::at(0));
        for (int i = 1; i < 11; i++)
            QCOMPARE(v.at(i), SimpleValue<T>::at(i + 1));
        v.erase(v.begin() + 1, v.end());
        QCOMPARE(v.size(), 1);
        QCOMPARE(v.at(0), SimpleValue<T>::at(0));
        if (shared)
            QCOMPARE(SimpleValue<T>::vector(12), *svc.copy);
    }
    {
        QVector<T> v = SimpleValue<T>::vector(12);
        SharedVectorChecker<T> svc(v, shared);
        v.erase(v.begin(), v.end() - 1);
        QCOMPARE(v.size(), 1);
        QCOMPARE(v.at(0), SimpleValue<T>::at(11));
        if (shared)
            QCOMPARE(SimpleValue<T>::vector(12), *svc.copy);
    }
    {
        QVector<T> v = SimpleValue<T>::vector(12);
        SharedVectorChecker<T> svc(v, shared);
        v.remove(5);
        QCOMPARE(v.size(), 11);
        for (int i = 0; i < 5; i++)
            QCOMPARE(v.at(i), SimpleValue<T>::at(i));
        for (int i = 5; i < 11; i++)
            QCOMPARE(v.at(i), SimpleValue<T>::at(i + 1));
        v.erase(v.begin() + 1, v.end() - 1);
        QCOMPARE(v.at(0), SimpleValue<T>::at(0));
        QCOMPARE(v.at(1), SimpleValue<T>::at(11));
        QCOMPARE(v.size(), 2);
        if (shared)
            QCOMPARE(SimpleValue<T>::vector(12), *svc.copy);
    }
    {
        QVector<T> v = SimpleValue<T>::vector(10);
        SharedVectorChecker<T> svc(v, shared);
        v.setSharable(false);
        SharedVectorChecker<T> svc2(v, shared);
        v.erase(v.begin() + 3);
        QCOMPARE(v.size(), 9);
        v.erase(v.begin(), v.end() - 1);
        QCOMPARE(v.size(), 1);
        if (shared)
            QCOMPARE(SimpleValue<T>::vector(10), *svc.copy);
    }
}

void tst_QVector::eraseInt() const
{
    erase<int>(false);
}

void tst_QVector::eraseIntShared() const
{
    erase<int>(true);
}

void tst_QVector::eraseMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    erase<Movable>(false);
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::eraseMovableShared() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    erase<Movable>(true);
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::eraseCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    erase<Custom>(false);
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QVector::eraseCustomShared() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    erase<Custom>(true);
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T> void tst_QVector::eraseReserved() const
{
    {
        QVector<T> v(12);
        v.reserve(16);
        v.erase(v.begin());
        QCOMPARE(v.size(), 11);
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
    {
        QVector<T> v(12);
        v.reserve(16);
        v.erase(v.begin() + 1);
        QCOMPARE(v.size(), 11);
        v.erase(v.begin() + 1, v.end());
        QCOMPARE(v.size(), 1);
    }
    {
        QVector<T> v(12);
        v.reserve(16);
        v.erase(v.begin(), v.end() - 1);
        QCOMPARE(v.size(), 1);
    }
    {
        QVector<T> v(12);
        v.reserve(16);
        v.erase(v.begin() + 5);
        QCOMPARE(v.size(), 11);
        v.erase(v.begin() + 1, v.end() - 1);
        QCOMPARE(v.size(), 2);
    }
    {
        QVector<T> v(10);
        v.reserve(16);
        v.setSharable(false);
        v.erase(v.begin() + 3);
        QCOMPARE(v.size(), 9);
        v.erase(v.begin(), v.end() - 1);
        QCOMPARE(v.size(), 1);
    }
}

void tst_QVector::eraseReservedInt() const
{
    eraseReserved<int>();
}

void tst_QVector::eraseReservedMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    eraseReserved<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::eraseReservedCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    eraseReserved<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QVector::fill() const
{
    QVector<T> myvec;

    // resize
    myvec.resize(5);
    myvec.fill(SimpleValue<T>::at(1));
    QCOMPARE(myvec, QVector<T>() << SimpleValue<T>::at(1) << SimpleValue<T>::at(1)
                                 << SimpleValue<T>::at(1) << SimpleValue<T>::at(1)
                                 << SimpleValue<T>::at(1));

    // make sure it can resize itself too
    myvec.fill(SimpleValue<T>::at(2), 10);
    QCOMPARE(myvec, QVector<T>() << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2));
}

void tst_QVector::fillInt() const
{
    fill<int>();
}

void tst_QVector::fillMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    fill<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::fillCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    fill<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::fromList() const
{
    QList<T> list;
    list << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3);

    QVector<T> myvec;
    myvec = QVector<T>::fromList(list);

    // test it worked ok
    QCOMPARE(myvec, QVector<T>() << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3));
    QCOMPARE(list, QList<T>() << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3));
}

void tst_QVector::fromListInt() const
{
    fromList<int>();
}

void tst_QVector::fromListMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    fromList<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::fromListCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    fromList<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::prepend() const
{
    QVector<T> myvec;
    T val1 = SimpleValue<T>::at(0);
    T val2 = SimpleValue<T>::at(1);
    T val3 = SimpleValue<T>::at(2);
    T val4 = SimpleValue<T>::at(3);
    T val5 = SimpleValue<T>::at(4);
    myvec << val1 << val2 << val3;

    // starts ok
    QVERIFY(myvec.size() == 3);
    QCOMPARE(myvec.at(0), val1);

    // add something
    myvec.prepend(val4);
    QCOMPARE(myvec.at(0), val4);
    QCOMPARE(myvec.at(1), val1);
    QVERIFY(myvec.size() == 4);

    // something else
    myvec.prepend(val5);
    QCOMPARE(myvec.at(0), val5);
    QCOMPARE(myvec.at(1), val4);
    QCOMPARE(myvec.at(2), val1);
    QVERIFY(myvec.size() == 5);

    // clear and prepend to an empty vector
    myvec.clear();
    QVERIFY(myvec.size() == 0);
    myvec.prepend(val5);
    QVERIFY(myvec.size() == 1);
    QCOMPARE(myvec.at(0), val5);
}

void tst_QVector::prependInt() const
{
    prepend<int>();
}

void tst_QVector::prependMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    prepend<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::prependCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    prepend<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QVector::remove() const
{
    QVector<T> myvec;
    T val1 = SimpleValue<T>::at(1);
    T val2 = SimpleValue<T>::at(2);
    T val3 = SimpleValue<T>::at(3);
    myvec << val1 << val2 << val3;
    // remove middle
    myvec.remove(1);
    QCOMPARE(myvec, QVector<T>() << val1 << val3);

    // remove rest
    myvec.remove(0, 2);
    QCOMPARE(myvec, QVector<T>());
}

void tst_QVector::removeInt() const
{
    remove<int>();
}

void tst_QVector::removeMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    remove<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::removeCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    remove<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

struct RemoveLastTestClass
{
    RemoveLastTestClass() { other = 0; deleted = false; }
    RemoveLastTestClass *other;
    bool deleted;
    ~RemoveLastTestClass()
    {
        deleted = true;
        if (other)
            other->other = 0;
    }
};

void tst_QVector::removeFirstLast() const
{
    // pop_pack - pop_front
    QVector<int> t, t2;
    t.append(1);
    t.append(2);
    t.append(3);
    t.append(4);
    t2 = t;
    t.pop_front();
    QCOMPARE(t.size(), 3);
    QCOMPARE(t.at(0), 2);
    t.pop_back();
    QCOMPARE(t.size(), 2);
    QCOMPARE(t.at(0), 2);
    QCOMPARE(t.at(1), 3);

    // takefirst - takeLast
    int n1 = t2.takeLast();
    QCOMPARE(t2.size(), 3);
    QCOMPARE(n1, 4);
    QCOMPARE(t2.at(0), 1);
    QCOMPARE(t2.at(2), 3);
    n1 = t2.takeFirst();
    QCOMPARE(t2.size(), 2);
    QCOMPARE(n1, 1);
    QCOMPARE(t2.at(0), 2);
    QCOMPARE(t2.at(1), 3);

    // remove first
    QVector<int> x, y;
    x.append(1);
    x.append(2);
    y = x;
    x.removeFirst();
    QCOMPARE(x.size(), 1);
    QCOMPARE(y.size(), 2);
    QCOMPARE(x.at(0), 2);

    // remove Last
    QVector<RemoveLastTestClass> v;
    v.resize(2);
    v[0].other = &(v[1]);
    v[1].other = &(v[0]);
    // Check dtor - complex type
    QVERIFY(v.at(0).other != 0);
    v.removeLast();
    QVERIFY(v.at(0).other == 0);
    QCOMPARE(v.at(0).deleted, false);
    // check iterator
    int count = 0;
    for (QVector<RemoveLastTestClass>::const_iterator i = v.constBegin(); i != v.constEnd(); ++i) {
        ++count;
        QVERIFY(i->other == 0);
        QCOMPARE(i->deleted, false);
    }
    // Check size
    QCOMPARE(count, 1);
    QCOMPARE(v.size(), 1);
    v.removeLast();
    QCOMPARE(v.size(), 0);
    // Check if we do correct realloc
    QVector<int> v2, v3;
    v2.append(1);
    v2.append(2);
    v3 = v2; // shared
    v2.removeLast();
    QCOMPARE(v2.size(), 1);
    QCOMPARE(v3.size(), 2);
    QCOMPARE(v2.at(0), 1);
    QCOMPARE(v3.at(0), 1);
    QCOMPARE(v3.at(1), 2);

    // Remove last with shared
    QVector<int> z1, z2;
    z1.append(9);
    z2 = z1;
    z1.removeLast();
    QCOMPARE(z1.size(), 0);
    QCOMPARE(z2.size(), 1);
    QCOMPARE(z2.at(0), 9);
}


void tst_QVector::resizePOD_data() const
{
    QTest::addColumn<QVector<int> >("vector");
    QTest::addColumn<int>("size");

    QVERIFY(!QTypeInfo<int>::isComplex);
    QVERIFY(!QTypeInfo<int>::isStatic);

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

    QVector<int> nullNotShared;
    QVector<int> emptyNotShared(0, 5);
    QVector<int> emptyReservedNotShared;
    QVector<int> nonEmptyNotShared;
    QVector<int> nonEmptyReservedNotShared;

    emptyReservedNotShared.reserve(10);
    nonEmptyReservedNotShared.reserve(15);
    nonEmptyNotShared << 0 << 1 << 2 << 3 << 4;
    nonEmptyReservedNotShared << 0 << 1 << 2 << 3 << 4 << 5 << 6;
    QVERIFY(emptyReservedNotShared.capacity() >= 10);
    QVERIFY(nonEmptyReservedNotShared.capacity() >= 15);

    emptyNotShared.setSharable(false);
    emptyReservedNotShared.setSharable(false);
    nonEmptyNotShared.setSharable(false);
    nonEmptyReservedNotShared.setSharable(false);

    QTest::newRow("null") << null << 10;
    QTest::newRow("empty") << empty << 10;
    QTest::newRow("emptyReserved") << emptyReserved << 10;
    QTest::newRow("nonEmpty") << nonEmpty << 10;
    QTest::newRow("nonEmptyReserved") << nonEmptyReserved << 10;
    QTest::newRow("nullNotShared") << nullNotShared << 10;
    QTest::newRow("emptyNotShared") << emptyNotShared << 10;
    QTest::newRow("emptyReservedNotShared") << emptyReservedNotShared << 10;
    QTest::newRow("nonEmptyNotShared") << nonEmptyNotShared << 10;
    QTest::newRow("nonEmptyReservedNotShared") << nonEmptyReservedNotShared << 10;
}

void tst_QVector::resizePOD() const
{
    QFETCH(QVector<int>, vector);
    QFETCH(int, size);

    const int oldSize = vector.size();

    vector.resize(size);
    QCOMPARE(vector.size(), size);
    QVERIFY(vector.capacity() >= size);
    for (int i = oldSize; i < size; ++i)
        QVERIFY(vector[i] == 0); // check initialization

    const int capacity = vector.capacity();

    vector.resize(0);
    QCOMPARE(vector.size(), 0);
    QVERIFY(vector.capacity() <= capacity);
}

void tst_QVector::resizeComplexMovable_data() const
{
    QTest::addColumn<QVector<Movable> >("vector");
    QTest::addColumn<int>("size");

    QVERIFY(QTypeInfo<Movable>::isComplex);
    QVERIFY(!QTypeInfo<Movable>::isStatic);

    QVector<Movable> null;
    QVector<Movable> empty(0, 'Q');
    QVector<Movable> emptyReserved;
    QVector<Movable> nonEmpty;
    QVector<Movable> nonEmptyReserved;

    emptyReserved.reserve(10);
    nonEmptyReserved.reserve(15);
    nonEmpty << '0' << '1' << '2' << '3' << '4';
    nonEmptyReserved << '0' << '1' << '2' << '3' << '4' << '5' << '6';
    QVERIFY(emptyReserved.capacity() >= 10);
    QVERIFY(nonEmptyReserved.capacity() >= 15);

    QVector<Movable> nullNotShared;
    QVector<Movable> emptyNotShared(0, 'Q');
    QVector<Movable> emptyReservedNotShared;
    QVector<Movable> nonEmptyNotShared;
    QVector<Movable> nonEmptyReservedNotShared;

    emptyReservedNotShared.reserve(10);
    nonEmptyReservedNotShared.reserve(15);
    nonEmptyNotShared << '0' << '1' << '2' << '3' << '4';
    nonEmptyReservedNotShared << '0' << '1' << '2' << '3' << '4' << '5' << '6';
    QVERIFY(emptyReservedNotShared.capacity() >= 10);
    QVERIFY(nonEmptyReservedNotShared.capacity() >= 15);

    emptyNotShared.setSharable(false);
    emptyReservedNotShared.setSharable(false);
    nonEmptyNotShared.setSharable(false);
    nonEmptyReservedNotShared.setSharable(false);

    QTest::newRow("null") << null << 10;
    QTest::newRow("empty") << empty << 10;
    QTest::newRow("emptyReserved") << emptyReserved << 10;
    QTest::newRow("nonEmpty") << nonEmpty << 10;
    QTest::newRow("nonEmptyReserved") << nonEmptyReserved << 10;
    QTest::newRow("nullNotShared") << nullNotShared << 10;
    QTest::newRow("emptyNotShared") << emptyNotShared << 10;
    QTest::newRow("emptyReservedNotShared") << emptyReservedNotShared << 10;
    QTest::newRow("nonEmptyNotShared") << nonEmptyNotShared << 10;
    QTest::newRow("nonEmptyReservedNotShared") << nonEmptyReservedNotShared << 10;
}

void tst_QVector::resizeComplexMovable() const
{
    const int items = Movable::counter.loadAcquire();
    {
        QFETCH(QVector<Movable>, vector);
        QFETCH(int, size);

        const int oldSize = vector.size();

        vector.resize(size);
        QCOMPARE(vector.size(), size);
        QVERIFY(vector.capacity() >= size);
        for (int i = oldSize; i < size; ++i)
            QVERIFY(vector[i] == 'j'); // check initialization

        const int capacity = vector.capacity();

        vector.resize(0);
        QCOMPARE(vector.size(), 0);
        QVERIFY(vector.capacity() <= capacity);
    }
    QCOMPARE(items, Movable::counter.loadAcquire());
}

void tst_QVector::resizeComplex_data() const
{
    QTest::addColumn<QVector<Custom> >("vector");
    QTest::addColumn<int>("size");

    QVERIFY(QTypeInfo<Custom>::isComplex);
    QVERIFY(QTypeInfo<Custom>::isStatic);

    QVector<Custom> null;
    QVector<Custom> empty(0, '0');
    QVector<Custom> emptyReserved;
    QVector<Custom> nonEmpty;
    QVector<Custom> nonEmptyReserved;

    emptyReserved.reserve(10);
    nonEmptyReserved.reserve(15);
    nonEmpty << '0' << '1' << '2' << '3' << '4';
    nonEmptyReserved << '0' << '1' << '2' << '3' << '4' << '5' << '6';
    QVERIFY(emptyReserved.capacity() >= 10);
    QVERIFY(nonEmptyReserved.capacity() >= 15);

    QVector<Custom> nullNotShared;
    QVector<Custom> emptyNotShared(0, '0');
    QVector<Custom> emptyReservedNotShared;
    QVector<Custom> nonEmptyNotShared;
    QVector<Custom> nonEmptyReservedNotShared;

    emptyReservedNotShared.reserve(10);
    nonEmptyReservedNotShared.reserve(15);
    nonEmptyNotShared << '0' << '1' << '2' << '3' << '4';
    nonEmptyReservedNotShared << '0' << '1' << '2' << '3' << '4' << '5' << '6';
    QVERIFY(emptyReservedNotShared.capacity() >= 10);
    QVERIFY(nonEmptyReservedNotShared.capacity() >= 15);

    emptyNotShared.setSharable(false);
    emptyReservedNotShared.setSharable(false);
    nonEmptyNotShared.setSharable(false);
    nonEmptyReservedNotShared.setSharable(false);

    QTest::newRow("null") << null << 10;
    QTest::newRow("empty") << empty << 10;
    QTest::newRow("emptyReserved") << emptyReserved << 10;
    QTest::newRow("nonEmpty") << nonEmpty << 10;
    QTest::newRow("nonEmptyReserved") << nonEmptyReserved << 10;
    QTest::newRow("nullNotShared") << nullNotShared << 10;
    QTest::newRow("emptyNotShared") << emptyNotShared << 10;
    QTest::newRow("emptyReservedNotShared") << emptyReservedNotShared << 10;
    QTest::newRow("nonEmptyNotShared") << nonEmptyNotShared << 10;
    QTest::newRow("nonEmptyReservedNotShared") << nonEmptyReservedNotShared << 10;
}

void tst_QVector::resizeComplex() const
{
    const int items = Custom::counter.loadAcquire();
    {
        QFETCH(QVector<Custom>, vector);
        QFETCH(int, size);

        int oldSize = vector.size();
        vector.resize(size);
        QCOMPARE(vector.size(), size);
        QVERIFY(vector.capacity() >= size);
        for (int i = oldSize; i < size; ++i)
            QVERIFY(vector[i].i == 'j'); // check default initialization

        const int capacity = vector.capacity();

        vector.resize(0);
        QCOMPARE(vector.size(), 0);
        QVERIFY(vector.isEmpty());
        QVERIFY(vector.capacity() <= capacity);
    }
    QCOMPARE(Custom::counter.loadAcquire(), items);
}

void tst_QVector::resizeCtorAndDtor() const
{
    const int items = Custom::counter.loadAcquire();
    {
        QVector<Custom> null;
        QVector<Custom> empty(0, '0');
        QVector<Custom> emptyReserved;
        QVector<Custom> nonEmpty;
        QVector<Custom> nonEmptyReserved;

        emptyReserved.reserve(10);
        nonEmptyReserved.reserve(15);
        nonEmpty << '0' << '1' << '2' << '3' << '4';
        nonEmptyReserved << '0' << '1' << '2' << '3' << '4' << '5' << '6';
        QVERIFY(emptyReserved.capacity() >= 10);
        QVERIFY(nonEmptyReserved.capacity() >= 15);

        // start playing with vectors
        null.resize(21);
        nonEmpty.resize(2);
        emptyReserved.resize(0);
        nonEmpty.resize(0);
        nonEmptyReserved.resize(2);
    }
    QCOMPARE(Custom::counter.loadAcquire(), items);
}

template<typename T>
void tst_QVector::size() const
{
    // zero size
    QVector<T> myvec;
    QVERIFY(myvec.size() == 0);

    // grow
    myvec.append(SimpleValue<T>::at(0));
    QVERIFY(myvec.size() == 1);
    myvec.append(SimpleValue<T>::at(1));
    QVERIFY(myvec.size() == 2);

    // shrink
    myvec.remove(0);
    QVERIFY(myvec.size() == 1);
    myvec.remove(0);
    QVERIFY(myvec.size() == 0);
}

void tst_QVector::sizeInt() const
{
    size<int>();
}

void tst_QVector::sizeMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    size<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::sizeCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    size<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::swap() const
{
    QVector<T> v1, v2;
    T val1 = SimpleValue<T>::at(0);
    T val2 = SimpleValue<T>::at(1);
    T val3 = SimpleValue<T>::at(2);
    T val4 = SimpleValue<T>::at(3);
    T val5 = SimpleValue<T>::at(4);
    T val6 = SimpleValue<T>::at(5);
    v1 << val1 << val2 << val3;
    v2 << val4 << val5 << val6;

    v1.swap(v2);
    QCOMPARE(v1,QVector<T>() << val4 << val5 << val6);
    QCOMPARE(v2,QVector<T>() << val1 << val2 << val3);
}

void tst_QVector::swapInt() const
{
    swap<int>();
}

void tst_QVector::swapMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    swap<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::swapCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    swap<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::initializeList()
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    T val1(SimpleValue<T>::at(1));
    T val2(SimpleValue<T>::at(2));
    T val3(SimpleValue<T>::at(3));
    T val4(SimpleValue<T>::at(4));

    QVector<T> v1 {val1, val2, val3};
    QCOMPARE(v1, QVector<T>() << val1 << val2 << val3);
    QCOMPARE(v1, (QVector<T> {val1, val2, val3}));

    QVector<QVector<T>> v2{ v1, {val4}, QVector<T>(), {val1, val2, val3} };
    QVector<QVector<T>> v3;
    v3 << v1 << (QVector<T>() << val4) << QVector<T>() << v1;
    QCOMPARE(v3, v2);
#endif
}

void tst_QVector::initializeListInt()
{
    initializeList<int>();
}

void tst_QVector::initializeListMovable()
{
    const int instancesCount = Movable::counter.loadAcquire();
    initializeList<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::initializeListCustom()
{
    const int instancesCount = Custom::counter.loadAcquire();
    initializeList<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

template<typename T>
void tst_QVector::setSharable_data() const
{
    QTest::addColumn<QVector<T> >("vector");
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("capacity");
    QTest::addColumn<bool>("isCapacityReserved");

    QVector<T> null;
    QVector<T> empty(0, SimpleValue<T>::at(1));
    QVector<T> emptyReserved;
    QVector<T> nonEmpty;
    QVector<T> nonEmptyReserved;

    emptyReserved.reserve(10);
    nonEmptyReserved.reserve(15);

    nonEmpty << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3) << SimpleValue<T>::at(4);
    nonEmptyReserved << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3) << SimpleValue<T>::at(4) << SimpleValue<T>::at(5) << SimpleValue<T>::at(6);

    QVERIFY(emptyReserved.capacity() >= 10);
    QVERIFY(nonEmptyReserved.capacity() >= 15);

    QTest::newRow("null") << null << 0 << 0 << false;
    QTest::newRow("empty") << empty << 0 << 0 << false;
    QTest::newRow("empty, Reserved") << emptyReserved << 0 << 10 << true;
    QTest::newRow("non-empty") << nonEmpty << 5 << 0 << false;
    QTest::newRow("non-empty, Reserved") << nonEmptyReserved << 7 << 15 << true;
}

void tst_QVector::setSharableInt_data()
{
    setSharable_data<int>();
}

void tst_QVector::setSharableMovable_data()
{
    setSharable_data<Movable>();
}

void tst_QVector::setSharableCustom_data()
{
    setSharable_data<Custom>();
}

template<typename T>
void tst_QVector::setSharable() const
{
    QFETCH(QVector<T>, vector);
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
        QVector<T> copy(vector);

        QVERIFY(!copy.isDetached());
        QVERIFY(copy.isSharedWith(vector));
    }

    vector.setSharable(false);
    QVERIFY(vector.isDetached() || vector.isSharedWith(QVector<T>()));

    {
        QVector<T> copy(vector);

        QVERIFY(copy.isDetached() || copy.isEmpty() || copy.isSharedWith(QVector<T>()));
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
        QVector<T> copy(vector);

        QVERIFY(!copy.isDetached());
        QVERIFY(copy.isSharedWith(vector));
    }

    for (int i = 0; i < vector.size(); ++i)
        QCOMPARE(vector[i], SimpleValue<T>::at(i));

    QCOMPARE(vector.size(), size);
    if (isCapacityReserved)
        QVERIFY2(vector.capacity() >= capacity,
                qPrintable(QString("Capacity is %1, expected at least %2.")
                    .arg(vector.capacity())
                    .arg(capacity)));
}

void tst_QVector::setSharableInt()
{
    setSharable<int>();
}

void tst_QVector::setSharableMovable()
{
    const int instancesCount = Movable::counter.loadAcquire();
    setSharable<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::setSharableCustom()
{
    const int instancesCount = Custom::counter.loadAcquire();
    setSharable<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QVector::detach() const
{
    {
        // detach an empty vector
        QVector<T> v;
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 0);
        QCOMPARE(v.capacity(), 0);
    }
    {
        // detach an empty referenced vector
        QVector<T> v;
        QVector<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 0);
        QCOMPARE(v.capacity(), 0);
    }
    {
        // detach a not empty referenced vector
        QVector<T> v(31);
        QVector<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 31);
        QCOMPARE(v.capacity(), 31);
    }
    {
        // detach a not empty vector
        QVector<T> v(31);
        QVERIFY(v.isDetached());
        v.detach(); // detaching a detached vector
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 31);
        QCOMPARE(v.capacity(), 31);
    }
    {
        // detach a not empty vector with preallocated space
        QVector<T> v(3);
        v.reserve(8);
        QVector<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 3);
        QCOMPARE(v.capacity(), 8);
    }
    {
        // detach a not empty vector with preallocated space
        QVector<T> v(3);
        v.reserve(8);
        QVERIFY(v.isDetached());
        v.detach(); // detaching a detached vector
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 3);
        QCOMPARE(v.capacity(), 8);
    }
    {
        // detach a not empty, initialized vector
        QVector<T> v(7, SimpleValue<T>::at(1));
        QVector<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 7);
        for (int i = 0; i < v.size(); ++i)
            QCOMPARE(v[i], SimpleValue<T>::at(1));
    }
    {
        // detach a not empty, initialized vector
        QVector<T> v(7, SimpleValue<T>::at(2));
        QVERIFY(v.isDetached());
        v.detach(); // detaching a detached vector
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 7);
        for (int i = 0; i < v.size(); ++i)
            QCOMPARE(v[i], SimpleValue<T>::at(2));
    }
    {
        // detach a not empty, initialized vector with preallocated space
        QVector<T> v(7, SimpleValue<T>::at(3));
        v.reserve(31);
        QVector<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 7);
        QCOMPARE(v.capacity(), 31);
        for (int i = 0; i < v.size(); ++i)
            QCOMPARE(v[i], SimpleValue<T>::at(3));
    }
}

void tst_QVector::detachInt() const
{
    detach<int>();
}

void tst_QVector::detachMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    detach<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::detachCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    detach<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

static QAtomicPointer<QVector<int> > detachThreadSafetyDataInt;
static QAtomicPointer<QVector<Movable> > detachThreadSafetyDataMovable;
static QAtomicPointer<QVector<Custom> > detachThreadSafetyDataCustom;

template<typename T> QAtomicPointer<QVector<T> > *detachThreadSafetyData();
template<> QAtomicPointer<QVector<int> > *detachThreadSafetyData() { return &detachThreadSafetyDataInt; }
template<> QAtomicPointer<QVector<Movable> > *detachThreadSafetyData() { return &detachThreadSafetyDataMovable; }
template<> QAtomicPointer<QVector<Custom> > *detachThreadSafetyData() { return &detachThreadSafetyDataCustom; }

static QSemaphore detachThreadSafetyLock;

template<typename T>
void tst_QVector::detachThreadSafety() const
{
    delete detachThreadSafetyData<T>()->fetchAndStoreOrdered(new QVector<T>(SimpleValue<T>::vector(400)));

    static const uint threadsCount = 5;

    struct : QThread {
        void run() Q_DECL_OVERRIDE
        {
            QVector<T> copy(*detachThreadSafetyData<T>()->load());
            QVERIFY(!copy.isDetached());
            detachThreadSafetyLock.release();
            detachThreadSafetyLock.acquire(100);
            copy.detach();
        }
    } threads[threadsCount];

    for (uint i = 0; i < threadsCount; ++i)
        threads[i].start();
    QThread::yieldCurrentThread();
    detachThreadSafetyLock.acquire(threadsCount);

    // destroy static original data
    delete detachThreadSafetyData<T>()->fetchAndStoreOrdered(0);

    QVERIFY(threadsCount < 100);
    detachThreadSafetyLock.release(threadsCount * 100);
    QThread::yieldCurrentThread();

    for (uint i = 0; i < threadsCount; ++i)
        threads[i].wait();
}

void tst_QVector::detachThreadSafetyInt() const
{
    for (uint i = 0; i < 128; ++i)
        detachThreadSafety<int>();
}

void tst_QVector::detachThreadSafetyMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    for (uint i = 0; i < 128; ++i) {
        detachThreadSafety<Movable>();
        QCOMPARE(Movable::counter.loadAcquire(), instancesCount);
    }
}

void tst_QVector::detachThreadSafetyCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    for (uint i = 0; i < 128; ++i) {
        detachThreadSafety<Custom>();
        QCOMPARE(Custom::counter.loadAcquire(), instancesCount);
    }
}


QTEST_MAIN(tst_QVector)
#include "tst_qvector.moc"
