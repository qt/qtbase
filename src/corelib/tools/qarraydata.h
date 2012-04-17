/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QARRAYDATA_H
#define QARRAYDATA_H

#include <QtCore/qrefcount.h>
#include <string.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

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

    // This refers to array data mutability, not "header data" represented by
    // data members in QArrayData. Shared data (array and header) must still
    // follow COW principles.
    bool isMutable() const
    {
        return alloc != 0;
    }

    enum AllocationOption {
        CapacityReserved    = 0x1,
        Unsharable          = 0x2,
        RawData             = 0x4,
        Grow                = 0x8,

        Default = 0
    };

    Q_DECLARE_FLAGS(AllocationOptions, AllocationOption)

    size_t detachCapacity(size_t newSize) const
    {
        if (capacityReserved && newSize < alloc)
            return alloc;
        return newSize;
    }

    AllocationOptions detachFlags() const
    {
        AllocationOptions result;
        if (!ref.isSharable())
            result |= Unsharable;
        if (capacityReserved)
            result |= CapacityReserved;
        return result;
    }

    AllocationOptions cloneFlags() const
    {
        AllocationOptions result;
        if (capacityReserved)
            result |= CapacityReserved;
        return result;
    }

    static QArrayData *allocate(size_t objectSize, size_t alignment,
            size_t capacity, AllocationOptions options = Default)
        Q_REQUIRED_RESULT;
    static void deallocate(QArrayData *data, size_t objectSize,
            size_t alignment);

    static const QArrayData shared_null[2];
    static QArrayData *sharedNull() { return const_cast<QArrayData*>(shared_null); }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QArrayData::AllocationOptions)

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
            AllocationOptions options = Default) Q_REQUIRED_RESULT
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        return static_cast<QTypedArrayData *>(QArrayData::allocate(sizeof(T),
                    Q_ALIGNOF(AlignmentDummy), capacity, options));
    }

    static void deallocate(QArrayData *data)
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QArrayData::deallocate(data, sizeof(T), Q_ALIGNOF(AlignmentDummy));
    }

    static QTypedArrayData *fromRawData(const T *data, size_t n,
            AllocationOptions options = Default)
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        QTypedArrayData *result = allocate(0, options | RawData);
        if (result) {
            Q_ASSERT(!result->ref.isShared()); // No shared empty, please!

            result->offset = reinterpret_cast<const char *>(data)
                - reinterpret_cast<const char *>(result);
            result->size = int(n);
        }
        return result;
    }

    static QTypedArrayData *sharedNull()
    {
        Q_STATIC_ASSERT(sizeof(QTypedArrayData) == sizeof(QArrayData));
        return static_cast<QTypedArrayData *>(QArrayData::sharedNull());
    }
};

template <class T, size_t N>
struct QStaticArrayData
{
    QArrayData header;
    T data[N];
};

// Support for returning QArrayDataPointer<T> from functions
template <class T>
struct QArrayDataPointerRef
{
    QTypedArrayData<T> *ptr;
};

#define Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(type, size) { \
    Q_REFCOUNT_INITIALIZE_STATIC, size, 0, 0, \
    (sizeof(QArrayData) + (Q_ALIGNOF(type) - 1)) \
        & ~(Q_ALIGNOF(type) - 1) } \
    /**/

////////////////////////////////////////////////////////////////////////////////
//  Q_ARRAY_LITERAL

// The idea here is to place a (read-only) copy of header and array data in an
// mmappable portion of the executable (typically, .rodata section). This is
// accomplished by hiding a static const instance of QStaticArrayData, which is
// POD.

#if defined(Q_COMPILER_VARIADIC_MACROS)
#if defined(Q_COMPILER_LAMBDA)
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
#elif defined(Q_CC_GNU)
// Hide array within GCC's __extension__ {( )} block
#define Q_ARRAY_LITERAL(Type, ...)                                              \
    __extension__ ({                                                            \
            Q_ARRAY_LITERAL_IMPL(Type, __VA_ARGS__)                             \
            ref;                                                                \
        })                                                                      \
    /**/
#endif
#endif // defined(Q_COMPILER_VARIADIC_MACROS)

#if defined(Q_ARRAY_LITERAL)
#define Q_ARRAY_LITERAL_IMPL(Type, ...)                                         \
    union { Type type_must_be_POD; } dummy; Q_UNUSED(dummy)                     \
                                                                                \
    /* Portable compile-time array size computation */                          \
    Type data[] = { __VA_ARGS__ }; Q_UNUSED(data)                               \
    enum { Size = sizeof(data) / sizeof(data[0]) };                             \
                                                                                \
    static const QStaticArrayData<Type, Size> literal = {                       \
        Q_STATIC_ARRAY_DATA_HEADER_INITIALIZER(Type, Size), { __VA_ARGS__ } };  \
                                                                                \
    QArrayDataPointerRef<Type> ref =                                            \
        { static_cast<QTypedArrayData<Type> *>(                                 \
            const_cast<QArrayData *>(&literal.header)) };                       \
    /**/
#else
// As a fallback, memory is allocated and data copied to the heap.

// The fallback macro does NOT use variadic macros and does NOT support
// variable number of arguments. It is suitable for char arrays.

namespace QtPrivate {
    template <class T, size_t N>
    inline QArrayDataPointerRef<T> qMakeArrayLiteral(const T (&array)[N])
    {
        union { T type_must_be_POD; } dummy; Q_UNUSED(dummy)

        QArrayDataPointerRef<T> result = { QTypedArrayData<T>::allocate(N) };
        Q_CHECK_PTR(result.ptr);

        ::memcpy(result.ptr->data(), array, N * sizeof(T));
        result.ptr->size = N;

        return result;
    }
}

#define Q_ARRAY_LITERAL(Type, Array) \
    QT_PREPEND_NAMESPACE(QtPrivate::qMakeArrayLiteral)<Type>( Array )
#endif // !defined(Q_ARRAY_LITERAL)

QT_END_NAMESPACE

QT_END_HEADER

#endif // include guard
