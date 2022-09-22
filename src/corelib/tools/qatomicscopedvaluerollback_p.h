// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QATOMICSCOPEDVALUEROLLBACK_P_H
#define QATOMICSCOPEDVALUEROLLBACK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>

#include <atomic>

QT_BEGIN_NAMESPACE

template <typename T>
class [[nodiscard]] QAtomicScopedValueRollback
{
    std::atomic<T> &m_atomic;
    T m_value;
    std::memory_order m_mo;

    Q_DISABLE_COPY_MOVE(QAtomicScopedValueRollback)

    constexpr std::memory_order store_part(std::memory_order mo) noexcept
    {
        switch (mo) {
        case std::memory_order_relaxed:
        case std::memory_order_consume:
        case std::memory_order_acquire: return std::memory_order_relaxed;
        case std::memory_order_release:
        case std::memory_order_acq_rel: return std::memory_order_release;
        case std::memory_order_seq_cst: return std::memory_order_seq_cst;
        }
        // GCC 8.x does not tread __builtin_unreachable() as constexpr
#if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
        // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
        Q_UNREACHABLE();
#endif
        return std::memory_order_seq_cst;
    }
public:
    //
    // std::atomic:
    //
    explicit constexpr
    QAtomicScopedValueRollback(std::atomic<T> &var,
                               std::memory_order mo = std::memory_order_seq_cst)
        : m_atomic(var), m_value(var.load(mo)), m_mo(mo) {}

    explicit constexpr
    QAtomicScopedValueRollback(std::atomic<T> &var, T value,
                               std::memory_order mo = std::memory_order_seq_cst)
        : m_atomic(var), m_value(var.exchange(value, mo)), m_mo(mo) {}

    //
    // Q(Basic)AtomicInteger:
    //
    explicit constexpr
    QAtomicScopedValueRollback(QBasicAtomicInteger<T> &var,
                               std::memory_order mo = std::memory_order_seq_cst)
        : QAtomicScopedValueRollback(var._q_value, mo) {}

    explicit constexpr
    QAtomicScopedValueRollback(QBasicAtomicInteger<T> &var, T value,
                               std::memory_order mo = std::memory_order_seq_cst)
        : QAtomicScopedValueRollback(var._q_value, value, mo) {}

    //
    // Q(Basic)AtomicPointer:
    //
    explicit constexpr
    QAtomicScopedValueRollback(QBasicAtomicPointer<std::remove_pointer_t<T>> &var,
                               std::memory_order mo = std::memory_order_seq_cst)
        : QAtomicScopedValueRollback(var._q_value, mo) {}

    explicit constexpr
    QAtomicScopedValueRollback(QBasicAtomicPointer<std::remove_pointer_t<T>> &var, T value,
                               std::memory_order mo = std::memory_order_seq_cst)
        : QAtomicScopedValueRollback(var._q_value, value, mo) {}

#if __cpp_constexpr >= 201907L
    constexpr
#endif
    ~QAtomicScopedValueRollback()
    {
        m_atomic.store(m_value, store_part(m_mo));
    }

    constexpr void commit()
    {
        m_value = m_atomic.load(m_mo);
    }
};

QT_END_NAMESPACE

#endif // QATOMICASCOPEDVALUEROLLBACK_P_H
