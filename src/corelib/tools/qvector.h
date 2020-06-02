/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2019 Intel Corporation
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QVECTOR_H
#define QVECTOR_H

#include <QtCore/qarraydatapointer.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qiterator.h>

#include <functional>
#include <limits>
#include <initializer_list>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
   template <typename V, typename U> int indexOf(const QVector<V> &list, const U &u, int from);
   template <typename V, typename U> int lastIndexOf(const QVector<V> &list, const U &u, int from);
}

template <typename T> struct QVectorSpecialMethods
{
protected:
    ~QVectorSpecialMethods() = default;
};
template <> struct QVectorSpecialMethods<QByteArray>;
template <> struct QVectorSpecialMethods<QString>;

template <typename T>
class QVector
#ifndef Q_QDOC
    : public QVectorSpecialMethods<T>
#endif
{
    typedef QTypedArrayData<T> Data;
    typedef QArrayDataOps<T> DataOps;
    typedef QArrayDataPointer<T> DataPointer;
    class DisableRValueRefs {};

    DataPointer d;

    template <typename V, typename U> friend int QtPrivate::indexOf(const QVector<V> &list, const U &u, int from);
    template <typename V, typename U> friend int QtPrivate::lastIndexOf(const QVector<V> &list, const U &u, int from);

public:
    typedef T Type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef int size_type;
    typedef qptrdiff difference_type;
    typedef typename Data::iterator iterator;
    typedef typename Data::const_iterator const_iterator;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef typename DataPointer::parameter_type parameter_type;
    using rvalue_ref = typename std::conditional<DataPointer::pass_parameter_by_value, DisableRValueRefs, T &&>::type;

private:
    void resize_internal(int i, Qt::Initialization);
    bool isValidIterator(const_iterator i) const
    {
        const std::less<const T*> less = {};
        return !less(d->end(), i) && !less(i, d->begin());
    }
public:
    QVector(DataPointer dd) noexcept
        : d(dd)
    {
    }

public:
    constexpr inline QVector() noexcept { }
    explicit QVector(int size)
        : d(Data::allocate(size))
    {
        if (size)
            d->appendInitialize(size);
    }
    QVector(int size, const T &t)
        : d(Data::allocate(size))
    {
        if (size)
            d->copyAppend(size, t);
    }

    inline QVector(const QVector<T> &other) noexcept : d(other.d) {}
    QVector(QVector<T> &&other) noexcept : d(std::move(other.d)) {}
    inline QVector(std::initializer_list<T> args)
        : d(Data::allocate(args.size()))
    {
        if (args.size())
            d->copyAppend(args.begin(), args.end());
    }

    ~QVector() /*noexcept(std::is_nothrow_destructible<T>::value)*/ {}
    QVector<T> &operator=(const QVector<T> &other) { d = other.d; return *this; }
    QVector &operator=(QVector &&other) noexcept(std::is_nothrow_destructible<T>::value)
    {
        d = std::move(other.d);
        return *this;
    }
    QVector<T> &operator=(std::initializer_list<T> args)
    {
        d = DataPointer(Data::allocate(args.size()));
        if (args.size())
            d->copyAppend(args.begin(), args.end());
        return *this;
    }
    template <typename InputIterator, QtPrivate::IfIsForwardIterator<InputIterator> = true>
    QVector(InputIterator i1, InputIterator i2)
        : d(Data::allocate(std::distance(i1, i2)))
    {
        if (std::distance(i1, i2))
            d->copyAppend(i1, i2);
    }

    template <typename InputIterator, QtPrivate::IfIsNotForwardIterator<InputIterator> = true>
    QVector(InputIterator i1, InputIterator i2)
        : QVector()
    {
        QtPrivate::reserveIfForwardIterator(this, i1, i2);
        std::copy(i1, i2, std::back_inserter(*this));
    }

    void swap(QVector<T> &other) noexcept { qSwap(d, other.d); }

    friend bool operator==(const QVector &l, const QVector &r)
    {
        if (l.size() != r.size())
            return false;
        if (l.begin() == r.begin())
            return true;

        // do element-by-element comparison
        return l.d->compare(l.begin(), r.begin(), l.size());
    }
    friend bool operator!=(const QVector &l, const QVector &r)
    {
        return !(l == r);
    }

    int size() const noexcept { return int(d->size); }
    int count() const noexcept { return size(); }
    int length() const noexcept { return size(); }

    inline bool isEmpty() const noexcept { return d->size == 0; }

    void resize(int size)
    {
        resize_internal(size, Qt::Uninitialized);
        if (size > this->size())
            d->appendInitialize(size);
    }
    void resize(int size, parameter_type c)
    {
        resize_internal(size, Qt::Uninitialized);
        if (size > this->size())
            d->copyAppend(size - this->size(), c);
    }

    inline int capacity() const { return int(d->constAllocatedCapacity()); }
    void reserve(int size);
    inline void squeeze();

    void detach() { d.detach(); }
    bool isDetached() const noexcept { return !d->isShared(); }

    inline bool isSharedWith(const QVector<T> &other) const { return d == other.d; }

    pointer data() { detach(); return d->data(); }
    const_pointer data() const noexcept { return d->data(); }
    const_pointer constData() const noexcept { return d->data(); }
    void clear() {
        if (!size())
            return;
        if (d->needsDetach()) {
            // must allocate memory
            DataPointer detached(Data::allocate(d.allocatedCapacity(), d->detachFlags()));
            d.swap(detached);
        } else {
            d->truncate(0);
        }
    }

    const_reference at(int i) const noexcept
    {
        Q_ASSERT_X(size_t(i) < size_t(d->size), "QVector::at", "index out of range");
        return data()[i];
    }
    reference operator[](int i)
    {
        Q_ASSERT_X(size_t(i) < size_t(d->size), "QVector::operator[]", "index out of range");
        detach();
        return data()[i];
    }
    const_reference operator[](int i) const noexcept { return at(i); }
    void append(const_reference t)
    { append(const_iterator(std::addressof(t)), const_iterator(std::addressof(t)) + 1); }
    void append(const_iterator i1, const_iterator i2);
    void append(rvalue_ref t) { emplaceBack(std::move(t)); }
    void append(const QVector<T> &l) { append(l.constBegin(), l.constEnd()); }
    void prepend(rvalue_ref t);
    void prepend(const T &t);

    template <typename ...Args>
    reference emplaceBack(Args&&... args) { return *emplace(count(), std::forward<Args>(args)...); }

    iterator insert(int i, parameter_type t)
    { return insert(i, 1, t); }
    iterator insert(int i, int n, parameter_type t);
    iterator insert(const_iterator before, parameter_type t)
    {
        Q_ASSERT_X(isValidIterator(before),  "QVector::insert", "The specified iterator argument 'before' is invalid");
        return insert(before, 1, t);
    }
    iterator insert(const_iterator before, int n, parameter_type t)
    {
        Q_ASSERT_X(isValidIterator(before),  "QVector::insert", "The specified iterator argument 'before' is invalid");
        return insert(std::distance(constBegin(), before), n, t);
    }
    iterator insert(const_iterator before, rvalue_ref t)
    {
        Q_ASSERT_X(isValidIterator(before),  "QVector::insert", "The specified iterator argument 'before' is invalid");
        return insert(std::distance(constBegin(), before), std::move(t));
    }
    iterator insert(int i, rvalue_ref t) { return emplace(i, std::move(t)); }

    template <typename ...Args>
    iterator emplace(const_iterator before, Args&&... args)
    {
        Q_ASSERT_X(isValidIterator(before),  "QVector::emplace", "The specified iterator argument 'before' is invalid");
        return emplace(std::distance(constBegin(), before), std::forward<Args>(args)...);
    }

    template <typename ...Args>
    iterator emplace(int i, Args&&... args);
#if 0
    template< class InputIt >
    iterator insert( const_iterator pos, InputIt first, InputIt last );
    iterator insert( const_iterator pos, std::initializer_list<T> ilist );
#endif
    void replace(int i, const T &t)
    {
        Q_ASSERT_X(i >= 0 && i < d->size, "QVector<T>::replace", "index out of range");
        const T copy(t);
        data()[i] = copy;
    }
    void replace(int i, rvalue_ref t)
    {
        Q_ASSERT_X(i >= 0 && i < d->size, "QVector<T>::replace", "index out of range");
        const T copy(std::move(t));
        data()[i] = std::move(copy);
    }

    void remove(int i, int n = 1);
    void removeFirst() { Q_ASSERT(!isEmpty()); remove(0); }
    void removeLast() { Q_ASSERT(!isEmpty()); remove(size() - 1); }
    value_type takeFirst() { Q_ASSERT(!isEmpty()); value_type v = std::move(first()); remove(0); return v; }
    value_type takeLast() { Q_ASSERT(!isEmpty()); value_type v = std::move(last()); remove(size() - 1); return v; }

    QVector<T> &fill(parameter_type t, int size = -1);

    int indexOf(const T &t, int from = 0) const noexcept;
    int lastIndexOf(const T &t, int from = -1) const noexcept;
    bool contains(const T &t) const noexcept
    {
        return indexOf(t) != -1;
    }
    int count(const T &t) const noexcept
    {
        return int(std::count(&*cbegin(), &*cend(), t));
    }

    // QList compatibility
    void removeAt(int i) { remove(i); }
    int removeAll(const T &t)
    {
        const const_iterator ce = this->cend(), cit = std::find(this->cbegin(), ce, t);
        if (cit == ce)
            return 0;
        int index = cit - this->cbegin();
        // next operation detaches, so ce, cit, t may become invalidated:
        const T tCopy = t;
        const iterator e = end(), it = std::remove(begin() + index, e, tCopy);
        const int result = std::distance(it, e);
        erase(it, e);
        return result;
    }
    bool removeOne(const T &t)
    {
        const int i = indexOf(t);
        if (i < 0)
            return false;
        remove(i);
        return true;
    }
    T takeAt(int i) { T t = std::move((*this)[i]); remove(i); return t; }
    void move(int from, int to)
    {
        Q_ASSERT_X(from >= 0 && from < size(), "QVector::move(int,int)", "'from' is out-of-range");
        Q_ASSERT_X(to >= 0 && to < size(), "QVector::move(int,int)", "'to' is out-of-range");
        if (from == to) // don't detach when no-op
            return;
        detach();
        T * const b = d->begin();
        if (from < to)
            std::rotate(b + from, b + from + 1, b + to + 1);
        else
            std::rotate(b + to, b + from, b + from + 1);
    }

    // STL-style
    iterator begin() { detach(); return d->begin(); }
    iterator end() { detach(); return d->end(); }

    const_iterator begin() const noexcept { return d->constBegin(); }
    const_iterator end() const noexcept { return d->constEnd(); }
    const_iterator cbegin() const noexcept { return d->constBegin(); }
    const_iterator cend() const noexcept { return d->constEnd(); }
    const_iterator constBegin() const noexcept { return d->constBegin(); }
    const_iterator constEnd() const noexcept { return d->constEnd(); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    iterator erase(const_iterator begin, const_iterator end);
    inline iterator erase(const_iterator pos) { return erase(pos, pos+1); }

    // more Qt
    inline T& first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T &first() const { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T &constFirst() const { Q_ASSERT(!isEmpty()); return *begin(); }
    inline T& last() { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline const T &last() const { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline const T &constLast() const { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline bool startsWith(const T &t) const { return !isEmpty() && first() == t; }
    inline bool endsWith(const T &t) const { return !isEmpty() && last() == t; }
    QVector<T> mid(int pos, int len = -1) const;

    T value(int i) const { return value(i, T()); }
    T value(int i, const T &defaultValue) const;

    void swapItemsAt(int i, int j) {
        Q_ASSERT_X(i >= 0 && i < size() && j >= 0 && j < size(),
                    "QVector<T>::swap", "index out of range");
        detach();
        qSwap(d->begin()[i], d->begin()[j]);
    }

    // STL compatibility
    inline void push_back(const T &t) { append(t); }
    void push_back(rvalue_ref t) { append(std::move(t)); }
    void push_front(rvalue_ref t) { prepend(std::move(t)); }
    inline void push_front(const T &t) { prepend(t); }
    void pop_back() { removeLast(); }
    void pop_front() { removeFirst(); }

    template <typename ...Args>
    reference emplace_back(Args&&... args) { return emplaceBack(std::forward<Args>(args)...); }

    inline bool empty() const
    { return d->size == 0; }
    inline reference front() { return first(); }
    inline const_reference front() const { return first(); }
    inline reference back() { return last(); }
    inline const_reference back() const { return last(); }
    void shrink_to_fit() { squeeze(); }

    // comfort
    QVector<T> &operator+=(const QVector<T> &l) { append(l.cbegin(), l.cend()); return *this; }
    inline QVector<T> operator+(const QVector<T> &l) const
    { QVector n = *this; n += l; return n; }
    inline QVector<T> &operator+=(const T &t)
    { append(t); return *this; }
    inline QVector<T> &operator<< (const T &t)
    { append(t); return *this; }
    inline QVector<T> &operator<<(const QVector<T> &l)
    { *this += l; return *this; }
    inline QVector<T> &operator+=(rvalue_ref t)
    { append(std::move(t)); return *this; }
    inline QVector<T> &operator<<(rvalue_ref t)
    { append(std::move(t)); return *this; }

    // Consider deprecating in 6.4 or later
    static QVector<T> fromList(const QVector<T> &list) { return list; }
    QVector<T> toList() const { return *this; }

    static inline QVector<T> fromVector(const QVector<T> &vector) { return vector; }
    inline QVector<T> toVector() const { return *this; }
};

#if defined(__cpp_deduction_guides) && __cpp_deduction_guides >= 201606
template <typename InputIterator,
          typename ValueType = typename std::iterator_traits<InputIterator>::value_type,
          QtPrivate::IfIsInputIterator<InputIterator> = true>
QVector(InputIterator, InputIterator) -> QVector<ValueType>;
#endif

template <typename T>
inline void QVector<T>::resize_internal(int newSize, Qt::Initialization)
{
    Q_ASSERT(newSize >= 0);

    if (d->needsDetach() || newSize > capacity()) {
        // must allocate memory
        DataPointer detached(Data::allocate(d->detachCapacity(newSize),
                                            d->detachFlags()));
        if (size() && newSize) {
            detached->copyAppend(constBegin(), constBegin() + qMin(newSize, size()));
        }
        d.swap(detached);
    }

    if (newSize < size())
        d->truncate(newSize);
}

template <typename T>
void QVector<T>::reserve(int asize)
{
    // capacity() == 0 for immutable data, so this will force a detaching below
    if (asize <= capacity()) {
        if (d->flags() & Data::CapacityReserved)
            return;  // already reserved, don't shrink
        if (!d->isShared()) {
            // accept current allocation, don't shrink
            d->flags() |= Data::CapacityReserved;
            return;
        }
    }

    DataPointer detached(Data::allocate(qMax(asize, size()),
                                        d->detachFlags() | Data::CapacityReserved));
    detached->copyAppend(constBegin(), constEnd());
    d.swap(detached);
}

template <typename T>
inline void QVector<T>::squeeze()
{
    if (d->needsDetach() || size() != capacity()) {
        // must allocate memory
        DataPointer detached(Data::allocate(size(), d->detachFlags() & ~Data::CapacityReserved));
        if (size()) {
            detached->copyAppend(constBegin(), constEnd());
        }
        d.swap(detached);
    }
}

template <typename T>
inline void QVector<T>::remove(int i, int n)
{
    Q_ASSERT_X(size_t(i) + size_t(n) <= size_t(d->size), "QVector::remove", "index out of range");
    Q_ASSERT_X(n >= 0, "QVector::remove", "invalid count");

    if (n == 0)
        return;

    const size_t newSize = size() - n;
    if (d->needsDetach() ||
            ((d->flags() & Data::CapacityReserved) == 0
             && newSize < d->allocatedCapacity()/2)) {
        // allocate memory
        DataPointer detached(Data::allocate(d->detachCapacity(newSize),
                             d->detachFlags() & ~(Data::GrowsBackwards | Data::GrowsForward)));
        const_iterator where = constBegin() + i;
        if (newSize) {
            detached->copyAppend(constBegin(), where);
            detached->copyAppend(where + n, constEnd());
        }
        d.swap(detached);
    } else {
        // we're detached and we can just move data around
        d->erase(d->begin() + i, d->begin() + i + n);
    }
}

template <typename T>
inline void QVector<T>::prepend(const T &t)
{ insert(0, 1, t); }
template <typename T>
void QVector<T>::prepend(rvalue_ref t)
{ insert(0, std::move(t)); }

template<typename T>
inline T QVector<T>::value(int i, const T &defaultValue) const
{
    return size_t(i) < size_t(d->size) ? at(i) : defaultValue;
}

template <typename T>
inline void QVector<T>::append(const_iterator i1, const_iterator i2)
{
    if (i1 == i2)
        return;
    const size_t newSize = size() + std::distance(i1, i2);
    if (d->needsDetach() || newSize > d->allocatedCapacity()) {
        DataPointer detached(Data::allocate(d->detachCapacity(newSize),
                                            d->detachFlags() | Data::GrowsForward));
        detached->copyAppend(constBegin(), constEnd());
        detached->copyAppend(i1, i2);
        d.swap(detached);
    } else {
        // we're detached and we can just move data around
        d->copyAppend(i1, i2);
    }
}

template <typename T>
inline typename QVector<T>::iterator
QVector<T>::insert(int i, int n, parameter_type t)
{
    Q_ASSERT_X(size_t(i) <= size_t(d->size), "QVector<T>::insert", "index out of range");

    // we don't have a quick exit for n == 0
    // it's not worth wasting CPU cycles for that

    const size_t newSize = size() + n;
    if (d->needsDetach() || newSize > d->allocatedCapacity()) {
        typename Data::ArrayOptions flags = d->detachFlags() | Data::GrowsForward;
        if (size_t(i) <= newSize / 4)
            flags |= Data::GrowsBackwards;

        DataPointer detached(Data::allocate(d->detachCapacity(newSize), flags));
        const_iterator where = constBegin() + i;
        detached->copyAppend(constBegin(), where);
        detached->copyAppend(n, t);
        detached->copyAppend(where, constEnd());
        d.swap(detached);
    } else {
        // we're detached and we can just move data around
        if (i == size()) {
            d->copyAppend(n, t);
        } else {
            T copy(t);
            d->insert(d.begin() + i, n, copy);
        }
    }
    return d.begin() + i;
}

template <typename T>
template <typename ...Args>
typename QVector<T>::iterator
QVector<T>::emplace(int i, Args&&... args)
{
     Q_ASSERT_X(i >= 0 && i <= d->size, "QVector<T>::insert", "index out of range");

    const size_t newSize = size() + 1;
    if (d->needsDetach() || newSize > d->allocatedCapacity()) {
        typename Data::ArrayOptions flags = d->detachFlags() | Data::GrowsForward;
        if (size_t(i) <= newSize / 4)
            flags |= Data::GrowsBackwards;

        DataPointer detached(Data::allocate(d->detachCapacity(newSize), flags));
        const_iterator where = constBegin() + i;

        // First, create an element to handle cases, when a user moves
        // the element from a container to the same container
        detached->createInPlace(detached.begin() + i, std::forward<Args>(args)...);

        // Then, put the first part of the elements to the new location
        detached->copyAppend(constBegin(), where);

        // After that, increase the actual size, because we created
        // one extra element
        ++detached.size;

        // Finally, put the rest of the elements to the new location
        detached->copyAppend(where, constEnd());

        d.swap(detached);
    } else {
        d->emplace(d.begin() + i, std::forward<Args>(args)...);
    }
    return d.begin() + i;
}

template <typename T>
typename QVector<T>::iterator QVector<T>::erase(const_iterator abegin, const_iterator aend)
{
    Q_ASSERT_X(isValidIterator(abegin), "QVector::erase", "The specified iterator argument 'abegin' is invalid");
    Q_ASSERT_X(isValidIterator(aend), "QVector::erase", "The specified iterator argument 'aend' is invalid");
    Q_ASSERT(aend >= abegin);

    int i = std::distance(d.constBegin(), abegin);
    int n = std::distance(abegin, aend);
    remove(i, n);

    return d.begin() + i;
}

template <typename T>
inline QVector<T> &QVector<T>::fill(parameter_type t, int newSize)
{
    if (newSize == -1)
        newSize = size();
    if (d->needsDetach() || newSize > capacity()) {
        // must allocate memory
        DataPointer detached(Data::allocate(d->detachCapacity(newSize),
                                            d->detachFlags()));
        detached->copyAppend(newSize, t);
        d.swap(detached);
    } else {
        // we're detached
        const T copy(t);
        d->assign(d.begin(), d.begin() + qMin(size(), newSize), t);
        if (newSize > size())
            d->copyAppend(newSize - size(), copy);
    }
    return *this;
}

namespace QtPrivate {
template <typename T, typename U>
int indexOf(const QVector<T> &vector, const U &u, int from)
{
    if (from < 0)
        from = qMax(from + vector.size(), 0);
    if (from < vector.size()) {
        auto n = vector.begin() + from - 1;
        auto e = vector.end();
        while (++n != e)
            if (*n == u)
                return int(n - vector.begin());
    }
    return -1;
}

template <typename T, typename U>
int lastIndexOf(const QVector<T> &vector, const U &u, int from)
{
    if (from < 0)
        from += vector.d->size;
    else if (from >= vector.size())
        from = vector.size() - 1;
    if (from >= 0) {
        auto b = vector.begin();
        auto n = vector.begin() + from + 1;
        while (n != b) {
            if (*--n == u)
                return int(n - b);
        }
    }
    return -1;
}
}

template <typename T>
int QVector<T>::indexOf(const T &t, int from) const noexcept
{
    return QtPrivate::indexOf<T, T>(*this, t, from);
}

template <typename T>
int QVector<T>::lastIndexOf(const T &t, int from) const noexcept
{
    return QtPrivate::lastIndexOf(*this, t, from);
}

template <typename T>
inline QVector<T> QVector<T>::mid(int pos, int len) const
{
    qsizetype p = pos;
    qsizetype l = len;
    using namespace QtPrivate;
    switch (QContainerImplHelper::mid(d.size, &p, &l)) {
    case QContainerImplHelper::Null:
    case QContainerImplHelper::Empty:
        return QVector();
    case QContainerImplHelper::Full:
        return *this;
    case QContainerImplHelper::Subset:
        break;
    }

    // Allocate memory
    DataPointer copied(Data::allocate(l));
    copied->copyAppend(constBegin() + p, constBegin() + p + l);
    return copied;
}

Q_DECLARE_SEQUENTIAL_ITERATOR(Vector)
Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(Vector)

template <typename T>
size_t qHash(const QVector<T> &key, size_t seed = 0)
    noexcept(noexcept(qHashRange(key.cbegin(), key.cend(), seed)))
{
    return qHashRange(key.cbegin(), key.cend(), seed);
}

template <typename T>
auto operator<(const QVector<T> &lhs, const QVector<T> &rhs)
    noexcept(noexcept(std::lexicographical_compare(lhs.begin(), lhs.end(),
                                                   rhs.begin(), rhs.end())))
    -> decltype(std::declval<T>() < std::declval<T>())
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                        rhs.begin(), rhs.end());
}

template <typename T>
auto operator>(const QVector<T> &lhs, const QVector<T> &rhs)
    noexcept(noexcept(lhs < rhs))
    -> decltype(lhs < rhs)
{
    return rhs < lhs;
}

template <typename T>
auto operator<=(const QVector<T> &lhs, const QVector<T> &rhs)
    noexcept(noexcept(lhs < rhs))
    -> decltype(lhs < rhs)
{
    return !(lhs > rhs);
}

template <typename T>
auto operator>=(const QVector<T> &lhs, const QVector<T> &rhs)
    noexcept(noexcept(lhs < rhs))
    -> decltype(lhs < rhs)
{
    return !(lhs < rhs);
}

/*
   ### Qt 5:
   ### This needs to be removed for next releases of Qt. It is a workaround for vc++ because
   ### Qt exports QPolygon and QPolygonF that inherit QVector<QPoint> and
   ### QVector<QPointF> respectively.
*/

#if defined(Q_CC_MSVC) && !defined(QT_BUILD_CORE_LIB)
QT_BEGIN_INCLUDE_NAMESPACE
#include <QtCore/qpoint.h>
QT_END_INCLUDE_NAMESPACE
extern template class Q_CORE_EXPORT QVector<QPointF>;
extern template class Q_CORE_EXPORT QVector<QPoint>;
#endif

QVector<uint> QStringView::toUcs4() const { return QtPrivate::convertToUcs4(*this); }

QT_END_NAMESPACE

#include <QtCore/qbytearraylist.h>
#include <QtCore/qstringlist.h>

#endif // QVECTOR_H
