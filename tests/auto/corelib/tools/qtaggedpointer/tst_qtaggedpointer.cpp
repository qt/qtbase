// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qtaggedpointer.h>

class tst_QTaggedPointer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constExpr();
    void construction();
    void assignment();
    void dereferenceOperator();
    void pointerOperator();
    void negationOperator();
    void operatorBool();
    void comparison();
    void tag();
    void objectMember();
    void customTagType();
    void taggedLinkedList();
};

void tst_QTaggedPointer::constExpr()
{
    {
        constexpr QTaggedPointer<int> p;
        Q_UNUSED(p);
    }
    {
        enum Foo : uint {};
        constexpr QTaggedPointer<int, Foo> p;
        Q_UNUSED(p);
    }
    {
        enum Foo : int {};
        constexpr QTaggedPointer<int, Foo> p;
        Q_UNUSED(p);
    }
    {
        constexpr QTaggedPointer<int> p = nullptr;
        Q_UNUSED(p);
    }
    {
        enum Foo : uint {};
        constexpr QTaggedPointer<int, Foo> p = nullptr;
        Q_UNUSED(p);
    }
    {
        enum Foo : int {};
        constexpr QTaggedPointer<int, Foo> p = nullptr;
        Q_UNUSED(p);
    }
}

void tst_QTaggedPointer::construction()
{
    {
        QTaggedPointer<int> p;
        QCOMPARE(p.data(), nullptr);
        QVERIFY(!p.tag());
    }
    {
        QTaggedPointer<int> p(nullptr, 0x1);
        QCOMPARE(p.data(), nullptr);
        QCOMPARE(p.tag(), quintptr(0x1));
    }
    {
        QScopedPointer<int> rawPointer(new int(5));
        QTaggedPointer<int> p(rawPointer.data());
        QCOMPARE(p.data(), rawPointer.data());
        QVERIFY(!p.tag());
    }
    {
        QScopedPointer<int> rawPointer(new int(5));
        QTaggedPointer<int> p(rawPointer.data(), 0x1);
        QCOMPARE(p.data(), rawPointer.data());
        QCOMPARE(p.tag(), quintptr(0x1));
    }
}

void tst_QTaggedPointer::assignment()
{
    QScopedPointer<int> rawPointer(new int(5));
    QTaggedPointer<int> p(rawPointer.data(), 0x1);
    QTaggedPointer<int> p2(rawPointer.data(), 0x2);

    QCOMPARE(p.data(), rawPointer.data());
    QCOMPARE(p.tag(), quintptr(0x1));

    QCOMPARE(p2.data(), rawPointer.data());
    QCOMPARE(p2.tag(), quintptr(0x2));

    p = nullptr;
    QCOMPARE(p.data(), nullptr);
    QCOMPARE(p.tag(), quintptr(0x1));

    p = rawPointer.data();
    QCOMPARE(p.data(), rawPointer.data());
    QCOMPARE(p.tag(), quintptr(0x1));

    p = {};
    QCOMPARE(p.data(), nullptr);
    QCOMPARE(p.tag(), quintptr(0x0));

    p = p2;
    QCOMPARE(p.data(), rawPointer.data());
    QCOMPARE(p.tag(), quintptr(0x2));

    p = nullptr;
    QCOMPARE(p.data(), nullptr);
    QCOMPARE(p.tag(), quintptr(0x2));

    p = {};
    QCOMPARE(p.data(), nullptr);
    QCOMPARE(p.tag(), quintptr(0x0));

    p = rawPointer.data();
    QCOMPARE(p.data(), rawPointer.data());
    QCOMPARE(p.tag(), quintptr(0x0));
}

class AbstractClass
{
public:
    virtual ~AbstractClass() {}
    virtual int member() const = 0;
};

class SubClass : public AbstractClass
{
public:
    int member() const override { return 5; }
};

void tst_QTaggedPointer::dereferenceOperator()
{
    /* Dereference a basic value. */
    {
        QScopedPointer<int> rawPointer(new int(5));
        QTaggedPointer<int> p(rawPointer.data());
        const int value = *p;
        QCOMPARE(value, 5);
    }

    /* Dereference a basic value with tag. */
    {
        QScopedPointer<int> rawPointer(new int(5));
        QTaggedPointer<int> p(rawPointer.data(), 0x1);
        const int value = *p;
        QCOMPARE(value, 5);
    }

    /* Dereference a pointer to an abstract class. This verifies
     * that the operator returns a reference, when compiling
     * with MSVC 2005. */
    {
        QScopedPointer<SubClass> ptr(new SubClass());
        QTaggedPointer<AbstractClass> p(ptr.data());
        QCOMPARE((*p).member(), 5);
    }

    /* The operator should be const. */
    {
        QScopedPointer<int> rawPointer(new int(5));
        const QTaggedPointer<int> p(rawPointer.data());
        *p;
    }

    /* A reference should be returned, not a value. */
    {
        QScopedPointer<int> rawPointer(new int(5));
        const QTaggedPointer<int> p(rawPointer.data());
        Q_UNUSED(static_cast<int &>(*p));
    }

    /* Instantiated on a const object, the returned object is a const reference. */
    {
        QScopedPointer<int> rawPointer(new int(5));
        const QTaggedPointer<const int> p(rawPointer.data());
        Q_UNUSED(static_cast<const int &>(*p));
    }
}

class Value
{
public:
    int value;
};

void tst_QTaggedPointer::pointerOperator()
{
    {
        QScopedPointer<Value> valuePtr(new Value{5});
        QTaggedPointer<Value> p(valuePtr.data());
        QCOMPARE(p->value, 5);
    }

    {
        QScopedPointer<Value> valuePtr(new Value{5});
        QTaggedPointer<Value> p(valuePtr.data(), 0x1);
        QCOMPARE(p->value, 5);
    }

    /* The operator should be const. */
    {
        QScopedPointer<Value> valuePtr(new Value{5});
        const QTaggedPointer<Value> p(valuePtr.data());
        QVERIFY(p->value);
    }
}

void tst_QTaggedPointer::negationOperator()
{
    /* Invoke on default constructed value. */
    {
        QTaggedPointer<int> p;
        QVERIFY(!p);
    }

    /* Invoke on nullptr value with tag. */
    {
        QTaggedPointer<int> p(nullptr, 0x1);
        QVERIFY(!p);
    }

    /* Invoke on a value. */
    {
        QScopedPointer<int> rawPointer(new int(2));
        QTaggedPointer<int> p(rawPointer.data());
        QCOMPARE(!p, false);
    }

    /* Invoke on a value with tag. */
    {
        QScopedPointer<int> rawPointer(new int(2));
        QTaggedPointer<int> p(rawPointer.data(), 0x1);
        QCOMPARE(!p, false);
    }

    /* The signature should be const. */
    {
        const QTaggedPointer<int> p;
        !p;
    }

    /* The return value should be bool. */
    {
        const QTaggedPointer<int> p;
        Q_UNUSED(static_cast<bool>(!p));
    }
}

void tst_QTaggedPointer::operatorBool()
{
    /* Invoke on default constructed value. */
    {
        QTaggedPointer<int> p;
        QCOMPARE(bool(p), false);
    }

    /* Invoke on nullptr value with tag. */
    {
        QTaggedPointer<int> p(nullptr, 0x1);
        QCOMPARE(bool(p), false);
    }

    /* Invoke on active value. */
    {
        QScopedPointer<int> rawPointer(new int(3));
        QTaggedPointer<int> p(rawPointer.data());
        QVERIFY(p);
    }

    /* Invoke on active value with tag. */
    {
        QScopedPointer<int> rawPointer(new int(3));
        QTaggedPointer<int> p(rawPointer.data(), 0x1);
        QVERIFY(p);
    }

    /* The signature should be const and return bool. */
    {
        const QTaggedPointer<int> p;
        (void)static_cast<bool>(p);
    }
}

template <class A1, class A2, class B>
void comparisonTest(const A1 &a1, const A2 &a2, const B &b)
{
    // test equality on equal pointers
    QVERIFY(a1 == a2);
    QVERIFY(a2 == a1);

    // test inequality on equal pointers
    QVERIFY(!(a1 != a2));
    QVERIFY(!(a2 != a1));

    // test equality on unequal pointers
    QVERIFY(!(a1 == b));
    QVERIFY(!(a2 == b));
    QVERIFY(!(b == a1));
    QVERIFY(!(b == a2));

    // test inequality on unequal pointers
    QVERIFY(b != a1);
    QVERIFY(b != a2);
    QVERIFY(a1 != b);
    QVERIFY(a2 != b);
}

void tst_QTaggedPointer::comparison()
{
    QScopedPointer<int> a(new int(5));

    {
        QTaggedPointer<int> a1(a.data());
        QTaggedPointer<int> a2(a.data());
        QScopedPointer<int> rawPointer(new int(6));
        QTaggedPointer<int> b(rawPointer.data());

        comparisonTest(a1, a1, b);
        comparisonTest(a2, a2, b);
        comparisonTest(a1, a2, b);
    }
    {
        QTaggedPointer<int> a1(a.data(), 0x1);
        QTaggedPointer<int> a2(a.data(), 0x1);
        QScopedPointer<int> rawPointer(new int(6));
        QTaggedPointer<int> b(rawPointer.data(), 0x1);

        comparisonTest(a1, a1, b);
        comparisonTest(a2, a2, b);
        comparisonTest(a1, a2, b);
    }
    {
        QTaggedPointer<int> a1(a.data(), 0x1);
        QTaggedPointer<int> a2(a.data(), 0x2);
        QScopedPointer<int> rawPointer(new int(6));
        QTaggedPointer<int> b(rawPointer.data(), 0x2);

        comparisonTest(a1, a1, b);
        comparisonTest(a2, a2, b);
        comparisonTest(a1, a2, b);
    }
    {
        QTaggedPointer<int> p;
        QVERIFY(p.isNull());
        QVERIFY(p == nullptr);
        QVERIFY(nullptr == p);
    }
    {
        QTaggedPointer<int> p(nullptr, 0x1);
        QVERIFY(p.isNull());
        QVERIFY(p == nullptr);
        QVERIFY(nullptr == p);
    }
    {
        QScopedPointer<int> rawPointer(new int(42));
        QTaggedPointer<int> p(rawPointer.data());
        QVERIFY(!p.isNull());
        QVERIFY(p != nullptr);
        QVERIFY(nullptr != p);
    }
    {
        QScopedPointer<int> rawPointer(new int(42));
        QTaggedPointer<int> p(rawPointer.data(), 0x1);
        QVERIFY(!p.isNull());
        QVERIFY(p != nullptr);
        QVERIFY(nullptr != p);
    }
}

void tst_QTaggedPointer::tag()
{
    QScopedPointer<int> rawPointer(new int(3));
    QTaggedPointer<int> p(rawPointer.data());
    QCOMPARE(*p.data(), 3);
    QVERIFY(!p.tag());

    p.setTag(0x1);
    QCOMPARE(p.tag(), 0x1);

    p.setTag(0x2);
    QCOMPARE(p.tag(), 0x2);
}

struct Foo
{
    Foo() : p(nullptr) {}
    Foo(const Foo &other) : p(other.p) {}
    Foo &operator=(const Foo &other) {
        p = other.p;
        return *this;
    }
    QTaggedPointer<int> p;
};

void tst_QTaggedPointer::objectMember()
{
    QScopedPointer<int> rawPointer(new int(42));
    Foo f;
    f.p = QTaggedPointer<int>(rawPointer.data(), 0x1);

    Foo f2(f);
    QCOMPARE(f2.p.data(), f.p.data());
    QCOMPARE(f2.p.tag(), f.p.tag());

    Foo f3 = f;
    QCOMPARE(f3.p.data(), f.p.data());
    QCOMPARE(f3.p.tag(), f.p.tag());
}

class Bar
{
    Q_GADGET
public:
    enum Tag {
        NoTag = 0,
        FirstTag = 1,
        SecondTag = 2
    };
    Q_DECLARE_FLAGS(Tags, Tag)
    Q_FLAG(Tags)

    int value;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Bar::Tags)

void tst_QTaggedPointer::customTagType()
{
    QScopedPointer<Bar> barPtr(new Bar{5});
    typedef QTaggedPointer<Bar, Bar::Tags> TaggedBar;
    TaggedBar p(barPtr.data());
    QCOMPARE(p->value, 5);
    QVERIFY(TaggedBar::maximumTag());

    QVERIFY(!p.tag());
    QCOMPARE(p.tag(), Bar::NoTag);

    p.setTag(Bar::FirstTag | Bar::SecondTag);
    QCOMPARE(p->value, 5);
    QCOMPARE(p.tag(), Bar::FirstTag | Bar::SecondTag);
}

// Compile-only test to ensure it's possible to use tagged pointers
// with incomplete types.
struct LinkedListItem
{
    enum Tag {
        NoTag = 0,
        FirstTag = 1
    };
    Q_DECLARE_FLAGS(Tags, Tag)

    QTaggedPointer<LinkedListItem, Tag> next;

    ~LinkedListItem()
    {
        delete next.data();
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LinkedListItem::Tags)

void tst_QTaggedPointer::taggedLinkedList()
{
    QScopedPointer<LinkedListItem> lli(new LinkedListItem);
    lli->next = QTaggedPointer<LinkedListItem, LinkedListItem::Tag>(new LinkedListItem);
    lli->next.setTag(LinkedListItem::FirstTag);
}

QTEST_MAIN(tst_QTaggedPointer)
#include "tst_qtaggedpointer.moc"
