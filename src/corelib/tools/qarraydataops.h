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
        qsizetype copy(const T *first, const T *last) noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            n = 0;
            for (; first != last; ++first) {
                new (where + n) T(*first);
                ++n;
            }
            return qsizetype(std::exchange(n, 0));
        }
        qsizetype clone(size_t size, parameter_type t) noexcept(std::is_nothrow_constructible_v<T, parameter_type>)
        {
            n = 0;
            while (n != size) {
                new (where + n) T(t);
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

QT_WARNING_PUSH
#if defined(Q_CC_GNU) && Q_CC_GNU >= 700
QT_WARNING_DISABLE_GCC("-Wstringop-overflow")
#endif

template <class T>
struct QPodArrayOps
        : public QArrayDataPointer<T>
{
    static_assert (std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

protected:
    typedef QTypedArrayData<T> Data;
    using DataPointer = QArrayDataPointer<T>;

public:
    typedef typename QArrayDataPointer<T>::parameter_type parameter_type;

    void appendInitialize(qsizetype newSize) noexcept
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize > this->size);
        Q_ASSERT(newSize - this->size <= this->freeSpaceAtEnd());

        ::memset(static_cast<void *>(this->end()), 0, (newSize - this->size) * sizeof(T));
        this->size = qsizetype(newSize);
    }

    void copyAppend(const T *b, const T *e) noexcept
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        if (b == e)
            return;

        ::memcpy(static_cast<void *>(this->end()), static_cast<const void *>(b), (e - b) * sizeof(T));
        this->size += (e - b);
    }

    void copyAppend(qsizetype n, parameter_type t) noexcept
    {
        Q_ASSERT(!this->isShared() || n == 0);
        Q_ASSERT(this->freeSpaceAtEnd() >= n);
        if (!n)
            return;

        T *where = this->end();
        this->size += qsizetype(n);
        while (n--)
            *where++ = t;
    }

    void moveAppend(T *b, T *e) noexcept
    {
        copyAppend(b, e);
    }

    void truncate(size_t newSize) noexcept
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize < size_t(this->size));

        this->size = qsizetype(newSize);
    }

    void destroyAll() noexcept // Call from destructors, ONLY!
    {
        Q_ASSERT(this->d);
        Q_ASSERT(this->d->ref_.loadRelaxed() == 0);

        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.
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
            insert(GrowsBackwardsTag{}, where, data, data + n);
        else
            insert(GrowsForwardTag{}, where, data, data + n);
    }

    void insert(GrowsForwardTag, T *where, const T *b, const T *e) noexcept
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

    void insert(GrowsBackwardsTag, T *where, const T *b, const T *e) noexcept
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
            insert(GrowsBackwardsTag{}, where, n, copy);
        else
            insert(GrowsForwardTag{}, where, n, copy);
    }

    void insert(GrowsForwardTag, T *where, size_t n, parameter_type t) noexcept
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

    void insert(GrowsBackwardsTag, T *where, size_t n, parameter_type t) noexcept
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
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e <= this->end());

        // Comply with std::vector::erase(): erased elements and all after them
        // are invalidated. However, erasing from the beginning effectively
        // means that all iterators are invalidated. We can use this freedom to
        // erase by moving towards the end.
        if (b == this->begin())
            this->ptr = e;
        else if (e != this->end())
            ::memmove(static_cast<void *>(b), static_cast<void *>(e), (static_cast<T *>(this->end()) - e) * sizeof(T));
        this->size -= (e - b);
    }

    void eraseFirst() noexcept
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(this->size);
        ++this->ptr;
        --this->size;
    }

    void eraseLast() noexcept
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(this->size);
        --this->size;
    }

    void assign(T *b, T *e, parameter_type t) noexcept
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
    static_assert (std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

protected:
    typedef QTypedArrayData<T> Data;
    using DataPointer = QArrayDataPointer<T>;

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

    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        if (b == e) // short-cut and handling the case b and e == nullptr
            return;

        T *data = this->begin();
        while (b < e) {
            new (data + this->size) T(*b);
            ++b;
            ++this->size;
        }
    }

    void copyAppend(qsizetype n, parameter_type t)
    {
        Q_ASSERT(!this->isShared() || n == 0);
        Q_ASSERT(this->freeSpaceAtEnd() >= n);
        if (!n)
            return;

        T *data = this->begin();
        while (n--) {
            new (data + this->size) T(t);
            ++this->size;
        }
    }

    void moveAppend(T *b, T *e)
    {
        Q_ASSERT(this->isMutable() || b == e);
        Q_ASSERT(!this->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= this->freeSpaceAtEnd());

        if (b == e)
            return;

        T *data = this->begin();
        while (b < e) {
            new (data + this->size) T(std::move(*b));
            ++b;
            ++this->size;
        }
    }

    void truncate(size_t newSize)
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(!this->isShared());
        Q_ASSERT(newSize < size_t(this->size));

        std::destroy(this->begin() + newSize, this->end());
        this->size = newSize;
    }

    void destroyAll() // Call from destructors, ONLY
    {
        Q_ASSERT(this->d);
        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.

        Q_ASSERT(this->d->ref_.loadRelaxed() == 0);

        std::destroy(this->begin(), this->end());
    }

    struct Inserter
    {
        QArrayDataPointer<T> *data;
        qsizetype increment = 1;
        T *begin;
        qsizetype size;

        qsizetype sourceCopyConstruct, nSource, move, sourceCopyAssign;
        T *end, *last, *where;

        Inserter(QArrayDataPointer<T> *d, QArrayData::GrowthPosition pos)
            : data(d), increment(pos == QArrayData::GrowsAtBeginning ? -1 : 1)
        {
            begin = d->ptr;
            size = d->size;
            if (increment < 0)
                begin += size - 1;
        }
        ~Inserter() {
            if (increment < 0)
                begin -= size - 1;
            data->ptr = begin;
            data->size = size;
        }

        void setup(qsizetype pos, qsizetype n)
        {

            if (increment > 0) {
                end = begin + size;
                last = end - 1;
                where = begin + pos;
                qsizetype dist = size - pos;
                sourceCopyConstruct = 0;
                nSource = n;
                move = n - dist; // smaller 0
                sourceCopyAssign = n;
                if (n > dist) {
                    sourceCopyConstruct = n - dist;
                    move = 0;
                    sourceCopyAssign -= sourceCopyConstruct;
                }
            } else {
                end = begin - size;
                last = end + 1;
                where = end + pos;
                sourceCopyConstruct = 0;
                nSource = -n;
                move = pos - n; // larger 0
                sourceCopyAssign = -n;
                if (n > pos) {
                    sourceCopyConstruct = pos - n;
                    move = 0;
                    sourceCopyAssign -= sourceCopyConstruct;
                }
            }
        }

        void insert(qsizetype pos, const T *source, qsizetype n)
        {
            qsizetype oldSize = size;
            Q_UNUSED(oldSize);

            setup(pos, n);
            if (increment < 0)
                source += n - 1;

            // first create new elements at the end, by copying from elements
            // to be inserted (if they extend past the current end of the array)
            for (qsizetype i = 0; i != sourceCopyConstruct; i += increment) {
                new (end + i) T(source[nSource - sourceCopyConstruct + i]);
                ++size;
            }
            Q_ASSERT(size <= oldSize + n);

            // now move construct new elements at the end from existing elements inside
            // the array.
            for (qsizetype i = sourceCopyConstruct; i != nSource; i += increment) {
                new (end + i) T(std::move(*(end + i - nSource)));
                ++size;
            }
            // array has the new size now!
            Q_ASSERT(size == oldSize + n);

            // now move assign existing elements towards the end
            for (qsizetype i = 0; i != move; i -= increment)
                last[i] = std::move(last[i - nSource]);

            // finally copy the remaining elements from source over
            for (qsizetype i = 0; i != sourceCopyAssign; i += increment)
                where[i] = source[i];
        }

        void insert(qsizetype pos, const T &t, qsizetype n)
        {
            qsizetype oldSize = size;
            Q_UNUSED(oldSize);

            setup(pos, n);

            // first create new elements at the end, by copying from elements
            // to be inserted (if they extend past the current end of the array)
            for (qsizetype i = 0; i != sourceCopyConstruct; i += increment) {
                new (end + i) T(t);
                ++size;
            }
            Q_ASSERT(size <= oldSize + n);

            // now move construct new elements at the end from existing elements inside
            // the array.
            for (qsizetype i = sourceCopyConstruct; i != nSource; i += increment) {
                new (end + i) T(std::move(*(end + i - nSource)));
                ++size;
            }
            // array has the new size now!
            Q_ASSERT(size == oldSize + n);

            // now move assign existing elements towards the end
            for (qsizetype i = 0; i != move; i -= increment)
                last[i] = std::move(last[i - nSource]);

            // finally copy the remaining elements from source over
            for (qsizetype i = 0; i != sourceCopyAssign; i += increment)
                where[i] = t;
        }

#if 0
        void insertHole(T *where)
        {
            T *oldEnd = end;

            // create a new element at the end by mov constructing one existing element
            // inside the array.
            new (end) T(std::move(end - increment));
            end += increment;

            // now move existing elements towards the end
            T *to = oldEnd;
            T *from = oldEnd - increment;
            while (from != where) {
                *to = std::move(*from);
                to -= increment;
                from -= increment;
            }
        }
#endif
    };

    void insert(qsizetype i, const T *data, qsizetype n)
    {
        typename Data::GrowthPosition pos = Data::GrowsAtEnd;
        if (this->size != 0 && i <= (this->size >> 1))
            pos = Data::GrowsAtBeginning;
        DataPointer oldData;
        this->detachAndGrow(pos, n, &oldData);
        Q_ASSERT((pos == Data::GrowsAtBeginning && this->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && this->freeSpaceAtEnd() >= n));

        Inserter(this, pos).insert(i, data, n);
    }

    void insert(qsizetype i, qsizetype n, parameter_type t)
    {
        T copy(t);

        typename Data::GrowthPosition pos = Data::GrowsAtEnd;
        if (this->size != 0 && i <= (this->size >> 1))
            pos = Data::GrowsAtBeginning;
        this->detachAndGrow(pos, n);
        Q_ASSERT((pos == Data::GrowsAtBeginning && this->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && this->freeSpaceAtEnd() >= n));

        Inserter(this, pos).insert(i, copy, n);
    }

    template<typename... Args>
    void emplace(GrowsForwardTag, T *where, Args &&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtEnd() >= 1);

        if (where == this->end()) {
            new (this->end()) T(std::forward<Args>(args)...);
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
            new (this->begin() - 1) T(std::forward<Args>(args)...);
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
            this->ptr = e;
        } else {
            const T *const end = this->end();

            // move (by assignment) the elements from e to end
            // onto b to the new end
            while (e != end) {
                *b = std::move(*e);
                ++b;
                ++e;
            }
        }
        this->size -= (e - b);
        std::destroy(b, e);
    }

    void eraseFirst()
    {
        Q_ASSERT(this->isMutable());
        Q_ASSERT(this->size);
        this->begin()->~T();
        ++this->ptr;
        --this->size;
    }

    void eraseLast()
    {
        Q_ASSERT(this->isMutable());
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
    static_assert (std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

protected:
    typedef QTypedArrayData<T> Data;
    using DataPointer = QArrayDataPointer<T>;

public:
    // using QGenericArrayOps<T>::appendInitialize;
    // using QGenericArrayOps<T>::copyAppend;
    // using QGenericArrayOps<T>::moveAppend;
    // using QGenericArrayOps<T>::truncate;
    // using QGenericArrayOps<T>::destroyAll;
    typedef typename QGenericArrayOps<T>::parameter_type parameter_type;

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
            insert(GrowsBackwardsTag{}, where, data, data + n);
        else
            insert(GrowsForwardTag{}, where, data, data + n);
    }

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
            insert(GrowsBackwardsTag{}, where, n, copy);
        else
            insert(GrowsForwardTag{}, where, n, copy);
    }

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
    void emplace(GrowsForwardTag, T *where, Args &&... args)
    {
        Q_ASSERT(!this->isShared());
        Q_ASSERT(where >= this->begin() && where <= this->end());
        Q_ASSERT(this->freeSpaceAtEnd() >= 1);

        if (where == this->end()) {
            new (where) T(std::forward<Args>(args)...);
        } else {
            T tmp(std::forward<Args>(args)...);
            typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;
            ReversibleDisplace displace(where, this->end(), 1);
            new (where) T(std::move(tmp));
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
            new (where - 1) T(std::forward<Args>(args)...);
        } else {
            T tmp(std::forward<Args>(args)...);
            typedef typename QArrayExceptionSafetyPrimitives<T>::Displacer ReversibleDisplace;
            ReversibleDisplace displace(this->begin(), where, -1);
            new (where - 1) T(std::move(tmp));
            displace.commit();
        }
        --this->ptr;
        ++this->size;
    }

    // use moving emplace
    using QGenericArrayOps<T>::emplace;

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

        std::destroy(b, e);
        if (b == this->begin()) {
            this->ptr = e;
        } else if (e != this->end()) {
            memmove(static_cast<void *>(b), static_cast<const void *>(e), (static_cast<const T *>(this->end()) - e)*sizeof(T));
        }
        this->size -= (e - b);
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

    using Base::copyAppend;

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

public:
    template<typename... Args>
    void emplace(qsizetype i, Args &&... args)
    {
        typename QArrayData::GrowthPosition pos = QArrayData::GrowsAtEnd;
        if (this->size != 0 && i <= (this->size >> 1))
            pos = QArrayData::GrowsAtBeginning;
        if (this->needsDetach() ||
            (pos == QArrayData::GrowsAtBeginning && !this->freeSpaceAtBegin()) ||
            (pos == QArrayData::GrowsAtEnd && !this->freeSpaceAtEnd())) {
            T tmp(std::forward<Args>(args)...);
            this->reallocateAndGrow(pos, 1);

            T *where = this->begin() + i;
            if (pos == QArrayData::GrowsAtBeginning)
                Base::emplace(GrowsBackwardsTag{}, where, std::move(tmp));
            else
                Base::emplace(GrowsForwardTag{}, where, std::move(tmp));
        } else {
            T *where = this->begin() + i;
            if (pos == QArrayData::GrowsAtBeginning)
                Base::emplace(GrowsBackwardsTag{}, where, std::forward<Args>(args)...);
            else
                Base::emplace(GrowsForwardTag{}, where, std::forward<Args>(args)...);
        }
    }

    template <typename ...Args>
    void emplaceBack(Args&&... args)
    {
        if (this->needsDetach() || !this->freeSpaceAtEnd()) {
            // protect against args being an element of the container
            T tmp(std::forward<Args>(args)...);
            this->reallocateAndGrow(QArrayData::GrowsAtEnd, 1);
            Q_ASSERT(!this->isShared());
            Q_ASSERT(this->freeSpaceAtEnd() >= 1);
            new (this->end()) T(std::move(tmp));
        } else {
            new (this->end()) T(std::forward<Args>(args)...);
        }
        ++this->size;
    }

    template <typename ...Args>
    void emplaceFront(Args&&... args)
    {
        if (this->needsDetach() || !this->freeSpaceAtBegin()) {
            // protect against args being an element of the container
            T tmp(std::forward<Args>(args)...);
            this->reallocateAndGrow(QArrayData::GrowsAtBeginning, 1);
            Q_ASSERT(!this->isShared());
            Q_ASSERT(this->freeSpaceAtBegin() >= 1);
            new (this->ptr - 1) T(std::move(tmp));
        } else {
            new (this->ptr - 1) T(std::forward<Args>(args)...);
        }
        --this->ptr;
        ++this->size;
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
