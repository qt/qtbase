/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt.io/licensing/
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
#ifndef QDUPLICATETRACKER_P_H
#define QDUPLICATETRACKER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>

#if __has_include(<memory_resource>)
#  include <unordered_set>
#  include <memory_resource>
#  include <qhash.h> // for the hashing helpers
#else
#  include <qset.h>
#endif

QT_BEGIN_NAMESPACE

template <typename T, size_t Prealloc = 32>
class QDuplicateTracker {
#ifdef __cpp_lib_memory_resource
    template <typename HT>
    struct QHasher {
        size_t storedSeed = QHashSeed::globalSeed();
        size_t operator()(const HT &t) const {
            return QHashPrivate::calculateHash(t, storedSeed);
        }
    };

    struct node_guesstimate { void *next; size_t hash; T value; };
    static constexpr size_t bufferSize(size_t N) {
        return N * sizeof(void*) // bucket list
                + N * sizeof(node_guesstimate); // nodes
    }

    char buffer[bufferSize(Prealloc)];
    std::pmr::monotonic_buffer_resource res{buffer, sizeof buffer};
    std::pmr::unordered_set<T, QHasher<T>> set{Prealloc, &res};
#else
    class Set : public QSet<T> {
        qsizetype setSize = 0;
    public:
        explicit Set(qsizetype n) : QSet<T>{}
        { this->reserve(n); }

        auto insert(const T &e) {
            auto it = QSet<T>::insert(e);
            const auto n = this->size();
            return std::pair{it, qExchange(setSize, n) != n};
        }

        auto insert(T &&e) {
            auto it = QSet<T>::insert(std::move(e));
            const auto n = this->size();
            return std::pair{it, qExchange(setSize, n) != n};
        }
    };
    Set set{Prealloc};
#endif
    Q_DISABLE_COPY_MOVE(QDuplicateTracker);
public:
    static constexpr inline bool uses_pmr =
        #ifdef __cpp_lib_memory_resource
            true
        #else
            false
        #endif
            ;
    QDuplicateTracker() = default;
    explicit QDuplicateTracker(qsizetype n)
#ifdef __cpp_lib_memory_resource
        : set{size_t(n), &res}
#else
        : set{n}
#endif
    {}
    Q_DECL_DEPRECATED_X("Pass the capacity to reserve() to the ctor instead.")
    void reserve(qsizetype n) { set.reserve(n); }
    [[nodiscard]] bool hasSeen(const T &s)
    {
        return !set.insert(s).second;
    }
    [[nodiscard]] bool hasSeen(T &&s)
    {
        return !set.insert(std::move(s)).second;
    }

    template <typename C>
    void appendTo(C &c) const &
    {
        for (const auto &e : set)
            c.push_back(e);
    }

    template <typename C>
    void appendTo(C &c) &&
    {
        if constexpr (uses_pmr) {
            while (!set.empty())
                c.push_back(std::move(set.extract(set.begin()).value()));
        } else {
            return appendTo(c); // lvalue version
        }
    }

    void clear()
    {
        set.clear();
    }
};

QT_END_NAMESPACE

#endif /* QDUPLICATETRACKER_P_H */
