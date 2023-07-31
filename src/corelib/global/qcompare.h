// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPARE_H
#define QCOMPARE_H

#if 0
#pragma qt_class(QtCompare)
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qcompare_impl.h>

#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif

QT_BEGIN_NAMESPACE

namespace QtPrivate {
using CompareUnderlyingType = qint8;

// [cmp.categories.pre] / 1
enum class Ordering : CompareUnderlyingType
{
    Equal = 0,
    Equivalent = Equal,
    Less = -1,
    Greater = 1
};

enum class Uncomparable : CompareUnderlyingType
{
    Unordered = -127
};

} // namespace QtPrivate

// [cmp.partialord]
class QPartialOrdering
{
public:
    static const QPartialOrdering Less;
    static const QPartialOrdering Equivalent;
    static const QPartialOrdering Greater;
    static const QPartialOrdering Unordered;

    friend constexpr bool operator==(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order == 0; }

    friend constexpr bool operator!=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order != 0; }

    friend constexpr bool operator< (QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <  0; }

    friend constexpr bool operator<=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <= 0; }

    friend constexpr bool operator> (QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >  0; }

    friend constexpr bool operator>=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 >= rhs.m_order; }


    friend constexpr bool operator==(QPartialOrdering lhs, QPartialOrdering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(QPartialOrdering lhs, QPartialOrdering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT QPartialOrdering(std::partial_ordering stdorder) noexcept
    {
        if (stdorder == std::partial_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::partial_ordering::equivalent)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equivalent);
        else if (stdorder == std::partial_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
        else if (stdorder == std::partial_ordering::unordered)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Uncomparable::Unordered);
    }

    constexpr Q_IMPLICIT operator std::partial_ordering() const noexcept
    {
        if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Less)
            return std::partial_ordering::less;
        else if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Equivalent)
            return std::partial_ordering::equivalent;
        else if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Greater)
            return std::partial_ordering::greater;
        else if (static_cast<QtPrivate::Uncomparable>(m_order) == QtPrivate::Uncomparable::Unordered)
            return std::partial_ordering::unordered;
        return std::partial_ordering::unordered;
    }

    friend constexpr bool operator==(QPartialOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QPartialOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    friend class QWeakOrdering;
    friend class QStrongOrdering;

    constexpr explicit QPartialOrdering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}
    constexpr explicit QPartialOrdering(QtPrivate::Uncomparable order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    // instead of the exposition only is_ordered member in [cmp.partialord],
    // use a private function
    constexpr bool isOrdered() noexcept
    { return m_order != static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Uncomparable::Unordered); }

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr QPartialOrdering QPartialOrdering::Less(QtPrivate::Ordering::Less);
inline constexpr QPartialOrdering QPartialOrdering::Equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr QPartialOrdering QPartialOrdering::Greater(QtPrivate::Ordering::Greater);
inline constexpr QPartialOrdering QPartialOrdering::Unordered(QtPrivate::Uncomparable::Unordered);

class QWeakOrdering
{
public:
    static const QWeakOrdering Less;
    static const QWeakOrdering Equivalent;
    static const QWeakOrdering Greater;

    constexpr Q_IMPLICIT operator QPartialOrdering() const noexcept
    { return QPartialOrdering(static_cast<QtPrivate::Ordering>(m_order)); }

    friend constexpr bool operator==(QWeakOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order == 0; }

    friend constexpr bool operator!=(QWeakOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order != 0; }

    friend constexpr bool operator< (QWeakOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <  0; }

    friend constexpr bool operator<=(QWeakOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <= 0; }

    friend constexpr bool operator> (QWeakOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >  0; }

    friend constexpr bool operator>=(QWeakOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     QWeakOrdering rhs) noexcept
    { return 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     QWeakOrdering rhs) noexcept
    { return 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     QWeakOrdering rhs) noexcept
    { return 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     QWeakOrdering rhs) noexcept
    { return 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     QWeakOrdering rhs) noexcept
    { return 0 > rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     QWeakOrdering rhs) noexcept
    { return 0 >= rhs.m_order; }


    friend constexpr bool operator==(QWeakOrdering lhs, QWeakOrdering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(QWeakOrdering lhs, QWeakOrdering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    friend constexpr bool operator==(QWeakOrdering lhs, QPartialOrdering rhs) noexcept
    { return static_cast<QPartialOrdering>(lhs) == rhs; }

    friend constexpr bool operator!=(QWeakOrdering lhs, QPartialOrdering rhs) noexcept
    { return static_cast<QPartialOrdering>(lhs) != rhs; }

    friend constexpr bool operator==(QPartialOrdering lhs, QWeakOrdering rhs) noexcept
    { return lhs == static_cast<QPartialOrdering>(rhs); }

    friend constexpr bool operator!=(QPartialOrdering lhs, QWeakOrdering rhs) noexcept
    { return lhs != static_cast<QPartialOrdering>(rhs); }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT QWeakOrdering(std::weak_ordering stdorder) noexcept
    {
        if (stdorder == std::weak_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::weak_ordering::equivalent)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equivalent);
        else if (stdorder == std::weak_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
    }

    constexpr Q_IMPLICIT operator std::weak_ordering() const noexcept
    {
        if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Less)
            return std::weak_ordering::less;
        else if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Equivalent)
            return std::weak_ordering::equivalent;
        else if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Greater)
            return std::weak_ordering::greater;
        return std::weak_ordering::equivalent;
    }

    friend constexpr bool operator==(QWeakOrdering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QWeakOrdering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(QWeakOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QWeakOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(QWeakOrdering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QWeakOrdering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::weak_ordering lhs, QWeakOrdering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, QWeakOrdering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator==(std::partial_ordering lhs, QWeakOrdering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, QWeakOrdering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator==(std::strong_ordering lhs, QWeakOrdering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, QWeakOrdering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    friend class QStrongOrdering;

    constexpr explicit QWeakOrdering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr QWeakOrdering QWeakOrdering::Less(QtPrivate::Ordering::Less);
inline constexpr QWeakOrdering QWeakOrdering::Equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr QWeakOrdering QWeakOrdering::Greater(QtPrivate::Ordering::Greater);

class QStrongOrdering
{
public:
    static const QStrongOrdering Less;
    static const QStrongOrdering Equivalent;
    static const QStrongOrdering Equal;
    static const QStrongOrdering Greater;

    constexpr Q_IMPLICIT operator QPartialOrdering() const noexcept
    { return QPartialOrdering(static_cast<QtPrivate::Ordering>(m_order)); }

    constexpr Q_IMPLICIT operator QWeakOrdering() const noexcept
    { return QWeakOrdering(static_cast<QtPrivate::Ordering>(m_order)); }

    friend constexpr bool operator==(QStrongOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order == 0; }

    friend constexpr bool operator!=(QStrongOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order != 0; }

    friend constexpr bool operator< (QStrongOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <  0; }

    friend constexpr bool operator<=(QStrongOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <= 0; }

    friend constexpr bool operator> (QStrongOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >  0; }

    friend constexpr bool operator>=(QStrongOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     QStrongOrdering rhs) noexcept
    { return 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     QStrongOrdering rhs) noexcept
    { return 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                    QStrongOrdering rhs) noexcept
    { return 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     QStrongOrdering rhs) noexcept
    { return 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                    QStrongOrdering rhs) noexcept
    { return 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     QStrongOrdering rhs) noexcept
    { return 0 >= rhs.m_order; }


    friend constexpr bool operator==(QStrongOrdering lhs, QStrongOrdering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(QStrongOrdering lhs, QStrongOrdering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    friend constexpr bool operator==(QStrongOrdering lhs, QPartialOrdering rhs) noexcept
    { return static_cast<QPartialOrdering>(lhs) == rhs; }

    friend constexpr bool operator!=(QStrongOrdering lhs, QPartialOrdering rhs) noexcept
    { return static_cast<QPartialOrdering>(lhs) == rhs; }

    friend constexpr bool operator==(QPartialOrdering lhs, QStrongOrdering rhs) noexcept
    { return lhs == static_cast<QPartialOrdering>(rhs); }

    friend constexpr bool operator!=(QPartialOrdering lhs, QStrongOrdering rhs) noexcept
    { return lhs != static_cast<QPartialOrdering>(rhs); }

    friend constexpr bool operator==(QStrongOrdering lhs, QWeakOrdering rhs) noexcept
    { return static_cast<QWeakOrdering>(lhs) == rhs; }

    friend constexpr bool operator!=(QStrongOrdering lhs, QWeakOrdering rhs) noexcept
    { return static_cast<QWeakOrdering>(lhs) == rhs; }

    friend constexpr bool operator==(QWeakOrdering lhs, QStrongOrdering rhs) noexcept
    { return lhs == static_cast<QWeakOrdering>(rhs); }

    friend constexpr bool operator!=(QWeakOrdering lhs, QStrongOrdering rhs) noexcept
    { return lhs != static_cast<QWeakOrdering>(rhs); }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT QStrongOrdering(std::strong_ordering stdorder) noexcept
    {
        if (stdorder == std::strong_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::strong_ordering::equivalent)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equivalent);
        else if (stdorder == std::strong_ordering::equal)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equal);
        else if (stdorder == std::strong_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
    }

    constexpr Q_IMPLICIT operator std::strong_ordering() const noexcept
    {
        if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Less)
            return std::strong_ordering::less;
        else if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Equivalent)
            return std::strong_ordering::equivalent;
        else if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Equal)
            return std::strong_ordering::equal;
        else if (static_cast<QtPrivate::Ordering>(m_order) == QtPrivate::Ordering::Greater)
            return std::strong_ordering::greater;
        return std::strong_ordering::equivalent;
    }

    friend constexpr bool operator==(QStrongOrdering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QStrongOrdering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(QStrongOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QStrongOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(QStrongOrdering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QStrongOrdering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::strong_ordering lhs, QStrongOrdering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, QStrongOrdering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator==(std::partial_ordering lhs, QStrongOrdering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, QStrongOrdering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator==(std::weak_ordering lhs, QStrongOrdering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, QStrongOrdering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

    private:
    constexpr explicit QStrongOrdering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr QStrongOrdering QStrongOrdering::Less(QtPrivate::Ordering::Less);
inline constexpr QStrongOrdering QStrongOrdering::Equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr QStrongOrdering QStrongOrdering::Equal(QtPrivate::Ordering::Equal);
inline constexpr QStrongOrdering QStrongOrdering::Greater(QtPrivate::Ordering::Greater);

QT_END_NAMESPACE

#endif // QCOMPARE_H
