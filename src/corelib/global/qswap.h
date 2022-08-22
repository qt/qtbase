// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSWAP_H
#define QSWAP_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qcompilerdetection.h>

#if 0
#pragma qt_class(QtSwap)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

QT_WARNING_PUSH
// warning: noexcept-expression evaluates to 'false' because of a call to 'void swap(..., ...)'
QT_WARNING_DISABLE_GCC("-Wnoexcept")

namespace QtPrivate
{
namespace SwapExceptionTester { // insulate users from the "using std::swap" below
    using std::swap; // import std::swap
    template <typename T>
    void checkSwap(T &t)
        noexcept(noexcept(swap(t, t)));
    // declared, but not implemented (only to be used in unevaluated contexts (noexcept operator))
}
} // namespace QtPrivate

// Documented in ../tools/qalgorithm.qdoc
template <typename T>
constexpr void qSwap(T &value1, T &value2)
    noexcept(noexcept(QtPrivate::SwapExceptionTester::checkSwap(value1)))
{
    using std::swap;
    swap(value1, value2);
}

// pure compile-time micro-optimization for our own headers, so not documented:
template <typename T>
constexpr inline void qt_ptr_swap(T* &lhs, T* &rhs) noexcept
{
    T *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
}

QT_WARNING_POP

QT_END_NAMESPACE

#endif // QSWAP_H
