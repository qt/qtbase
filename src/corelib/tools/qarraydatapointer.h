/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QARRAYDATAPOINTER_H
#define QARRAYDATAPOINTER_H

#include <QtCore/qarraydataops.h>
#include <QtCore/qcontainertools_impl.h>

QT_BEGIN_NAMESPACE

template <class T>
struct QArrayDataPointer
{
private:
    typedef QTypedArrayData<T> Data;
    typedef QArrayDataOps<T> DataOps;

public:
    typedef typename Data::iterator iterator;
    typedef typename Data::const_iterator const_iterator;
    enum { pass_parameter_by_value = std::is_fundamental<T>::value || std::is_pointer<T>::value };

    typedef typename std::conditional<pass_parameter_by_value, T, const T &>::type parameter_type;

    constexpr QArrayDataPointer() noexcept
        : d(nullptr), ptr(nullptr), size(0)
    {
    }

    QArrayDataPointer(const QArrayDataPointer &other) noexcept
        : d(other.d), ptr(other.ptr), size(other.size)
    {
        ref();
    }

    constexpr QArrayDataPointer(Data *header, T *adata, qsizetype n = 0) noexcept
        : d(header), ptr(adata), size(n)
    {
    }

    explicit QArrayDataPointer(QPair<QTypedArrayData<T> *, T *> adata, qsizetype n = 0) noexcept
        : d(adata.first), ptr(adata.second), size(n)
    {
    }

    static QArrayDataPointer fromRawData(const T *rawData, qsizetype length) noexcept
    {
        Q_ASSERT(rawData || !length);
        return { nullptr, const_cast<T *>(rawData), length };
    }

    QArrayDataPointer &operator=(const QArrayDataPointer &other) noexcept
    {
        QArrayDataPointer tmp(other);
        this->swap(tmp);
        return *this;
    }

    QArrayDataPointer(QArrayDataPointer &&other) noexcept
        : d(other.d), ptr(other.ptr), size(other.size)
    {
        other.d = nullptr;
        other.ptr = nullptr;
        other.size = 0;
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QArrayDataPointer)

    DataOps &operator*() noexcept
    {
        return *static_cast<DataOps *>(this);
    }

    DataOps *operator->() noexcept
    {
        return static_cast<DataOps *>(this);
    }

    const DataOps &operator*() const noexcept
    {
        return *static_cast<const DataOps *>(this);
    }

    const DataOps *operator->() const noexcept
    {
        return static_cast<const DataOps *>(this);
    }

    ~QArrayDataPointer()
    {
        if (!deref()) {
            (*this)->destroyAll();
            Data::deallocate(d);
        }
    }

    bool isNull() const noexcept
    {
        return !ptr;
    }

    T *data() noexcept { return ptr; }
    const T *data() const noexcept { return ptr; }

    iterator begin(iterator = iterator()) noexcept { return data(); }
    iterator end(iterator = iterator()) noexcept { return data() + size; }
    const_iterator begin(const_iterator = const_iterator()) const noexcept { return data(); }
    const_iterator end(const_iterator = const_iterator()) const noexcept { return data() + size; }
    const_iterator constBegin(const_iterator = const_iterator()) const noexcept { return data(); }
    const_iterator constEnd(const_iterator = const_iterator()) const noexcept { return data() + size; }

    void swap(QArrayDataPointer &other) noexcept
    {
        qSwap(d, other.d);
        qSwap(ptr, other.ptr);
        qSwap(size, other.size);
    }

    void clear() noexcept(std::is_nothrow_destructible<T>::value)
    {
        QArrayDataPointer tmp;
        swap(tmp);
    }

    bool detach()
    {
        if (needsDetach()) {
            QPair<Data *, T *> copy = clone(detachFlags());
            QArrayDataPointer old(d, ptr, size);
            d = copy.first;
            ptr = copy.second;
            return true;
        }

        return false;
    }

    // forwards from QArrayData
    qsizetype allocatedCapacity() noexcept { return d ? d->allocatedCapacity() : 0; }
    qsizetype constAllocatedCapacity() const noexcept { return d ? d->constAllocatedCapacity() : 0; }
    void ref() noexcept { if (d) d->ref(); }
    bool deref() noexcept { return !d || d->deref(); }
    bool isMutable() const noexcept { return d; }
    bool isShared() const noexcept { return !d || d->isShared(); }
    bool isSharedWith(const QArrayDataPointer &other) const noexcept { return d && d == other.d; }
    bool needsDetach() const noexcept { return !d || d->needsDetach(); }
    qsizetype detachCapacity(qsizetype newSize) const noexcept { return d ? d->detachCapacity(newSize) : newSize; }
    const typename Data::ArrayOptions flags() const noexcept { return d ? typename Data::ArrayOption(d->flags) : Data::DefaultAllocationFlags; }
    void setFlag(typename Data::ArrayOptions f) noexcept { Q_ASSERT(d); d->flags |= f; }
    void clearFlag(typename Data::ArrayOptions f) noexcept { Q_ASSERT(d); d->flags &= ~f; }
    typename Data::ArrayOptions detachFlags() const noexcept { return d ? d->detachFlags() : Data::DefaultAllocationFlags; }

    Data *d_ptr() noexcept { return d; }
    void setBegin(T *begin) noexcept { ptr = begin; }

    qsizetype freeSpaceAtBegin() const noexcept
    {
        if (d == nullptr)
            return 0;
        return this->ptr - Data::dataStart(d, alignof(typename Data::AlignmentDummy));
    }

    qsizetype freeSpaceAtEnd() const noexcept
    {
        if (d == nullptr)
            return 0;
        return d->constAllocatedCapacity() - freeSpaceAtBegin() - this->size;
    }

    static QArrayDataPointer allocateGrow(const QArrayDataPointer &from,
                                          qsizetype newSize, QArrayData::ArrayOptions options)
    {
        return allocateGrow(from, from.detachCapacity(newSize), newSize, options);
    }

    static QArrayDataPointer allocateGrow(const QArrayDataPointer &from, qsizetype capacity,
                                          qsizetype newSize, QArrayData::ArrayOptions options)
    {
        auto [header, dataPtr] = Data::allocate(capacity, options);
        const bool valid = header != nullptr && dataPtr != nullptr;
        const bool grows = (options & (Data::GrowsForward | Data::GrowsBackwards));
        if (!valid || !grows)
            return QArrayDataPointer(header, dataPtr);

        // when growing, special rules apply to memory layout

        if (from.needsDetach()) {
            // When detaching: the free space reservation is biased towards
            // append as in Qt5 QList. If we're growing backwards, put the data
            // in the middle instead of at the end - assuming that prepend is
            // uncommon and even initial prepend will eventually be followed by
            // at least some appends.
            if (options & Data::GrowsBackwards)
                dataPtr += (header->alloc - newSize) / 2;
        } else {
            // When not detaching: fake ::realloc() policy - preserve existing
            // free space at beginning.
            dataPtr += from.freeSpaceAtBegin();
        }
        return QArrayDataPointer(header, dataPtr);
    }

private:
    [[nodiscard]] QPair<Data *, T *> clone(QArrayData::ArrayOptions options) const
    {
        QPair<Data *, T *> pair = Data::allocate(detachCapacity(size), options);
        QArrayDataPointer copy(pair.first, pair.second, 0);
        if (size)
            copy->copyAppend(begin(), end());

        pair.first = copy.d;
        copy.d = nullptr;
        copy.ptr = nullptr;
        return pair;
    }

protected:
    Data *d;
    T *ptr;

public:
    qsizetype size;
};

template <class T>
inline bool operator==(const QArrayDataPointer<T> &lhs, const QArrayDataPointer<T> &rhs) noexcept
{
    return lhs.data() == rhs.data() && lhs.size == rhs.size;
}

template <class T>
inline bool operator!=(const QArrayDataPointer<T> &lhs, const QArrayDataPointer<T> &rhs) noexcept
{
    return lhs.data() != rhs.data() || lhs.size != rhs.size;
}

template <class T>
inline void qSwap(QArrayDataPointer<T> &p1, QArrayDataPointer<T> &p2) noexcept
{
    p1.swap(p2);
}

////////////////////////////////////////////////////////////////////////////////
//  Q_ARRAY_LITERAL

// The idea here is to place a (read-only) copy of header and array data in an
// mmappable portion of the executable (typically, .rodata section).

// Hide array inside a lambda
#define Q_ARRAY_LITERAL(Type, ...) \
    ([]() -> QArrayDataPointer<Type> { \
        static Type const data[] = { __VA_ARGS__ }; \
        return QArrayDataPointer<Type>::fromRawData(const_cast<Type *>(data), std::size(data)); \
    }())
/**/

QT_END_NAMESPACE

#endif // include guard
