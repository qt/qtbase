// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QtCore/QSharedData>

/*!
 \class tst_QExplicitlySharedDataPointer
 \internal
 \since 4.4
 \brief Tests class QExplicitlySharedDataPointer.

 */
class tst_QExplicitlySharedDataPointer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pointerOperatorOnConst() const;
    void pointerOperatorOnMutable() const;
    void copyConstructor() const;
    void clone() const;
    void data() const;
    void reset() const;
    void swap() const;
};

class MyClass : public QSharedData
{
public:
    void mutating()
    {
    }

    void notMutating() const
    {
    }

    MyClass &operator=(const MyClass &)
    {
        return *this;
    }
};

class Base : public QSharedData
{
public:
    virtual ~Base() { }
    virtual Base *clone() { return new Base(*this); }
    virtual bool isBase() const { return true; }
};

class Derived : public Base
{
public:
    virtual Base *clone() override { return new Derived(*this); }
    virtual bool isBase() const override { return false; }
};

QT_BEGIN_NAMESPACE
template<> Base *QExplicitlySharedDataPointer<Base>::clone()
{
    return d->clone();
}
QT_END_NAMESPACE

void tst_QExplicitlySharedDataPointer::pointerOperatorOnConst() const
{
    /* Pointer itself is const. */
    {
        const QExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer->notMutating();
    }

    /* Pointer itself is mutable. */
    {
        QExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer->notMutating();
    }
}

void tst_QExplicitlySharedDataPointer::pointerOperatorOnMutable() const
{
    /* Pointer itself is const. */
    {
        const QExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer->notMutating();
        pointer->mutating();
        *pointer = MyClass();
    }

    /* Pointer itself is mutable. */
    {
        const QExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer->notMutating();
        pointer->mutating();
        *pointer = MyClass();
    }
}

void tst_QExplicitlySharedDataPointer::copyConstructor() const
{
    const QExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
    const QExplicitlySharedDataPointer<const MyClass> copy(pointer);
}

void tst_QExplicitlySharedDataPointer::clone() const
{
    /* holding a base element */
    {
        QExplicitlySharedDataPointer<Base> pointer(new Base);
        QVERIFY(pointer->isBase());

        QExplicitlySharedDataPointer<Base> copy(pointer);
        pointer.detach();
        QVERIFY(pointer->isBase());
    }

    /* holding a derived element */
    {
        QExplicitlySharedDataPointer<Base> pointer(new Derived);
        QVERIFY(!pointer->isBase());

        QExplicitlySharedDataPointer<Base> copy(pointer);
        pointer.detach();
        QVERIFY(!pointer->isBase());
    }
}

void tst_QExplicitlySharedDataPointer::data() const
{
    /* Check default value. */
    {
        QExplicitlySharedDataPointer<const MyClass> pointer;
        QCOMPARE(pointer.data(), static_cast<const MyClass *>(0));
        QVERIFY(pointer == nullptr);
        QVERIFY(nullptr == pointer);
    }

    /* On const pointer. Must not mutate the pointer. */
    {
        const QExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer.data();

        /* Check that this cast is possible. */
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));

        QVERIFY(! (pointer == nullptr));
        QVERIFY(! (nullptr == pointer));
    }

    /* On mutatable pointer. Must not mutate the pointer. */
    {
        QExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer.data();

        /* Check that this cast is possible. */
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }

    /* Must not mutate the pointer. */
    {
        const QExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer.data();

        /* Check that these casts are possible. */
        Q_UNUSED(static_cast<MyClass *>(pointer.data()));
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }

    /* Must not mutate the pointer. */
    {
        QExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer.data();

        /* Check that these casts are possible. */
        Q_UNUSED(static_cast<MyClass *>(pointer.data()));
        Q_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }
}

void tst_QExplicitlySharedDataPointer::reset() const
{
    /* Do reset on a single ref count. */
    {
        QExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        QVERIFY(pointer.data() != 0);

        pointer.reset();
        QCOMPARE(pointer.data(), static_cast<MyClass *>(0));
    }

    /* Do reset on a default constructed object. */
    {
        QExplicitlySharedDataPointer<MyClass> pointer;
        QCOMPARE(pointer.data(), static_cast<MyClass *>(0));

        pointer.reset();
        QCOMPARE(pointer.data(), static_cast<MyClass *>(0));
    }
}

void tst_QExplicitlySharedDataPointer::swap() const
{
    QExplicitlySharedDataPointer<MyClass> p1(0), p2(new MyClass());
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

QTEST_MAIN(tst_QExplicitlySharedDataPointer)

#include "tst_qexplicitlyshareddatapointer.moc"
