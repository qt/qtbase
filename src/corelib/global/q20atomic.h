// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef Q20ATOMIC_H
#define Q20ATOMIC_H

#include <QtCore/qtconfigmacros.h>

#include <atomic>

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

namespace q20 {

// This only ensures fixed initialization, nothing else (e.g. no wait/notify):
#ifdef __cpp_lib_atomic_value_initialization
template <class T>
using atomic = std::atomic<T>;
#else
template <class T>
class atomic : public std::atomic<T>
{
public:
    using std::atomic<T>::atomic;
    constexpr atomic() noexcept(noexcept(std::atomic<T>(T()))) : std::atomic<T>(T()) { }
};

template <class T>
atomic(T) -> atomic<T>;
#endif

} // namespace q20

QT_END_NAMESPACE

#endif /* Q20ATOMIC_H */
