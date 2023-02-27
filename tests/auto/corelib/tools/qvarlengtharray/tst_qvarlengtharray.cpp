// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <QVarLengthArray>
#include <qvariant.h>
#include <qscopeguard.h>
#include <qscopedvaluerollback.h>

#include <algorithm>
#include <q20iterator.h>
#include <memory>

struct Tracker
{
    static int count;
    Tracker() { ++count; }
    Tracker(const Tracker &) { ++count; }
    Tracker(Tracker &&) { ++count; }

    Tracker &operator=(const Tracker &) = default;
    Tracker &operator=(Tracker &&) = default;

    ~Tracker() { --count; }

};

int Tracker::count = 0;

template <typename T>
class ValueTracker
{
    Tracker m_tracker;
public:
    ValueTracker() = default;
    ValueTracker(T value) : value{std::move(value)} {}
    T value;

    friend bool operator==(const ValueTracker &lhs, const ValueTracker &rhs) noexcept
    { return lhs.value == rhs.value; }
    friend bool operator!=(const ValueTracker &lhs, const ValueTracker &rhs) noexcept
    { return !operator==(lhs, rhs); }
};

class NonCopyable
{
    Q_DISABLE_COPY(NonCopyable)
    int n;
public:
    NonCopyable() : n(0) {}
    explicit NonCopyable(int n) : n(n) {}

    friend bool operator==(const NonCopyable &lhs, const NonCopyable &rhs) noexcept
    { return lhs.n == rhs.n; }
    friend bool operator!=(const NonCopyable &lhs, const NonCopyable &rhs) noexcept
    { return !operator==(lhs, rhs); }
};

class tst_QVarLengthArray : public QObject
{
    Q_OBJECT
private slots:
    void defaultConstructor_int() { defaultConstructor<int>(); }
    void defaultConstructor_QString() { defaultConstructor<QString>(); }
    void sizeConstructor_int() { sizeConstructor<int>(); }
    void sizeConstructor_QString() { sizeConstructor<QString>(); }
    void sizeConstructor_NonCopyable() { sizeConstructor<NonCopyable>(); }
    void append();
#if QT_DEPRECATED_SINCE(6, 3)
    void prepend();
#endif
    void emplace();
    void move_int_1() { move_int<1>(); }
    void move_int_2() { move_int<2>(); }
    void move_int_3() { move_int<3>(); }
    void move_QString_1() { move_QString<1>(); }
    void move_QString_2() { move_QString<2>(); }
    void move_QString_3() { move_QString<3>(); }
    void move_Tracker_1() { move_Tracker<1>(); }
    void move_Tracker_2() { move_Tracker<2>(); }
    void move_Tracker_3() { move_Tracker<3>(); }
    void removeLast();
    void oldTests();
    void appendCausingRealloc();
    void appendIsStronglyExceptionSafe();
    void resize();
    void realloc();
    void iterators();
    void reverseIterators();
    void count();
    void cpp17ctad();
    void first();
    void last();
    void squeeze();
    void operators();
    void indexOf();
    void lastIndexOf();
    void contains();
    void clear();
    void initializeListInt();
    void initializeListMovable();
    void initializeListComplex();
    void insertMove();
    void nonCopyable();
    void implicitDefaultCtor();
    void reserve();
    void value();
    void insert();
    void insert_data();
    void replace();
    void remove();
    void erase();

    // special cases:
    void copesWithCopyabilityOfMoveOnlyVector(); // QTBUG-109745
private:
    template <typename T>
    void defaultConstructor();
    template <typename T>
    void sizeConstructor();
    template <qsizetype N, typename T>
    void move(T t1, T t2);
    template <qsizetype N>
    void move_int() { move<N, int>(42, 24); }
    template <qsizetype N>
    void move_QString() { move<N, QString>("Hello", "World"); }
    template <qsizetype N>
    void move_Tracker();
    template<typename T>
    void initializeList();
};

template <typename T>
void tst_QVarLengthArray::defaultConstructor()
{
    {
        QVarLengthArray<T, 123> vla;
        QCOMPARE(vla.size(), 0);
        QVERIFY(vla.empty());
        QVERIFY(vla.isEmpty());
        QCOMPARE(vla.begin(), vla.end());
        QCOMPARE(vla.capacity(), 123);
    }
    {
        QVarLengthArray<T> vla;
        QCOMPARE(vla.capacity(), 256);    // notice, should we change the default
    }
}

template <typename T>
void tst_QVarLengthArray::sizeConstructor()
{
    {
        QVarLengthArray<T, 123> vla(0);
        QCOMPARE(vla.size(), 0);
        QVERIFY(vla.empty());
        QVERIFY(vla.isEmpty());
        QCOMPARE(vla.begin(), vla.end());
        QCOMPARE(vla.capacity(), 123);
    }
    {
        QVarLengthArray<T, 124> vla(124);
        QCOMPARE(vla.size(), 124);
        QVERIFY(!vla.empty());
        QCOMPARE(vla.capacity(), 124);
    }
    {
        QVarLengthArray<T, 124> vla(125);
        QCOMPARE(vla.size(), 125);
        QVERIFY(!vla.empty());
        QCOMPARE_GE(vla.capacity(), 125);
    }
}

void tst_QVarLengthArray::append()
{
    QVarLengthArray<QString, 2> v;
    v.append(QString("1"));
    v.append(v.front());
    QCOMPARE(v.capacity(), 2);
    // transition from prealloc to heap:
    v.append(v.front());
    QVERIFY(v.capacity() > 2);
    QCOMPARE(v.front(), v.back());
    while (v.size() < v.capacity())
        v.push_back(v[0]);
    QCOMPARE(v.back(), v.front());
    QCOMPARE(v.size(), v.capacity());
    // transition from heap to larger heap:
    v.push_back(v.front());
    QCOMPARE(v.back(), v.front());

    QVarLengthArray<int> v2; // rocket!
    v2.append(5);
}

#if QT_DEPRECATED_SINCE(6, 3)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QVarLengthArray::prepend()
{
    QVarLengthArray<QString, 2> v;
    v.prepend(QString("1"));
    v.prepend(v.front());
    QCOMPARE(v.capacity(), 2);
    // transition from stack to heap
    v.prepend(v.back());
    QVERIFY(v.capacity() > 2);
    QCOMPARE(v.front(), v.back());
    while (v.size() < v.capacity())
        v.prepend(v.back());
    QCOMPARE(v.front(), v.back());
    QCOMPARE(v.size(), v.capacity());
    // transition from heap to larger heap:
    v.prepend(v.back());
    QCOMPARE(v.front(), v.back());
}
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 3)

void tst_QVarLengthArray::emplace()
{
    {
        QVarLengthArray<QString, 2> strings;
        strings.emplace_back();
        QCOMPARE(strings.size(), 1);
        QCOMPARE(strings.front().isNull(), true);
        strings.emplace(strings.begin(), 42, u'x');
        QCOMPARE(strings.size(), 2);
        QCOMPARE(strings.back().isNull(), true);
        QCOMPARE(strings.front(), QString(42, u'x'));
        auto &r = strings.emplace_back(42, u'y');
        QCOMPARE(&r, &strings.back());
        QCOMPARE(strings.size(), 3);
        QCOMPARE(strings.back(), QString(42, u'y'));

        // test growing from empty arrays
        QVarLengthArray<QString> emptyArrDefaultPrealloc;
        QCOMPARE(emptyArrDefaultPrealloc.size(), 0);
        emptyArrDefaultPrealloc.emplace_back();
        QCOMPARE(emptyArrDefaultPrealloc.size(), 1);
        emptyArrDefaultPrealloc.resize(1024);
        QCOMPARE(emptyArrDefaultPrealloc.size(), 1024);
        emptyArrDefaultPrealloc.resize(0);
        QCOMPARE(emptyArrDefaultPrealloc.size(), 0);
        emptyArrDefaultPrealloc.squeeze();
        QCOMPARE(emptyArrDefaultPrealloc.size(), 0);
        emptyArrDefaultPrealloc.emplace_back();
        QCOMPARE(emptyArrDefaultPrealloc.size(), 1);

        QVarLengthArray<QString, 1> emptyArrSmallPrealloc;
        QCOMPARE(emptyArrSmallPrealloc.size(), 0);
        emptyArrSmallPrealloc.emplace_back();
        QCOMPARE(emptyArrSmallPrealloc.size(), 1);
        emptyArrSmallPrealloc.resize(1024);
        QCOMPARE(emptyArrSmallPrealloc.size(), 1024);
        emptyArrSmallPrealloc.resize(0);
        QCOMPARE(emptyArrSmallPrealloc.size(), 0);
        emptyArrSmallPrealloc.squeeze();
        QCOMPARE(emptyArrSmallPrealloc.size(), 0);
        emptyArrSmallPrealloc.emplace_back();
        QCOMPARE(emptyArrSmallPrealloc.size(), 1);
    }
}

template <qsizetype N>
void tst_QVarLengthArray::move_Tracker()
{
    const auto reset = qScopeGuard([] { Tracker::count = 0; });
    move<N, ValueTracker<int>>({24}, {24});
    QCOMPARE(Tracker::count, 0);
}

template <qsizetype N, typename T>
void tst_QVarLengthArray::move(T t1, T t2)
{
    {
        QVarLengthArray<T, N> v;
        v.append(t1);
        v.append(t2);

        auto moved = std::move(v);
        QCOMPARE(moved.size(), 2);
        QCOMPARE(moved[0], t1);
        QCOMPARE(moved[1], t2);

        v = std::move(moved);
        QCOMPARE(v.size(), 2);
        QCOMPARE(v[0], t1);
        QCOMPARE(v[1], t2);
    }
}

void tst_QVarLengthArray::removeLast()
{
    {
        QVarLengthArray<char, 2> v;
        v.append(0);
        v.append(1);
        QCOMPARE(v.size(), 2);
        v.append(2);
        v.append(3);
        QCOMPARE(v.size(), 4);
        v.removeLast();
        QCOMPARE(v.size(), 3);
        v.removeLast();
        QCOMPARE(v.size(), 2);
    }

    {
        QVarLengthArray<QString, 2> v;
        v.append("0");
        v.append("1");
        QCOMPARE(v.size(), 2);
        v.append("2");
        v.append("3");
        QCOMPARE(v.size(), 4);
        v.removeLast();
        QCOMPARE(v.size(), 3);
        v.removeLast();
        QCOMPARE(v.size(), 2);
    }

    {
        Tracker t;
        QCOMPARE(Tracker::count, 1);
        QVarLengthArray<Tracker, 2> v;
        v.append(t);
        v.append({});
        QCOMPARE(Tracker::count, 3);
        v.removeLast();
        QCOMPARE(Tracker::count, 2);
        v.append(t);
        v.append({});
        QCOMPARE(Tracker::count, 4);
        v.removeLast();
        QCOMPARE(Tracker::count, 3);
    }
    QCOMPARE(Tracker::count, 0);
}

void tst_QVarLengthArray::oldTests()
{
    {
        QVarLengthArray<int, 256> sa(128);
        QVERIFY(sa.data() == &sa[0]);
        sa[0] = 0xfee;
        sa[10] = 0xff;
        QVERIFY(sa[0] == 0xfee);
        QVERIFY(sa[10] == 0xff);
        sa.resize(512);
        QVERIFY(sa.data() == &sa[0]);
        QVERIFY(sa[0] == 0xfee);
        QVERIFY(sa[10] == 0xff);
        QVERIFY(sa.at(0) == 0xfee);
        QVERIFY(sa.at(10) == 0xff);
        QVERIFY(sa.value(0) == 0xfee);
        QVERIFY(sa.value(10) == 0xff);
        QVERIFY(sa.value(1000) == 0);
        QVERIFY(sa.value(1000, 12) == 12);
        QVERIFY(sa.size() == 512);
        sa.reserve(1024);
        QVERIFY(sa.capacity() == 1024);
        QVERIFY(sa.size() == 512);
    }
    {
        QVarLengthArray<QString> sa(10);
        sa[0] = "Hello";
        sa[9] = "World";
        QCOMPARE(*sa.data(), QLatin1String("Hello"));
        QCOMPARE(sa[9], QLatin1String("World"));
        sa.reserve(512);
        QCOMPARE(*sa.data(), QLatin1String("Hello"));
        QCOMPARE(sa[9], QLatin1String("World"));
        sa.resize(512);
        QCOMPARE(*sa.data(), QLatin1String("Hello"));
        QCOMPARE(sa[9], QLatin1String("World"));
    }
    {
        int arr[2] = {1, 2};
        QVarLengthArray<int> sa(10);
        QCOMPARE(sa.size(), 10);
        sa.append(arr, 2);
        QCOMPARE(sa.size(), 12);
        QCOMPARE(sa[10], 1);
        QCOMPARE(sa[11], 2);
    }
    {
        QString arr[2] = { QString("hello"), QString("world") };
        QVarLengthArray<QString> sa(10);
        QCOMPARE(sa.size(), 10);
        sa.append(arr, 2);
        QCOMPARE(sa.size(), 12);
        QCOMPARE(sa[10], QString("hello"));
        QCOMPARE(sa[11], QString("world"));
        QCOMPARE(sa.at(10), QString("hello"));
        QCOMPARE(sa.at(11), QString("world"));
        QCOMPARE(sa.value(10), QString("hello"));
        QCOMPARE(sa.value(11), QString("world"));
        QCOMPARE(sa.value(10000), QString());
        QCOMPARE(sa.value(1212112, QString("none")), QString("none"));
        QCOMPARE(sa.value(-12, QString("neg")), QString("neg"));

        sa.append(arr, 1);
        QCOMPARE(sa.size(), 13);
        QCOMPARE(sa[12], QString("hello"));

        sa.append(arr, 0);
        QCOMPARE(sa.size(), 13);
    }
    {
        // assignment operator and copy constructor

        QVarLengthArray<int> sa(10);
        sa[5] = 5;

        QVarLengthArray<int> sa2(10);
        sa2[5] = 6;
        sa2 = sa;
        QCOMPARE(sa2[5], 5);

        QVarLengthArray<int> sa3(sa);
        QCOMPARE(sa3[5], 5);
    }
}

void tst_QVarLengthArray::appendCausingRealloc()
{
    // This is a regression test for an old bug where creating a
    // QVarLengthArray of the same size as the prealloc size would make
    // the next call to append(const T&) corrupt the memory.
    QVarLengthArray<float, 1> d(1);
    for (int i=0; i<30; i++)
        d.append(i);

    // Regression test for QTBUG-110412:
    constexpr qsizetype InitialCapacity = 10;
    QVarLengthArray<float, InitialCapacity> d2(InitialCapacity);
    std::iota(d2.begin(), d2.end(), 0.0f);
    QCOMPARE_EQ(d2.size(), d2.capacity()); // by construction
    float floats[1000];
    std::iota(std::begin(floats), std::end(floats), InitialCapacity + 0.0f);
    d2.append(floats, q20::ssize(floats));
    QCOMPARE_EQ(d2.size(), q20::ssize(floats) + InitialCapacity);
    QCOMPARE_GE(d2.capacity(), d2.size());
}

void tst_QVarLengthArray::appendIsStronglyExceptionSafe()
{
#ifdef QT_NO_EXCEPTIONS
    QSKIP("This test requires exception support enabled in the compiler.");
#else
    static bool throwOnCopyNow = false;
    static bool throwOnMoveNow = false;
    struct Thrower {
        Thrower() = default;
        Thrower(const Thrower &)
        {
            if (throwOnCopyNow)
                throw 1;
        }
        Thrower &operator=(const Thrower &) = default;
        Thrower(Thrower &&)
        {
            if (throwOnMoveNow)
                throw 1;
        }
        Thrower &operator=(Thrower &&) = default;
        ~Thrower() = default;
    };

    {
        QVarLengthArray<Thrower, 2> vla(1);
        {
            Thrower t;
            const QScopedValueRollback rb(throwOnCopyNow, true);
            QVERIFY_THROWS_EXCEPTION(int, vla.push_back(t));
            QCOMPARE(vla.size(), 1);
        }
        {
            const QScopedValueRollback rb(throwOnMoveNow, true);
            QVERIFY_THROWS_EXCEPTION(int, vla.push_back({}));
            QCOMPARE(vla.size(), 1);
        }
        vla.push_back({});
        QCOMPARE(vla.size(), 2);
        {
            Thrower t;
            {
                // tests the copy inside append()
                const QScopedValueRollback rb(throwOnCopyNow, true);
                QVERIFY_THROWS_EXCEPTION(int, vla.push_back(t));
                QCOMPARE(vla.size(), 2);
            }
            {
                // tests the move inside reallocate()
                const QScopedValueRollback rb(throwOnMoveNow, true);
                QVERIFY_THROWS_EXCEPTION(int, vla.push_back(t));
                QCOMPARE(vla.size(), 2);
            }
        }
        {
            const QScopedValueRollback rb(throwOnMoveNow, true);
            QVERIFY_THROWS_EXCEPTION(int, vla.push_back({}));
            QCOMPARE(vla.size(), 2);
        }
    }
#endif
}

void tst_QVarLengthArray::resize()
{
    // Empty Movable
    {
        QVarLengthArray<QVariant, 1> values;
        QCOMPARE(values.size(), 0);
        values.resize(2);
        QCOMPARE(values.size(), 2);
        QCOMPARE(values[0], QVariant());
        QCOMPARE(values[1], QVariant());
    }

    // Empty POD
    {
        QVarLengthArray<int, 1> values;
        QCOMPARE(values.size(), 0);
        values.resize(2);
        QCOMPARE(values.size(), 2);
        // POD values are uninitialized, but we can check that we can assign
        // new values
        values[0] = 0;
        values[1] = 1;

        QCOMPARE(values[0], 0);
        QCOMPARE(values[1], 1);
    }

    //MOVABLE
    {
        QVarLengthArray<QVariant,1> values(1);
        QCOMPARE(values.size(), 1);
        values[0] = 1;
        values.resize(2);
        QCOMPARE(values[1], QVariant());
        QCOMPARE(values[0], QVariant(1));
        values[1] = 2;
        QCOMPARE(values[1], QVariant(2));
        QCOMPARE(values.size(), 2);
    }

    //POD
    {
        QVarLengthArray<int,1> values(1);
        QCOMPARE(values.size(), 1);
        values[0] = 1;
        values.resize(2);
        QCOMPARE(values[0], 1);
        values[1] = 2;
        QCOMPARE(values[1], 2);
        QCOMPARE(values.size(), 2);
    }

    //COMPLEX
    {
        QVarLengthArray<QVarLengthArray<QString, 15>,1> values(1);
        QCOMPARE(values.size(), 1);
        values[0].resize(10);
        values.resize(2);
        QCOMPARE(values[1].size(), 0);
        QCOMPARE(values[0].size(), 10);
        values[1].resize(20);
        QCOMPARE(values[1].size(), 20);
        QCOMPARE(values.size(), 2);
    }
}

struct MyBase
{
    MyBase()
        : data(this)
        , isCopy(false)
    {
        ++liveCount;
    }

    MyBase(MyBase const &)
        : data(this)
        , isCopy(true)
    {
        ++copyCount;
        ++liveCount;
    }

    MyBase & operator=(MyBase const &)
    {
        if (!isCopy) {
            isCopy = true;
            ++copyCount;
        } else {
            ++errorCount;
        }
        if (!data) {
            --movedCount;
            ++liveCount;
        }
        data = this;

        return *this;
    }

    ~MyBase()
    {
        if (isCopy) {
            if (!copyCount || !data)
                ++errorCount;
            else
                --copyCount;
        }

        if (data) {
            if (!liveCount)
                ++errorCount;
            else
                --liveCount;
        } else
            --movedCount;
    }

    bool wasConstructedAt(const MyBase *that) const
    {
        return that == data;
    }

    bool hasMoved() const { return !wasConstructedAt(this); }

protected:
    void swap(MyBase &other) {
        using std::swap;
        swap(data, other.data);
        swap(isCopy, other.isCopy);
    }

    MyBase(const MyBase *data, bool isCopy)
            : data(data), isCopy(isCopy) {}

    const MyBase *data;
    bool isCopy;

public:
    static int errorCount;
    static int liveCount;
    static int copyCount;
    static int movedCount;
};

int MyBase::errorCount = 0;
int MyBase::liveCount = 0;
int MyBase::copyCount = 0;
int MyBase::movedCount = 0;

struct MyPrimitive
    : MyBase
{
    MyPrimitive()
    {
        ++errorCount;
    }

    ~MyPrimitive()
    {
        ++errorCount;
    }

    MyPrimitive(MyPrimitive const &other)
        : MyBase(other)
    {
        ++errorCount;
    }
};

struct MyMovable
    : MyBase
{
    MyMovable(char input = 'j') : MyBase(), i(input) {}

    MyMovable(MyMovable const &other) : MyBase(other), i(other.i) {}

    MyMovable(MyMovable &&other) : MyBase(other.data, other.isCopy), i(other.i)
    {
        ++movedCount;
        other.isCopy = false;
        other.data = nullptr;
    }

    MyMovable & operator=(const MyMovable &other)
    {
        MyBase::operator=(other);
        i = other.i;
        return *this;
    }

    MyMovable & operator=(MyMovable &&other)
    {
        if (isCopy)
            --copyCount;
        ++movedCount;
        if (other.data)
            --liveCount;
        isCopy = other.isCopy;
        data = other.data;
        other.isCopy = false;
        other.data = nullptr;

        return *this;
    }

    void swap(MyMovable &other) noexcept
    {
        MyBase::swap(other);
        std::swap(i, other.i);
    }

    friend void swap(MyMovable &lhs, MyMovable &rhs) noexcept { lhs.swap(rhs); }

    bool operator==(const MyMovable &other) const
    {
        return i == other.i;
    }
    char i;
};

struct MyComplex
    : MyBase
{
    MyComplex(char input = 'j') : i(input) {}
    bool operator==(const MyComplex &other) const
    {
        return i == other.i;
    }

    void swap(MyComplex &other) noexcept
    {
        MyBase::swap(other);
        std::swap(i, other.i);
    }

    friend void swap(MyComplex &lhs, MyComplex &rhs) noexcept { lhs.swap(rhs); }

    char i;
};

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(MyPrimitive, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(MyMovable, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(MyComplex, Q_COMPLEX_TYPE);

QT_END_NAMESPACE

bool reallocTestProceed = true;

template <class T, qsizetype PreAlloc>
qsizetype countMoved(QVarLengthArray<T, PreAlloc> const &c)
{
    qsizetype result = 0;
    for (qsizetype i = 0; i < c.size(); ++i)
        if (c[i].hasMoved())
            ++result;

    return result;
}

template <class T>
void reallocTest()
{
    reallocTestProceed = false;

    typedef QVarLengthArray<T, 16> Container;
    enum {
        isRelocatable = QTypeInfo<T>::isRelocatable,
        isComplex = QTypeInfo<T>::isComplex,

        isPrimitive = !isComplex && isRelocatable,
        isMovable = isRelocatable
    };

    // Constructors
    Container a;
    QCOMPARE( MyBase::liveCount, 0 );
    QCOMPARE( MyBase::copyCount, 0 );

    QVERIFY( a.capacity() >= 16 );
    QCOMPARE( a.size(), 0 );

    Container b_real(8);
    Container const &b = b_real;
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 8 );
    QCOMPARE( MyBase::copyCount, 0 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // Assignment
    a = b;
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 16 );
    QCOMPARE( MyBase::copyCount, isComplex ? 8 : 0 );
    QVERIFY( a.capacity() >= 16 );
    QCOMPARE( a.size(), 8 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // append
    a.append(b.data(), b.size());
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 24 );
    QCOMPARE( MyBase::copyCount, isComplex ? 16 : 0 );

    QVERIFY( a.capacity() >= 16 );
    QCOMPARE( a.size(), 16 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // removeLast
    a.removeLast();
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 23 );
    QCOMPARE( MyBase::copyCount, isComplex ? 15 : 0 );

    QVERIFY( a.capacity() >= 16 );
    QCOMPARE( a.size(), 15 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // Movable types
    const int capacity = a.capacity();
    if (!isPrimitive)
        QCOMPARE( countMoved(a), 0 );

    // Reserve, no re-allocation
    a.reserve(capacity);
    if (!isPrimitive)
        QCOMPARE( countMoved(a), 0 );
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 23 );
    QCOMPARE( MyBase::copyCount, isComplex ? 15 : 0 );

    QCOMPARE( a.capacity(), capacity );
    QCOMPARE( a.size(), 15 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // Reserve, force re-allocation
    a.reserve(capacity * 2);
    if (!isPrimitive)
        QCOMPARE( countMoved(a), isMovable ? 15 : 0 );
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 23 );
    QCOMPARE( MyBase::copyCount, isComplex ? 15 : 0 );

    QVERIFY( a.capacity() >= capacity * 2 );
    QCOMPARE( a.size(), 15 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // resize, grow
    a.resize(40);
    if (!isPrimitive)
        QCOMPARE( countMoved(a), isMovable ? 15 : 0 );
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 48 );
    QCOMPARE( MyBase::copyCount, isComplex ? 15 : 0 );

    QVERIFY( a.capacity() >= a.size() );
    QCOMPARE( a.size(), 40 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // Copy constructor, allocate
    {
        Container c(a);
        if (!isPrimitive)
            QCOMPARE( countMoved(c), 0 );
        QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 88 );
        QCOMPARE( MyBase::copyCount, isComplex ? 55 : 0 );

        QVERIFY( a.capacity() >= a.size() );
        QCOMPARE( a.size(), 40 );

        QVERIFY( b.capacity() >= 16 );
        QCOMPARE( b.size(), 8 );

        QVERIFY( c.capacity() >= 40 );
        QCOMPARE( c.size(), 40 );
    }

    // resize, shrink
    a.resize(10);
    if (!isPrimitive)
        QCOMPARE( countMoved(a), isMovable ? 10 : 0 );
    QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 18 );
    QCOMPARE( MyBase::copyCount, isComplex ? 10 : 0 );

    QVERIFY( a.capacity() >= a.size() );
    QCOMPARE( a.size(), 10 );

    QVERIFY( b.capacity() >= 16 );
    QCOMPARE( b.size(), 8 );

    // Copy constructor, don't allocate
    {
        Container c(a);
        if (!isPrimitive)
            QCOMPARE( countMoved(c), 0 );
        QCOMPARE( MyBase::liveCount, isPrimitive ? 0 : 28 );
        QCOMPARE( MyBase::copyCount, isComplex ? 20 : 0 );

        QVERIFY( a.capacity() >= a.size() );
        QCOMPARE( a.size(), 10 );

        QVERIFY( b.capacity() >= 16 );
        QCOMPARE( b.size(), 8 );

        QVERIFY( c.capacity() >= 16 );
        QCOMPARE( c.size(), 10 );
    }

    a.clear();
    QCOMPARE( a.size(), 0 );

    b_real.clear();
    QCOMPARE( b.size(), 0 );

    QCOMPARE(MyBase::errorCount, 0);
    QCOMPARE(MyBase::liveCount, 0);

    // All done
    reallocTestProceed = true;
}

void tst_QVarLengthArray::realloc()
{
    reallocTest<MyBase>();
    QVERIFY(reallocTestProceed);

    reallocTest<MyPrimitive>();
    QVERIFY(reallocTestProceed);

    reallocTest<MyMovable>();
    QVERIFY(reallocTestProceed);

    reallocTest<MyComplex>();
    QVERIFY(reallocTestProceed);
}

void tst_QVarLengthArray::iterators()
{
    QVarLengthArray<int> emptyArr;
    QCOMPARE(emptyArr.constBegin(), emptyArr.constEnd());
    QCOMPARE(emptyArr.cbegin(), emptyArr.cend());
    QCOMPARE(emptyArr.begin(), emptyArr.end());

    QVarLengthArray<int> arr { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto it = arr.begin();
    auto constIt = arr.cbegin();
    qsizetype idx = 0;

    QCOMPARE(*it, arr[idx]);
    QCOMPARE(*constIt, arr[idx]);

    it++;
    constIt++;
    idx++;
    QCOMPARE(*it, arr[idx]);
    QCOMPARE(*constIt, arr[idx]);

    it += 5;
    constIt += 5;
    idx += 5;
    QCOMPARE(*it, arr[idx]);
    QCOMPARE(*constIt, arr[idx]);

    it -= 3;
    constIt -= 3;
    idx -= 3;
    QCOMPARE(*it, arr[idx]);
    QCOMPARE(*constIt, arr[idx]);

    it--;
    constIt--;
    idx--;
    QCOMPARE(*it, arr[idx]);
    QCOMPARE(*constIt, arr[idx]);
}

void tst_QVarLengthArray::reverseIterators()
{
    QVarLengthArray<int> emptyArr;
    QCOMPARE(emptyArr.crbegin(), emptyArr.crend());
    QCOMPARE(emptyArr.rbegin(), emptyArr.rend());

    QVarLengthArray<int> v;
    v << 1 << 2 << 3 << 4;
    QVarLengthArray<int> vr = v;
    std::reverse(vr.begin(), vr.end());
    const QVarLengthArray<int> &cvr = vr;
    QVERIFY(std::equal(v.begin(), v.end(), vr.rbegin()));
    QVERIFY(std::equal(v.begin(), v.end(), vr.crbegin()));
    QVERIFY(std::equal(v.begin(), v.end(), cvr.rbegin()));
    QVERIFY(std::equal(vr.rbegin(), vr.rend(), v.begin()));
    QVERIFY(std::equal(vr.crbegin(), vr.crend(), v.begin()));
    QVERIFY(std::equal(cvr.rbegin(), cvr.rend(), v.begin()));
}

void tst_QVarLengthArray::count()
{
    // tests size(), count() and length(), since they're the same thing
    {
        const QVarLengthArray<int> list;
        QCOMPARE(list.size(), 0);
        QCOMPARE(list.size(), 0);
        QCOMPARE(list.size(), 0);
        QVERIFY(list.isEmpty());
    }

    {
        QVarLengthArray<int> list;
        list.append(0);
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.size(), 1);
        QVERIFY(!list.isEmpty());
    }

    {
        QVarLengthArray<int> list;
        list.append(0);
        list.append(1);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.size(), 2);
        QVERIFY(!list.isEmpty());
    }

    {
        QVarLengthArray<int> list;
        list.append(0);
        list.append(0);
        list.append(0);
        QCOMPARE(list.size(), 3);
        QCOMPARE(list.size(), 3);
        QCOMPARE(list.size(), 3);
        QVERIFY(!list.isEmpty());
    }

    // test removals too
    {
        QVarLengthArray<int> list;
        list.append(0);
        list.append(0);
        list.append(0);
        QCOMPARE(list.size(), 3);
        QCOMPARE(list.size(), 3);
        QCOMPARE(list.size(), 3);
        QVERIFY(!list.isEmpty());
        list.removeLast();
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.size(), 2);
        QVERIFY(!list.isEmpty());
        list.removeLast();
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.size(), 1);
        QVERIFY(!list.isEmpty());
        list.removeLast();
        QCOMPARE(list.size(), 0);
        QCOMPARE(list.size(), 0);
        QCOMPARE(list.size(), 0);
        QVERIFY(list.isEmpty());
    }
}

void tst_QVarLengthArray::cpp17ctad()
{
#define QVERIFY_IS_VLA_OF(obj, Type) \
    QVERIFY2((std::is_same<decltype(obj), QVarLengthArray<Type>>::value), \
             QMetaType::fromType<decltype(obj)::value_type>().name())
#define CHECK(Type, One, Two, Three) \
    do { \
        const Type v[] = {One, Two, Three}; \
        QVarLengthArray v1 = {One, Two, Three}; \
        QVERIFY_IS_VLA_OF(v1, Type); \
        QVarLengthArray v2(v1.begin(), v1.end()); \
        QVERIFY_IS_VLA_OF(v2, Type); \
        QVarLengthArray v3(std::begin(v), std::end(v)); \
        QVERIFY_IS_VLA_OF(v3, Type); \
    } while (false) \
    /*end*/
    CHECK(int, 1, 2, 3);
    CHECK(double, 1.0, 2.0, 3.0);
    CHECK(QString, QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three"));
#undef QVERIFY_IS_VLA_OF
#undef CHECK
}

void tst_QVarLengthArray::first()
{
    // append some items, make sure it stays sane
    QVarLengthArray<int> list;
    list.append(27);
    QCOMPARE(list.first(), 27);
    list.append(4);
    QCOMPARE(list.first(), 27);
    list.append(1987);
    QCOMPARE(list.first(), 27);
    QCOMPARE(list.size(), 3);

    // remove some, make sure it stays sane
    list.removeLast();
    QCOMPARE(list.first(), 27);
    QCOMPARE(list.size(), 2);

    list.removeLast();
    QCOMPARE(list.first(), 27);
    QCOMPARE(list.size(), 1);
}

void tst_QVarLengthArray::last()
{
    // append some items, make sure it stays sane
    QVarLengthArray<int> list;
    list.append(27);
    QCOMPARE(list.last(), 27);
    list.append(4);
    QCOMPARE(list.last(), 4);
    list.append(1987);
    QCOMPARE(list.last(), 1987);
    QCOMPARE(list.size(), 3);

    // remove some, make sure it stays sane
    list.removeLast();
    QCOMPARE(list.last(), 4);
    QCOMPARE(list.size(), 2);

    list.removeLast();
    QCOMPARE(list.last(), 27);
    QCOMPARE(list.size(), 1);
}

void tst_QVarLengthArray::squeeze()
{
    QVarLengthArray<int, 100> list;
    qsizetype sizeOnStack = list.capacity();
    QCOMPARE(sizeOnStack, 100);
    list.squeeze();
    QCOMPARE(list.capacity(), sizeOnStack);

    qsizetype sizeOnHeap = sizeOnStack * 2;
    list.resize(0);
    QCOMPARE(list.capacity(), sizeOnStack);
    list.resize(sizeOnHeap);
    QCOMPARE(list.capacity(), sizeOnHeap);
    list.resize(sizeOnStack);
    QCOMPARE(list.capacity(), sizeOnHeap);
    list.resize(0);
    QCOMPARE(list.capacity(), sizeOnHeap);
    list.squeeze();
    QCOMPARE(list.capacity(), sizeOnStack);
    list.resize(sizeOnStack);
    list.squeeze();
    QCOMPARE(list.capacity(), sizeOnStack);
    list.resize(sizeOnHeap);
    list.squeeze();
    QCOMPARE(list.capacity(), sizeOnHeap);
}

void tst_QVarLengthArray::operators()
{
    QVarLengthArray<QString, 6> myvla;
    myvla << "A" << "B" << "C";
    QVarLengthArray<QString, 3> myvlatwo;
    myvlatwo << "D" << "E" << "F";
    QVarLengthArray<QString, 7> combined;
    combined << "A" << "B" << "C" << "D" << "E" << "F";

    // !=
    QVERIFY(myvla != myvlatwo);

    // +=: not provided, emulate
    //myvla += myvlatwo;
    for (const QString &s : std::as_const(myvlatwo))
        myvla.push_back(s);
    QCOMPARE(myvla, combined);

    // ==
    QVERIFY(myvla == combined);

    // <, >, <=, >=
    QVERIFY(!(myvla <  combined));
    QVERIFY(!(myvla >  combined));
    QVERIFY(  myvla <= combined);
    QVERIFY(  myvla >= combined);
    combined.push_back("G");
    QVERIFY(  myvla <  combined);
    QVERIFY(!(myvla >  combined));
    QVERIFY(  myvla <= combined);
    QVERIFY(!(myvla >= combined));
    QVERIFY(combined >  myvla);
    QVERIFY(combined >= myvla);

    // []
    QCOMPARE(myvla[0], QLatin1String("A"));
    QCOMPARE(myvla[1], QLatin1String("B"));
    QCOMPARE(myvla[2], QLatin1String("C"));
    QCOMPARE(myvla[3], QLatin1String("D"));
    QCOMPARE(myvla[4], QLatin1String("E"));
    QCOMPARE(myvla[5], QLatin1String("F"));
}

void tst_QVarLengthArray::indexOf()
{
    QVarLengthArray<QString> myvec;

    QCOMPARE(myvec.indexOf("A"), -1);
    QCOMPARE(myvec.indexOf("A", 5), -1);

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

void tst_QVarLengthArray::lastIndexOf()
{
    QVarLengthArray<QString> myvec;

    QCOMPARE(myvec.lastIndexOf("A"), -1);
    QCOMPARE(myvec.lastIndexOf("A", 5), -1);

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

void tst_QVarLengthArray::contains()
{
    QVarLengthArray<QString> myvec;

    QVERIFY(!myvec.contains(QLatin1String("aaa")));
    QVERIFY(!myvec.contains(QString()));

    myvec << "aaa" << "bbb" << "ccc";

    QVERIFY(myvec.contains(QLatin1String("aaa")));
    QVERIFY(myvec.contains(QLatin1String("bbb")));
    QVERIFY(myvec.contains(QLatin1String("ccc")));
    QVERIFY(!myvec.contains(QLatin1String("I don't exist")));

    // add it and make sure it does :)
    myvec.append(QLatin1String("I don't exist"));
    QVERIFY(myvec.contains(QLatin1String("I don't exist")));
}

void tst_QVarLengthArray::clear()
{
    QVarLengthArray<QString, 5> myvec;
    QCOMPARE(myvec.size(), 0);
    myvec.clear();
    QCOMPARE(myvec.size(), 0);

    for (int i = 0; i < 10; ++i)
        myvec << "aaa";

    QCOMPARE(myvec.size(), 10);
    QVERIFY(myvec.capacity() >= myvec.size());
    const int oldCapacity = myvec.capacity();
    myvec.clear();
    QCOMPARE(myvec.size(), 0);
    QCOMPARE(myvec.capacity(), oldCapacity);
}

void tst_QVarLengthArray::initializeListInt()
{
    initializeList<int>();
}

void tst_QVarLengthArray::initializeListMovable()
{
    const int instancesCount = MyMovable::liveCount;
    initializeList<MyMovable>();
    QCOMPARE(MyMovable::liveCount, instancesCount);
}

void tst_QVarLengthArray::initializeListComplex()
{
    const int instancesCount = MyComplex::liveCount;
    initializeList<MyComplex>();
    QCOMPARE(MyComplex::liveCount, instancesCount);
}

template<typename T>
void tst_QVarLengthArray::initializeList()
{
    T val1(110);
    T val2(105);
    T val3(101);
    T val4(114);

    // QVarLengthArray(std::initializer_list<>)
    QVarLengthArray<T> v1 {val1, val2, val3};
    QCOMPARE(v1, QVarLengthArray<T>() << val1 << val2 << val3);
    QCOMPARE(v1, (QVarLengthArray<T> {val1, val2, val3}));

    QVarLengthArray<QVarLengthArray<T>, 4> v2{ v1, {val4}, QVarLengthArray<T>(), {val1, val2, val3} };
    QVarLengthArray<QVarLengthArray<T>, 4> v3;
    v3 << v1 << (QVarLengthArray<T>() << val4) << QVarLengthArray<T>() << v1;
    QCOMPARE(v3, v2);

    QVarLengthArray<T> v4({});
    QCOMPARE(v4.size(), 0);

    // operator=(std::initializer_list<>)

    QVarLengthArray<T> v5({val2, val1});
    v1 = { val1, val2 }; // make array smaller
    v4 = { val1, val2 }; // make array bigger
    v5 = { val1, val2 }; // same size
    QCOMPARE(v1, QVarLengthArray<T>() << val1 << val2);
    QCOMPARE(v4, v1);
    QCOMPARE(v5, v1);

    QVarLengthArray<T, 1> v6 = { val1 };
    v6 = { val1, val2 }; // force allocation on heap
    QCOMPARE(v6.size(), 2);
    QCOMPARE(v6.first(), val1);
    QCOMPARE(v6.last(), val2);

    v6 = {}; // assign empty
    QCOMPARE(v6.size(), 0);
}

void tst_QVarLengthArray::insertMove()
{
    MyBase::errorCount = 0;
    QCOMPARE(MyBase::liveCount, 0);
    QCOMPARE(MyBase::copyCount, 0);

    {
        MyMovable m1, m2;
        QCOMPARE(MyBase::liveCount, 2);
        QCOMPARE(MyBase::copyCount, 0);
        using std::swap;
        swap(m1, m2);
        QCOMPARE(MyBase::liveCount, 2);
        QCOMPARE(MyBase::movedCount, 0);
        QCOMPARE(MyBase::copyCount, 0);
    }

    {
        QVarLengthArray<MyMovable, 6> vec;
        MyMovable m1;
        MyMovable m2;
        MyMovable m3;
        MyMovable m4;
        MyMovable m5;
        MyMovable m6;
        QCOMPARE(MyBase::copyCount, 0);
        QCOMPARE(MyBase::liveCount, 6);

        vec.append(std::move(m3));
        QVERIFY(m3.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m3));
        QCOMPARE(MyBase::errorCount, 0);
        QCOMPARE(MyBase::liveCount, 6);
        QCOMPARE(MyBase::movedCount, 1);

        vec.push_back(std::move(m4));
        QVERIFY(m4.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m3));
        QVERIFY(vec.at(1).wasConstructedAt(&m4));
        QCOMPARE(MyBase::errorCount, 0);
        QCOMPARE(MyBase::liveCount, 6);
        QCOMPARE(MyBase::movedCount, 2);

        vec.insert(vec.cbegin(), std::move(m1));
        QVERIFY(m1.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m3));
        QVERIFY(vec.at(2).wasConstructedAt(&m4));
        QCOMPARE(MyBase::errorCount, 0);
        QCOMPARE(MyBase::liveCount, 6);
        QCOMPARE(MyBase::movedCount, 3);

        vec.insert(1, std::move(m2));
        QVERIFY(m2.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m2));
        QVERIFY(vec.at(2).wasConstructedAt(&m3));
        QVERIFY(vec.at(3).wasConstructedAt(&m4));
        QCOMPARE(MyBase::errorCount, 0);
        QCOMPARE(MyBase::liveCount, 6);
        QCOMPARE(MyBase::movedCount, 4);

        vec += std::move(m5);
        QVERIFY(m5.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m2));
        QVERIFY(vec.at(2).wasConstructedAt(&m3));
        QVERIFY(vec.at(3).wasConstructedAt(&m4));
        QVERIFY(vec.at(4).wasConstructedAt(&m5));
        QCOMPARE(MyBase::errorCount, 0);
        QCOMPARE(MyBase::liveCount, 6);
        QCOMPARE(MyBase::movedCount, 5);

        vec << std::move(m6);
        QVERIFY(m6.wasConstructedAt(nullptr));
        QVERIFY(vec.at(0).wasConstructedAt(&m1));
        QVERIFY(vec.at(1).wasConstructedAt(&m2));
        QVERIFY(vec.at(2).wasConstructedAt(&m3));
        QVERIFY(vec.at(3).wasConstructedAt(&m4));
        QVERIFY(vec.at(4).wasConstructedAt(&m5));
        QVERIFY(vec.at(5).wasConstructedAt(&m6));

        QCOMPARE(MyBase::copyCount, 0);
        QCOMPARE(MyBase::liveCount, 6);
        QCOMPARE(MyBase::errorCount, 0);
        QCOMPARE(MyBase::movedCount, 6);
    }
    QCOMPARE(MyBase::liveCount, 0);
    QCOMPARE(MyBase::errorCount, 0);
    QCOMPARE(MyBase::movedCount, 0);
}

void tst_QVarLengthArray::nonCopyable()
{
    QVarLengthArray<std::unique_ptr<int>> vec;
    std::unique_ptr<int> val1(new int(1));
    std::unique_ptr<int> val2(new int(2));
    std::unique_ptr<int> val3(new int(3));
    std::unique_ptr<int> val4(new int(4));
    std::unique_ptr<int> val5(new int(5));
    std::unique_ptr<int> val6(new int(6));
    int *const ptr1 = val1.get();
    int *const ptr2 = val2.get();
    int *const ptr3 = val3.get();
    int *const ptr4 = val4.get();
    int *const ptr5 = val5.get();
    int *const ptr6 = val6.get();

    vec.append(std::move(val3));
    QVERIFY(!val3);
    QVERIFY(ptr3 == vec.at(0).get());
    vec.append(std::move(val4));
    QVERIFY(!val4);
    QVERIFY(ptr3 == vec.at(0).get());
    QVERIFY(ptr4 == vec.at(1).get());
    vec.insert(vec.cbegin(), std::move(val1));
    QVERIFY(!val1);
    QVERIFY(ptr1 == vec.at(0).get());
    QVERIFY(ptr3 == vec.at(1).get());
    QVERIFY(ptr4 == vec.at(2).get());
    vec.insert(1, std::move(val2));
    QVERIFY(!val2);
    QVERIFY(ptr1 == vec.at(0).get());
    QVERIFY(ptr2 == vec.at(1).get());
    QVERIFY(ptr3 == vec.at(2).get());
    QVERIFY(ptr4 == vec.at(3).get());
    vec += std::move(val5);
    QVERIFY(!val5);
    QVERIFY(ptr1 == vec.at(0).get());
    QVERIFY(ptr2 == vec.at(1).get());
    QVERIFY(ptr3 == vec.at(2).get());
    QVERIFY(ptr4 == vec.at(3).get());
    QVERIFY(ptr5 == vec.at(4).get());
    vec << std::move(val6);
    QVERIFY(!val6);
    QVERIFY(ptr1 == vec.at(0).get());
    QVERIFY(ptr2 == vec.at(1).get());
    QVERIFY(ptr3 == vec.at(2).get());
    QVERIFY(ptr4 == vec.at(3).get());
    QVERIFY(ptr5 == vec.at(4).get());
    QVERIFY(ptr6 == vec.at(5).get());
}

void tst_QVarLengthArray::implicitDefaultCtor()
{
    QVarLengthArray<int> def = {};
    QCOMPARE(def.size(), 0);
}

void tst_QVarLengthArray::reserve()
{
    QVarLengthArray<int, 100> arr;
    QCOMPARE(arr.capacity(), 100);
    QCOMPARE(arr.size(), 0);

    const auto *stackPtr = arr.constData();
    arr.reserve(50);
    // Nothing changed, as we reserve less than pre-allocated
    QCOMPARE(arr.capacity(), 100);
    QCOMPARE(arr.size(), 0);
    QCOMPARE(arr.constData(), stackPtr);

    arr.reserve(150);
    // Allocate memory on heap, as we reserve more than pre-allocated
    QCOMPARE(arr.capacity(), 150);
    QCOMPARE(arr.size(), 0);
    const auto *heapPtr = arr.constData();
    QVERIFY(heapPtr != stackPtr);

    arr.reserve(50);
    // Nothing changed
    QCOMPARE(arr.capacity(), 150);
    QCOMPARE(arr.constData(), heapPtr);

    arr.squeeze();
    // After squeeze() we go back to using stack
    QCOMPARE(arr.capacity(), 100);
    QCOMPARE(arr.constData(), stackPtr);
}

void tst_QVarLengthArray::value()
{
    const QString def("default value");

    QVarLengthArray<QString> arr;
    QCOMPARE(arr.value(0), QString());
    QCOMPARE(arr.value(1, def), def);
    QCOMPARE(arr.value(-1, def), def);

    const qsizetype size = 5;
    const QString dataStr("data%1");
    arr.resize(size);
    for (qsizetype i = 0; i < size; ++i)
        arr[i] = dataStr.arg(i);

    for (qsizetype i = 0; i < size; ++i)
        QCOMPARE(arr.value(i, def), dataStr.arg(i));

    QCOMPARE(arr.value(size + 1), QString());
    QCOMPARE(arr.value(-1, def), def);
}

void tst_QVarLengthArray::insert()
{
    QFETCH(QVarLengthArray<QString>, arr);
    QFETCH(int, pos);
    QFETCH(int, count);
    QFETCH(QString, data);
    QFETCH(QVarLengthArray<QString>, expected);

    // Insert using index
    {
        QVarLengthArray<QString> copy = arr;
        if (count == 1) {
            copy.insert(pos, data);
            QCOMPARE(copy, expected);

            copy = arr;
            QString d = data;
            copy.insert(pos, std::move(d));
            QCOMPARE(copy, expected);
        } else {
            copy.insert(pos, count, data);
            QCOMPARE(copy, expected);
        }
    }

    // Insert using iterator
    {
        QVarLengthArray<QString> copy = arr;
        if (count == 1) {
            copy.insert(copy.cbegin() + pos, data);
            QCOMPARE(copy, expected);

            copy = arr;
            QString d = data;
            copy.insert(copy.cbegin() + pos, std::move(d));
            QCOMPARE(copy, expected);
        } else {
            copy.insert(copy.cbegin() + pos, count, data);
            QCOMPARE(copy, expected);
        }
    }
}

void tst_QVarLengthArray::insert_data()
{
    QTest::addColumn<QVarLengthArray<QString>>("arr");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("count");
    QTest::addColumn<QString>("data");
    QTest::addColumn<QVarLengthArray<QString>>("expected");

    const QString data("Test");

    QTest::newRow("empty")
            << QVarLengthArray<QString>() << 0 << 1 << data << QVarLengthArray<QString>({ data });
    QTest::newRow("empty-none")
            << QVarLengthArray<QString>() << 0 << 0 << data << QVarLengthArray<QString>();
    QTest::newRow("begin")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 0 << 1 << data
            << QVarLengthArray<QString>({ data, "value1", "value2" });
    QTest::newRow("end")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 2 << 1 << data
            << QVarLengthArray<QString>({ "value1", "value2", data });
    QTest::newRow("middle")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 1 << 1 << data
            << QVarLengthArray<QString>({ "value1", data, "value2" });
    QTest::newRow("begin-none")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 0 << 0 << data
            << QVarLengthArray<QString>({ "value1", "value2" });
    QTest::newRow("end-none")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 2 << 0 << data
            << QVarLengthArray<QString>({ "value1", "value2" });
    QTest::newRow("middle-none")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 1 << 0 << data
            << QVarLengthArray<QString>({ "value1", "value2" });
    QTest::newRow("multi begin")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 0 << 2 << data
            << QVarLengthArray<QString>({ data, data, "value1", "value2" });
    QTest::newRow("multi end")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 2 << 2 << data
            << QVarLengthArray<QString>({ "value1", "value2", data, data });
    QTest::newRow("multi middle")
            << QVarLengthArray<QString>({ "value1", "value2" }) << 1 << 2 << data
            << QVarLengthArray<QString>({ "value1", data, data, "value2" });
}

void tst_QVarLengthArray::replace()
{
    QVarLengthArray<QString> arr({ "val0", "val1", "val2" });

    arr.replace(0, "data0");
    QCOMPARE(arr, QVarLengthArray<QString>({ "data0", "val1", "val2" }));

    arr.replace(2, "data2");
    QCOMPARE(arr, QVarLengthArray<QString>({ "data0", "val1", "data2" }));

    arr.replace(1, "data1");
    QCOMPARE(arr, QVarLengthArray<QString>({ "data0", "data1", "data2" }));
}

void tst_QVarLengthArray::remove()
{
    auto isVal2 = [](const QString &str) { return str == "val2"; };

    QVarLengthArray<QString> arr;
    QCOMPARE(arr.removeAll("val0"), 0);
    QVERIFY(!arr.removeOne("val1"));
    QCOMPARE(arr.removeIf(isVal2), 0);

    arr << "val0" << "val1" << "val2";
    arr << "val0" << "val1" << "val2";
    arr << "val0" << "val1" << "val2";

    QCOMPARE(arr.size(), 9);

    arr.remove(1, 3);
    QCOMPARE(arr, QVarLengthArray<QString>({ "val0", "val1", "val2", "val0", "val1", "val2" }));

    arr.remove(2);
    QCOMPARE(arr, QVarLengthArray<QString>({ "val0", "val1", "val0", "val1", "val2" }));

    QVERIFY(arr.removeOne("val1"));
    QCOMPARE(arr, QVarLengthArray<QString>({ "val0", "val0", "val1", "val2" }));

    QCOMPARE(arr.removeAll("val0"), 2);
    QCOMPARE(arr, QVarLengthArray<QString>({ "val1", "val2" }));

    QCOMPARE(arr.removeIf(isVal2), 1);
    QCOMPARE(arr, QVarLengthArray<QString>({ "val1" }));

    arr.removeLast();
    QVERIFY(arr.isEmpty());
}

void tst_QVarLengthArray::erase()
{
    QVarLengthArray<QString> arr;
    QCOMPARE(arr.erase(arr.cbegin(), arr.cend()), arr.cend());

    arr << "val0" << "val1" << "val2";
    arr << "val0" << "val1" << "val2";
    arr << "val0" << "val1" << "val2";

    auto it = arr.erase(arr.cbegin() + 1, arr.cend() - 3);
    QCOMPARE(it, arr.cend() - 3);
    QCOMPARE(arr, QVarLengthArray<QString>({ "val0", "val0", "val1", "val2" }));

    it = arr.erase(arr.cbegin());
    QCOMPARE(it, arr.cbegin());
    QCOMPARE(arr, QVarLengthArray<QString>({ "val0", "val1", "val2" }));

    it = arr.erase(arr.cbegin() + 1);
    QCOMPARE(it, arr.cend() - 1);
    QCOMPARE(arr, QVarLengthArray<QString>({ "val0", "val2" }));

    it = arr.erase(arr.cend() - 1);
    QCOMPARE(it, arr.cend());
    QCOMPARE(arr, QVarLengthArray<QString>({ "val0" }));
}

void tst_QVarLengthArray::copesWithCopyabilityOfMoveOnlyVector()
{
    // std::vector<move-only-type> is_copyable
    // (https://quuxplusone.github.io/blog/2020/02/05/vector-is-copyable-except-when-its-not/)

    QVarLengthArray<std::vector<std::unique_ptr<int>>, 2> vla;
    vla.emplace_back(42);
    vla.emplace_back(43);
    vla.emplace_back(44); // goes to the heap
    QCOMPARE_EQ(vla.size(), 3);
    QCOMPARE_EQ(vla.front().size(), 42U);
    QCOMPARE_EQ(vla.front().front(), nullptr);
    QCOMPARE_EQ(vla.back().size(), 44U);

    auto moved = std::move(vla);
    QCOMPARE_EQ(moved.size(), 3);
    QCOMPARE_EQ(moved.front().size(), 42U);
    QCOMPARE_EQ(moved.front().front(), nullptr);
    QCOMPARE_EQ(moved.back().size(), 44U);
}

QTEST_APPLESS_MAIN(tst_QVarLengthArray)
#include "tst_qvarlengtharray.moc"
