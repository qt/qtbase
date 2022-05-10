// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QList>
#include <QTest>

#include <utility>

static const int N = 1000;

struct MyBase
{
    MyBase(int i_) : i(i_) { }

    MyBase(const MyBase &other) : i(other.i) { }

    MyBase &operator=(const MyBase &other)
    {
        i = other.i;
        return *this;
    }

    bool operator==(const MyBase &other) const
    { return i == other.i; }

protected:
    int i;
};

struct MyPrimitive : public MyBase
{
    MyPrimitive(int i_ = -1) : MyBase(i_) { }
    MyPrimitive(const MyPrimitive &other) : MyBase(other) { }
    MyPrimitive &operator=(const MyPrimitive &other)
    {
        MyBase::operator=(other);
        return *this;
    }
};

struct MyMovable : public MyBase
{
    MyMovable(int i_ = -1) : MyBase(i_) {}
};

struct MyComplex : public MyBase
{
    MyComplex(int i_ = -1) : MyBase(i_) {}
};

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(MyPrimitive, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(MyMovable, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(MyComplex, Q_COMPLEX_TYPE);

QT_END_NAMESPACE


class tst_QList: public QObject
{
    Q_OBJECT

    const int million = 1000000;
private Q_SLOTS:
    void removeAll_primitive_data();
    void removeAll_primitive() { removeAll_impl<MyPrimitive>(); }
    void removeAll_movable_data() { removeAll_primitive_data(); }
    void removeAll_movable() { removeAll_impl<MyMovable>(); }
    void removeAll_complex_data() { removeAll_primitive_data(); }
    void removeAll_complex() { removeAll_impl<MyComplex>(); }

    // append 1 element:
    void appendOne_int_data() const { commonBenchmark_data<int>(); }
    void appendOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void appendOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void appendOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void appendOne_QString_data() const { commonBenchmark_data<QString>(); }

    void appendOne_int() const { appendOne_impl<QList, int>(); } // QTBUG-87330
    void appendOne_primitive() const { appendOne_impl<QList, MyPrimitive>(); }
    void appendOne_movable() const { appendOne_impl<QList, MyMovable>(); }
    void appendOne_complex() const { appendOne_impl<QList, MyComplex>(); }
    void appendOne_QString() const { appendOne_impl<QList, QString>(); }

    // prepend 1 element:
    void prependOne_int_data() const { commonBenchmark_data<int>(); }
    void prependOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void prependOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void prependOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void prependOne_QString_data() const { commonBenchmark_data<QString>(); }

    void prependOne_int() const { prependOne_impl<QList, int>(); }
    void prependOne_primitive() const { prependOne_impl<QList, MyPrimitive>(); }
    void prependOne_movable() const { prependOne_impl<QList, MyMovable>(); }
    void prependOne_complex() const { prependOne_impl<QList, MyComplex>(); }
    void prependOne_QString() const { prependOne_impl<QList, QString>(); }

    // insert in middle 1 element (quadratic, slow):
    void midInsertOne_int_data() const { commonBenchmark_data<int>(million); }
    void midInsertOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(million); }
    void midInsertOne_movable_data() const { commonBenchmark_data<MyMovable>(million); }
    void midInsertOne_complex_data() const { commonBenchmark_data<MyComplex>(million / 10); }
    void midInsertOne_QString_data() const { commonBenchmark_data<QString>(million / 10); }

    void midInsertOne_int() const { midInsertOne_impl<QList, int>(); }
    void midInsertOne_primitive() const { midInsertOne_impl<QList, MyPrimitive>(); }
    void midInsertOne_movable() const { midInsertOne_impl<QList, MyMovable>(); }
    void midInsertOne_complex() const { midInsertOne_impl<QList, MyComplex>(); }
    void midInsertOne_QString() const { midInsertOne_impl<QList, QString>(); }

    // append/prepend 1 element - hard times for branch predictor:
    void appendPrependOne_int_data() const { commonBenchmark_data<int>(); }
    void appendPrependOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void appendPrependOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void appendPrependOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void appendPrependOne_QString_data() const { commonBenchmark_data<QString>(); }

    void appendPrependOne_int() const { appendPrependOne_impl<QList, int>(); }
    void appendPrependOne_primitive() const { appendPrependOne_impl<QList, MyPrimitive>(); }
    void appendPrependOne_movable() const { appendPrependOne_impl<QList, MyMovable>(); }
    void appendPrependOne_complex() const { appendPrependOne_impl<QList, MyComplex>(); }
    void appendPrependOne_QString() const { appendPrependOne_impl<QList, QString>(); }

    // prepend half elements, then appen another half:
    void prependAppendHalvesOne_int_data() const { commonBenchmark_data<int>(); }
    void prependAppendHalvesOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void prependAppendHalvesOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void prependAppendHalvesOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void prependAppendHalvesOne_QString_data() const { commonBenchmark_data<QString>(); }

    void prependAppendHalvesOne_int() const { prependAppendHalvesOne_impl<QList, int>(); }
    void prependAppendHalvesOne_primitive() const
    {
        prependAppendHalvesOne_impl<QList, MyPrimitive>();
    }
    void prependAppendHalvesOne_movable() const { prependAppendHalvesOne_impl<QList, MyMovable>(); }
    void prependAppendHalvesOne_complex() const { prependAppendHalvesOne_impl<QList, MyComplex>(); }
    void prependAppendHalvesOne_QString() const { prependAppendHalvesOne_impl<QList, QString>(); }

    // emplace in middle 1 element (quadratic, slow):
    void midEmplaceOne_int_data() const { commonBenchmark_data<int>(million); }
    void midEmplaceOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(million); }
    void midEmplaceOne_movable_data() const { commonBenchmark_data<MyMovable>(million); }
    void midEmplaceOne_complex_data() const { commonBenchmark_data<MyComplex>(million / 10); }
    void midEmplaceOne_QString_data() const { commonBenchmark_data<QString>(million / 10); }

    void midEmplaceOne_int() const { midEmplaceOne_impl<QList, int>(); }
    void midEmplaceOne_primitive() const { midEmplaceOne_impl<QList, MyPrimitive>(); }
    void midEmplaceOne_movable() const { midEmplaceOne_impl<QList, MyMovable>(); }
    void midEmplaceOne_complex() const { midEmplaceOne_impl<QList, MyComplex>(); }
    void midEmplaceOne_QString() const { midEmplaceOne_impl<QList, QString>(); }

    // remove from beginning in a general way
    void removeFirstGeneral_int_data() const { commonBenchmark_data<int>(); }
    void removeFirstGeneral_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void removeFirstGeneral_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void removeFirstGeneral_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void removeFirstGeneral_QString_data() const { commonBenchmark_data<QString>(); }

    void removeFirstGeneral_int() const { removeFirstGeneral_impl<QList, int>(); }
    void removeFirstGeneral_primitive() const { removeFirstGeneral_impl<QList, MyPrimitive>(); }
    void removeFirstGeneral_movable() const { removeFirstGeneral_impl<QList, MyMovable>(); }
    void removeFirstGeneral_complex() const { removeFirstGeneral_impl<QList, MyComplex>(); }
    void removeFirstGeneral_QString() const { removeFirstGeneral_impl<QList, QString>(); }

    // remove from beginning in a special way (using fast part of QList::removeFirst())
    void removeFirstSpecial_int_data() const { commonBenchmark_data<int>(); }
    void removeFirstSpecial_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void removeFirstSpecial_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void removeFirstSpecial_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void removeFirstSpecial_QString_data() const { commonBenchmark_data<QString>(); }

    void removeFirstSpecial_int() const { removeFirstSpecial_impl<QList, int>(); }
    void removeFirstSpecial_primitive() const { removeFirstSpecial_impl<QList, MyPrimitive>(); }
    void removeFirstSpecial_movable() const { removeFirstSpecial_impl<QList, MyMovable>(); }
    void removeFirstSpecial_complex() const { removeFirstSpecial_impl<QList, MyComplex>(); }
    void removeFirstSpecial_QString() const { removeFirstSpecial_impl<QList, QString>(); }

private:
    template <class T>
    void removeAll_impl() const;

    template<typename>
    void commonBenchmark_data(int max = 200000000) const;

    template<template<typename> typename, typename>
    void appendOne_impl() const;

    template<template<typename> typename, typename>
    void prependOne_impl() const;

    template<template<typename> typename, typename>
    void midInsertOne_impl() const;

    template<template<typename> typename, typename>
    void appendPrependOne_impl() const;

    template<template<typename> typename, typename>
    void prependAppendHalvesOne_impl() const;

    template<template<typename> typename, typename>
    void midEmplaceOne_impl() const;

    template<template<typename> typename, typename>
    void removeFirstGeneral_impl() const;

    template<template<typename> typename, typename>
    void removeFirstSpecial_impl() const;
};

template <class T>
void tst_QList::removeAll_impl() const
{
    QFETCH(QList<int>, i10);
    QFETCH(int, itemsToRemove);

    constexpr int valueToRemove = 5;

    QList<T> list;
    for (int i = 0; i < 10 * N; ++i) {
        T t(i10.at(i % 10));
        list.append(t);
    }

    T t(valueToRemove);

    qsizetype removedCount = 0; // make compiler happy by setting to 0
    QList<T> l;

    QBENCHMARK {
        l = list;
        removedCount = l.removeAll(t);
    }
    QCOMPARE(removedCount, itemsToRemove * N);
    QCOMPARE(l.size() + removedCount, list.size());
    QVERIFY(!l.contains(valueToRemove));
}

void tst_QList::removeAll_primitive_data()
{
    qRegisterMetaType<QList<int> >();

    QTest::addColumn<QList<int> >("i10");
    QTest::addColumn<int>("itemsToRemove");

    QTest::newRow("0%")   << QList<int>(10, 0) << 0;
    QTest::newRow("10%")  << (QList<int>() << 0 << 0 << 0 << 0 << 5 << 0 << 0 << 0 << 0 << 0) << 1;
    QTest::newRow("90%")  << (QList<int>() << 5 << 5 << 5 << 5 << 0 << 5 << 5 << 5 << 5 << 5) << 9;
    QTest::newRow("100%") << QList<int>(10, 5) << 10;
}

template<typename T>
void tst_QList::commonBenchmark_data(int max) const
{
    QTest::addColumn<int>("elemCount");

    const auto addRow = [](int count, const char *text) { QTest::newRow(text) << count; };

    const auto p = [](int i, const char *text) { return std::make_pair(i, text); };

    // cap at 20m elements to allow 5.15/6.0 coverage to be the same
    for (auto pair : { p(100, "100"), p(1000, "1k"), p(10000, "10k"), p(100000, "100k"),
                       p(1000000, "1m"), p(10000000, "10m"), p(20000000, "20m") }) {
        if (pair.first <= max)
            addRow(pair.first, pair.second);
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::appendOne_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container;
        auto lvalue = getValue();

        for (int i = 0; i < elemCount; ++i) {
            container.append(lvalue);
        }
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::prependOne_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container;
        auto lvalue = getValue();

        for (int i = 0; i < elemCount; ++i) {
            container.prepend(lvalue);
        }
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::midInsertOne_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container;
        auto lvalue = getValue();

        for (int i = 0; i < elemCount; ++i) {
            const int remainder = i % 2;
            // use insert(i, n, t) as insert(i, t) calls emplace (implementation
            // detail)
            container.insert(container.size() / 2 + remainder, 1, lvalue);
        }
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::appendPrependOne_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container;
        auto lvalue = getValue();

        for (int i = 0; i < elemCount; ++i) {
            if (i % 2 == 0) {
                container.append(lvalue);
            } else {
                container.prepend(lvalue);
            }
        }
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::prependAppendHalvesOne_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container;
        auto lvalue = getValue();

        for (int i = 0; i < elemCount / 2; ++i) {
            container.prepend(lvalue);
        }

        for (int i = elemCount / 2; i < elemCount; ++i) {
            container.append(lvalue);
        }
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::midEmplaceOne_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container;
        auto lvalue = getValue();

        for (int i = 0; i < elemCount; ++i) {
            const int remainder = i % 2;
            container.emplace(container.size() / 2 + remainder, lvalue);
        }
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::removeFirstGeneral_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container(elemCount, getValue());

        for (int i = 0; i < elemCount - 1; ++i) {
            container.remove(0, 1);
        }
    }
}

template<template<typename> typename Container, typename T>
void tst_QList::removeFirstSpecial_impl() const
{
    QFETCH(int, elemCount);
    constexpr auto getValue = []() { return T {}; };

    QBENCHMARK {
        Container<T> container(elemCount, getValue());

        for (int i = 0; i < elemCount; ++i) {
            container.removeFirst();
        }
    }
}

QTEST_APPLESS_MAIN(tst_QList)

#include "tst_bench_qlist.moc"
