// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QALGORITHMS_H
#define QALGORITHMS_H

#if 0
#pragma qt_class(QtAlgorithms)
#endif

#include <QtCore/qglobal.h>
#include <QtCore/q20bit.h>
#include <QtCore/q20functional.h>
#include <type_traits>

#define QT_HAS_CONSTEXPR_BITOPS

QT_BEGIN_NAMESPACE

template <typename ForwardIterator>
Q_OUTOFLINE_TEMPLATE void qDeleteAll(ForwardIterator begin, ForwardIterator end)
{
    while (begin != end) {
        delete *begin;
        ++begin;
    }
}

template <typename Container>
inline void qDeleteAll(const Container &c)
{
    qDeleteAll(c.begin(), c.end());
}


Q_DECL_CONST_FUNCTION constexpr inline uint qPopulationCount(quint32 v) noexcept
{
    return q20::popcount(v);
}

Q_DECL_CONST_FUNCTION constexpr inline uint qPopulationCount(quint8 v) noexcept
{
    return q20::popcount(v);
}

Q_DECL_CONST_FUNCTION constexpr inline uint qPopulationCount(quint16 v) noexcept
{
    return q20::popcount(v);
}

Q_DECL_CONST_FUNCTION constexpr inline uint qPopulationCount(quint64 v) noexcept
{
    return q20::popcount(v);
}

Q_DECL_CONST_FUNCTION constexpr inline uint qPopulationCount(long unsigned int v) noexcept
{
    return q20::popcount(v);
}

constexpr inline uint qCountTrailingZeroBits(quint32 v) noexcept
{
    return q20::countr_zero(v);
}

constexpr inline uint qCountTrailingZeroBits(quint8 v) noexcept
{
    return q20::countr_zero(v);
}

constexpr inline uint qCountTrailingZeroBits(quint16 v) noexcept
{
    return q20::countr_zero(v);
}

constexpr inline uint qCountTrailingZeroBits(quint64 v) noexcept
{
    return q20::countr_zero(v);
}

constexpr inline uint qCountTrailingZeroBits(unsigned long v) noexcept
{
    return q20::countr_zero(v);
}

constexpr inline uint qCountLeadingZeroBits(quint32 v) noexcept
{
    return q20::countl_zero(v);
}

constexpr inline uint qCountLeadingZeroBits(quint8 v) noexcept
{
    return q20::countl_zero(v);
}

constexpr inline uint qCountLeadingZeroBits(quint16 v) noexcept
{
    return q20::countl_zero(v);
}

constexpr inline uint qCountLeadingZeroBits(quint64 v) noexcept
{
    return q20::countl_zero(v);
}

constexpr inline uint qCountLeadingZeroBits(unsigned long v) noexcept
{
    return q20::countl_zero(v);
}

template <typename InputIterator, typename Result, typename Separator = Result,
          typename Projection = q20::identity>
Result qJoin(InputIterator first, InputIterator last, Result init, const Separator &separator = {},
             Projection p = {})
{
    if (first != last) {
        init += std::invoke(p, *first);
        ++first;
    }

    while (first != last) {
        init += separator;
        init += std::invoke(p, *first);
        ++first;
    }

    return init;
}

namespace QtPrivate {

template <typename T>
constexpr
std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>, int>
log2i(T x)
{
    // Integral -> int version of std::log2():
    Q_ASSERT(x > 0); // Q_PRE
    // C++20: return std::bit_width(x) - 1
    return int(sizeof(T) * 8 - 1 - qCountLeadingZeroBits(x));
}

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QALGORITHMS_H
