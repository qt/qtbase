/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QList>
#include <QTest>

#include <utility>

static const int N = 1000;

struct MyBase
{
    MyBase(int i_)
        : isCopy(false)
    {
        ++liveCount;

        i = i_;
    }

    MyBase(const MyBase &other)
        : isCopy(true)
    {
        if (isCopy)
            ++copyCount;
        ++liveCount;

        i = other.i;
    }

    MyBase &operator=(const MyBase &other)
    {
        if (!isCopy) {
            isCopy = true;
            ++copyCount;
        } else {
            ++errorCount;
        }

        i = other.i;
        return *this;
    }

    ~MyBase()
    {
        if (isCopy) {
            if (!copyCount)
                ++errorCount;
            else
                --copyCount;
        }
        if (!liveCount)
            ++errorCount;
        else
            --liveCount;
    }

    bool operator==(const MyBase &other) const
    { return i == other.i; }

protected:
    ushort i;
    bool isCopy;

public:
    static int errorCount;
    static int liveCount;
    static int copyCount;
};

int MyBase::errorCount = 0;
int MyBase::liveCount = 0;
int MyBase::copyCount = 0;

struct MyPrimitive : public MyBase
{
    MyPrimitive(int i = -1) : MyBase(i)
    { ++errorCount; }
    MyPrimitive(const MyPrimitive &other) : MyBase(other)
    { ++errorCount; }
    ~MyPrimitive()
    { ++errorCount; }
};

struct MyMovable : public MyBase
{
    MyMovable(int i = -1) : MyBase(i) {}
};

struct MyComplex : public MyBase
{
    MyComplex(int i = -1) : MyBase(i) {}
};

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(MyPrimitive, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(MyMovable, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(MyComplex, Q_COMPLEX_TYPE);

QT_END_NAMESPACE


class tst_QList: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void removeAll_primitive_data();
    void removeAll_primitive();
    void removeAll_movable_data();
    void removeAll_movable();
    void removeAll_complex_data();
    void removeAll_complex();

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

    // insert in middle 1 element:
    void midInsertOne_int_data() const { commonBenchmark_data<int>(); }
    void midInsertOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void midInsertOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void midInsertOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void midInsertOne_QString_data() const { commonBenchmark_data<QString>(); }

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

    // emplace in middle 1 element:
    void midEmplaceOne_int_data() const { commonBenchmark_data<int>(); }
    void midEmplaceOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void midEmplaceOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void midEmplaceOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void midEmplaceOne_QString_data() const { commonBenchmark_data<QString>(); }

    void midEmplaceOne_int() const { midEmplaceOne_impl<QList, int>(); }
    void midEmplaceOne_primitive() const { midEmplaceOne_impl<QList, MyPrimitive>(); }
    void midEmplaceOne_movable() const { midEmplaceOne_impl<QList, MyMovable>(); }
    void midEmplaceOne_complex() const { midEmplaceOne_impl<QList, MyComplex>(); }
    void midEmplaceOne_QString() const { midEmplaceOne_impl<QList, QString>(); }

// For 5.15 we also want to compare against QVector
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // append 1 element:
    void qvector_appendOne_int_data() const { commonBenchmark_data<int>(); }
    void qvector_appendOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void qvector_appendOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void qvector_appendOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void qvector_appendOne_QString_data() const { commonBenchmark_data<QString>(); }

    void qvector_appendOne_int() const { appendOne_impl<QVector, int>(); } // QTBUG-87330
    void qvector_appendOne_primitive() const { appendOne_impl<QVector, MyPrimitive>(); }
    void qvector_appendOne_movable() const { appendOne_impl<QVector, MyMovable>(); }
    void qvector_appendOne_complex() const { appendOne_impl<QVector, MyComplex>(); }
    void qvector_appendOne_QString() const { appendOne_impl<QVector, QString>(); }

    // prepend 1 element:
    void qvector_prependOne_int_data() const { commonBenchmark_data<int>(); }
    void qvector_prependOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void qvector_prependOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void qvector_prependOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void qvector_prependOne_QString_data() const { commonBenchmark_data<QString>(); }

    void qvector_prependOne_int() const { prependOne_impl<QVector, int>(); }
    void qvector_prependOne_primitive() const { prependOne_impl<QVector, MyPrimitive>(); }
    void qvector_prependOne_movable() const { prependOne_impl<QVector, MyMovable>(); }
    void qvector_prependOne_complex() const { prependOne_impl<QVector, MyComplex>(); }
    void qvector_prependOne_QString() const { prependOne_impl<QVector, QString>(); }

    // insert in middle 1 element:
    void qvector_midInsertOne_int_data() const { commonBenchmark_data<int>(); }
    void qvector_midInsertOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void qvector_midInsertOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void qvector_midInsertOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void qvector_midInsertOne_QString_data() const { commonBenchmark_data<QString>(); }

    void qvector_midInsertOne_int() const { midInsertOne_impl<QVector, int>(); }
    void qvector_midInsertOne_primitive() const { midInsertOne_impl<QVector, MyPrimitive>(); }
    void qvector_midInsertOne_movable() const { midInsertOne_impl<QVector, MyMovable>(); }
    void qvector_midInsertOne_complex() const { midInsertOne_impl<QVector, MyComplex>(); }
    void qvector_midInsertOne_QString() const { midInsertOne_impl<QVector, QString>(); }

    // append/prepend 1 element - hard times for branch predictor:
    void qvector_appendPrependOne_int_data() const { commonBenchmark_data<int>(); }
    void qvector_appendPrependOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void qvector_appendPrependOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void qvector_appendPrependOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void qvector_appendPrependOne_QString_data() const { commonBenchmark_data<QString>(); }

    void qvector_appendPrependOne_int() const { appendPrependOne_impl<QVector, int>(); }
    void qvector_appendPrependOne_primitive() const
    {
        appendPrependOne_impl<QVector, MyPrimitive>();
    }
    void qvector_appendPrependOne_movable() const { appendPrependOne_impl<QVector, MyMovable>(); }
    void qvector_appendPrependOne_complex() const { appendPrependOne_impl<QVector, MyComplex>(); }
    void qvector_appendPrependOne_QString() const { appendPrependOne_impl<QVector, QString>(); }

    // prepend half elements, then appen another half:
    void qvector_prependAppendHalvesOne_int_data() const { commonBenchmark_data<int>(); }
    void qvector_prependAppendHalvesOne_primitive_data() const
    {
        commonBenchmark_data<MyPrimitive>();
    }
    void qvector_prependAppendHalvesOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void qvector_prependAppendHalvesOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void qvector_prependAppendHalvesOne_QString_data() const { commonBenchmark_data<QString>(); }

    void qvector_prependAppendHalvesOne_int() const { prependAppendHalvesOne_impl<QVector, int>(); }
    void qvector_prependAppendHalvesOne_primitive() const
    {
        prependAppendHalvesOne_impl<QVector, MyPrimitive>();
    }
    void qvector_prependAppendHalvesOne_movable() const
    {
        prependAppendHalvesOne_impl<QVector, MyMovable>();
    }
    void qvector_prependAppendHalvesOne_complex() const
    {
        prependAppendHalvesOne_impl<QVector, MyComplex>();
    }
    void qvector_prependAppendHalvesOne_QString() const
    {
        prependAppendHalvesOne_impl<QVector, QString>();
    }

    // emplace in middle 1 element:
    void qvector_midEmplaceOne_int_data() const { commonBenchmark_data<int>(); }
    void qvector_midEmplaceOne_primitive_data() const { commonBenchmark_data<MyPrimitive>(); }
    void qvector_midEmplaceOne_movable_data() const { commonBenchmark_data<MyMovable>(); }
    void qvector_midEmplaceOne_complex_data() const { commonBenchmark_data<MyComplex>(); }
    void qvector_midEmplaceOne_QString_data() const { commonBenchmark_data<QString>(); }

    void qvector_midEmplaceOne_int() const { midEmplaceOne_impl<QVector, int>(); }
    void qvector_midEmplaceOne_primitive() const { midEmplaceOne_impl<QVector, MyPrimitive>(); }
    void qvector_midEmplaceOne_movable() const { midEmplaceOne_impl<QVector, MyMovable>(); }
    void qvector_midEmplaceOne_complex() const { midEmplaceOne_impl<QVector, MyComplex>(); }
    void qvector_midEmplaceOne_QString() const { midEmplaceOne_impl<QVector, QString>(); }
#endif

private:
    template<typename>
    void commonBenchmark_data() const;

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
};

template <class T>
void removeAll_test(const QList<int> &i10, ushort valueToRemove, int itemsToRemove)
{
    bool isComplex = QTypeInfo<T>::isComplex;

    MyBase::errorCount = 0;
    MyBase::liveCount = 0;
    MyBase::copyCount = 0;
    {
        QList<T> list;
        QCOMPARE(MyBase::liveCount, 0);
        QCOMPARE(MyBase::copyCount, 0);

        for (int i = 0; i < 10 * N; ++i) {
            T t(i10.at(i % 10));
            list.append(t);
        }
        QCOMPARE(MyBase::liveCount, isComplex ? list.size() : 0);
        QCOMPARE(MyBase::copyCount, isComplex ? list.size() : 0);

        T t(valueToRemove);
        QCOMPARE(MyBase::liveCount, isComplex ? list.size() + 1 : 1);
        QCOMPARE(MyBase::copyCount, isComplex ? list.size() : 0);

        int removedCount;
        QList<T> l;

        QBENCHMARK {
            l = list;
            removedCount = l.removeAll(t);
        }
        QCOMPARE(removedCount, itemsToRemove * N);
        QCOMPARE(l.size() + removedCount, list.size());
        QVERIFY(!l.contains(valueToRemove));

        QCOMPARE(MyBase::liveCount, isComplex ? l.isDetached() ? list.size() + l.size() + 1 : list.size() + 1 : 1);
        QCOMPARE(MyBase::copyCount, isComplex ? l.isDetached() ? list.size() + l.size() : list.size() : 0);
    }
    if (isComplex)
        QCOMPARE(MyBase::errorCount, 0);
}


void tst_QList::removeAll_primitive_data()
{
    qRegisterMetaType<QList<int> >();

    QTest::addColumn<QList<int> >("i10");
    QTest::addColumn<int>("valueToRemove");
    QTest::addColumn<int>("itemsToRemove");

    QTest::newRow("0%")   << (QList<int>() << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0) << 5 << 0;
    QTest::newRow("10%")  << (QList<int>() << 0 << 0 << 0 << 0 << 5 << 0 << 0 << 0 << 0 << 0) << 5 << 1;
    QTest::newRow("90%")  << (QList<int>() << 5 << 5 << 5 << 5 << 0 << 5 << 5 << 5 << 5 << 5) << 5 << 9;
    QTest::newRow("100%") << (QList<int>() << 5 << 5 << 5 << 5 << 5 << 5 << 5 << 5 << 5 << 5) << 5 << 10;
}

void tst_QList::removeAll_primitive()
{
    QFETCH(QList<int>, i10);
    QFETCH(int, valueToRemove);
    QFETCH(int, itemsToRemove);

    removeAll_test<MyPrimitive>(i10, valueToRemove, itemsToRemove);
}

void tst_QList::removeAll_movable_data()
{
    removeAll_primitive_data();
}

void tst_QList::removeAll_movable()
{
    QFETCH(QList<int>, i10);
    QFETCH(int, valueToRemove);
    QFETCH(int, itemsToRemove);

    removeAll_test<MyMovable>(i10, valueToRemove, itemsToRemove);
}

void tst_QList::removeAll_complex_data()
{
    removeAll_primitive_data();
}

void tst_QList::removeAll_complex()
{
    QFETCH(QList<int>, i10);
    QFETCH(int, valueToRemove);
    QFETCH(int, itemsToRemove);

    removeAll_test<MyComplex>(i10, valueToRemove, itemsToRemove);
}

template<typename T>
void tst_QList::commonBenchmark_data() const
{
    QTest::addColumn<int>("elemCount");

    const auto addRow = [](int count, const char *text) { QTest::newRow(text) << count; };

    const auto p = [](int i, const char *text) { return std::make_pair(i, text); };

    // cap at 20m elements to allow 5.15/6.0 coverage to be the same
    for (auto pair : { p(100, "100"), p(1000, "1k"), p(10000, "10k"), p(100000, "100k"),
                       p(1000000, "1m"), p(10000000, "10m"), p(20000000, "20m") }) {
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
            container.insert(container.size() / 2, lvalue);
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
            container.emplace(container.size() / 2, lvalue);
        }
    }
}

QTEST_APPLESS_MAIN(tst_QList)

#include "main.moc"
