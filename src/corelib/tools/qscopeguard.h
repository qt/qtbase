/****************************************************************************
**
** Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sérgio Martins <sergio.martins@kdab.com>
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSCOPEGUARD_H
#define QSCOPEGUARD_H

#include <QtCore/qglobal.h>

#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

template <typename F>
class
#if __has_cpp_attribute(nodiscard)
// Q_REQUIRED_RESULT can be defined as __warn_unused_result__ or as [[nodiscard]]
// but the 1st one has some limitations for example can be placed only on functions.
Q_REQUIRED_RESULT
#endif
QScopeGuard
{
public:
    explicit QScopeGuard(F &&f) noexcept
        : m_func(std::move(f))
    {
    }

    explicit QScopeGuard(const F &f) noexcept
        : m_func(f)
    {
    }

    QScopeGuard(QScopeGuard &&other) noexcept
        : m_func(std::move(other.m_func))
        , m_invoke(qExchange(other.m_invoke, false))
    {
    }

    ~QScopeGuard() noexcept
    {
        if (m_invoke)
            m_func();
    }

    void dismiss() noexcept
    {
        m_invoke = false;
    }

private:
    Q_DISABLE_COPY(QScopeGuard)

    F m_func;
    bool m_invoke = true;
};

#ifdef __cpp_deduction_guides
template <typename F> QScopeGuard(F(&)()) -> QScopeGuard<F(*)()>;
#endif

//! [qScopeGuard]
template <typename F>
#if __has_cpp_attribute(nodiscard)
Q_REQUIRED_RESULT
#endif
QScopeGuard<typename std::decay<F>::type> qScopeGuard(F &&f)
{
    return QScopeGuard<typename std::decay<F>::type>(std::forward<F>(f));
}

QT_END_NAMESPACE

#endif // QSCOPEGUARD_H
