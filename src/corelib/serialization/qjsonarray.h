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

#ifndef QJSONARRAY_H
#define QJSONARRAY_H

#include <QtCore/qjsonvalue.h>
#include <QtCore/qiterator.h>
#include <QtCore/qshareddata.h>
#include <initializer_list>

QT_BEGIN_NAMESPACE

class QDebug;
typedef QList<QVariant> QVariantList;

class Q_CORE_EXPORT QJsonArray
{
public:
    QJsonArray();

    QJsonArray(std::initializer_list<QJsonValue> args);

    ~QJsonArray();

    QJsonArray(const QJsonArray &other);
    QJsonArray &operator =(const QJsonArray &other);

    QJsonArray(QJsonArray &&other) noexcept;

    QJsonArray &operator =(QJsonArray &&other) noexcept
    {
        swap(other);
        return *this;
    }

    static QJsonArray fromStringList(const QStringList &list);
    static QJsonArray fromVariantList(const QVariantList &list);
    QVariantList toVariantList() const;

    qsizetype size() const;
    inline qsizetype count() const { return size(); }

    bool isEmpty() const;
    QJsonValue at(qsizetype i) const;
    QJsonValue first() const;
    QJsonValue last() const;

    void prepend(const QJsonValue &value);
    void append(const QJsonValue &value);
    void removeAt(qsizetype i);
    QJsonValue takeAt(qsizetype i);
    inline void removeFirst() { removeAt(0); }
    inline void removeLast() { removeAt(size() - 1); }

    void insert(qsizetype i, const QJsonValue &value);
    void replace(qsizetype i, const QJsonValue &value);

    bool contains(const QJsonValue &element) const;
    QJsonValueRef operator[](qsizetype i);
    QJsonValue operator[](qsizetype i) const;

    bool operator==(const QJsonArray &other) const;
    bool operator!=(const QJsonArray &other) const;

    void swap(QJsonArray &other) noexcept
    {
        qSwap(a, other.a);
    }

    class const_iterator;

    class iterator {
    public:
        typedef std::random_access_iterator_tag  iterator_category;
        typedef qsizetype difference_type;
        typedef QJsonValue value_type;
        typedef QJsonValueRef reference;
        typedef QJsonValueRef *pointer;

        inline iterator() : item(static_cast<QJsonArray *>(nullptr), 0) { }
        explicit inline iterator(QJsonArray *array, qsizetype index) : item(array, index) { }

        constexpr iterator(const iterator &other) = default;
        iterator &operator=(const iterator &other)
        {
            item.a = other.item.a;
            item.index = other.item.index;
            return *this;
        }

        inline QJsonValueRef operator*() const { return item; }
        inline QJsonValueRef *operator->() const { return &item; }
        inline QJsonValueRef operator[](qsizetype j) const
        { return { item.a, qsizetype(item.index) + j }; }

        inline bool operator==(const iterator &o) const
        { return item.a == o.item.a && item.index == o.item.index; }
        inline bool operator!=(const iterator &o) const { return !(*this == o); }
        inline bool operator<(const iterator &other) const
        { Q_ASSERT(item.a == other.item.a); return item.index < other.item.index; }
        inline bool operator<=(const iterator &other) const
        { Q_ASSERT(item.a == other.item.a); return item.index <= other.item.index; }
        inline bool operator>(const iterator &other) const { return !(*this <= other); }
        inline bool operator>=(const iterator &other) const { return !(*this < other); }
        inline bool operator==(const const_iterator &o) const
        { return item.a == o.item.a && item.index == o.item.index; }
        inline bool operator!=(const const_iterator &o) const { return !(*this == o); }
        inline bool operator<(const const_iterator &other) const
        { Q_ASSERT(item.a == other.item.a); return item.index < other.item.index; }
        inline bool operator<=(const const_iterator &other) const
        { Q_ASSERT(item.a == other.item.a); return item.index <= other.item.index; }
        inline bool operator>(const const_iterator &other) const { return !(*this <= other); }
        inline bool operator>=(const const_iterator &other) const { return !(*this < other); }
        inline iterator &operator++() { ++item.index; return *this; }
        inline iterator operator++(int) { iterator n = *this; ++item.index; return n; }
        inline iterator &operator--() { item.index--; return *this; }
        inline iterator operator--(int) { iterator n = *this; item.index--; return n; }
        inline iterator &operator+=(qsizetype j) { item.index += quint64(j); return *this; }
        inline iterator &operator-=(qsizetype j) { item.index -= quint64(j); return *this; }
        inline iterator operator+(qsizetype j) const
        { return iterator(item.a, qsizetype(item.index) + j); }
        inline iterator operator-(qsizetype j) const
        { return iterator(item.a, qsizetype(item.index) - j); }
        inline qsizetype operator-(iterator j) const { return item.index - j.item.index; }

    private:
        mutable QJsonValueRef item;
        friend class QJsonArray;
    };
    friend class iterator;

    class const_iterator {
    public:
        typedef std::random_access_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef QJsonValue value_type;
        typedef const QJsonValueRef reference;
        typedef const QJsonValueRef *pointer;

        inline const_iterator() : item(static_cast<QJsonArray *>(nullptr), 0) { }
        explicit inline const_iterator(const QJsonArray *array, qsizetype index)
            : item(const_cast<QJsonArray *>(array), index) { }
        inline const_iterator(const iterator &o) : item(o.item) { }

        constexpr const_iterator(const const_iterator &other) = default;
        const_iterator &operator=(const const_iterator &other)
        {
            item.a = other.item.a;
            item.index = other.item.index;
            return *this;
        }

        inline const QJsonValueRef operator*() const { return item; }
        inline const QJsonValueRef *operator->() const { return &item; }

        inline QJsonValueRef operator[](qsizetype j) const
        { return { item.a, qsizetype(item.index) + j }; }
        inline bool operator==(const const_iterator &o) const
        { return item.a == o.item.a && item.index == o.item.index; }
        inline bool operator!=(const const_iterator &o) const { return !(*this == o); }
        inline bool operator<(const const_iterator &other) const
        { Q_ASSERT(item.a == other.item.a); return item.index < other.item.index; }
        inline bool operator<=(const const_iterator &other) const
        { Q_ASSERT(item.a == other.item.a); return item.index <= other.item.index; }
        inline bool operator>(const const_iterator &other) const { return !(*this <= other); }
        inline bool operator>=(const const_iterator &other) const { return !(*this < other); }
        inline const_iterator &operator++() { ++item.index; return *this; }
        inline const_iterator operator++(int) { const_iterator n = *this; ++item.index; return n; }
        inline const_iterator &operator--() { item.index--; return *this; }
        inline const_iterator operator--(int) { const_iterator n = *this; item.index--; return n; }
        inline const_iterator &operator+=(qsizetype j) { item.index += quint64(j); return *this; }
        inline const_iterator &operator-=(qsizetype j) { item.index -= quint64(j); return *this; }
        inline const_iterator operator+(qsizetype j) const
        { return const_iterator(item.a, qsizetype(item.index) + j); }
        inline const_iterator operator-(qsizetype j) const
        { return const_iterator(item.a, qsizetype(item.index) - j); }
        inline qsizetype operator-(const_iterator j) const { return item.index - j.item.index; }

    private:
        QJsonValueRef item;
        friend class QJsonArray;
    };
    friend class const_iterator;

    // stl style
    inline iterator begin() { detach(); return iterator(this, 0); }
    inline const_iterator begin() const { return const_iterator(this, 0); }
    inline const_iterator constBegin() const { return const_iterator(this, 0); }
    inline const_iterator cbegin() const { return const_iterator(this, 0); }
    inline iterator end() { detach(); return iterator(this, size()); }
    inline const_iterator end() const { return const_iterator(this, size()); }
    inline const_iterator constEnd() const { return const_iterator(this, size()); }
    inline const_iterator cend() const { return const_iterator(this, size()); }
    iterator insert(iterator before, const QJsonValue &value)
    { insert(before.item.index, value); return before; }
    iterator erase(iterator it)
    { removeAt(it.item.index); return it; }

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;

    // convenience
    inline QJsonArray operator+(const QJsonValue &v) const
    { QJsonArray n = *this; n += v; return n; }
    inline QJsonArray &operator+=(const QJsonValue &v)
    { append(v); return *this; }
    inline QJsonArray &operator<< (const QJsonValue &v)
    { append(v); return *this; }

    // stl compatibility
    inline void push_back(const QJsonValue &t) { append(t); }
    inline void push_front(const QJsonValue &t) { prepend(t); }
    inline void pop_front() { removeFirst(); }
    inline void pop_back() { removeLast(); }
    inline bool empty() const { return isEmpty(); }
    typedef qsizetype size_type;
    typedef QJsonValue value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef QJsonValueRef reference;
    typedef QJsonValue const_reference;
    typedef qsizetype difference_type;

private:
    friend class QJsonValue;
    friend class QJsonDocument;
    friend class QCborArray;
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonArray &);

    QJsonArray(QCborContainerPrivate *array);
    bool detach(qsizetype reserve = 0);

    QExplicitlySharedDataPointer<QCborContainerPrivate> a;
};

Q_DECLARE_SHARED(QJsonArray)

Q_CORE_EXPORT size_t qHash(const QJsonArray &array, size_t seed = 0);

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonArray &);
#endif

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QJsonArray &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QJsonArray &);
#endif

QT_END_NAMESPACE

#endif // QJSONARRAY_H
