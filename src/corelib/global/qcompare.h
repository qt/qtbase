/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Contact: http://www.qt.io/licensing/
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

    friend constexpr bool operator==(QPartialOrdering p, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return p.isOrdered() && p.m_order == 0; }
    friend constexpr bool operator!=(QPartialOrdering p, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return p.isOrdered() && p.m_order != 0; }
    friend constexpr bool operator< (QPartialOrdering p, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return p.isOrdered() && p.m_order <  0; }
    friend constexpr bool operator<=(QPartialOrdering p, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return p.isOrdered() && p.m_order <= 0; }
    friend constexpr bool operator> (QPartialOrdering p, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return p.isOrdered() && p.m_order >  0; }
    friend constexpr bool operator>=(QPartialOrdering p, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return p.isOrdered() && p.m_order >= 0; }

    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering p) noexcept
    { return p.isOrdered() && 0 == p.m_order; }
    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering p) noexcept
    { return p.isOrdered() && 0 != p.m_order; }
    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero, QPartialOrdering p) noexcept
    { return p.isOrdered() && 0 <  p.m_order; }
    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering p) noexcept
    { return p.isOrdered() && 0 <= p.m_order; }
    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero, QPartialOrdering p) noexcept
    { return p.isOrdered() && 0 >  p.m_order; }
    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering p) noexcept
    { return p.isOrdered() && 0 >= p.m_order; }

    friend constexpr bool operator==(QPartialOrdering p1, QPartialOrdering p2) noexcept
    { return p1.m_order == p2.m_order; }
    friend constexpr bool operator!=(QPartialOrdering p1, QPartialOrdering p2) noexcept
    { return p1.m_order != p2.m_order; }

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
