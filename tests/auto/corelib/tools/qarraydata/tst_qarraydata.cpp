/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include <QtCore/QString>
#include <QtCore/qarraydata.h>

#include "simplevector.h"

#include <array>
#include <tuple>
#include <algorithm>
#include <vector>
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
    void dataPointerAllocate_data() { arrayOps_data(); }
    void dataPointerAllocate();
    void dataPointerAllocateAlignedWithReallocate_data();
    void dataPointerAllocateAlignedWithReallocate();
#ifndef QT_NO_EXCEPTIONS
    void exceptionSafetyPrimitives_constructor();
    void exceptionSafetyPrimitives_destructor();
    void exceptionSafetyPrimitives_mover();
    void exceptionSafetyPrimitives_displacer();
#endif
};

template <class T> const T &const_(const T &t) { return t; }

void tst_QArrayData::referenceCounting()
{
    {
        // Reference counting initialized to 1 (owned)
        QArrayData array = { Q_BASIC_ATOMIC_INITIALIZER(1), 0, 0 };

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
    SimpleVector<int> v3(nullptr, nullptr, 0);
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
    QTest::addColumn<QArrayData::ArrayOptions>("allocateOptions");
    QTest::addColumn<bool>("isCapacityReserved");

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
        QArrayData::ArrayOptions allocateOptions;
        bool isCapacityReserved;
    } options[] = {
        { "Default", QArrayData::DefaultAllocationFlags, false },
        { "Reserved", QArrayData::CapacityReserved, true },
        { "Grow", QArrayData::GrowsForward, false },
        { "GrowBack", QArrayData::GrowsBackwards, false }
    };

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i)
        for (size_t j = 0; j < sizeof(options)/sizeof(options[0]); ++j)
            QTest::newRow(qPrintable(
                        QLatin1String(types[i].typeName)
                        + QLatin1String(": ")
                        + QLatin1String(options[j].description)))
                << types[i].objectSize << types[i].alignment
                << options[j].allocateOptions << options[j].isCapacityReserved;
}

void tst_QArrayData::allocate()
{
    QFETCH(size_t, objectSize);
    QFETCH(size_t, alignment);
    QFETCH(QArrayData::ArrayOptions, allocateOptions);
    QFETCH(bool, isCapacityReserved);

    // Minimum alignment that can be requested is that of QArrayData.
    // Typically, this alignment is sizeof(void *) and ensured by malloc.
    size_t minAlignment = qMax(alignment, alignof(QArrayData));

    Deallocator keeper(objectSize, minAlignment);
    keeper.headers.reserve(1024);

    for (qsizetype capacity = 1; capacity <= 1024; capacity <<= 1) {
        QArrayData *data;
        void *dataPointer = QArrayData::allocate(&data, objectSize, minAlignment,
                capacity, QArrayData::ArrayOptions(allocateOptions));

        keeper.headers.append(data);

        if (allocateOptions & (QArrayData::GrowsForward | QArrayData::GrowsBackwards))
            QVERIFY(data->allocatedCapacity() > capacity);
        else
            QCOMPARE(data->allocatedCapacity(), capacity);
        QCOMPARE(bool(data->flags & QArrayData::CapacityReserved), isCapacityReserved);

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(dataPointer, 'A', objectSize * capacity);
    }
}

void tst_QArrayData::reallocate()
{
    QFETCH(size_t, objectSize);
    QFETCH(size_t, alignment);
    QFETCH(QArrayData::ArrayOptions, allocateOptions);
    QFETCH(bool, isCapacityReserved);

    // Minimum alignment that can be requested is that of QArrayData.
    // Typically, this alignment is sizeof(void *) and ensured by malloc.
    size_t minAlignment = qMax(alignment, alignof(QArrayData));

    int capacity = 10;
    Deallocator keeper(objectSize, minAlignment);
    QArrayData *data;
    void *dataPointer = QArrayData::allocate(&data, objectSize, minAlignment, capacity,
                                             QArrayData::ArrayOptions(allocateOptions) & ~QArrayData::GrowsForward);
    keeper.headers.append(data);

    memset(dataPointer, 'A', objectSize * capacity);

    // now try to reallocate
    int newCapacity = 40;
    auto pair = QArrayData::reallocateUnaligned(data, dataPointer, objectSize, newCapacity,
                                                QArrayData::ArrayOptions(allocateOptions));
    data = pair.first;
    dataPointer = pair.second;
    QVERIFY(data);
    keeper.headers.clear();
    keeper.headers.append(data);

    if (allocateOptions & (QArrayData::GrowsForward | QArrayData::GrowsBackwards))
        QVERIFY(data->allocatedCapacity() > newCapacity);
    else
        QCOMPARE(data->allocatedCapacity(), newCapacity);
    QCOMPARE(!(data->flags & QArrayData::CapacityReserved), !isCapacityReserved);

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
        void *dataPointer = QArrayData::allocate(&data, sizeof(Unaligned),
                minAlignment, 8, QArrayData::DefaultAllocationFlags);
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
    QTest::addColumn<QArrayData::ArrayOptions>("allocationOptions");

    QTest::newRow("default-alloc") << QArrayData::ArrayOptions(QArrayData::DefaultAllocationFlags);
    QTest::newRow("grows-forward") << QArrayData::ArrayOptions(QArrayData::GrowsForward);
    QTest::newRow("grows-backwards") << QArrayData::ArrayOptions(QArrayData::GrowsBackwards);
    QTest::newRow("grows-bidirectional")
        << QArrayData::ArrayOptions(QArrayData::GrowsForward | QArrayData::GrowsBackwards);
    QTest::newRow("reserved-capacity")
        << QArrayData::ArrayOptions(QArrayData::CapacityReserved);
    QTest::newRow("reserved-capacity-grows-forward")
        << QArrayData::ArrayOptions(QArrayData::GrowsForward | QArrayData::CapacityReserved);
    QTest::newRow("reserved-capacity-grows-backwards")
        << QArrayData::ArrayOptions(QArrayData::GrowsBackwards | QArrayData::CapacityReserved);
    QTest::newRow("reserved-capacity-grows-bidirectional")
        << QArrayData::ArrayOptions(QArrayData::GrowsForward | QArrayData::GrowsBackwards
                                    | QArrayData::CapacityReserved);
}

void tst_QArrayData::arrayOps()
{
    QFETCH(QArrayData::ArrayOptions, allocationOptions);
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
    SimpleVector<int> vi(intArray, intArray + 5, allocationOptions);
    SimpleVector<QString> vs(stringArray, stringArray + 5, allocationOptions);
    SimpleVector<CountedObject> vo(objArray, objArray + 5, allocationOptions);

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

    vi = SimpleVector<int>(5, referenceInt, allocationOptions);
    vs = SimpleVector<QString>(5, referenceString, allocationOptions);
    vo = SimpleVector<CountedObject>(5, referenceObject, allocationOptions);

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
        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
                | CountedObject::CopyAssigned);
    }

    for (int i = 15; i < 20; ++i) {
        QCOMPARE(vi[i], referenceInt);
        QVERIFY(vs[i].isSharedWith(referenceString));

        QCOMPARE(vo[i].id, referenceObject.id);
        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
                | CountedObject::CopyAssigned);
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
        QCOMPARE(vo[i].flags & CountedObject::CopyAssigned,
                int(CountedObject::CopyAssigned));
    }

    for (int i = 25; i < 30; ++i) {
        QCOMPARE(vi[i], referenceInt);
        QVERIFY(vs[i].isSharedWith(referenceString));

        QCOMPARE(vo[i].id, referenceObject.id);
        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
                | CountedObject::CopyAssigned);
    }
}

void tst_QArrayData::arrayOps2_data()
{
    arrayOps_data();
}

void tst_QArrayData::arrayOps2()
{
    QFETCH(QArrayData::ArrayOptions, allocationOptions);
    CountedObject::LeakChecker leakChecker; Q_UNUSED(leakChecker);

    ////////////////////////////////////////////////////////////////////////////
    // appendInitialize
    SimpleVector<int> vi(5, allocationOptions);
    SimpleVector<QString> vs(5, allocationOptions);
    SimpleVector<CountedObject> vo(5, allocationOptions);

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
    arrayOps_data();
}

void tst_QArrayData::arrayOpsExtra()
{
    QSKIP("Skipped while changing QArrayData operations.", SkipAll);
    QFETCH(QArrayData::ArrayOptions, allocationOptions);
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

    const auto setupDataPointers = [&allocationOptions] (size_t capacity, size_t initialSize = 0) {
        const qsizetype alloc = qsizetype(capacity);
        auto i = QArrayDataPointer<int>::allocateGrow(QArrayDataPointer<int>(), alloc,
                                                      initialSize, allocationOptions);
        auto s = QArrayDataPointer<QString>::allocateGrow(QArrayDataPointer<QString>(), alloc,
                                                          initialSize, allocationOptions);
        auto o = QArrayDataPointer<CountedObject>::allocateGrow(QArrayDataPointer<CountedObject>(), alloc,
                                                                initialSize, allocationOptions);
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
        ArrayPointer copy(QTypedArrayData<Type>::allocate(qsizetype(capacity), dataPointer.flags()));
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

            dataPointer->copyAppend(first, last);
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
        if (allocationOptions & QArrayData::GrowsBackwards) {
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
        if (allocationOptions & QArrayData::GrowsBackwards) {
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
        if (allocationOptions & QArrayData::GrowsBackwards) {
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

            dataPointer->insert(dataPointer.begin() + pos, first, last);
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

            dataPointer->insert(dataPointer.begin() + pos, n, value);
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

            dataPointer->insert(dataPointer.begin(), n, value);
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
        intData->insert(intData.begin(), intData.freeSpaceAtBegin(), intData.data()[0]);
        strData->insert(strData.begin(), strData.freeSpaceAtBegin(), strData.data()[0]);
        objData->insert(objData.begin(), objData.freeSpaceAtBegin(), objData.data()[0]);

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

            dataPointer->emplace(dataPointer.begin() + pos, value);
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

            dataPointer->erase(first, last);
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
        QCOMPARE(d.size, 10u + 1u);
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
        QCOMPARE(d.size, 10u + 1u);
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
        QCOMPARE(d.size, 10u);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], i);
    }

    {
        QArrayDataPointer<char> d = Q_ARRAY_LITERAL(char,
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J');
        QCOMPARE(d.size, 10u);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d.data()[i], char('A' + i));
    }

    {
        QArrayDataPointer<const char *> d = Q_ARRAY_LITERAL(const char *,
                "A", "B", "C", "D", "E", "F", "G", "H", "I", "J");
        QCOMPARE(d.size, 10u);
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
    QTest::addColumn<QArrayData::ArrayOptions>("allocationOptions");
    QTest::addColumn<size_t>("n");

    for (const size_t n : {1, 3, 5, 7, 16, 25}) {
        QString suffix = QString::number(n) + QLatin1String("-elements");
        QTest::newRow(qPrintable(QLatin1String("default-alloc-") + suffix))
            << QArrayData::ArrayOptions(QArrayData::DefaultAllocationFlags) << n;
        QTest::newRow(qPrintable(QLatin1String("grows-forward-") + suffix))
            << QArrayData::ArrayOptions(QArrayData::GrowsForward) << n;
        QTest::newRow(qPrintable(QLatin1String("grows-bidirectional-") + suffix))
            << QArrayData::ArrayOptions(QArrayData::GrowsForward | QArrayData::GrowsBackwards) << n;
    }
}

void tst_QArrayData::freeSpace()
{
    QFETCH(QArrayData::ArrayOptions, allocationOptions);
    QFETCH(size_t, n);
    const auto testFreeSpace = [] (auto dummy, auto options, qsizetype n) {
        using Type = std::decay_t<decltype(dummy)>;
        using DataPointer = QArrayDataPointer<Type>;
        Q_UNUSED(dummy);
        const qsizetype capacity = n + 1;
        auto ptr = DataPointer::allocateGrow(DataPointer(), capacity, n, options);
        const auto alloc = qsizetype(ptr.constAllocatedCapacity());
        QVERIFY(alloc >= capacity);
        QCOMPARE(ptr.freeSpaceAtBegin() + ptr.freeSpaceAtEnd(), alloc);
    };
    RUN_TEST_FUNC(testFreeSpace, char(0), allocationOptions, n);
    RUN_TEST_FUNC(testFreeSpace, char16_t(0), allocationOptions, n);
    RUN_TEST_FUNC(testFreeSpace, int(0), allocationOptions, n);
    RUN_TEST_FUNC(testFreeSpace, QString(), allocationOptions, n);
    RUN_TEST_FUNC(testFreeSpace, CountedObject(), allocationOptions, n);
}

void tst_QArrayData::dataPointerAllocate()
{
    QFETCH(QArrayData::ArrayOptions, allocationOptions);
    const auto createDataPointer = [] (qsizetype capacity, auto initValue) {
        using Type = std::decay_t<decltype(initValue)>;
        Q_UNUSED(initValue);
        return QArrayDataPointer<Type>(QTypedArrayData<Type>::allocate(capacity));
    };

    const auto testRealloc = [&] (qsizetype capacity, qsizetype newSize, auto initValue) {
        using Type = std::decay_t<decltype(initValue)>;
        using DataPointer = QArrayDataPointer<Type>;

        auto oldDataPointer = createDataPointer(capacity, initValue);
        oldDataPointer->insert(oldDataPointer.begin(), 1, initValue);
        oldDataPointer->insert(oldDataPointer.begin(), 1, initValue);  // trigger prepend
        QVERIFY(!oldDataPointer.needsDetach());

        auto newDataPointer = DataPointer::allocateGrow(
            oldDataPointer, oldDataPointer->detachCapacity(newSize), newSize, allocationOptions);
        const auto newAlloc = newDataPointer.constAllocatedCapacity();
        const auto freeAtBegin = newDataPointer.freeSpaceAtBegin();
        const auto freeAtEnd = newDataPointer.freeSpaceAtEnd();

        QVERIFY(newAlloc > oldDataPointer.constAllocatedCapacity());
        QCOMPARE(freeAtBegin + freeAtEnd, newAlloc);
        if (allocationOptions & QArrayData::GrowsBackwards) {
            // bad check, but will suffice for now, hopefully
            QCOMPARE(freeAtBegin, (newAlloc - newSize) / 2);
        } else if (allocationOptions & QArrayData::GrowsForward) {
            QCOMPARE(freeAtBegin, oldDataPointer.freeSpaceAtBegin());
        } else {
            QCOMPARE(freeAtBegin, 0);
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
        oldDataPointer->insert(oldDataPointer.begin(), 1, initValue);  // trigger prepend
        auto oldDataPointerCopy = oldDataPointer;  // force detach later
        QVERIFY(oldDataPointer.needsDetach());

        auto newDataPointer = DataPointer::allocateGrow(
            oldDataPointer, oldDataPointer->detachCapacity(newSize), newSize, allocationOptions);
        const auto newAlloc = newDataPointer.constAllocatedCapacity();
        const auto freeAtBegin = newDataPointer.freeSpaceAtBegin();
        const auto freeAtEnd = newDataPointer.freeSpaceAtEnd();

        QVERIFY(newAlloc > oldDataPointer.constAllocatedCapacity());
        QCOMPARE(freeAtBegin + freeAtEnd, newAlloc);
        if (allocationOptions & QArrayData::GrowsBackwards) {
            QCOMPARE(freeAtBegin, (newAlloc - newSize) / 2);
        } else {
            QCOMPARE(freeAtBegin, 0);
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

void tst_QArrayData::dataPointerAllocateAlignedWithReallocate_data()
{
    QTest::addColumn<QArrayData::ArrayOptions>("initFlags");
    QTest::addColumn<QArrayData::ArrayOptions>("newFlags");

    QTest::newRow("default-flags") << QArrayData::ArrayOptions(QArrayData::DefaultAllocationFlags)
                                   << QArrayData::ArrayOptions(QArrayData::DefaultAllocationFlags);
    QTest::newRow("no-grows-backwards") << QArrayData::ArrayOptions(QArrayData::GrowsForward)
                                        << QArrayData::ArrayOptions(QArrayData::GrowsForward);
    QTest::newRow("grows-backwards") << QArrayData::ArrayOptions(QArrayData::GrowsBackwards)
                                     << QArrayData::ArrayOptions(QArrayData::GrowsBackwards);
    QTest::newRow("removed-grows-backwards") << QArrayData::ArrayOptions(QArrayData::GrowsBackwards)
                                             << QArrayData::ArrayOptions(QArrayData::GrowsForward);
    QTest::newRow("removed-growth") << QArrayData::ArrayOptions(QArrayData::GrowsBackwards)
                                    << QArrayData::ArrayOptions(QArrayData::DefaultAllocationFlags);
}

void tst_QArrayData::dataPointerAllocateAlignedWithReallocate()
{
    QFETCH(QArrayData::ArrayOptions, initFlags);
    QFETCH(QArrayData::ArrayOptions, newFlags);

    // Note: using the same type to ensure alignment and padding are the same.
    //       otherwise, we may get differences in the allocated size
    auto a = QArrayDataPointer<int>::allocateGrow(QArrayDataPointer<int>(), 50, 0, initFlags);
    auto b = QArrayDataPointer<int>::allocateGrow(QArrayDataPointer<int>(), 50, 0, initFlags);

    if (initFlags & QArrayData::GrowsBackwards) {
        QVERIFY(a.freeSpaceAtBegin() > 0);
    } else {
        QVERIFY(a.freeSpaceAtBegin() == 0);
    }
    QCOMPARE(a.freeSpaceAtBegin(), b.freeSpaceAtBegin());
    const auto oldSpaceAtBeginA = a.freeSpaceAtBegin();

    a->reallocate(100, newFlags);
    b = QArrayDataPointer<int>::allocateGrow(b, 100, b.size, newFlags);

    // NB: when growing backwards, the behavior is not aligned
    if (!(newFlags & QArrayData::GrowsBackwards)) {
        QCOMPARE(a.freeSpaceAtBegin(), b.freeSpaceAtBegin());
    } else {
        QCOMPARE(a.freeSpaceAtBegin(), oldSpaceAtBeginA);
        QCOMPARE(b.freeSpaceAtBegin(), b.constAllocatedCapacity() / 2);
    }
}

#ifndef QT_NO_EXCEPTIONS
struct ThrowingTypeWatcher
{
    std::vector<int> destroyedIds;
    bool watch = false;
    void destroyed(int id)
    {
        if (watch)
            destroyedIds.push_back(id);
    }
};
ThrowingTypeWatcher& throwingTypeWatcher() { static ThrowingTypeWatcher global; return global; }

struct ThrowingType
{
    static unsigned int throwOnce;
    static unsigned int throwOnceInDtor;
    static constexpr char throwString[] = "Requested to throw";
    static constexpr char throwStringDtor[] = "Requested to throw in dtor";
    void checkThrow()  {
        // deferred throw
        if (throwOnce > 0) {
            --throwOnce;
            if (throwOnce == 0) {
                throw std::runtime_error(throwString);
            }
        }
        return;
    }
    int id = 0;

    ThrowingType(int val = 0) noexcept(false) : id(val)
    {
        checkThrow();
    }
    ThrowingType(const ThrowingType &other) noexcept(false) : id(other.id)
    {
        checkThrow();
    }
    ThrowingType& operator=(const ThrowingType &other) noexcept(false)
    {
        id = other.id;
        checkThrow();
        return *this;
    }
    ~ThrowingType() noexcept(false)
    {
        throwingTypeWatcher().destroyed(id);  // notify global watcher
        id = -1;

        // deferred throw
        if (throwOnceInDtor > 0) {
            --throwOnceInDtor;
            if (throwOnceInDtor == 0) {
                throw std::runtime_error(throwStringDtor);
            }
        }
    }
};
unsigned int ThrowingType::throwOnce = 0;
unsigned int ThrowingType::throwOnceInDtor = 0;
bool operator==(const ThrowingType &a, const ThrowingType &b) {
    return a.id == b.id;
}
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(ThrowingType, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE

template<typename T>  // T must be constructible from a single int parameter
static QArrayDataPointer<T> createDataPointer(qsizetype capacity, qsizetype initSize)
{
    QArrayDataPointer<T> adp(QTypedArrayData<T>::allocate(capacity));
    adp->appendInitialize(initSize);
    // assign unique values
    int i = 0;
    std::generate(adp.begin(), adp.end(), [&i] () { return T(i++); });
    return adp;
}

void tst_QArrayData::exceptionSafetyPrimitives_constructor()
{
    using Prims = QtPrivate::QArrayExceptionSafetyPrimitives<ThrowingType>;
    using Constructor = typename Prims::Constructor;

    struct WatcherScope
    {
        WatcherScope() { throwingTypeWatcher().watch = true; }
        ~WatcherScope()
        {
            throwingTypeWatcher().watch = false;
            throwingTypeWatcher().destroyedIds.clear();
        }
    };

    const auto doConstruction = [] (auto &dataPointer, auto where, auto op) {
        Constructor ctor(where);
        dataPointer.size += op(ctor);
    };

    // empty ranges
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        const auto originalSize = data.size;
        const std::array<ThrowingType, 0> emptyRange{};

        doConstruction(data, data.end(), [] (Constructor &ctor) { return ctor.create(0); });
        QCOMPARE(data.size, originalSize);

        doConstruction(data, data.end(), [&emptyRange] (Constructor &ctor) {
            return ctor.copy(emptyRange.begin(), emptyRange.end());
        });
        QCOMPARE(data.size, originalSize);

        doConstruction(data, data.end(), [] (Constructor &ctor) {
            return ctor.clone(0, ThrowingType(42));
        });
        QCOMPARE(data.size, originalSize);

        doConstruction(data, data.end(), [emptyRange] (Constructor &ctor) mutable {
            return ctor.move(emptyRange.begin(), emptyRange.end());
        });
        QCOMPARE(data.size, originalSize);
    }

    // successful create
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->appendInitialize(reference.size + 1);

        doConstruction(data, data.end(), [] (Constructor &ctor) { return ctor.create(1); });

        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
    }

    // successful copy
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        const std::array<ThrowingType, 3> source = {
            ThrowingType(42), ThrowingType(43), ThrowingType(44)
        };
        reference->copyAppend(source.begin(), source.end());

        doConstruction(data, data.end(), [&source] (Constructor &ctor) {
            return ctor.copy(source.begin(), source.end());
        });

        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);

        reference->copyAppend(2, source[0]);

        doConstruction(data, data.end(), [&source] (Constructor &ctor) {
            return ctor.clone(2, source[0]);
        });

        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
    }

    // successful move
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        const std::array<ThrowingType, 3> source = {
            ThrowingType(42), ThrowingType(43), ThrowingType(44)
        };
        reference->copyAppend(source.begin(), source.end());

        doConstruction(data, data.end(), [source] (Constructor &ctor) mutable {
            return ctor.move(source.begin(), source.end());
        });

        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
    }

    // failed create
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);

        for (uint throwOnNthConstruction : {1, 3}) {
            WatcherScope scope; Q_UNUSED(scope);
            try {
                ThrowingType::throwOnce = throwOnNthConstruction;
                doConstruction(data, data.end(), [] (Constructor &ctor) {
                    return ctor.create(5);
                });
            } catch (const std::runtime_error &e) {
                QCOMPARE(std::string(e.what()), ThrowingType::throwString);
                QCOMPARE(data.size, reference.size);
                for (qsizetype i = 0; i < data.size; ++i)
                    QCOMPARE(data.data()[i], reference.data()[i]);
                QCOMPARE(throwingTypeWatcher().destroyedIds.size(), (throwOnNthConstruction - 1));
                for (auto id : throwingTypeWatcher().destroyedIds)
                    QCOMPARE(id, 0);
            }
        }
    }

    // failed copy
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        const std::array<ThrowingType, 4> source = {
            ThrowingType(42), ThrowingType(43), ThrowingType(44), ThrowingType(170)
        };

        // copy range
        for (uint throwOnNthConstruction : {1, 3}) {
            WatcherScope scope; Q_UNUSED(scope);
            try {
                ThrowingType::throwOnce = throwOnNthConstruction;
                doConstruction(data, data.end(), [&source] (Constructor &ctor) {
                    return ctor.copy(source.begin(), source.end());
                });
            } catch (const std::runtime_error &e) {
                QCOMPARE(std::string(e.what()), ThrowingType::throwString);
                QCOMPARE(data.size, reference.size);
                for (qsizetype i = 0; i < data.size; ++i)
                    QCOMPARE(data.data()[i], reference.data()[i]);
                const auto destroyedSize = throwingTypeWatcher().destroyedIds.size();
                QCOMPARE(destroyedSize, (throwOnNthConstruction - 1));
                for (size_t i = 0; i < destroyedSize; ++i)
                    QCOMPARE(throwingTypeWatcher().destroyedIds[i], source[destroyedSize - i - 1]);
            }
        }

        // copy value
        for (uint throwOnNthConstruction : {1, 3}) {
            const ThrowingType value(512);
            QVERIFY(QArrayDataPointer<ThrowingType>::pass_parameter_by_value == false);
            WatcherScope scope; Q_UNUSED(scope);
            try {
                ThrowingType::throwOnce = throwOnNthConstruction;
                doConstruction(data, data.end(), [&value] (Constructor &ctor) {
                    return ctor.clone(5, value);
                });
            } catch (const std::runtime_error &e) {
                QCOMPARE(std::string(e.what()), ThrowingType::throwString);
                QCOMPARE(data.size, reference.size);
                for (qsizetype i = 0; i < data.size; ++i)
                    QCOMPARE(data.data()[i], reference.data()[i]);
                QCOMPARE(throwingTypeWatcher().destroyedIds.size(), (throwOnNthConstruction - 1));
                for (auto id : throwingTypeWatcher().destroyedIds)
                    QCOMPARE(id, 512);
            }
        }
    }

    // failed move
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        const std::array<ThrowingType, 4> source = {
            ThrowingType(42), ThrowingType(43), ThrowingType(44), ThrowingType(170)
        };

        for (uint throwOnNthConstruction : {1, 3}) {
            WatcherScope scope; Q_UNUSED(scope);
            try {
                ThrowingType::throwOnce = throwOnNthConstruction;
                doConstruction(data, data.end(), [source] (Constructor &ctor) mutable {
                    return ctor.move(source.begin(), source.end());
                });
            } catch (const std::runtime_error &e) {
                QCOMPARE(std::string(e.what()), ThrowingType::throwString);
                QCOMPARE(data.size, reference.size);
                for (qsizetype i = 0; i < data.size; ++i)
                    QCOMPARE(data.data()[i], reference.data()[i]);
                const auto destroyedSize = throwingTypeWatcher().destroyedIds.size();
                QCOMPARE(destroyedSize, (throwOnNthConstruction - 1));
                for (size_t i = 0; i < destroyedSize; ++i)
                    QCOMPARE(throwingTypeWatcher().destroyedIds[i], source[destroyedSize - i - 1]);
            }
        }
    }
}

void tst_QArrayData::exceptionSafetyPrimitives_destructor()
{
    using Prims = QtPrivate::QArrayExceptionSafetyPrimitives<ThrowingType>;
    using Destructor = typename Prims::Destructor<>;

    struct WatcherScope
    {
        WatcherScope() { throwingTypeWatcher().watch = true; }
        ~WatcherScope()
        {
            throwingTypeWatcher().watch = false;
            throwingTypeWatcher().destroyedIds.clear();
        }
    };

    // successful operation with no rollback, elements added from left to right
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->insert(reference.end(), 2, ThrowingType(42));

        WatcherScope scope; Q_UNUSED(scope);
        {
            auto where = data.end() - 1;
            Destructor destroyer(where);
            for (int i = 0; i < 2; ++i) {
                new (where + 1) ThrowingType(42);
                ++where;
                ++data.size;
            }
            destroyer.commit();
        }

        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
        QVERIFY(throwingTypeWatcher().destroyedIds.size() == 0);
    }

    // failed operation with rollback, elements added from left to right
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);

        WatcherScope scope; Q_UNUSED(scope);
        try {
            auto where = data.end() - 1;
            Destructor destroyer(where);
            for (int i = 0; i < 2; ++i) {
                new (where + 1) ThrowingType(42 + i);
                ++where;
                ThrowingType::throwOnce = 1;
            }
            QFAIL("Unreachable line!");
            destroyer.commit();
        } catch (const std::runtime_error &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
            QCOMPARE(data.size, reference.size);
            for (qsizetype i = 0; i < data.size; ++i)
                QCOMPARE(data.data()[i], reference.data()[i]);
            QVERIFY(throwingTypeWatcher().destroyedIds.size() == 1);
            QCOMPARE(throwingTypeWatcher().destroyedIds[0], 42);
        }
    }

    // successful operation with no rollback, elements added from right to left
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->erase(reference.begin(), reference.begin() + 2);
        reference->insert(reference.begin(), 2, ThrowingType(42));

        data.begin()->~ThrowingType();
        data.begin()->~ThrowingType();
        data.size -= 2;
        WatcherScope scope; Q_UNUSED(scope);
        {
            auto where = data.begin() + 2;  // Note: not updated data ptr, so begin + 2
            Destructor destroyer(where);
            for (int i = 0; i < 2; ++i) {
                new (where - 1) ThrowingType(42);
                --where;
                ++data.size;
            }
            destroyer.commit();
        }
        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
        QVERIFY(throwingTypeWatcher().destroyedIds.size() == 0);
    }

    // failed operation with rollback, elements added from right to left
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->erase(reference.begin(), reference.begin() + 2);

        data.begin()->~ThrowingType();
        data.begin()->~ThrowingType();
        data.size -= 2;
        WatcherScope scope; Q_UNUSED(scope);
        try {
            auto where = data.begin() + 2;  // Note: not updated data ptr, so begin + 2
            Destructor destroyer(where);
            for (int i = 0; i < 2; ++i) {
                new (where - 1) ThrowingType(42 + i);
                --where;
                ThrowingType::throwOnce = 1;
            }
            QFAIL("Unreachable line!");
            destroyer.commit();
        } catch (const std::runtime_error &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
            QCOMPARE(data.size, reference.size);
            for (qsizetype i = 0; i < data.size; ++i)
                QCOMPARE(data.data()[i + 2], reference.data()[i]);
            QVERIFY(throwingTypeWatcher().destroyedIds.size() == 1);
            QCOMPARE(throwingTypeWatcher().destroyedIds[0], 42);
        }
    }

    // extra: the very first operation throws - destructor has to do nothing,
    //        since nothing is properly constructed
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);

        WatcherScope scope; Q_UNUSED(scope);
        try {
            auto where = data.end() - 1;
            Destructor destroyer(where);
            ThrowingType::throwOnce = 1;
            new (where + 1) ThrowingType(42);
            ++where;
            QFAIL("Unreachable line!");
            destroyer.commit();
        } catch (const std::runtime_error &e) {
            QCOMPARE(data.size, reference.size);
            for (qsizetype i = 0; i < data.size; ++i)
                QCOMPARE(data.data()[i], reference.data()[i]);
            QVERIFY(throwingTypeWatcher().destroyedIds.size() == 0);
        }
    }

    // extra: special case when iterator is intentionally out of bounds: this is
    // to cover the case when we work on the uninitialized memory region instead
    // of being near the border
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->erase(reference.begin(), reference.begin() + 2);

        data.begin()->~ThrowingType();
        data.begin()->~ThrowingType();
        data.size -= 2;
        WatcherScope scope; Q_UNUSED(scope);
        try {
            auto where = data.begin() - 1;  // Note: intentionally out of range
            Destructor destroyer(where);
                for (int i = 0; i < 2; ++i) {
                new (where + 1) ThrowingType(42);
                ++where;
                ThrowingType::throwOnce = 1;
            }
            QFAIL("Unreachable line!");
            destroyer.commit();
        } catch (const std::runtime_error &e) {
            QCOMPARE(data.size, reference.size);
            for (qsizetype i = 0; i < data.size; ++i)
                QCOMPARE(data.data()[i + 2], reference.data()[i]);
            QVERIFY(throwingTypeWatcher().destroyedIds.size() == 1);
            QVERIFY(throwingTypeWatcher().destroyedIds[0] == 42);
        }
    }

    // extra: special case of freezing the position
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->erase(reference.end() - 1, reference.end());
        data.data()[data.size - 1] = ThrowingType(42);

        WatcherScope scope; Q_UNUSED(scope);
        {
            auto where = data.end();
            Destructor destroyer(where);
            for (int i = 0; i < 3; ++i) {
                --where;
                destroyer.freeze();
            }
        }
        --data.size;  // destroyed 1 element above
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
        QVERIFY(throwingTypeWatcher().destroyedIds.size() == 1);
        QCOMPARE(throwingTypeWatcher().destroyedIds[0], 42);
    }
}

void tst_QArrayData::exceptionSafetyPrimitives_mover()
{
    QVERIFY(QTypeInfo<ThrowingType>::isRelocatable);
    using Prims = QtPrivate::QArrayExceptionSafetyPrimitives<ThrowingType>;
    using Mover = typename Prims::Mover;

    const auto testMoveLeft = [] (size_t posB, size_t posE) {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);

        ThrowingType *b = data.begin() + posB;
        ThrowingType *e = data.begin() + posE;
        const auto originalSize = data.size;
        const auto length = std::distance(b, e);
        {
            Mover mover(e, static_cast<ThrowingType *>(data.end()) - e, data.size);
            while (e != b)
                (--e)->~ThrowingType();
        }
        QCOMPARE(data.size + length, originalSize);
        qsizetype i = 0;
        for (; i < std::distance(static_cast<ThrowingType *>(data.begin()), b); ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
        for (; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i + length]);
    };

    const auto testMoveRight = [] (size_t posB, size_t posE) {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);

        ThrowingType *begin = data.begin();
        ThrowingType *b = data.begin() + posB;
        ThrowingType *e = data.begin() + posE;
        const auto originalSize = data.size;
        const auto length = std::distance(b, e);
        {
            Mover mover(begin, b - static_cast<ThrowingType *>(data.begin()), data.size);
            while (b != e) {
                ++begin;
                (b++)->~ThrowingType();
            }
        }
        QCOMPARE(data.size + length, originalSize);

        // restore original data size
        {
            for (qsizetype i = 0; i < length; ++i) {
                new (static_cast<ThrowingType *>(data.begin() + i)) ThrowingType(42);
                ++data.size;
            }
        }

        qsizetype i = length;
        for (; i < std::distance(static_cast<ThrowingType *>(data.begin()), e); ++i)
            QCOMPARE(data.data()[i], reference.data()[i - length]);
        for (; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
    };

    // normal move left
    RUN_TEST_FUNC(testMoveLeft, 2, 4);
    // no move left
    RUN_TEST_FUNC(testMoveLeft, 2, 2);
    // normal move right
    RUN_TEST_FUNC(testMoveRight, 3, 5);
    // no move right
    RUN_TEST_FUNC(testMoveRight, 4, 4);
}

void tst_QArrayData::exceptionSafetyPrimitives_displacer()
{
    QVERIFY(QTypeInfo<ThrowingType>::isRelocatable);
    using Prims = QtPrivate::QArrayExceptionSafetyPrimitives<ThrowingType>;
    const auto doDisplace = [] (auto &dataPointer, auto start, auto finish, qsizetype diff) {
        typename Prims::Displacer displace(start, finish, diff);
        new (start) ThrowingType(42);
        ++dataPointer.size;
        displace.commit();
    };

    // successful operation with displace to the right
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->insert(reference.end() - 1, 1, ThrowingType(42));

        auto where = data.end() - 1;
        doDisplace(data, where, data.end(), 1);

        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
    }

    // failed operation with displace to the right
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        try {
            ThrowingType::throwOnce = 1;
            doDisplace(data, data.end() - 1, data.end(), 1);
            QFAIL("Unreachable line!");
        } catch (const std::exception &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
            QCOMPARE(data.size, reference.size);
            for (qsizetype i = 0; i < data.size; ++i)
                QCOMPARE(data.data()[i], reference.data()[i]);
        }
    }

    // successful operation with displace to the left
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference.data()[0] = reference.data()[1];
        reference.data()[1] = ThrowingType(42);

        data.begin()->~ThrowingType();  // free space at begin
        --data.size;
        auto where = data.begin() + 1;
        doDisplace(data, where, where + 1, -1);

        QCOMPARE(data.size, reference.size);
        for (qsizetype i = 0; i < data.size; ++i)
            QCOMPARE(data.data()[i], reference.data()[i]);
    }

    // failed operation with displace to the left
    {
        auto data = createDataPointer<ThrowingType>(20, 10);
        auto reference = createDataPointer<ThrowingType>(20, 10);
        reference->erase(reference.begin(), reference.begin() + 1);

        try {
            data.begin()->~ThrowingType();  // free space at begin
            --data.size;
            ThrowingType::throwOnce = 1;
            auto where = data.begin() + 1;
            doDisplace(data, where, where + 1, -1);
            QFAIL("Unreachable line!");
        } catch (const std::exception &e) {
            QCOMPARE(std::string(e.what()), ThrowingType::throwString);
            QCOMPARE(data.size, reference.size);
            for (qsizetype i = 0; i < data.size; ++i)
                QCOMPARE(data.data()[i + 1], reference.data()[i]);
        }
    }
}
#endif  // QT_NO_EXCEPTIONS

QTEST_APPLESS_MAIN(tst_QArrayData)
#include "tst_qarraydata.moc"
