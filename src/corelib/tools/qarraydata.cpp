// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qalloc.h>
#include <QtCore/qarraydata.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/private/qtools_p.h>
#include <QtCore/qmath.h>

#include <QtCore/qbytearray.h>  // QBA::value_type
#include <QtCore/qstring.h>  // QString::value_type

#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*
 * This pair of functions is declared in qtools_p.h and is used by the Qt
 * containers to allocate memory and grow the memory block during append
 * operations.
 *
 * They take qsizetype parameters and return qsizetype so they will change sizes
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

    This function returns -1 on overflow or if the memory block size
    would not fit a qsizetype.
*/
qsizetype qCalculateBlockSize(qsizetype elementCount, qsizetype elementSize, qsizetype headerSize) noexcept
{
    Q_ASSERT(elementSize);

    size_t bytes;
    if (Q_UNLIKELY(qMulOverflow(size_t(elementSize), size_t(elementCount), &bytes)) ||
            Q_UNLIKELY(qAddOverflow(bytes, size_t(headerSize), &bytes)))
        return -1;
    if (Q_UNLIKELY(qsizetype(bytes) < 0))
        return -1;

    return qsizetype(bytes);
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

    This function returns -1 on overflow or if the memory block size
    would not fit a qsizetype.

    \note The memory block may contain up to \a elementSize - 1 bytes more than
    needed.
*/
CalculateGrowingBlockSizeResult
qCalculateGrowingBlockSize(qsizetype elementCount, qsizetype elementSize, qsizetype headerSize) noexcept
{
    CalculateGrowingBlockSizeResult result = {
        qsizetype(-1), qsizetype(-1)
    };

    qsizetype bytes = qCalculateBlockSize(elementCount, elementSize, headerSize);
    if (bytes < 0)
        return result;

    size_t morebytes = static_cast<size_t>(qNextPowerOfTwo(quint64(bytes)));
    if (Q_UNLIKELY(qsizetype(morebytes) < 0)) {
        // grow by half the difference between bytes and morebytes
        // this slows the growth and avoids trying to allocate exactly
        // 2G of memory (on 32bit), something that many OSes can't deliver
        bytes += (morebytes - bytes) / 2;
    } else {
        bytes = qsizetype(morebytes);
    }
    size_t fittedBytes = QtPrivate::expectedAllocSize(bytes, alignof(std::max_align_t));
    if (fittedBytes != 0)
        bytes = fittedBytes;

    result.elementCount = (bytes - headerSize) / elementSize;
    result.size = result.elementCount * elementSize + headerSize;
    return result;
}

using QtPrivate::AlignedQArrayData;

static qsizetype calculateHeaderSize(qsizetype alignment)
{
    qsizetype headerSize = sizeof(AlignedQArrayData);
    const qsizetype headerAlignment = alignof(AlignedQArrayData);

    if (alignment > headerAlignment) {
        // Allocate extra (alignment - Q_ALIGNOF(AlignedQArrayData)) padding
        // bytes so we can properly align the data array. This assumes malloc is
        // able to provide appropriate alignment for the header -- as it should!
        // Effectively, we allocate one QTypedArrayData<T>::AlignmentDummy.
        headerSize += alignment - headerAlignment;
    }
    Q_ASSERT(headerSize > 0);

    return headerSize;
}

/*
    Calculate the byte size for a block of \a capacity objects of size \a
    objectSize, with a header of size \a headerSize. If the \a option is
    QArrayData::Grow, the capacity itself adjusted up, preallocating room for
    more elements to be added later; otherwise, it is an exact calculation.

    Returns a structure containing the size in bytes and elements available.
*/
static inline CalculateGrowingBlockSizeResult
calculateBlockSize(qsizetype capacity, qsizetype objectSize, qsizetype headerSize, QArrayData::AllocationOption option)
{
    // Adjust the header size up to account for the trailing null for QString
    // and QByteArray. This is not checked for overflow because headers sizes
    // should not be anywhere near the overflow limit.
    constexpr qsizetype FooterSize = qMax(sizeof(QString::value_type), sizeof(QByteArray::value_type));
    if (objectSize <= FooterSize)
        headerSize += FooterSize;

    // allocSize = objectSize * capacity + headerSize, but checked for overflow
    // plus padded to grow in size
    if (option == QArrayData::Grow) {
        return qCalculateGrowingBlockSize(capacity, objectSize, headerSize);
    } else {
        return { qCalculateBlockSize(capacity, objectSize, headerSize), capacity };
    }
}

static inline void *
allocateHelper(QArrayData **dptr, qsizetype objectSize, qsizetype alignment, qsizetype capacity,
               QArrayData::AllocationOption option) noexcept
{
    *dptr = nullptr;
    if (capacity == 0)
        return {};

    const qsizetype headerSize = calculateHeaderSize(alignment);
    Q_ASSERT(headerSize > 0);

    auto blockSize = calculateBlockSize(capacity, objectSize, headerSize, option);
    capacity = blockSize.elementCount;
    qsizetype allocSize = blockSize.size;
    if (Q_UNLIKELY(allocSize < 0))      // handle overflow. cannot allocate reliably
        return {};

    void *data = nullptr;
    void *mem = ::malloc(size_t(allocSize));
    if (Q_LIKELY(mem)) {
        *dptr = new (mem) QArrayData{1, {}, capacity};
        // find where offset should point to so that data() is aligned to alignment bytes
        data = QTypedArrayData<void>::dataStart(*dptr, alignment);
    }

    return data;
}

// Generic size and alignment allocation function
void *QArrayData::allocate(QArrayData **dptr, qsizetype objectSize, qsizetype alignment,
                           qsizetype capacity, AllocationOption option) noexcept
{
    Q_ASSERT(dptr);
    // Alignment is a power of two
    Q_ASSERT(alignment >= qsizetype(alignof(QArrayData))
            && !(alignment & (alignment - 1)));

    return allocateHelper(dptr, objectSize, alignment, capacity, option);
}

// Fixed size and alignment allocation functions
void *QArrayData::allocate1(QArrayData **dptr, qsizetype capacity, AllocationOption option) noexcept
{
    Q_ASSERT(dptr);

    return allocateHelper(dptr, 1, alignof(AlignedQArrayData), capacity, option);
}

void *QArrayData::allocate2(QArrayData **dptr, qsizetype capacity, AllocationOption option) noexcept
{
    Q_ASSERT(dptr);

    return allocateHelper(dptr, 2, alignof(AlignedQArrayData), capacity, option);
}

std::pair<QArrayData *, void *>
QArrayData::reallocateUnaligned(QArrayData *data, void *dataPointer,
                                qsizetype objectSize, qsizetype capacity, AllocationOption option) noexcept
{
    Q_ASSERT(!data || !data->isShared());

    const qsizetype headerSize = sizeof(AlignedQArrayData);
    auto r = calculateBlockSize(capacity, objectSize, headerSize, option);
    qsizetype allocSize = r.size;
    capacity = r.elementCount;
    if (Q_UNLIKELY(allocSize < 0))
        return {};

    const qptrdiff offset = dataPointer
            ? reinterpret_cast<char *>(dataPointer) - reinterpret_cast<char *>(data)
            : headerSize;
    Q_ASSERT(offset > 0);
    Q_ASSERT(offset <= allocSize); // equals when all free space is at the beginning

    const bool hadData = data;
    void *mem = ::realloc(data, size_t(allocSize));
    QArrayData *header = static_cast<QArrayData *>(mem);
    if (mem) {
        if (!hadData)
            header = new (mem) QArrayData{0, {}, {}};
        header->alloc = capacity;
        dataPointer = reinterpret_cast<char *>(header) + offset;
    } else {
        dataPointer = nullptr;
    }
    return {header, dataPointer};
}

void QArrayData::deallocate(QArrayData *data, qsizetype objectSize,
        qsizetype alignment) noexcept
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= qsizetype(alignof(QArrayData))
            && !(alignment & (alignment - 1)));

    const qsizetype capacity = data->alloc;
    const qsizetype headerSize = calculateHeaderSize(alignment);
    Q_ASSERT(headerSize > 0);
    const auto blockSize = calculateBlockSize(capacity, objectSize,
                                              headerSize, QArrayData::KeepSize);
    const qsizetype allocSize = blockSize.size;

    if (Q_LIKELY(allocSize > 0))
        QtPrivate::sizedFree(data, size_t(allocSize));
    else // something went wrong, fallback to slow free()
        free(data);
}

QT_END_NAMESPACE
