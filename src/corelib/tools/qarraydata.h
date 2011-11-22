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

#ifndef QARRAYDATA_H
#define QARRAYDATA_H

#include <QtCore/qrefcount.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

struct Q_CORE_EXPORT QArrayData
{
    QtPrivate::RefCount ref;
    int size;
    uint alloc : 31;
    uint capacityReserved : 1;

    qptrdiff offset; // in bytes from beginning of header

    void *data()
    {
        Q_ASSERT(size == 0
                || offset < 0 || size_t(offset) >= sizeof(QArrayData));
        return reinterpret_cast<char *>(this) + offset;
    }

    const void *data() const
    {
        Q_ASSERT(size == 0
                || offset < 0 || size_t(offset) >= sizeof(QArrayData));
        return reinterpret_cast<const char *>(this) + offset;
    }

    enum AllocateOption {
        CapacityReserved = 0x1,
        Unsharable = 0x2,

        Default = 0
    };

    Q_DECLARE_FLAGS(AllocateOptions, AllocateOption)

    AllocateOptions detachFlags() const
    {
        AllocateOptions result;
        if (!ref.isSharable())
            result |= Unsharable;
        if (capacityReserved)
            result |= CapacityReserved;
        return result;
    }

    AllocateOptions cloneFlags() const
    {
        AllocateOptions result;
        if (capacityReserved)
            result |= CapacityReserved;
        return result;
    }

    static QArrayData *allocate(size_t objectSize, size_t alignment,
            size_t capacity, AllocateOptions options = Default) Q_REQUIRED_RESULT;
    static void deallocate(QArrayData *data, size_t objectSize,
            size_t alignment);

    static const QArrayData shared_null;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QArrayData::AllocateOptions)

template <class T>
struct QTypedArrayData
    : QArrayData
{
    typedef T *iterator;
    typedef const T *const_iterator;

    T *data() { return static_cast<T *>(QArrayData::data()); }
    const T *data() const { return static_cast<const T *>(QArrayData::data()); }

    T *begin() { return data(); }
    T *end() { return data() + size; }
    const T *begin() const { return data(); }
    const T *end() const { return data() + size; }

    class AlignmentDummy { QArrayData header; T data; };

    static QTypedArrayData *allocate(size_t capacity,
            AllocateOptions options = Default) Q_REQUIRED_RESULT
    {
        return static_cast<QTypedArrayData *>(QArrayData::allocate(sizeof(T),
                    Q_ALIGNOF(AlignmentDummy), capacity, options));
    }

    static void deallocate(QArrayData *data)
    {
        QArrayData::deallocate(data, sizeof(T), Q_ALIGNOF(AlignmentDummy));
    }

    static QTypedArrayData *sharedNull()
    {
        return static_cast<QTypedArrayData *>(
                const_cast<QArrayData *>(&QArrayData::shared_null));
    }
};

template <class T, size_t N>
struct QStaticArrayData
{
    QArrayData header;
    T data[N];
};

#define Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(type, size) { \
    Q_REFCOUNT_INITIALIZE_STATIC, size, 0, 0, \
    (sizeof(QArrayData) + (Q_ALIGNOF(type) - 1)) \
        & ~(Q_ALIGNOF(type) - 1) } \
    /**/

QT_END_NAMESPACE

QT_END_HEADER

#endif // include guard
