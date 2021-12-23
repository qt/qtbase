/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2019 Intel Corporation.
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

#ifndef QFOREACH_H
#define QFOREACH_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if 0
#pragma qt_class(QForeach)
#pragma qt_sync_stop_processing
#endif

#ifndef QT_NO_FOREACH

namespace QtPrivate {

template <typename T>
class QForeachContainer {
    Q_DISABLE_COPY(QForeachContainer)
public:
    QForeachContainer(const T &t) : c(t), i(qAsConst(c).begin()), e(qAsConst(c).end()) {}
    QForeachContainer(T &&t) : c(std::move(t)), i(qAsConst(c).begin()), e(qAsConst(c).end())  {}

    QForeachContainer(QForeachContainer &&other)
        : c(std::move(other.c)),
          i(qAsConst(c).begin()),
          e(qAsConst(c).end()),
          control(std::move(other.control))
    {
    }

    QForeachContainer &operator=(QForeachContainer &&other)
    {
        c = std::move(other.c);
        i = qAsConst(c).begin();
        e = qAsConst(c).end();
        control = std::move(other.control);
        return *this;
    }

    T c;
    typename T::const_iterator i, e;
    int control = 1;
};

// Containers that have a detach function are considered shared, and are OK in a foreach loop
template <typename T, typename = decltype(std::declval<T>().detach())>
inline void warnIfContainerIsNotShared(int) {}

#if QT_DEPRECATED_SINCE(6, 0)
// Other containers will copy themselves if used in foreach, this use is deprecated
template <typename T>
QT_DEPRECATED_VERSION_X_6_0("Do not use foreach/Q_FOREACH with containers which are not implicitly shared. "
    "Prefer using a range-based for loop with these containers: `for (const auto &it : container)`, "
    "keeping in mind that range-based for doesn't copy the container as Q_FOREACH does")
inline void warnIfContainerIsNotShared(...) {}
#endif

template<typename T>
QForeachContainer<typename std::decay<T>::type> qMakeForeachContainer(T &&t)
{
    warnIfContainerIsNotShared<typename std::decay<T>::type>(0);
    return QForeachContainer<typename std::decay<T>::type>(std::forward<T>(t));
}

}

// Use C++17 if statement with initializer. User's code ends up in a else so
// scoping of different ifs is not broken
#define Q_FOREACH_IMPL(variable, name, container)                                             \
    for (auto name = QtPrivate::qMakeForeachContainer(container); name.i != name.e; ++name.i) \
        if (variable = *name.i; false) {} else

#define Q_FOREACH_JOIN(A, B) Q_FOREACH_JOIN_IMPL(A, B)
#define Q_FOREACH_JOIN_IMPL(A, B) A ## B

#define Q_FOREACH(variable, container) \
    Q_FOREACH_IMPL(variable, Q_FOREACH_JOIN(_container_, __LINE__), container)
#endif // QT_NO_FOREACH

#define Q_FOREVER for(;;)
#ifndef QT_NO_KEYWORDS
# ifndef QT_NO_FOREACH
#  ifndef foreach
#    define foreach Q_FOREACH
#  endif
# endif // QT_NO_FOREACH
#  ifndef forever
#    define forever Q_FOREVER
#  endif
#endif

QT_END_NAMESPACE

#endif /* QFOREACH_H */
