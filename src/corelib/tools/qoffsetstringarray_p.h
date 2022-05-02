/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2021 Intel Corporation.
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

#include <QByteArrayView>

#include <array>
#include <limits>
#include <string_view>
#include <tuple>

class tst_QOffsetStringArray;

QT_BEGIN_NAMESPACE

QT_WARNING_PUSH
#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 1100
// we usually don't overread, but GCC has a false positive
QT_WARNING_DISABLE_GCC("-Wstringop-overread")
#endif


template <typename StaticString, typename OffsetList>
class QOffsetStringArray
{
public:
    constexpr QOffsetStringArray(const StaticString &string, const OffsetList &offsets)
        : m_string(string), m_offsets(offsets)
    {}

    constexpr const char *operator[](const int index) const noexcept
    {
        return m_string.data() + m_offsets[qBound(int(0), index, count())];
    }

    constexpr const char *at(const int index) const noexcept
    {
        return m_string.data() + m_offsets[index];
    }

    constexpr QByteArrayView viewAt(qsizetype index) const noexcept
    {
        return { m_string.data() + m_offsets[index],
                    qsizetype(m_offsets[index + 1]) - qsizetype(m_offsets[index]) - 1 };
    }

    constexpr int count() const { return int(m_offsets.size()) - 1; }

private:
    StaticString m_string;
    OffsetList m_offsets;
    friend tst_QOffsetStringArray;
};

namespace QtPrivate {
// std::copy is not constexpr in C++17
template <typename II, typename OO>
static constexpr OO copyData(II input, qsizetype n, OO output)
{
    using E = decltype(+*output);
    for (qsizetype i = 0; i < n; ++i)
        output[i] = E(input[i]);
    return output + n;
}

template <size_t Highest> constexpr auto minifyValue()
{
    if constexpr (Highest <= (std::numeric_limits<quint8>::max)()) {
        return quint8(Highest);
    } else if constexpr (Highest <= (std::numeric_limits<quint16>::max)()) {
        return quint16(Highest);
    } else {
        // int is probably enough for everyone
        return int(Highest);
    }
}

template <size_t StringLength, typename Extractor, typename... T>
constexpr auto makeStaticString(Extractor extract, const T &... entries)
{
    std::array<char, StringLength> result = {};
    qptrdiff offset = 0;

    const char *strings[] = { extract(entries).operator const char *()... };
    size_t lengths[] = { sizeof(extract(T{}))... };
    for (size_t i = 0; i < std::size(strings); ++i) {
        copyData(strings[i], lengths[i], result.begin() + offset);
        offset += lengths[i];
    }
    return result;
}

template <size_t N> struct StaticString
{
    char value[N] = {};
    constexpr StaticString() = default;
    constexpr StaticString(const char (&s)[N])  { copyData(s, N, value); }
    constexpr operator const char *() const     { return value; }
};

template <size_t KL, size_t VL> struct StaticMapEntry
{
    StaticString<KL> key = {};
    StaticString<VL> value = {};
    constexpr StaticMapEntry() = default;
    constexpr StaticMapEntry(const char (&k)[KL], const char (&v)[VL])
        : key(k), value(v)
    {}
};

template <typename StringExtractor, typename... T>
constexpr auto qOffsetStringArray(StringExtractor extractString, const T &... entries)
{
    constexpr size_t Count = sizeof...(T);
    constexpr qsizetype StringLength = (sizeof(extractString(T{})) + ...);
    using MinifiedOffsetType = decltype(QtPrivate::minifyValue<StringLength>());

    size_t offset = 0;
    std::array fullOffsetList = { offset += sizeof(extractString(T{}))... };

    // prepend zero
    std::array<MinifiedOffsetType, Count + 1> minifiedOffsetList = {};
    QtPrivate::copyData(fullOffsetList.begin(), Count, minifiedOffsetList.begin() + 1);

    std::array staticString = QtPrivate::makeStaticString<StringLength>(extractString, entries...);
    return QOffsetStringArray(staticString, minifiedOffsetList);
}
}

template<int ... Nx>
constexpr auto qOffsetStringArray(const char (&...strings)[Nx]) noexcept
{
    auto extractString = [](const auto &s) -> decltype(auto) { return s; };
    return QtPrivate::qOffsetStringArray(extractString, QtPrivate::StaticString(strings)...);
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif // QOFFSETSTRINGARRAY_P_H
