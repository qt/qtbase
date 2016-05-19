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
#include <QtCore/QString>
#include <QtCore/qarraydata.h>

#include "simplevector.h"

struct SharedNullVerifier
{
    SharedNullVerifier()
    {
        Q_ASSERT(QArrayData::shared_null[0].ref.isStatic());
        Q_ASSERT(QArrayData::shared_null[0].ref.isShared());
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
        Q_ASSERT(QArrayData::shared_null[0].ref.isSharable());
#endif
    }
};

// This is meant to verify/ensure that shared_null is not being dynamically
// initialized and stays away from the order-of-static-initialization fiasco.
//
// Of course, if this was to fail, qmake and the build should have crashed and
// burned before we ever got to this point :-)
SharedNullVerifier globalInit;

class tst_QArrayData : public QObject
{
    Q_OBJECT

private slots:
    void referenceCounting();
    void sharedNullEmpty();
    void staticData();
    void simpleVector();
    void simpleVectorReserve_data();
    void simpleVectorReserve();
    void allocate_data();
    void allocate();
    void alignment_data();
    void alignment();
    void typedData();
    void gccBug43247();
    void arrayOps();
    void arrayOps2();
    void setSharable_data();
    void setSharable();
    void fromRawData_data();
    void fromRawData();
    void literals();
#if defined(Q_COMPILER_VARIADIC_MACROS) && defined(Q_COMPILER_LAMBDA)
    void variadicLiterals();
#endif
#ifdef Q_COMPILER_RVALUE_REFS
    void rValueReferences();
#endif
    void grow();
};

template <class T> const T &const_(const T &t) { return t; }

void tst_QArrayData::referenceCounting()
{
    {
        // Reference counting initialized to 1 (owned)
        QArrayData array = { { Q_BASIC_ATOMIC_INITIALIZER(1) }, 0, 0, 0, 0 };

        QCOMPARE(array.ref.atomic.load(), 1);

        QVERIFY(!array.ref.isStatic());
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
        QVERIFY(array.ref.isSharable());
#endif

        QVERIFY(array.ref.ref());
        QCOMPARE(array.ref.atomic.load(), 2);

        QVERIFY(array.ref.deref());
        QCOMPARE(array.ref.atomic.load(), 1);

        QVERIFY(array.ref.ref());
        QCOMPARE(array.ref.atomic.load(), 2);

        QVERIFY(array.ref.deref());
        QCOMPARE(array.ref.atomic.load(), 1);

        QVERIFY(!array.ref.deref());
        QCOMPARE(array.ref.atomic.load(), 0);

        // Now would be a good time to free/release allocated data
    }

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    {
        // Reference counting initialized to 0 (non-sharable)
        QArrayData array = { { Q_BASIC_ATOMIC_INITIALIZER(0) }, 0, 0, 0, 0 };

        QCOMPARE(array.ref.atomic.load(), 0);

        QVERIFY(!array.ref.isStatic());
        QVERIFY(!array.ref.isSharable());

        QVERIFY(!array.ref.ref());
        // Reference counting fails, data should be copied
        QCOMPARE(array.ref.atomic.load(), 0);

        QVERIFY(!array.ref.deref());
        QCOMPARE(array.ref.atomic.load(), 0);

        // Free/release data
    }
#endif

    {
        // Reference counting initialized to -1 (static read-only data)
        QArrayData array = { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, 0 };

        QCOMPARE(array.ref.atomic.load(), -1);

        QVERIFY(array.ref.isStatic());
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
        QVERIFY(array.ref.isSharable());
#endif

        QVERIFY(array.ref.ref());
        QCOMPARE(array.ref.atomic.load(), -1);

        QVERIFY(array.ref.deref());
        QCOMPARE(array.ref.atomic.load(), -1);

    }
}

void tst_QArrayData::sharedNullEmpty()
{
    QArrayData *null = const_cast<QArrayData *>(QArrayData::shared_null);
    QArrayData *empty = QArrayData::allocate(1, Q_ALIGNOF(QArrayData), 0);

    QVERIFY(null->ref.isStatic());
    QVERIFY(null->ref.isShared());

    QVERIFY(empty->ref.isStatic());
    QVERIFY(empty->ref.isShared());

    QCOMPARE(null->ref.atomic.load(), -1);
    QCOMPARE(empty->ref.atomic.load(), -1);

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    QVERIFY(null->ref.isSharable());
    QVERIFY(empty->ref.isSharable());
#endif

    QVERIFY(null->ref.ref());
    QVERIFY(empty->ref.ref());

    QCOMPARE(null->ref.atomic.load(), -1);
    QCOMPARE(empty->ref.atomic.load(), -1);

    QVERIFY(null->ref.deref());
    QVERIFY(empty->ref.deref());

    QCOMPARE(null->ref.atomic.load(), -1);
    QCOMPARE(empty->ref.atomic.load(), -1);

    QVERIFY(null != empty);

    QCOMPARE(null->size, 0);
    QCOMPARE(null->alloc, 0u);
    QCOMPARE(null->capacityReserved, 0u);

    QCOMPARE(empty->size, 0);
    QCOMPARE(empty->alloc, 0u);
    QCOMPARE(empty->capacityReserved, 0u);
}

void tst_QArrayData::staticData()
{
    QStaticArrayData<char, 10> charArray = {
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(char, 10),
        { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j' }
    };
    QStaticArrayData<int, 10> intArray = {
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(int, 10),
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }
    };
    QStaticArrayData<double, 10> doubleArray = {
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(double, 10),
        { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f }
    };

    QCOMPARE(charArray.header.size, 10);
    QCOMPARE(intArray.header.size, 10);
    QCOMPARE(doubleArray.header.size, 10);

    QCOMPARE(charArray.header.data(), reinterpret_cast<void *>(&charArray.data));
    QCOMPARE(intArray.header.data(), reinterpret_cast<void *>(&intArray.data));
    QCOMPARE(doubleArray.header.data(), reinterpret_cast<void *>(&doubleArray.data));
}

void tst_QArrayData::simpleVector()
{
    QArrayData data0 = { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, 0 };
    QStaticArrayData<int, 7> data1 = {
            Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(int, 7),
            { 0, 1, 2, 3, 4, 5, 6 }
        };

    int array[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    SimpleVector<int> v1;
    SimpleVector<int> v2(v1);
    SimpleVector<int> v3(static_cast<QTypedArrayData<int> *>(&data0));
    SimpleVector<int> v4(static_cast<QTypedArrayData<int> *>(&data1.header));
    SimpleVector<int> v5(static_cast<QTypedArrayData<int> *>(&data0));
    SimpleVector<int> v6(static_cast<QTypedArrayData<int> *>(&data1.header));
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
    QVERIFY(v5.isEmpty());
    QVERIFY(!v6.isEmpty());
    QVERIFY(!v7.isEmpty());
    QVERIFY(!v8.isEmpty());

    QCOMPARE(v1.size(), size_t(0));
    QCOMPARE(v2.size(), size_t(0));
    QCOMPARE(v3.size(), size_t(0));
    QCOMPARE(v4.size(), size_t(0));
    QCOMPARE(v5.size(), size_t(0));
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

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    QVERIFY(v1.isSharable());
    QVERIFY(v2.isSharable());
    QVERIFY(v3.isSharable());
    QVERIFY(v4.isSharable());
    QVERIFY(v5.isSharable());
    QVERIFY(v6.isSharable());
    QVERIFY(v7.isSharable());
    QVERIFY(v8.isSharable());
#endif

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
    QVERIFY(v1 == v5);
    QVERIFY(!(v1 == v6));

    QVERIFY(v1 != v6);
    QVERIFY(v4 != v6);
    QVERIFY(v5 != v6);
    QVERIFY(!(v1 != v5));

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

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    {
        v7.setSharable(true);
        QVERIFY(v7.isSharable());

        SimpleVector<int> copy1(v7);
        QVERIFY(copy1.isSharedWith(v7));

        v7.setSharable(false);
        QVERIFY(!v7.isSharable());

        QVERIFY(!copy1.isSharedWith(v7));
        QCOMPARE(v7.size(), copy1.size());
        for (size_t i = 0; i < copy1.size(); ++i)
            QCOMPARE(v7[i], copy1[i]);

        SimpleVector<int> clone(v7);
        QVERIFY(!clone.isSharedWith(v7));
        QCOMPARE(clone.size(), copy1.size());
        for (size_t i = 0; i < copy1.size(); ++i)
            QCOMPARE(clone[i], copy1[i]);

        v7.setSharable(true);
        QVERIFY(v7.isSharable());

        SimpleVector<int> copy2(v7);
        QVERIFY(copy2.isSharedWith(v7));
    }

    {
        SimpleVector<int> null;
        SimpleVector<int> empty(0, 5);

        QVERIFY(null.isSharable());
        QVERIFY(empty.isSharable());

        null.setSharable(true);
        empty.setSharable(true);

        QVERIFY(null.isSharable());
        QVERIFY(empty.isSharable());

        QVERIFY(null.isEmpty());
        QVERIFY(empty.isEmpty());

        null.setSharable(false);
        empty.setSharable(false);

        QVERIFY(!null.isSharable());
        QVERIFY(!empty.isSharable());

        QVERIFY(null.isEmpty());
        QVERIFY(empty.isEmpty());

        null.setSharable(true);
        empty.setSharable(true);

        QVERIFY(null.isSharable());
        QVERIFY(empty.isSharable());

        QVERIFY(null.isEmpty());
        QVERIFY(empty.isEmpty());
    }
#endif
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

    static const QStaticArrayData<int, 15> array = {
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(int, 15),
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } };
    QArrayDataPointerRef<int> p = {
         static_cast<QTypedArrayData<int> *>(
            const_cast<QArrayData *>(&array.header)) };

    QTest::newRow("static") << SimpleVector<int>(p) << size_t(0) << size_t(15);
    QTest::newRow("raw-data") << SimpleVector<int>::fromRawData(array.data, 15) << size_t(0) << size_t(15);
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
    QVector<QArrayData *> headers;
};

Q_DECLARE_METATYPE(const QArrayData *)
Q_DECLARE_METATYPE(QArrayData::AllocationOptions)

void tst_QArrayData::allocate_data()
{
    QTest::addColumn<size_t>("objectSize");
    QTest::addColumn<size_t>("alignment");
    QTest::addColumn<QArrayData::AllocationOptions>("allocateOptions");
    QTest::addColumn<bool>("isCapacityReserved");
    QTest::addColumn<bool>("isSharable");       // ### Qt6: remove
    QTest::addColumn<const QArrayData *>("commonEmpty");

    struct {
        char const *typeName;
        size_t objectSize;
        size_t alignment;
    } types[] = {
        { "char", sizeof(char), Q_ALIGNOF(char) },
        { "short", sizeof(short), Q_ALIGNOF(short) },
        { "void *", sizeof(void *), Q_ALIGNOF(void *) }
    };

    QArrayData *shared_empty = QArrayData::allocate(0, Q_ALIGNOF(QArrayData), 0);
    QVERIFY(shared_empty);

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    QArrayData *unsharable_empty = QArrayData::allocate(0, Q_ALIGNOF(QArrayData), 0, QArrayData::Unsharable);
    QVERIFY(unsharable_empty);
#endif

    struct {
        char const *description;
        QArrayData::AllocationOptions allocateOptions;
        bool isCapacityReserved;
        bool isSharable;
        const QArrayData *commonEmpty;
    } options[] = {
        { "Default", QArrayData::Default, false, true, shared_empty },
        { "Reserved", QArrayData::CapacityReserved, true, true, shared_empty },
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
        { "Reserved | Unsharable",
            QArrayData::CapacityReserved | QArrayData::Unsharable, true, false,
            unsharable_empty },
        { "Unsharable", QArrayData::Unsharable, false, false, unsharable_empty },
#endif
        { "Grow", QArrayData::Grow, false, true, shared_empty }
    };

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i)
        for (size_t j = 0; j < sizeof(options)/sizeof(options[0]); ++j)
            QTest::newRow(qPrintable(
                        QLatin1String(types[i].typeName)
                        + QLatin1String(": ")
                        + QLatin1String(options[j].description)))
                << types[i].objectSize << types[i].alignment
                << options[j].allocateOptions << options[j].isCapacityReserved
                << options[j].isSharable << options[j].commonEmpty;
}

void tst_QArrayData::allocate()
{
    QFETCH(size_t, objectSize);
    QFETCH(size_t, alignment);
    QFETCH(QArrayData::AllocationOptions, allocateOptions);
    QFETCH(bool, isCapacityReserved);
    QFETCH(const QArrayData *, commonEmpty);

    // Minimum alignment that can be requested is that of QArrayData.
    // Typically, this alignment is sizeof(void *) and ensured by malloc.
    size_t minAlignment = qMax(alignment, Q_ALIGNOF(QArrayData));

    // Shared Empty
    QCOMPARE(QArrayData::allocate(objectSize, minAlignment, 0,
                QArrayData::AllocationOptions(allocateOptions)), commonEmpty);

    Deallocator keeper(objectSize, minAlignment);
    keeper.headers.reserve(1024);

    for (int capacity = 1; capacity <= 1024; capacity <<= 1) {
        QArrayData *data = QArrayData::allocate(objectSize, minAlignment,
                capacity, QArrayData::AllocationOptions(allocateOptions));
        keeper.headers.append(data);

        QCOMPARE(data->size, 0);
        if (allocateOptions & QArrayData::Grow)
            QVERIFY(data->alloc > uint(capacity));
        else
            QCOMPARE(data->alloc, uint(capacity));
        QCOMPARE(data->capacityReserved, uint(isCapacityReserved));
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
        QFETCH(bool, isSharable);
        QCOMPARE(data->ref.isSharable(), isSharable);
#endif

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(data->data(), 'A', objectSize * capacity);
    }
}

class Unaligned
{
    char dummy[8];
};

void tst_QArrayData::alignment_data()
{
    QTest::addColumn<size_t>("alignment");

    for (int i = 1; i < 10; ++i) {
        size_t alignment = 1u << i;
        QTest::newRow(qPrintable(QString::number(alignment))) << alignment;
    }
}

void tst_QArrayData::alignment()
{
    QFETCH(size_t, alignment);

    // Minimum alignment that can be requested is that of QArrayData.
    // Typically, this alignment is sizeof(void *) and ensured by malloc.
    size_t minAlignment = qMax(alignment, Q_ALIGNOF(QArrayData));

    Deallocator keeper(sizeof(Unaligned), minAlignment);
    keeper.headers.reserve(100);

    for (int i = 0; i < 100; ++i) {
        QArrayData *data = QArrayData::allocate(sizeof(Unaligned),
                minAlignment, 8, QArrayData::Default);
        keeper.headers.append(data);

        QVERIFY(data);
        QCOMPARE(data->size, 0);
        QVERIFY(data->alloc >= uint(8));

        // These conditions should hold as long as header and array are
        // allocated together
        QVERIFY(data->offset >= qptrdiff(sizeof(QArrayData)));
        QVERIFY(data->offset <= qptrdiff(sizeof(QArrayData)
                    + minAlignment - Q_ALIGNOF(QArrayData)));

        // Data is aligned
        QCOMPARE(quintptr(quintptr(data->data()) % alignment), quintptr(0u));

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(data->data(), 'A', sizeof(Unaligned) * 8);
    }
}

void tst_QArrayData::typedData()
{
    QStaticArrayData<int, 10> data = {
            Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(int, 10),
            { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }
        };
    QCOMPARE(data.header.size, 10);

    {
        QTypedArrayData<int> *array =
            static_cast<QTypedArrayData<int> *>(&data.header);
        QCOMPARE(array->data(), data.data);

        int j = 0;
        for (QTypedArrayData<int>::iterator iter = array->begin();
                iter != array->end(); ++iter, ++j)
            QCOMPARE((const int *)iter, data.data + j);
        QCOMPARE(j, 10);
    }

    {
        const QTypedArrayData<int> *array =
            static_cast<const QTypedArrayData<int> *>(&data.header);

        QCOMPARE(array->data(), data.data);

        int j = 0;
        for (QTypedArrayData<int>::const_iterator iter = array->begin();
                iter != array->end(); ++iter, ++j)
            QCOMPARE((const int *)iter, data.data + j);
        QCOMPARE(j, 10);
    }

    {
        QTypedArrayData<int> *null = QTypedArrayData<int>::sharedNull();
        QTypedArrayData<int> *empty = QTypedArrayData<int>::allocate(0);

        QVERIFY(null != empty);

        QCOMPARE(null->size, 0);
        QCOMPARE(empty->size, 0);

        QCOMPARE(null->begin(), null->end());
        QCOMPARE(empty->begin(), empty->end());
    }


    {
        Deallocator keeper(sizeof(char),
                Q_ALIGNOF(QTypedArrayData<char>::AlignmentDummy));
        QArrayData *array = QTypedArrayData<char>::allocate(10);
        keeper.headers.append(array);

        QVERIFY(array);
        QCOMPARE(array->size, 0);
        QCOMPARE(array->alloc, 10u);

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(array->data(), 0, 10 * sizeof(char));

        keeper.headers.clear();
        QTypedArrayData<short>::deallocate(array);

        QVERIFY(true);
    }

    {
        Deallocator keeper(sizeof(short),
                Q_ALIGNOF(QTypedArrayData<short>::AlignmentDummy));
        QArrayData *array = QTypedArrayData<short>::allocate(10);
        keeper.headers.append(array);

        QVERIFY(array);
        QCOMPARE(array->size, 0);
        QCOMPARE(array->alloc, 10u);

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(array->data(), 0, 10 * sizeof(short));

        keeper.headers.clear();
        QTypedArrayData<short>::deallocate(array);

        QVERIFY(true);
    }

    {
        Deallocator keeper(sizeof(double),
                Q_ALIGNOF(QTypedArrayData<double>::AlignmentDummy));
        QArrayData *array = QTypedArrayData<double>::allocate(10);
        keeper.headers.append(array);

        QVERIFY(array);
        QCOMPARE(array->size, 0);
        QCOMPARE(array->alloc, 10u);

        // Check that the allocated array can be used. Best tested with a
        // memory checker, such as valgrind, running.
        ::memset(array->data(), 0, 10 * sizeof(double));

        keeper.headers.clear();
        QTypedArrayData<double>::deallocate(array);

        QVERIFY(true);
    }
}

void tst_QArrayData::gccBug43247()
{
    // This test tries to verify QArrayData is not affected by GCC optimizer
    // bug #43247.
    // Reported on GCC 4.4.3, Linux, affects QVector

    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (3)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (4)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (5)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (6)");
    QTest::ignoreMessage(QtDebugMsg, "GCC Optimization bug #43247 not triggered (7)");

    SimpleVector<int> array(10, 0);
    // QVector<int> vector(10, 0);

    for (int i = 0; i < 10; ++i) {
        if (i >= 3 && i < 8)
            qDebug("GCC Optimization bug #43247 not triggered (%i)", i);

        // When access to data is implemented through an array of size 1, this
        // line lets the compiler assume i == 0, and the conditional above is
        // skipped.
        QVERIFY(array.at(i) == 0);
        // QVERIFY(vector.at(i) == 0);
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

size_t CountedObject::liveCount = 0;

void tst_QArrayData::arrayOps()
{
    CountedObject::LeakChecker leakChecker; Q_UNUSED(leakChecker)

    const int intArray[5] = { 80, 101, 100, 114, 111 };
    const QString stringArray[5] = {
        QLatin1String("just"),
        QLatin1String("for"),
        QLatin1String("testing"),
        QLatin1String("a"),
        QLatin1String("vector")
    };
    const CountedObject objArray[5];

    QVERIFY(!QTypeInfo<int>::isComplex && !QTypeInfo<int>::isStatic);
    QVERIFY(QTypeInfo<QString>::isComplex && !QTypeInfo<QString>::isStatic);
    QVERIFY(QTypeInfo<CountedObject>::isComplex && QTypeInfo<CountedObject>::isStatic);

    QCOMPARE(CountedObject::liveCount, size_t(5));
    for (size_t i = 0; i < 5; ++i)
        QCOMPARE(objArray[i].id, i);

    ////////////////////////////////////////////////////////////////////////////
    // copyAppend (I)
    SimpleVector<int> vi(intArray, intArray + 5);
    SimpleVector<QString> vs(stringArray, stringArray + 5);
    SimpleVector<CountedObject> vo(objArray, objArray + 5);

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

    vi = SimpleVector<int>(5, referenceInt);
    vs = SimpleVector<QString>(5, referenceString);
    vo = SimpleVector<CountedObject>(5, referenceObject);

    QCOMPARE(vi.size(), size_t(5));
    QCOMPARE(vs.size(), size_t(5));
    QCOMPARE(vo.size(), size_t(5));

    QCOMPARE(CountedObject::liveCount, size_t(11));
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(vi[i], referenceInt);
        QVERIFY(vs[i].isSharedWith(referenceString));

        QCOMPARE(vo[i].id, referenceObject.id);
        QCOMPARE(int(vo[i].flags), CountedObject::CopyConstructed
                | CountedObject::DefaultConstructed);
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
        QCOMPARE(int(vo[i].flags), CountedObject::DefaultConstructed
                | CountedObject::CopyAssigned);
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

void tst_QArrayData::arrayOps2()
{
    CountedObject::LeakChecker leakChecker; Q_UNUSED(leakChecker)

    ////////////////////////////////////////////////////////////////////////////
    // appendInitialize
    SimpleVector<int> vi(5);
    SimpleVector<QString> vs(5);
    SimpleVector<CountedObject> vo(5);

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

Q_DECLARE_METATYPE(QArrayDataPointer<int>)

static inline bool arrayIsFilledWith(const QArrayDataPointer<int> &array,
        int fillValue, size_t size)
{
    const int *iter = array->begin();
    const int *const end = array->end();

    for (size_t i = 0; i < size; ++i, ++iter)
        if (*iter != fillValue)
            return false;

    if (iter != end)
        return false;

    return true;
}

void tst_QArrayData::setSharable_data()
{
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    QTest::addColumn<QArrayDataPointer<int> >("array");
    QTest::addColumn<size_t>("size");
    QTest::addColumn<size_t>("capacity");
    QTest::addColumn<bool>("isCapacityReserved");
    QTest::addColumn<int>("fillValue");

    QArrayDataPointer<int> null;
    QArrayDataPointer<int> empty; empty.clear();

    static QStaticArrayData<int, 10> staticArrayData = {
            Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(int, 10),
            { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }
        };

    QArrayDataPointer<int> emptyReserved(QTypedArrayData<int>::allocate(5,
                QArrayData::CapacityReserved));
    QArrayDataPointer<int> nonEmpty(QTypedArrayData<int>::allocate(5,
                QArrayData::Default));
    QArrayDataPointer<int> nonEmptyExtraCapacity(
            QTypedArrayData<int>::allocate(10, QArrayData::Default));
    QArrayDataPointer<int> nonEmptyReserved(QTypedArrayData<int>::allocate(15,
                QArrayData::CapacityReserved));
    QArrayDataPointer<int> staticArray(
            static_cast<QTypedArrayData<int> *>(&staticArrayData.header));
    QArrayDataPointer<int> rawData(
            QTypedArrayData<int>::fromRawData(staticArrayData.data, 10));

    nonEmpty->copyAppend(5, 1);
    nonEmptyExtraCapacity->copyAppend(5, 1);
    nonEmptyReserved->copyAppend(7, 2);

    QTest::newRow("shared-null") << null << size_t(0) << size_t(0) << false << 0;
    QTest::newRow("shared-empty") << empty << size_t(0) << size_t(0) << false << 0;
    // unsharable-empty implicitly tested in shared-empty
    QTest::newRow("empty-reserved") << emptyReserved << size_t(0) << size_t(5) << true << 0;
    QTest::newRow("non-empty") << nonEmpty << size_t(5) << size_t(5) << false << 1;
    QTest::newRow("non-empty-extra-capacity") << nonEmptyExtraCapacity << size_t(5) << size_t(10) << false << 1;
    QTest::newRow("non-empty-reserved") << nonEmptyReserved << size_t(7) << size_t(15) << true << 2;
    QTest::newRow("static-array") << staticArray << size_t(10) << size_t(0) << false << 3;
    QTest::newRow("raw-data") << rawData << size_t(10) << size_t(0) << false << 3;
#endif
}

void tst_QArrayData::setSharable()
{
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    QFETCH(QArrayDataPointer<int>, array);
    QFETCH(size_t, size);
    QFETCH(size_t, capacity);
    QFETCH(bool, isCapacityReserved);
    QFETCH(int, fillValue);

    QVERIFY(array->ref.isShared()); // QTest has a copy
    QVERIFY(array->ref.isSharable());

    QCOMPARE(size_t(array->size), size);
    QCOMPARE(size_t(array->alloc), capacity);
    QCOMPARE(bool(array->capacityReserved), isCapacityReserved);
    QVERIFY(arrayIsFilledWith(array, fillValue, size));

    // shared-null becomes shared-empty, may otherwise detach
    array.setSharable(true);

    QVERIFY(array->ref.isSharable());
    QVERIFY(arrayIsFilledWith(array, fillValue, size));

    {
        QArrayDataPointer<int> copy(array);
        QVERIFY(array->ref.isShared());
        QVERIFY(array->ref.isSharable());
        QCOMPARE(copy.data(), array.data());
    }

    // Unshare, must detach
    array.setSharable(false);

    // Immutability (alloc == 0) is lost on detach, as is additional capacity
    // if capacityReserved flag is not set.
    if ((capacity == 0 && size != 0)
            || (!isCapacityReserved && capacity > size))
        capacity = size;

    QVERIFY(!array->ref.isShared());
    QVERIFY(!array->ref.isSharable());

    QCOMPARE(size_t(array->size), size);
    QCOMPARE(size_t(array->alloc), capacity);
    QCOMPARE(bool(array->capacityReserved), isCapacityReserved);
    QVERIFY(arrayIsFilledWith(array, fillValue, size));

    {
        QArrayDataPointer<int> copy(array);
        QVERIFY(!array->ref.isShared());
        QVERIFY(!array->ref.isSharable());

        // Null/empty is always shared
        QCOMPARE(copy->ref.isShared(), !(size || isCapacityReserved));
        QVERIFY(copy->ref.isSharable());

        QCOMPARE(size_t(copy->size), size);
        QCOMPARE(size_t(copy->alloc), capacity);
        QCOMPARE(bool(copy->capacityReserved), isCapacityReserved);
        QVERIFY(arrayIsFilledWith(copy, fillValue, size));
    }

    // Make sharable, again
    array.setSharable(true);

    QCOMPARE(array->ref.isShared(), !(size || isCapacityReserved));
    QVERIFY(array->ref.isSharable());

    QCOMPARE(size_t(array->size), size);
    QCOMPARE(size_t(array->alloc), capacity);
    QCOMPARE(bool(array->capacityReserved), isCapacityReserved);
    QVERIFY(arrayIsFilledWith(array, fillValue, size));

    {
        QArrayDataPointer<int> copy(array);
        QVERIFY(array->ref.isShared());
        QCOMPARE(copy.data(), array.data());
    }

    QCOMPARE(array->ref.isShared(), !(size || isCapacityReserved));
    QVERIFY(array->ref.isSharable());
#endif
}

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
                sizeof(array)/sizeof(array[0]), QArrayData::Default);

        QCOMPARE(raw.size(), size_t(11));
        QCOMPARE((const T *)raw.constBegin(), array);
        QCOMPARE((const T *)raw.constEnd(), (const T *)(array + sizeof(array)/sizeof(array[0])));

        QVERIFY(!raw.isShared());
        QVERIFY(SimpleVector<T>(raw).isSharedWith(raw));
        QVERIFY(!raw.isShared());

        // Detach
        QCOMPARE(raw.back(), T(11));
        QVERIFY((const T *)raw.constBegin() != array);
    }

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    {
        // Immutable, unsharable
        SimpleVector<T> raw = SimpleVector<T>::fromRawData(array,
                sizeof(array)/sizeof(array[0]), QArrayData::Unsharable);

        QCOMPARE(raw.size(), size_t(11));
        QCOMPARE((const T *)raw.constBegin(), array);
        QCOMPARE((const T *)raw.constEnd(), (const T *)(array + sizeof(array)/sizeof(array[0])));

        SimpleVector<T> copy(raw);
        QVERIFY(!copy.isSharedWith(raw));
        QVERIFY(!raw.isShared());

        QCOMPARE(copy.size(), size_t(11));

        for (size_t i = 0; i < 11; ++i) {
            QCOMPARE(const_(copy)[i], const_(raw)[i]);
            QCOMPARE(const_(copy)[i], T(i + 1));
        }

        QCOMPARE(raw.size(), size_t(11));
        QCOMPARE((const T *)raw.constBegin(), array);
        QCOMPARE((const T *)raw.constEnd(), (const T *)(array + sizeof(array)/sizeof(array[0])));

        // Detach
        QCOMPARE(raw.back(), T(11));
        QVERIFY((const T *)raw.constBegin() != array);
    }
#endif
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
        QCOMPARE(d->size, 10 + 1);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d->data()[i], char('A' + i));
    }

    {
        // wchar_t is not necessarily 2-bytes
        QArrayDataPointer<wchar_t> d = Q_ARRAY_LITERAL(wchar_t, L"ABCDEFGHIJ");
        QCOMPARE(d->size, 10 + 1);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d->data()[i], wchar_t('A' + i));
    }

    {
        SimpleVector<char> v = Q_ARRAY_LITERAL(char, "ABCDEFGHIJ");

        QVERIFY(!v.isNull());
        QVERIFY(!v.isEmpty());
        QCOMPARE(v.size(), size_t(11));
        // v.capacity() is unspecified, for now

#if defined(Q_COMPILER_VARIADIC_MACROS) && defined(Q_COMPILER_LAMBDA)
        QVERIFY(v.isStatic());
#endif

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
        QVERIFY(v.isSharable());
#endif
        QCOMPARE((void*)(const char*)(v.constBegin() + v.size()), (void*)(const char*)v.constEnd());

        for (int i = 0; i < 10; ++i)
            QCOMPARE(const_(v)[i], char('A' + i));
        QCOMPARE(const_(v)[10], char('\0'));
    }
}

#if defined(Q_COMPILER_VARIADIC_MACROS) && defined(Q_COMPILER_LAMBDA)
// Variadic Q_ARRAY_LITERAL need to be available in the current configuration.
void tst_QArrayData::variadicLiterals()
{
    {
        QArrayDataPointer<int> d =
            Q_ARRAY_LITERAL(int, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
        QCOMPARE(d->size, 10);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d->data()[i], i);
    }

    {
        QArrayDataPointer<char> d = Q_ARRAY_LITERAL(char,
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J');
        QCOMPARE(d->size, 10);
        for (int i = 0; i < 10; ++i)
            QCOMPARE(d->data()[i], char('A' + i));
    }

    {
        QArrayDataPointer<const char *> d = Q_ARRAY_LITERAL(const char *,
                "A", "B", "C", "D", "E", "F", "G", "H", "I", "J");
        QCOMPARE(d->size, 10);
        for (int i = 0; i < 10; ++i) {
            QCOMPARE(d->data()[i][0], char('A' + i));
            QCOMPARE(d->data()[i][1], '\0');
        }
    }

    {
        SimpleVector<int> v = Q_ARRAY_LITERAL(int, 0, 1, 2, 3, 4, 5, 6);

        QVERIFY(!v.isNull());
        QVERIFY(!v.isEmpty());
        QCOMPARE(v.size(), size_t(7));
        // v.capacity() is unspecified, for now

        QVERIFY(v.isStatic());

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
        QVERIFY(v.isSharable());
#endif
        QCOMPARE((const int *)(v.constBegin() + v.size()), (const int *)v.constEnd());

        for (int i = 0; i < 7; ++i)
            QCOMPARE(const_(v)[i], i);
    }
}
#endif

#ifdef Q_COMPILER_RVALUE_REFS
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
        Q_DECL_CONSTEXPR DetectConstructor()
            : constructor(DefaultConstructor)
        {
        }

        Q_DECL_CONSTEXPR DetectConstructor(const DetectConstructor &)
            : constructor(CopyConstructor)
        {
        }

        Q_DECL_CONSTEXPR DetectConstructor(DetectConstructor &&)
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
#endif

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

QTEST_APPLESS_MAIN(tst_QArrayData)
#include "tst_qarraydata.moc"
