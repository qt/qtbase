/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#ifndef QARRAYDATAOPS_H
#define QARRAYDATAOPS_H

#include <QtCore/qarraydata.h>
#include <QtCore/qcontainertools_impl.h>

#include <algorithm>
#include <new>
#include <string.h>
#include <utility>
#include <iterator>
#include <tuple>
#include <type_traits>

QT_BEGIN_NAMESPACE

template <class T> struct QArrayDataPointer;

namespace QtPrivate {

/*!
    \internal

    This class provides basic building blocks to do reversible operations. This
    in turn allows to reason about exception safety under certain conditions.

    This class is not part of the Qt API. It exists for the convenience of other
    Qt classes.  This class may change from version to version without notice,
    or even be removed.

    We mean it.
 */
template<typename T>
struct QArrayExceptionSafetyPrimitives
{
    using parameter_type = typename QArrayDataPointer<T>::parameter_type;
    using iterator = typename QArrayDataPointer<T>::iterator;

    // Constructs a range of elements at the specified position. If an exception
    // is thrown during construction, already constructed elements are
    // destroyed. By design, only one function (create/copy/clone/move) and only
    // once is supposed to be called per class instance.
    struct Constructor
    {
        T *const where;
        size_t n = 0;

        template<typename It>
        using iterator_copy_value = decltype(*std::declval<It>());
        template<typename It>
        using iterator_move_value = decltype(std::move(*std::declval<It>()));

        Constructor(T *w) noexcept : where(w) {}
        qsizetype create(size_t size) noexcept(std::is_nothrow_default_constructible_v<T>)
        {
            n = 0;
            while (n != size) {
                new (where + n) T;
                ++n;
            }
            return qsizetype(std::exchange(n, 0));
        }
        template<typename ForwardIt>
        qsizetype copy(ForwardIt first, ForwardIt last)
            noexcept(std::is_nothrow_constructible_v<T, iterator_copy_value<ForwardIt>>)
        {
            n = 0;
            for (; first != last; ++first) {
                new (where + n) T(*first);
                ++n;
            }
            return qsizetype(std::exchange(n, 0));
        }
        qsizetype clone(size_t size, parameter_type t)
            noexcept(std::is_nothrow_constructible_v<T, parameter_type>)
        {
            n = 0;
            while (n != size) {
                new (where + n) T(t);
                ++n;
            }
            return qsizetype(std::exchange(n, 0));
        }
        template<typename ForwardIt>
        qsizetype move(ForwardIt first, ForwardIt last)
            noexcept(std::is_nothrow_constructible_v<T, iterator_move_value<ForwardIt>>)
        {
            n = 0;
            for (; first != last; ++first) {
                new (where + n) T(std::move(*first));
                ++n;
            }
            return qsizetype(std::exchange(n, 0));
        }
        ~Constructor() noexcept(std::is_nothrow_destructible_v<T>)
        {
            while (n)
                where[--n].~T();
        }
    };

    // Watches passed iterator. Unless commit() is called, all the elements that
    // the watched iterator passes through are deleted at the end of object
    // lifetime.
    //
    // Requirements: when not at starting position, the iterator is expected to
    //               point to a valid object (to initialized memory)
    template<typename It = iterator>
    struct Destructor
    {
        It *iter;
        It end;
        It intermediate;

        Destructor(It &it) noexcept(std::is_nothrow_copy_constructible_v<It>)
            : iter(std::addressof(it)), end(it)
        { }
        void commit() noexcept
        {
            iter = std::addressof(end);
        }
        void freeze() noexcept(std::is_nothrow_copy_constructible_v<It>)
        {
            intermediate = *iter; iter = std::addressof(intermediate);
        }
        ~Destructor() noexcept(std::is_nothrow_destructible_v<T>)
        {
            // Step is either 1 or -1 and depends on the interposition of *iter
            // and end. Note that *iter is expected to point to a valid object
            // (see the logic below).
            for (const int step = *iter < end ? 1 : -1; *iter != end; std::advance(*iter, step))
                (*iter)->~T();
        }
    };

    // Moves the data range in memory by the specified amount. Unless commit()
    // is called, the data is moved back to the original place at the end of
    // object lifetime.
    struct Displacer
    {
        T *const begin;
        T *const end;
        qsizetype displace;

        static_assert(QTypeInfo<T>::isRelocatable, "Type must be relocatable");

        Displacer(T *start, T *finish, qsizetype diff) noexcept
            : begin(start), end(finish), displace(diff)
        {
            if (displace)
                ::memmove(static_cast<void *>(begin + displace), static_cast<void *>(begin),
                          (end - begin) * sizeof(T));
        }
        void commit() noexcept { displace = 0; }
        ~Displacer() noexcept
        {
            if (displace)
                ::memmove(static_cast<void *>(begin), static_cast<void *>(begin + displace),
                          (end - begin) * sizeof(T));
        }
    };

    // Watches passed iterator. Moves the data range (defined as a start
    // iterator and a length) to the new starting position at the end of object
    // lifetime.
    struct Mover
    {
        T *&destination;
        const T *const source;
        size_t n;
        qsizetype &size;

        static_assert(QTypeInfo<T>::isRelocatable, "Type must be relocatable");

        Mover(T *&start, size_t length, qsizetype &sz) noexcept
            : destination(start), source(start), n(length), size(sz)
        { }
        ~Mover() noexcept
        {
            if (destination != source)
                ::memmove(static_cast<void *>(destination), static_cast<const void *>(source),
                          n * sizeof(T));
            size -= source > destination ? source - destination : destination - source;
        }
    };
};

// Tags for compile-time dispatch based on backwards vs forward growing policy
struct GrowsForwardTag {};
struct GrowsBackwardsTag {};
template<typename> struct InverseTag;
template<> struct InverseTag<GrowsForwardTag> { using tag = GrowsBackwardsTag; };
template<> struct InverseTag<GrowsBackwardsTag> { using tag = GrowsForwardTag; };

QT_WARNING_PUSH
#if defined(Q_CC_GNU) && Q_CC_GNU >= 700
QT_WARNING_DISABLE_GCC("-Wstringop-overflow")
#endif

template <class T>
struct QPodArrayOps
        : public QArrayDataPointer<T>
{
protected:
    typedef QTypedArrayData<T> Data;

    template <typename ...Args>
    void createInPlace(T *where, Args&&... args) { new (where) T(std::forward<Args>(args)...); }

public:
    typedef typename QArrayDataPointer<T>::parameter_type parameter_type;

    void appendInitialize(qsizetype newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > this->size);
        Q_ASSERT(newSize - this->size <= this->freeSpaceAtEnd());

        ::memset(static_cast<void *>(this->end()), 0, (newSize - this->size) * sizeof(T));
        this->size = qsizetype(newSize);
    }

    void moveAppend(T *b, T *e)
    { insert(this->end(), b, e); }

    void truncate(size_t newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize < size_t(this->size));

        this->size = qsizetype(newSize);
    }

    void destroyAll() // Call from destructors, ONLY!
    {
        Q_ASSERT(this->d);
        Q_ASSERT(this->d->ref_.loadRelaxed() == 0);

        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.
    }

    void insert(T *where, const T *b, const T *e)
    { insert(GrowsForwardTag{}, where, b, e); }

    void insert(GrowsForwardTag, T *where, const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || (b == e && where == this->end()));
        Q_ASSERT(!this->isShared() || (b == e && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        if (where != this->end())
            ::memmove(static_cast<void *>(where + (e - b)), static_cast<void *>(where),
                      (static_cast<const T*>(this->end()) - where) * sizeof(T));
        ::memcpy(static_cast<void *>(where), static_cast<const void *>(b), (e - b) * sizeof(T));
        this->size += (e - b);
    }

    void insert(GrowsBackwardsTag, T *where, const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || (b == e && where == this->end()));
        Q_ASSERT(!this->isShared() || (b == e && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtBegin());

        const T *oldBegin = this->begin();
        this->ptr -= (e - b);
        if (where != oldBegin)
            ::memmove(static_cast<void *>(this->begin()), static_cast<const void *>(oldBegin),
                      (where - oldBegin) * sizeof(T));
        ::memcpy(static_cast<void *>(where - (e - b)), static_cast<const void *>(b),
                 (e - b) * sizeof(T));
        this->size += (e - b);
    }

    void insert(T *where, size_t n, parameter_type t)
    { insert(GrowsForwardTag{}, where, n, t); }

    void insert(GrowsForwardTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(n);
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtEnd()) >= n);

        if (where != this->end())
            ::memmove(static_cast<void *>(where + n), static_cast<void *>(where),
                      (static_cast<const T*>(this->end()) - where) * sizeof(T));
        this->size += qsizetype(n); // PODs can't throw on copy
        while (n--)
            *where++ = t;
    }

    void insert(GrowsBackwardsTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(n);
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtBegin()) >= n);

        const T *oldBegin = this->begin();
        this->ptr -= n;
        if (where != oldBegin)
            ::memmove(static_cast<void *>(this->begin()), static_cast<const void *>(oldBegin),
                      (where - oldBegin) * sizeof(T));
        this->size += qsizetype(n); // PODs can't throw on copy
        where -= n;
        while (n--)
            *where++ = t;
    }

    template <typename ...Args>
    void emplace(T *where, Args&&... args)
    { emplace(GrowsForwardTag{}, where, std::forward<Args>(args)...); }

    template <typename ...Args>
    void emplace(GrowsForwardTag, T *where, Args&&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtEnd() >= 1);

        if (where == this->end()) {
            new (this->end()) T(std::forward<Args>(args)...);
        } else {
            // Preserve the value, because it might be a reference to some part of the moved chunk
            T t(std::forward<Args>(args)...);

            ::memmove(static_cast<void *>(where + 1), static_cast<void *>(where),
                      (static_cast<const T*>(this->end()) - where) * sizeof(T));
            *where = t;
        }

        ++this->size;
    }

    template <typename ...Args>
    void emplace(GrowsBackwardsTag, T *where, Args&&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtBegin() >= 1);

        if (where == this->begin()) {
            new (this->begin() - 1) T(std::forward<Args>(args)...);
            --this->ptr;
        } else {
            // Preserve the value, because it might be a reference to some part of the moved chunk
            T t(std::forward<Args>(args)...);

            auto oldBegin = this->begin();
            --this->ptr;
            ::memmove(static_cast<void *>(this->begin()), static_cast<void *>(oldBegin), (where - oldBegin) * sizeof(T));
            *(where - 1) = t;
        }

        ++this->size;
    }

    void erase(T *b, T *e)
    { erase(GrowsForwardTag{}, b, e); }

    void erase(GrowsForwardTag, T *b, T *e)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        if (e != this->end())
            ::memmove(static_cast<void *>(b), static_cast<void *>(e), (static_cast<T *>(this->end()) - e) * sizeof(T));
        this->size -= (e - b);
    }

    void erase(GrowsBackwardsTag, T *b, T *e)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        const auto oldBegin = this->begin();
        this->ptr += (e - b);
        if (b != oldBegin)
            ::memmove(static_cast<void *>(this->begin()), static_cast<void *>(oldBegin), (b - static_cast<T *>(oldBegin)) * sizeof(T));
        this->size -= (e - b);
    }

    void eraseFirst()
    {
        Q_ASSERT(this->size);
        ++this->ptr;
        --this->size;
    }

    void eraseLast()
    {
        Q_ASSERT(this->size);
        --this->size;
    }

    void assign(T *b, T *e, parameter_type t)
    {
        Q_ASSERT(b <= e);
        Q_ASSERT(b >= this->begin() && e <= this->end());

        while (b != e)
            ::memcpy(static_cast<void *>(b++), static_cast<const void *>(&t), sizeof(T));
    }

    bool compare(const T *begin1, const T *begin2, size_t n) const
    {
        // only use memcmp for fundamental types or pointers.
        // Other types could have padding in the data structure or custom comparison
        // operators that would break the comparison using memcmp
        if constexpr (QArrayDataPointer<T>::pass_parameter_by_value) {
            return ::memcmp(begin1, begin2, n * sizeof(T)) == 0;
        } else {
            const T *end1 = begin1 + n;
            while (begin1 != end1) {
                if (*begin1 == *begin2) {
                    ++begin1;
                    ++begin2;
                } else {
                    return false;
                }
            }
            return true;
        }
    }

    void reallocate(qsizetype alloc, QArrayData::AllocationOption option)
    {
        auto pair = Data::reallocateUnaligned(this->d, this->ptr, alloc, option);
        Q_ASSERT(pair.first != nullptr);
        this->d = pair.first;
        this->ptr = pair.second;
    }
};
QT_WARNING_POP

template <class T>
struct QGenericArrayOps
        : public QArrayDataPointer<T>
{
protected:
    typedef QTypedArrayData<T> Data;

    template <typename ...Args>
    void createInPlace(T *where, Args&&... args) { new (where) T(std::forward<Args>(args)...); }

public:
    typedef typename QArrayDataPointer<T>::parameter_type parameter_type;

    void appendInitialize(qsizetype newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > this->size);
        Q_ASSERT(newSize - this->size <= this->freeSpaceAtEnd());

        T *const b = this->begin();
        do {
            new (b + this->size) T;
        } while (++this->size != newSize);
    }

    void moveAppend(T *b, T *e)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        typedef typename QArrayExceptionSafetyPrimitives<T>::Constructor CopyConstructor;

        // Provides strong exception safety guarantee,
        // provided T::~T() nothrow

        CopyConstructor copier(this->end());
        this->size += copier.move(b, e);
    }

    void truncate(size_t newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize < size_t(this->size));

        const T *const b = this->begin();
        do {
            (b + --this->size)->~T();
        } while (size_t(this->size) != newSize);
    }

    void destroyAll() // Call from destructors, ONLY
    {
        Q_ASSERT(this->d);
        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.

        Q_ASSERT(this->d->ref_.loadRelaxed() == 0);

        const T *const b = this->begin();
        const T *i = this->end();

        while (i != b)
            (--i)->~T();
    }

    void insert(T *where, const T *b, const T *e)
    { insert(GrowsForwardTag{}, where, b, e); }

    void insert(GrowsForwardTag, T *where, const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || (b == e && where == this->end()));
        Q_ASSERT(!this->isShared() || (b == e && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        // Array may be truncated at where in case of exceptions

        T *end = this->end();
        T *readIter = end;
        T *writeIter = end + (e - b);

        const T *const step1End = where + qMax(e - b, end - where);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            --readIter;
            // If exception happens on construction, we should not call ~T()
            new (writeIter - 1) T(std::move(*readIter));
            --writeIter;
        }

        while (writeIter != end) {
            --e;
            // If exception happens on construction, we should not call ~T()
            new (writeIter - 1) T(*e);
            --writeIter;
        }

        destroyer.commit();
        this->size += destroyer.end - end;

        // Copy assign over existing elements
        while (readIter != where) {
            --readIter;
            --writeIter;
            *writeIter = std::move(*readIter);
        }

        while (writeIter != where) {
            --e;
            --writeIter;
            *writeIter = *e;
        }
    }

    void insert(GrowsBackwardsTag, T *where, const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || (b == e && where == this->end()));
        Q_ASSERT(!this->isShared() || (b == e && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtBegin());

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        T *begin = this->begin();
        T *readIter = begin;
        T *writeIter = begin - (e - b);

        const T *const step1End = where - qMax(e - b, where - begin);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            new (writeIter) T(std::move(*readIter));
            ++readIter;
            ++writeIter;
        }

        while (writeIter != begin) {
            new (writeIter) T(*b);
            ++b;
            ++writeIter;
        }

        destroyer.commit();
        this->size += begin - destroyer.end;
        this->ptr -= begin - destroyer.end;

        // Copy assign over existing elements
        while (readIter != where) {
            *writeIter = std::move(*readIter);
            ++readIter;
            ++writeIter;
        }

        while (writeIter != where) {
            *writeIter = *b;
            ++b;
            ++writeIter;
        }
    }

    void insert(T *where, size_t n, parameter_type t)
    { insert(GrowsForwardTag{}, where, n, t); }

    void insert(GrowsForwardTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(n);
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtEnd()) >= n);

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        // Array may be truncated at where in case of exceptions
        T *end = this->end();
        T *readIter = end;
        T *writeIter = end + n;

        const T *const step1End = where + qMax<size_t>(n, end - where);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            --readIter;
            // If exception happens on construction, we should not call ~T()
            new (writeIter - 1) T(std::move(*readIter));
            --writeIter;
        }

        while (writeIter != end) {
            --n;
            // If exception happens on construction, we should not call ~T()
            new (writeIter - 1) T(t);
            --writeIter;
        }

        destroyer.commit();
        this->size += destroyer.end - end;

        // Copy assign over existing elements
        while (readIter != where) {
            --readIter;
            --writeIter;
            *writeIter = std::move(*readIter);
        }

        while (writeIter != where) {
            --n;
            --writeIter;
            *writeIter = t;
        }
    }

    void insert(GrowsBackwardsTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(n);
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtBegin()) >= n);

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        T *begin = this->begin();
        T *readIter = begin;
        T *writeIter = begin - n;

        const T *const step1End = where - qMax<size_t>(n, where - begin);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            new (writeIter) T(std::move(*readIter));
            ++readIter;
            ++writeIter;
        }

        while (writeIter != begin) {
            new (writeIter) T(t);
            ++writeIter;
        }

        destroyer.commit();
        this->size += begin - destroyer.end;
        this->ptr -= begin - destroyer.end;

        // Copy assign over existing elements
        while (readIter != where) {
            *writeIter = std::move(*readIter);
            ++readIter;
            ++writeIter;
        }

        while (writeIter != where) {
            *writeIter = t;
            ++writeIter;
        }
    }

    template<typename... Args>
    void emplace(T *where, Args &&... args)
    { emplace(GrowsForwardTag{}, where, std::forward<Args>(args)...); }

    template<typename... Args>
    void emplace(GrowsForwardTag, T *where, Args &&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtEnd() >= 1);

        if (where == this->end()) {
            createInPlace(this->end(), std::forward<Args>(args)...);
            ++this->size;
        } else {
            T tmp(std::forward<Args>(args)...);

            T *const end = this->end();
            T *readIter = end - 1;
            T *writeIter = end;

            // Create new element at the end
            new (writeIter) T(std::move(*readIter));
            ++this->size;

            // Move assign over existing elements
            while (readIter != where) {
                --readIter;
                --writeIter;
                *writeIter = std::move(*readIter);
            }

            // Assign new element
            --writeIter;
            *writeIter = std::move(tmp);
        }
    }

    template<typename... Args>
    void emplace(GrowsBackwardsTag, T *where, Args &&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtBegin() >= 1);

        if (where == this->begin()) {
            createInPlace(this->begin() - 1, std::forward<Args>(args)...);
            --this->ptr;
            ++this->size;
        } else {
            T tmp(std::forward<Args>(args)...);

            T *const begin = this->begin();
            T *readIter = begin;
            T *writeIter = begin - 1;

            // Create new element at the beginning
            new (writeIter) T(std::move(*readIter));
            --this->ptr;
            ++this->size;

            ++readIter;
            ++writeIter;

            // Move assign over existing elements
            while (readIter != where) {
                *writeIter = std::move(*readIter);
                ++readIter;
                ++writeIter;
            }

            // Assign new element
            *writeIter = std::move(tmp);
        }
    }

    void erase(T *b, T *e)
    { erase(GrowsForwardTag{}, b, e); }

    void erase(GrowsForwardTag, T *b, T *e)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        const T *const end = this->end();

        // move (by assignment) the elements from e to end
        // onto b to the new end
        while (e != end) {
            *b = std::move(*e);
            ++b;
            ++e;
        }

        // destroy the final elements at the end
        // here, b points to the new end and e to the actual end
        do {
            // Exceptions or not, dtor called once per instance
            --this->size;
            (--e)->~T();
        } while (e != b);
    }

    void erase(GrowsBackwardsTag, T *b, T *e)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        const T *const begin = this->begin();

        // move (by assignment) the elements from begin to b
        // onto the new begin to e
        while (b != begin) {
            --b;
            --e;
            *e = std::move(*b);
        }

        // destroy the final elements at the begin
        // here, e points to the new begin and b to the actual begin
        do {
            // Exceptions or not, dtor called once per instance
            ++this->ptr;
            --this->size;
            (b++)->~T();
        } while (b != e);
    }

    void eraseFirst()
    {
        Q_ASSERT(this->size);
        this->begin()->~T();
        ++this->ptr;
        --this->size;
    }

    void eraseLast()
    {
        Q_ASSERT(this->size);
        (--this->end())->~T();
        --this->size;
    }


    void assign(T *b, T *e, parameter_type t)
    {
        Q_ASSERT(b <= e);
        Q_ASSERT(b >= this->begin() && e <= this->end());

        while (b != e)
            *b++ = t;
    }

    bool compare(const T *begin1, const T *begin2, size_t n) const
    {
        const T *end1 = begin1 + n;
        while (begin1 != end1) {
            if (*begin1 == *begin2) {
                ++begin1;
                ++begin2;
            } else {
                return false;
            }
        }
        return true;
    }
};

template <class T>
struct QMovableArrayOps
    : QGenericArrayOps<T>
{
protected:
    typedef QTypedArrayData<T> Data;

public:
    // using QGenericArrayOps<T>::appendInitialize;
    // using QGenericArrayOps<T>::copyAppend;
    // using QGenericArrayOps<T>::moveAppend;
    // using QGenericArrayOps<T>::truncate;
    // using QGenericArrayOps<T>::destroyAll;
    typedef typename QGenericArrayOps<T>::parameter_type parameter_type;

    void insert(T *where, const T *b, const T *e)
    { insert(GrowsForwardTag{}, where, b, e); }

    void insert(GrowsForwardTag, T *where, const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || (b == e && where == this->end()));
        Q_ASSERT(!this->isShared() || (b == e && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;
        typedef typename QArrayExceptionSafetyPrimitives<T>::Constructor CopyConstructor;

        // Provides strong exception safety guarantee,
        // provided T::~T() nothrow

        ReversibleDisplace displace(where, this->end(), e - b);
        CopyConstructor copier(where);
        const auto copiedSize = copier.copy(b, e);
        displace.commit();
        this->size += copiedSize;
    }

    void insert(GrowsBackwardsTag, T *where, const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || (b == e && where == this->end()));
        Q_ASSERT(!this->isShared() || (b == e && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtBegin());

        typedef typename QArrayExceptionSafetyPrimitives<T>::Constructor CopyConstructor;
        typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;

        // Provides strong exception safety guarantee,
        // provided T::~T() nothrow

        ReversibleDisplace displace(this->begin(), where, -(e - b));
        CopyConstructor copier(where - (e - b));
        const auto copiedSize = copier.copy(b, e);
        displace.commit();
        this->ptr -= copiedSize;
        this->size += copiedSize;
    }

    void insert(T *where, size_t n, parameter_type t)
    { insert(GrowsForwardTag{}, where, n, t); }

    void insert(GrowsForwardTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(n);
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtEnd()) >= n);

        typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;
        typedef typename QArrayExceptionSafetyPrimitives<T>::Constructor CopyConstructor;

        // Provides strong exception safety guarantee,
        // provided T::~T() nothrow

        ReversibleDisplace displace(where, this->end(), qsizetype(n));
        CopyConstructor copier(where);
        const auto copiedSize = copier.clone(n, t);
        displace.commit();
        this->size += copiedSize;
    }

    void insert(GrowsBackwardsTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(n);
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtBegin()) >= n);

        typedef typename QArrayExceptionSafetyPrimitives<T>::Constructor CopyConstructor;
        typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;

        // Provides strong exception safety guarantee,
        // provided T::~T() nothrow

        ReversibleDisplace displace(this->begin(), where, -qsizetype(n));
        CopyConstructor copier(where - n);
        const auto copiedSize = copier.clone(n, t);
        displace.commit();
        this->ptr -= copiedSize;
        this->size += copiedSize;
    }

    // use moving insert
    using QGenericArrayOps<T>::insert;

    template<typename... Args>
    void emplace(T *where, Args &&... args)
    {
        emplace(GrowsForwardTag {}, where, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void emplace(GrowsForwardTag, T *where, Args &&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtEnd() >= 1);

        if (where == this->end()) {
            this->createInPlace(where, std::forward<Args>(args)...);
        } else {
            T tmp(std::forward<Args>(args)...);
            typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;
            ReversibleDisplace displace(where, this->end(), 1);
            this->createInPlace(where, std::move(tmp));
            displace.commit();
        }
        ++this->size;
    }

    template<typename... Args>
    void emplace(GrowsBackwardsTag, T *where, Args &&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtBegin() >= 1);

        if (where == this->begin()) {
            this->createInPlace(where - 1, std::forward<Args>(args)...);
        } else {
            T tmp(std::forward<Args>(args)...);
            typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;
            ReversibleDisplace displace(this->begin(), where, -1);
            this->createInPlace(where - 1, std::move(tmp));
            displace.commit();
        }
        --this->ptr;
        ++this->size;
    }

    // use moving emplace
    using QGenericArrayOps<T>::emplace;

    void erase(T *b, T *e)
    { erase(GrowsForwardTag{}, b, e); }

    void erase(GrowsForwardTag, T *b, T *e)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        typedef typename QArrayExceptionSafetyPrimitives<T>::Mover Mover;

        Mover mover(e, static_cast<const T *>(this->end()) - e, this->size);

        // destroy the elements we're erasing
        do {
            // Exceptions or not, dtor called once per instance
            (--e)->~T();
        } while (e != b);
    }

    void erase(GrowsBackwardsTag, T *b, T *e)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        typedef typename QArrayExceptionSafetyPrimitives<T>::Mover Mover;

        Mover mover(this->ptr, b - static_cast<const T *>(this->begin()), this->size);

        // destroy the elements we're erasing
        do {
            // Exceptions or not, dtor called once per instance
            ++this->ptr;
            (b++)->~T();
        } while (b != e);
    }

    void reallocate(qsizetype alloc, QArrayData::AllocationOption option)
    {
        auto pair = Data::reallocateUnaligned(this->d, this->ptr, alloc, option);
        Q_ASSERT(pair.first != nullptr);
        this->d = pair.first;
        this->ptr = pair.second;
    }
};

template <class T, class = void>
struct QArrayOpsSelector
{
    typedef QGenericArrayOps<T> Type;
};

template <class T>
struct QArrayOpsSelector<T,
    typename std::enable_if<
        !QTypeInfo<T>::isComplex && QTypeInfo<T>::isRelocatable
    >::type>
{
    typedef QPodArrayOps<T> Type;
};

template <class T>
struct QArrayOpsSelector<T,
    typename std::enable_if<
        QTypeInfo<T>::isComplex && QTypeInfo<T>::isRelocatable
    >::type>
{
    typedef QMovableArrayOps<T> Type;
};

template <class T>
struct QCommonArrayOps : QArrayOpsSelector<T>::Type
{
    using Base = typename QArrayOpsSelector<T>::Type;
    using Data = QTypedArrayData<T>;
    using DataPointer = QArrayDataPointer<T>;
    using parameter_type = typename Base::parameter_type;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

protected:
    using Self = QCommonArrayOps<T>;

    // Tag dispatched helper functions
    inline void adjustPointer(GrowsBackwardsTag, size_t distance) noexcept
    {
        this->ptr -= distance;
    }
    inline void adjustPointer(GrowsForwardTag, size_t distance) noexcept
    {
        this->ptr += distance;
    }
    qsizetype freeSpace(GrowsBackwardsTag) const noexcept { return this->freeSpaceAtBegin(); }
    qsizetype freeSpace(GrowsForwardTag) const noexcept { return this->freeSpaceAtEnd(); }

    // Tells how much of the given size to insert at the beginning of the
    // container. This is insert-specific helper function
    qsizetype sizeToInsertAtBegin(const T *const where, qsizetype maxSize)
    {
        Q_ASSERT(maxSize <= this->allocatedCapacity() - this->size);
        Q_ASSERT(where >= this->begin() && where <= this->end());  // in range

        const auto freeAtBegin = this->freeSpaceAtBegin();
        const auto freeAtEnd = this->freeSpaceAtEnd();

        // Idea: * if enough space on both sides, try to affect less elements
        //       * if enough space on one of the sides, use only that side
        //       * otherwise, split between front and back (worst case)
        if (freeAtBegin >= maxSize && freeAtEnd >= maxSize) {
            if (where - this->begin() < this->end() - where) {
                return maxSize;
            } else {
                return 0;
            }
        } else if (freeAtBegin >= maxSize) {
            return maxSize;
        } else if (freeAtEnd >= maxSize) {
            return 0;
        } else {
            return maxSize - freeAtEnd;
        }
    }

public:

    // does the iterator point into this array?
    template <typename It>
    bool iteratorPointsIntoArray(const It &it)
    {
        using DecayedIt = std::decay_t<It>;
        using RemovedConstVolatileIt = std::remove_cv_t<It>;
        constexpr bool selfIterator =
            // if passed type is an iterator type:
            std::is_same_v<DecayedIt, iterator>
            || std::is_same_v<DecayedIt, const_iterator>
            // if passed type is a pointer type:
            || std::is_same_v<RemovedConstVolatileIt, T *>
            || std::is_same_v<RemovedConstVolatileIt, const T *>
            || std::is_same_v<RemovedConstVolatileIt, const volatile T *>;
        if constexpr (selfIterator) {
            return (it >= this->begin() && it <= this->end());
        } else {
            return false;
        }
    }

    // using Base::truncate;
    // using Base::destroyAll;
    // using Base::assign;
    // using Base::compare;

    void appendInitialize(qsizetype newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > this->size);
        Q_ASSERT(newSize - this->size <= this->freeSpaceAtEnd());

        Base::appendInitialize(newSize);
    }

    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        if (b == e) // short-cut and handling the case b and e == nullptr
            return;

        Base::insert(GrowsForwardTag{}, this->end(), b, e);
    }

    template<typename It>
    void copyAppend(It b, It e, QtPrivate::IfIsForwardIterator<It> = true,
                    QtPrivate::IfIsNotConvertible<It, const T *> = true,
                    QtPrivate::IfIsNotConvertible<It, const T *> = true)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);

        const qsizetype distance = std::distance(b, e);
        Q_ASSERT(distance >= 0 && distance <= this->allocatedCapacity() - this->size);

        T *iter = this->end();
        for (; b != e; ++iter, ++b) {
            new (iter) T(*b);
            ++this->size;
        }
    }

    void moveAppend(T *b, T *e)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= this->allocatedCapacity() - this->size);

        if (b == e) // short-cut and handling the case b and e == nullptr
            return;

        Base::moveAppend(b, e);
    }

    void copyAppend(size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared() || n == 0);
        Q_ASSERT(size_t(this->allocatedCapacity() - this->size) >= n);
        if (!n)
            return;

        Base::insert(GrowsForwardTag{}, this->end(), n, t);
    }

public:
    void insert(qsizetype i, qsizetype n, parameter_type t)
    {
        T copy(t);

        typename Data::GrowthPosition pos = Data::GrowsAtEnd;
        if (this->size != 0 && i <= (this->size >> 1))
            pos = Data::GrowsAtBeginning;
        this->detachAndGrow(pos, n);
        Q_ASSERT((pos == Data::GrowsAtBeginning && this->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && this->freeSpaceAtEnd() >= n));

        T *where = this->begin() + i;
        if (pos == QArrayData::GrowsAtBeginning)
            Base::insert(GrowsBackwardsTag{}, where, n, copy);
        else
            Base::insert(GrowsForwardTag{}, where, n, copy);
    }

    void insert(qsizetype i, const T *data, qsizetype n)
    {
        typename Data::GrowthPosition pos = Data::GrowsAtEnd;
        if (this->size != 0 && i <= (this->size >> 1))
            pos = Data::GrowsAtBeginning;
        DataPointer oldData;
        this->detachAndGrow(pos, n, &oldData);
        Q_ASSERT((pos == Data::GrowsAtBeginning && this->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && this->freeSpaceAtEnd() >= n));

        T *where = this->begin() + i;
        if (pos == QArrayData::GrowsAtBeginning)
            Base::insert(GrowsBackwardsTag{}, where, data, data + n);
        else
            Base::insert(GrowsForwardTag{}, where, data, data + n);
    }

    template<typename... Args>
    void emplace(T *where, Args &&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->allocatedCapacity() - this->size >= 1);

        const T *begin = this->begin();
        const T *end = this->end();
        // Qt5 QList in insert(1): try to move less data around
        // Now:
        const bool shouldInsertAtBegin =
                (where - begin) < (end - where) || this->freeSpaceAtEnd() <= 0;
        if (this->freeSpaceAtBegin() > 0 && shouldInsertAtBegin) {
            Base::emplace(GrowsBackwardsTag{}, where, std::forward<Args>(args)...);
        } else {
            Base::emplace(GrowsForwardTag{}, where, std::forward<Args>(args)...);
        }
    }

    template <typename ...Args>
    void emplaceBack(Args&&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(this->freeSpaceAtEnd() >= 1);
        new (this->end()) T(std::forward<Args>(args)...);
        ++this->size;
    }

    template <typename ...Args>
    void emplaceFront(Args&&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(this->freeSpaceAtBegin() >= 1);
        new (this->ptr - 1) T(std::forward<Args>(args)...);
        --this->ptr;
        ++this->size;
    }

    void erase(T *b, T *e)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        // Comply with std::vector::erase(): erased elements and all after them
        // are invalidated. However, erasing from the beginning effectively
        // means that all iterators are invalidated. We can use this freedom to
        // erase by moving towards the end.
        if (b == this->begin()) {
            Base::erase(GrowsBackwardsTag{}, b, e);
        } else {
            Base::erase(GrowsForwardTag{}, b, e);
        }
    }

    void eraseFirst()
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(this->size);
        Base::eraseFirst();
    }

    void eraseLast()
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(this->size);
        Base::eraseLast();
    }

};

} // namespace QtPrivate

template <class T>
struct QArrayDataOps
    : QtPrivate::QCommonArrayOps<T>
{
};

QT_END_NAMESPACE

#endif // include guard
