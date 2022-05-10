// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QTBASE_GENERATION_HELPERS_H
#define QTBASE_GENERATION_HELPERS_H

#include "qglobal.h"

#include <vector>

struct tag_input
{
};
struct tag_mapped
{
};
struct tag_reduction
{
};

template<typename tag>
struct SequenceItem
{
    SequenceItem() = default;
    // bool as a stronger "explicit": should never be called inside of QtConcurrent
    SequenceItem(int val, bool) : value(val) { }

    bool operator==(const SequenceItem<tag> &other) const { return value == other.value; }
    bool isOdd() const { return value & 1; }
    void multiplyByTwo() { value *= 2; }

    int value = 0;
};

template<typename tag>
struct NoConstructSequenceItem
{
    NoConstructSequenceItem() = delete;
    // bool as a stronger "explicit": should never be called inside of QtConcurrent
    NoConstructSequenceItem(int val, bool) : value(val) { }

    bool operator==(const NoConstructSequenceItem<tag> &other) const
    {
        return value == other.value;
    }
    bool isOdd() const { return value & 1; }
    void multiplyByTwo() { value *= 2; }

    int value = 0;
};

template<typename tag>
struct MoveOnlySequenceItem
{
    MoveOnlySequenceItem() = default;
    ~MoveOnlySequenceItem() = default;
    MoveOnlySequenceItem(const MoveOnlySequenceItem &) = delete;
    MoveOnlySequenceItem(MoveOnlySequenceItem &&other) : value(other.value) { other.value = -1; }
    MoveOnlySequenceItem &operator=(const MoveOnlySequenceItem &) = delete;
    MoveOnlySequenceItem &operator=(MoveOnlySequenceItem &&other)
    {
        value = other.value;
        other.value = -1;
    }

    // bool as a stronger "explicit": should never be called inside of QtConcurrent
    MoveOnlySequenceItem(int val, bool) : value(val) { }

    bool operator==(const MoveOnlySequenceItem<tag> &other) const { return value == other.value; }
    bool isOdd() const { return value & 1; }
    void multiplyByTwo() { value *= 2; }

    int value = 0;
};

template<typename tag>
struct MoveOnlyNoConstructSequenceItem
{
    MoveOnlyNoConstructSequenceItem() = delete;
    ~MoveOnlyNoConstructSequenceItem() = default;
    MoveOnlyNoConstructSequenceItem(const MoveOnlyNoConstructSequenceItem &) = delete;
    MoveOnlyNoConstructSequenceItem(MoveOnlyNoConstructSequenceItem &&other) : value(other.value)
    {
        other.value = -1;
    }
    MoveOnlyNoConstructSequenceItem &operator=(const MoveOnlyNoConstructSequenceItem &) = delete;
    MoveOnlyNoConstructSequenceItem &operator=(MoveOnlyNoConstructSequenceItem &&other)
    {
        value = other.value;
        other.value = -1;
    }

    // bool as a stronger "explicit": should never be called inside of QtConcurrent
    MoveOnlyNoConstructSequenceItem(int val, bool) : value(val) { }

    bool operator==(const MoveOnlyNoConstructSequenceItem<tag> &other) const
    {
        return value == other.value;
    }
    bool isOdd() const { return value & 1; }
    void multiplyByTwo() { value *= 2; }

    int value = 0;
};

template<typename T>
bool myfilter(const T &el)
{
    return el.isOdd();
}

template<typename T>
class MyFilter
{
public:
    bool operator()(const T &el) { return el.isOdd(); }
};

template<typename T>
class MyMoveOnlyFilter
{
    bool movedFrom = false;

public:
    MyMoveOnlyFilter() = default;
    MyMoveOnlyFilter(const MyMoveOnlyFilter<T> &) = delete;
    MyMoveOnlyFilter &operator=(const MyMoveOnlyFilter<T> &) = delete;

    MyMoveOnlyFilter(MyMoveOnlyFilter<T> &&other) { other.movedFrom = true; }
    MyMoveOnlyFilter &operator=(MyMoveOnlyFilter<T> &&other) { other.movedFrom = true; }

    bool operator()(const T &el)
    {
        if (!movedFrom)
            return el.isOdd();
        else
            return -1;
    }
};

template<typename From, typename To>
To myMap(const From &f)
{
    return To(f.value * 2, true);
}

template<typename T>
void myInplaceMap(T &el)
{
    el.multiplyByTwo();
}

template<typename From, typename To>
class MyMap
{
public:
    To operator()(const From &el) { return To(el.value * 2, true); }
};

template<typename T>
class MyInplaceMap
{
public:
    void operator()(T &el) { el.multiplyByTwo(); }
};

template<typename From, typename To>
class MyMoveOnlyMap
{
    bool movedFrom = false;

public:
    MyMoveOnlyMap() = default;
    MyMoveOnlyMap(const MyMoveOnlyMap<From, To> &) = delete;
    MyMoveOnlyMap &operator=(const MyMoveOnlyMap<From, To> &) = delete;

    MyMoveOnlyMap(MyMoveOnlyMap<From, To> &&other) { other.movedFrom = true; }
    MyMoveOnlyMap &operator=(MyMoveOnlyMap<From, To> &&other) { other.movedFrom = true; }

    To operator()(const From &el)
    {
        if (!movedFrom)
            return To(el.value * 2, true);
        else
            return To(-1, true);
    }
};

template<typename T>
class MyMoveOnlyInplaceMap
{
    bool movedFrom = false;

public:
    MyMoveOnlyInplaceMap() = default;
    MyMoveOnlyInplaceMap(const MyMoveOnlyInplaceMap<T> &) = delete;
    MyMoveOnlyInplaceMap &operator=(const MyMoveOnlyInplaceMap<T> &) = delete;

    MyMoveOnlyInplaceMap(MyMoveOnlyInplaceMap<T> &&other) { other.movedFrom = true; }
    MyMoveOnlyInplaceMap &operator=(MyMoveOnlyInplaceMap<T> &&other) { other.movedFrom = true; }

    void operator()(T &el)
    {
        if (!movedFrom)
            el.multiplyByTwo();
    }
};

template<typename From, typename To>
void myReduce(To &sum, const From &val)
{
    sum.value += val.value;
}

template<typename From, typename To>
class MyReduce
{
public:
    void operator()(To &sum, const From &val) { sum.value += val.value; }
};

template<typename From, typename To>
class MyMoveOnlyReduce
{
    bool movedFrom = false;

public:
    MyMoveOnlyReduce() = default;
    MyMoveOnlyReduce(const MyMoveOnlyReduce<From, To> &) = delete;
    MyMoveOnlyReduce &operator=(const MyMoveOnlyReduce<From, To> &) = delete;

    MyMoveOnlyReduce(MyMoveOnlyReduce<From, To> &&other) { other.movedFrom = true; }
    MyMoveOnlyReduce &operator=(MyMoveOnlyReduce<From, To> &&other) { other.movedFrom = true; }

    void operator()(To &sum, const From &val)
    {
        if (!movedFrom)
            sum.value += val.value;
    }
};

QT_BEGIN_NAMESPACE

// pretty printing
template<typename tag>
char *toString(const SequenceItem<tag> &i)
{
    using QTest::toString;
    return toString(QString::number(i.value));
}

// pretty printing
template<typename T>
char *toString(const std::vector<T> &vec)
{
    using QTest::toString;
    QString result("");
    for (const auto &i : vec) {
        result.append(QString::number(i.value) + ", ");
    }
    if (result.size())
        result.chop(2);
    return toString(result);
}

QT_END_NAMESPACE

#endif // QTBASE_GENERATION_HELPERS_H
