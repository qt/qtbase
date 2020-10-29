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

    void appendInitialize(size_t newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > size_t(this->size));
        Q_ASSERT(newSize - this->size <= size_t(this->freeSpaceAtEnd()));

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
        Q_ASSERT(b <= e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

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
        Q_ASSERT(b <= e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtBegin());

        auto oldBegin = this->begin();
        this->ptr -= (e - b);
        ::memmove(static_cast<void *>(this->begin()), static_cast<void *>(oldBegin),
                  (where - static_cast<const T*>(oldBegin)) * sizeof(T));
        ::memcpy(static_cast<void *>(where - (e - b)), static_cast<const void *>(b),
                 (e - b) * sizeof(T));
        this->size += (e - b);
    }

    void insert(T *where, size_t n, parameter_type t)
    { insert(GrowsForwardTag{}, where, n, t); }

    void insert(GrowsForwardTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared() || (n == 0 && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtEnd()) >= n);

        ::memmove(static_cast<void *>(where + n), static_cast<void *>(where),
                  (static_cast<const T*>(this->end()) - where) * sizeof(T));
        this->size += qsizetype(n); // PODs can't throw on copy
        while (n--)
            *where++ = t;
    }

    void insert(GrowsBackwardsTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared() || (n == 0 && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtBegin()) >= n);

        auto oldBegin = this->begin();
        this->ptr -= n;
        ::memmove(static_cast<void *>(this->begin()), static_cast<void *>(oldBegin),
                  (where - static_cast<const T*>(oldBegin)) * sizeof(T));
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
            ::memmove(static_cast<void *>(this->begin()), static_cast<void *>(oldBegin),
                      (where - oldBegin) * sizeof(T));
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

        ::memmove(static_cast<void *>(b), static_cast<void *>(e),
                  (static_cast<T *>(this->end()) - e) * sizeof(T));
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
        ::memmove(static_cast<void *>(this->begin()), static_cast<void *>(oldBegin),
                  (b - static_cast<T *>(oldBegin)) * sizeof(T));
        this->size -= (e - b);
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

    void reallocate(qsizetype alloc, typename Data::ArrayOptions options)
    {
        // when reallocating, take care of the situation when no growth is
        // happening - need to move the data in this case, unfortunately
        const bool grows = options & (Data::GrowsForward | Data::GrowsBackwards);

        // ### optimize me: there may be cases when moving is not obligatory
        if (const auto gap = this->freeSpaceAtBegin(); this->d && !grows && gap) {
            auto oldBegin = this->begin();
            this->ptr -= gap;
            ::memmove(static_cast<void *>(this->begin()), static_cast<void *>(oldBegin),
                      this->size * sizeof(T));
        }

        auto pair = Data::reallocateUnaligned(this->d, this->ptr, alloc, options);
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

    void appendInitialize(size_t newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > size_t(this->size));
        Q_ASSERT(newSize - this->size <= size_t(this->freeSpaceAtEnd()));

        T *const b = this->begin();
        do {
            new (b + this->size) T;
        } while (size_t(++this->size) != newSize);
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
        Q_ASSERT(b <= e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        // Array may be truncated at where in case of exceptions

        T *const end = this->end();
        const T *readIter = end;
        T *writeIter = end + (e - b);

        const T *const step1End = where + qMax(e - b, end - where);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            --readIter;
            // If exception happens on construction, we should not call ~T()
            new (writeIter - 1) T(*readIter);
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
            *writeIter = *readIter;
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
        Q_ASSERT(b <= e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->freeSpaceAtBegin());

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        T *const begin = this->begin();
        const T *readIter = begin;
        T *writeIter = begin - (e - b);

        const T *const step1End = where - qMax(e - b, where - begin);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            new (writeIter) T(*readIter);
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
            *writeIter = *readIter;
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
        Q_ASSERT(!this->isShared() || (n == 0 && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtEnd()) >= n);

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        // Array may be truncated at where in case of exceptions
        T *const end = this->end();
        const T *readIter = end;
        T *writeIter = end + n;

        const T *const step1End = where + qMax<size_t>(n, end - where);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            --readIter;
            // If exception happens on construction, we should not call ~T()
            new (writeIter - 1) T(*readIter);
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
            *writeIter = *readIter;
        }

        while (writeIter != where) {
            --n;
            --writeIter;
            *writeIter = t;
        }
    }

    void insert(GrowsBackwardsTag, T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared() || (n == 0 && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->freeSpaceAtBegin()) >= n);

        typedef typename QArrayExceptionSafetyPrimitives<T>::template Destructor<T *> Destructor;

        T *const begin = this->begin();
        const T *readIter = begin;
        T *writeIter = begin - n;

        const T *const step1End = where - qMax<size_t>(n, where - begin);

        Destructor destroyer(writeIter);

        // Construct new elements in array
        while (writeIter != step1End) {
            new (writeIter) T(*readIter);
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
            *writeIter = *readIter;
            ++readIter;
            ++writeIter;
        }

        while (writeIter != where) {
            *writeIter = t;
            ++writeIter;
        }
    }


    template <typename iterator, typename ...Args>
    void emplace(iterator where, Args&&... args)
    { emplace(GrowsForwardTag{}, where, std::forward<Args>(args)...); }

    template <typename iterator, typename ...Args>
    void emplace(GrowsForwardTag, iterator where, Args&&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtEnd() >= 1);

        createInPlace(this->end(), std::forward<Args>(args)...);
        ++this->size;

        std::rotate(where, this->end() - 1, this->end());
    }

    template <typename iterator, typename ...Args>
    void emplace(GrowsBackwardsTag, iterator where, Args&&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtBegin() >= 1);

        createInPlace(this->begin() - 1, std::forward<Args>(args)...);
        --this->ptr;
        ++this->size;

        std::rotate(this->begin(), this->begin() + 1, where);
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
            *b = *e;
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
            *e = *b;
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
        Q_ASSERT(b <= e);
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
        Q_ASSERT(b <= e);
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
        Q_ASSERT(!this->isShared() || (n == 0 && where == this->end()));
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
        Q_ASSERT(!this->isShared() || (n == 0 && where == this->end()));
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

    struct RelocatableMoveOps
    {
        // The necessary evil. Performs move "to the left" when grows backwards and
        // move "to the right" when grows forward
        template<typename GrowthTag>
        static void moveInGrowthDirection(GrowthTag tag, Self *this_, size_t futureGrowth)
        {
            Q_ASSERT(this_->isMutable());
            Q_ASSERT(!this_->isShared());
            Q_ASSERT(futureGrowth <= size_t(this_->freeSpace(tag)));

            const auto oldBegin = this_->begin();
            this_->adjustPointer(tag, futureGrowth);

            // Note: move all elements!
            ::memmove(static_cast<void *>(this_->begin()), static_cast<const void *>(oldBegin),
                      this_->size * sizeof(T));
        }
    };

    struct GenericMoveOps
    {
        template <typename ...Args>
        static void createInPlace(T *where, Args&&... args)
        {
            new (where) T(std::forward<Args>(args)...);
        }

        template <typename ...Args>
        static void createInPlace(std::reverse_iterator<iterator> where, Args&&... args)
        {
            // Note: instead of std::addressof(*where)
            createInPlace(where.base() - 1, std::forward<Args>(args)...);
        }

        // Moves non-pod data range. Handles overlapping regions. By default, expect
        // this method to perform move to the _right_. When move to the _left_ is
        // needed, use reverse iterators.
        template<typename GrowthTag, typename It>
        static void moveNonPod(GrowthTag, Self *this_, It where, It begin, It end)
        {
            Q_ASSERT(begin <= end);
            Q_ASSERT(where > begin);  // move to the right

            using Destructor = typename QArrayExceptionSafetyPrimitives<T>::template Destructor<It>;

            auto start = where + std::distance(begin, end);
            auto e = end;

            Destructor destroyer(start);  // Keep track of added items

            auto [oldRangeEnd, overlapStart] = std::minmax(where, end);

            // step 1. move-initialize elements in uninitialized memory region
            while (start != overlapStart) {
                --e;
                createInPlace(start - 1, std::move_if_noexcept(*e));
                // change tracked iterator only after creation succeeded - avoid
                // destructing partially constructed objects if exception thrown
                --start;
            }

            // re-created the range. now there is an initialized memory region
            // somewhere in the allocated area. if something goes wrong, we must
            // clean it up, so "freeze" the position for now (cannot commit yet)
            destroyer.freeze();

            // step 2. move assign over existing elements in the overlapping
            //         region (if there's an overlap)
            while (e != begin) {
                --start;
                --e;
                *start = std::move_if_noexcept(*e);
            }

            // step 3. destroy elements in the old range
            const qsizetype originalSize = this_->size;
            start = begin; // delete elements in reverse order to prevent any gaps
            while (start != oldRangeEnd) {
                // Exceptions or not, dtor called once per instance
                if constexpr (std::is_same_v<std::decay_t<GrowthTag>, GrowsForwardTag>)
                    ++this_->ptr;
                --this_->size;
                (start++)->~T();
            }

            destroyer.commit();
            // restore old size as we consider data move to be done, the pointer
            // still has to be adjusted!
            this_->size = originalSize;
        }

        // Super inefficient function. The necessary evil. Performs move "to
        // the left" when grows backwards and move "to the right" when grows
        // forward
        template<typename GrowthTag>
        static void moveInGrowthDirection(GrowthTag tag, Self *this_, size_t futureGrowth)
        {
            Q_ASSERT(this_->isMutable());
            Q_ASSERT(!this_->isShared());
            Q_ASSERT(futureGrowth <= size_t(this_->freeSpace(tag)));

            if (futureGrowth == 0)  // avoid doing anything if there's no need
                return;

            // Note: move all elements!
            if constexpr (std::is_same_v<std::decay_t<GrowthTag>, GrowsBackwardsTag>) {
                auto where = this_->begin() - futureGrowth;
                // here, magic happens. instead of having move to the right, we'll
                // have move to the left by using reverse iterators
                moveNonPod(tag, this_,
                           std::make_reverse_iterator(where + this_->size),  // rwhere
                           std::make_reverse_iterator(this_->end()),  // rbegin
                           std::make_reverse_iterator(this_->begin()));  // rend
                this_->ptr = where;
            } else {
                auto where = this_->begin() + futureGrowth;
                moveNonPod(tag, this_, where, this_->begin(), this_->end());
                this_->ptr = where;
            }
        }
    };

    // Moves all elements in a specific direction by moveSize if available
    // free space at one of the ends is smaller than required. Free space
    // becomes available at the beginning if grows backwards and at the end
    // if grows forward
    template<typename GrowthTag>
    qsizetype prepareFreeSpace(GrowthTag tag, size_t required, size_t moveSize)
    {
        Q_ASSERT(this->isMutable() || required == 0);
        Q_ASSERT(!this->isShared() || required == 0);
        Q_ASSERT(required <= size_t(this->constAllocatedCapacity() - this->size));

        using MoveOps = std::conditional_t<QTypeInfo<T>::isRelocatable,
                                           RelocatableMoveOps,
                                           GenericMoveOps>;

        // if free space at the end is not enough, we need to move the data,
        // move is performed in an inverse direction
        if (size_t(freeSpace(tag)) < required) {
            using MoveTag = typename InverseTag<std::decay_t<GrowthTag>>::tag;
            MoveOps::moveInGrowthDirection(MoveTag{}, this, moveSize);

            if constexpr (std::is_same_v<MoveTag, GrowsBackwardsTag>) {
                return -qsizetype(moveSize);  // moving data to the left
            } else {
                return  qsizetype(moveSize);  // moving data to the right
            }
        }
        return 0;
    }

    // Helper wrapper that adjusts passed iterators along with moving the data
    // around. The adjustment is necessary when iterators point inside the
    // to-be-moved range
    template<typename GrowthTag, typename It>
    void prepareFreeSpace(GrowthTag tag, size_t required, size_t moveSize, It &b, It &e) {
        // Returns whether passed iterators are inside [this->begin(), this->end()]
        const auto iteratorsInRange = [&] (const It &first, const It &last) {
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
                return (first >= this->begin() && last <= this->end());
            } else {
                return false;
            }
        };

        const bool inRange = iteratorsInRange(b, e);
        const auto diff = prepareFreeSpace(tag, required, moveSize);
        if (inRange) {
            std::advance(b, diff);
            std::advance(e, diff);
        }
    }

    size_t moveSizeForAppend(size_t)
    {
        // Qt5 QList in append: make 100% free space at end if not enough space
        // Now:
        return this->freeSpaceAtBegin();
    }

    size_t moveSizeForPrepend(size_t required)
    {
        // Qt5 QList in prepend: make 33% of all space at front if not enough space
        // Now:
        qsizetype space = this->allocatedCapacity() / 3;
        space = qMax(space, qsizetype(required));  // in case required > 33% of all space
        return qMin(space, this->freeSpaceAtEnd());
    }

    // Helper functions that reduce usage boilerplate
    void prepareSpaceForAppend(size_t required)
    {
        prepareFreeSpace(GrowsForwardTag{}, required, moveSizeForAppend(required));
    }
    void prepareSpaceForPrepend(size_t required)
    {
        prepareFreeSpace(GrowsBackwardsTag{}, required, moveSizeForPrepend(required));
    }
    template<typename It>
    void prepareSpaceForAppend(It &b, It &e, size_t required)
    {
        prepareFreeSpace(GrowsForwardTag{}, required, moveSizeForAppend(required), b, e);
    }
    template<typename It>
    void prepareSpaceForPrepend(It &b, It &e, size_t required)
    {
        prepareFreeSpace(GrowsBackwardsTag{}, required, moveSizeForPrepend(required), b, e);
    }

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
    // Returns whether reallocation is desirable before adding more elements
    // into the container. This is a helper function that one can use to
    // theoretically improve average operations performance. Ignoring this
    // function does not affect the correctness of the array operations.
    bool shouldGrowBeforeInsert(const_iterator where, qsizetype n) const noexcept
    {
        if (this->d == nullptr)
            return true;
        if (this->d->flags & QArrayData::CapacityReserved)
            return false;
        if (!(this->d->flags & (QArrayData::GrowsForward | QArrayData::GrowsBackwards)))
            return false;
        Q_ASSERT(where >= this->begin() && where <= this->end());  // in range

        const qsizetype freeAtBegin = this->freeSpaceAtBegin();
        const qsizetype freeAtEnd = this->freeSpaceAtEnd();

        // Idea: always reallocate when not enough space at the corresponding end
        if (where == this->end()) { // append or size == 0
            return freeAtEnd < n;
        } else if (where == this->begin()) { // prepend
            return freeAtBegin < n;
        } else { // general insert
            return (freeAtBegin < n && freeAtEnd < n);
        }
    }

    // using Base::truncate;
    // using Base::destroyAll;
    // using Base::assign;
    // using Base::compare;

    void appendInitialize(size_t newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > size_t(this->size));
        Q_ASSERT(newSize <= size_t(this->allocatedCapacity()));

        // Since this is mostly an initialization function, do not follow append
        // logic of space arrangement. Instead, only prepare as much free space
        // as needed for this specific operation
        const size_t n = newSize - this->size;
        prepareFreeSpace(GrowsForwardTag{}, n,
                         qMin(n, size_t(this->freeSpaceAtBegin())));  // ### perf. loss

        Base::appendInitialize(newSize);
    }

    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= this->allocatedCapacity() - this->size);
        if (b == e) // short-cut and handling the case b and e == nullptr
            return;

        prepareSpaceForAppend(b, e, e - b);  // ### perf. loss
        Base::insert(GrowsForwardTag{}, this->end(), b, e);
    }

    template<typename It>
    void copyAppend(It b, It e, QtPrivate::IfIsForwardIterator<It> = true,
                    QtPrivate::IfIsNotSame<std::decay_t<It>, iterator> = true,
                    QtPrivate::IfIsNotSame<std::decay_t<It>, const_iterator> = true)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        const qsizetype distance = std::distance(b, e);
        Q_ASSERT(distance >= 0 && distance <= this->allocatedCapacity() - this->size);

        prepareSpaceForAppend(b, e, distance);  // ### perf. loss

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

        prepareSpaceForAppend(b, e, e - b);  // ### perf. loss
        Base::moveAppend(b, e);
    }

    void copyAppend(size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared() || n == 0);
        Q_ASSERT(size_t(this->allocatedCapacity() - this->size) >= n);

        // Preserve the value, because it might be a reference to some part of the moved chunk
        T tmp(t);
        prepareSpaceForAppend(n);  // ### perf. loss
        Base::insert(GrowsForwardTag{}, this->end(), n, tmp);
    }

    void insert(T *where, const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || (b == e && where == this->end()));
        Q_ASSERT(!this->isShared() || (b == e && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(b <= e);
        Q_ASSERT(e <= where || b > this->end() || where == this->end()); // No overlap or append
        Q_ASSERT((e - b) <= this->allocatedCapacity() - this->size);
        if (b == e) // short-cut and handling the case b and e == nullptr
            return;

        if (this->size > 0 && where == this->begin()) {  // prepend case - special space arrangement
            prepareSpaceForPrepend(b, e, e - b);  // ### perf. loss
            Base::insert(GrowsBackwardsTag{}, this->begin(), b, e);
            return;
        } else if (where == this->end()) {  // append case - special space arrangement
            copyAppend(b, e);
            return;
        }

        // Insert elements based on the divided distance. Good case: only 1
        // insert happens (either to the front part or to the back part). Bad
        // case: both inserts happen, meaning that we touch all N elements in
        // the container (this should be handled "outside" by ensuring enough
        // free space by reallocating more frequently)
        const auto k = sizeToInsertAtBegin(where, e - b);
        Base::insert(GrowsBackwardsTag{}, where, b, b + k);
        Base::insert(GrowsForwardTag{}, where, b + k, e);
    }

    void insert(T *where, size_t n, parameter_type t)
    {
        Q_ASSERT(!this->isShared() || (n == 0 && where == this->end()));
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(size_t(this->allocatedCapacity() - this->size) >= n);

        if (this->size > 0 && where == this->begin()) {  // prepend case - special space arrangement
            // Preserve the value, because it might be a reference to some part of the moved chunk
            T tmp(t);
            prepareSpaceForPrepend(n);  // ### perf. loss
            Base::insert(GrowsBackwardsTag{}, this->begin(), n, tmp);
            return;
        } else if (where == this->end()) {  // append case - special space arrangement
            copyAppend(n, t);
            return;
        }

        // Insert elements based on the divided distance. Good case: only 1
        // insert happens (either to the front part or to the back part). Bad
        // case: both inserts happen, meaning that we touch all N elements in
        // the container (this should be handled "outside" by ensuring enough
        // free space by reallocating more frequently)
        const auto beginSize = sizeToInsertAtBegin(where, qsizetype(n));
        Base::insert(GrowsBackwardsTag{}, where, beginSize, t);
        Base::insert(GrowsForwardTag{}, where, qsizetype(n) - beginSize, t);
    }

    template <typename iterator, typename ...Args>
    void emplace(iterator where, Args&&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->allocatedCapacity() - this->size >= 1);

        // Qt5 QList in insert(1): try to move less data around
        // Now:
        const bool shouldInsertAtBegin = (where - this->begin()) < (this->end() - where)
                                         || this->freeSpaceAtEnd() <= 0;
        if (this->freeSpaceAtBegin() > 0 && shouldInsertAtBegin) {
            Base::emplace(GrowsBackwardsTag{}, where, std::forward<Args>(args)...);
        } else {
            Base::emplace(GrowsForwardTag{}, where, std::forward<Args>(args)...);
        }
    }

    template <typename ...Args>
    void emplaceBack(Args&&... args)
    {
        this->emplace(this->end(), std::forward<Args>(args)...);
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
};

} // namespace QtPrivate

template <class T>
struct QArrayDataOps
    : QtPrivate::QCommonArrayOps<T>
{
};

QT_END_NAMESPACE

#endif // include guard
