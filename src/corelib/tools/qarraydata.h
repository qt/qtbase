/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2019 Intel Corporation.
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

#ifndef QARRAYDATA_H
#define QARRAYDATA_H

#include <QtCore/qpair.h>
#include <QtCore/qatomic.h>
#include <string.h>

QT_BEGIN_NAMESPACE

template <class T> struct QTypedArrayData;

struct QArrayData
{
    enum AllocationOption {
        Grow,
        KeepSize
    };

    enum GrowthPosition {
        GrowsAtEnd,
        GrowsAtBeginning
    };

   enum ArrayOption {
        ArrayOptionDefault = 0,
        CapacityReserved     = 0x1  //!< the capacity was reserved by the user, try to keep it
    };
    Q_DECLARE_FLAGS(ArrayOptions, ArrayOption)

    QBasicAtomicInt ref_;
    ArrayOptions flags;
    qsizetype alloc;

    qsizetype allocatedCapacity() noexcept
    {
        return alloc;
    }

    qsizetype constAllocatedCapacity() const noexcept
    {
        return alloc;
    }

    /// Returns true if sharing took place
    bool ref() noexcept
    {
        ref_.ref();
        return true;
    }

    /// Returns false if deallocation is necessary
    bool deref() noexcept
    {
        return ref_.deref();
    }

    bool isShared() const noexcept
    {
        return ref_.loadRelaxed() != 1;
    }

    // Returns true if a detach is necessary before modifying the data
    // This method is intentionally not const: if you want to know whether
    // detaching is necessary, you should be in a non-const function already
    bool needsDetach() const noexcept
    {
        return ref_.loadRelaxed() > 1;
    }

    qsizetype detachCapacity(qsizetype newSize) const noexcept
    {
        if (flags & CapacityReserved && newSize < constAllocatedCapacity())
            return constAllocatedCapacity();
        return newSize;
    }

    [[nodiscard]]
#if defined(Q_CC_GNU)
    __attribute__((__malloc__))
#endif
    static Q_CORE_EXPORT void *allocate(QArrayData **pdata, qsizetype objectSize, qsizetype alignment,
            qsizetype capacity, AllocationOption option = QArrayData::KeepSize) noexcept;
    [[nodiscard]] static Q_CORE_EXPORT QPair<QArrayData *, void *> reallocateUnaligned(QArrayData *data, void *dataPointer,
            qsizetype objectSize, qsizetype newCapacity, AllocationOption option) noexcept;
    static Q_CORE_EXPORT void deallocate(QArrayData *data, qsizetype objectSize,
            qsizetype alignment) noexcept;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QArrayData::ArrayOptions)

template <class T>
struct QTypedArrayData
    : QArrayData
{
    struct AlignmentDummy { QArrayData header; T data; };

    [[nodiscard]] static QPair<QTypedArrayData *, T *> allocate(qsizetype capacity, AllocationOption option = QArrayData::KeepSize)
    {
        static_assert(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayData *d;
        void *result = QArrayData::allocate(&d, sizeof(T), alignof(AlignmentDummy), capacity, option);
#if __has_builtin(__builtin_assume_aligned)
        result = __builtin_assume_aligned(result, Q_ALIGNOF(AlignmentDummy));
#endif
        return qMakePair(static_cast<QTypedArrayData *>(d), static_cast<T *>(result));
    }

    static QPair<QTypedArrayData *, T *>
    reallocateUnaligned(QTypedArrayData *data, T *dataPointer, qsizetype capacity, AllocationOption option)
    {
        static_assert(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QPair<QArrayData *, void *> pair =
                QArrayData::reallocateUnaligned(data, dataPointer, sizeof(T), capacity, option);
        return qMakePair(static_cast<QTypedArrayData *>(pair.first), static_cast<T *>(pair.second));
    }

    static void deallocate(QArrayData *data) noexcept
    {
        static_assert(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayData::deallocate(data, sizeof(T), alignof(AlignmentDummy));
    }

    static T *dataStart(QArrayData *data, qsizetype alignment) noexcept
    {
        // Alignment is a power of two
        Q_ASSERT(alignment >= qsizetype(alignof(QArrayData)) && !(alignment & (alignment - 1)));
        void *start =  reinterpret_cast<void *>(
            (quintptr(data) + sizeof(QArrayData) + alignment - 1) & ~(alignment - 1));
        return static_cast<T *>(start);
    }
};

namespace QtPrivate {
struct Q_CORE_EXPORT QContainerImplHelper
{
    enum CutResult { Null, Empty, Full, Subset };
    static constexpr CutResult mid(qsizetype originalLength, qsizetype *_position, qsizetype *_length)
    {
        qsizetype &position = *_position;
        qsizetype &length = *_length;
        if (position > originalLength) {
            position = 0;
            length = 0;
            return Null;
        }

        if (position < 0) {
            if (length < 0 || length + position >= originalLength) {
                position = 0;
                length = originalLength;
                return Full;
            }
            if (length + position <= 0) {
                position = length = 0;
                return Null;
            }
            length += position;
            position = 0;
        } else if (size_t(length) > size_t(originalLength - position)) {
            length = originalLength - position;
        }

        if (position == 0 && length == originalLength)
            return Full;

        return length > 0 ? Subset : Empty;
    }
};
}

QT_END_NAMESPACE

#endif // include guard
