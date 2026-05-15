// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QARRAYDATAOPS_H
#define QARRAYDATAOPS_H

#include <QtCore/qarraydata.h>
#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qnamespace.h>

#include <QtCore/q20functional.h>
#include <QtCore/q20memory.h>
#include <new>
#include <string.h>
#include <utility>
#include <iterator>
#include <type_traits>

QT_BEGIN_NAMESPACE

template <class T> struct QArrayDataPointer;

namespace QtPrivate {

template <class T>
struct QPodArrayOps
        : public QArrayDataPointer<T>
{
    static_assert (std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

protected:
    typedef QTypedArrayData<T> Data;
    using DataPointer = QArrayDataPointer<T>;

public:
    DataPointer *that()
    { return this; }
    const DataPointer *that() const
    { return this; }

    typedef typename QArrayDataPointer<T>::parameter_type parameter_type;

    void copyAppend(const T *b, const T *e) noexcept
    {
        Q_ASSERT(that()->isMutable() || b == e);
        Q_ASSERT(!that()->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= that()->freeSpaceAtEnd());

        if (b == e)
            return;

        ::memcpy(static_cast<void *>(that()->end()), static_cast<const void *>(b), (e - b) * sizeof(T));
        that()->size += (e - b);
    }

    void copyAppend(qsizetype n, parameter_type t) noexcept
    {
        Q_ASSERT(!that()->isShared() || n == 0);
        Q_ASSERT(that()->freeSpaceAtEnd() >= n);
        if (!n)
            return;

        T *where = that()->end();
        that()->size += qsizetype(n);
        while (n--)
            *where++ = t;
    }

    void moveAppend(T *b, T *e) noexcept
    {
        copyAppend(b, e);
    }

    void truncate(size_t newSize) noexcept
    {
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(!that()->isShared());
        Q_ASSERT(newSize <= size_t(that()->size));

        that()->size = qsizetype(newSize);
    }

    void destroyAll() noexcept // Call from destructors, ONLY!
    {
        Q_ASSERT(that()->d);
        Q_ASSERT(that()->d->ref_.loadRelaxed() == 0);

        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.
    }

    T *createHole(QArrayData::GrowthPosition pos, qsizetype where, qsizetype n)
    {
        Q_ASSERT((pos == QArrayData::GrowsAtBeginning && n <= that()->freeSpaceAtBegin()) ||
                 (pos == QArrayData::GrowsAtEnd && n <= that()->freeSpaceAtEnd()));

        T *insertionPoint = that()->ptr + where;
        if (pos == QArrayData::GrowsAtEnd) {
            if (where < that()->size)
                ::memmove(static_cast<void *>(insertionPoint + n), static_cast<void *>(insertionPoint), (that()->size - where) * sizeof(T));
        } else {
            Q_ASSERT(where == 0);
            that()->ptr -= n;
            insertionPoint -= n;
        }
        that()->size += n;
        return insertionPoint;
    }

    void insert(qsizetype i, const T *data, qsizetype n)
    {
        typename Data::GrowthPosition pos = Data::GrowsAtEnd;
        if (that()->size != 0 && i == 0)
            pos = Data::GrowsAtBeginning;

        DataPointer oldData;
        that()->detachAndGrow(pos, n, &data, &oldData);
        Q_ASSERT((pos == Data::GrowsAtBeginning && that()->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && that()->freeSpaceAtEnd() >= n));

        T *where = createHole(pos, i, n);
        ::memcpy(static_cast<void *>(where), static_cast<const void *>(data), n * sizeof(T));
    }

    void insert(qsizetype i, qsizetype n, parameter_type t)
    {
        T copy(t);

        typename Data::GrowthPosition pos = Data::GrowsAtEnd;
        if (that()->size != 0 && i == 0)
            pos = Data::GrowsAtBeginning;

        that()->detachAndGrow(pos, n, nullptr, nullptr);
        Q_ASSERT((pos == Data::GrowsAtBeginning && that()->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && that()->freeSpaceAtEnd() >= n));

        T *where = createHole(pos, i, n);
        while (n--)
            *where++ = copy;
    }

    template<typename... Args>
    void emplace(qsizetype i, Args &&... args)
    {
        bool detach = that()->needsDetach();
        if (!detach) {
            if (i == that()->size && that()->freeSpaceAtEnd()) {
                new (that()->end()) T(std::forward<Args>(args)...);
                ++that()->size;
                return;
            }
            if (i == 0 && that()->freeSpaceAtBegin()) {
                new (that()->begin() - 1) T(std::forward<Args>(args)...);
                --that()->ptr;
                ++that()->size;
                return;
            }
        }
        T tmp(std::forward<Args>(args)...);
        typename QArrayData::GrowthPosition pos = QArrayData::GrowsAtEnd;
        if (that()->size != 0 && i == 0)
            pos = QArrayData::GrowsAtBeginning;

        that()->detachAndGrow(pos, 1, nullptr, nullptr);

        T *where = createHole(pos, i, 1);
        new (where) T(std::move(tmp));
    }

    void erase(T *b, qsizetype n)
    {
        T *e = b + n;
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= that()->begin() && b < that()->end());
        Q_ASSERT(e > that()->begin() && e <= that()->end());

        // Comply with std::vector::erase(): erased elements and all after them
        // are invalidated. However, erasing from the beginning effectively
        // means that all iterators are invalidated. We can use this freedom to
        // erase by moving towards the end.
        if (b == that()->begin() && e != that()->end()) {
            that()->ptr = e;
        } else if (e != that()->end()) {
            ::memmove(static_cast<void *>(b), static_cast<void *>(e),
                      (static_cast<T *>(that()->end()) - e) * sizeof(T));
        }
        that()->size -= n;
    }

    void eraseFirst() noexcept
    {
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(that()->size);
        ++that()->ptr;
        --that()->size;
    }

    void eraseLast() noexcept
    {
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(that()->size);
        --that()->size;
    }

    template <typename Predicate>
    qsizetype eraseIf(Predicate pred)
    {
        qsizetype result = 0;
        if (that()->size == 0)
            return result;

        if (!that()->needsDetach()) {
            auto end = that()->end();
            auto it = std::remove_if(that()->begin(), end, pred);
            if (it != end) {
                result = std::distance(it, end);
                erase(it, result);
            }
        } else {
            const auto begin = that()->begin();
            const auto end = that()->end();
            auto it = std::find_if(begin, end, pred);
            if (it == end)
                return result;

            QArrayDataPointer<T> other(that()->size);
            Q_CHECK_PTR(other.data());
            auto dest = other.begin();
            // std::uninitialized_copy will fallback to ::memcpy/memmove()
            dest = std::uninitialized_copy(begin, it, dest);
            dest = q_uninitialized_remove_copy_if(std::next(it), end, dest, pred);
            other.size = std::distance(other.data(), dest);
            result = that()->size - other.size;
            that()->swap(other);
        }
        return result;
    }

    struct Span { T *begin; T *end; };

    void copyRanges(std::initializer_list<Span> ranges)
    {
        auto it = that()->begin();
        std::for_each(ranges.begin(), ranges.end(), [&it](const auto &span) {
            it = std::copy(span.begin, span.end, it);
        });
        that()->size = std::distance(that()->begin(), it);
    }

    void assign(T *b, T *e, parameter_type t) noexcept
    {
        Q_ASSERT(b <= e);
        Q_ASSERT(b >= that()->begin() && e <= that()->end());

        while (b != e)
            ::memcpy(static_cast<void *>(b++), static_cast<const void *>(&t), sizeof(T));
    }

    void reallocate(qsizetype alloc, QArrayData::AllocationOption option)
    {
        auto pair = Data::reallocateUnaligned(that()->d, that()->ptr, alloc, option);
        Q_CHECK_PTR(pair.ptr);
        Q_ASSERT(pair.header != nullptr);
        that()->d = pair.header;
        that()->ptr = pair.ptr;
    }
};

template <class T>
struct QGenericArrayOps
        : public QArrayDataPointer<T>
{
    static_assert (std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

protected:
    typedef QTypedArrayData<T> Data;
    using DataPointer = QArrayDataPointer<T>;

public:
    DataPointer *that()
    { return this; }
    const DataPointer *that() const
    { return this; }

    typedef typename QArrayDataPointer<T>::parameter_type parameter_type;

    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(that()->isMutable() || b == e);
        Q_ASSERT(!that()->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= that()->freeSpaceAtEnd());

        if (b == e) // short-cut and handling the case b and e == nullptr
            return;

        T *data = that()->begin();
        while (b < e) {
            new (data + that()->size) T(*b);
            ++b;
            ++that()->size;
        }
    }

    void copyAppend(qsizetype n, parameter_type t)
    {
        Q_ASSERT(!that()->isShared() || n == 0);
        Q_ASSERT(that()->freeSpaceAtEnd() >= n);
        if (!n)
            return;

        T *data = that()->begin();
        while (n--) {
            new (data + that()->size) T(t);
            ++that()->size;
        }
    }

    void moveAppend(T *b, T *e)
    {
        Q_ASSERT(that()->isMutable() || b == e);
        Q_ASSERT(!that()->isShared() || b == e);
        Q_ASSERT(b <= e);
        Q_ASSERT((e - b) <= that()->freeSpaceAtEnd());

        if (b == e)
            return;

        T *data = that()->begin();
        while (b < e) {
            new (data + that()->size) T(std::move(*b));
            ++b;
            ++that()->size;
        }
    }

    void truncate(size_t newSize)
    {
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(!that()->isShared());
        Q_ASSERT(newSize <= size_t(that()->size));

        std::destroy(that()->begin() + newSize, that()->end());
        that()->size = newSize;
    }

    void destroyAll() // Call from destructors, ONLY
    {
        Q_ASSERT(that()->d);
        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.

        Q_ASSERT(that()->d->ref_.loadRelaxed() == 0);

        std::destroy(that()->begin(), that()->end());
    }

    struct Inserter
    {
        QArrayDataPointer<T> *data;
        T *begin;
        qsizetype size;

        qsizetype sourceCopyConstruct = 0, nSource = 0, move = 0, sourceCopyAssign = 0;
        T *end = nullptr, *last = nullptr, *where = nullptr;

        Inserter(QArrayDataPointer<T> *d) : data(d)
        {
            begin = d->ptr;
            size = d->size;
        }
        ~Inserter() {
            data->ptr = begin;
            data->size = size;
        }
        Q_DISABLE_COPY(Inserter)

        void setup(qsizetype pos, qsizetype n)
        {
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
        }

        void insert(qsizetype pos, const T *source, qsizetype n)
        {
            qsizetype oldSize = size;
            Q_UNUSED(oldSize);

            setup(pos, n);

            // first create new elements at the end, by copying from elements
            // to be inserted (if they extend past the current end of the array)
            for (qsizetype i = 0; i != sourceCopyConstruct; ++i) {
                new (end + i) T(source[nSource - sourceCopyConstruct + i]);
                ++size;
            }
            Q_ASSERT(size <= oldSize + n);

            // now move construct new elements at the end from existing elements inside
            // the array.
            for (qsizetype i = sourceCopyConstruct; i != nSource; ++i) {
                new (end + i) T(std::move(*(end + i - nSource)));
                ++size;
            }
            // array has the new size now!
            Q_ASSERT(size == oldSize + n);

            // now move assign existing elements towards the end
            for (qsizetype i = 0; i != move; --i)
                last[i] = std::move(last[i - nSource]);

            // finally copy the remaining elements from source over
            for (qsizetype i = 0; i != sourceCopyAssign; ++i)
                where[i] = source[i];
        }

        void insert(qsizetype pos, const T &t, qsizetype n)
        {
            const qsizetype oldSize = size;
            Q_UNUSED(oldSize);

            setup(pos, n);

            // first create new elements at the end, by copying from elements
            // to be inserted (if they extend past the current end of the array)
            for (qsizetype i = 0; i != sourceCopyConstruct; ++i) {
                new (end + i) T(t);
                ++size;
            }
            Q_ASSERT(size <= oldSize + n);

            // now move construct new elements at the end from existing elements inside
            // the array.
            for (qsizetype i = sourceCopyConstruct; i != nSource; ++i) {
                new (end + i) T(std::move(*(end + i - nSource)));
                ++size;
            }
            // array has the new size now!
            Q_ASSERT(size == oldSize + n);

            // now move assign existing elements towards the end
            for (qsizetype i = 0; i != move; --i)
                last[i] = std::move(last[i - nSource]);

            // finally copy the remaining elements from source over
            for (qsizetype i = 0; i != sourceCopyAssign; ++i)
                where[i] = t;
        }

        void insertOne(qsizetype pos, T &&t)
        {
            setup(pos, 1);

            if (sourceCopyConstruct) {
                Q_ASSERT(sourceCopyConstruct == 1);
                new (end) T(std::move(t));
                ++size;
            } else {
                // create a new element at the end by move constructing one existing element
                // inside the array.
                new (end) T(std::move(*(end - 1)));
                ++size;

                // now move assign existing elements towards the end
                for (qsizetype i = 0; i != move; --i)
                    last[i] = std::move(last[i - 1]);

                // and move the new item into place
                *where = std::move(t);
            }
        }
    };

    void insert(qsizetype i, const T *data, qsizetype n)
    {
        const bool growsAtBegin = that()->size != 0 && i == 0;
        const auto pos = growsAtBegin ? Data::GrowsAtBeginning : Data::GrowsAtEnd;

        DataPointer oldData;
        that()->detachAndGrow(pos, n, &data, &oldData);
        Q_ASSERT((pos == Data::GrowsAtBeginning && that()->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && that()->freeSpaceAtEnd() >= n));

        if (growsAtBegin) {
            // copy construct items in reverse order at the begin
            Q_ASSERT(that()->freeSpaceAtBegin() >= n);
            while (n) {
                --n;
                new (that()->begin() - 1) T(data[n]);
                --that()->ptr;
                ++that()->size;
            }
        } else {
            Inserter{that()}.insert(i, data, n);
        }
    }

    void insert(qsizetype i, qsizetype n, parameter_type t)
    {
        T copy(t);

        const bool growsAtBegin = that()->size != 0 && i == 0;
        const auto pos = growsAtBegin ? Data::GrowsAtBeginning : Data::GrowsAtEnd;

        that()->detachAndGrow(pos, n, nullptr, nullptr);
        Q_ASSERT((pos == Data::GrowsAtBeginning && that()->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && that()->freeSpaceAtEnd() >= n));

        if (growsAtBegin) {
            // copy construct items in reverse order at the begin
            Q_ASSERT(that()->freeSpaceAtBegin() >= n);
            while (n--) {
                new (that()->begin() - 1) T(copy);
                --that()->ptr;
                ++that()->size;
            }
        } else {
            Inserter{that()}.insert(i, copy, n);
        }
    }

    template<typename... Args>
    void emplace(qsizetype i, Args &&... args)
    {
        bool detach = that()->needsDetach();
        if (!detach) {
            if (i == that()->size && that()->freeSpaceAtEnd()) {
                new (that()->end()) T(std::forward<Args>(args)...);
                ++that()->size;
                return;
            }
            if (i == 0 && that()->freeSpaceAtBegin()) {
                new (that()->begin() - 1) T(std::forward<Args>(args)...);
                --that()->ptr;
                ++that()->size;
                return;
            }
        }
        T tmp(std::forward<Args>(args)...);
        const bool growsAtBegin = that()->size != 0 && i == 0;
        const auto pos = growsAtBegin ? Data::GrowsAtBeginning : Data::GrowsAtEnd;

        that()->detachAndGrow(pos, 1, nullptr, nullptr);

        if (growsAtBegin) {
            Q_ASSERT(that()->freeSpaceAtBegin());
            new (that()->begin() - 1) T(std::move(tmp));
            --that()->ptr;
            ++that()->size;
        } else {
            Inserter{that()}.insertOne(i, std::move(tmp));
        }
    }

    void erase(T *b, qsizetype n)
    {
        T *e = b + n;
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= that()->begin() && b < that()->end());
        Q_ASSERT(e > that()->begin() && e <= that()->end());

        // Comply with std::vector::erase(): erased elements and all after them
        // are invalidated. However, erasing from the beginning effectively
        // means that all iterators are invalidated. We can use this freedom to
        // erase by moving towards the end.
        if (b == that()->begin() && e != that()->end()) {
            that()->ptr = e;
        } else {
            const T *const end = that()->end();

            // move (by assignment) the elements from e to end
            // onto b to the new end
            while (e != end) {
                *b = std::move(*e);
                ++b;
                ++e;
            }
        }
        that()->size -= n;
        std::destroy(b, e);
    }

    void eraseFirst() noexcept
    {
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(that()->size);
        that()->begin()->~T();
        ++that()->ptr;
        --that()->size;
    }

    void eraseLast() noexcept
    {
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(that()->size);
        (that()->end() - 1)->~T();
        --that()->size;
    }


    void assign(T *b, T *e, parameter_type t)
    {
        Q_ASSERT(b <= e);
        Q_ASSERT(b >= that()->begin() && e <= that()->end());

        while (b != e)
            *b++ = t;
    }
};

template <class T>
struct QMovableArrayOps
    : QGenericArrayOps<T>
{
    using Base = QGenericArrayOps<T>;
    static_assert (std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

protected:
    typedef QTypedArrayData<T> Data;
    using DataPointer = QArrayDataPointer<T>;

public:
    DataPointer *that()
    { return Base::that(); }
    const DataPointer *that() const
    { return Base::that(); }

    // using QGenericArrayOps<T>::copyAppend;
    // using QGenericArrayOps<T>::moveAppend;
    // using QGenericArrayOps<T>::truncate;
    // using QGenericArrayOps<T>::destroyAll;
    typedef typename QGenericArrayOps<T>::parameter_type parameter_type;

    struct Inserter
    {
        QArrayDataPointer<T> * const data;
        T *displaceFrom;
        T * const displaceTo;
        const qsizetype nInserts = 0;
        const size_t bytes;

        void verifyPost()
        { Q_ASSERT(displaceFrom == displaceTo); }

        explicit Inserter(QArrayDataPointer<T> *d, qsizetype pos, qsizetype n)
            : data{d},
              displaceFrom{d->ptr + pos},
              displaceTo{displaceFrom + n},
              nInserts{n},
              bytes{(data->size - pos) * sizeof(T)}
        {
            ::memmove(static_cast<void *>(displaceTo), static_cast<void *>(displaceFrom), bytes);
        }
        ~Inserter() {
            auto inserts = nInserts;
            if constexpr (!std::is_nothrow_copy_constructible_v<T>) {
                if (displaceFrom != displaceTo) {
                    ::memmove(static_cast<void *>(displaceFrom), static_cast<void *>(displaceTo), bytes);
                    inserts -= qAbs(displaceFrom - displaceTo);
                }
            }
            data->size += inserts;
        }
        Q_DISABLE_COPY(Inserter)

        void insertRange(const T *source, qsizetype n)
        {
            while (n--) {
                new (displaceFrom) T(*source);
                ++source;
                ++displaceFrom;
            }
            verifyPost();
        }

        void insertFill(const T &t, qsizetype n)
        {
            while (n--) {
                new (displaceFrom) T(t);
                ++displaceFrom;
            }
            verifyPost();
        }

        void insertOne(T &&t)
        {
            new (displaceFrom) T(std::move(t));
            ++displaceFrom;
            verifyPost();
        }

    };


    void insert(qsizetype i, const T *data, qsizetype n)
    {
        const bool growsAtBegin = that()->size != 0 && i == 0;
        const auto pos = growsAtBegin ? Data::GrowsAtBeginning : Data::GrowsAtEnd;

        DataPointer oldData;
        that()->detachAndGrow(pos, n, &data, &oldData);
        Q_ASSERT((pos == Data::GrowsAtBeginning && that()->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && that()->freeSpaceAtEnd() >= n));

        if (growsAtBegin) {
            // copy construct items in reverse order at the begin
            Q_ASSERT(that()->freeSpaceAtBegin() >= n);
            while (n) {
                --n;
                new (that()->begin() - 1) T(data[n]);
                --that()->ptr;
                ++that()->size;
            }
        } else {
            Inserter{that(), i, n}.insertRange(data, n);
        }
    }

    void insert(qsizetype i, qsizetype n, parameter_type t)
    {
        T copy(t);

        const bool growsAtBegin = that()->size != 0 && i == 0;
        const auto pos = growsAtBegin ? Data::GrowsAtBeginning : Data::GrowsAtEnd;

        that()->detachAndGrow(pos, n, nullptr, nullptr);
        Q_ASSERT((pos == Data::GrowsAtBeginning && that()->freeSpaceAtBegin() >= n) ||
                 (pos == Data::GrowsAtEnd && that()->freeSpaceAtEnd() >= n));

        if (growsAtBegin) {
            // copy construct items in reverse order at the begin
            Q_ASSERT(that()->freeSpaceAtBegin() >= n);
            while (n--) {
                new (that()->begin() - 1) T(copy);
                --that()->ptr;
                ++that()->size;
            }
        } else {
            Inserter{that(), i, n}.insertFill(copy, n);
        }
    }

    template<typename... Args>
    void emplace(qsizetype i, Args &&... args)
    {
        bool detach = that()->needsDetach();
        if (!detach) {
            if (i == that()->size && that()->freeSpaceAtEnd()) {
                new (that()->end()) T(std::forward<Args>(args)...);
                ++that()->size;
                return;
            }
            if (i == 0 && that()->freeSpaceAtBegin()) {
                new (that()->begin() - 1) T(std::forward<Args>(args)...);
                --that()->ptr;
                ++that()->size;
                return;
            }
        }
        T tmp(std::forward<Args>(args)...);
        const bool growsAtBegin = that()->size != 0 && i == 0;
        const auto pos = growsAtBegin ? Data::GrowsAtBeginning : Data::GrowsAtEnd;

        that()->detachAndGrow(pos, 1, nullptr, nullptr);
        if (growsAtBegin) {
            Q_ASSERT(that()->freeSpaceAtBegin());
            new (that()->begin() - 1) T(std::move(tmp));
            --that()->ptr;
            ++that()->size;
        } else {
            Inserter{that(), i, 1}.insertOne(std::move(tmp));
        }
    }

    void erase(T *b, qsizetype n)
    {
        T *e = b + n;

        Q_ASSERT(that()->isMutable());
        Q_ASSERT(b < e);
        Q_ASSERT(b >= that()->begin() && b < that()->end());
        Q_ASSERT(e > that()->begin() && e <= that()->end());

        // Comply with std::vector::erase(): erased elements and all after them
        // are invalidated. However, erasing from the beginning effectively
        // means that all iterators are invalidated. We can use this freedom to
        // erase by moving towards the end.

        std::destroy(b, e);
        if (b == that()->begin() && e != that()->end()) {
            that()->ptr = e;
        } else if (e != that()->end()) {
            memmove(static_cast<void *>(b), static_cast<const void *>(e), (static_cast<const T *>(that()->end()) - e)*sizeof(T));
        }
        that()->size -= n;
    }

    void reallocate(qsizetype alloc, QArrayData::AllocationOption option)
    {
        auto pair = Data::reallocateUnaligned(that()->d, that()->ptr, alloc, option);
        Q_CHECK_PTR(pair.ptr);
        Q_ASSERT(pair.header != nullptr);
        that()->d = pair.header;
        that()->ptr = pair.ptr;
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

protected:
    using Self = QCommonArrayOps<T>;

public:
    DataPointer *that()
    { return Base::that(); }
    const DataPointer *that() const
    { return Base::that(); }

    // using Base::truncate;
    // using Base::destroyAll;

    template<typename It>
    void appendIteratorRange(It b, It e, QtPrivate::IfIsForwardIterator<It> = true)
    {
        Q_ASSERT(that()->isMutable() || b == e);
        Q_ASSERT(!that()->isShared() || b == e);
        const qsizetype distance = std::distance(b, e);
        Q_ASSERT(distance >= 0 && distance <= that()->allocatedCapacity() - that()->size);
        Q_UNUSED(distance);

#if __cplusplus >= 202002L && defined(__cpp_concepts) && defined(__cpp_lib_concepts)
        constexpr bool canUseCopyAppend =
                std::contiguous_iterator<It> &&
                std::is_same_v<
                    std::remove_cv_t<typename std::iterator_traits<It>::value_type>,
                    T
                >;
        if constexpr (canUseCopyAppend) {
            Base::copyAppend(std::to_address(b), std::to_address(e));
        } else
#endif
        {
            T *iter = that()->end();
            for (; b != e; ++iter, ++b) {
                new (iter) T(*b);
                ++that()->size;
            }
        }
    }

    // slightly higher level API than copyAppend() that also preallocates space
    void growAppend(const T *b, const T *e)
    {
        if (b == e)
            return;
        Q_ASSERT(b < e);
        const qsizetype n = e - b;
        DataPointer old;

        // points into range:
        if (QtPrivate::q_points_into_range(b, *that()))
            that()->detachAndGrow(QArrayData::GrowsAtEnd, n, &b, &old);
        else
            that()->detachAndGrow(QArrayData::GrowsAtEnd, n, nullptr, nullptr);
        Q_ASSERT(that()->freeSpaceAtEnd() >= n);
        // b might be updated so use [b, n)
        Base::copyAppend(b, b + n);
    }

    void appendUninitialized(qsizetype newSize)
    {
        Q_ASSERT(that()->isMutable());
        Q_ASSERT(!that()->isShared());
        Q_ASSERT(newSize > that()->size);
        Q_ASSERT(newSize - that()->size <= that()->freeSpaceAtEnd());


        T *const b = that()->begin() + that()->size;
        T *const e = that()->begin() + newSize;
        if constexpr (std::is_constructible_v<T, Qt::Initialization>)
            std::uninitialized_fill(b, e, Qt::Uninitialized);
        else
            std::uninitialized_default_construct(b, e);
        that()->size = newSize;
    }

    using Base::assign;

    template <typename InputIterator, typename Projection = q20::identity>
    void assign(InputIterator first, InputIterator last, Projection proj = {})
    {
        // This function only provides the basic exception guarantee.
        using Category = typename std::iterator_traits<InputIterator>::iterator_category;
        constexpr bool IsFwdIt = std::is_convertible_v<Category, std::forward_iterator_tag>;

        const qsizetype n = IsFwdIt ? std::distance(first, last) : 0;
        bool undoPrependOptimization = true;
        bool needCapacity = n > that()->constAllocatedCapacity();
        if (needCapacity || that()->needsDetach()) {
            qsizetype newCapacity = that()->detachCapacity(n);
            bool wasLastRef = !that()->deref();
            if (wasLastRef && needCapacity) {
                // free memory we can't reuse
                Base::destroyAll();
                Data::deallocate(that()->d);
            }
            if (!needCapacity && wasLastRef) {
                // we were the last reference and can reuse the storage
                that()->d->ref_.storeRelaxed(1);
            } else {
                // we must allocate new memory
                auto [hdr, p] = Data::allocate(newCapacity);
                that()->d = hdr;
                that()->ptr = p;
                that()->size = 0;
                undoPrependOptimization = false;
            }
        }

        if constexpr (!std::is_nothrow_constructible_v<T, decltype(std::invoke(proj, *first))>
                      || !std::is_nothrow_invocable_v<Projection, decltype(*first)>)
        {
            // If construction can throw, and we have freeSpaceAtBegin(),
            // it's easiest to just clear the container and start fresh.
            // The alternative would be to keep track of two active, disjoint ranges.
            if (undoPrependOptimization) {
                Base::truncate(0);
                that()->setBegin(Data::dataStart(that()->d, alignof(typename Data::AlignmentDummy)));
                undoPrependOptimization = false;
            }
        }

        const auto dend = that()->end();
        T *dst = that()->begin();
        T *capacityBegin = dst;
        if (undoPrependOptimization) {
            capacityBegin = Data::dataStart(that()->d, alignof(typename Data::AlignmentDummy));
            that()->setBegin(capacityBegin); // undo prepend optimization
        }

        assign_impl(first, last, capacityBegin, dst, dend, proj, Category{});
    }

    template <typename InputIterator, typename Projection>
    void assign_impl(InputIterator first, InputIterator last, T *capacityBegin, T *dst, T *dend,
                     Projection proj, std::input_iterator_tag)
    {
        if (qsizetype offset = dst - capacityBegin) {
            T *prependBufferEnd = dst;
            dst = capacityBegin;

            // By construction, the following loop is nothrow!
            // (otherwise, we can't reach here)
            // Assumes InputIterator operations don't throw.
            // (but we can't statically assert that, as these operations
            //  have preconditons, so typically aren't noexcept)
            while (true) {
                if (dst == prependBufferEnd) {  // ran out of prepend buffer space
                    that()->size += offset;
                    // we now have a contiguous buffer, continue with the main loop:
                    break;
                }
                if (first == last) {            // ran out of elements to assign
                    std::destroy(prependBufferEnd, dend);
                    that()->size = dst - that()->begin();
                    return;
                }
                // construct element in prepend buffer
                q20::construct_at(dst, std::invoke(proj, *first));
                ++dst;
                ++first;
            }
        }
        while (true) {
            if (first == last) {    // ran out of elements to assign
                std::destroy(dst, dend);
                break;
            }
            if (dst == dend) {      // ran out of existing elements to overwrite
                do {
                    Base::emplace(that()->size, std::invoke(proj, *first));
                } while (++first != last);
                return;         // size() is already correct (and dst invalidated)!
            }
            *dst = std::invoke(proj, *first);    // overwrite existing element
            ++dst;
            ++first;
        }
        that()->size = dst - that()->begin();
    }

    template <typename InputIterator, typename Projection>
    void assign_impl(InputIterator first, InputIterator last, T *capacityBegin, T *dst, T *dend,
                     Projection proj, std::forward_iterator_tag)
    {
        constexpr bool IsIdentity = std::is_same_v<Projection, q20::identity>;
        const qsizetype n = std::distance(first, last);
        if constexpr (IsIdentity && !QTypeInfo<T>::isComplex) {
            // For non-complex types, we prefer a single std::copy() -> memcpy()
            // call. We can do that because either the default constructor is
            // trivial (so the lifetime has started) or the copy constructor is
            // (and won't care what the stored value is).
            std::copy(first, last, capacityBegin);
        } else {
            // There are two possibilities:
            // 1) fewer elements than the current allocated space
            //    | prepend buffer | array |  destroy  |
            // 2) more elements than the current allocated space
            //    | prepend buffer | array | construct |
            //
            // Both the prepend buffer and the current array may be empty.

            // construct elements in the prepend buffer
            while (first != last && capacityBegin != dst) {
                q20::construct_at(capacityBegin, std::invoke(proj, *first));
                ++first;
                ++capacityBegin;
            }

            // overwrite elements in the existing array
            while (first != last && dst != dend) {
                *dst = std::invoke(proj, *first);    // overwrite existing element
                ++first;
                ++dst;
            }

            // construct new elements in the append buffer
            while (first != last) {
                q20::construct_at(dst, std::invoke(proj, *first));
                ++first;
                ++dst;
            }
            // or destroy elements from the existing array
            if (dst < dend)
                std::destroy(dst, dend);
        }
        that()->size = n;
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
