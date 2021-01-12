/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QCACHE_H
#define QCACHE_H

#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE


template <class Key, class T>
class QCache
{
    struct Value
    {
        T *t = nullptr;
        qsizetype cost = 0;
        Value() noexcept = default;
        Value(T *tt, qsizetype c) noexcept
            : t(tt), cost(c)
        {}
        Value(Value &&other) noexcept
            : t(other.t),
              cost(other.cost)
        {
            other.t = nullptr;
        }
        Value &operator=(Value &&other) noexcept
        {
            qSwap(t, other.t);
            qSwap(cost, other.cost);
            return *this;
        }
        ~Value() { delete t; }

    private:
        Q_DISABLE_COPY(Value)
    };

    struct Chain
    {
        Chain() noexcept : prev(this), next(this) { }
        Chain *prev;
        Chain *next;
    };

    struct Node : public Chain
    {
        using KeyType = Key;
        using ValueType = Value;

        Key key;
        Value value;

        Node(const Key &k, Value &&t) noexcept(std::is_nothrow_move_assignable_v<Key>)
            : Chain(),
              key(k),
              value(std::move(t))
        {
        }
        Node(Key &&k, Value &&t) noexcept(std::is_nothrow_move_assignable_v<Key>)
            : Chain(),
              key(std::move(k)),
              value(std::move(t))
        {
        }
        static void createInPlace(Node *n, const Key &k, T *o, qsizetype cost)
        {
            new (n) Node{ Key(k), Value(o, cost) };
        }
        void emplace(T *o, qsizetype cost)
        {
            value = Value(o, cost);
        }
        static Node create(const Key &k, Value &&t) noexcept(std::is_nothrow_move_assignable_v<Key> && std::is_nothrow_move_assignable_v<T>)
        {
            return Node(k, std::move(t));
        }
        void replace(const Value &t) noexcept(std::is_nothrow_assignable_v<T, T>)
        {
            value = t;
        }
        void replace(Value &&t) noexcept(std::is_nothrow_move_assignable_v<T>)
        {
            value = std::move(t);
        }
        Value takeValue() noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            return std::move(value);
        }
        bool valuesEqual(const Node *other) const { return value == other->value; }

        Node(Node &&other)
            : Chain(other),
              key(std::move(other.key)),
              value(std::move(other.value))
        {
            Q_ASSERT(this->prev);
            Q_ASSERT(this->next);
            this->prev->next = this;
            this->next->prev = this;
        }
    private:
        Q_DISABLE_COPY(Node)
    };

    using Data = QHashPrivate::Data<Node>;

    mutable Chain chain;
    Data d;
    qsizetype mx = 0;
    qsizetype total = 0;

    void unlink(Node *n) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        Q_ASSERT(n->prev);
        Q_ASSERT(n->next);
        n->prev->next = n->next;
        n->next->prev = n->prev;
        total -= n->value.cost;
        auto it = d.find(n->key);
        d.erase(it);
    }
    T *relink(const Key &key) const noexcept
    {
        Node *n = d.findNode(key);
        if (!n)
            return nullptr;

        if (chain.next != n) {
            Q_ASSERT(n->prev);
            Q_ASSERT(n->next);
            n->prev->next = n->next;
            n->next->prev = n->prev;
            n->next = chain.next;
            chain.next->prev = n;
            n->prev = &chain;
            chain.next = n;
        }
        return n->value.t;
    }

    void trim(qsizetype m) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        Chain *n = chain.prev;
        while (n != &chain && total > m) {
            Node *u = static_cast<Node *>(n);
            n = n->prev;
            unlink(u);
        }
    }


    Q_DISABLE_COPY(QCache)

public:
    inline explicit QCache(qsizetype maxCost = 100) noexcept
        : mx(maxCost)
    {
    }
    inline ~QCache()
    {
        static_assert(std::is_nothrow_destructible_v<Key>, "Types with throwing destructors are not supported in Qt containers.");
        static_assert(std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

        clear();
    }

    inline qsizetype maxCost() const noexcept { return mx; }
    void setMaxCost(qsizetype m) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        mx = m;
        trim(mx);
    }
    inline qsizetype totalCost() const noexcept { return total; }

    inline qsizetype size() const noexcept { return qsizetype(d.size); }
    inline qsizetype count() const noexcept { return qsizetype(d.size); }
    inline bool isEmpty() const noexcept { return !d.size; }
    inline QList<Key> keys() const
    {
        QList<Key> k;
        if (size()) {
            k.reserve(size());
            for (auto it = d.begin(); it != d.end(); ++it)
                k << it.node()->key;
        }
        Q_ASSERT(k.size() == size());
        return k;
    }

    void clear() noexcept(std::is_nothrow_destructible_v<Node>)
    {
        d.clear();
        total = 0;
        chain.next = &chain;
        chain.prev = &chain;
    }

    bool insert(const Key &key, T *object, qsizetype cost = 1)
    {
        if (cost > mx) {
            remove(key);
            delete object;
            return false;
        }
        trim(mx - cost);
        auto result = d.findOrInsert(key);
        Node *n = result.it.node();
        if (result.initialized) {
            auto prevCost = n->value.cost;
            result.it.node()->emplace(object, cost);
            cost -= prevCost;
            relink(key);
        } else {
            Node::createInPlace(n, key, object, cost);
            n->prev = &chain;
            n->next = chain.next;
            chain.next->prev = n;
            chain.next = n;
        }
        total += cost;
        return true;
    }
    T *object(const Key &key) const noexcept
    {
        return relink(key);
    }
    T *operator[](const Key &key) const noexcept
    {
        return relink(key);
    }
    inline bool contains(const Key &key) const noexcept
    {
        return d.findNode(key) != nullptr;
    }

    bool remove(const Key &key) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        Node *n = d.findNode(key);
        if (!n) {
            return false;
        } else {
            unlink(n);
            return true;
        }
    }

    T *take(const Key &key) noexcept(std::is_nothrow_destructible_v<Key>)
    {
        Node *n = d.findNode(key);
        if (!n)
            return nullptr;

        T *t = n->value.t;
        n->value.t = nullptr;
        unlink(n);
        return t;
    }

};

QT_END_NAMESPACE

#endif // QCACHE_H
