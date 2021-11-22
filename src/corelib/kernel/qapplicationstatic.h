/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <QtCore/qglobalstatic.h>

QT_BEGIN_NAMESPACE

#define Q_APPLICATION_STATIC_INTERNAL_HOLDER(ARGS)                          \
    Q_GLOBAL_STATIC_INTERNAL_HOLDER(ARGS)                                   \
    struct LifecycleHolder : public Holder                                  \
    {                                                                       \
        LifecycleHolder() { connectLifecycle(); }                           \
        BaseType *get()                                                     \
        {                                                                   \
            const QMutexLocker locker(&theMutex);                           \
            if (value.get() == nullptr                                      \
                && guard.loadRelaxed() == QtGlobalStatic::Initialized) {    \
                value.reset(ARGS);                                          \
                connectLifecycle();                                         \
            }                                                               \
            return value.get();                                             \
        }                                                                   \
        void connectLifecycle()                                             \
        {                                                                   \
            Q_ASSERT(QCoreApplication::instance());                         \
            QObject::connect(QCoreApplication::instance(),                  \
                &QCoreApplication::destroyed, [this] {                      \
                const QMutexLocker locker(&theMutex);                       \
                value.reset(nullptr);                                       \
            });                                                             \
        }                                                                   \
    };

#define Q_APPLICATION_STATIC_INTERNAL(ARGS)                                 \
    Q_GLOBAL_STATIC_INTERNAL_DECORATION BaseType *innerFunction()           \
    {                                                                       \
        Q_APPLICATION_STATIC_INTERNAL_HOLDER(ARGS)                          \
        static LifecycleHolder holder;                                      \
        return holder.get();                                                \
    }

#define Q_APPLICATION_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                    \
    QT_WARNING_PUSH                                                         \
    QT_WARNING_DISABLE_CLANG("-Wunevaluated-expression")                    \
    namespace { namespace Q_QAS_ ## NAME {                                  \
        typedef TYPE BaseType;                                              \
        typedef std::unique_ptr<BaseType> Type;                             \
        QBasicAtomicInt guard = Q_BASIC_ATOMIC_INITIALIZER(QtGlobalStatic::Uninitialized); \
        QBasicMutex theMutex;                                               \
        Q_APPLICATION_STATIC_INTERNAL((new BaseType ARGS))                  \
    } }                                                                     \
    static QGlobalStatic<TYPE,                                              \
                         Q_QAS_ ## NAME::innerFunction,                     \
                         Q_QAS_ ## NAME::guard> NAME;                       \
    QT_WARNING_POP

#define Q_APPLICATION_STATIC(TYPE, NAME) \
    Q_APPLICATION_STATIC_WITH_ARGS(TYPE, NAME, ())

QT_END_NAMESPACE

#endif // QAPPLICATIONSTATIC_H
