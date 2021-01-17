/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QVARLENGTHARRAY_H
#define QVARLENGTHARRAY_H

#include <QtCore/qcontainerfwd.h>
#include <QtCore/qglobal.h>
#include <QtCore/qalgorithms.h>
#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qhashfunctions.h>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <new>
#include <string.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE


// Prealloc = 256 by default, specified in qcontainerfwd.h
template<class T, qsizetype Prealloc>
class QVarLengthArray
{
    static_assert(std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

public:
    QVarLengthArray() : QVarLengthArray(0) {}

    inline explicit QVarLengthArray(qsizetype size);

    inline QVarLengthArray(const QVarLengthArray<T, Prealloc> &other)
        : a(Prealloc), s(0), ptr(reinterpret_cast<T *>(array))
    {
        append(other.constData(), other.size());
    }

    QVarLengthArray(QVarLengthArray &&other)
            noexcept(std::is_nothrow_move_constructible_v<T>)
        : a{other.a},
          s{other.s},
          ptr{other.ptr}
    {
        const auto otherInlineStorage = reinterpret_cast<T*>(other.array);
        if (ptr == otherInlineStorage) {
            // inline buffer - move into our inline buffer:
            ptr = reinterpret_cast<T*>(array);
            QtPrivate::q_uninitialized_relocate_n(otherInlineStorage, s, ptr);
        } else {
            // heap buffer - we just stole the memory
        }
        // reset other to internal storage:
        other.a = Prealloc;
        other.s = 0;
        other.ptr = otherInlineStorage;
    }

    QVarLengthArray(std::initializer_list<T> args)
        : QVarLengthArray(args.begin(), args.end())
    {
    }

    template <typename InputIterator, QtPrivate::IfIsInputIterator<InputIterator> = true>
    inline QVarLengthArray(InputIterator first, InputIterator last)
        : QVarLengthArray()
    {
        QtPrivate::reserveIfForwardIterator(this, first, last);
        std::copy(first, last, std::back_inserter(*this));
    }

    inline ~QVarLengthArray()
    {
        if constexpr (QTypeInfo<T>::isComplex)
            std::destroy_n(ptr, s);
        if (ptr != reinterpret_cast<T *>(array))
            free(ptr);
    }
    inline QVarLengthArray<T, Prealloc> &operator=(const QVarLengthArray<T, Prealloc> &other)
    {
        if (this != &other) {
            clear();
            append(other.constData(), other.size());
        }
        return *this;
    }

    QVarLengthArray &operator=(QVarLengthArray &&other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        // we're only required to be self-move-assignment-safe
        // when we're in the moved-from state (Hinnant criterion)
        // the moved-from state is the empty state, so we're good with the clear() here:
        clear();
        Q_ASSERT(capacity() >= Prealloc);
        const auto otherInlineStorage = reinterpret_cast<T *>(other.array);
        if (other.ptr != otherInlineStorage) {
            // heap storage: steal the external buffer, reset other to otherInlineStorage
            a = std::exchange(other.a, Prealloc);
            ptr = std::exchange(other.ptr, otherInlineStorage);
        } else {
            // inline storage: move into our storage (doesn't matter whether inline or external)
            QtPrivate::q_uninitialized_relocate_n(other.ptr, other.s, ptr);
        }
        s = std::exchange(other.s, 0);
        return *this;
    }

    QVarLengthArray<T, Prealloc> &operator=(std::initializer_list<T> list)
    {
        resize(qsizetype(list.size()));
        std::copy(list.begin(), list.end(),
                  QT_MAKE_CHECKED_ARRAY_ITERATOR(this->begin(), this->size()));
        return *this;
    }

    inline void removeLast()
    {
        Q_ASSERT(s > 0);
        if constexpr (QTypeInfo<T>::isComplex)
            ptr[s - 1].~T();
        --s;
    }
    inline qsizetype size() const { return s; }
    inline qsizetype count() const { return s; }
    inline qsizetype length() const { return s; }
    inline T &first()
    {
        Q_ASSERT(!isEmpty());
        return *begin();
    }
    inline const T &first() const
    {
        Q_ASSERT(!isEmpty());
        return *begin();
    }
    T &last()
    {
        Q_ASSERT(!isEmpty());
        return *(end() - 1);
    }
    const T &last() const
    {
        Q_ASSERT(!isEmpty());
        return *(end() - 1);
    }
    inline bool isEmpty() const { return (s == 0); }
    inline void resize(qsizetype size);
    inline void clear() { resize(0); }
    inline void squeeze();

    inline qsizetype capacity() const { return a; }
    inline void reserve(qsizetype size);

    template <typename AT = T>
    inline qsizetype indexOf(const AT &t, qsizetype from = 0) const;
    template <typename AT = T>
    inline qsizetype lastIndexOf(const AT &t, qsizetype from = -1) const;
    template <typename AT = T>
    inline bool contains(const AT &t) const;

    inline T &operator[](qsizetype idx)
    {
        Q_ASSERT(idx >= 0 && idx < s);
        return ptr[idx];
    }
    inline const T &operator[](qsizetype idx) const
    {
        Q_ASSERT(idx >= 0 && idx < s);
        return ptr[idx];
    }
    inline const T &at(qsizetype idx) const { return operator[](idx); }

    T value(qsizetype i) const;
    T value(qsizetype i, const T &defaultValue) const;

    inline void append(const T &t)
    {
        if (s == a) { // i.e. s != 0
            T copy(t);
            reallocate(s, s << 1);
            const qsizetype idx = s++;
            new (ptr + idx) T(std::move(copy));
        } else {
            const qsizetype idx = s++;
            new (ptr + idx) T(t);
        }
    }

    void append(T &&t)
    {
        if (s == a)
            reallocate(s, s << 1);
        const qsizetype idx = s++;
        new (ptr + idx) T(std::move(t));
    }

    void append(const T *buf, qsizetype size);
    inline QVarLengthArray<T, Prealloc> &operator<<(const T &t)
    { append(t); return *this; }
    inline QVarLengthArray<T, Prealloc> &operator<<(T &&t)
    { append(std::move(t)); return *this; }
    inline QVarLengthArray<T, Prealloc> &operator+=(const T &t)
    { append(t); return *this; }
    inline QVarLengthArray<T, Prealloc> &operator+=(T &&t)
    { append(std::move(t)); return *this; }

    void prepend(T &&t);
    void prepend(const T &t);
    void insert(qsizetype i, T &&t);
    void insert(qsizetype i, const T &t);
    void insert(qsizetype i, qsizetype n, const T &t);
    void replace(qsizetype i, const T &t);
    void remove(qsizetype i);
    void remove(qsizetype i, qsizetype n);
    template <typename AT = T>
    qsizetype removeAll(const AT &t);
    template <typename AT = T>
    bool removeOne(const AT &t);
    template <typename Predicate>
    qsizetype removeIf(Predicate pred);

    inline T *data() { return ptr; }
    inline const T *data() const { return ptr; }
    inline const T *constData() const { return ptr; }
    typedef qsizetype size_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;

    typedef T *iterator;
    typedef const T *const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    inline iterator begin() { return ptr; }
    inline const_iterator begin() const { return ptr; }
    inline const_iterator cbegin() const { return ptr; }
    inline const_iterator constBegin() const { return ptr; }
    inline iterator end() { return ptr + s; }
    inline const_iterator end() const { return ptr + s; }
    inline const_iterator cend() const { return ptr + s; }
    inline const_iterator constEnd() const { return ptr + s; }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }
    iterator insert(const_iterator before, qsizetype n, const T &x);
    iterator insert(const_iterator before, T &&x);
    inline iterator insert(const_iterator before, const T &x) { return insert(before, 1, x); }
    iterator erase(const_iterator begin, const_iterator end);
    inline iterator erase(const_iterator pos) { return erase(pos, pos + 1); }

    // STL compatibility:
    inline bool empty() const { return isEmpty(); }
    inline void push_back(const T &t) { append(t); }
    void push_back(T &&t) { append(std::move(t)); }
    inline void pop_back() { removeLast(); }
    inline T &front() { return first(); }
    inline const T &front() const { return first(); }
    inline T &back() { return last(); }
    inline const T &back() const { return last(); }
    void shrink_to_fit() { squeeze(); }

#ifdef Q_QDOC
    template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
    friend inline bool operator==(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r);
    template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
    friend inline bool operator!=(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r);
    template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
    friend inline bool operator< (const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r);
    template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
    friend inline bool operator> (const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r);
    template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
    friend inline bool operator<=(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r);
    template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
    friend inline bool operator>=(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r);
#else
    template <typename U = T, qsizetype Prealloc2 = Prealloc> friend
    QTypeTraits::compare_eq_result<U> operator==(const QVarLengthArray<T, Prealloc> &l, const QVarLengthArray<T, Prealloc2> &r)
    {
        if (l.size() != r.size())
            return false;
        const T *rb = r.begin();
        const T *b  = l.begin();
        const T *e  = l.end();
        return std::equal(b, e, QT_MAKE_CHECKED_ARRAY_ITERATOR(rb, r.size()));
    }

    template <typename U = T, qsizetype Prealloc2 = Prealloc> friend
    QTypeTraits::compare_eq_result<U> operator!=(const QVarLengthArray<T, Prealloc> &l, const QVarLengthArray<T, Prealloc2> &r)
    {
        return !(l == r);
    }

    template <typename U = T, qsizetype Prealloc2 = Prealloc> friend
    QTypeTraits::compare_lt_result<U> operator<(const QVarLengthArray<T, Prealloc> &lhs, const QVarLengthArray<T, Prealloc2> &rhs)
        noexcept(noexcept(std::lexicographical_compare(lhs.begin(), lhs.end(),
                                                       rhs.begin(), rhs.end())))
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                            rhs.begin(), rhs.end());
    }

    template <typename U = T, qsizetype Prealloc2 = Prealloc> friend
    QTypeTraits::compare_lt_result<U> operator>(const QVarLengthArray<T, Prealloc> &lhs, const QVarLengthArray<T, Prealloc2> &rhs)
        noexcept(noexcept(lhs < rhs))
    {
        return rhs < lhs;
    }

    template <typename U = T, qsizetype Prealloc2 = Prealloc> friend
    QTypeTraits::compare_lt_result<U> operator<=(const QVarLengthArray<T, Prealloc> &lhs, const QVarLengthArray<T, Prealloc2> &rhs)
        noexcept(noexcept(lhs < rhs))
    {
        return !(lhs > rhs);
    }

    template <typename U = T, qsizetype Prealloc2 = Prealloc> friend
    QTypeTraits::compare_lt_result<U> operator>=(const QVarLengthArray<T, Prealloc> &lhs, const QVarLengthArray<T, Prealloc2> &rhs)
        noexcept(noexcept(lhs < rhs))
    {
        return !(lhs < rhs);
    }
#endif

private:
    void reallocate(qsizetype size, qsizetype alloc);

    qsizetype a;      // capacity
    qsizetype s;      // size
    T *ptr;     // data
    std::aligned_storage_t<sizeof(T), alignof(T)> array[Prealloc];

    bool isValidIterator(const const_iterator &i) const
    {
        const std::less<const T *> less = {};
        return !less(cend(), i) && !less(i, cbegin());
    }
};

#if defined(__cpp_deduction_guides) && __cpp_deduction_guides >= 201606
template <typename InputIterator,
          typename ValueType = typename std::iterator_traits<InputIterator>::value_type,
          QtPrivate::IfIsInputIterator<InputIterator> = true>
QVarLengthArray(InputIterator, InputIterator) -> QVarLengthArray<ValueType>;
#endif

template <class T, qsizetype Prealloc>
Q_INLINE_TEMPLATE QVarLengthArray<T, Prealloc>::QVarLengthArray(qsizetype asize)
    : s(asize) {
    static_assert(Prealloc > 0, "QVarLengthArray Prealloc must be greater than 0.");
    Q_ASSERT_X(s >= 0, "QVarLengthArray::QVarLengthArray()", "Size must be greater than or equal to 0.");
    if (s > Prealloc) {
        ptr = reinterpret_cast<T *>(malloc(s * sizeof(T)));
        Q_CHECK_PTR(ptr);
        a = s;
    } else {
        ptr = reinterpret_cast<T *>(array);
        a = Prealloc;
    }
    if (QTypeInfo<T>::isComplex) {
        T *i = ptr + s;
        while (i != ptr)
            new (--i) T;
    }
}

template <class T, qsizetype Prealloc>
Q_INLINE_TEMPLATE void QVarLengthArray<T, Prealloc>::resize(qsizetype asize)
{ reallocate(asize, qMax(asize, a)); }

template <class T, qsizetype Prealloc>
Q_INLINE_TEMPLATE void QVarLengthArray<T, Prealloc>::reserve(qsizetype asize)
{ if (asize > a) reallocate(s, asize); }

template <class T, qsizetype Prealloc>
template <typename AT>
Q_INLINE_TEMPLATE qsizetype QVarLengthArray<T, Prealloc>::indexOf(const AT &t, qsizetype from) const
{
    if (from < 0)
        from = qMax(from + s, qsizetype(0));
    if (from < s) {
        T *n = ptr + from - 1;
        T *e = ptr + s;
        while (++n != e)
            if (*n == t)
                return n - ptr;
    }
    return -1;
}

template <class T, qsizetype Prealloc>
template <typename AT>
Q_INLINE_TEMPLATE qsizetype QVarLengthArray<T, Prealloc>::lastIndexOf(const AT &t, qsizetype from) const
{
    if (from < 0)
        from += s;
    else if (from >= s)
        from = s - 1;
    if (from >= 0) {
        T *b = ptr;
        T *n = ptr + from + 1;
        while (n != b) {
            if (*--n == t)
                return n - b;
        }
    }
    return -1;
}

template <class T, qsizetype Prealloc>
template <typename AT>
Q_INLINE_TEMPLATE bool QVarLengthArray<T, Prealloc>::contains(const AT &t) const
{
    T *b = ptr;
    T *i = ptr + s;
    while (i != b) {
        if (*--i == t)
            return true;
    }
    return false;
}

template <class T, qsizetype Prealloc>
Q_OUTOFLINE_TEMPLATE void QVarLengthArray<T, Prealloc>::append(const T *abuf, qsizetype increment)
{
    Q_ASSERT(abuf);
    if (increment <= 0)
        return;

    const qsizetype asize = s + increment;

    if (asize >= a)
        reallocate(s, qMax(s * 2, asize));

    if constexpr (QTypeInfo<T>::isComplex)
        std::uninitialized_copy_n(abuf, increment, ptr + s);
    else
        memcpy(static_cast<void *>(&ptr[s]), static_cast<const void *>(abuf), increment * sizeof(T));

    s = asize;
}

template <class T, qsizetype Prealloc>
Q_INLINE_TEMPLATE void QVarLengthArray<T, Prealloc>::squeeze()
{ reallocate(s, s); }

template <class T, qsizetype Prealloc>
Q_OUTOFLINE_TEMPLATE void QVarLengthArray<T, Prealloc>::reallocate(qsizetype asize, qsizetype aalloc)
{
    Q_ASSERT(aalloc >= asize);
    Q_ASSERT(ptr);
    T *oldPtr = ptr;
    qsizetype osize = s;

    const qsizetype copySize = qMin(asize, osize);
    Q_ASSUME(copySize >= 0);
    if (aalloc != a) {
        if (aalloc > Prealloc) {
            T *newPtr = reinterpret_cast<T *>(malloc(aalloc * sizeof(T)));
            Q_CHECK_PTR(newPtr); // could throw
            // by design: in case of QT_NO_EXCEPTIONS malloc must not fail or it crashes here
            ptr = newPtr;
            a = aalloc;
        } else {
            ptr = reinterpret_cast<T *>(array);
            a = Prealloc;
        }
        s = 0;
        if (!QTypeInfo<T>::isRelocatable) {
            QT_TRY {
                // move all the old elements
                while (s < copySize) {
                    new (ptr+s) T(std::move(*(oldPtr+s)));
                    (oldPtr+s)->~T();
                    s++;
                }
            } QT_CATCH(...) {
                // clean up all the old objects and then free the old ptr
                qsizetype sClean = s;
                while (sClean < osize)
                    (oldPtr+(sClean++))->~T();
                if (oldPtr != reinterpret_cast<T *>(array) && oldPtr != ptr)
                    free(oldPtr);
                QT_RETHROW;
            }
        } else {
            memcpy(static_cast<void *>(ptr), static_cast<const void *>(oldPtr), copySize * sizeof(T));
        }
    }
    s = copySize;

    // destroy remaining old objects
    if constexpr (QTypeInfo<T>::isComplex) {
        if (osize > asize)
            std::destroy(oldPtr + asize, oldPtr + osize);
    }

    if (oldPtr != reinterpret_cast<T *>(array) && oldPtr != ptr)
        free(oldPtr);

    if (QTypeInfo<T>::isComplex) {
        // call default constructor for new objects (which can throw)
        while (s < asize)
            new (ptr+(s++)) T;
    } else {
        s = asize;
    }
}

template <class T, qsizetype Prealloc>
Q_OUTOFLINE_TEMPLATE T QVarLengthArray<T, Prealloc>::value(qsizetype i) const
{
    if (size_t(i) >= size_t(size()))
        return T();
    return at(i);
}
template <class T, qsizetype Prealloc>
Q_OUTOFLINE_TEMPLATE T QVarLengthArray<T, Prealloc>::value(qsizetype i, const T &defaultValue) const
{
    return (size_t(i) >= size_t(size())) ? defaultValue : at(i);
}

template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::insert(qsizetype i, T &&t)
{ Q_ASSERT_X(i >= 0 && i <= s, "QVarLengthArray::insert", "index out of range");
  insert(cbegin() + i, std::move(t)); }
template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::insert(qsizetype i, const T &t)
{ Q_ASSERT_X(i >= 0 && i <= s, "QVarLengthArray::insert", "index out of range");
  insert(begin() + i, 1, t); }
template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::insert(qsizetype i, qsizetype n, const T &t)
{ Q_ASSERT_X(i >= 0 && i <= s, "QVarLengthArray::insert", "index out of range");
  insert(begin() + i, n, t); }
template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::remove(qsizetype i, qsizetype n)
{ Q_ASSERT_X(i >= 0 && n >= 0 && i + n <= s, "QVarLengthArray::remove", "index out of range");
  erase(begin() + i, begin() + i + n); }
template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::remove(qsizetype i)
{ Q_ASSERT_X(i >= 0 && i < s, "QVarLengthArray::remove", "index out of range");
  erase(begin() + i, begin() + i + 1); }
template <class T, qsizetype Prealloc>
template <typename AT>
inline qsizetype QVarLengthArray<T, Prealloc>::removeAll(const AT &t)
{ return QtPrivate::sequential_erase_with_copy(*this, t); }
template <class T, qsizetype Prealloc>
template <typename AT>
inline bool QVarLengthArray<T, Prealloc>::removeOne(const AT &t)
{ return QtPrivate::sequential_erase_one(*this, t); }
template <class T, qsizetype Prealloc>
template <typename Predicate>
inline qsizetype QVarLengthArray<T, Prealloc>::removeIf(Predicate pred)
{ return QtPrivate::sequential_erase_if(*this, pred); }
template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::prepend(T &&t)
{ insert(cbegin(), std::move(t)); }
template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::prepend(const T &t)
{ insert(begin(), 1, t); }

template <class T, qsizetype Prealloc>
inline void QVarLengthArray<T, Prealloc>::replace(qsizetype i, const T &t)
{
    Q_ASSERT_X(i >= 0 && i < s, "QVarLengthArray::replace", "index out of range");
    const T copy(t);
    data()[i] = copy;
}

template <class T, qsizetype Prealloc>
Q_OUTOFLINE_TEMPLATE typename QVarLengthArray<T, Prealloc>::iterator QVarLengthArray<T, Prealloc>::insert(const_iterator before, T &&t)
{
    Q_ASSERT_X(isValidIterator(before), "QVarLengthArray::insert", "The specified const_iterator argument 'before' is invalid");

    qsizetype offset = qsizetype(before - ptr);
    reserve(s + 1);
    if (!QTypeInfo<T>::isRelocatable) {
        T *b = ptr + offset;
        T *i = ptr + s;
        T *j = i + 1;
        // The new end-element needs to be constructed, the rest must be move assigned
        if (i != b) {
            new (--j) T(std::move(*--i));
            while (i != b)
                *--j = std::move(*--i);
            *b = std::move(t);
        } else {
            new (b) T(std::move(t));
        }
    } else {
        T *b = ptr + offset;
        memmove(static_cast<void *>(b + 1), static_cast<const void *>(b), (s - offset) * sizeof(T));
        new (b) T(std::move(t));
    }
    s += 1;
    return ptr + offset;
}

template <class T, qsizetype Prealloc>
Q_OUTOFLINE_TEMPLATE typename QVarLengthArray<T, Prealloc>::iterator QVarLengthArray<T, Prealloc>::insert(const_iterator before, size_type n, const T &t)
{
    Q_ASSERT_X(isValidIterator(before), "QVarLengthArray::insert", "The specified const_iterator argument 'before' is invalid");

    qsizetype offset = qsizetype(before - ptr);
    if (n != 0) {
        resize(s + n);
        const T copy(t);
        if (!QTypeInfo<T>::isRelocatable) {
            T *b = ptr + offset;
            T *j = ptr + s;
            T *i = j - n;
            while (i != b)
                *--j = *--i;
            i = b + n;
            while (i != b)
                *--i = copy;
        } else {
            T *b = ptr + offset;
            T *i = b + n;
            memmove(static_cast<void *>(i), static_cast<const void *>(b), (s - offset - n) * sizeof(T));
            while (i != b)
                new (--i) T(copy);
        }
    }
    return ptr + offset;
}

template <class T, qsizetype Prealloc>
Q_OUTOFLINE_TEMPLATE typename QVarLengthArray<T, Prealloc>::iterator QVarLengthArray<T, Prealloc>::erase(const_iterator abegin, const_iterator aend)
{
    Q_ASSERT_X(isValidIterator(abegin), "QVarLengthArray::insert", "The specified const_iterator argument 'abegin' is invalid");
    Q_ASSERT_X(isValidIterator(aend), "QVarLengthArray::insert", "The specified const_iterator argument 'aend' is invalid");

    qsizetype f = qsizetype(abegin - ptr);
    qsizetype l = qsizetype(aend - ptr);
    qsizetype n = l - f;

    if constexpr (QTypeInfo<T>::isComplex) {
        std::move(ptr + l, ptr + s, QT_MAKE_CHECKED_ARRAY_ITERATOR(ptr + f, s - f));
        std::destroy(ptr + s - n, ptr + s);
    } else {
        memmove(static_cast<void *>(ptr + f), static_cast<const void *>(ptr + l), (s - l) * sizeof(T));
    }
    s -= n;
    return ptr + f;
}

#ifdef Q_QDOC
// Fake definitions for qdoc, only the redeclaration is used.
template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
bool operator==(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r)
{ return bool{}; }
template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
bool operator!=(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r)
{ return bool{}; }
template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
bool operator< (const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r)
{ return bool{}; }
template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
bool operator> (const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r)
{ return bool{}; }
template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
bool operator<=(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r)
{ return bool{}; }
template <typename T, qsizetype Prealloc1, qsizetype Prealloc2>
bool operator>=(const QVarLengthArray<T, Prealloc1> &l, const QVarLengthArray<T, Prealloc2> &r)
{ return bool{}; }
#endif

template <typename T, qsizetype Prealloc>
size_t qHash(const QVarLengthArray<T, Prealloc> &key, size_t seed = 0)
    noexcept(noexcept(qHashRange(key.cbegin(), key.cend(), seed)))
{
    return qHashRange(key.cbegin(), key.cend(), seed);
}

template <typename T, qsizetype Prealloc, typename AT>
qsizetype erase(QVarLengthArray<T, Prealloc> &array, const AT &t)
{
    return QtPrivate::sequential_erase(array, t);
}

template <typename T, qsizetype Prealloc, typename Predicate>
qsizetype erase_if(QVarLengthArray<T, Prealloc> &array, Predicate pred)
{
    return QtPrivate::sequential_erase_if(array, pred);
}

QT_END_NAMESPACE

#endif // QVARLENGTHARRAY_H
