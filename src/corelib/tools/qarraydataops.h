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

#include <new>
#include <string.h>
#include <utility>
#include <iterator>

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

        Destructor(It &it) noexcept(std::is_nothrow_copy_constructible_v<It>)
            : iter(std::addressof(it)), end(it)
        { }
        void commit() noexcept
        {
            iter = std::addressof(end);
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

        static_assert(QTypeInfoQuery<T>::isRelocatable, "Type must be relocatable");

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

        static_assert(QTypeInfoQuery<T>::isRelocatable, "Type must be relocatable");

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

QT_WARNING_PUSH
#if defined(Q_CC_GNU) && Q_CC_GNU >= 700
QT_WARNING_DISABLE_GCC("-Wstringop-overflow")
#endif

template <class T>
struct QPodArrayOps
        : public QArrayDataPointer<T>
{
private:
    typedef QTypedArrayData<T> Data;
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

    template<typename iterator>
    void copyAppend(iterator b, iterator e, QtPrivate::IfIsForwardIterator<iterator> = true)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(std::distance(b, e) >= 0 && qsizetype(std::distance(b, e)) <= this->freeSpaceAtEnd());

        T *iter = this->end();
        for (; b != e; ++iter, ++b) {
            new (iter) T(*b);
            ++this->size;
        }
    }

    void copyAppend(const T *b, const T *e)
    { insert(this->end(), b, e); }

    void moveAppend(T *b, T *e)
    { copyAppend(b, e); }

    void copyAppend(size_t n, parameter_type t)
    { insert(this->end(), n, t); }

    template <typename ...Args>
    void emplaceBack(Args&&... args) { this->emplace(this->end(), T(std::forward<Args>(args)...)); }

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
    void createInPlace(T *where, Args&&... args) { new (where) T(std::forward<Args>(args)...); }

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
        if (QArrayDataPointer<T>::pass_parameter_by_value)
            return ::memcmp(begin1, begin2, n * sizeof(T)) == 0;
        const T *end1 = begin1 + n;
        while (begin1 != end1) {
            if (*begin1 == *begin2)
                ++begin1, ++begin2;
            else
                return false;
        }
        return true;
    }

    void reallocate(qsizetype alloc, typename Data::ArrayOptions options)
    {
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

    template<typename iterator>
    void copyAppend(iterator b, iterator e, QtPrivate::IfIsForwardIterator<iterator> = true)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(std::distance(b, e) >= 0 && qsizetype(std::distance(b, e)) <= this->freeSpaceAtEnd());

        T *iter = this->end();
        for (; b != e; ++iter, ++b) {
            new (iter) T(*b);
            ++this->size;
        }
    }

    void copyAppend(const T *b, const T *e)
    { insert(this->end(), b, e); }

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

    void copyAppend(size_t n, parameter_type t)
    { insert(this->end(), n, t); }

    template <typename ...Args>
    void emplaceBack(Args&&... args)
    {
        this->emplace(this->end(), std::forward<Args>(args)...);
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
            --readIter, --writeIter;
            *writeIter = *readIter;
        }

        while (writeIter != where) {
            --e, --writeIter;
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
            ++readIter, ++writeIter;
        }

        while (writeIter != begin) {
            new (writeIter) T(*b);
            ++b, ++writeIter;
        }

        destroyer.commit();
        this->size += begin - destroyer.end;
        this->ptr -= begin - destroyer.end;

        // Copy assign over existing elements
        while (readIter != where) {
            *writeIter = *readIter;
            ++readIter, ++writeIter;
        }

        while (writeIter != where) {
            *writeIter = *b;
            ++b, ++writeIter;
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
            --readIter, --writeIter;
            *writeIter = *readIter;
        }

        while (writeIter != where) {
            --n, --writeIter;
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
            ++readIter, ++writeIter;
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
            ++readIter, ++writeIter;
        }

        while (writeIter != where) {
            *writeIter = t;
            ++writeIter;
        }
    }

    template <typename ...Args>
    void createInPlace(T *where, Args&&... args) { new (where) T(std::forward<Args>(args)...); }

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
            ++b, ++e;
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
            --b, --e;
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
            if (*begin1 == *begin2)
                ++begin1, ++begin2;
            else
                return false;
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
        !QTypeInfoQuery<T>::isComplex && QTypeInfoQuery<T>::isRelocatable
    >::type>
{
    typedef QPodArrayOps<T> Type;
};

template <class T>
struct QArrayOpsSelector<T,
    typename std::enable_if<
        QTypeInfoQuery<T>::isComplex && QTypeInfoQuery<T>::isRelocatable
    >::type>
{
    typedef QMovableArrayOps<T> Type;
};

} // namespace QtPrivate

template <class T>
struct QArrayDataOps
    : QtPrivate::QArrayOpsSelector<T>::Type
{
};

QT_END_NAMESPACE

#endif // include guard
