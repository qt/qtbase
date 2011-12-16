/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qarraydata.h>

QT_BEGIN_NAMESPACE

const QArrayData QArrayData::shared_null = { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, 0 };
const QArrayData QArrayData::shared_empty = { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, 0 };

QArrayData *QArrayData::allocate(size_t objectSize, size_t alignment,
        size_t capacity, bool reserve)
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= Q_ALIGNOF(QArrayData)
            && !(alignment & (alignment - 1)));

    // Don't allocate empty headers
    if (!capacity)
        return const_cast<QArrayData *>(&shared_empty);

    // Allocate extra (alignment - Q_ALIGNOF(QArrayData)) padding bytes so we
    // can properly align the data array. This assumes malloc is able to
    // provide appropriate alignment for the header -- as it should!
    size_t allocSize = sizeof(QArrayData) + objectSize * capacity
        + (alignment - Q_ALIGNOF(QArrayData));

    QArrayData *header = static_cast<QArrayData *>(qMalloc(allocSize));
    Q_CHECK_PTR(header);
    if (header) {
        quintptr data = (quintptr(header) + sizeof(QArrayData) + alignment - 1)
                & ~(alignment - 1);

        header->ref.initializeOwned();
        header->size = 0;
        header->alloc = capacity;
        header->capacityReserved = reserve;
        header->offset = data - quintptr(header);
    }

    return header;
}

void QArrayData::deallocate(QArrayData *data, size_t objectSize,
        size_t alignment)
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= Q_ALIGNOF(QArrayData)
            && !(alignment & (alignment - 1)));
    Q_UNUSED(objectSize) Q_UNUSED(alignment)

    qFree(data);
}

QT_END_NAMESPACE
