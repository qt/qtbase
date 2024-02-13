// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTCORE_QMINIMALFLATSET_P_H
#define QTCORE_QMINIMALFLATSET_P_H

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

#include <QtCore/qcontainerfwd.h>
#include <QtCore/private/qglobal_p.h>

//#define QMINIMAL_FLAT_SET_DEBUG
#ifdef QMINIMAL_FLAT_SET_DEBUG
# include <QtCore/qscopeguard.h>
# include <QtCore/qdebug.h>
# define QMINIMAL_FLAT_SET_PRINT_AT_END \
    const auto sg = qScopeGuard([&] { qDebug() << this << *this; });
#else
# define QMINIMAL_FLAT_SET_PRINT_AT_END
#endif

#include <algorithm> // for std::lower_bound

QT_BEGIN_NAMESPACE

/*
    This is a minimal version of a QFlatSet, the std::set version of QFlatMap.
    Like QFlatMap, it has linear insertion and removal, not logarithmic, like
    real QMap and std::set, so it's only a good container if you either have
    very few entries or lots, but with separate setup and lookup stages.
    Because a full QFlatSet would be 10x the work on writing this minimal one,
    we keep it here for now. When more users pop up and the class has matured a
    bit, we can consider moving it as QFlatSet alongside QFlatMap.
*/

template <typename T, typename Container = QList<T>>
class QMinimalFlatSet
{
    Container c;
public:
    // compiler-generated default ctor is ok!
    // compiler-generated copy/move ctor/assignment operators are ok!
    // compiler-generated dtor is ok!

    using const_iterator = typename Container::const_iterator;
    using iterator = const_iterator;
    using const_reverse_iterator = typename Container::const_reverse_iterator;
    using reverse_iterator = const_reverse_iterator;
    using value_type = T;

    iterator begin() const { return c.cbegin(); }
    iterator end() const { return c.cend(); }
    iterator cbegin() const { return begin(); }
    iterator cend() const { return cend(); }

    reverse_iterator rbegin() const { return c.crbegin(); }
    reverse_iterator rend() const { return c.crend(); }
    reverse_iterator crbegin() const { return rbegin(); }
    reverse_iterator crend() const { return rend(); }

    void clear() {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        c.clear();
    }
    auto size() const { return c.size(); }
    auto count() const { return size(); }
    bool isEmpty() const { return size() == 0; }

    std::pair<iterator, bool> insert(value_type &&v)
    {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        const auto r = lookup(v);
        if (r.exists)
            return {r.it, false};
        else
            return {c.insert(r.it, std::move(v)), true};
    }

    std::pair<iterator, bool> insert(const value_type &v)
    {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        const auto r = lookup(v);
        if (r.exists)
            return {r.it, false};
        else
            return {c.insert(r.it, v), true};
    }

    void erase(const value_type &v)
    {
        QMINIMAL_FLAT_SET_PRINT_AT_END
        const auto r = lookup(v);
        if (r.exists)
            c.erase(r.it);
    }
    void remove(const value_type &v) { erase(v); }

    bool contains(const value_type &v) const
    {
        return lookup(v).exists;
    }

    const Container &values() const & { return c; }
    Container values() && { return std::move(c); }

private:
    auto lookup(const value_type &v) const
    {
        struct R {
            iterator it;
            bool exists;
        };

        const auto it = std::lower_bound(c.cbegin(), c.cend(), v);
        return R{it, it != c.cend() && !(v < *it)};
    }

#ifdef QMINIMAL_FLAT_SET_DEBUG
    friend QDebug operator<<(QDebug dbg, const QMinimalFlatSet &set)
    {
        const QDebugStateSaver saver(dbg);
        dbg.nospace() << "QMinimalFlatSet{";
        for (auto &e : set)
            dbg << e << ", ";
        return dbg << "}";
    }
#endif
};

#undef QMINIMAL_FLAT_SET_PRINT_AT_END

template <typename T, qsizetype N = QVarLengthArrayDefaultPrealloc>
using QMinimalVarLengthFlatSet = QMinimalFlatSet<T, QVarLengthArray<T, N>>;

QT_END_NAMESPACE

#endif // QTCORE_QMINIMALFLATSET_P_H
