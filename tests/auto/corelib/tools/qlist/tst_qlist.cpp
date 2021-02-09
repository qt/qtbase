/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QTest>
#include <QAtomicInt>
#include <QThread>
#include <QSemaphore>
#include <QScopedValueRollback>
#include <qlist.h>

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
        QCOMPARE(int(state1), int(state2));
    }
};

inline size_t qHash(const Movable &key, size_t seed = 0) { return qHash(key.i, seed); }

QAtomicInt Movable::counter = 0;
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Movable, Q_RELOCATABLE_TYPE);
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
        QCOMPARE(int(c->state), int(Constructed));
    }
};
QAtomicInt Custom::counter = 0;

inline size_t qHash(const Custom &key, size_t seed = 0) { return qHash(key.i, seed); }

Q_DECLARE_METATYPE(Custom);

// tests depends on the fact that:
static_assert(QTypeInfo<int>::isRelocatable);
static_assert(!QTypeInfo<int>::isComplex);
static_assert(QTypeInfo<Movable>::isRelocatable);
static_assert(QTypeInfo<Movable>::isComplex);
static_assert(!QTypeInfo<Custom>::isRelocatable);
static_assert(QTypeInfo<Custom>::isComplex);


class tst_QList : public QObject
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
    void assignFromInitializerListInt() const;
    void assignFromInitializerListMovable() const;
    void assignFromInitializerListCustom() const;
    void addInt() const;
    void addMovable() const;
    void addCustom() const;
    void appendInt() const;
    void appendMovable() const;
    void appendCustom() const;
    void appendRvalue() const;
    void appendList() const;
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
    void cpp17ctad() const;
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
    void fillDetachInt() const;
    void fillDetachMovable() const;
    void fillDetachCustom() const;
    void first() const;
    void fromListInt() const;
    void fromListMovable() const;
    void fromListCustom() const;
    void indexOf() const;
    void insertInt() const;
    void insertMovable() const;
    void insertCustom() const;
    void insertZeroCount_data();
    void insertZeroCount() const;
    void isEmpty() const;
    void last() const;
    void lastIndexOf() const;
    void mid() const;
    void sliced() const;
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
    void resizeToZero() const;
    void resizeToTheSameSize_data();
    void resizeToTheSameSize() const;
    void reverseIterators() const;
    void sizeInt() const;
    void sizeMovable() const;
    void sizeCustom() const;
    void startsWith() const;
    void swapInt() const;
    void swapMovable() const;
    void swapCustom() const;
    void toList() const;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    void fromStdVector() const;
    void toStdVector() const;
#endif
    void value() const;

    void testOperators() const;

    void reserve();
    void reserveZero();
    void initializeListInt();
    void initializeListMovable();
    void initializeListCustom();

    void const_shared_null();

    void detachInt() const;
    void detachMovable() const;
    void detachCustom() const;
    void detachThreadSafetyInt() const;
    void detachThreadSafetyMovable() const;
    void detachThreadSafetyCustom() const;

    void insertMove() const;

    void swapItemsAt() const;

    void emplaceInt();
    void emplaceCustom();
    void emplaceMovable();
    void emplaceConsistentWithStdVectorInt();
    void emplaceConsistentWithStdVectorCustom();
    void emplaceConsistentWithStdVectorMovable();
    void emplaceConsistentWithStdVectorQString();
    void emplaceReturnsIterator();
    void emplaceBack();
    void emplaceBackReturnsRef();
    void emplaceWithElementFromTheSameContainer();
    void emplaceWithElementFromTheSameContainer_data();

    void fromReadOnlyData() const;

    void qtbug_90359() const;

private:
    template<typename T> void copyConstructor() const;
    template<typename T> void add() const;
    template<typename T> void append() const;
    template<typename T> void assignFromInitializerList() const;
    template<typename T> void capacity() const;
    template<typename T> void clear() const;
    template<typename T> void count() const;
    template<typename T> void empty() const;
    template<typename T> void eraseEmpty() const;
    template<typename T> void eraseEmptyReserved() const;
    template<typename T> void erase(bool shared) const;
    template<typename T> void eraseReserved() const;
    template<typename T> void fill() const;
    template<typename T> void fillDetach() const;
    template<typename T> void fromList() const;
    template<typename T> void insert() const;
    template<typename T> void qhash() const;
    template<typename T> void move() const;
    template<typename T> void prepend() const;
    template<typename T> void remove() const;
    template<typename T> void size() const;
    template<typename T> void swap() const;
    template<typename T> void initializeList();
    template<typename T> void detach() const;
    template<typename T> void detachThreadSafety() const;
    template<typename T> void emplaceImpl() const;
    template<typename T> void emplaceConsistentWithStdVectorImpl() const;
};


template<typename T> struct SimpleValue
{
    static T at(int index)
    {
        return Values[index % MaxIndex];
    }

    static QList<T> vector(int size)
    {
        QList<T> ret;
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

void tst_QList::constructors_empty() const
{
    QList<int> emptyInt;
    QList<Movable> emptyMovable;
    QList<Custom> emptyCustom;
}

void tst_QList::constructors_emptyReserveZero() const
{
    QList<int> emptyInt(0);
    QList<Movable> emptyMovable(0);
    QList<Custom> emptyCustom(0);
}

void tst_QList::constructors_emptyReserve() const
{
    // pre-reserve capacity
    QList<int> myInt(5);
    QVERIFY(myInt.capacity() == 5);
    QList<Movable> myMovable(5);
    QVERIFY(myMovable.capacity() == 5);
    QList<Custom> myCustom(4);
    QVERIFY(myCustom.capacity() == 4);
}

void tst_QList::constructors_reserveAndInitialize() const
{
    // default-initialise items

    QList<int> myInt(5, 42);
    QVERIFY(myInt.capacity() == 5);
    foreach (int meaningoflife, myInt) {
        QCOMPARE(meaningoflife, 42);
    }

    QList<QString> myString(5, QString::fromLatin1("c++"));
    QVERIFY(myString.capacity() == 5);
    // make sure all items are initialised ok
    foreach (QString meaningoflife, myString) {
        QCOMPARE(meaningoflife, QString::fromLatin1("c++"));
    }

    QList<Custom> myCustom(5, Custom('n'));
    QVERIFY(myCustom.capacity() == 5);
    // make sure all items are initialised ok
    foreach (Custom meaningoflife, myCustom) {
        QCOMPARE(meaningoflife.i, 'n');
    }
}

template<typename T>
void tst_QList::copyConstructor() const
{
    T value1(SimpleValue<T>::at(0));
    T value2(SimpleValue<T>::at(1));
    T value3(SimpleValue<T>::at(2));
    T value4(SimpleValue<T>::at(3));
    {
        QList<T> v1;
        QList<T> v2(v1);
        QCOMPARE(v1, v2);
    }
    {
        QList<T> v1;
        v1 << value1 << value2 << value3 << value4;
        QList<T> v2(v1);
        QCOMPARE(v1, v2);
    }
}

void tst_QList::copyConstructorInt() const
{
    copyConstructor<int>();
}

void tst_QList::copyConstructorMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    copyConstructor<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::copyConstructorCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    copyConstructor<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template <class T>
static inline void testAssignment()
{
    QList<T> v1(5);
    QCOMPARE(v1.size(), 5);
    QVERIFY(v1.isDetached());

    QList<T> v2(7);
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

void tst_QList::assignmentInt() const
{
    testAssignment<int>();
}

void tst_QList::assignmentMovable() const
{
    testAssignment<Movable>();
}

void tst_QList::assignmentCustom() const
{
    testAssignment<Custom>();
}

template<typename T>
void tst_QList::assignFromInitializerList() const
{
    T val1(SimpleValue<T>::at(1));
    T val2(SimpleValue<T>::at(2));
    T val3(SimpleValue<T>::at(3));

    QList<T> v1 = {val1, val2, val3};
    QCOMPARE(v1, QList<T>() << val1 << val2 << val3);
    QCOMPARE(v1, (QList<T> {val1, val2, val3}));

    v1 = {};
    QCOMPARE(v1.size(), 0);
}

void tst_QList::assignFromInitializerListInt() const
{
    assignFromInitializerList<int>();
}

void tst_QList::assignFromInitializerListMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    assignFromInitializerList<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::assignFromInitializerListCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    assignFromInitializerList<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QList::add() const
{
    {
        QList<T> empty1;
        QList<T> empty2;
        QVERIFY((empty1 + empty2).isEmpty());
        empty1 += empty2;
        QVERIFY(empty1.isEmpty());
        QVERIFY(empty2.isEmpty());
    }
    {
        QList<T> v(12);
        QList<T> empty;
        QCOMPARE((v + empty), v);
        v += empty;
        QVERIFY(!v.isEmpty());
        QCOMPARE(v.size(), 12);
        QVERIFY(empty.isEmpty());
    }
    {
        QList<T> v1(12);
        QList<T> v2;
        v2 += v1;
        QVERIFY(!v1.isEmpty());
        QCOMPARE(v1.size(), 12);
        QVERIFY(!v2.isEmpty());
        QCOMPARE(v2.size(), 12);
    }
}

void tst_QList::addInt() const
{
    add<int>();
}

void tst_QList::addMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    add<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::addCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    add<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QList::append() const
{
    {
        QList<T> myvec;
        myvec.append(SimpleValue<T>::at(0));
        QVERIFY(myvec.size() == 1);
        myvec.append(SimpleValue<T>::at(1));
        QVERIFY(myvec.size() == 2);
        myvec.append(SimpleValue<T>::at(2));
        QVERIFY(myvec.size() == 3);

        QCOMPARE(myvec, QList<T>() << SimpleValue<T>::at(0)
                                     << SimpleValue<T>::at(1)
                                     << SimpleValue<T>::at(2));
    }
    {
        QList<T> v(2);
        v.append(SimpleValue<T>::at(0));
        QVERIFY(v.size() == 3);
        QCOMPARE(v.at(v.size() - 1), SimpleValue<T>::at(0));
    }
    {
        QList<T> v(2);
        v.reserve(12);
        v.append(SimpleValue<T>::at(0));
        QVERIFY(v.size() == 3);
        QCOMPARE(v.at(v.size() - 1), SimpleValue<T>::at(0));
    }
    {
        QList<int> v;
        v << 1 << 2 << 3;
        QList<int> x;
        x << 4 << 5 << 6;
        v.append(x);

        QList<int> combined;
        combined << 1 << 2 << 3 << 4 << 5 << 6;

        QCOMPARE(v, combined);
    }
}

void tst_QList::appendInt() const
{
    append<int>();
}

void tst_QList::appendMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    append<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::appendCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    append<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::appendRvalue() const
{
    QList<QString> v;
    v.append("hello");
    QString world = "world";
    v.append(std::move(world));
    QVERIFY(world.isEmpty());
    QCOMPARE(v.front(), QString("hello"));
    QCOMPARE(v.back(),  QString("world"));
}

struct ConstructionCounted
{
    ConstructionCounted(int i) : i(i) { }
    ConstructionCounted(ConstructionCounted &&other) noexcept
        : i(other.i), copies(other.copies), moves(other.moves + 1)
    {
        // set to some easily noticeable values
        other.i = -64;
        other.copies = -64;
        other.moves = -64;
    }
    ConstructionCounted &operator=(ConstructionCounted &&other) noexcept
    {
        i = other.i;
        copies = other.copies;
        moves = other.moves + 1;
        // set to some easily noticeable values
        other.i = -64;
        other.copies = -64;
        other.moves = -64;
        return *this;
    }
    ConstructionCounted(const ConstructionCounted &other) noexcept
        : i(other.i), copies(other.copies + 1), moves(other.moves)
    {
    }
    ConstructionCounted &operator=(const ConstructionCounted &other) noexcept
    {
        i = other.i;
        copies = other.copies + 1;
        moves = other.moves;
        return *this;
    }
    ~ConstructionCounted() = default;

    friend bool operator==(const ConstructionCounted &lhs, const ConstructionCounted &rhs)
    {
        return lhs.i == rhs.i;
    }

    QString toString() { return QString::number(i); }

    int i;
    int copies = 0;
    int moves = 0;
};
QT_BEGIN_NAMESPACE
namespace QTest {
char *toString(const ConstructionCounted &cc)
{
    char *str = new char[5];
    qsnprintf(str, 4, "%d", cc.i);
    return str;
}
}
QT_END_NAMESPACE

void tst_QList::appendList() const
{
    // By const-ref
    {
        QList<int> v1 = { 1, 2, 3, 4 };
        QList<int> v2 = { 5, 6, 7, 8 };
        v1.append(v2);
        QCOMPARE(v2.size(), 4);
        QCOMPARE(v1.size(), 8);
        QList<int> expected = { 1, 2, 3, 4, 5, 6, 7, 8 };
        QCOMPARE(v1, expected);

        QList<int> doubleExpected = { 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8 };
        // append self to self
        v1.append(v1);
        QCOMPARE(v1.size(), 16);
        QCOMPARE(v1, doubleExpected);
        v1.resize(8);

        // append to self, but was shared
        QList v1_2(v1);
        v1.append(v1);
        QCOMPARE(v1_2.size(), 8);
        QCOMPARE(v1_2, expected);
        QCOMPARE(v1.size(), 16);
        QCOMPARE(v1, doubleExpected);
        v1.resize(8);

        // append empty
        QList<int> v3;
        v1.append(v3);

        // append to empty
        QList<int> v4;
        v4.append(v1);
        QCOMPARE(v4, expected);

        v1 = { 1, 2, 3, 4 };
        // Using operators
        // <<
        QList<int> v5;
        v5 << v1 << v2;
        QCOMPARE(v5, expected);

        // +=
        QList<int> v6;
        v6 += v1;
        v6 += v2;
        QCOMPARE(v6, expected);

        // +
        QCOMPARE(v1 + v2, expected);
    }
    // By move
    {
        QList<ConstructionCounted> v1 = { 1, 2, 3, 4 };
        // Sanity check
        QCOMPARE(v1.at(3).moves, 0);
        QCOMPARE(v1.at(3).copies, 1); // because of initializer list

        QList<ConstructionCounted> v2 = { 5, 6, 7, 8 };
        v1.append(std::move(v2));
        QCOMPARE(v1.size(), 8);
        QList<ConstructionCounted> expected = { 1, 2, 3, 4, 5, 6, 7, 8 };
        QCOMPARE(v1, expected);
        QCOMPARE(v1.at(0).copies, 1);
        QCOMPARE(v1.at(0).moves, 1);

        QCOMPARE(v1.at(4).copies, 1); // was v2.at(0)
        QCOMPARE(v1.at(4).moves, 1);

        // append move from empty
        QList<ConstructionCounted> v3;
        v1.append(std::move(v3));
        QCOMPARE(v1.size(), 8);
        QCOMPARE(v1, expected);

        for (qsizetype i = 0; i < v1.size(); ++i) {
            const auto &counter = v1.at(i);
            QCOMPARE(counter.copies, 1);
            QCOMPARE(counter.moves, 1);
        }

        // append move to empty
        QList<ConstructionCounted> v4;
        v4.reserve(64);
        v4.append(std::move(v1));
        QCOMPARE(v4.size(), 8);
        QCOMPARE(v4, expected);

        for (qsizetype i = 0; i < v4.size(); ++i) {
            const auto &counter = v4.at(i);
            QCOMPARE(counter.copies, 1);
            QCOMPARE(counter.moves, 2);
        }

        QVERIFY(v4.capacity() >= 64);

        v1.swap(v4); // swap back...

        // append move from shared
        QList<ConstructionCounted> v5 = { 1, 2, 3, 4 };
        QList<ConstructionCounted> v5_2(v5);
        v1.append(std::move(v5_2));
        QCOMPARE(v1.size(), 12);
        QList<ConstructionCounted> expectedTwelve = { 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4 };
        QCOMPARE(v1, expectedTwelve);
        QCOMPARE(v5.size(), 4);
        QList<ConstructionCounted> expectedFour = { 1, 2, 3, 4 };
        QCOMPARE(v5, expectedFour);

        QCOMPARE(v5.at(0).copies, 1); // from constructing with std::initializer_list
        QCOMPARE(v5.at(0).moves, 0);

        // Using operators
        // <<
        QList<ConstructionCounted> v6;
        v6 << (QList<ConstructionCounted>() << 1 << 2);
        v6 << (QList<ConstructionCounted>() << 3 << 4);
        QCOMPARE(v6, expectedFour);
        QCOMPARE(v6.at(0).copies, 1);
        QCOMPARE(v6.at(0).moves, 3);

        // +=
        QList<ConstructionCounted> v7;
        v7 += (QList<ConstructionCounted>() << 1 << 2);
        v7 += (QList<ConstructionCounted>() << 3 << 4);
        QCOMPARE(v7, expectedFour);

        // +
        QList<ConstructionCounted> v8;
        QCOMPARE(v8 + (QList<ConstructionCounted>() << 1 << 2 << 3 << 4), expectedFour);
        v8 = { 1, 2 };
        QCOMPARE(v8 + (QList<ConstructionCounted>() << 3 << 4), expectedFour);
    }
}

void tst_QList::at() const
{
    QList<QString> myvec;
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
void tst_QList::capacity() const
{
    QList<T> myvec;

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
    myvec.squeeze();
    QVERIFY(myvec.capacity() >= 3);

    myvec.remove(0);
    myvec.remove(0);
    myvec.remove(0);
    myvec.squeeze();
    QVERIFY(myvec.capacity() == 0);
}

void tst_QList::capacityInt() const
{
    capacity<int>();
}

void tst_QList::capacityMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    capacity<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::capacityCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    capacity<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QList::clear() const
{
    QList<T> myvec;
    myvec << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2);

    const auto oldCapacity = myvec.capacity();
    QCOMPARE(myvec.size(), 3);
    myvec.clear();
    QCOMPARE(myvec.size(), 0);
    QCOMPARE(myvec.capacity(), oldCapacity);
}

void tst_QList::clearInt() const
{
    clear<int>();
}

void tst_QList::clearMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    clear<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::clearCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    clear<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::constData() const
{
    int arr[] = { 42, 43, 44 };
    QList<int> myvec;
    myvec << 42 << 43 << 44;

    QVERIFY(memcmp(myvec.constData(), reinterpret_cast<const int *>(&arr), sizeof(int) * 3) == 0);
}

void tst_QList::contains() const
{
    QList<QString> myvec;
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
void tst_QList::count() const
{
    // total size
    {
        // zero size
        QList<T> myvec;
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
        QList<T> myvec;
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

void tst_QList::countInt() const
{
    count<int>();
}

void tst_QList::countMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    count<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::countCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    count<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::cpp17ctad() const
{
#ifdef __cpp_deduction_guides
#define QVERIFY_IS_VECTOR_OF(obj, Type) \
    QVERIFY2((std::is_same<decltype(obj), QList<Type>>::value), \
             QMetaType::fromType<decltype(obj)::value_type>().name())
#define CHECK(Type, One, Two, Three) \
    do { \
        const Type v[] = {One, Two, Three}; \
        QList v1 = {One, Two, Three}; \
        QVERIFY_IS_VECTOR_OF(v1, Type); \
        QList v2(v1.begin(), v1.end()); \
        QVERIFY_IS_VECTOR_OF(v2, Type); \
        QList v3(std::begin(v), std::end(v)); \
        QVERIFY_IS_VECTOR_OF(v3, Type); \
    } while (false) \
    /*end*/
    CHECK(int, 1, 2, 3);
    CHECK(double, 1.0, 2.0, 3.0);
    CHECK(QString, QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three"));
#undef QVERIFY_IS_VECTOR_OF
#undef CHECK
#else
    QSKIP("This test requires C++17 Constructor Template Argument Deduction support enabled in the compiler.");
#endif
}

void tst_QList::data() const
{
    QList<int> myvec;
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
void tst_QList::empty() const
{
    QList<T> myvec;

    // starts empty
    QVERIFY(myvec.empty());

    // not empty
    myvec.append(SimpleValue<T>::at(2));
    QVERIFY(!myvec.empty());

    // empty again
    myvec.remove(0);
    QVERIFY(myvec.empty());
}

void tst_QList::emptyInt() const
{
    empty<int>();
}

void tst_QList::emptyMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    empty<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::emptyCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    empty<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::endsWith() const
{
    QList<int> myvec;

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
void tst_QList::eraseEmpty() const
{
    QList<T> v;
    v.erase(v.begin(), v.end());
    QCOMPARE(v.size(), 0);
}

void tst_QList::eraseEmptyInt() const
{
    eraseEmpty<int>();
}

void tst_QList::eraseEmptyMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    eraseEmpty<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::eraseEmptyCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    eraseEmpty<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QList::eraseEmptyReserved() const
{
    QList<T> v;
    v.reserve(10);
    v.erase(v.begin(), v.end());
    QCOMPARE(v.size(), 0);
}

void tst_QList::eraseEmptyReservedInt() const
{
    eraseEmptyReserved<int>();
}

void tst_QList::eraseEmptyReservedMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    eraseEmptyReserved<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::eraseEmptyReservedCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    eraseEmptyReserved<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
struct SharedVectorChecker
{
    SharedVectorChecker(const QList<T> &original, bool doCopyVector)
        : originalSize(-1),
          copy(0)
    {
        if (doCopyVector) {
            originalSize = original.size();
            copy = new QList<T>(original);
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
    QList<T> *copy;
};

template<typename T>
void tst_QList::erase(bool shared) const
{
    // note: remove() is actually more efficient, and more dangerous, because it uses the non-detaching
    // begin() / end() internally. you can also use constBegin() and constEnd() with erase(), but only
    // using reinterpret_cast... because both iterator types are really just pointers.
    // so we use a mix of erase() and remove() to cover more cases.
    {
        QList<T> v = SimpleValue<T>::vector(12);
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
        QList<T> v = SimpleValue<T>::vector(12);
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
        QList<T> v = SimpleValue<T>::vector(12);
        SharedVectorChecker<T> svc(v, shared);
        v.erase(v.begin(), v.end() - 1);
        QCOMPARE(v.size(), 1);
        QCOMPARE(v.at(0), SimpleValue<T>::at(11));
        if (shared)
            QCOMPARE(SimpleValue<T>::vector(12), *svc.copy);
    }
    {
        QList<T> v = SimpleValue<T>::vector(12);
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
}

void tst_QList::eraseInt() const
{
    erase<int>(false);
}

void tst_QList::eraseIntShared() const
{
    erase<int>(true);
}

void tst_QList::eraseMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    erase<Movable>(false);
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::eraseMovableShared() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    erase<Movable>(true);
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::eraseCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    erase<Custom>(false);
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::eraseCustomShared() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    erase<Custom>(true);
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T> void tst_QList::eraseReserved() const
{
    {
        QList<T> v(12);
        v.reserve(16);
        v.erase(v.begin());
        QCOMPARE(v.size(), 11);
        v.erase(v.begin(), v.end());
        QCOMPARE(v.size(), 0);
    }
    {
        QList<T> v(12);
        v.reserve(16);
        v.erase(v.begin() + 1);
        QCOMPARE(v.size(), 11);
        v.erase(v.begin() + 1, v.end());
        QCOMPARE(v.size(), 1);
    }
    {
        QList<T> v(12);
        v.reserve(16);
        v.erase(v.begin(), v.end() - 1);
        QCOMPARE(v.size(), 1);
    }
    {
        QList<T> v(12);
        v.reserve(16);
        v.erase(v.begin() + 5);
        QCOMPARE(v.size(), 11);
        v.erase(v.begin() + 1, v.end() - 1);
        QCOMPARE(v.size(), 2);
    }
}

void tst_QList::eraseReservedInt() const
{
    eraseReserved<int>();
}

void tst_QList::eraseReservedMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    eraseReserved<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::eraseReservedCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    eraseReserved<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QList::fill() const
{
    QList<T> myvec;

    // resize
    myvec.resize(5);
    myvec.fill(SimpleValue<T>::at(1));
    QCOMPARE(myvec, QList<T>() << SimpleValue<T>::at(1) << SimpleValue<T>::at(1)
                                 << SimpleValue<T>::at(1) << SimpleValue<T>::at(1)
                                 << SimpleValue<T>::at(1));

    // make sure it can resize itself too
    myvec.fill(SimpleValue<T>::at(2), 10);
    QCOMPARE(myvec, QList<T>() << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2)
                                 << SimpleValue<T>::at(2) << SimpleValue<T>::at(2));

    // make sure it can resize to smaller size as well
    myvec.fill(SimpleValue<T>::at(3), 2);
    QCOMPARE(myvec, QList<T>() << SimpleValue<T>::at(3) << SimpleValue<T>::at(3));
}

void tst_QList::fillInt() const
{
    fill<int>();
}

void tst_QList::fillMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    fill<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::fillCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    fill<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QList::fillDetach() const
{
    // detaches to the same size
    {
        QList<T> original = { SimpleValue<T>::at(1), SimpleValue<T>::at(1), SimpleValue<T>::at(1) };
        QList<T> copy = original;
        copy.fill(SimpleValue<T>::at(2));

        QCOMPARE(original,
                 QList<T>({ SimpleValue<T>::at(1), SimpleValue<T>::at(1), SimpleValue<T>::at(1) }));
        QCOMPARE(copy,
                 QList<T>({ SimpleValue<T>::at(2), SimpleValue<T>::at(2), SimpleValue<T>::at(2) }));
    }

    // detaches and grows in size
    {
        QList<T> original = { SimpleValue<T>::at(1), SimpleValue<T>::at(1), SimpleValue<T>::at(1) };
        QList<T> copy = original;
        copy.fill(SimpleValue<T>::at(2), 5);

        QCOMPARE(original,
                 QList<T>({ SimpleValue<T>::at(1), SimpleValue<T>::at(1), SimpleValue<T>::at(1) }));
        QCOMPARE(copy,
                 QList<T>({ SimpleValue<T>::at(2), SimpleValue<T>::at(2), SimpleValue<T>::at(2),
                            SimpleValue<T>::at(2), SimpleValue<T>::at(2) }));
    }

    // detaches and shrinks in size
    {
        QList<T> original = { SimpleValue<T>::at(1), SimpleValue<T>::at(1), SimpleValue<T>::at(1) };
        QList<T> copy = original;
        copy.fill(SimpleValue<T>::at(2), 1);

        QCOMPARE(original,
                 QList<T>({ SimpleValue<T>::at(1), SimpleValue<T>::at(1), SimpleValue<T>::at(1) }));
        QCOMPARE(copy, QList<T>({ SimpleValue<T>::at(2) }));
    }
}

void tst_QList::fillDetachInt() const
{
    fillDetach<int>();
}

void tst_QList::fillDetachMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    fillDetach<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::fillDetachCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    fillDetach<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::first() const
{
    QList<int> myvec;
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


    QCOMPARE(QList<int>().first(0), QList<int>());
    QCOMPARE(myvec.first(0), QList<int>());
    QCOMPARE(myvec.first(1), (QList<int>{23}));
    QCOMPARE(myvec.first(2), (QList<int>{23, 42}));
    QCOMPARE(myvec.first(3), myvec);
}

void tst_QList::constFirst() const
{
    QList<int> myvec;
    myvec << 69 << 42 << 3;

    // test it starts ok
    QCOMPARE(myvec.constFirst(), 69);
    QVERIFY(myvec.isDetached());

    QList<int> myvecCopy = myvec;
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
void tst_QList::fromList() const
{
    QList<T> list;
    list << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3);

    QList<T> myvec;
    myvec = QList<T>::fromList(list);

    // test it worked ok
    QCOMPARE(myvec, QList<T>() << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3));
    QCOMPARE(list, QList<T>() << SimpleValue<T>::at(0) << SimpleValue<T>::at(1) << SimpleValue<T>::at(2) << SimpleValue<T>::at(3));
}

void tst_QList::fromListInt() const
{
    fromList<int>();
}

void tst_QList::fromListMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    fromList<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::fromListCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    fromList<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
void tst_QList::fromStdVector() const
{
    // stl = :(
    std::vector<QString> svec;
    svec.push_back(QLatin1String("aaa"));
    svec.push_back(QLatin1String("bbb"));
    svec.push_back(QLatin1String("ninjas"));
    svec.push_back(QLatin1String("pirates"));
    QList<QString> myvec = QList<QString>::fromStdVector(svec);

    // test it converts ok
    QCOMPARE(myvec, QList<QString>() << "aaa" << "bbb" << "ninjas" << "pirates");
}
#endif

void tst_QList::indexOf() const
{
    QList<QString> myvec;
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
void tst_QList::insert() const
{
    QList<T> myvec;
    const T
        tA = SimpleValue<T>::at(0),
        tB = SimpleValue<T>::at(1),
        tC = SimpleValue<T>::at(2),
        tX = SimpleValue<T>::at(3),
        tZ = SimpleValue<T>::at(4),
        tT = SimpleValue<T>::at(5),
        ti = SimpleValue<T>::at(6);
    myvec << tA << tB << tC;
    QList<T> myvec2 = myvec;

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
    QCOMPARE(myvec, QList<T>() << tX << tZ << ti << ti
             << tA << tB << tC << tT);

    myvec2.insert(myvec2.begin() + 2, 2, ti);
    QCOMPARE(myvec2, myvec);

    // insert from references to the same container:
    myvec.insert(0, 1, myvec[5]);   // inserts tB
    myvec2.insert(0, 1, myvec2[5]); // inserts tB
    QCOMPARE(myvec, QList<T>() << tB << tX << tZ << ti << ti
             << tA << tB << tC << tT);
    QCOMPARE(myvec2, myvec);

    myvec.insert(0, 1, const_cast<const QList<T>&>(myvec)[0]);   // inserts tB
    myvec2.insert(0, 1, const_cast<const QList<T>&>(myvec2)[0]); // inserts tB
    QCOMPARE(myvec, QList<T>() << tB << tB << tX << tZ << ti << ti
             << tA << tB << tC << tT);
    QCOMPARE(myvec2, myvec);
}

void tst_QList::insertInt() const
{
    insert<int>();
}

void tst_QList::insertMovable() const
{
    insert<Movable>();
}

void tst_QList::insertCustom() const
{
    insert<Custom>();
}

void tst_QList::insertZeroCount_data()
{
    QTest::addColumn<int>("pos");
    QTest::newRow("0") << 0;
    QTest::newRow("1") << 1;
}

void tst_QList::insertZeroCount() const
{
    QFETCH(int, pos);
    QList<int> x;
    x << 0 << 0;
    x.insert(pos, 0, 1);
    QCOMPARE(x[pos], 0);
    QList<int> y;
    y = x;
    y.insert(pos, 0, 2);
    QCOMPARE(y[pos], 0);
}

void tst_QList::isEmpty() const
{
    QList<QString> myvec;

    // starts ok
    QVERIFY(myvec.isEmpty());

    // not empty now
    myvec.append(QLatin1String("hello there"));
    QVERIFY(!myvec.isEmpty());

    // empty again
    myvec.remove(0);
    QVERIFY(myvec.isEmpty());
}

void tst_QList::last() const
{
    QList<QString> myvec;
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

    QCOMPARE(QList<QString>().last(0), QList<QString>());
    QCOMPARE(myvec.last(0), QList<QString>());
    QCOMPARE(myvec.last(1), (QList<QString>{QLatin1String("C")}));
    QCOMPARE(myvec.last(2), (QList<QString>{QLatin1String("B"), QLatin1String("C")}));
    QCOMPARE(myvec.last(3), myvec);
}

void tst_QList::constLast() const
{
    QList<int> myvec;
    myvec << 69 << 42 << 3;

    // test it starts ok
    QCOMPARE(myvec.constLast(), 3);
    QVERIFY(myvec.isDetached());

    QList<int> myvecCopy = myvec;
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

void tst_QList::lastIndexOf() const
{
    QList<QString> myvec;
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

void tst_QList::mid() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz" << "bak" << "buck" << "hello" << "kitty";

    QCOMPARE(list.mid(3, 3), QList<QString>() << "bak" << "buck" << "hello");
    QCOMPARE(list.mid(6, 10), QList<QString>() << "kitty");
    QCOMPARE(list.mid(-1, 20), list);
    QCOMPARE(list.mid(4), QList<QString>() << "buck" << "hello" << "kitty");
}

void tst_QList::sliced() const
{
    QList<QString> list;
    list << "foo" << "bar" << "baz" << "bak" << "buck" << "hello" << "kitty";

    QCOMPARE(QList<QString>().sliced(0), QList<QString>());
    QCOMPARE(QList<QString>().sliced(0, 0), QList<QString>());
    QCOMPARE(list.sliced(3, 3), QList<QString>() << "bak" << "buck" << "hello");
    QCOMPARE(list.sliced(3), QList<QString>() << "bak" << "buck" << "hello" << "kitty");
    QCOMPARE(list.sliced(6, 1), QList<QString>() << "kitty");
    QCOMPARE(list.sliced(6), QList<QString>() << "kitty");
    QCOMPARE(list.sliced(0, list.size()), list);
    QCOMPARE(list.sliced(0), list);
    QCOMPARE(list.sliced(4), QList<QString>() << "buck" << "hello" << "kitty");
}

template <typename T>
void tst_QList::qhash() const
{
    QList<T> l1, l2;
    QCOMPARE(qHash(l1), qHash(l2));
    l1 << SimpleValue<T>::at(0);
    l2 << SimpleValue<T>::at(0);
    QCOMPARE(qHash(l1), qHash(l2));
}

template <typename T>
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
    const int instancesCount = Movable::counter.loadAcquire();
    move<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::moveCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    move<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

template<typename T>
void tst_QList::prepend() const
{
    QList<T> myvec;
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

void tst_QList::prependInt() const
{
    prepend<int>();
}

void tst_QList::prependMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    prepend<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::prependCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    prepend<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::removeAllWithAlias() const
{
    QList<QString> strings;
    strings << "One" << "Two" << "Three" << "One" /* must be distinct, but equal */;
    QCOMPARE(strings.removeAll(strings.front()), 2); // will trigger asan/ubsan
}

template<typename T>
void tst_QList::remove() const
{
    QList<T> myvec;
    T val1 = SimpleValue<T>::at(1);
    T val2 = SimpleValue<T>::at(2);
    T val3 = SimpleValue<T>::at(3);
    T val4 = SimpleValue<T>::at(4);
    myvec << val1 << val2 << val3;
    myvec << val1 << val2 << val3;
    myvec << val1 << val2 << val3;
    // remove middle
    myvec.remove(1);
    QCOMPARE(myvec, QList<T>() << val1 << val3  << val1 << val2 << val3  << val1 << val2 << val3);

    // removeOne()
    QVERIFY(!myvec.removeOne(val4));
    QVERIFY(myvec.removeOne(val2));
    QCOMPARE(myvec, QList<T>() << val1 << val3  << val1 << val3  << val1 << val2 << val3);

    QList<T> myvecCopy = myvec;
    QVERIFY(myvecCopy.isSharedWith(myvec));
    // removeAll()
    QCOMPARE(myvec.removeAll(val4), 0);
    QVERIFY(myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec.removeAll(val1), 3);
    QVERIFY(!myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec, QList<T>() << val3  << val3  << val2 << val3);
    myvecCopy = myvec;
    QVERIFY(myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec.removeAll(val2), 1);
    QVERIFY(!myvecCopy.isSharedWith(myvec));
    QCOMPARE(myvec, QList<T>() << val3  << val3  << val3);

    // remove rest
    myvec.remove(0, 3);
    QCOMPARE(myvec, QList<T>());
}

void tst_QList::removeInt() const
{
    remove<int>();
}

void tst_QList::removeMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    remove<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::removeCustom() const
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

void tst_QList::removeFirstLast() const
{
    // pop_pack - pop_front
    QList<int> t, t2;
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
    QList<int> x, y;
    x.append(1);
    x.append(2);
    y = x;
    x.removeFirst();
    QCOMPARE(x.size(), 1);
    QCOMPARE(y.size(), 2);
    QCOMPARE(x.at(0), 2);

    // remove Last
    QList<RemoveLastTestClass> v;
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
    for (QList<RemoveLastTestClass>::const_iterator i = v.constBegin(); i != v.constEnd(); ++i) {
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
    QList<int> v2, v3;
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
    QList<int> z1, z2;
    z1.append(9);
    z2 = z1;
    z1.removeLast();
    QCOMPARE(z1.size(), 0);
    QCOMPARE(z2.size(), 1);
    QCOMPARE(z2.at(0), 9);
}


void tst_QList::resizePOD_data() const
{
    QTest::addColumn<QList<int> >("vector");
    QTest::addColumn<int>("size");

    QVERIFY(!QTypeInfo<int>::isComplex);
    QVERIFY(QTypeInfo<int>::isRelocatable);

    QList<int> null;
    QList<int> empty(0, 5);
    QList<int> emptyReserved;
    QList<int> nonEmpty;
    QList<int> nonEmptyReserved;

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
}

void tst_QList::resizePOD() const
{
    QFETCH(QList<int>, vector);
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

void tst_QList::resizeComplexMovable_data() const
{
    QTest::addColumn<QList<Movable> >("vector");
    QTest::addColumn<int>("size");

    QVERIFY(QTypeInfo<Movable>::isComplex);
    QVERIFY(QTypeInfo<Movable>::isRelocatable);

    QList<Movable> null;
    QList<Movable> empty(0, 'Q');
    QList<Movable> emptyReserved;
    QList<Movable> nonEmpty;
    QList<Movable> nonEmptyReserved;

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
}

void tst_QList::resizeComplexMovable() const
{
    const int items = Movable::counter.loadAcquire();
    {
        QFETCH(QList<Movable>, vector);
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

void tst_QList::resizeComplex_data() const
{
    QTest::addColumn<QList<Custom> >("vector");
    QTest::addColumn<int>("size");

    QVERIFY(QTypeInfo<Custom>::isComplex);
    QVERIFY(!QTypeInfo<Custom>::isRelocatable);

    QList<Custom> null;
    QList<Custom> empty(0, '0');
    QList<Custom> emptyReserved;
    QList<Custom> nonEmpty;
    QList<Custom> nonEmptyReserved;

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
}

void tst_QList::resizeComplex() const
{
    const int items = Custom::counter.loadAcquire();
    {
        QFETCH(QList<Custom>, vector);
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

void tst_QList::resizeCtorAndDtor() const
{
    const int items = Custom::counter.loadAcquire();
    {
        QList<Custom> null;
        QList<Custom> empty(0, '0');
        QList<Custom> emptyReserved;
        QList<Custom> nonEmpty;
        QList<Custom> nonEmptyReserved;

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

void tst_QList::resizeToZero() const
{
    QList<int> x;
    QList<int> y;
    x << 1 << 2;
    y = x;
    y.resize(0);
    QCOMPARE(y.size(), 0);
    // grow back
    y.resize(x.size());
    QCOMPARE(y.size(), x.size());
    // default initialized
    QCOMPARE(y[0], 0);
    QCOMPARE(y[1], 0);
}

void tst_QList::resizeToTheSameSize_data()
{
    QTest::addColumn<QList<int>>("x");
    QTest::newRow("size 2") << QList({ 1, 2 });
    QTest::newRow("size 0") << QList<int>();
}

void tst_QList::resizeToTheSameSize() const
{
    QFETCH(QList<int>, x);
    QList<int> y;
    y = x;
    y.resize(x.size());
    QCOMPARE(y.size(), x.size());
}

void tst_QList::reverseIterators() const
{
    QList<int> v;
    v << 1 << 2 << 3 << 4;
    QList<int> vr = v;
    std::reverse(vr.begin(), vr.end());
    const QList<int> &cvr = vr;
    QVERIFY(std::equal(v.begin(), v.end(), vr.rbegin()));
    QVERIFY(std::equal(v.begin(), v.end(), vr.crbegin()));
    QVERIFY(std::equal(v.begin(), v.end(), cvr.rbegin()));
    QVERIFY(std::equal(vr.rbegin(), vr.rend(), v.begin()));
    QVERIFY(std::equal(vr.crbegin(), vr.crend(), v.begin()));
    QVERIFY(std::equal(cvr.rbegin(), cvr.rend(), v.begin()));
}

template<typename T>
void tst_QList::size() const
{
    // zero size
    QList<T> myvec;
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

void tst_QList::sizeInt() const
{
    size<int>();
}

void tst_QList::sizeMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    size<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::sizeCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    size<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

// ::squeeze() is tested in ::capacity().

void tst_QList::startsWith() const
{
    QList<int> myvec;

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
void tst_QList::swap() const
{
    QList<T> v1, v2;
    T val1 = SimpleValue<T>::at(0);
    T val2 = SimpleValue<T>::at(1);
    T val3 = SimpleValue<T>::at(2);
    T val4 = SimpleValue<T>::at(3);
    T val5 = SimpleValue<T>::at(4);
    T val6 = SimpleValue<T>::at(5);
    v1 << val1 << val2 << val3;
    v2 << val4 << val5 << val6;

    v1.swap(v2);
    QCOMPARE(v1,QList<T>() << val4 << val5 << val6);
    QCOMPARE(v2,QList<T>() << val1 << val2 << val3);
}

void tst_QList::swapInt() const
{
    swap<int>();
}

void tst_QList::swapMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    swap<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::swapCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    swap<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::toList() const
{
    QList<QString> myvec;
    myvec << "A" << "B" << "C";

    // make sure it converts and doesn't modify the original vector
    QCOMPARE(myvec.toList(), QList<QString>() << "A" << "B" << "C");
    QCOMPARE(myvec, QList<QString>() << "A" << "B" << "C");
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
void tst_QList::toStdVector() const
{
    QList<QString> myvec;
    myvec << "A" << "B" << "C";

    std::vector<QString> svec = myvec.toStdVector();
    QCOMPARE(svec.at(0), QLatin1String("A"));
    QCOMPARE(svec.at(1), QLatin1String("B"));
    QCOMPARE(svec.at(2), QLatin1String("C"));

    QCOMPARE(myvec, QList<QString>() << "A" << "B" << "C");
}
#endif

void tst_QList::value() const
{
    QList<QString> myvec;
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

void tst_QList::testOperators() const
{
    QList<QString> myvec;
    myvec << "A" << "B" << "C";
    QList<QString> myvectwo;
    myvectwo << "D" << "E" << "F";
    QList<QString> combined;
    combined << "A" << "B" << "C" << "D" << "E" << "F";

    // !=
    QVERIFY(myvec != myvectwo);

    // +
    QCOMPARE(myvec + myvectwo, combined);
    QCOMPARE(myvec, QList<QString>() << "A" << "B" << "C");
    QCOMPARE(myvectwo, QList<QString>() << "D" << "E" << "F");

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

void tst_QList::reserve()
{
    fooCtor = 0;
    fooDtor = 0;
    {
        QList<Foo> a;
        a.resize(2);
        QCOMPARE(fooCtor, 2);
        QList<Foo> b(a);
        b.reserve(1);
        QCOMPARE(b.size(), a.size());
        QCOMPARE(fooDtor, 0);
    }
    QCOMPARE(fooCtor, fooDtor);
}

// This is a regression test for QTBUG-51758
void tst_QList::reserveZero()
{
    QList<int> vec;
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

template<typename T>
void tst_QList::initializeList()
{
    T val1(SimpleValue<T>::at(1));
    T val2(SimpleValue<T>::at(2));
    T val3(SimpleValue<T>::at(3));
    T val4(SimpleValue<T>::at(4));

    QList<T> v1 {val1, val2, val3};
    QCOMPARE(v1, QList<T>() << val1 << val2 << val3);
    QCOMPARE(v1, (QList<T> {val1, val2, val3}));

    QList<QList<T>> v2{ v1, {val4}, QList<T>(), {val1, val2, val3} };
    QList<QList<T>> v3;
    v3 << v1 << (QList<T>() << val4) << QList<T>() << v1;
    QCOMPARE(v3, v2);

    QList<T> v4({});
    QCOMPARE(v4.size(), 0);
}

void tst_QList::initializeListInt()
{
    initializeList<int>();
}

void tst_QList::initializeListMovable()
{
    const int instancesCount = Movable::counter.loadAcquire();
    initializeList<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::initializeListCustom()
{
    const int instancesCount = Custom::counter.loadAcquire();
    initializeList<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

void tst_QList::const_shared_null()
{
    QList<int> v2;
    QVERIFY(!v2.isDetached());
}

template<typename T>
void tst_QList::detach() const
{
    {
        // detach an empty vector
        QList<T> v;
        v.detach();
        QVERIFY(!v.isDetached());
        QCOMPARE(v.size(), 0);
        QCOMPARE(v.capacity(), 0);
    }
    {
        // detach an empty referenced vector
        QList<T> v;
        QList<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(!v.isDetached());
        QCOMPARE(v.size(), 0);
        QCOMPARE(v.capacity(), 0);
    }
    {
        // detach a not empty referenced vector
        QList<T> v(31);
        QList<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 31);
        QCOMPARE(v.capacity(), 31);
    }
    {
        // detach a not empty vector
        QList<T> v(31);
        QVERIFY(v.isDetached());
        v.detach(); // detaching a detached vector
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 31);
        QCOMPARE(v.capacity(), 31);
    }
    {
        // detach a not empty vector with preallocated space
        QList<T> v(3);
        v.reserve(8);
        QList<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 3);
        QCOMPARE(v.capacity(), 8);
    }
    {
        // detach a not empty vector with preallocated space
        QList<T> v(3);
        v.reserve(8);
        QVERIFY(v.isDetached());
        v.detach(); // detaching a detached vector
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 3);
        QCOMPARE(v.capacity(), 8);
    }
    {
        // detach a not empty, initialized vector
        QList<T> v(7, SimpleValue<T>::at(1));
        QList<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 7);
        for (int i = 0; i < v.size(); ++i)
            QCOMPARE(v[i], SimpleValue<T>::at(1));
    }
    {
        // detach a not empty, initialized vector
        QList<T> v(7, SimpleValue<T>::at(2));
        QVERIFY(v.isDetached());
        v.detach(); // detaching a detached vector
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 7);
        for (int i = 0; i < v.size(); ++i)
            QCOMPARE(v[i], SimpleValue<T>::at(2));
    }
    {
        // detach a not empty, initialized vector with preallocated space
        QList<T> v(7, SimpleValue<T>::at(3));
        v.reserve(31);
        QList<T> ref(v);
        QVERIFY(!v.isDetached());
        v.detach();
        QVERIFY(v.isDetached());
        QCOMPARE(v.size(), 7);
        QCOMPARE(v.capacity(), 31);
        for (int i = 0; i < v.size(); ++i)
            QCOMPARE(v[i], SimpleValue<T>::at(3));
    }
}

void tst_QList::detachInt() const
{
    detach<int>();
}

void tst_QList::detachMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    detach<Movable>();
    QCOMPARE(instancesCount, Movable::counter.loadAcquire());
}

void tst_QList::detachCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    detach<Custom>();
    QCOMPARE(instancesCount, Custom::counter.loadAcquire());
}

static QAtomicPointer<QList<int> > detachThreadSafetyDataInt;
static QAtomicPointer<QList<Movable> > detachThreadSafetyDataMovable;
static QAtomicPointer<QList<Custom> > detachThreadSafetyDataCustom;

template<typename T> QAtomicPointer<QList<T> > *detachThreadSafetyData();
template<> QAtomicPointer<QList<int> > *detachThreadSafetyData() { return &detachThreadSafetyDataInt; }
template<> QAtomicPointer<QList<Movable> > *detachThreadSafetyData() { return &detachThreadSafetyDataMovable; }
template<> QAtomicPointer<QList<Custom> > *detachThreadSafetyData() { return &detachThreadSafetyDataCustom; }

static QSemaphore detachThreadSafetyLock;

template<typename T>
void tst_QList::detachThreadSafety() const
{
    delete detachThreadSafetyData<T>()->fetchAndStoreOrdered(new QList<T>(SimpleValue<T>::vector(400)));

    static const uint threadsCount = 5;

    struct : QThread {
        void run() override
        {
            QList<T> copy(*detachThreadSafetyData<T>()->loadRelaxed());
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

void tst_QList::detachThreadSafetyInt() const
{
    for (uint i = 0; i < 128; ++i)
        detachThreadSafety<int>();
}

void tst_QList::detachThreadSafetyMovable() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    for (uint i = 0; i < 128; ++i) {
        detachThreadSafety<Movable>();
        QCOMPARE(Movable::counter.loadAcquire(), instancesCount);
    }
}

void tst_QList::detachThreadSafetyCustom() const
{
    const int instancesCount = Custom::counter.loadAcquire();
    for (uint i = 0; i < 128; ++i) {
        detachThreadSafety<Custom>();
        QCOMPARE(Custom::counter.loadAcquire(), instancesCount);
    }
}

void tst_QList::insertMove() const
{
    const int instancesCount = Movable::counter.loadAcquire();
    {
        QList<Movable> vec;
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

void tst_QList::swapItemsAt() const
{
    QList<int> v;
    v << 0 << 1 << 2 << 3;

    v.swapItemsAt(0, 2);
    QCOMPARE(v.at(0), 2);
    QCOMPARE(v.at(2), 0);

    auto copy = v;
    copy.swapItemsAt(0, 2);
    QCOMPARE(v.at(0), 2);
    QCOMPARE(v.at(2), 0);
    QCOMPARE(copy.at(0), 0);
    QCOMPARE(copy.at(2), 2);
}

void tst_QList::emplaceInt()
{
    emplaceImpl<int>();
}

void tst_QList::emplaceCustom()
{
    emplaceImpl<Custom>();
}

void tst_QList::emplaceMovable()
{
    emplaceImpl<Movable>();
}

void tst_QList::emplaceConsistentWithStdVectorInt()
{
    emplaceConsistentWithStdVectorImpl<int>();
}

void tst_QList::emplaceConsistentWithStdVectorCustom()
{
    emplaceConsistentWithStdVectorImpl<Custom>();
}

void tst_QList::emplaceConsistentWithStdVectorMovable()
{
    emplaceConsistentWithStdVectorImpl<Movable>();
}

void tst_QList::emplaceConsistentWithStdVectorQString()
{
    emplaceConsistentWithStdVectorImpl<QString>();
}

void tst_QList::emplaceReturnsIterator()
{
    QList<Movable> vec;

    vec.emplace(0, 'k')->i = 'p';

    QCOMPARE(vec[0].i, 'p');
}

void tst_QList::emplaceBack()
{
    QScopedValueRollback<QAtomicInt> rollback(Movable::counter, 0);

    QList<Movable> vec;

    vec.emplaceBack('k');

    QCOMPARE(Movable::counter, 1);
}

void tst_QList::emplaceBackReturnsRef()
{
    QList<Movable> vec;

    vec.emplaceBack('k').i = 'p';

    QCOMPARE(vec.at(0).i, 'p');
}

void tst_QList::emplaceWithElementFromTheSameContainer()
{
    QFETCH(int, elementPos);
    QFETCH(int, insertPos);
    QFETCH(bool, doCopy);

    QList<QString> vec {"a", "b", "c", "d", "e"};
    const QString e = vec[elementPos];

    if (doCopy)
        vec.emplace(insertPos, vec[elementPos]);
    else
        vec.emplace(insertPos, std::move(vec[elementPos]));

    QCOMPARE(vec[insertPos], e);
}

void tst_QList::emplaceWithElementFromTheSameContainer_data()
{
    QTest::addColumn<int>("elementPos");
    QTest::addColumn<int>("insertPos");
    QTest::addColumn<bool>("doCopy");

    for (int i = 0; i < 2; ++i) {
        const bool doCopy = i == 0;
        const char *opName = doCopy ? "copy" : "move";

        QTest::addRow("%s: begin  -> end"   , opName) << 0 << 5 << doCopy;
        QTest::addRow("%s: begin  -> middle", opName) << 0 << 2 << doCopy;
        QTest::addRow("%s: middle -> begin" , opName) << 2 << 0 << doCopy;
        QTest::addRow("%s: middle -> end"   , opName) << 2 << 5 << doCopy;
        QTest::addRow("%s: end    -> middle", opName) << 4 << 2 << doCopy;
        QTest::addRow("%s: end    -> begin" , opName) << 4 << 0 << doCopy;
    }
}

template<typename T>
void tst_QList::emplaceImpl() const
{
    QList<T> vec {'a', 'b', 'c', 'd'};

    vec.emplace(2, 'k');

    QCOMPARE(vec[2], T('k'));
}

template <class T>
static void vecEq(const QList<T> &qVec, const std::vector<T> &stdVec)
{
    QCOMPARE(std::size_t(qVec.size()), stdVec.size());
    QVERIFY(std::equal(qVec.begin(), qVec.end(), stdVec.begin(), stdVec.end()));
}

template <class T>
static void squeezeVec(QList<T> &qVec, std::vector<T> &stdVec)
{
    qVec.squeeze();
    stdVec.shrink_to_fit();
}

template<typename T>
void tst_QList::emplaceConsistentWithStdVectorImpl() const
{
    // fast-patch to make QString work with the old logic
    const auto convert = [] (char i) {
        if constexpr (std::is_same_v<QString, T>) {
            return QChar(i);
        } else {
            return i;
        }
    };

    QList<T> qVec {convert('a'), convert('b'), convert('c'), convert('d'), convert('e')};
    std::vector<T> stdVec {convert('a'), convert('b'), convert('c'), convert('d'), convert('e')};
    vecEq(qVec, stdVec);

    qVec.emplaceBack(convert('f'));
    stdVec.emplace_back(convert('f'));
    vecEq(qVec, stdVec);

    qVec.emplace(3, convert('g'));
    stdVec.emplace(stdVec.begin() + 3, convert('g'));
    vecEq(qVec, stdVec);

    T t;
    // while QList is safe with regards to emplacing elements moved from itself, it's UB
    // for std::vector, so do the moving in two steps there.
    qVec.emplaceBack(std::move(qVec[0]));
    stdVec.emplace_back(std::move(t = std::move(stdVec[0])));
    vecEq(qVec, stdVec);

    squeezeVec(qVec, stdVec);

    qVec.emplaceBack(std::move(qVec[1]));
    stdVec.emplace_back(std::move(t = std::move(stdVec[1])));
    vecEq(qVec, stdVec);

    squeezeVec(qVec, stdVec);

    qVec.emplace(3, std::move(qVec[5]));
    stdVec.emplace(stdVec.begin() + 3, std::move(t = std::move(stdVec[5])));

    vecEq(qVec, stdVec);

    qVec.emplaceBack(qVec[3]);
    stdVec.emplace_back((t = stdVec[3]));
    vecEq(qVec, stdVec);

    squeezeVec(qVec, stdVec);

    qVec.emplaceBack(qVec[4]);
    stdVec.emplace_back((t = stdVec[4]));
    vecEq(qVec, stdVec);

    squeezeVec(qVec, stdVec);

    qVec.emplace(5, qVec[7]);
    stdVec.emplace(stdVec.begin() + 5, (t = stdVec[7]));
    vecEq(qVec, stdVec);
}

void tst_QList::fromReadOnlyData() const
{
    {
        QVector<char> d = QVector<char>::fromReadOnlyData("ABCDEFGHIJ");
        QCOMPARE(d.capacity(), 0);
        d.squeeze();
        QCOMPARE(d.capacity(), 0);
        QCOMPARE(d.size(), 10u + 1u);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], char('A' + i));
    }

    {
        // wchar_t is not necessarily 2-bytes
        QVector<wchar_t> d = QVector<wchar_t>::fromReadOnlyData(L"ABCDEFGHIJ");
        QCOMPARE(d.size(), 10u + 1u);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], wchar_t('A' + i));
        QVERIFY(d.isDetached());
    }

    {
        const char data[] = "ABCDEFGHIJ";
        const QVector<char> v = QVector<char>::fromReadOnlyData(data);

        QVERIFY(v.constData() == data);
        QVERIFY(!v.isEmpty());
        QCOMPARE(v.size(), qsizetype(11));
        // v.capacity() is unspecified, for now

        QCOMPARE((void*)(const char*)(v.constBegin() + v.size()), (void*)(const char*)v.constEnd());

        for (int i = 0; i < 10; ++i)
            QCOMPARE(v[i], char('A' + i));
        QCOMPARE(v[10], char('\0'));
    }

    {
        struct LiteralType {
            int value;
            constexpr LiteralType(int v = 0) : value(v) {}
        };
        const LiteralType literal[] = {LiteralType(0), LiteralType(1), LiteralType(2)};

        const QVector<LiteralType> d = QVector<LiteralType>::fromReadOnlyData(literal);
        QCOMPARE(d.size(), 3);
        for (int i = 0; i < 3; ++i)
            QCOMPARE(d.data()[i].value, i);
    }
}

struct alignas(8) CustomAligned
{
    qint64 v = 0;
    CustomAligned() = default;
    CustomAligned(qint64 i) : v(i) { }
    friend bool operator==(const CustomAligned &x, const CustomAligned &y) { return x.v == y.v; }
};

void tst_QList::qtbug_90359() const
{
    // Note: a very special test that could only fail for specific alignments
    constexpr bool canFail = (alignof(QArrayData) == 4) && (sizeof(QArrayData) == 12);
    if constexpr (!canFail)
        qWarning() << "This test will always succeed on this system.";
    if constexpr (alignof(CustomAligned) > alignof(std::max_align_t))
        QSKIP("The codepaths tested here wouldn't be executed.");

    const QList<CustomAligned> expected({ 0, 1, 2, 3, 4, 5, 6 });
    QList<CustomAligned> actual;
    for (int i = 0; i < 7; ++i) {
        actual.append(i);
        QCOMPARE(actual.at(i), i);
    }
    QCOMPARE(actual, expected);
}

QTEST_MAIN(tst_QList)
#include "tst_qlist.moc"
