/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QCOMPARE_H
#define QCOMPARE_H

#if 0
#pragma qt_class(QtCompare)
#endif

#include <QtCore/qglobal.h>

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

// [cmp.categories.pre] / 3, but using a safe bool trick
// and also rejecting std::nullptr_t (unlike the example)
class CompareAgainstLiteralZero {
public:
    using SafeZero = void (CompareAgainstLiteralZero::*)();
    Q_IMPLICIT constexpr CompareAgainstLiteralZero(SafeZero) noexcept {}

    template <typename T, std::enable_if_t<!std::is_same_v<T, int>, bool> = false>
    CompareAgainstLiteralZero(T) = delete;
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

private:
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

QT_END_NAMESPACE

#endif // QCOMPARE_H
