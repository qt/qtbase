// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTMOCHELPERS_H
#define QTMOCHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists to be used by the code that
// moc generates. This file will not change quickly, but it over the long term,
// it will likely change or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#include <algorithm> // std::min
#include <limits>

#if 0
#pragma qt_no_master_include
#endif

QT_BEGIN_NAMESPACE
namespace QtMocHelpers {
// The maximum Size of a string literal is 2 GB on 32-bit and 4 GB on 64-bit
// (but the compiler is likely to give up before you get anywhere near that much)
static constexpr size_t MaxStringSize =
        (std::min)(size_t((std::numeric_limits<uint>::max)()),
                   size_t((std::numeric_limits<qsizetype>::max)()));

template <uint... Nx> constexpr size_t stringDataSizeHelper(std::integer_sequence<uint, Nx...>)
{
    // same as:
    //   return (0 + ... + Nx);
    // but not using the fold expression to avoid exceeding compiler limits
    size_t total = 0;
    uint sizes[] = { Nx... };
    for (uint n : sizes)
        total += n;
    return total;
}

template <int Count, size_t StringSize> struct StringData
{
    static_assert(StringSize <= MaxStringSize, "Meta Object data is too big");
    uint offsetsAndSizes[Count] = {};
    char stringdata0[StringSize] = {};
    constexpr StringData() = default;
};

template <uint... Nx> constexpr auto stringData(const char (&...strings)[Nx])
{
    constexpr size_t StringSize = stringDataSizeHelper<Nx...>({});
    constexpr size_t Count = 2 * sizeof...(Nx);

    StringData<Count, StringSize> result;
    const char *inputs[] = { strings... };
    uint sizes[] = { Nx... };

    uint offset = 0;
    char *output = result.stringdata0;
    for (size_t i = 0; i < sizeof...(Nx); ++i) {
        // copy the input string, including the terminating null
        uint len = sizes[i];
        for (uint j = 0; j < len; ++j)
            output[offset + j] = inputs[i][j];
        result.offsetsAndSizes[2 * i] = offset + sizeof(result.offsetsAndSizes);
        result.offsetsAndSizes[2 * i + 1] = len - 1;
        offset += len;
    }

    return result;
}

#  define QT_MOC_HAS_STRINGDATA       1

} // namespace QtMocHelpers
QT_END_NAMESPACE

QT_USE_NAMESPACE

#endif // QTMOCHELPERS_H
