/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

struct Q_CORE_EXPORT QArrayData
{
    enum ArrayOption {
        RawDataType          = 0x0001,  //!< this class is really a QArrayData
        AllocatedDataType    = 0x0002,  //!< this class is really a QArrayAllocatedData
        DataTypeBits         = 0x000f,

        CapacityReserved     = 0x0010,  //!< the capacity was reserved by the user, try to keep it
        GrowsForward         = 0x0020,  //!< allocate with eyes towards growing through append()
        GrowsBackwards       = 0x0040,  //!< allocate with eyes towards growing through prepend()
        MutableData          = 0x0080,  //!< the data can be changed; doesn't say anything about the header
        ImmutableHeader      = 0x0100,  //!< the header is static, it can't be changed

        /// this option is used by the Q_ARRAY_LITERAL and similar macros
        StaticDataFlags = RawDataType | ImmutableHeader,
        /// this option is used by the allocate() function
        DefaultAllocationFlags = MutableData,
        /// this option is used by the prepareRawData() function
        DefaultRawFlags = 0
    };
    Q_DECLARE_FLAGS(ArrayOptions, ArrayOption)

    QBasicAtomicInt ref_;
    uint flags;
    uint alloc;

    inline size_t allocatedCapacity()
    {
        return alloc;
    }

    inline size_t constAllocatedCapacity() const
    {
        return alloc;
    }

    /// Returns true if sharing took place
    bool ref()
    {
        if (!isStatic())
            ref_.ref();
        return true;
    }

    /// Returns false if deallocation is necessary
    bool deref()
    {
        if (isStatic())
            return true;
        return ref_.deref();
    }

    // This refers to array data mutability, not "header data" represented by
    // data members in QArrayData. Shared data (array and header) must still
    // follow COW principles.
    bool isMutable() const
    {
        return flags & MutableData;
    }

    bool isStatic() const
    {
        return flags & ImmutableHeader;
    }

    bool isShared() const
    {
        return ref_.loadRelaxed() != 1;
    }

    // Returns true if a detach is necessary before modifying the data
    // This method is intentionally not const: if you want to know whether
    // detaching is necessary, you should be in a non-const function already
    bool needsDetach()
    {
        // requires two conditionals
        return !isMutable() || isShared();
    }

    size_t detachCapacity(size_t newSize) const
    {
        if (flags & CapacityReserved && newSize < constAllocatedCapacity())
            return constAllocatedCapacity();
        return newSize;
    }

    ArrayOptions detachFlags() const
    {
        ArrayOptions result = DefaultAllocationFlags;
        if (flags & CapacityReserved)
            result |= CapacityReserved;
        return result;
    }

    ArrayOptions cloneFlags() const
    {
        ArrayOptions result = DefaultAllocationFlags;
        if (flags & CapacityReserved)
            result |= CapacityReserved;
        return result;
    }

    Q_REQUIRED_RESULT
#if defined(Q_CC_GNU)
    __attribute__((__malloc__))
#endif
    static void *allocate(QArrayData **pdata, size_t objectSize, size_t alignment,
            size_t capacity, ArrayOptions options = DefaultAllocationFlags) noexcept;
    Q_REQUIRED_RESULT static QArrayData *reallocateUnaligned(QArrayData *data, size_t objectSize,
            size_t newCapacity, ArrayOptions newOptions = DefaultAllocationFlags) noexcept;
    Q_REQUIRED_RESULT static QPair<QArrayData *, void *> reallocateUnaligned(QArrayData *data, void *dataPointer,
            size_t objectSize, size_t newCapacity, ArrayOptions newOptions = DefaultAllocationFlags) Q_DECL_NOTHROW;
    Q_REQUIRED_RESULT static QArrayData *prepareRawData(ArrayOptions options = ArrayOptions(RawDataType))
        Q_DECL_NOTHROW;
    static void deallocate(QArrayData *data, size_t objectSize,
            size_t alignment) noexcept;

    static const QArrayData shared_null[2];
    static QArrayData *sharedNull() noexcept { return const_cast<QArrayData*>(shared_null); }
    static void *sharedNullData()
    {
        QArrayData *const null = const_cast<QArrayData *>(&shared_null[1]);
        return null;
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QArrayData::ArrayOptions)

template <class T, size_t N>
struct QStaticArrayData
{
    // static arrays are of type RawDataType
    QArrayData header;
    T data[N];
};

// Support for returning QArrayDataPointer<T> from functions
template <class T>
struct QArrayDataPointerRef
{
    QTypedArrayData<T> *ptr;
    T *data;
    uint size;
};

template <class T>
struct QTypedArrayData
    : QArrayData
{
    typedef T* iterator;
    typedef const T* const_iterator;

    class AlignmentDummy { QArrayData header; T data; };

    Q_REQUIRED_RESULT static QPair<QTypedArrayData *, T *> allocate(size_t capacity,
            ArrayOptions options = DefaultAllocationFlags)
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayData *d;
        void *result = QArrayData::allocate(&d, sizeof(T), alignof(AlignmentDummy), capacity, options);
#if (defined(Q_CC_GNU) && Q_CC_GNU >= 407) || QT_HAS_BUILTIN(__builtin_assume_aligned)
        result = __builtin_assume_aligned(result, Q_ALIGNOF(AlignmentDummy));
#endif
        return qMakePair(static_cast<QTypedArrayData *>(d), static_cast<T *>(result));
    }

    static QPair<QTypedArrayData *, T *>
    reallocateUnaligned(QTypedArrayData *data, T *dataPointer, size_t capacity,
            ArrayOptions options = DefaultAllocationFlags)
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QPair<QArrayData *, void *> pair =
                QArrayData::reallocateUnaligned(data, dataPointer, sizeof(T), capacity, options);
        return qMakePair(static_cast<QTypedArrayData *>(pair.first), static_cast<T *>(pair.second));
    }

    static void deallocate(QArrayData *data)
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayData::deallocate(data, sizeof(T), alignof(AlignmentDummy));
    }

    static QArrayDataPointerRef<T> fromRawData(const T *data, size_t n,
            ArrayOptions options = DefaultRawFlags)
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayDataPointerRef<T> result = {
            static_cast<QTypedArrayData *>(prepareRawData(options)), const_cast<T *>(data), uint(n)
        };
        if (result.ptr) {
            Q_ASSERT(!result.ptr->isShared()); // No shared empty, please!
        }
        return result;
    }

    static QTypedArrayData *sharedNull() noexcept
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        return static_cast<QTypedArrayData *>(QArrayData::sharedNull());
    }

    static QTypedArrayData *sharedEmpty()
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        return allocate(/* capacity */ 0);
    }

    static T *sharedNullData()
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        return static_cast<T *>(QArrayData::sharedNullData());
    }
};

////////////////////////////////////////////////////////////////////////////////
//  Q_ARRAY_LITERAL

// The idea here is to place a (read-only) copy of header and array data in an
// mmappable portion of the executable (typically, .rodata section). This is
// accomplished by hiding a static const instance of QStaticArrayData, which is
// POD.

// Hide array inside a lambda
#define Q_ARRAY_LITERAL(Type, ...)                                              \
    ([]() -> QArrayDataPointerRef<Type> {                                       \
            /* MSVC 2010 Doesn't support static variables in a lambda, but */   \
            /* happily accepts them in a static function of a lambda-local */   \
            /* struct :-) */                                                    \
            struct StaticWrapper {                                              \
                static QArrayDataPointerRef<Type> get()                         \
                {                                                               \
                    Q_ARRAY_LITERAL_IMPL(Type, __VA_ARGS__)                     \
                    return ref;                                                 \
                }                                                               \
            };                                                                  \
            return StaticWrapper::get();                                        \
        }())                                                                    \
    /**/

#define Q_ARRAY_LITERAL_IMPL(Type, ...)                                         \
    /* Portable compile-time array size computation */                          \
    static constexpr Type data[] = { __VA_ARGS__ };                             \
    enum { Size = sizeof(data) / sizeof(data[0]) };                             \
                                                                                \
    static constexpr QArrayData literal = { Q_BASIC_ATOMIC_INITIALIZER(-1), QArrayData::StaticDataFlags, 0 };\
                                                                                \
    QArrayDataPointerRef<Type> ref =                                            \
        { static_cast<QTypedArrayData<Type> *>(                                 \
            const_cast<QArrayData *>(&literal)),                                \
          const_cast<Type *>(data),                                             \
          Size };                                                               \
    /**/

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
