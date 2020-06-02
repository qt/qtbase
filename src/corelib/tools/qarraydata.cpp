/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtCore/qarraydata.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/private/qtools_p.h>
#include <QtCore/qmath.h>

#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*
 * This pair of functions is declared in qtools_p.h and is used by the Qt
 * containers to allocate memory and grow the memory block during append
 * operations.
 *
 * They take size_t parameters and return size_t so they will change sizes
 * according to the pointer width. However, knowing Qt containers store the
 * container size and element indexes in ints, these functions never return a
 * size larger than INT_MAX. This is done by casting the element count and
 * memory block size to int in several comparisons: the check for negative is
 * very fast on most platforms as the code only needs to check the sign bit.
 *
 * These functions return SIZE_MAX on overflow, which can be passed to malloc()
 * and will surely cause a NULL return (there's no way you can allocate a
 * memory block the size of your entire VM space).
 */

/*!
    \internal
    \since 5.7

    Returns the memory block size for a container containing \a elementCount
    elements, each of \a elementSize bytes, plus a header of \a headerSize
    bytes. That is, this function returns \c
      {elementCount * elementSize + headerSize}

    but unlike the simple calculation, it checks for overflows during the
    multiplication and the addition.

    Both \a elementCount and \a headerSize can be zero, but \a elementSize
    cannot.

    This function returns SIZE_MAX (~0) on overflow or if the memory block size
    would not fit an int.
*/
size_t qCalculateBlockSize(size_t elementCount, size_t elementSize, size_t headerSize) noexcept
{
    unsigned count = unsigned(elementCount);
    unsigned size = unsigned(elementSize);
    unsigned header = unsigned(headerSize);
    Q_ASSERT(elementSize);
    Q_ASSERT(size == elementSize);
    Q_ASSERT(header == headerSize);

    if (Q_UNLIKELY(count != elementCount))
        return std::numeric_limits<size_t>::max();

    unsigned bytes;
    if (Q_UNLIKELY(mul_overflow(size, count, &bytes)) ||
            Q_UNLIKELY(add_overflow(bytes, header, &bytes)))
        return std::numeric_limits<size_t>::max();
    if (Q_UNLIKELY(int(bytes) < 0))     // catches bytes >= 2GB
        return std::numeric_limits<size_t>::max();

    return bytes;
}

/*!
    \internal
    \since 5.7

    Returns the memory block size and the number of elements that will fit in
    that block for a container containing \a elementCount elements, each of \a
    elementSize bytes, plus a header of \a headerSize bytes. This function
    assumes the container will grow and pre-allocates a growth factor.

    Both \a elementCount and \a headerSize can be zero, but \a elementSize
    cannot.

    This function returns SIZE_MAX (~0) on overflow or if the memory block size
    would not fit an int.

    \note The memory block may contain up to \a elementSize - 1 bytes more than
    needed.
*/
CalculateGrowingBlockSizeResult
qCalculateGrowingBlockSize(size_t elementCount, size_t elementSize, size_t headerSize) noexcept
{
    CalculateGrowingBlockSizeResult result = {
        std::numeric_limits<size_t>::max(),std::numeric_limits<size_t>::max()
    };

    unsigned bytes = unsigned(qCalculateBlockSize(elementCount, elementSize, headerSize));
    if (int(bytes) < 0)     // catches std::numeric_limits<size_t>::max()
        return result;

    unsigned morebytes = qNextPowerOfTwo(bytes);
    if (Q_UNLIKELY(int(morebytes) < 0)) {
        // catches morebytes == 2GB
        // grow by half the difference between bytes and morebytes
        bytes += (morebytes - bytes) / 2;
    } else {
        bytes = morebytes;
    }

    result.elementCount = (bytes - unsigned(headerSize)) / unsigned(elementSize);
    result.size = result.elementCount * elementSize + headerSize;
    return result;
}

// End of qtools_p.h implementation

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

const QArrayData QArrayData::shared_null[2] = {
    { Q_BASIC_ATOMIC_INITIALIZER(-1), QArrayData::StaticDataFlags, 0 }, // shared null
    /* zero initialized terminator */};

static const QArrayData emptyNotNullShared[2] = {
    { Q_BASIC_ATOMIC_INITIALIZER(-1), QArrayData::StaticDataFlags, 0 }, // shared empty
    /* zero initialized terminator */};

QT_WARNING_POP

static const QArrayData &qt_array_empty = emptyNotNullShared[0];

static inline size_t calculateBlockSize(size_t &capacity, size_t objectSize, size_t headerSize,
                                        uint options)
{
    // Calculate the byte size
    // allocSize = objectSize * capacity + headerSize, but checked for overflow
    // plus padded to grow in size
    if (options & QArrayData::GrowsForward) {
        auto r = qCalculateGrowingBlockSize(capacity, objectSize, headerSize);
        capacity = r.elementCount;
        return r.size;
    } else {
        return qCalculateBlockSize(capacity, objectSize, headerSize);
    }
}

static QArrayData *allocateData(size_t allocSize, uint options)
{
    QArrayData *header = static_cast<QArrayData *>(::malloc(allocSize));
    if (header) {
        header->ref_.storeRelaxed(1);
        header->flags = options;
        header->alloc = 0;
    }
    return header;
}

static QArrayData *reallocateData(QArrayData *header, size_t allocSize, uint options)
{
    header = static_cast<QArrayData *>(::realloc(header, allocSize));
    if (header)
        header->flags = options;
    return header;
}

void *QArrayData::allocate(QArrayData **dptr, size_t objectSize, size_t alignment,
        size_t capacity, ArrayOptions options) noexcept
{
    Q_ASSERT(dptr);
    // Alignment is a power of two
    Q_ASSERT(alignment >= alignof(QArrayData)
            && !(alignment & (alignment - 1)));

    if (capacity == 0) {
        // optimization for empty headers
        *dptr = const_cast<QArrayData *>(&qt_array_empty);
        return sharedNullData();
    }

    size_t headerSize = sizeof(QArrayData);

    if (alignment > alignof(QArrayData)) {
        // Allocate extra (alignment - Q_ALIGNOF(QArrayData)) padding bytes so we
        // can properly align the data array. This assumes malloc is able to
        // provide appropriate alignment for the header -- as it should!
        headerSize += alignment - alignof(QArrayData);
    }

    if (headerSize > size_t(MaxAllocSize))
        return nullptr;

    size_t allocSize = calculateBlockSize(capacity, objectSize, headerSize, options);
    options |= AllocatedDataType | MutableData;
    options &= ~ImmutableHeader;
    QArrayData *header = allocateData(allocSize, options);
    quintptr data = 0;
    if (header) {
        // find where offset should point to so that data() is aligned to alignment bytes
        data = (quintptr(header) + sizeof(QArrayData) + alignment - 1)
                & ~(alignment - 1);
        header->alloc = uint(capacity);
    }

    *dptr = header;
    return reinterpret_cast<void *>(data);
}

QArrayData *QArrayData::prepareRawData(ArrayOptions options) Q_DECL_NOTHROW
{
    QArrayData *header = allocateData(sizeof(QArrayData), (options & ~DataTypeBits) | RawDataType);
    if (header)
        header->alloc = 0;
    return header;
}

QPair<QArrayData *, void *>
QArrayData::reallocateUnaligned(QArrayData *data, void *dataPointer,
                                size_t objectSize, size_t capacity, ArrayOptions options) noexcept
{
    Q_ASSERT(data);
    Q_ASSERT(data->isMutable());
    Q_ASSERT(!data->isShared());

    size_t headerSize = sizeof(QArrayData);
    size_t allocSize = calculateBlockSize(capacity, objectSize, headerSize, options);
    qptrdiff offset = reinterpret_cast<char *>(dataPointer) - reinterpret_cast<char *>(data);
    options |= AllocatedDataType | MutableData;
    QArrayData *header = reallocateData(data, allocSize, options);
    if (header) {
        header->alloc = uint(capacity);
        dataPointer = reinterpret_cast<char *>(header) + offset;
    }
    return qMakePair(static_cast<QArrayData *>(header), dataPointer);
}

void QArrayData::deallocate(QArrayData *data, size_t objectSize,
        size_t alignment) noexcept
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= alignof(QArrayData)
            && !(alignment & (alignment - 1)));
    Q_UNUSED(objectSize) Q_UNUSED(alignment)

    Q_ASSERT_X(data == nullptr || !data->isStatic(), "QArrayData::deallocate",
               "Static data cannot be deleted");
    ::free(data);
}

QT_END_NAMESPACE
