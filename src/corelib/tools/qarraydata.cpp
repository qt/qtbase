/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qarraydata.h>
#include <QtCore/private/qtools_p.h>

#include <stdlib.h>

QT_BEGIN_NAMESPACE

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

const QArrayData QArrayData::shared_null[2] = {
    { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, sizeof(QArrayData) }, // shared null
    /* zero initialized terminator */};

static const QArrayData qt_array[3] = {
    { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, sizeof(QArrayData) }, // shared empty
    { { Q_BASIC_ATOMIC_INITIALIZER(0) }, 0, 0, 0, sizeof(QArrayData) }, // unsharable empty
    /* zero initialized terminator */};

QT_WARNING_POP

static const QArrayData &qt_array_empty = qt_array[0];
static const QArrayData &qt_array_unsharable_empty = qt_array[1];

QArrayData *QArrayData::allocate(size_t objectSize, size_t alignment,
        size_t capacity, AllocationOptions options) Q_DECL_NOTHROW
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= Q_ALIGNOF(QArrayData)
            && !(alignment & (alignment - 1)));

    // Don't allocate empty headers
    if (!(options & RawData) && !capacity) {
#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
        if (options & Unsharable)
            return const_cast<QArrayData *>(&qt_array_unsharable_empty);
#endif
        return const_cast<QArrayData *>(&qt_array_empty);
    }

    size_t headerSize = sizeof(QArrayData);

    // Allocate extra (alignment - Q_ALIGNOF(QArrayData)) padding bytes so we
    // can properly align the data array. This assumes malloc is able to
    // provide appropriate alignment for the header -- as it should!
    // Padding is skipped when allocating a header for RawData.
    if (!(options & RawData))
        headerSize += (alignment - Q_ALIGNOF(QArrayData));

    // Allocate additional space if array is growing
    if (options & Grow) {

        // Guard against integer overflow when multiplying.
        if (capacity > std::numeric_limits<size_t>::max() / objectSize)
            return 0;

        size_t alloc = objectSize * capacity;

        // Make sure qAllocMore won't overflow.
        if (headerSize > size_t(MaxAllocSize) || alloc > size_t(MaxAllocSize) - headerSize)
            return 0;

        capacity = qAllocMore(int(alloc), int(headerSize)) / int(objectSize);
    }

    size_t allocSize = headerSize + objectSize * capacity;

    QArrayData *header = static_cast<QArrayData *>(::malloc(allocSize));
    if (header) {
        quintptr data = (quintptr(header) + sizeof(QArrayData) + alignment - 1)
                & ~(alignment - 1);

#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
        header->ref.atomic.store(bool(!(options & Unsharable)));
#else
        header->ref.atomic.store(1);
#endif
        header->size = 0;
        header->alloc = capacity;
        header->capacityReserved = bool(options & CapacityReserved);
        header->offset = data - quintptr(header);
    }

    return header;
}

void QArrayData::deallocate(QArrayData *data, size_t objectSize,
        size_t alignment) Q_DECL_NOTHROW
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= Q_ALIGNOF(QArrayData)
            && !(alignment & (alignment - 1)));
    Q_UNUSED(objectSize) Q_UNUSED(alignment)

#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
    if (data == &qt_array_unsharable_empty)
        return;
#endif

    Q_ASSERT_X(data == 0 || !data->ref.isStatic(), "QArrayData::deallocate",
               "Static data can not be deleted");
    ::free(data);
}

namespace QtPrivate {
/*!
  \internal
*/
QContainerImplHelper::CutResult QContainerImplHelper::mid(int originalLength, int *_position, int *_length)
{
    int &position = *_position;
    int &length = *_length;
    if (position > originalLength)
        return Null;

    if (position < 0) {
        if (length < 0 || length + position >= originalLength)
            return Full;
        if (length + position <= 0)
            return Null;
        length += position;
        position = 0;
    } else if (uint(length) > uint(originalLength - position)) {
        length = originalLength - position;
    }

    if (position == 0 && length == originalLength)
        return Full;

    return length > 0 ? Subset : Empty;
}
}

QT_END_NAMESPACE
