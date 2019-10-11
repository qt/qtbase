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
#include <qvarlengtharray.h>
#include <qvariant.h>

#include <memory>

class tst_QVarLengthArray : public QObject
{
    Q_OBJECT
private slots:
    void append();
    void removeLast();
    void oldTests();
    void appendCausingRealloc();
    void resize();
    void realloc();
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

private:
    template<typename T>
    void initializeList();
};

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
}

void tst_QVarLengthArray::resize()
{
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
    char i;
};

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(MyPrimitive, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(MyMovable, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(MyComplex, Q_COMPLEX_TYPE);

QT_END_NAMESPACE

bool reallocTestProceed = true;

template <class T, int PreAlloc>
int countMoved(QVarLengthArray<T, PreAlloc> const &c)
{
    int result = 0;
    for (int i = 0; i < c.size(); ++i)
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
        isStatic = QTypeInfo<T>::isStatic,
        isComplex = QTypeInfo<T>::isComplex,

        isPrimitive = !isComplex && !isStatic,
        isMovable = !isStatic
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

void tst_QVarLengthArray::reverseIterators()
{
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
        QCOMPARE(list.length(), 0);
        QCOMPARE(list.count(), 0);
        QCOMPARE(list.size(), 0);
    }

    {
        QVarLengthArray<int> list;
        list.append(0);
        QCOMPARE(list.length(), 1);
        QCOMPARE(list.count(), 1);
        QCOMPARE(list.size(), 1);
    }

    {
        QVarLengthArray<int> list;
        list.append(0);
        list.append(1);
        QCOMPARE(list.length(), 2);
        QCOMPARE(list.count(), 2);
        QCOMPARE(list.size(), 2);
    }

    {
        QVarLengthArray<int> list;
        list.append(0);
        list.append(0);
        list.append(0);
        QCOMPARE(list.length(), 3);
        QCOMPARE(list.count(), 3);
        QCOMPARE(list.size(), 3);
    }

    // test removals too
    {
        QVarLengthArray<int> list;
        list.append(0);
        list.append(0);
        list.append(0);
        QCOMPARE(list.length(), 3);
        QCOMPARE(list.count(), 3);
        QCOMPARE(list.size(), 3);
        list.removeLast();
        QCOMPARE(list.length(), 2);
        QCOMPARE(list.count(), 2);
        QCOMPARE(list.size(), 2);
        list.removeLast();
        QCOMPARE(list.length(), 1);
        QCOMPARE(list.count(), 1);
        QCOMPARE(list.size(), 1);
        list.removeLast();
        QCOMPARE(list.length(), 0);
        QCOMPARE(list.count(), 0);
        QCOMPARE(list.size(), 0);
    }
}

void tst_QVarLengthArray::cpp17ctad()
{
#ifdef __cpp_deduction_guides
#define QVERIFY_IS_VLA_OF(obj, Type) \
    QVERIFY2((std::is_same<decltype(obj), QVarLengthArray<Type>>::value), \
             QMetaType::typeName(qMetaTypeId<decltype(obj)::value_type>()))
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
#else
    QSKIP("This test requires C++17 Constructor Template Argument Deduction support enabled in the compiler.");
#endif

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
    QCOMPARE(list.length(), 3);

    // remove some, make sure it stays sane
    list.removeLast();
    QCOMPARE(list.first(), 27);
    QCOMPARE(list.length(), 2);

    list.removeLast();
    QCOMPARE(list.first(), 27);
    QCOMPARE(list.length(), 1);
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
    QCOMPARE(list.length(), 3);

    // remove some, make sure it stays sane
    list.removeLast();
    QCOMPARE(list.last(), 4);
    QCOMPARE(list.length(), 2);

    list.removeLast();
    QCOMPARE(list.last(), 27);
    QCOMPARE(list.length(), 1);
}

void tst_QVarLengthArray::squeeze()
{
    QVarLengthArray<int> list;
    int sizeOnStack = list.capacity();
    int sizeOnHeap = sizeOnStack * 2;
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
    QVarLengthArray<QString> myvla;
    myvla << "A" << "B" << "C";
    QVarLengthArray<QString> myvlatwo;
    myvlatwo << "D" << "E" << "F";
    QVarLengthArray<QString> combined;
    combined << "A" << "B" << "C" << "D" << "E" << "F";

    // !=
    QVERIFY(myvla != myvlatwo);

    // +=: not provided, emulate
    //myvla += myvlatwo;
    Q_FOREACH (const QString &s, myvlatwo)
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

        vec.prepend(std::move(m1));
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
    vec.prepend(std::move(val1));
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

QTEST_APPLESS_MAIN(tst_QVarLengthArray)
#include "tst_qvarlengtharray.moc"
