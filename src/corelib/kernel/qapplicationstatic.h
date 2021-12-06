/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QAPPLICATIONSTATIC_H
#define QAPPLICATIONSTATIC_H

#include <QtCore/QMutex>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qglobalstatic.h>

QT_BEGIN_NAMESPACE

namespace QtGlobalStatic {
template <typename QAS> struct ApplicationHolder
{
    using Type = typename QAS::QAS_Type;
    using PlainType = std::remove_cv_t<Type>;

    static inline std::aligned_union_t<1, PlainType> storage;
    static inline QBasicAtomicInteger<qint8> guard = { QtGlobalStatic::Uninitialized };
    static inline QBasicMutex mutex {};

    static constexpr bool MutexLockIsNoexcept = noexcept(mutex.lock());
    static constexpr bool ConstructionIsNoexcept = noexcept(QAS::innerFunction(nullptr));

    ApplicationHolder() = default;
    Q_DISABLE_COPY_MOVE(ApplicationHolder)
    ~ApplicationHolder()
    {
        if (guard.loadRelaxed() == QtGlobalStatic::Initialized) {
            guard.storeRelease(QtGlobalStatic::Destroyed);
            realPointer()->~PlainType();
        }
    }

    static PlainType *realPointer()
    {
        return reinterpret_cast<PlainType *>(&storage);
    }

    // called from QGlobalStatic::instance()
    PlainType *pointer() noexcept(MutexLockIsNoexcept && ConstructionIsNoexcept)
    {
        if (guard.loadRelaxed() == QtGlobalStatic::Initialized)
            return realPointer();
        QMutexLocker locker(&mutex);
        if (guard.loadRelaxed() == QtGlobalStatic::Uninitialized) {
            QAS::innerFunction(realPointer());
            QObject::connect(QCoreApplication::instance(), &QObject::destroyed, reset);
            guard.storeRelaxed(QtGlobalStatic::Initialized);
        }
        return realPointer();
    }

    static void reset()
    {
        if (guard.loadRelaxed() == QtGlobalStatic::Initialized) {
            QMutexLocker locker(&mutex);
            realPointer()->~PlainType();
            guard.storeRelaxed(QtGlobalStatic::Uninitialized);
        }
    }
};
} // namespace QtGlobalStatic

#define Q_APPLICATION_STATIC(TYPE, NAME, ...) \
    namespace { struct Q_QAS_ ## NAME {                                     \
        typedef TYPE QAS_Type;                                              \
        static void innerFunction(void *pointer)                            \
            noexcept(noexcept(std::remove_cv_t<QAS_Type>(__VA_ARGS__)))     \
        {                                                                   \
            new (pointer) QAS_Type(__VA_ARGS__);                            \
        }                                                                   \
    }; }                                                                    \
    static QGlobalStatic<QtGlobalStatic::ApplicationHolder<Q_QAS_ ## NAME>> NAME;\
    /**/

QT_END_NAMESPACE

#endif // QAPPLICATIONSTATIC_H
