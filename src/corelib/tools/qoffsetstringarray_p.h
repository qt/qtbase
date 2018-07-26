/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QOFFSETSTRINGARRAY_P_H
#define QOFFSETSTRINGARRAY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qglobal_p.h"

#include <tuple>
#include <array>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
template<int N, int O, int I, int ... Idx>
struct OffsetSequenceHelper : OffsetSequenceHelper<N - 1, O + I, Idx..., O> { };

template<int O, int I, int ... Idx>
struct OffsetSequenceHelper<0, O, I, Idx...> : IndexesList<O, Idx...>
{
    static const constexpr auto Length = O;
};

template<int I, int ... Idx>
struct OffsetSequence : OffsetSequenceHelper<sizeof ... (Idx) + 1, 0, I, Idx..., 0>
{
    static const constexpr auto Count = sizeof ... (Idx) + 1;
    using Type = typename QConditional<
        Count <= std::numeric_limits<quint8>::max() + 1,
        quint8,
        typename QConditional<
            Count <= std::numeric_limits<quint16>::max() + 1,
            quint16,
            int>::Type
        >::Type;
};

template<int N>
struct StaticString
{
    const char data[N];

    constexpr StaticString(const char (&literal)[N]) noexcept
        : StaticString(literal, makeIndexSequence<N> {})
    { }

    template<int ... Idx>
    constexpr StaticString(const char (&literal)[N],
                           IndexesList<Idx...>) noexcept
        : data{literal[Idx]...}
    { }

    template<typename ... T>
    constexpr StaticString(const T ... c) noexcept : data{c...} { }

    constexpr char operator[](int i) const noexcept
    {
        return data[i];
    }

    template<int N2>
    constexpr StaticString<N + N2> operator+(const StaticString<N2> &rs) const noexcept
    {
        return concatenate(rs, makeIndexSequence<N>{}, makeIndexSequence<N2>{});
    }

    template<int N2, int ... I1, int ... I2>
    constexpr StaticString<N + N2> concatenate(const StaticString<N2> &rs,
                                               IndexesList<I1...>,
                                               IndexesList<I2...>) const noexcept
    {
        return StaticString<N + N2>(data[I1]..., rs[I2]...);
    }

    constexpr int size() const noexcept
    {
        return N;
    }
};

template<>
struct StaticString<0>
{
};

template<int Sum>
constexpr StaticString<0> staticString() noexcept
{
    return StaticString<0>{};
}

template<int Sum, int I, int ... Ix>
constexpr StaticString<Sum> staticString(const char (&s)[I], const char (&...sx)[Ix]) noexcept
{
    return StaticString<I>(s) + staticString<Sum - I>(sx...);
}
} // namespace QtPrivate

template<typename T, int SizeString, int SizeOffsets>
class QOffsetStringArray
{
public:
    using Type = T;

    template<int ... Cx, int ... Ox>
    constexpr QOffsetStringArray(const QtPrivate::StaticString<SizeString> &str,
                                 QtPrivate::IndexesList<Cx...>,
                                 QtPrivate::IndexesList<SizeString, Ox...>) noexcept
        : m_string{str[Cx]...},
          m_offsets{Ox...}
    { }

    constexpr inline const char *operator[](const int index) const noexcept
    {
        return m_string + m_offsets[qBound(int(0), index, SizeOffsets - 1)];
    }

    constexpr inline const char *at(const int index) const noexcept
    {
        return m_string + m_offsets[index];
    }

    constexpr inline const char *str() const { return m_string; }
    constexpr inline const int *offsets() const { return m_offsets; }
    constexpr inline int count() const { return SizeOffsets; };

    static constexpr const auto sizeString = SizeString;
    static constexpr const auto sizeOffsets = SizeOffsets;

private:
    const char m_string[SizeString];
    const T m_offsets[SizeOffsets];
};

template<typename T, int N, int ... Ox>
constexpr QOffsetStringArray<T, N, sizeof ... (Ox)> qOffsetStringArray(
    const QtPrivate::StaticString<N> &string,
    QtPrivate::IndexesList<N, Ox...> offsets) noexcept
{
    return QOffsetStringArray<T, N, sizeof ... (Ox)>(
               string,
               QtPrivate::makeIndexSequence<N> {},
               offsets);
}

template<int ... Nx>
struct QOffsetStringArrayRet
{
    using Offsets = QtPrivate::OffsetSequence<Nx...>;
    using Type = QOffsetStringArray<typename Offsets::Type, Offsets::Length, sizeof ... (Nx)>;
};

template<int ... Nx>
constexpr auto qOffsetStringArray(const char (&...strings)[Nx]) noexcept -> typename QOffsetStringArrayRet<Nx...>::Type
{
    using Offsets = QtPrivate::OffsetSequence<Nx...>;
    return qOffsetStringArray<typename Offsets::Type>(
            QtPrivate::staticString<Offsets::Length>(strings...), Offsets{});
}


QT_END_NAMESPACE

#endif // QOFFSETSTRINGARRAY_P_H
