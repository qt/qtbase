// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#ifndef QARRAY_TEST_SIMPLE_VECTOR_H
#define QARRAY_TEST_SIMPLE_VECTOR_H

#include <QtCore/qarraydata.h>
#include <QtCore/qarraydatapointer.h>
#include <QtCore/qvarlengtharray.h>

#include <algorithm>

template <class T>
struct SimpleVector
{
private:
    typedef QTypedArrayData<T> Data;
    typedef QArrayDataPointer<T> DataPointer;

public:
    typedef T value_type;
    typedef T *iterator;
    typedef const T *const_iterator;

    SimpleVector()
    {
    }

    explicit SimpleVector(size_t n, bool capacityReserved = false)
        : d(Data::allocate(n))
    {
        if (n)
            d->appendInitialize(n);
        if (capacityReserved)
            d.setFlag(QArrayData::CapacityReserved);
    }

    SimpleVector(size_t n, const T &t, bool capacityReserved = false)
        : d(Data::allocate(n))
    {
        if (n)
            d->copyAppend(n, t);
        if (capacityReserved)
            d.setFlag(QArrayData::CapacityReserved);
    }

    SimpleVector(const T *begin, const T *end, bool capacityReserved = false)
        : d(Data::allocate(end - begin))
    {
        if (end - begin)
            d->copyAppend(begin, end);
        if (capacityReserved)
            d.setFlag(QArrayData::CapacityReserved);
    }

    SimpleVector(Data *header, T *data, size_t len = 0)
        : d(header, data, len)
    {
    }

    explicit SimpleVector(QPair<Data*, T*> ptr, size_t len = 0)
        : d(ptr, len)
    {
    }

    SimpleVector(const QArrayDataPointer<T> &other)
        : d(other)
    {
    }

    bool empty() const { return d.size == 0; }
    bool isNull() const { return d.isNull(); }
    bool isEmpty() const { return this->empty(); }

    bool isStatic() const { return !d.isMutable(); }
    bool isShared() const { return d->isShared(); }
    bool isSharedWith(const SimpleVector &other) const { return d == other.d; }

    size_t size() const { return d.size; }
    size_t capacity() const { return d->constAllocatedCapacity(); }

    iterator begin() { detach(); return d->begin(); }
    iterator end() { detach(); return d->end(); }

    const_iterator begin() const { return d->constBegin(); }
    const_iterator end() const { return d->constEnd(); }

    const_iterator constBegin() const { return begin(); }
    const_iterator constEnd() const { return end(); }

    T &operator[](size_t i) { Q_ASSERT(i < size_t(d->size)); detach(); return begin()[i]; }
    T &at(size_t i) { Q_ASSERT(i < size_t(d->size)); detach(); return begin()[i]; }

    const T &operator[](size_t i) const { Q_ASSERT(i < size_t(d->size)); return begin()[i]; }
    const T &at(size_t i) const { Q_ASSERT(i < size_t(d->size)); return begin()[i]; }

    T &front()
    {
        Q_ASSERT(!isEmpty());
        detach();
        return *begin();
    }

    T &back()
    {
        Q_ASSERT(!isEmpty());
        detach();
        return *(end() - 1);
    }

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
        if (n == 0)
            return;

        if (n <= capacity()) {
            if (d->flags() & Data::CapacityReserved)
                return;
            if (!d->isShared()) {
                d.setFlag(Data::CapacityReserved);
                return;
            }
        }

        SimpleVector detached(Data::allocate(qMax(n, size())));
        if (size()) {
            detached.d->copyAppend(constBegin(), constEnd());
            detached.d->setFlag(QArrayData::CapacityReserved);
        }
        detached.swap(*this);
    }

    void resize(size_t newSize)
    {
        if (size() == newSize)
            return;

        if (d->needsDetach() || newSize > capacity()) {
            SimpleVector detached(Data::allocate(d->detachCapacity(newSize)));
            if (newSize) {
                if (newSize < size()) {
                    const T *const begin = constBegin();
                    detached.d->copyAppend(begin, begin + newSize);
                } else {
                    if (size()) {
                        const T *const begin = constBegin();
                        detached.d->copyAppend(begin, begin + size());
                    }
                    detached.d->appendInitialize(newSize);
                }
            }
            detached.swap(*this);
            return;
        }

        if (newSize > size())
            d->appendInitialize(newSize);
        else
            d->truncate(newSize);
    }

    void prepend(const_iterator first, const_iterator last)
    {
        if (!d->size) {
            append(first, last);
            return;
        }

        if (first == last)
            return;

        d->insert(0, first, last - first);
    }

    void append(const_iterator first, const_iterator last) { d->growAppend(first, last); }

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

        if (first >= d.begin() && first <= d.end()) {
            QVarLengthArray<T> copy(first, last);
            insert(position, copy.begin(), copy.end());
            return;
        }

        d->insert(position, first, last - first);
    }

    void erase(iterator first, iterator last)
    {
        if (first == last)
            return;

        const T *const begin = d->begin();
        const T *const end = begin + d->size;

        if (d->needsDetach()) {
            SimpleVector detached(Data::allocate(d->detachCapacity(size() - (last - first))));
            if (first != begin)
                detached.d->copyAppend(begin, first);
            detached.d->copyAppend(last, end);
            detached.swap(*this);

            return;
        }

        if (last == end)
            d->truncate(end - first);
        else
            d->erase(first, last - first);
    }

    void swap(SimpleVector &other)
    {
        qSwap(d, other.d);
    }

    void clear()
    {
        d.clear();
    }

    void detach()
    {
        d.detach();
    }

    static SimpleVector fromRawData(const T *data, size_t size)
    {
        return SimpleVector(QArrayDataPointer<T>::fromRawData(data, size));
    }

private:
    QArrayDataPointer<T> d;
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
