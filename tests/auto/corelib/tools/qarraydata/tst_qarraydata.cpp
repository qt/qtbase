// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QtCore/QString>
#include <QtCore/qarraydata.h>

#include "simplevector.h"

#include <array>
#include <tuple>
#include <algorithm>
#include <vector>
#include <set>
#include <stdexcept>
#include <functional>
#include <memory>

// A wrapper for a test function. Calls a function, if it fails, reports failure
#define RUN_TEST_FUNC(test, ...) \
do { \
    test(__VA_ARGS__); \
    if (QTest::currentTestFailed()) \
        QFAIL("Test case " #test "(" #__VA_ARGS__ ") failed"); \
} while (false)

class tst_QArrayData : public QObject
{
    Q_OBJECT

private slots:
    void referenceCounting();
    void simpleVector();
    void simpleVectorReserve_data();
    void simpleVectorReserve();
    void allocate_data();
    void allocate();
    void reallocate_data() { allocate_data(); }
    void reallocate();
    void alignment_data();
    void alignment();
    void typedData();
    void gccBug43247();
    void arrayOps_data();
    void arrayOps();
    void arrayOps2_data();
    void arrayOps2();
    void arrayOpsExtra_data();
    void arrayOpsExtra();
    void fromRawData_data();
    void fromRawData();
    void literals();
    void variadicLiterals();
    void rValueReferences();
    void grow();
    void freeSpace_data();
    void freeSpace();
    void dataPointerAllocate_data();
    void dataPointerAllocate();
    void selfEmplaceBackwards();
    void selfEmplaceForward();
#ifndef QT_NO_EXCEPTIONS
    void relocateWithExceptions_data();
    void relocateWithExceptions();
#endif // QT_NO_EXCEPTIONS
};

template <class T> const T &const_(const T &t) { return t; }

void tst_QArrayData::referenceCounting()
{
    {
        // Reference counting initialized to 1 (owned)
        QArrayData array = { Q_BASIC_ATOMIC_INITIALIZER(1), {}, 0 };

        QCOMPARE(array.ref_.loadRelaxed(), 1);

        QVERIFY(array.ref());
        QCOMPARE(array.ref_.loadRelaxed(), 2);

        QVERIFY(array.deref());
        QCOMPARE(array.ref_.loadRelaxed(), 1);

        QVERIFY(array.ref());
        QCOMPARE(array.ref_.loadRelaxed(), 2);

        QVERIFY(array.deref());
        QCOMPARE(array.ref_.loadRelaxed(), 1);

        QVERIFY(!array.deref());
        QCOMPARE(array.ref_.loadRelaxed(), 0);

        // Now would be a good time to free/release allocated data
    }
}

void tst_QArrayData::simpleVector()
{
    int data[] = { 0, 1, 2, 3, 4, 5, 6 };
    int array[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    SimpleVector<int> v1;
    SimpleVector<int> v2(v1);
    SimpleVector<int> v3(nullptr, (int *)nullptr, 0);
    SimpleVector<int> v4(nullptr, data, 0);
    SimpleVector<int> v5(nullptr, data, 1);
    SimpleVector<int> v6(nullptr, data, 7);
    SimpleVector<int> v7(10, 5);
    SimpleVector<int> v8(array, array + sizeof(array)/sizeof(*array));

    v3 = v1;
    v1.swap(v3);
    v4.clear();

    QVERIFY(v1.isNull());
    QVERIFY(v2.isNull());
    QVERIFY(v3.isNull());
    QVERIFY(v4.isNull());
    QVERIFY(!v5.isNull());
    QVERIFY(!v6.isNull());
    QVERIFY(!v7.isNull());
    QVERIFY(!v8.isNull());

    QVERIFY(v1.isEmpty());
    QVERIFY(v2.isEmpty());
    QVERIFY(v3.isEmpty());
    QVERIFY(v4.isEmpty());
    QVERIFY(!v5.isEmpty());
    QVERIFY(!v6.isEmpty());
    QVERIFY(!v7.isEmpty());
    QVERIFY(!v8.isEmpty());

    QCOMPARE(v1.size(), size_t(0));
    QCOMPARE(v2.size(), size_t(0));
    QCOMPARE(v3.size(), size_t(0));
    QCOMPARE(v4.size(), size_t(0));
    QCOMPARE(v5.size(), size_t(1));
    QCOMPARE(v6.size(), size_t(7));
    QCOMPARE(v7.size(), size_t(10));
    QCOMPARE(v8.size(), size_t(10));

    QCOMPARE(v1.capacity(), size_t(0));
    QCOMPARE(v2.capacity(), size_t(0));
    QCOMPARE(v3.capacity(), size_t(0));
    QCOMPARE(v4.capacity(), size_t(0));
    QCOMPARE(v5.capacity(), size_t(0));
    // v6.capacity() is unspecified, for now
    QVERIFY(v7.capacity() >= size_t(10));
    QVERIFY(v8.capacity() >= size_t(10));

    QVERIFY(v1.isStatic());
    QVERIFY(v2.isStatic());
    QVERIFY(v3.isStatic());
    QVERIFY(v4.isStatic());
    QVERIFY(v5.isStatic());
    QVERIFY(v6.isStatic());
    QVERIFY(!v7.isStatic());
    QVERIFY(!v8.isStatic());

    QVERIFY(v1.isShared());
    QVERIFY(v2.isShared());
    QVERIFY(v3.isShared());
    QVERIFY(v4.isShared());
    QVERIFY(v5.isShared());
    QVERIFY(v6.isShared());
    QVERIFY(!v7.isShared());
    QVERIFY((SimpleVector<int>(v7), v7.isShared()));
    QVERIFY(!v7.isShared());
    QVERIFY(!v8.isShared());

    QVERIFY(v1.isSharedWith(v2));
    QVERIFY(v1.isSharedWith(v3));
    QVERIFY(v1.isSharedWith(v4));
    QVERIFY(!v1.isSharedWith(v5));
    QVERIFY(!v1.isSharedWith(v6));

    QCOMPARE(v1.constBegin(), v1.constEnd());
    QCOMPARE(v4.constBegin(), v4.constEnd());
    QCOMPARE((v6.constBegin() + v6.size()), v6.constEnd());
    QCOMPARE((v7.constBegin() + v7.size()), v7.constEnd());
    QCOMPARE((v8.constBegin() + v8.size()), v8.constEnd());

    QVERIFY(v1 == v2);
    QVERIFY(v1 == v3);
    QVERIFY(v1 == v4);
    QVERIFY(v1 != v5);
    QVERIFY(!(v1 == v6));

    QVERIFY(v1 != v6);
    QVERIFY(v4 != v6);
    QVERIFY(v5 != v6);
    QVERIFY(!(v1 == v5));

    QVERIFY(v1 < v6);
    QVERIFY(!(v6 < v1));
    QVERIFY(v6 > v1);
    QVERIFY(!(v1 > v6));
    QVERIFY(v1 <= v6);
    QVERIFY(!(v6 <= v1));
    QVERIFY(v6 >= v1);
    QVERIFY(!(v1 >= v6));

    {
        SimpleVector<int> temp(v6);

        QCOMPARE(const_(v6).front(), 0);
        QCOMPARE(const_(v6).back(), 6);

        QVERIFY(temp.isShared());
        QVERIFY(temp.isSharedWith(v6));

        QCOMPARE(temp.front(), 0);
        QCOMPARE(temp.back(), 6);

        // Detached
        QVERIFY(!temp.isShared());
        const int *const tempBegin = temp.begin();

        for (size_t i = 0; i < v6.size(); ++i) {
            QCOMPARE(const_(v6)[i], int(i));
            QCOMPARE(const_(v6).at(i), int(i));
            QCOMPARE(&const_(v6)[i], &const_(v6).at(i));

            QCOMPARE(const_(v8)[i], const_(v6)[i]);

            QCOMPARE(temp[i], int(i));
            QCOMPARE(temp.at(i), int(i));
            QCOMPARE(&temp[i], &temp.at(i));
        }

        // A single detach should do
        QCOMPARE((const int *)temp.begin(), tempBegin);
    }

    {
        int count = 0;
        Q_FOREACH (int value, v7) {
            QCOMPARE(value, 5);
            ++count;
        }

        QCOMPARE(count, 10);
    }

    {
        int count = 0;
        Q_FOREACH (int value, v8) {
            QCOMPARE(value, count);
            ++count;
        }

        QCOMPARE(count, 10);
    }

    v5 = v6;
    QVERIFY(v5.isSharedWith(v6));
    QVERIFY(!v1.isSharedWith(v5));

    v1.swap(v6);
    QVERIFY(v6.isNull());
    QVERIFY(v1.isSharedWith(v5));

    {
        using std::swap;
        swap(v1, v6);
        QVERIFY(v5.isSharedWith(v6));
        QVERIFY(!v1.isSharedWith(v5));
    }

    v1.prepend(array, array + sizeof(array)/sizeof(array[0]));
    QCOMPARE(v1.size(), size_t(10));
    QVERIFY(v1 == v8);

    v6 = v1;
    QVERIFY(v1.isSharedWith(v6));

    v1.prepend(array, array + sizeof(array)/sizeof(array[0]));
    QVERIFY(!v1.isSharedWith(v6));
    QCOMPARE(v1.size(), size_t(20));
    QCOMPARE(v6.size(), size_t(10));

    for (int i = 0; i < 20; ++i)
        QCOMPARE(v1[i], v6[i % 10]);

    v1.clear();

    v1.append(array, array + sizeof(array)/sizeof(array[0]));
    // v1 is now [0 .. 9]
    QCOMPARE(v1.size(), size_t(10));
    QVERIFY(v1 == v8);

    v6 = v1;
    QVERIFY(v1.isSharedWith(v6));

    v1.append(array, array + sizeof(array)/sizeof(array[0]));
    // v1 is now [0 .. 9, 0 .. 9]
    QVERIFY(!v1.isSharedWith(v6));
    QCOMPARE(v1.size(), size_t(20));
    QCOMPARE(v6.size(), size_t(10));

    for (int i = 0; i < 20; ++i)
        QCOMPARE(v1[i], v6[i % 10]);

    v1.insert(0, v6.constBegin(), v6.constEnd());
    // v1 is now [ 0 .. 9, 0 .. 9, 0 .. 9]
    QCOMPARE(v1.size(), size_t(30));

    for (int i = 0; i < 30; ++i)
        QCOMPARE(v1[i], v8[i % 10]);

    v6 = v1;
    QVERIFY(v1.isSharedWith(v6));

    v1.insert(10, v6.constBegin(), v6.constEnd());
    // v1 is now [ 0..9, <new data>0..9, 0..9, 0..9</new data>, 0..9, 0..9 ]
    QVERIFY(!v1.isSharedWith(v6));
    QCOMPARE(v1.size(), size_t(60));
    QCOMPARE(v6.size(), size_t(30));

    for (int i = 0; i < 30; ++i)
        QCOMPARE(v6[i], v8[i % 10]);
    for (int i = 0; i < 60; ++i)
        QCOMPARE(v1[i], v8[i % 10]);

    v1.insert(int(v1.size()), v6.constBegin(), v6.constEnd());
    // v1 is now [ 0..9 x 6, <new data>0..9 x 3</new data> ]
    QCOMPARE(v1.size(), size_t(90));

    for (int i = 0; i < 90; ++i)
        QCOMPARE(v1[i], v8[i % 10]);

    v1.insert(-1, v8.constBegin(), v8.constEnd());
    // v1 is now [ 0..9 x 9, <new data>0..9</new data> ]
    QCOMPARE(v1.size(), size_t(100));

    for (int i = 0; i < 100; ++i)
        QCOMPARE(v1[i], v8[i % 10]);

    v1.insert(-11, v8.constBegin(), v8.constEnd());
    // v1 is now [ 0..9 x 9, <new data>0..9</new data>, 0..9 ]
    QCOMPARE(v1.size(), size_t(110));

    for (int i = 0; i < 110; ++i)
        QCOMPARE(v1[i], v8[i % 10]);

    v1.insert(-200, v8.constBegin(), v8.constEnd());
    // v1 is now [ <new data>0..9</new data>, 0..9 x 11 ]
    QCOMPARE(v1.size(), size_t(120));

    for (int i = 0; i < 120; ++i)
        QCOMPARE(v1[i], v8[i % 10]);
}

Q_DECLARE_METATYPE(SimpleVector<int>)

void tst_QArrayData::simpleVectorReserve_data()
{
    QTest::addColumn<SimpleVector<int> >("vector");
    QTest::addColumn<size_t>("capacity");
    QTest::addColumn<size_t>("size");

    QTest::newRow("null") << SimpleVector<int>() << size_t(0) << size_t(0);
    QTest::newRow("empty") << SimpleVector<int>(0, 42) << size_t(0) << size_t(0);
    QTest::newRow("non-empty") << SimpleVector<int>(5, 42) << size_t(5) << size_t(5);

    static const int array[] =
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    QTest::newRow("raw-data") << SimpleVector<int>::fromRawData(array, 15) << size_t(0) << size_t(15);
}

void tst_QArrayData::simpleVectorReserve()
{
    QFETCH(SimpleVector<int>, vector);
    QFETCH(size_t, capacity);
    QFETCH(size_t, size);

    QVERIFY(!capacity || capacity >= size);

    QCOMPARE(vector.capacity(), capacity);
    QCOMPARE(vector.size(), size);

    const SimpleVector<int> copy(vector);

    vector.reserve(0);
    QCOMPARE(vector.capacity(), capacity);
    QCOMPARE(vector.size(), size);

    vector.reserve(10);

    // zero-capacity (immutable) resets with detach
    if (!capacity)
        capacity = size;

    QCOMPARE(vector.capacity(), qMax(size_t(10), capacity));
    QCOMPARE(vector.size(), size);

    vector.reserve(20);
    QCOMPARE(vector.capacity(), size_t(20));
    QCOMPARE(vector.size(), size);

    vector.reserve(30);
    QCOMPARE(vector.capacity(), size_t(30));
    QCOMPARE(vector.size(), size);

    QVERIFY(vector == copy);
}

struct Deallocator
{
    Deallocator(size_t objectSize, size_t alignment)
        : objectSize(objectSize)
        , alignment(alignment)
    {
    }

    ~Deallocator()
    {
        Q_FOREACH (QArrayData *data, headers)
            QArrayData::deallocate(data, objectSize, alignment);
    }

    size_t objectSize;
    size_t alignment;
    QList<QArrayData *> headers;
};

Q_DECLARE_METATYPE(const QArrayData *)
Q_DECLARE_METATYPE(QArrayData::ArrayOptions)

void tst_QArrayData::allocate_data()
{
    QTest::addColumn<size_t>("objectSize");
    QTest::addColumn<size_t>("alignment");
    QTest::addColumn<bool>("grow");

    struct {
        char const *typeName;
        size_t objectSize;
        size_t alignment;
    } types[] = {
        { "char", sizeof(char), alignof(char) },
        { "short", sizeof(short), alignof(short) },
        { "void *", sizeof(void *), alignof(void *) }
    };

    struct {
        char const *description;
        bool grow;
    } options[] = {
        { "Default", false },
        { "Grow", true }
    };

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i)
        for (size_t j = 0; j < sizeof(options)/sizeof(options[0]); ++j)
            QTest::newRow(qPrintable(
                        QLatin1String(types[i].typeName)
                        + QLatin1String(": ")
                        + QLatin1String(options[j].description)))
                << types[i].objectSize << types[i].alignment
                << options[j].grow;
}

void tst_QArrayData::allocate()
{
    QFETCH(size_t, objectSize);
    QFETCH(size_t, alignment);
    QFETCH(bool, grow);

    // Minimum alignment that can be requested is that of QArrayData.
    // Typically, this alignment is sizeof(void *) and ensured by malloc.
    size_t minAlignment = qMax(alignment, alignof(QArrayData));

    Deallocator keeper(objectSize, minAlignment);
    keeper.headers.reserve(1024);

    for (qsizetype capacity = 1; capacity <= 1024; capacity <<= 1) {
        QArrayData *data;
        void *dataPointer = QArrayData::allocate(&data, objectSize, minAlignment, capacity, grow ? QArrayData::Grow : QArrayData::KeepSize);

        keeper.headers.append(data);

        if (grow)
            QCOMPARE_GE(data->allocatedCapacity(), capacity);
        else
            QCOMPARE(data->allocatedCapacity(), capacity);

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(dataPointer, 'A', objectSize * capacity);
    }
}

void tst_QArrayData::reallocate()
{
    QFETCH(size_t, objectSize);
    QFETCH(size_t, alignment);
    QFETCH(bool, grow);

    // Minimum alignment that can be requested is that of QArrayData.
    // Typically, this alignment is sizeof(void *) and ensured by malloc.
    size_t minAlignment = qMax(alignment, alignof(QArrayData));

    int capacity = 10;
    Deallocator keeper(objectSize, minAlignment);
    QArrayData *data;
    void *dataPointer = QArrayData::allocate(&data, objectSize, minAlignment, capacity, grow ? QArrayData::Grow : QArrayData::KeepSize);
    keeper.headers.append(data);

    memset(dataPointer, 'A', objectSize * capacity);

    // now try to reallocate
    int newCapacity = 40;
    auto pair = QArrayData::reallocateUnaligned(data, dataPointer, objectSize, newCapacity, grow ? QArrayData::Grow : QArrayData::KeepSize);
    data = pair.first;
    dataPointer = pair.second;
    QVERIFY(data);
    keeper.headers.clear();
    keeper.headers.append(data);

    if (grow)
        QVERIFY(data->allocatedCapacity() > newCapacity);
    else
        QCOMPARE(data->allocatedCapacity(), newCapacity);

    for (int i = 0; i < capacity; ++i)
        QCOMPARE(static_cast<char *>(dataPointer)[i], 'A');
}

class Unaligned
{
    Q_DECL_UNUSED_MEMBER char dummy[8];
};

void tst_QArrayData::alignment_data()
{
    QTest::addColumn<size_t>("alignment");

    for (size_t i = 1; i < 10; ++i) {
        size_t alignment = size_t(1u) << i;
        QTest::newRow(qPrintable(QString::number(alignment))) << alignment;
    }
}

void tst_QArrayData::alignment()
{
    QFETCH(size_t, alignment);

    // Minimum alignment that can be requested is that of QArrayData.
    // Typically, this alignment is sizeof(void *) and ensured by malloc.
    size_t minAlignment = qMax(alignment, alignof(QArrayData));

    Deallocator keeper(sizeof(Unaligned), minAlignment);
    keeper.headers.reserve(100);

    for (int i = 0; i < 100; ++i) {
        QArrayData *data;
        void *dataPointer = QArrayData::allocate(&data, sizeof(Unaligned), minAlignment, 8, QArrayData::KeepSize);
        keeper.headers.append(data);

        QVERIFY(data);
        QVERIFY(data->allocatedCapacity() >= uint(8));

        // These conditions should hold as long as header and array are
        // allocated together
        qptrdiff offset = reinterpret_cast<char *>(dataPointer) -
                reinterpret_cast<char *>(data);
        QVERIFY(offset >= qptrdiff(sizeof(QArrayData)));
        QVERIFY(offset <= qptrdiff(sizeof(QArrayData)
                    + minAlignment - alignof(QArrayData)));

        // Data is aligned
        QCOMPARE(quintptr(quintptr(dataPointer) % alignment), quintptr(0u));

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(dataPointer, 'A', sizeof(Unaligned) * 8);
    }
}

void tst_QArrayData::typedData()
{
    {
        Deallocator keeper(sizeof(char),
                alignof(QTypedArrayData<char>::AlignmentDummy));
        QPair<QTypedArrayData<char> *, char *> pair = QTypedArrayData<char>::allocate(10);
        QArrayData *array = pair.first;
        keeper.headers.append(array);

        QVERIFY(array);
        QCOMPARE(array->allocatedCapacity(), qsizetype(10));

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(pair.second, 0, 10 * sizeof(char));

        keeper.headers.clear();
        QTypedArrayData<short>::deallocate(array);

        QVERIFY(true);
    }

    {
        Deallocator keeper(sizeof(short),
                alignof(QTypedArrayData<short>::AlignmentDummy));
        QPair<QTypedArrayData<short> *, short *> pair = QTypedArrayData<short>::allocate(10);
        QArrayData *array = pair.first;
        keeper.headers.append(array);

        QVERIFY(array);
        QCOMPARE(array->allocatedCapacity(), qsizetype(10));

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(pair.second, 0, 10 * sizeof(short));

        keeper.headers.clear();
        QTypedArrayData<short>::deallocate(array);

        QVERIFY(true);
    }

    {
        Deallocator keeper(sizeof(double),
                alignof(QTypedArrayData<double>::AlignmentDummy));
        QPair<QTypedArrayData<double> *, double *> pair = QTypedArrayData<double>::allocate(10);
        QArrayData *array = pair.first;
        keeper.headers.append(array);

        QVERIFY(array);
        QCOMPARE(array->allocatedCapacity(), qsizetype(10));

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(pair.second, 0, 10 * sizeof(double));

        keeper.headers.clear();
        QTypedArrayData<double>::deallocate(array);

        QVERIFY(true);
    }
}

void tst_QArrayData::gccBug43247()
{
    // This test tries to verify QArrayData is not affected by GCC optimizer
    // bug #43247.
    // Reported on GCC 4.4.3, Linux, affects QList

    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (3)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (4)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (5)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (6)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (7)");

    SimpleVector<int> array(10, 0);
    // QList<int> list(10, 0);

    for (int i = 0; i < 10; ++i) {
        if (i >= 3 && i < 8)
            qDebug("GCC Optimization bug #43247 not triggered (%i)", i);

        // When access to data is implemented through an array of size 1, this
        // line lets the compiler assume i == 0, and the conditional above is
        // skipped.
        QVERIFY(array.at(i) == 0);
        // QVERIFY(list.at(i) == 0);
    }
}

struct CountedObject
{
    CountedObject()
        : id(liveCount++)
        , flags(DefaultConstructed)
    {
    }

    CountedObject(const CountedObject &other)
        : id(other.id)
        , flags(other.flags == DefaultConstructed
                ? ObjectFlags(CopyConstructed | DefaultConstructed)
                : CopyConstructed)
    {
        ++liveCount;
    }

    ~CountedObject()
    {
        --liveCount;
    }

    CountedObject &operator=(const CountedObject &other)
    {
        flags = ObjectFlags(other.flags | CopyAssigned);
        id = other.id;
        return *this;
    }

    struct LeakChecker
    {
        LeakChecker()
            : previousLiveCount(liveCount)
        {
        }

        ~LeakChecker()
        {
            QCOMPARE(liveCount, previousLiveCount);
        }

    private:
        const size_t previousLiveCount;
    };

    enum ObjectFlags {
        DefaultConstructed  = 1,
        CopyConstructed     = 2,
        CopyAssigned        = 4
    };

    size_t id; // not unique
    ObjectFlags flags;

    static size_t liveCount;
};

bool operator==(const CountedObject &lhs, const CountedObject &rhs)
{
    return lhs.id == rhs.id;  // TODO: anything better than this?
}

size_t CountedObject::liveCount = 0;

void tst_QArrayData::arrayOps_data()
{
    QTest::addColumn<bool>("capacityReserved");

    QTest::newRow("default") << false;
    QTest::newRow("capacity-reserved") << true;
}

void tst_QArrayData::arrayOps()
{
    QFETCH(bool, capacityReserved);
    CountedObject::LeakChecker leakChecker; Q_UNUSED(leakChecker);

    const int intArray[5] = { 80, 101, 100, 114, 111 };
    const QString stringArray[5] = {
        QLatin1String("just"),
        QLatin1String("for"),
        QLatin1String("testing"),
        QLatin1String("a"),
        QLatin1String("vector")
    };
    const CountedObject objArray[5];

    static_assert(!QTypeInfo<int>::isComplex);
    static_assert(QTypeInfo<int>::isRelocatable);
    static_assert(QTypeInfo<QString>::isComplex);
    static_assert(QTypeInfo<QString>::isRelocatable);
    static_assert(QTypeInfo<CountedObject>::isComplex);
    static_assert(!QTypeInfo<CountedObject>::isRelocatable);

    QCOMPARE(CountedObject::liveCount, size_t(5));
    for (size_t i = 0; i < 5; ++i)
        QCOMPARE(objArray[i].id, i);

    ////////////////////////////////////////////////////////////////////////////
    // copyAppend (I)
    SimpleVector<int> vi(intArray, intArray + 5, capacityReserved);
    SimpleVector<QString> vs(stringArray, stringArray + 5, capacityReserved);
    SimpleVector<CountedObject> vo(objArray, objArray + 5, capacityReserved);

    QCOMPARE(CountedObject::liveCount, size_t(10));
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(vi[i], intArray[i]);
        QVERIFY(vs[i].isSharedWith(stringArray[i]));

        QCOMPARE(vo[i].id, objArray[i].id);
        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
                | CountedObject::DefaultConstructed);
    }

    ////////////////////////////////////////////////////////////////////////////
    // destroyAll
    vi.clear();
    vs.clear();
    vo.clear();

    QCOMPARE(CountedObject::liveCount, size_t(5));

    ////////////////////////////////////////////////////////////////////////////
    // copyAppend (II)
    int referenceInt = 7;
    QString referenceString = QLatin1String("reference");
    CountedObject referenceObject;

    vi = SimpleVector<int>(5, referenceInt, capacityReserved);
    vs = SimpleVector<QString>(5, referenceString, capacityReserved);
    vo = SimpleVector<CountedObject>(5, referenceObject, capacityReserved);

    QCOMPARE(vi.size(), size_t(5));
    QCOMPARE(vs.size(), size_t(5));
    QCOMPARE(vo.size(), size_t(5));

    QCOMPARE(CountedObject::liveCount, size_t(11));
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(vi[i], referenceInt);
        QVERIFY(vs[i].isSharedWith(referenceString));

        QCOMPARE(vo[i].id, referenceObject.id);

        // A temporary object is created as DefaultConstructed |
        // CopyConstructed, then it is used instead of the original value to
        // construct elements in the container which are CopyConstructed only
        //QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed);
    }

    ////////////////////////////////////////////////////////////////////////////
    // insert
    vi.reserve(30);
    vs.reserve(30);
    vo.reserve(30);

    QCOMPARE(vi.size(), size_t(5));
    QCOMPARE(vs.size(), size_t(5));
    QCOMPARE(vo.size(), size_t(5));

    QVERIFY(vi.capacity() >= 30);
    QVERIFY(vs.capacity() >= 30);
    QVERIFY(vo.capacity() >= 30);

    // Displace as many elements as array is extended by
    vi.insert(0, intArray, intArray + 5);
    vs.insert(0, stringArray, stringArray + 5);
    vo.insert(0, objArray, objArray + 5);

    QCOMPARE(vi.size(), size_t(10));
    QCOMPARE(vs.size(), size_t(10));
    QCOMPARE(vo.size(), size_t(10));

    // Displace more elements than array is extended by
    vi.insert(0, intArray, intArray + 5);
    vs.insert(0, stringArray, stringArray + 5);
    vo.insert(0, objArray, objArray + 5);

    QCOMPARE(vi.size(), size_t(15));
    QCOMPARE(vs.size(), size_t(15));
    QCOMPARE(vo.size(), size_t(15));

    // Displace less elements than array is extended by
    vi.insert(5, vi.constBegin(), vi.constEnd());
    vs.insert(5, vs.constBegin(), vs.constEnd());
    vo.insert(5, vo.constBegin(), vo.constEnd());

    QCOMPARE(vi.size(), size_t(30));
    QCOMPARE(vs.size(), size_t(30));
    QCOMPARE(vo.size(), size_t(30));

    QCOMPARE(CountedObject::liveCount, size_t(36));
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(vi[i], intArray[i % 5]);
        QVERIFY(vs[i].isSharedWith(stringArray[i % 5]));

        QCOMPARE(vo[i].id, objArray[i % 5].id);

        // Insertion at begin (prepend) caused the elements to move, meaning
        // that instead of being displaced, newly added elements got constructed
        // in uninitialized memory with DefaultConstructed | CopyConstructed
        // ### QArrayData::insert does copy assign some of the values, so this test doesn't
        // work
//        QCOMPARE(int(vo[i].flags), CountedObject::DefaultConstructed
//                | CountedObject::CopyConstructed);
    }

    for (int i = 5; i < 15; ++i) {
        QCOMPARE(vi[i], intArray[i % 5]);
        QVERIFY(vs[i].isSharedWith(stringArray[i % 5]));

        QCOMPARE(vo[i].id, objArray[i % 5].id);
//        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
//                | CountedObject::CopyAssigned);
    }

    for (int i = 15; i < 20; ++i) {
        QCOMPARE(vi[i], referenceInt);
        QVERIFY(vs[i].isSharedWith(referenceString));

        QCOMPARE(vo[i].id, referenceObject.id);
//        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
//                | CountedObject::CopyAssigned);
    }

    for (int i = 20; i < 25; ++i) {
        QCOMPARE(vi[i], intArray[i % 5]);
        QVERIFY(vs[i].isSharedWith(stringArray[i % 5]));

        QCOMPARE(vo[i].id, objArray[i % 5].id);

        //  Originally inserted as (DefaultConstructed | CopyAssigned), later
        //  get shuffled into place by std::rotate (SimpleVector::insert,
        //  overlapping mode).
        //  Depending on implementation of rotate, final assignment can be:
        //     - straight from source: DefaultConstructed | CopyAssigned
        //     - through a temporary: CopyConstructed | CopyAssigned
//        QCOMPARE(vo[i].flags & CountedObject::CopyAssigned,
//                int(CountedObject::CopyAssigned));
    }

    for (int i = 25; i < 30; ++i) {
        QCOMPARE(vi[i], referenceInt);
        QVERIFY(vs[i].isSharedWith(referenceString));

        QCOMPARE(vo[i].id, referenceObject.id);
//        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
//                | CountedObject::CopyAssigned);
    }
}

void tst_QArrayData::arrayOps2_data()
{
    arrayOps_data();
}

void tst_QArrayData::arrayOps2()
{
    QFETCH(bool, capacityReserved);
    CountedObject::LeakChecker leakChecker; Q_UNUSED(leakChecker);

    ////////////////////////////////////////////////////////////////////////////
    // appendInitialize
    SimpleVector<int> vi(5, capacityReserved);
    SimpleVector<QString> vs(5, capacityReserved);
    SimpleVector<CountedObject> vo(5, capacityReserved);

    QCOMPARE(vi.size(), size_t(5));
    QCOMPARE(vs.size(), size_t(5));
    QCOMPARE(vo.size(), size_t(5));

    QCOMPARE(CountedObject::liveCount, size_t(5));
    for (size_t i = 0; i < 5; ++i) {
        QCOMPARE(vi[i], 0);
        QVERIFY(vs[i].isNull());

        QCOMPARE(vo[i].id, i);
        QCOMPARE(int(vo[i].flags), int(CountedObject::DefaultConstructed));
    }

    ////////////////////////////////////////////////////////////////////////////
    // appendInitialize, again

    // These will detach
    vi.resize(10);
    vs.resize(10);
    vo.resize(10);

    QCOMPARE(vi.size(), size_t(10));
    QCOMPARE(vs.size(), size_t(10));
    QCOMPARE(vo.size(), size_t(10));

    QCOMPARE(CountedObject::liveCount, size_t(10));
    for (size_t i = 0; i < 5; ++i) {
        QCOMPARE(vi[i], 0);
        QVERIFY(vs[i].isNull());

        QCOMPARE(vo[i].id, i);
        QCOMPARE(int(vo[i].flags), CountedObject::DefaultConstructed
                | CountedObject::CopyConstructed);
    }

    for (size_t i = 5; i < 10; ++i) {
        QCOMPARE(vi[i], 0);
        QVERIFY(vs[i].isNull());

        QCOMPARE(vo[i].id, i + 5);
        QCOMPARE(int(vo[i].flags), int(CountedObject::DefaultConstructed));
    }

    ////////////////////////////////////////////////////////////////////////////
    // truncate
    QVERIFY(!vi.isShared());
    QVERIFY(!vs.isShared());
    QVERIFY(!vo.isShared());

    // These shouldn't detach
    vi.resize(7);
    vs.resize(7);
    vo.resize(7);

    QCOMPARE(vi.size(), size_t(7));
    QCOMPARE(vs.size(), size_t(7));
    QCOMPARE(vo.size(), size_t(7));

    QCOMPARE(CountedObject::liveCount, size_t(7));
    for (size_t i = 0; i < 5; ++i) {
        QCOMPARE(vi[i], 0);
        QVERIFY(vs[i].isNull());

        QCOMPARE(vo[i].id, i);
        QCOMPARE(int(vo[i].flags), CountedObject::DefaultConstructed
                | CountedObject::CopyConstructed);
    }

    for (size_t i = 5; i < 7; ++i) {
        QCOMPARE(vi[i], 0);
        QVERIFY(vs[i].isNull());

        QCOMPARE(vo[i].id, i + 5);
        QCOMPARE(int(vo[i].flags), int(CountedObject::DefaultConstructed));
    }

    ////////////////////////////////////////////////////////////////////////////
    vi.resize(10);
    vs.resize(10);
    vo.resize(10);

    for (size_t i = 7; i < 10; ++i) {
        vi[i] = int(i);
        vs[i] = QString::number(i);

        QCOMPARE(vo[i].id, i);
        QCOMPARE(int(vo[i].flags), int(CountedObject::DefaultConstructed));
    }

    QCOMPARE(CountedObject::liveCount, size_t(10));

    ////////////////////////////////////////////////////////////////////////////
    // erase
    vi.erase(vi.begin() + 2, vi.begin() + 5);
    vs.erase(vs.begin() + 2, vs.begin() + 5);
    vo.erase(vo.begin() + 2, vo.begin() + 5);

    QCOMPARE(vi.size(), size_t(7));
    QCOMPARE(vs.size(), size_t(7));
    QCOMPARE(vo.size(), size_t(7));

    QCOMPARE(CountedObject::liveCount, size_t(7));
    for (size_t i = 0; i < 2; ++i) {
        QCOMPARE(vi[i], 0);
        QVERIFY(vs[i].isNull());

        QCOMPARE(vo[i].id, i);
        // Erasing not from begin always shifts left - consistency with
        // std::vector::erase. Elements before erase position are not affected.
        QCOMPARE(int(vo[i].flags), CountedObject::DefaultConstructed
                | CountedObject::CopyConstructed);
    }

    for (size_t i = 2; i < 4; ++i) {
        QCOMPARE(vi[i], 0);
        QVERIFY(vs[i].isNull());

        QCOMPARE(vo[i].id, i + 8);
        QCOMPARE(int(vo[i].flags), int(CountedObject::DefaultConstructed)
                | CountedObject::CopyAssigned);
    }

    for (size_t i = 4; i < 7; ++i) {
        QCOMPARE(vi[i], int(i + 3));
        QCOMPARE(vs[i], QString::number(i + 3));

        QCOMPARE(vo[i].id, i + 3);
        QCOMPARE(int(vo[i].flags), CountedObject::DefaultConstructed
                | CountedObject::CopyAssigned);
    }
}

void tst_QArrayData::arrayOpsExtra_data()
{
    dataPointerAllocate_data();
}

void tst_QArrayData::arrayOpsExtra()
{
    QSKIP("Skipped while changing QArrayData operations.", SkipAll);
    QFETCH(QArrayData::GrowthPosition, GrowthPosition);
    CountedObject::LeakChecker leakChecker; Q_UNUSED(leakChecker);

    constexpr size_t inputSize = 5;
    const std::array<int, inputSize> intArray = { 80, 101, 100, 114, 111 };
    const std::array<QString, inputSize> stringArray = {
        QLatin1String("just"), QLatin1String("for"), QLatin1String("testing"), QLatin1String("a"),
        QLatin1String("vector")
    };
    const std::array<CountedObject, inputSize> objArray;

    QVERIFY(!QTypeInfo<int>::isComplex && QTypeInfo<int>::isRelocatable);
    QVERIFY(QTypeInfo<QString>::isComplex && QTypeInfo<QString>::isRelocatable);
    QVERIFY(QTypeInfo<CountedObject>::isComplex && !QTypeInfo<CountedObject>::isRelocatable);

    QCOMPARE(CountedObject::liveCount, inputSize);
    for (size_t i = 0; i < 5; ++i)
        QCOMPARE(objArray[i].id, i);

    const auto setupDataPointers = [&GrowthPosition] (size_t capacity, size_t initialSize = 0) {
        const qsizetype alloc = qsizetype(capacity);
        auto i = QArrayDataPointer<int>::allocateGrow(QArrayDataPointer<int>(), alloc, GrowthPosition);
        auto s = QArrayDataPointer<QString>::allocateGrow(QArrayDataPointer<QString>(), alloc, GrowthPosition);
        auto o = QArrayDataPointer<CountedObject>::allocateGrow(QArrayDataPointer<CountedObject>(), alloc, GrowthPosition);
        if (initialSize) {
            i->appendInitialize(initialSize);
            s->appendInitialize(initialSize);
            o->appendInitialize(initialSize);
        }

        // assign unique values
        std::generate(i.begin(), i.end(), [] () { static int i = 0; return i++; });
        std::generate(s.begin(), s.end(), [] () { static int i = 0; return QString::number(i++); });
        std::generate(o.begin(), o.end(), [] () { return CountedObject(); });
        return std::make_tuple(i, s, o);
    };

    const auto cloneArrayDataPointer = [] (auto &dataPointer, size_t capacity) {
        using ArrayPointer = std::decay_t<decltype(dataPointer)>;
        using Type = std::decay_t<typename ArrayPointer::parameter_type>;
        ArrayPointer copy(QTypedArrayData<Type>::allocate(qsizetype(capacity)));
        copy->copyAppend(dataPointer.begin(), dataPointer.end());
        return copy;
    };

    // Test allocation first
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        auto [intData, strData, objData] = setupDataPointers(inputSize);
        QVERIFY(intData.size == 0);
        QVERIFY(intData.d_ptr() != nullptr);
        QVERIFY(size_t(intData.constAllocatedCapacity()) >= inputSize);
        QVERIFY(intData.data() != nullptr);

        QVERIFY(strData.size == 0);
        QVERIFY(strData.d_ptr() != nullptr);
        QVERIFY(size_t(strData.constAllocatedCapacity()) >= inputSize);
        QVERIFY(strData.data() != nullptr);

        QVERIFY(objData.size == 0);
        QVERIFY(objData.d_ptr() != nullptr);
        QVERIFY(size_t(objData.constAllocatedCapacity()) >= inputSize);
        QVERIFY(objData.data() != nullptr);
    }

    // copyAppend (iterator version)
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testCopyAppend = [&] (auto &dataPointer, auto first, auto last) {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);
            const size_t distance = std::distance(first, last);

            dataPointer->appendIteratorRange(first, last);
            QCOMPARE(size_t(dataPointer.size), originalSize + distance);
            size_t i = 0;
            for (; i < originalSize; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], *(first + (i - originalSize)));
        };

        auto [intData, strData, objData] = setupDataPointers(inputSize * 2, inputSize / 2);
        // empty range
        const std::array<int, 0> emptyIntArray{};
        const std::array<QString, 0> emptyStrArray{};
        const std::array<CountedObject, 0> emptyObjArray{};
        RUN_TEST_FUNC(testCopyAppend, intData, emptyIntArray.begin(), emptyIntArray.end());
        RUN_TEST_FUNC(testCopyAppend, strData, emptyStrArray.begin(), emptyStrArray.end());
        RUN_TEST_FUNC(testCopyAppend, objData, emptyObjArray.begin(), emptyObjArray.end());

        // from arbitrary iterators
        RUN_TEST_FUNC(testCopyAppend, intData, intArray.begin(), intArray.end());
        RUN_TEST_FUNC(testCopyAppend, strData, stringArray.begin(), stringArray.end());
        RUN_TEST_FUNC(testCopyAppend, objData, objArray.begin(), objArray.end());

        // append to full
        const size_t intDataFreeSpace = intData.freeSpaceAtEnd();
//        QVERIFY(intDataFreeSpace > 0);
        const size_t strDataFreeSpace = strData.freeSpaceAtEnd();
//        QVERIFY(strDataFreeSpace > 0);
        const size_t objDataFreeSpace = objData.freeSpaceAtEnd();
//        QVERIFY(objDataFreeSpace > 0);
        const std::vector<int> intVec(intDataFreeSpace, int(55));
        const std::vector<QString> strVec(strDataFreeSpace, QLatin1String("filler"));
        const std::vector<CountedObject> objVec(objDataFreeSpace, CountedObject());
        RUN_TEST_FUNC(testCopyAppend, intData, intVec.begin(), intVec.end());
        RUN_TEST_FUNC(testCopyAppend, strData, strVec.begin(), strVec.end());
        RUN_TEST_FUNC(testCopyAppend, objData, objVec.begin(), objVec.end());
        QCOMPARE(intData.size, intData.constAllocatedCapacity() - intData.freeSpaceAtBegin());
        QCOMPARE(strData.size, strData.constAllocatedCapacity() - strData.freeSpaceAtBegin());
        QCOMPARE(objData.size, objData.constAllocatedCapacity() - objData.freeSpaceAtBegin());
    }

    // copyAppend (iterator version) - special case of copying from self iterators
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testCopyAppendSelf = [&] (auto &dataPointer, auto first, auto last) {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);
            const size_t distance = std::distance(first, last);
            auto firstCopy = copy->begin() + std::distance(dataPointer->begin(), first);

            dataPointer->copyAppend(first, last);
            QCOMPARE(size_t(dataPointer.size), originalSize + distance);
            size_t i = 0;
            for (; i < originalSize; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], *(firstCopy + (i - originalSize)));
        };

        auto [intData, strData, objData] = setupDataPointers(inputSize * 2, inputSize / 2);
        // make no free space at the end
        intData->appendInitialize(intData.size + intData.freeSpaceAtEnd());
        strData->appendInitialize(strData.size + strData.freeSpaceAtEnd());
        objData->appendInitialize(objData.size + objData.freeSpaceAtEnd());

        // make all values unique. this would ensure that we do not have erroneously passed test
        int i = 0;
        std::generate(intData.begin(), intData.end(), [&i] () { return i++; });
        std::generate(strData.begin(), strData.end(), [&i] () { return QString::number(i++); });
        std::generate(objData.begin(), objData.end(), [] () { return CountedObject(); });

        // sanity checks:
        if (GrowthPosition & QArrayData::GrowsAtBeginning) {
            QVERIFY(intData.freeSpaceAtBegin() > 0);
            QVERIFY(strData.freeSpaceAtBegin() > 0);
            QVERIFY(objData.freeSpaceAtBegin() > 0);
        }
        QVERIFY(intData.freeSpaceAtBegin() <= intData.size);
        QVERIFY(strData.freeSpaceAtBegin() <= strData.size);
        QVERIFY(objData.freeSpaceAtBegin() <= objData.size);
        QVERIFY(intData.freeSpaceAtEnd() == 0);
        QVERIFY(strData.freeSpaceAtEnd() == 0);
        QVERIFY(objData.freeSpaceAtEnd() == 0);

        // now, append to full size causing the data to move internally. passed
        // iterators that refer to the object itself must be used correctly
        RUN_TEST_FUNC(testCopyAppendSelf, intData, intData.begin(),
                      intData.begin() + intData.freeSpaceAtBegin());
        RUN_TEST_FUNC(testCopyAppendSelf, strData, strData.begin(),
                      strData.begin() + strData.freeSpaceAtBegin());
        RUN_TEST_FUNC(testCopyAppendSelf, objData, objData.begin(),
                      objData.begin() + objData.freeSpaceAtBegin());
    }

    // copyAppend (value version)
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testCopyAppend = [&] (auto &dataPointer, size_t n, auto value) {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);

            dataPointer->copyAppend(n, value);
            QCOMPARE(size_t(dataPointer.size), originalSize + n);
            size_t i = 0;
            for (; i < originalSize; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], value);
        };

        auto [intData, strData, objData] = setupDataPointers(inputSize * 2, inputSize / 2);
        // no values
        RUN_TEST_FUNC(testCopyAppend, intData, 0, int());
        RUN_TEST_FUNC(testCopyAppend, strData, 0, QString());
        RUN_TEST_FUNC(testCopyAppend, objData, 0, CountedObject());

        // several values
        RUN_TEST_FUNC(testCopyAppend, intData, inputSize, int(5));
        RUN_TEST_FUNC(testCopyAppend, strData, inputSize, QLatin1String("42"));
        RUN_TEST_FUNC(testCopyAppend, objData, inputSize, CountedObject());

        // from self
        RUN_TEST_FUNC(testCopyAppend, intData, 2, intData.data()[3]);
        RUN_TEST_FUNC(testCopyAppend, strData, 2, strData.data()[3]);
        RUN_TEST_FUNC(testCopyAppend, objData, 2, objData.data()[3]);

        // append to full
        const size_t intDataFreeSpace = intData.constAllocatedCapacity() - intData.size;
        QVERIFY(intDataFreeSpace > 0);
        const size_t strDataFreeSpace = strData.constAllocatedCapacity() - strData.size;
        QVERIFY(strDataFreeSpace > 0);
        const size_t objDataFreeSpace = objData.constAllocatedCapacity() - objData.size;
        QVERIFY(objDataFreeSpace > 0);
        RUN_TEST_FUNC(testCopyAppend, intData, intDataFreeSpace, int(-1));
        RUN_TEST_FUNC(testCopyAppend, strData, strDataFreeSpace, QLatin1String("foo"));
        RUN_TEST_FUNC(testCopyAppend, objData, objDataFreeSpace, CountedObject());
        QCOMPARE(intData.size, intData.constAllocatedCapacity());
        QCOMPARE(strData.size, strData.constAllocatedCapacity());
        QCOMPARE(objData.size, objData.constAllocatedCapacity());
    }

    // copyAppend (value version) - special case of copying self value
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testCopyAppendSelf = [&] (auto &dataPointer, size_t n, const auto &value) {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);
            auto valueCopy = value;

            dataPointer->copyAppend(n, value);
            QCOMPARE(size_t(dataPointer.size), originalSize + n);
            size_t i = 0;
            for (; i < originalSize; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], valueCopy);
        };

        auto [intData, strData, objData] = setupDataPointers(inputSize * 2, inputSize / 2);
        // make no free space at the end
        intData->appendInitialize(intData.size + intData.freeSpaceAtEnd());
        strData->appendInitialize(strData.size + strData.freeSpaceAtEnd());
        objData->appendInitialize(objData.size + objData.freeSpaceAtEnd());

        // make all values unique. this would ensure that we do not have erroneously passed test
        int i = 0;
        std::generate(intData.begin(), intData.end(), [&i] () { return i++; });
        std::generate(strData.begin(), strData.end(), [&i] () { return QString::number(i++); });
        std::generate(objData.begin(), objData.end(), [] () { return CountedObject(); });

        // sanity checks:
        if (GrowthPosition & QArrayData::GrowsAtBeginning) {
            QVERIFY(intData.freeSpaceAtBegin() > 0);
            QVERIFY(strData.freeSpaceAtBegin() > 0);
            QVERIFY(objData.freeSpaceAtBegin() > 0);
        }
        QVERIFY(intData.freeSpaceAtEnd() == 0);
        QVERIFY(strData.freeSpaceAtEnd() == 0);
        QVERIFY(objData.freeSpaceAtEnd() == 0);

        // now, append to full size causing the data to move internally. passed
        // value that refers to the object itself must be used correctly
        RUN_TEST_FUNC(testCopyAppendSelf, intData, intData.freeSpaceAtBegin(), intData.data()[0]);
        RUN_TEST_FUNC(testCopyAppendSelf, strData, strData.freeSpaceAtBegin(), strData.data()[0]);
        RUN_TEST_FUNC(testCopyAppendSelf, objData, objData.freeSpaceAtBegin(), objData.data()[0]);
    }

    // moveAppend
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        // now there's only one version that accepts "T*" as input parameters
        const auto testMoveAppend = [&] (auto &dataPointer, const auto &source)
        {
            const size_t originalSize = dataPointer.size;
            const size_t addedSize = std::distance(source.begin(), source.end());
            auto sourceCopy = source;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);

            dataPointer->moveAppend(sourceCopy.data(), sourceCopy.data() + sourceCopy.size());
            QCOMPARE(size_t(dataPointer.size), originalSize + addedSize);
            size_t i = 0;
            for (; i < originalSize; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], source[i - originalSize]);
        };

        auto [intData, strData, objData] = setupDataPointers(inputSize * 2, inputSize / 2);
        // empty range
        RUN_TEST_FUNC(testMoveAppend, intData, std::array<int, 0>{});
        RUN_TEST_FUNC(testMoveAppend, strData, std::array<QString, 0>{});
        RUN_TEST_FUNC(testMoveAppend, objData, std::array<CountedObject, 0>{});

        // non-empty range
        RUN_TEST_FUNC(testMoveAppend, intData, intArray);
        RUN_TEST_FUNC(testMoveAppend, strData, stringArray);
        RUN_TEST_FUNC(testMoveAppend, objData, objArray);

        // append to full
        const size_t intDataFreeSpace = intData.constAllocatedCapacity() - intData.size;
        QVERIFY(intDataFreeSpace > 0);
        const size_t strDataFreeSpace = strData.constAllocatedCapacity() - strData.size;
        QVERIFY(strDataFreeSpace > 0);
        const size_t objDataFreeSpace = objData.constAllocatedCapacity() - objData.size;
        QVERIFY(objDataFreeSpace > 0);
        RUN_TEST_FUNC(testMoveAppend, intData, std::vector<int>(intDataFreeSpace, int(55)));
        RUN_TEST_FUNC(testMoveAppend, strData,
                      std::vector<QString>(strDataFreeSpace, QLatin1String("barbaz")));
        RUN_TEST_FUNC(testMoveAppend, objData,
                      std::vector<CountedObject>(objDataFreeSpace, CountedObject()));
        QCOMPARE(intData.size, intData.constAllocatedCapacity());
        QCOMPARE(strData.size, strData.constAllocatedCapacity());
        QCOMPARE(objData.size, objData.constAllocatedCapacity());
    }

    // moveAppend - special case of moving from self (this is legal yet rather useless)
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testMoveAppendSelf = [&] (auto &dataPointer, auto first, auto last) {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);
            const size_t addedSize = std::distance(first, last);
            const size_t firstPos = std::distance(dataPointer->begin(), first);
            auto firstCopy = copy->begin() + firstPos;

            dataPointer->moveAppend(first, last);
            QCOMPARE(size_t(dataPointer.size), originalSize + addedSize);
            size_t i = 0;
            for (; i < originalSize; ++i) {
                if (i >= firstPos && i < (firstPos + addedSize))  // skip "moved from" chunk
                    continue;
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            }
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], *(firstCopy + (i - originalSize)));
        };

        auto [intData, strData, objData] = setupDataPointers(inputSize * 2, inputSize / 2);
        // make no free space at the end
        intData->appendInitialize(intData.size + intData.freeSpaceAtEnd());
        strData->appendInitialize(strData.size + strData.freeSpaceAtEnd());
        objData->appendInitialize(objData.size + objData.freeSpaceAtEnd());

        // make all values unique. this would ensure that we do not have erroneously passed test
        int i = 0;
        std::generate(intData.begin(), intData.end(), [&i] () { return i++; });
        std::generate(strData.begin(), strData.end(), [&i] () { return QString::number(i++); });
        std::generate(objData.begin(), objData.end(), [] () { return CountedObject(); });

        // sanity checks:
        if (GrowthPosition & QArrayData::GrowsAtBeginning) {
            QVERIFY(intData.freeSpaceAtBegin() > 0);
            QVERIFY(strData.freeSpaceAtBegin() > 0);
            QVERIFY(objData.freeSpaceAtBegin() > 0);
        }
        QVERIFY(intData.freeSpaceAtBegin() <= intData.size);
        QVERIFY(strData.freeSpaceAtBegin() <= strData.size);
        QVERIFY(objData.freeSpaceAtBegin() <= objData.size);
        QVERIFY(intData.freeSpaceAtEnd() == 0);
        QVERIFY(strData.freeSpaceAtEnd() == 0);
        QVERIFY(objData.freeSpaceAtEnd() == 0);

        // now, append to full size causing the data to move internally. passed
        // iterators that refer to the object itself must be used correctly
        RUN_TEST_FUNC(testMoveAppendSelf, intData, intData.begin(),
                      intData.begin() + intData.freeSpaceAtBegin());
        RUN_TEST_FUNC(testMoveAppendSelf, strData, strData.begin(),
                      strData.begin() + strData.freeSpaceAtBegin());
        RUN_TEST_FUNC(testMoveAppendSelf, objData, objData.begin(),
                      objData.begin() + objData.freeSpaceAtBegin());
    }

    // truncate
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testTruncate = [&] (auto &dataPointer, size_t newSize)
        {
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);
            dataPointer->truncate(newSize);
            QCOMPARE(size_t(dataPointer.size), newSize);
            for (size_t i = 0; i < newSize; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
        };

        auto [intData, strData, objData] = setupDataPointers(inputSize, inputSize);
        // truncate one
        RUN_TEST_FUNC(testTruncate, intData, inputSize - 1);
        RUN_TEST_FUNC(testTruncate, strData, inputSize - 1);
        RUN_TEST_FUNC(testTruncate, objData, inputSize - 1);

        // truncate all
        RUN_TEST_FUNC(testTruncate, intData, 0);
        RUN_TEST_FUNC(testTruncate, strData, 0);
        RUN_TEST_FUNC(testTruncate, objData, 0);
    }

    // insert
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testInsertRange = [&] (auto &dataPointer, size_t pos, auto first, auto last)
        {
            const size_t originalSize = dataPointer.size;
            const size_t distance = std::distance(first, last);
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);

            dataPointer->insert(pos, first, last - first);
            QCOMPARE(size_t(dataPointer.size), originalSize + distance);
            size_t i = 0;
            for (; i < pos; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < pos + distance; ++i)
                QCOMPARE(dataPointer.data()[i], *(first + (i - pos)));
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i - distance]);
        };

        const auto testInsertValue = [&] (auto &dataPointer, size_t pos, size_t n, auto value)
        {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);

            dataPointer->insert(pos, n, value);
            QCOMPARE(size_t(dataPointer.size), originalSize + n);
            size_t i = 0;
            for (; i < pos; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < pos + n; ++i)
                QCOMPARE(dataPointer.data()[i], value);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i - n]);
        };

        auto [intData, strData, objData] = setupDataPointers(100, 10);

        // empty ranges
        RUN_TEST_FUNC(testInsertRange, intData, 0, intArray.data(), intArray.data());
        RUN_TEST_FUNC(testInsertRange, strData, 0, stringArray.data(), stringArray.data());
        RUN_TEST_FUNC(testInsertRange, objData, 0, objArray.data(), objArray.data());
        RUN_TEST_FUNC(testInsertValue, intData, 1, 0, int());
        RUN_TEST_FUNC(testInsertValue, strData, 1, 0, QString());
        RUN_TEST_FUNC(testInsertValue, objData, 1, 0, CountedObject());

        // insert at the beginning
        RUN_TEST_FUNC(testInsertRange, intData, 0, intArray.data(), intArray.data() + 1);
        RUN_TEST_FUNC(testInsertRange, strData, 0, stringArray.data(), stringArray.data() + 1);
        RUN_TEST_FUNC(testInsertRange, objData, 0, objArray.data(), objArray.data() + 1);
        RUN_TEST_FUNC(testInsertValue, intData, 0, 1, int(-100));
        RUN_TEST_FUNC(testInsertValue, strData, 0, 1, QLatin1String("12"));
        RUN_TEST_FUNC(testInsertValue, objData, 0, 1, CountedObject());

        // insert into the middle (with the left part of the data being smaller)
        RUN_TEST_FUNC(testInsertRange, intData, 1, intArray.data() + 2, intArray.data() + 4);
        RUN_TEST_FUNC(testInsertRange, strData, 1, stringArray.data() + 2, stringArray.data() + 4);
        RUN_TEST_FUNC(testInsertRange, objData, 1, objArray.data() + 2, objArray.data() + 4);
        RUN_TEST_FUNC(testInsertValue, intData, 2, 2, int(11));
        RUN_TEST_FUNC(testInsertValue, strData, 2, 2, QLatin1String("abcdefxdeadbeef"));
        RUN_TEST_FUNC(testInsertValue, objData, 2, 2, CountedObject());

        // insert into the middle (with the right part of the data being smaller)
        RUN_TEST_FUNC(testInsertRange, intData, intData.size - 1, intArray.data(),
                      intArray.data() + intArray.size());
        RUN_TEST_FUNC(testInsertRange, strData, strData.size - 1, stringArray.data(),
                      stringArray.data() + stringArray.size());
        RUN_TEST_FUNC(testInsertRange, objData, objData.size - 1, objArray.data(),
                      objArray.data() + objArray.size());
        RUN_TEST_FUNC(testInsertValue, intData, intData.size - 3, 3, int(512));
        RUN_TEST_FUNC(testInsertValue, strData, strData.size - 3, 3, QLatin1String("foo"));
        RUN_TEST_FUNC(testInsertValue, objData, objData.size - 3, 3, CountedObject());

        // insert at the end
        RUN_TEST_FUNC(testInsertRange, intData, intData.size, intArray.data(), intArray.data() + 3);
        RUN_TEST_FUNC(testInsertRange, strData, strData.size, stringArray.data(),
                      stringArray.data() + 3);
        RUN_TEST_FUNC(testInsertRange, objData, objData.size, objArray.data(), objArray.data() + 3);
        RUN_TEST_FUNC(testInsertValue, intData, intData.size, 1, int(-42));
        RUN_TEST_FUNC(testInsertValue, strData, strData.size, 1, QLatin1String("hello, world"));
        RUN_TEST_FUNC(testInsertValue, objData, objData.size, 1, CountedObject());
    }

    // insert - special case of inserting from self value. this test only makes
    // sense for prepend - insert at begin.
    {
        const auto testInsertValueSelf = [&] (auto &dataPointer, size_t n, const auto &value) {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);
            auto valueCopy = value;

            dataPointer->insert(0, n, value);
            QCOMPARE(size_t(dataPointer.size), originalSize + n);
            size_t i = 0;
            for (; i < n; ++i)
                QCOMPARE(dataPointer.data()[i], valueCopy);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i - n]);
        };

        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        auto [intData, strData, objData] = setupDataPointers(inputSize * 2, inputSize / 2);

        // make no free space at the begin
        intData->insert(0, intData.freeSpaceAtBegin(), intData.data()[0]);
        strData->insert(0, strData.freeSpaceAtBegin(), strData.data()[0]);
        objData->insert(0, objData.freeSpaceAtBegin(), objData.data()[0]);

        // make all values unique. this would ensure that we do not have erroneously passed test
        int i = 0;
        std::generate(intData.begin(), intData.end(), [&i] () { return i++; });
        std::generate(strData.begin(), strData.end(), [&i] () { return QString::number(i++); });
        std::generate(objData.begin(), objData.end(), [] () { return CountedObject(); });

        // sanity checks:
        QVERIFY(intData.freeSpaceAtEnd() > 0);
        QVERIFY(strData.freeSpaceAtEnd() > 0);
        QVERIFY(objData.freeSpaceAtEnd() > 0);
        QVERIFY(intData.freeSpaceAtBegin() == 0);
        QVERIFY(strData.freeSpaceAtBegin() == 0);
        QVERIFY(objData.freeSpaceAtBegin() == 0);

        // now, prepend to full size causing the data to move internally. passed
        // value that refers to the object itself must be used correctly
        RUN_TEST_FUNC(testInsertValueSelf, intData, intData.freeSpaceAtEnd(),
                      intData.data()[intData.size - 1]);
        RUN_TEST_FUNC(testInsertValueSelf, strData, strData.freeSpaceAtEnd(),
                      strData.data()[strData.size - 1]);
        RUN_TEST_FUNC(testInsertValueSelf, objData, objData.freeSpaceAtEnd(),
                      objData.data()[objData.size - 1]);
    }

    // emplace
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        // testing simple case when emplacing a copy of the same type
        const auto testEmplace = [&] (auto &dataPointer, size_t pos, auto value)
        {
            const size_t originalSize = dataPointer.size;
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);

            dataPointer->emplace(pos, value);
            QCOMPARE(size_t(dataPointer.size), originalSize + 1);
            size_t i = 0;
            for (; i < pos; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            QCOMPARE(dataPointer.data()[i++], value);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i - 1]);
        };

        auto [intData, strData, objData] = setupDataPointers(20, 5);

        // emplace at the beginning
        RUN_TEST_FUNC(testEmplace, intData, 0, int(2));
        RUN_TEST_FUNC(testEmplace, strData, 0, QLatin1String("foo"));
        RUN_TEST_FUNC(testEmplace, objData, 0, CountedObject());
        // emplace into the middle (with the left part of the data being smaller)
        RUN_TEST_FUNC(testEmplace, intData, 1, int(-1));
        RUN_TEST_FUNC(testEmplace, strData, 1, QLatin1String("bar"));
        RUN_TEST_FUNC(testEmplace, objData, 1, CountedObject());
        // emplace into the middle (with the right part of the data being smaller)
        RUN_TEST_FUNC(testEmplace, intData, intData.size - 2, int(42));
        RUN_TEST_FUNC(testEmplace, strData, strData.size - 2, QLatin1String("baz"));
        RUN_TEST_FUNC(testEmplace, objData, objData.size - 2, CountedObject());
        // emplace at the end
        RUN_TEST_FUNC(testEmplace, intData, intData.size, int(123));
        RUN_TEST_FUNC(testEmplace, strData, strData.size, QLatin1String("bak"));
        RUN_TEST_FUNC(testEmplace, objData, objData.size, CountedObject());
    }

    // erase
    {
        CountedObject::LeakChecker localLeakChecker; Q_UNUSED(localLeakChecker);
        const auto testErase = [&] (auto &dataPointer, auto first, auto last)
        {
            const size_t originalSize = dataPointer.size;
            const size_t distance = std::distance(first, last);
            const size_t pos = std::distance(dataPointer.begin(), first);
            auto copy = cloneArrayDataPointer(dataPointer, dataPointer.size);

            dataPointer->erase(first, last - first);
            QCOMPARE(size_t(dataPointer.size), originalSize - distance);
            size_t i = 0;
            for (; i < pos; ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i]);
            for (; i < size_t(dataPointer.size); ++i)
                QCOMPARE(dataPointer.data()[i], copy.data()[i + distance]);
        };

        auto [intData, strData, objData] = setupDataPointers(100, 100);

        // erase chunk from the beginning
        RUN_TEST_FUNC(testErase, intData, intData.begin(), intData.begin() + 10);
        RUN_TEST_FUNC(testErase, strData, strData.begin(), strData.begin() + 10);
        RUN_TEST_FUNC(testErase, objData, objData.begin(), objData.begin() + 10);

        // erase chunk from the end
        RUN_TEST_FUNC(testErase, intData, intData.end() - 10, intData.end());
        RUN_TEST_FUNC(testErase, strData, strData.end() - 10, strData.end());
        RUN_TEST_FUNC(testErase, objData, objData.end() - 10, objData.end());

        // erase the middle chunk
        RUN_TEST_FUNC(testErase, intData, intData.begin() + (intData.size / 2) - 5,
                      intData.begin() + (intData.size / 2) + 5);
        RUN_TEST_FUNC(testErase, strData, strData.begin() + (strData.size / 2) - 5,
                      strData.begin() + (strData.size / 2) + 5);
        RUN_TEST_FUNC(testErase, objData, objData.begin() + (objData.size / 2) - 5,
                      objData.begin() + (objData.size / 2) + 5);

        // erase chunk in the left part of the data
        RUN_TEST_FUNC(testErase, intData, intData.begin() + 1, intData.begin() + 6);
        RUN_TEST_FUNC(testErase, strData, strData.begin() + 1, strData.begin() + 6);
        RUN_TEST_FUNC(testErase, objData, objData.begin() + 1, objData.begin() + 6);

        // erase chunk in the right part of the data
        RUN_TEST_FUNC(testErase, intData, intData.end() - 6, intData.end() - 1);
        RUN_TEST_FUNC(testErase, strData, strData.end() - 6, strData.end() - 1);
        RUN_TEST_FUNC(testErase, objData, objData.end() - 6, objData.end() - 1);

        // erase all
        RUN_TEST_FUNC(testErase, intData, intData.begin(), intData.end());
        RUN_TEST_FUNC(testErase, strData, strData.begin(), strData.end());
        RUN_TEST_FUNC(testErase, objData, objData.begin(), objData.end());
    }
}

Q_DECLARE_METATYPE(QArrayDataPointer<int>)

struct ResetOnDtor
{
    ResetOnDtor()
        : value_()
    {
    }

    ResetOnDtor(int value)
        : value_(value)
    {
    }

    ~ResetOnDtor()
    {
        value_ = 0;
    }

    int value_;
};

bool operator==(const ResetOnDtor &lhs, const ResetOnDtor &rhs)
{
    return lhs.value_ == rhs.value_;
}

template <class T>
void fromRawData_impl()
{
    static const T array[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    {
        // Default: Immutable, sharable
        SimpleVector<T> raw = SimpleVector<T>::fromRawData(array,
                sizeof(array)/sizeof(array[0]));

        QCOMPARE(raw.size(), size_t(11));
        QCOMPARE((const T *)raw.constBegin(), array);
        QCOMPARE((const T *)raw.constEnd(), (const T *)(array + sizeof(array)/sizeof(array[0])));

        QVERIFY(raw.isShared());
        QVERIFY(SimpleVector<T>(raw).isSharedWith(raw));
        QVERIFY(raw.isShared());

        // Detach
        QCOMPARE(raw.back(), T(11));
        QVERIFY((const T *)raw.constBegin() != array);
    }
}

void tst_QArrayData::fromRawData_data()
{
    QTest::addColumn<int>("type");

    QTest::newRow("int") << 0;
    QTest::newRow("ResetOnDtor") << 1;
}
void tst_QArrayData::fromRawData()
{
    QFETCH(int, type);

    switch (type)
    {
        case 0:
            fromRawData_impl<int>();
            break;
        case 1:
            fromRawData_impl<ResetOnDtor>();
            break;

        default:
            QFAIL("Unexpected type data");
    }
}

void tst_QArrayData::literals()
{
    {
        QArrayDataPointer<char> d = Q_ARRAY_LITERAL(char, "ABCDEFGHIJ");
        QCOMPARE(d.size, 10 + 1);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], char('A' + i));
    }

    {
        QList<char> l(Q_ARRAY_LITERAL(char, "ABCDEFGHIJ"));
        QCOMPARE(l.size(), 11);
        QCOMPARE(l.capacity(), 0);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(l.at(i), char('A' + i));

        (void)l.begin(); // "detach"

        QCOMPARE(l.size(), 11);
        QVERIFY(l.capacity() >= l.size());
        for (int i = 0; i < 10; ++i)
            QCOMPARE(l[i], char('A' + i));
    }

    {
        // wchar_t is not necessarily 2-bytes
        QArrayDataPointer<wchar_t> d = Q_ARRAY_LITERAL(wchar_t, L"ABCDEFGHIJ");
        QCOMPARE(d.size, 10 + 1);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], wchar_t('A' + i));
    }

    struct LiteralType {
        int value;
        constexpr LiteralType(int v = 0) : value(v) {}
    };

    {
        QArrayDataPointer<LiteralType> d = Q_ARRAY_LITERAL(LiteralType, LiteralType(0), LiteralType(1), LiteralType(2));
        QCOMPARE(d->size, 3);
        for (int i = 0; i < 3; ++i)
            QCOMPARE(d->data()[i].value, i);
    }

    {
        QList<LiteralType> l(Q_ARRAY_LITERAL(LiteralType, LiteralType(0), LiteralType(1), LiteralType(2)));
        QCOMPARE(l.size(), 3);
        QCOMPARE(l.capacity(), 0);
        for (int i = 0; i < 3; ++i)
            QCOMPARE(l.at(i).value, i);
        l.squeeze(); // shouldn't detach
        QCOMPARE(l.capacity(), 0);

        (void)l.begin(); // "detach"

        QCOMPARE(l.size(), 3);
        QVERIFY(l.capacity() >= l.size());
        for (int i = 0; i < 3; ++i)
            QCOMPARE(l[i].value, i);
    }
}

// Variadic Q_ARRAY_LITERAL need to be available in the current configuration.
void tst_QArrayData::variadicLiterals()
{
    {
        QArrayDataPointer<int> d =
            Q_ARRAY_LITERAL(int, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
        QCOMPARE(d.size, 10);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], i);
    }

    {
        QArrayDataPointer<char> d = Q_ARRAY_LITERAL(char,
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J');
        QCOMPARE(d.size, 10);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], char('A' + i));
    }

    {
        QArrayDataPointer<const char *> d = Q_ARRAY_LITERAL(const char *,
                "A", "B", "C", "D", "E", "F", "G", "H", "I", "J");
        QCOMPARE(d.size, 10);
        for (int i = 0; i < 10; ++i) {
            QCOMPARE(d.data()[i][0], char('A' + i));
            QCOMPARE(d.data()[i][1], '\0');
        }
    }
}

// std::remove_reference is in C++11, but requires library support
template <class T> struct RemoveReference { typedef T Type; };
template <class T> struct RemoveReference<T &> { typedef T Type; };

// single-argument std::move is in C++11, but requires library support
template <class T>
typename RemoveReference<T>::Type &&cxx11Move(T &&t)
{
    return static_cast<typename RemoveReference<T>::Type &&>(t);
}

struct CompilerHasCxx11ImplicitMoves
{
    static bool value()
    {
        DetectImplicitMove d(cxx11Move(DetectImplicitMove()));
        return d.constructor == DetectConstructor::MoveConstructor;
    }

    struct DetectConstructor
    {
        constexpr DetectConstructor()
            : constructor(DefaultConstructor)
        {
        }

        constexpr DetectConstructor(const DetectConstructor &)
            : constructor(CopyConstructor)
        {
        }

        constexpr DetectConstructor(DetectConstructor &&)
            : constructor(MoveConstructor)
        {
        }

        enum Constructor {
            DefaultConstructor,
            CopyConstructor,
            MoveConstructor
        };

        Constructor constructor;
    };

    struct DetectImplicitMove
        : DetectConstructor
    {
    };
};

// RValue references need to be supported in the current configuration
void tst_QArrayData::rValueReferences()
{
    if (!CompilerHasCxx11ImplicitMoves::value())
        QSKIP("Implicit move ctor not supported in current configuration");

    SimpleVector<int> v1(1, 42);
    SimpleVector<int> v2;

    const SimpleVector<int>::const_iterator begin = v1.constBegin();

    QVERIFY(!v1.isNull());
    QVERIFY(v2.isNull());

    // move-assign
    v2 = cxx11Move(v1);

    QVERIFY(v1.isNull());
    QVERIFY(!v2.isNull());
    QCOMPARE(v2.constBegin(), begin);

    SimpleVector<int> v3(cxx11Move(v2));

    QVERIFY(v1.isNull());
    QVERIFY(v2.isNull());
    QVERIFY(!v3.isNull());
    QCOMPARE(v3.constBegin(), begin);

    QCOMPARE(v3.size(), size_t(1));
    QCOMPARE(v3.front(), 42);
}

void tst_QArrayData::grow()
{
    SimpleVector<int> vector;

    QCOMPARE(vector.size(), size_t(0));
    QCOMPARE(vector.capacity(), size_t(0));

    size_t previousCapacity = 0;
    size_t allocations = 0;
    for (size_t i = 1; i < (1 << 20); ++i) {
        int source[1] = { int(i) };
        vector.append(source, source + 1);
        QCOMPARE(vector.size(), i);
        if (vector.capacity() != previousCapacity) {
            // Don't re-allocate until necessary
            QVERIFY(previousCapacity < i);

            previousCapacity = vector.capacity();
            ++allocations;

            // Going element-wise is slow under valgrind
            if (previousCapacity - i > 10) {
                i = previousCapacity - 5;
                vector.back() = -int(i);
                vector.resize(i);

                // It's still not the time to re-allocate
                QCOMPARE(vector.capacity(), previousCapacity);
            }
        }
    }
    QVERIFY(vector.size() >= size_t(1 << 20));

    // QArrayData::Grow prevents excessive allocations on a growing container
    QVERIFY(allocations > 20 / 2);
    QVERIFY(allocations < 20 * 2);

    for (size_t i = 0; i < vector.size(); ++i) {
        int value = const_(vector).at(i);
        if (value < 0) {
            i = -value;
            continue;
        }

        QCOMPARE(value, int(i + 1));
    }
}

void tst_QArrayData::freeSpace_data()
{
    QTest::addColumn<size_t>("n");

    for (const size_t n : {1, 3, 5, 7, 16, 25}) {
        QString suffix = QString::number(n) + QLatin1String("-elements");
        QTest::newRow(qPrintable(QLatin1String("alloc-") + suffix))
            << n;
    }
}

void tst_QArrayData::freeSpace()
{
    QFETCH(size_t, n);
    const auto testFreeSpace = [] (auto dummy, qsizetype n) {
        using Type = std::decay_t<decltype(dummy)>;
        using DataPointer = QArrayDataPointer<Type>;
        Q_UNUSED(dummy);
        const qsizetype capacity = n + 1;
        auto ptr = DataPointer::allocateGrow(DataPointer(), capacity, QArrayData::GrowsAtEnd);
        const auto alloc = qsizetype(ptr.constAllocatedCapacity());
        QVERIFY(alloc >= capacity);
        QCOMPARE(ptr.freeSpaceAtBegin() + ptr.freeSpaceAtEnd(), alloc);
    };
    RUN_TEST_FUNC(testFreeSpace, char(0), n);
    RUN_TEST_FUNC(testFreeSpace, char16_t(0), n);
    RUN_TEST_FUNC(testFreeSpace, int(0), n);
    RUN_TEST_FUNC(testFreeSpace, QString(), n);
    RUN_TEST_FUNC(testFreeSpace, CountedObject(), n);
}

void tst_QArrayData::dataPointerAllocate_data()
{
    QTest::addColumn<QArrayData::GrowthPosition>("GrowthPosition");

    QTest::newRow("at-end") << QArrayData::GrowsAtEnd;
    QTest::newRow("at-begin") << QArrayData::GrowsAtBeginning;
}

void tst_QArrayData::dataPointerAllocate()
{
    QFETCH(QArrayData::GrowthPosition, GrowthPosition);
    const auto createDataPointer = [] (qsizetype capacity, auto initValue) {
        using Type = std::decay_t<decltype(initValue)>;
        Q_UNUSED(initValue);
        return QArrayDataPointer<Type>(QTypedArrayData<Type>::allocate(capacity));
    };

    const auto testRealloc = [&] (qsizetype capacity, qsizetype newSize, auto initValue) {
        using Type = std::decay_t<decltype(initValue)>;
        using DataPointer = QArrayDataPointer<Type>;

        auto oldDataPointer = createDataPointer(capacity, initValue);
        oldDataPointer->insert(0, 1, initValue);
        oldDataPointer->insert(0, 1, initValue);  // trigger prepend
        QVERIFY(!oldDataPointer.needsDetach());

        auto newDataPointer = DataPointer::allocateGrow(oldDataPointer, newSize, GrowthPosition);
        const auto newAlloc = newDataPointer.constAllocatedCapacity();
        const auto freeAtBegin = newDataPointer.freeSpaceAtBegin();
        const auto freeAtEnd = newDataPointer.freeSpaceAtEnd();

        QVERIFY(newAlloc >= oldDataPointer.constAllocatedCapacity());
        QCOMPARE(freeAtBegin + freeAtEnd, newAlloc);
        if (GrowthPosition == QArrayData::GrowsAtBeginning) {
            QVERIFY(freeAtBegin > 0);
        } else if (GrowthPosition & QArrayData::GrowsAtEnd) {
            QCOMPARE(freeAtBegin, oldDataPointer.freeSpaceAtBegin());
            QVERIFY(freeAtEnd > 0);
        }
    };

    for (size_t n : {10, 512, 1000}) {
        RUN_TEST_FUNC(testRealloc, n, n + 1, int(0));
        RUN_TEST_FUNC(testRealloc, n, n + 1, char('a'));
        RUN_TEST_FUNC(testRealloc, n, n + 1, char16_t(u'a'));
        RUN_TEST_FUNC(testRealloc, n, n + 1, QString("hello, world!"));
        RUN_TEST_FUNC(testRealloc, n, n + 1, CountedObject());
    }

    const auto testDetachRealloc = [&] (qsizetype capacity, qsizetype newSize, auto initValue) {
        using Type = std::decay_t<decltype(initValue)>;
        using DataPointer = QArrayDataPointer<Type>;

        auto oldDataPointer = createDataPointer(capacity, initValue);
        oldDataPointer->insert(0, 1, initValue);  // trigger prepend
        auto oldDataPointerCopy = oldDataPointer;  // force detach later
        QVERIFY(oldDataPointer.needsDetach());

        auto newDataPointer = DataPointer::allocateGrow(oldDataPointer, oldDataPointer->detachCapacity(newSize), GrowthPosition);
        const auto newAlloc = newDataPointer.constAllocatedCapacity();
        const auto freeAtBegin = newDataPointer.freeSpaceAtBegin();
        const auto freeAtEnd = newDataPointer.freeSpaceAtEnd();

        QVERIFY(newAlloc > oldDataPointer.constAllocatedCapacity());
        QCOMPARE(freeAtBegin + freeAtEnd, newAlloc);
        if (GrowthPosition == QArrayData::GrowsAtBeginning) {
            QVERIFY(freeAtBegin > 0);
        } else if (GrowthPosition & QArrayData::GrowsAtEnd) {
            QCOMPARE(freeAtBegin, oldDataPointer.freeSpaceAtBegin());
            QVERIFY(freeAtEnd > 0);
        }
    };

    for (size_t n : {10, 512, 1000}) {
        RUN_TEST_FUNC(testDetachRealloc, n, n + 1, int(0));
        RUN_TEST_FUNC(testDetachRealloc, n, n + 1, char('a'));
        RUN_TEST_FUNC(testDetachRealloc, n, n + 1, char16_t(u'a'));
        RUN_TEST_FUNC(testDetachRealloc, n, n + 1, QString("hello, world!"));
        RUN_TEST_FUNC(testDetachRealloc, n, n + 1, CountedObject());
    }
}

struct MyQStringWrapper : public QString
{
    bool movedTo = false;
    bool movedFrom = false;
    MyQStringWrapper() = default;
    MyQStringWrapper(QChar c) : QString(c) { }
    MyQStringWrapper(MyQStringWrapper &&other) : QString(std::move(static_cast<QString &>(other)))
    {
        movedTo = true;
        movedFrom = other.movedFrom;
        other.movedFrom = true;
    }
    MyQStringWrapper &operator=(MyQStringWrapper &&other)
    {
        QString::operator=(std::move(static_cast<QString &>(other)));
        movedTo = true;
        movedFrom = other.movedFrom;
        other.movedFrom = true;
        return *this;
    }
    MyQStringWrapper(const MyQStringWrapper &) = default;
    MyQStringWrapper &operator=(const MyQStringWrapper &) = default;
    ~MyQStringWrapper() = default;
};

struct MyMovableQString : public MyQStringWrapper
{
    MyMovableQString() = default;
    MyMovableQString(QChar c) : MyQStringWrapper(c) { }

private:
    friend bool operator==(const MyMovableQString &a, QChar c)
    {
        return static_cast<QString>(a) == QString(c);
    }

    friend bool operator==(const MyMovableQString &a, const MyMovableQString &b)
    {
        return static_cast<QString>(a) == static_cast<QString>(b);
    }
};

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(MyMovableQString, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE
static_assert(QTypeInfo<MyMovableQString>::isComplex);
static_assert(QTypeInfo<MyMovableQString>::isRelocatable);

struct MyComplexQString : public MyQStringWrapper
{
    MyComplexQString() = default;
    MyComplexQString(QChar c) : MyQStringWrapper(c) { }

private:
    friend bool operator==(const MyComplexQString &a, QChar c)
    {
        return static_cast<QString>(a) == QString(c);
    }

    friend bool operator==(const MyComplexQString &a, const MyComplexQString &b)
    {
        return static_cast<QString>(a) == static_cast<QString>(b);
    }
};
static_assert(QTypeInfo<MyComplexQString>::isComplex);
static_assert(!QTypeInfo<MyComplexQString>::isRelocatable);

void tst_QArrayData::selfEmplaceBackwards()
{
    const auto createDataPointer = [](qsizetype capacity, int spaceAtEnd, auto dummy) {
        using Type = std::decay_t<decltype(dummy)>;
        Q_UNUSED(dummy);
        auto [header, ptr] = QTypedArrayData<Type>::allocate(capacity, QArrayData::Grow);
        // do custom adjustments to make sure there's free space at end
        ptr += header->alloc - spaceAtEnd;
        return QArrayDataPointer(header, ptr);
    };

    const auto testSelfEmplace = [&](auto dummy, int spaceAtEnd, auto initValues) {
        auto adp = createDataPointer(100, spaceAtEnd, dummy);
        for (auto v : initValues) {
            adp->emplace(adp.size, v);
        }
        QVERIFY(!adp.freeSpaceAtEnd());
        QVERIFY(adp.freeSpaceAtBegin());

        adp->emplace(adp.size, adp.data()[0]);
        for (qsizetype i = 0; i < adp.size - 1; ++i) {
            QCOMPARE(adp.data()[i], initValues[i]);
        }
        QCOMPARE(adp.data()[adp.size - 1], initValues[0]);

        adp->emplace(adp.size, std::move(adp.data()[0]));
        for (qsizetype i = 1; i < adp.size - 2; ++i) {
            QCOMPARE(adp.data()[i], initValues[i]);
        }
        QCOMPARE(adp.data()[adp.size - 2], initValues[0]);
        QCOMPARE(adp.data()[0].movedFrom, true);
        QCOMPARE(adp.data()[adp.size - 1], initValues[0]);
        QCOMPARE(adp.data()[adp.size - 1].movedTo, true);
    };

    QList<QChar> movableObjs { u'a', u'b', u'c', u'd' };
    RUN_TEST_FUNC(testSelfEmplace, MyMovableQString(), 4, movableObjs);
    QList<QChar> complexObjs { u'a', u'b', u'c', u'd' };
    RUN_TEST_FUNC(testSelfEmplace, MyComplexQString(), 4, complexObjs);
}

void tst_QArrayData::selfEmplaceForward()
{
    const auto createDataPointer = [](qsizetype capacity, int spaceAtBegin, auto dummy) {
        using Type = std::decay_t<decltype(dummy)>;
        Q_UNUSED(dummy);
        auto [header, ptr] = QTypedArrayData<Type>::allocate(capacity, QArrayData::Grow);
        // do custom adjustments to make sure there's free space at end
        ptr += spaceAtBegin;
        return QArrayDataPointer(header, ptr);
    };

    const auto testSelfEmplace = [&](auto dummy, int spaceAtBegin, auto initValues) {
        // need a -1 below as the first emplace will go towards the end (as the array is still empty)
        auto adp = createDataPointer(100, spaceAtBegin - 1, dummy);
        auto reversedInitValues = initValues;
        std::reverse(reversedInitValues.begin(), reversedInitValues.end());
        for (auto v : reversedInitValues) {
            adp->emplace(0, v);
        }
        QVERIFY(!adp.freeSpaceAtBegin());
        QVERIFY(adp.freeSpaceAtEnd());

        adp->emplace(0, adp.data()[adp.size - 1]);
        for (qsizetype i = 1; i < adp.size; ++i) {
            QCOMPARE(adp.data()[i], initValues[i - 1]);
        }
        QCOMPARE(adp.data()[0], initValues[spaceAtBegin - 1]);

        adp->emplace(0, std::move(adp.data()[adp.size - 1]));
        for (qsizetype i = 2; i < adp.size - 1; ++i) {
            QCOMPARE(adp.data()[i], initValues[i - 2]);
        }
        QCOMPARE(adp.data()[1], initValues[spaceAtBegin - 1]);
        QCOMPARE(adp.data()[adp.size - 1].movedFrom, true);
        QCOMPARE(adp.data()[0], initValues[spaceAtBegin - 1]);
        QCOMPARE(adp.data()[0].movedTo, true);
    };

    QList<QChar> movableObjs { u'a', u'b', u'c', u'd' };
    RUN_TEST_FUNC(testSelfEmplace, MyMovableQString(), 4, movableObjs);
    QList<QChar> complexObjs { u'a', u'b', u'c', u'd' };
    RUN_TEST_FUNC(testSelfEmplace, MyComplexQString(), 4, complexObjs);
}

#ifndef QT_NO_EXCEPTIONS
struct ThrowingTypeWatcher
{
    std::vector<void *> destroyedAddrs;
    bool watch = false;

    void destroyed(void *addr)
    {
        if (watch)
            destroyedAddrs.push_back(addr);
    }
};

ThrowingTypeWatcher &throwingTypeWatcher()
{
    static ThrowingTypeWatcher global;
    return global;
}

struct ThrowingType
{
    static unsigned int throwOnce;
    static constexpr char throwString[] = "Requested to throw";
    enum MoveCase {
        MoveRightNoOverlap,
        MoveRightOverlap,
        MoveLeftNoOverlap,
        MoveLeftOverlap,
    };
    enum ThrowCase {
        NoThrow,
        ThrowInUninitializedRegion,
        ThrowInOverlapRegion,
    };

    // reinforce basic checkers with std::shared_ptr which happens to signal
    // very explicitly about use-after-free and so on under ASan
    std::shared_ptr<int> doubleFreeHelper = std::shared_ptr<int>(new int(42));
    int id = 0;

    void checkThrow()
    {
        // deferred throw
        if (throwOnce > 0) {
            --throwOnce;
            if (throwOnce == 0) {
                throw std::runtime_error(throwString);
            }
        }
        return;
    }

    void copy(const ThrowingType &other) noexcept(false)
    {
        doubleFreeHelper = other.doubleFreeHelper;
        id = other.id;
        checkThrow();
    }

    ThrowingType(int val = 0) noexcept(false) : id(val) { checkThrow(); }
    ThrowingType(const ThrowingType &other) noexcept(false) { copy(other); }
    ThrowingType &operator=(const ThrowingType &other) noexcept(false)
    {
        copy(other);
        return *this;
    }
    ThrowingType(ThrowingType &&other) noexcept(false) { copy(other); }
    ThrowingType &operator=(ThrowingType &&other) noexcept(false)
    {
        copy(other);
        return *this;
    }
    ~ThrowingType() noexcept(true)
    {
        throwingTypeWatcher().destroyed(this); // notify global watcher
        id = -1;
        // if we're in dtor but use_count is 0, it's double free
        QVERIFY(doubleFreeHelper.use_count() > 0);
    }

    friend bool operator==(const ThrowingType &a, const ThrowingType &b) { return a.id == b.id; }
};

unsigned int ThrowingType::throwOnce = 0;
static_assert(!QTypeInfo<ThrowingType>::isRelocatable);

void tst_QArrayData::relocateWithExceptions_data()
{
    QTest::addColumn<ThrowingType::MoveCase>("moveCase");
    QTest::addColumn<ThrowingType::ThrowCase>("throwCase");
    // Not throwing
    QTest::newRow("no-throw-move-right-no-overlap")
            << ThrowingType::MoveRightNoOverlap << ThrowingType::NoThrow;
    QTest::newRow("no-throw-move-right-overlap")
            << ThrowingType::MoveRightOverlap << ThrowingType::NoThrow;
    QTest::newRow("no-throw-move-left-no-overlap")
            << ThrowingType::MoveLeftNoOverlap << ThrowingType::NoThrow;
    QTest::newRow("no-throw-move-left-overlap")
            << ThrowingType::MoveLeftOverlap << ThrowingType::NoThrow;
    // Throwing in uninitialized region
    QTest::newRow("throw-in-uninit-region-move-right-no-overlap")
            << ThrowingType::MoveRightNoOverlap << ThrowingType::ThrowInUninitializedRegion;
    QTest::newRow("throw-in-uninit-region-move-right-overlap")
            << ThrowingType::MoveRightOverlap << ThrowingType::ThrowInUninitializedRegion;
    QTest::newRow("throw-in-uninit-region-move-left-no-overlap")
            << ThrowingType::MoveLeftNoOverlap << ThrowingType::ThrowInUninitializedRegion;
    QTest::newRow("throw-in-uninit-region-move-left-overlap")
            << ThrowingType::MoveLeftOverlap << ThrowingType::ThrowInUninitializedRegion;
    // Throwing in overlap region
    QTest::newRow("throw-in-overlap-region-move-right-overlap")
            << ThrowingType::MoveRightOverlap << ThrowingType::ThrowInOverlapRegion;
    QTest::newRow("throw-in-overlap-region-move-left-overlap")
            << ThrowingType::MoveLeftOverlap << ThrowingType::ThrowInOverlapRegion;
}

void tst_QArrayData::relocateWithExceptions()
{
    // Assume that non-throwing moves perform correctly. Otherwise, all previous
    // tests would've failed. Test only what happens when exceptions are thrown.
    QFETCH(ThrowingType::MoveCase, moveCase);
    QFETCH(ThrowingType::ThrowCase, throwCase);

    struct ThrowingTypeLeakChecker
    {
        ThrowingType::MoveCase moveCase;
        ThrowingType::ThrowCase throwCase;
        size_t containerSize = 0;

        ThrowingTypeLeakChecker(ThrowingType::MoveCase mc, ThrowingType::ThrowCase tc)
            : moveCase(mc), throwCase(tc)
        {
        }

        void start(qsizetype size)
        {
            containerSize = size_t(size);
            throwingTypeWatcher().watch = true;
        }

        ~ThrowingTypeLeakChecker()
        {
            const size_t destroyedElementsCount = throwingTypeWatcher().destroyedAddrs.size();
            const size_t destroyedElementsUniqueCount =
                    std::set<void *>(throwingTypeWatcher().destroyedAddrs.begin(),
                                     throwingTypeWatcher().destroyedAddrs.end())
                            .size();

            // reset the global watcher first and only then verify things
            throwingTypeWatcher().watch = false;
            throwingTypeWatcher().destroyedAddrs.clear();

            size_t deletedByRelocate = 0;
            switch (throwCase) {
            case ThrowingType::NoThrow:
                // if no overlap, N elements from old range. otherwise, N - 1
                // elements from old range
                if (moveCase == ThrowingType::MoveLeftNoOverlap
                    || moveCase == ThrowingType::MoveRightNoOverlap) {
                    deletedByRelocate = containerSize;
                } else {
                    deletedByRelocate = containerSize - 1;
                }
                break;
            case ThrowingType::ThrowInUninitializedRegion:
                // 1 relocated element from uninitialized region
                deletedByRelocate = 1u;
                break;
            case ThrowingType::ThrowInOverlapRegion:
                // 2 relocated elements from uninitialized region
                deletedByRelocate = 2u;
                break;
            default:
                QFAIL("Unknown throwCase");
            }

            QCOMPARE(destroyedElementsCount, deletedByRelocate + containerSize);
            QCOMPARE(destroyedElementsUniqueCount, destroyedElementsCount);
        }
    };

    const auto setDeferredThrow = [throwCase]() {
        switch (throwCase) {
        case ThrowingType::NoThrow:
            break; // do nothing
        case ThrowingType::ThrowInUninitializedRegion:
            ThrowingType::throwOnce = 2;
            break;
        case ThrowingType::ThrowInOverlapRegion:
            ThrowingType::throwOnce = 3;
            break;
        default:
            QFAIL("Unknown throwCase");
        }
    };

    const auto createDataPointer = [](qsizetype capacity, qsizetype initSize) {
        QArrayDataPointer<ThrowingType> qadp(QTypedArrayData<ThrowingType>::allocate(capacity));
        qadp->appendInitialize(initSize);
        int i = 0;
        std::generate(qadp.begin(), qadp.end(), [&i]() { return ThrowingType(i++); });
        return qadp;
    };

    switch (moveCase) {
    case ThrowingType::MoveRightNoOverlap: {
        ThrowingTypeLeakChecker watch(moveCase, throwCase);
        auto storage = createDataPointer(20, 3);
        QVERIFY(storage.freeSpaceAtEnd() > 3);

        watch.start(storage.size);
        try {
            setDeferredThrow();
            storage->relocate(4);
            if (throwCase != ThrowingType::NoThrow)
                QFAIL("Unreachable line!");
        } catch (const std::runtime_error &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
        }
        break;
    }
    case ThrowingType::MoveRightOverlap: {
        ThrowingTypeLeakChecker watch(moveCase, throwCase);
        auto storage = createDataPointer(20, 3);
        QVERIFY(storage.freeSpaceAtEnd() > 3);

        watch.start(storage.size);
        try {
            setDeferredThrow();
            storage->relocate(2);
            if (throwCase != ThrowingType::NoThrow)
                QFAIL("Unreachable line!");
        } catch (const std::runtime_error &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
        }
        break;
    }
    case ThrowingType::MoveLeftNoOverlap: {
        ThrowingTypeLeakChecker watch(moveCase, throwCase);
        auto storage = createDataPointer(20, 2);
        storage->insert(0, 1, ThrowingType(42));
        QVERIFY(storage.freeSpaceAtBegin() > 3);

        watch.start(storage.size);
        try {
            setDeferredThrow();
            storage->relocate(-4);
            if (throwCase != ThrowingType::NoThrow)
                QFAIL("Unreachable line!");
        } catch (const std::runtime_error &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
        }
        break;
    }
    case ThrowingType::MoveLeftOverlap: {
        ThrowingTypeLeakChecker watch(moveCase, throwCase);
        auto storage = createDataPointer(20, 2);
        storage->insert(0, 1, ThrowingType(42));
        QVERIFY(storage.freeSpaceAtBegin() > 3);

        watch.start(storage.size);
        try {
            setDeferredThrow();
            storage->relocate(-2);
            if (throwCase != ThrowingType::NoThrow)
                QFAIL("Unreachable line!");
        } catch (const std::runtime_error &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
        }
        break;
    }
    default:
        QFAIL("Unknown ThrowingType::MoveCase");
    };
}
#endif // QT_NO_EXCEPTIONS

QTEST_APPLESS_MAIN(tst_QArrayData)
#include "tst_qarraydata.moc"
