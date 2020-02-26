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

#ifndef QTAGGEDPOINTER_H
#define QTAGGEDPOINTER_H

#include <QtCore/qglobal.h>
#include <QtCore/qalgorithms.h>
#include <QtCore/qmath.h>

#include <limits.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
    constexpr quint8 nextByteSize(quint8 bits) { return (bits + 7) / 8; }

    template <typename T>
    struct TagInfo
    {
        static constexpr size_t alignment = alignof(T);
        static_assert((alignment & (alignment - 1)) == 0,
            "Alignment of template parameter must be power of two");

        static constexpr quint8 tagBits = QtPrivate::qConstexprCountTrailingZeroBits(alignment);
        static_assert(tagBits > 0,
            "Alignment of template parameter does not allow any tags");

        static constexpr size_t tagSize = QtPrivate::qConstexprNextPowerOfTwo(nextByteSize(tagBits));
        static_assert(tagSize < sizeof(quintptr),
            "Alignment of template parameter allows tags masking away pointer");

        using TagType = typename QIntegerForSize<tagSize>::Unsigned;
    };
}

template <typename T, typename Tag = typename QtPrivate::TagInfo<T>::TagType>
class QTaggedPointer
{
    static constexpr quintptr tagMask() { return QtPrivate::TagInfo<T>::alignment - 1; }
    static constexpr quintptr pointerMask() { return ~tagMask(); }

public:
    using Type = T;
    using TagType = Tag;

    explicit QTaggedPointer(Type *pointer = nullptr, TagType tag = TagType()) noexcept
        : d(quintptr(pointer))
    {
        Q_STATIC_ASSERT(sizeof(Type*) == sizeof(QTaggedPointer<Type>));

        Q_ASSERT_X((quintptr(pointer) & tagMask()) == 0,
            "QTaggedPointer<T, Tag>", "Pointer is not aligned");

        setTag(tag);
    }

    Type &operator*() const
    {
        Q_ASSERT(pointer());
        return *pointer();
    }

    Type *operator->() const noexcept
    {
        return pointer();
    }

    explicit operator bool() const
    {
        return !isNull();
    }

    QTaggedPointer<T, Tag> &operator=(T *other) noexcept
    {
        d = reinterpret_cast<quintptr>(other) | (d & tagMask());
        return *this;
    }

    QTaggedPointer<T, Tag> &operator=(std::nullptr_t) noexcept
    {
        d &= tagMask();
        return *this;
    }

    static constexpr TagType maximumTag() noexcept
    {
        return TagType(typename QtPrivate::TagInfo<T>::TagType(tagMask()));
    }

    void setTag(TagType tag)
    {
        Q_ASSERT_X((static_cast<typename QtPrivate::TagInfo<T>::TagType>(tag) & pointerMask()) == 0,
            "QTaggedPointer<T, Tag>::setTag", "Tag is larger than allowed by number of available tag bits");

        d = (d & pointerMask()) | (static_cast<typename QtPrivate::TagInfo<T>::TagType>(tag) & tagMask());
    }

    TagType tag() const noexcept
    {
        return TagType(typename QtPrivate::TagInfo<T>::TagType(d & tagMask()));
    }

    Type* pointer() const noexcept
    {
        return reinterpret_cast<T*>(d & pointerMask());
    }

    bool isNull() const noexcept
    {
        return !pointer();
    }

    void swap(QTaggedPointer<Type, Tag> &other) noexcept
    {
        qSwap(d, other.d);
    }

    friend inline bool operator==(const QTaggedPointer<T, Tag> &lhs, const QTaggedPointer<T, Tag> &rhs) noexcept
    {
        return lhs.pointer() == rhs.pointer();
    }

    friend inline bool operator!=(const QTaggedPointer<T, Tag> &lhs, const QTaggedPointer<T, Tag> &rhs) noexcept
    {
        return lhs.pointer() != rhs.pointer();
    }

    friend inline bool operator==(const QTaggedPointer<T, Tag> &lhs, std::nullptr_t) noexcept
    {
        return lhs.isNull();
    }

    friend inline bool operator==(std::nullptr_t, const QTaggedPointer<T, Tag> &rhs) noexcept
    {
        return rhs.isNull();
    }

    friend inline bool operator!=(const QTaggedPointer<T, Tag> &lhs, std::nullptr_t) noexcept
    {
        return !lhs.isNull();
    }

    friend inline bool operator!=(std::nullptr_t, const QTaggedPointer<T, Tag> &rhs) noexcept
    {
        return !rhs.isNull();
    }

    friend inline bool operator!(const QTaggedPointer<T, Tag> &ptr) noexcept
    {
        return !ptr.pointer();
    }

protected:
    quintptr d;
};

template <typename T>
Q_DECLARE_TYPEINFO_BODY(QTaggedPointer<T>, Q_MOVABLE_TYPE);

template <typename T, typename Tag>
inline void swap(QTaggedPointer<T, Tag> &p1, QTaggedPointer<T, Tag> &p2) noexcept
{
    p1.swap(p2);
}

QT_END_NAMESPACE

#endif // QTAGGEDPOINTER_H
