// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q20UTILITY_H
#define Q20UTILITY_H

#include <QtCore/qtconfigmacros.h>

#include <utility>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

// like C++20 std::exchange (ie. constexpr, not yet noexcept)
namespace q20 {
#ifdef __cpp_lib_constexpr_algorithms
using std::exchange;
#else
template <typename T, typename U = T>
constexpr T exchange(T& obj, U&& newValue)
{
    T old = std::move(obj);
    obj = std::forward<U>(newValue);
    return old;
}
#endif
}

QT_END_NAMESPACE

#endif /* Q20UTILITY_H */
