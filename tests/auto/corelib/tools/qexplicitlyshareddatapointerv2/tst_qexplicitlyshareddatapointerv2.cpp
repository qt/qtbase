// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qshareddata_impl.h>

class tst_QExplicitlySharedDataPointerv2 : public QObject
{
    Q_OBJECT

    template <typename T> using QESDP_V2 = QtPrivate::QExplicitlySharedDataPointerV2<T>;

private slots:
    void copyConstructor() const;
    void moveConstructor() const;
    void copyAssignment() const;
    void moveAssignment() const;
    void compare() const;
    void mutability() const;
    void data() const;
    void reset() const;
    void swap() const;
    void take() const;
};

class MyClass : public QSharedData
{
public:
    MyClass() = default;
    MyClass(int v) : m_value(v) { ref.ref(); };
    int m_value;
    void mutating() {}
    void notMutating() const {}
    MyClass &operator=(const MyClass &) { return *this; }
};

void tst_QExplicitlySharedDataPointerv2::copyConstructor() const
{
    const QESDP_V2<const MyClass> pointer(new MyClass());
    const QESDP_V2<const MyClass> copy(pointer);
    QCOMPARE_EQ(pointer, copy);
}

void tst_QExplicitlySharedDataPointerv2::moveConstructor() const
{
    QESDP_V2<const MyClass> pointer(new MyClass());
    const QESDP_V2<const MyClass> moved(std::move(pointer));
    QCOMPARE_NE(moved.data(), static_cast<MyClass *>(0));
    QCOMPARE_EQ(pointer.data(), static_cast<MyClass *>(0));
}

void tst_QExplicitlySharedDataPointerv2::copyAssignment() const
{
    const QESDP_V2<const MyClass> pointer(new MyClass());
    const QESDP_V2<const MyClass> copy = pointer;
    QCOMPARE_EQ(pointer, copy);
}

void tst_QExplicitlySharedDataPointerv2::moveAssignment() const
{
    QESDP_V2<const MyClass> pointer(new MyClass());
    const QESDP_V2<const MyClass> moved = std::move(pointer);
    QCOMPARE_NE(moved.data(), static_cast<MyClass *>(0));
    QCOMPARE_EQ(pointer.data(), static_cast<MyClass *>(0));
}

void tst_QExplicitlySharedDataPointerv2::compare() const
{
    const QESDP_V2<MyClass> ptr;
    const QESDP_V2<MyClass> ptr2;
    QCOMPARE_EQ(ptr.data(), static_cast<MyClass *>(0));
    QCOMPARE_EQ(ptr2.data(), static_cast<MyClass *>(0));
    QCOMPARE_EQ(ptr, ptr2);

    const QESDP_V2<MyClass> copy(ptr);
    QCOMPARE_EQ(ptr, copy);

    const QESDP_V2<MyClass> new_ptr(new MyClass());
    QCOMPARE_NE(new_ptr.data(), static_cast<MyClass *>(0));
    QCOMPARE_NE(ptr, new_ptr);

    std::array<MyClass, 3> myArray {MyClass(2), MyClass(1), MyClass(0)};
    const QESDP_V2<const MyClass> val0(&myArray[0]);
    const QESDP_V2<const MyClass> val1(&myArray[1]);
    QCOMPARE_NE(val1, val0);
}

void tst_QExplicitlySharedDataPointerv2::mutability() const
{
    /* Pointer itself is const & points to const type. */
    {
        const QESDP_V2<const MyClass> pointer(new MyClass());
        pointer->notMutating();
    }

    /* Pointer itself is mutable & points to const type. */
    {
        QESDP_V2<const MyClass> pointer(new MyClass());
        pointer->notMutating();
    }

    /* Pointer itself is const & points to mutable type. */
    {
        const QESDP_V2<MyClass> pointer(new MyClass());
        pointer->notMutating();
    }

    /* Pointer itself is mtable & points to mutable type. */
    {
        QESDP_V2<MyClass> pointer(new MyClass());
        pointer->notMutating();
        pointer->mutating();
        *pointer = MyClass();
    }
}

void tst_QExplicitlySharedDataPointerv2::data() const
{
    /* Check default value. */
    {
        QESDP_V2<const MyClass> pointer;
        QCOMPARE_EQ(pointer.data(), static_cast<const MyClass *>(0));
    }

    {
        const QESDP_V2<const MyClass> pointer(new MyClass());
        /* Check that this cast is possible. */
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }

    {
        QESDP_V2<const MyClass> pointer(new MyClass());
        /* Check that this cast is possible. */
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }

    {
        const QESDP_V2<MyClass> pointer(new MyClass());
        /* Check that this cast is possible. */
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }

    {
        QESDP_V2<MyClass> pointer(new MyClass());
        /* Check that these casts are possible. */
        Q_UNUSED(static_cast<MyClass *>(pointer.data()));
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }
}

void tst_QExplicitlySharedDataPointerv2::reset() const
{
    /* Reset a default constructed shared data object: reference count is equal to 0. */
    {
        QESDP_V2<MyClass> pointer;
        QCOMPARE_EQ(pointer.data(), static_cast<MyClass *>(0));

        pointer.reset();
        QCOMPARE_EQ(pointer.data(), static_cast<MyClass *>(0));
    }

    /* Reset a shared data object where the reference count is equal to 1. */
    {
        QESDP_V2<MyClass> pointer(new MyClass());
        QCOMPARE_NE(pointer.data(), static_cast<MyClass *>(0));

        pointer.reset();
        QCOMPARE_EQ(pointer.data(), static_cast<MyClass *>(0));
    }

    /* Reset a shared data object where the reference count is greater than 1. */
    {
        QESDP_V2<MyClass> pointer(new MyClass());
        QCOMPARE_NE(pointer.data(), static_cast<MyClass *>(0));
        QESDP_V2<MyClass> pointer2(pointer);
        QCOMPARE_NE(pointer2.data(), static_cast<MyClass *>(0));

        pointer.reset();
        QCOMPARE_EQ(pointer.data(), static_cast<MyClass *>(0));
        QCOMPARE_NE(pointer2.data(), static_cast<MyClass *>(0));
    }
}

void tst_QExplicitlySharedDataPointerv2::swap() const
{
    QESDP_V2<MyClass> p1(0), p2(new MyClass());
    QVERIFY(!p1.data());
    QVERIFY(p2.data());

    p1.swap(p2);
    QVERIFY(p1.data());
    QVERIFY(!p2.data());

    p1.swap(p2);
    QVERIFY(!p1.data());
    QVERIFY(p2.data());

    qSwap(p1, p2);
    QVERIFY(p1.data());
    QVERIFY(!p2.data());
}

void tst_QExplicitlySharedDataPointerv2::take() const
{
    QESDP_V2<MyClass> pointer(new MyClass());
    QCOMPARE_NE(pointer.data(), static_cast<MyClass *>(0));
    delete pointer.take();
    QCOMPARE_EQ(pointer.data(), static_cast<MyClass *>(0));
}

QTEST_MAIN(tst_QExplicitlySharedDataPointerv2)

#include "tst_qexplicitlyshareddatapointerv2.moc"
