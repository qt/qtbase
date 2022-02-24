/****************************************************************************
**
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

#include <QtCore/qglobal.h>

#ifndef QGLOBALSTATIC_H
#define QGLOBALSTATIC_H

#include <QtCore/qatomic.h>

#include <atomic>           // for bootstrapped (no thread) builds
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtGlobalStatic {
enum GuardValues {
    Destroyed = -2,
    Initialized = -1,
    Uninitialized = 0,
    Initializing = 1
};

template <typename QGS> union Holder
{
    using Type = typename QGS::QGS_Type;
    using PlainType = std::remove_cv_t<Type>;

    static constexpr bool ConstructionIsNoexcept = noexcept(QGS::innerFunction(nullptr));
    static inline QBasicAtomicInteger<qint8> guard = { QtGlobalStatic::Uninitialized };

    // union's sole member
    PlainType storage;

    Holder() noexcept(ConstructionIsNoexcept)
    {
        QGS::innerFunction(pointer());
        guard.storeRelaxed(QtGlobalStatic::Initialized);
    }

    ~Holder()
    {
        pointer()->~PlainType();
        std::atomic_thread_fence(std::memory_order_acquire); // avoid mixing stores to guard and *pointer()
        guard.storeRelaxed(QtGlobalStatic::Destroyed);
    }

    PlainType *pointer() noexcept
    {
        return &storage;
    }

    Q_DISABLE_COPY_MOVE(Holder)
};
}

template <typename Holder> struct QGlobalStatic
{
    using Type = typename Holder::Type;

    bool isDestroyed() const { return guardValue() <= QtGlobalStatic::Destroyed; }
    bool exists() const { return guardValue() == QtGlobalStatic::Initialized; }
    operator Type *()
    {
        if (isDestroyed())
            return nullptr;
        return instance();
    }
    Type *operator()()
    {
        if (isDestroyed())
            return nullptr;
        return instance();
    }
    Type *operator->()
    {
        Q_ASSERT_X(!isDestroyed(), "Q_GLOBAL_STATIC",
                   "The global static was used after being destroyed");
        return instance();
    }
    Type &operator*()
    {
        Q_ASSERT_X(!isDestroyed(), "Q_GLOBAL_STATIC",
                   "The global static was used after being destroyed");
        return *instance();
    }

protected:
    static Type *instance() noexcept(Holder::ConstructionIsNoexcept)
    {
        static Holder holder;
        return holder.pointer();
    }
    static QtGlobalStatic::GuardValues guardValue()
    {
        return QtGlobalStatic::GuardValues(Holder::guard.loadAcquire());
    }
};

#define Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                         \
    QT_WARNING_PUSH                                                         \
    QT_WARNING_DISABLE_CLANG("-Wunevaluated-expression")                    \
    namespace { struct Q_QGS_ ## NAME {                                     \
        typedef TYPE QGS_Type;                                              \
        static void innerFunction(void *pointer)                            \
            noexcept(noexcept(std::remove_cv_t<QGS_Type> ARGS))             \
        {                                                                   \
            new (pointer) QGS_Type ARGS;                                    \
        }                                                                   \
    }; }                                                                    \
    static QGlobalStatic<QtGlobalStatic::Holder<Q_QGS_ ## NAME>> NAME;      \
    QT_WARNING_POP
    /**/

#define Q_GLOBAL_STATIC(TYPE, NAME, ...)                                    \
    Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, (__VA_ARGS__))

QT_END_NAMESPACE
#endif // QGLOBALSTATIC_H
