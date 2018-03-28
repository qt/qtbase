/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
        , that(this)
        , state(Constructed)
    {
        counter.fetchAndAddRelaxed(1);
    }
    Movable(const Movable &other)
        : i(other.i)
        , that(this)
        , state(Constructed)
    {
        check(other.state, Constructed);
        counter.fetchAndAddRelaxed(1);
    }
    Movable(Movable &&other)
        : i(other.i)
        , that(other.that)
        , state(Constructed)
    {
        check(other.state, Constructed);
        counter.fetchAndAddRelaxed(1);
        other.that = nullptr;
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
        that = this;
        return *this;
    }
    Movable &operator=(Movable &&other)
    {
        check(state, Constructed);
        check(other.state, Constructed);
        i = other.i;
        that = other.that;
        other.that = nullptr;
        return *this;
    }
    bool wasConstructedAt(const Movable *other) const
    {
        return that == other;
    }
    char i;
    static QAtomicInt counter;
private:
    Movable *that;       // used to check if an instance was moved

    enum State { Constructed = 106, Destructed = 110 };
    State state;

    static void check(const State state1, const State state2)
    {
        QCOMPARE(state1, state2);
    }
};

inline uint qHash(const Movable &key, uint seed = 0) { return qHash(key.i, seed); }

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

    bool operator<(const Custom &other) const
    {
        check(&other);
        check(this);
        return i < other.i;
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

inline uint qHash(const Custom &key, uint seed = 0) { return qHash(key.i, seed); }

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
    void assignmentInt() const;
    void assignmentMovable() const;
    void assignmentCustom() const;
    void addInt() const;
    void addMovable() const;
    void addCustom() const;
    void appendInt() const;
    void appendMovable() const;
    void appendCustom() const;
    void appendRvalue() const;
    void at() const;
    void capacityInt() const;
    void capacityMovable() const;
    void capacityCustom() const;
    void clearInt() const;
    void clearMovable() const;
    void clearCustom() const;
    void constData() const;
    void constFirst() const;
    void constLast() const;
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
    void insertInt() const;
    void insertMovable() const;
    void insertCustom() const;
    void isEmpty() const;
    void last() const;
    void lastIndexOf() const;
    void mid() const;
    void moveInt() const;
    void moveMovable() const;
    void moveCustom() const;
    void prependInt() const;
    void prependMovable() const;
    void prependCustom() const;
    void qhashInt() const { qhash<int>(); }
    void qhashMovable() const { qhash<Movable>(); }
    void qhashCustom() const { qhash<Custom>(); }
    void removeAllWithAlias() const;
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
    void reverseIterators() const;
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
    void reserveZero();
    void reallocAfterCopy_data();
    void reallocAfterCopy();
    void initializeListInt();
    void initializeListMovable();
    void initializeListCustom();

    void const_shared_null();
#if 1
    // ### Qt6 remove this section
    void setSharableInt_data();
    void setSharableInt();
    void setSharableMovable_data();
    void setSharableMovable();
    void setSharableCustom_data();
    void setSharableCustom();
#endif

    void detachInt() const;
    void detachMovable() const;
    void detachCustom() const;
    void detachThreadSafetyInt() const;
    void detachThreadSafetyMovable() const;
    void detachThreadSafetyCustom() const;

    void insertMove() const;

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
    template<typename T> void insert() const;
    template<typename T> void qhash() const;
    template<typename T> void move() const;
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

// Make some macros for the tests to use in order to be slightly more readable...
#define T_FOO SimpleValue<T>::at(0)
#define T_BAR SimpleValue<T>::at(1)
#define T_BAZ SimpleValue<T>::at(2)
#define T_CAT SimpleValue<T>::at(3)
#define T_DOG SimpleValue<T>::at(4)
#define T_BLAH SimpleValue<T>::at(5)

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
        v1 << value1 << value2 << value3 << value4;
        QVector<T> v2(v1);
        QCOMPARE(v1, v2);
    }
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
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
        v1.setSharable(false);
        QVector<T> v2(v1);
        QVERIFY(!v1.isSharedWith(v2));
        QCOMPARE(v1, v2);
    }
#endif
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

template <class T>
static inline void testAssignment()
{
    QVector<T> v1(5);
    QCOMPARE(v1.size(), 5);
    QVERIFY(v1.isDetached());

    QVector<T> v2(7);
    QCOMPARE(v2.size(), 7);
    QVERIFY(v2.isDetached());

    QVERIFY(!v1.isSharedWith(v2));

    v1 = v2;

    QVERIFY(!v1.isDetached());
    QVERIFY(!v2.isDetached());
    QVERIFY(v1.isSharedWith(v2));

    const void *const data1 = v1.constData();
    const void *const data2 = v2.constData();

    QCOMPARE(data1, data2);

    v1.clear();

    QVERIFY(v2.isDetached());
    QVERIFY(!v1.isSharedWith(v2));
    QCOMPARE((void *)v2.constData(), data2);
}

void tst_QVector::assignmentInt() const
{
    testAssignment<int>();
}

void tst_QVector::assignmentMovable() const
{
    testAssignment<Movable>();
}

void tst_QVector::assignmentCustom() const
{
    testAssignment<Custom>();
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
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
    {
        QVector<T> v(2);
        v.reserve(12);
        v.setSharable(false);
        v.append(SimpleValue<T>::at(0));
        QVERIFY(v.size() == 3);
        QCOMPARE(v.last(), SimpleValue<T>::at(0));
    }
#endif
    {
        QVector<int> v;
        v << 1 << 2 << 3;
        QVector<int> x;
        x << 4 << 5 << 6;
        v.append(x);

        QVector<int> combined;
        combined << 1 << 2 << 3 << 4 << 5 << 6;

        QCOMPARE(v, combined);
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

void tst_QVector::appendRvalue() const
{
#ifdef Q_COMPILER_RVALUE_REFS
    QVector<QString> v;
    v.append("hello");
    QString world = "world";
    v.append(std::move(world));
    QVERIFY(world.isEmpty());
    QCOMPARE(v.front(), QString("hello"));
    QCOMPARE(v.back(),  QString("world"));
#else
    QSKIP("This test requires that C++11 move semantics support is enabled in the compiler");
#endif
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

    const auto oldCapacity = myvec.capacity();
    QCOMPARE(myvec.size(), 3);
    myvec.clear();
    QCOMPARE(myvec.size(), 0);
    QCOMPARE(myvec.capacity(), oldCapacity);
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
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
    {
        QVector<T> v;
        v.setSharable(false);
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
#endif
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
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
    {
        QVector<T> v;
        v.reserve(10);
        v.setSharable(false);
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
#endif
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
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
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
#endif
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
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
    {
        QVector<T> v(10);
        v.reserve(16);
        v.setSharable(false);
        v.erase(v.begin() + 3);
        QCOMPARE(v.size(), 9);
        v.erase(v.begin(), v.end() - 1);
        QCOMPARE(v.size(), 1);
    }
#endif
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
    QCOMPARE(myvec.constFirst(), 69);

    // test removal changes
    myvec.remove(0);
    QCOMPARE(myvec.first(), 42);
    QCOMPARE(myvec.constFirst(), 42);

    // test prepend changes
    myvec.prepend(23);
    QCOMPARE(myvec.first(), 23);
    QCOMPARE(myvec.constFirst(), 23);
}

void tst_QVector::constFirst() const
{
    QVector<int> myvec;
    myvec << 69 << 42 << 3;

    // test it starts ok
    QCOMPARE(myvec.constFirst(), 69);
    QVERIFY(myvec.isDetached());

    QVector<int> myvecCopy = myvec;
    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    QCOMPARE(myvec.constFirst(), 69);
    QCOMPARE(myvecCopy.constFirst(), 69);

    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    // test removal changes
    myvec.remove(0);
    QVERIFY(myvec.isDetached());
    QVERIFY(!myvec.isSharedWith(myvecCopy));
    QCOMPARE(myvec.constFirst(), 42);
    QCOMPARE(myvecCopy.constFirst(), 69);

    myvecCopy = myvec;
    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    QCOMPARE(myvec.constFirst(), 42);
    QCOMPARE(myvecCopy.constFirst(), 42);

    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    // test prepend changes
    myvec.prepend(23);
    QVERIFY(myvec.isDetached());
    QVERIFY(!myvec.isSharedWith(myvecCopy));
    QCOMPARE(myvec.constFirst(), 23);
    QCOMPARE(myvecCopy.constFirst(), 42);

    myvecCopy = myvec;
    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    QCOMPARE(myvec.constFirst(), 23);
    QCOMPARE(myvecCopy.constFirst(), 23);

    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));
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

template <typename T>
void tst_QVector::insert() const
{
    QVector<T> myvec;
    const T
        tA = SimpleValue<T>::at(0),
        tB = SimpleValue<T>::at(1),
        tC = SimpleValue<T>::at(2),
        tX = SimpleValue<T>::at(3),
        tZ = SimpleValue<T>::at(4),
        tT = SimpleValue<T>::at(5),
        ti = SimpleValue<T>::at(6);
    myvec << tA << tB << tC;
    QVector<T> myvec2 = myvec;

    // first position
    QCOMPARE(myvec.at(0), tA);
    myvec.insert(0, tX);
    QCOMPARE(myvec.at(0), tX);
    QCOMPARE(myvec.at(1), tA);

    QCOMPARE(myvec2.at(0), tA);
    myvec2.insert(myvec2.begin(), tX);
    QCOMPARE(myvec2.at(0), tX);
    QCOMPARE(myvec2.at(1), tA);

    // middle
    myvec.insert(1, tZ);
    QCOMPARE(myvec.at(0), tX);
    QCOMPARE(myvec.at(1), tZ);
    QCOMPARE(myvec.at(2), tA);

    myvec2.insert(myvec2.begin() + 1, tZ);
    QCOMPARE(myvec2.at(0), tX);
    QCOMPARE(myvec2.at(1), tZ);
    QCOMPARE(myvec2.at(2), tA);

    // end
    myvec.insert(5, tT);
    QCOMPARE(myvec.at(5), tT);
    QCOMPARE(myvec.at(4), tC);

    myvec2.insert(myvec2.end(), tT);
    QCOMPARE(myvec2.at(5), tT);
    QCOMPARE(myvec2.at(4), tC);

    // insert a lot of garbage in the middle
    myvec.insert(2, 2, ti);
    QCOMPARE(myvec, QVector<T>() << tX << tZ << ti << ti
             << tA << tB << tC << tT);

    myvec2.insert(myvec2.begin() + 2, 2, ti);
    QCOMPARE(myvec2, myvec);

    // insert from references to the same container:
    myvec.insert(0, 1, myvec[5]);   // inserts tB
    myvec2.insert(0, 1, myvec2[5]); // inserts tB
    QCOMPARE(myvec, QVector<T>() << tB << tX << tZ << ti << ti
             << tA << tB << tC << tT);
    QCOMPARE(myvec2, myvec);

    myvec.insert(0, 1, const_cast<const QVector<T>&>(myvec)[0]);   // inserts tB
    myvec2.insert(0, 1, const_cast<const QVector<T>&>(myvec2)[0]); // inserts tB
    QCOMPARE(myvec, QVector<T>() << tB << tB << tX << tZ << ti << ti
             << tA << tB << tC << tT);
    QCOMPARE(myvec2, myvec);
}

void tst_QVector::insertInt() const
{
    insert<int>();
}

void tst_QVector::insertMovable() const
{
    insert<Movable>();
}

void tst_QVector::insertCustom() const
{
    insert<Custom>();
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
    QCOMPARE(myvec.constLast(), QLatin1String("C"));

    // test it changes ok
    myvec.append(QLatin1String("X"));
    QCOMPARE(myvec.last(), QLatin1String("X"));
    QCOMPARE(myvec.constLast(), QLatin1String("X"));

    // and remove again
    myvec.remove(3);
    QCOMPARE(myvec.last(), QLatin1String("C"));
    QCOMPARE(myvec.constLast(), QLatin1String("C"));
}

void tst_QVector::constLast() const
{
    QVector<int> myvec;
    myvec << 69 << 42 << 3;

    // test it starts ok
    QCOMPARE(myvec.constLast(), 3);
    QVERIFY(myvec.isDetached());

    QVector<int> myvecCopy = myvec;
    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    QCOMPARE(myvec.constLast(), 3);
    QCOMPARE(myvecCopy.constLast(), 3);

    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    // test removal changes
    myvec.removeLast();
    QVERIFY(myvec.isDetached());
    QVERIFY(!myvec.isSharedWith(myvecCopy));
    QCOMPARE(myvec.constLast(), 42);
    QCOMPARE(myvecCopy.constLast(), 3);

    myvecCopy = myvec;
    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    QCOMPARE(myvec.constLast(), 42);
    QCOMPARE(myvecCopy.constLast(), 42);

    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    // test prepend changes
    myvec.append(23);
    QVERIFY(myvec.isDetached());
    QVERIFY(!myvec.isSharedWith(myvecCopy));
    QCOMPARE(myvec.constLast(), 23);
    QCOMPARE(myvecCopy.constLast(), 42);

    myvecCopy = myvec;
    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));

    QCOMPARE(myvec.constLast(), 23);
    QCOMPARE(myvecCopy.constLast(), 23);

    QVERIFY(!myvec.isDetached());
    QVERIFY(!myvecCopy.isDetached());
    QVERIFY(myvec.isSharedWith(myvecCopy));
    QVERIFY(myvecCopy.isSharedWith(myvec));
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
    QCOMPARE(list.mid(6, 10), QVector<QString>() << "kitty");
    QCOMPARE(list.mid(-1, 20), list);
    QCOMPARE(list.mid(4), QVector<QString>() << "buck" << "hello" << "kitty");
}

template <typename T>
void tst_QVector::qhash() const
{
    QVector<T> l1, l2;
    QCOMPARE(qHash(l1), qHash(l2));
    l1 << SimpleValue<T>::at(0);
    l2 << SimpleValue<T>::at(0);
    QCOMPARE(qHash(l1), qHash(l2));
}

template <typename T>
void tst_QVector::move() const
{
    QVector<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // move an item
    list.move(0, list.count() - 1);
    QCOMPARE(list, QVector<T>() << T_BAR << T_BAZ << T_FOO);

    // move it back
    list.move(list.count() - 1, 0);
    QCOMPARE(list, QVector<T>() << T_FOO << T_BAR << T_BAZ);

    // move an item in the middle
    list.move(1, 0);
    QCOMPARE(list, QVector<T>() << T_BAR << T_FOO << T_BAZ);
}

void tst_QVector::moveInt() const
{
    move<int>();
}

void tst_QVector::moveMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    move<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QVector::moveCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    move<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
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

void tst_QVector::removeAllWithAlias() const
{
    QVector<QString> strings;
    strings << "One" << "Two" << "Three" << "One" /* must be distinct, but equal */;
    QCOMPARE(strings.removeAll(strings.front()), 2); // will trigger asan/ubsan
}

template<typename T>
void tst_QVector::remove() const
{
    QVector<T> myvec;
    T val1 = SimpleValue<T>::at(1);
    T val2 = SimpleValue<T>::at(2);
    T val3 = SimpleValue<T>::at(3);
    T val4 = SimpleValue<T>::at(4);
    myvec << val1 << val2 << val3;
    myvec << val1 << val2 << val3;
    myvec << val1 << val2 << val3;
    // remove middle
    myvec.remove(1);
    QCOMPARE(myvec, QVector<T>() << val1 << val3  << val1 << val2 << val3  << val1 << val2 << val3);

    // removeOne()
    QVERIFY(!myvec.removeOne(val4));
    QVERIFY(myvec.removeOne(val2));
    QCOMPARE(myvec, QVector<T>() << val1 << val3  << val1 << val3  << val1 << val2 << val3);

    QVector<T> myvecCopy = myvec;
    QVERIFY(myvecCopy.isSharedWith(myvec));
    // removeAll()
    QCOMPARE(myvec.removeAll(val4), 0);
    QVERIFY(myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec.removeAll(val1), 3);
    QVERIFY(!myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec, QVector<T>() << val3  << val3  << val2 << val3);
    myvecCopy = myvec;
    QVERIFY(myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec.removeAll(val2), 1);
    QVERIFY(!myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec, QVector<T>() << val3  << val3  << val3);

    // remove rest
    myvec.remove(0, 3);
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

    QTest::newRow("null") << null << 10;
    QTest::newRow("empty") << empty << 10;
    QTest::newRow("emptyReserved") << emptyReserved << 10;
    QTest::newRow("nonEmpty") << nonEmpty << 10;
    QTest::newRow("nonEmptyReserved") << nonEmptyReserved << 10;

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
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

    QTest::newRow("nullNotShared") << nullNotShared << 10;
    QTest::newRow("emptyNotShared") << emptyNotShared << 10;
    QTest::newRow("emptyReservedNotShared") << emptyReservedNotShared << 10;
    QTest::newRow("nonEmptyNotShared") << nonEmptyNotShared << 10;
    QTest::newRow("nonEmptyReservedNotShared") << nonEmptyReservedNotShared << 10;
#endif
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

    vector.clear();
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

    QTest::newRow("null") << null << 10;
    QTest::newRow("empty") << empty << 10;
    QTest::newRow("emptyReserved") << emptyReserved << 10;
    QTest::newRow("nonEmpty") << nonEmpty << 10;
    QTest::newRow("nonEmptyReserved") << nonEmptyReserved << 10;

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
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

    QTest::newRow("nullNotShared") << nullNotShared << 10;
    QTest::newRow("emptyNotShared") << emptyNotShared << 10;
    QTest::newRow("emptyReservedNotShared") << emptyReservedNotShared << 10;
    QTest::newRow("nonEmptyNotShared") << nonEmptyNotShared << 10;
    QTest::newRow("nonEmptyReservedNotShared") << nonEmptyReservedNotShared << 10;
#endif
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

    QTest::newRow("null") << null << 10;
    QTest::newRow("empty") << empty << 10;
    QTest::newRow("emptyReserved") << emptyReserved << 10;
    QTest::newRow("nonEmpty") << nonEmpty << 10;
    QTest::newRow("nonEmptyReserved") << nonEmptyReserved << 10;

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
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

    QTest::newRow("nullNotShared") << nullNotShared << 10;
    QTest::newRow("emptyNotShared") << emptyNotShared << 10;
    QTest::newRow("emptyReservedNotShared") << emptyReservedNotShared << 10;
    QTest::newRow("nonEmptyNotShared") << nonEmptyNotShared << 10;
    QTest::newRow("nonEmptyReservedNotShared") << nonEmptyReservedNotShared << 10;
#endif
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

void tst_QVector::reverseIterators() const
{
    QVector<int> v;
    v << 1 << 2 << 3 << 4;
    QVector<int> vr = v;
    std::reverse(vr.begin(), vr.end());
    const QVector<int> &cvr = vr;
    QVERIFY(std::equal(v.begin(), v.end(), vr.rbegin()));
    QVERIFY(std::equal(v.begin(), v.end(), vr.crbegin()));
    QVERIFY(std::equal(v.begin(), v.end(), cvr.rbegin()));
    QVERIFY(std::equal(vr.rbegin(), vr.rend(), v.begin()));
    QVERIFY(std::equal(vr.crbegin(), vr.crend(), v.begin()));
    QVERIFY(std::equal(cvr.rbegin(), cvr.rend(), v.begin()));
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

    // <, >, <=, >=
    QVERIFY(!(myvec <  combined));
    QVERIFY(!(myvec >  combined));
    QVERIFY(  myvec <= combined);
    QVERIFY(  myvec >= combined);
    combined.push_back("G");
    QVERIFY(  myvec <  combined);
    QVERIFY(!(myvec >  combined));
    QVERIFY(  myvec <= combined);
    QVERIFY(!(myvec >= combined));
    QVERIFY(combined >  myvec);
    QVERIFY(combined >= myvec);

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
        QCOMPARE(fooCtor, 2);
        QVector<Foo> b(a);
        b.reserve(1);
        QCOMPARE(b.size(), a.size());
        QCOMPARE(fooDtor, 0);
    }
    QCOMPARE(fooCtor, fooDtor);
}

// This is a regression test for QTBUG-51758
void tst_QVector::reserveZero()
{
    QVector<int> vec;
    vec.detach();
    vec.reserve(0); // should not crash
    QCOMPARE(vec.size(), 0);
    QCOMPARE(vec.capacity(), 0);
    vec.squeeze();
    QCOMPARE(vec.size(), 0);
    QCOMPARE(vec.capacity(), 0);
    vec.reserve(-1);
    QCOMPARE(vec.size(), 0);
    QCOMPARE(vec.capacity(), 0);
    vec.append(42);
    QCOMPARE(vec.size(), 1);
    QVERIFY(vec.capacity() >= 1);
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
        const QByteArray prefix = "reallocAfterCopy:" + QByteArray::number(i) + ',';
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
            QTest::newRow((prefix + QByteArray::number(j)).constData())
                    << i << fill_size << j << result1 << result2 << result3 << result4;
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

    QVector<T> v4({});
    QCOMPARE(v4.size(), 0);
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
    QVector<int> v2;
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    // ### Qt6 remove this section
    QVector<int> v1;
    v1.setSharable(false);
    QVERIFY(v1.isDetached());

    v2.setSharable(true);
#endif
    QVERIFY(!v2.isDetached());
}

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
// ### Qt6 remove this section
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
                        .arg(copy.capacity())
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
#else
template<typename T> void tst_QVector::setSharable_data() const
{
}

template<typename T> void tst_QVector::setSharable() const
{
}
#endif

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
        void run() override
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

void tst_QVector::insertMove() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    {
        QVector<Movable> vec;
        vec.reserve(7);
        Movable m0;
        Movable m1;
        Movable m2;
        Movable m3;
        Movable m4;
        Movable m5;
        Movable m6;

        vec.append(std::move(m3));
        QVERIFY(m3.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m3));
        vec.push_back(std::move(m4));
        QVERIFY(m4.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m3));
        QVERIFY(vec.at(1).wasConstructedAt(&m4));
        vec.prepend(std::move(m1));
        QVERIFY(m1.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m3));
        QVERIFY(vec.at(2).wasConstructedAt(&m4));
        vec.insert(1, std::move(m2));
        QVERIFY(m2.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m2));
        QVERIFY(vec.at(2).wasConstructedAt(&m3));
        QVERIFY(vec.at(3).wasConstructedAt(&m4));
        vec += std::move(m5);
        QVERIFY(m5.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m2));
        QVERIFY(vec.at(2).wasConstructedAt(&m3));
        QVERIFY(vec.at(3).wasConstructedAt(&m4));
        QVERIFY(vec.at(4).wasConstructedAt(&m5));
        vec << std::move(m6);
        QVERIFY(m6.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m2));
        QVERIFY(vec.at(2).wasConstructedAt(&m3));
        QVERIFY(vec.at(3).wasConstructedAt(&m4));
        QVERIFY(vec.at(4).wasConstructedAt(&m5));
        QVERIFY(vec.at(5).wasConstructedAt(&m6));
        vec.push_front(std::move(m0));
        QVERIFY(m0.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m0));
        QVERIFY(vec.at(1).wasConstructedAt(&m1));
        QVERIFY(vec.at(2).wasConstructedAt(&m2));
        QVERIFY(vec.at(3).wasConstructedAt(&m3));
        QVERIFY(vec.at(4).wasConstructedAt(&m4));
        QVERIFY(vec.at(5).wasConstructedAt(&m5));
        QVERIFY(vec.at(6).wasConstructedAt(&m6));

        QCOMPARE(Movable::counter.loadAcquire(), instancesCount + 14);
    }
    QCOMPARE(Movable::counter.loadAcquire(), instancesCount);
}

QTEST_MAIN(tst_QVector)
#include "tst_qvector.moc"
