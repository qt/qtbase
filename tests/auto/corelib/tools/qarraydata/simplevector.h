/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QARRAY_TEST_SIMPLE_VECTOR_H
#define QARRAY_TEST_SIMPLE_VECTOR_H

#include <QtCore/qarraydata.h>
#include <QtCore/qarraydataops.h>

#include <algorithm>

template <class T>
struct SimpleVector
{
private:
    typedef QTypedArrayData<T> Data;
    typedef QArrayDataOps<T> DataOps;

public:
    typedef T value_type;
    typedef typename Data::iterator iterator;
    typedef typename Data::const_iterator const_iterator;

    SimpleVector()
        : d(Data::sharedNull())
    {
    }

    SimpleVector(const SimpleVector &vec)
        : d(vec.d)
    {
        d->ref.ref();
    }

    SimpleVector(size_t n, const T &t)
        : d(Data::allocate(n))
    {
        if (n)
            static_cast<DataOps *>(d)->copyAppend(n, t);
    }

    SimpleVector(const T *begin, const T *end)
        : d(Data::allocate(end - begin))
    {
        if (end - begin)
            static_cast<DataOps *>(d)->copyAppend(begin, end);
    }

    explicit SimpleVector(Data *ptr)
        : d(ptr)
    {
    }

    ~SimpleVector()
    {
        if (!d->ref.deref()) {
            static_cast<DataOps *>(d)->destroyAll();
            Data::deallocate(d);
        }
    }

    SimpleVector &operator=(const SimpleVector &vec)
    {
        SimpleVector temp(vec);
        this->swap(temp);
        return *this;
    }

    bool empty() const { return d->size == 0; }
    bool isNull() const { return d == Data::sharedNull(); }
    bool isEmpty() const { return this->empty(); }

    bool isSharedWith(const SimpleVector &other) const { return d == other.d; }

    size_t size() const { return d->size; }
    size_t capacity() const { return d->alloc; }

    const_iterator begin() const { return d->begin(); }
    const_iterator end() const { return d->end(); }

    const_iterator constBegin() const { return begin(); }
    const_iterator constEnd() const { return end(); }

    const T &operator[](size_t i) const { Q_ASSERT(i < size_t(d->size)); return begin()[i]; }
    const T &at(size_t i) const { Q_ASSERT(i < size_t(d->size)); return begin()[i]; }

    const T &front() const
    {
        Q_ASSERT(!isEmpty());
        return *begin();
    }

    const T &back() const
    {
        Q_ASSERT(!isEmpty());
        return *(end() - 1);
    }

    void reserve(size_t n)
    {
        if (n > capacity()
                || (n
                && !d->capacityReserved
                && (d->ref != 1 || (d->capacityReserved = 1, false)))) {
            SimpleVector detached(Data::allocate(n, true));
            static_cast<DataOps *>(detached.d)->copyAppend(constBegin(), constEnd());
            detached.swap(*this);
        }
    }

    void prepend(const_iterator first, const_iterator last)
    {
        if (!d->size) {
            append(first, last);
            return;
        }

        if (first == last)
            return;

        T *const begin = d->begin();
        if (d->ref != 1
                || capacity() - size() < size_t(last - first)) {
            SimpleVector detached(Data::allocate(
                        qMax(capacity(), size() + (last - first)), d->capacityReserved));

            static_cast<DataOps *>(detached.d)->copyAppend(first, last);
            static_cast<DataOps *>(detached.d)->copyAppend(begin, begin + d->size);
            detached.swap(*this);

            return;
        }

        static_cast<DataOps *>(d)->insert(begin, first, last);
    }

    void append(const_iterator first, const_iterator last)
    {
        if (first == last)
            return;

        if (d->ref != 1
                || capacity() - size() < size_t(last - first)) {
            SimpleVector detached(Data::allocate(
                        qMax(capacity(), size() + (last - first)), d->capacityReserved));

            if (d->size) {
                const T *const begin = constBegin();
                static_cast<DataOps *>(detached.d)->copyAppend(begin, begin + d->size);
            }
            static_cast<DataOps *>(detached.d)->copyAppend(first, last);
            detached.swap(*this);

            return;
        }

        static_cast<DataOps *>(d)->copyAppend(first, last);
    }

    void insert(int position, const_iterator first, const_iterator last)
    {
        if (position < 0)
            position += d->size + 1;

        if (position <= 0) {
            prepend(first, last);
            return;
        }

        if (size_t(position) >= size()) {
            append(first, last);
            return;
        }

        if (first == last)
            return;

        T *const begin = d->begin();
        T *const where = begin + position;
        const T *const end = begin + d->size;
        if (d->ref != 1
                || capacity() - size() < size_t(last - first)) {
            SimpleVector detached(Data::allocate(
                        qMax(capacity(), size() + (last - first)), d->capacityReserved));

            if (position)
                static_cast<DataOps *>(detached.d)->copyAppend(begin, where);
            static_cast<DataOps *>(detached.d)->copyAppend(first, last);
            static_cast<DataOps *>(detached.d)->copyAppend(where, end);
            detached.swap(*this);

            return;
        }

        // Temporarily copy overlapping data, if needed
        if ((first >= where && first < end)
                || (last > where && last <= end)) {
            SimpleVector tmp(first, last);
            static_cast<DataOps *>(d)->insert(where, tmp.constBegin(), tmp.constEnd());
            return;
        }

        static_cast<DataOps *>(d)->insert(where, first, last);
    }

    void swap(SimpleVector &other)
    {
        qSwap(d, other.d);
    }

    void clear()
    {
        SimpleVector tmp(d);
        d = Data::sharedEmpty();
    }

private:
    Data *d;
};

template <class T>
bool operator==(const SimpleVector<T> &lhs, const SimpleVector<T> &rhs)
{
    if (lhs.isSharedWith(rhs))
        return true;
    if (lhs.size() != rhs.size())
        return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <class T>
bool operator!=(const SimpleVector<T> &lhs, const SimpleVector<T> &rhs)
{
    return !(lhs == rhs);
}

template <class T>
bool operator<(const SimpleVector<T> &lhs, const SimpleVector<T> &rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <class T>
bool operator>(const SimpleVector<T> &lhs, const SimpleVector<T> &rhs)
{
    return rhs < lhs;
}

template <class T>
bool operator<=(const SimpleVector<T> &lhs, const SimpleVector<T> &rhs)
{
    return !(rhs < lhs);
}

template <class T>
bool operator>=(const SimpleVector<T> &lhs, const SimpleVector<T> &rhs)
{
    return !(lhs < rhs);
}

namespace std {
    template <class T>
    void swap(SimpleVector<T> &v1, SimpleVector<T> &v2)
    {
        v1.swap(v2);
    }
}

#endif // include guard
