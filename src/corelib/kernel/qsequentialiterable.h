/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QSEQUENTIALITERABLE_H
#define QSEQUENTIALITERABLE_H

#include <QtCore/qiterable.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QSequentialIterator : public QIterator<QMetaSequence>
{
public:
    using value_type = QVariant;
    using reference = QVariantRef<QSequentialIterator>;
    using pointer = QVariantPointer<QSequentialIterator>;

    QSequentialIterator(QIterator &&it)
        : QIterator(std::move(it))
    {}

    QVariantRef<QSequentialIterator> operator*() const;
    QVariantPointer<QSequentialIterator> operator->() const;
};

class Q_CORE_EXPORT QSequentialConstIterator : public QConstIterator<QMetaSequence>
{
public:
    using value_type = QVariant;
    using reference = const QVariant &;
    using pointer = QVariantConstPointer;

    QSequentialConstIterator(QConstIterator &&it)
        : QConstIterator(std::move(it))
    {}

    QVariant operator*() const;
    QVariantConstPointer operator->() const;
};

class Q_CORE_EXPORT QSequentialIterable : public QIterable<QMetaSequence>
{
public:
    using iterator = QTaggedIterator<QSequentialIterator, void>;
    using const_iterator = QTaggedIterator<QSequentialConstIterator, void>;

    using RandomAccessIterator = QTaggedIterator<iterator, std::random_access_iterator_tag>;
    using BidirectionalIterator = QTaggedIterator<iterator, std::bidirectional_iterator_tag>;
    using ForwardIterator = QTaggedIterator<iterator, std::forward_iterator_tag>;
    using InputIterator = QTaggedIterator<iterator, std::input_iterator_tag>;

    using RandomAccessConstIterator = QTaggedIterator<const_iterator, std::random_access_iterator_tag>;
    using BidirectionalConstIterator = QTaggedIterator<const_iterator, std::bidirectional_iterator_tag>;
    using ForwardConstIterator = QTaggedIterator<const_iterator, std::forward_iterator_tag>;
    using InputConstIterator = QTaggedIterator<const_iterator, std::input_iterator_tag>;

    template<class T>
    QSequentialIterable(const T *p)
        : QIterable(QMetaSequence::fromContainer<T>(), p)
    {
        Q_UNUSED(m_revision);
    }

    template<class T>
    QSequentialIterable(T *p)
        : QIterable(QMetaSequence::fromContainer<T>(), p)
    {
    }

    QSequentialIterable()
        : QIterable(QMetaSequence(), nullptr)
    {
    }

    template<typename Pointer>
    QSequentialIterable(const QMetaSequence &metaSequence, Pointer iterable)
        : QIterable(metaSequence, iterable)
    {
    }

    QSequentialIterable(const QMetaSequence &metaSequence, const QMetaType &metaType,
                        void *iterable)
        : QIterable(metaSequence, metaType.alignOf(), iterable)
    {
    }

    QSequentialIterable(const QMetaSequence &metaSequence, const QMetaType &metaType,
                        const void *iterable)
        : QIterable(metaSequence, metaType.alignOf(), iterable)
    {
    }

    QSequentialIterable(QIterable<QMetaSequence> &&other) : QIterable(std::move(other)) {}

    QSequentialIterable &operator=(QIterable<QMetaSequence> &&other)
    {
        QIterable::operator=(std::move(other));
        return *this;
    }

    const_iterator begin() const { return constBegin(); }
    const_iterator end() const { return constEnd(); }

    const_iterator constBegin() const { return const_iterator(QIterable::constBegin()); }
    const_iterator constEnd() const { return const_iterator(QIterable::constEnd()); }

    iterator mutableBegin() { return iterator(QIterable::mutableBegin()); }
    iterator mutableEnd() { return iterator(QIterable::mutableEnd()); }

    QVariant at(qsizetype idx) const;
    void set(qsizetype idx, const QVariant &value);

    enum Position { Unspecified, AtBegin, AtEnd };
    void addValue(const QVariant &value, Position position = Unspecified);
    void removeValue(Position position = Unspecified);

    QMetaType valueMetaType() const;
};

template<>
inline QVariantRef<QSequentialIterator>::operator QVariant() const
{
    if (m_pointer == nullptr)
        return QVariant();
    const QMetaType metaType(m_pointer->metaContainer().valueMetaType());
    QVariant v(metaType);
    void *dataPtr = metaType == QMetaType::fromType<QVariant>() ? &v : v.data();
    m_pointer->metaContainer().valueAtIterator(m_pointer->constIterator(), dataPtr);
    return v;
}

template<>
inline QVariantRef<QSequentialIterator> &QVariantRef<QSequentialIterator>::operator=(
        const QVariant &value)
{
    if (m_pointer == nullptr)
        return *this;

    QtPrivate::QVariantTypeCoercer coercer;
    m_pointer->metaContainer().setValueAtIterator(
                m_pointer->constIterator(),
                coercer.coerce(value, m_pointer->metaContainer().valueMetaType()));
    return *this;
}

Q_DECLARE_TYPEINFO(QSequentialIterable, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QSequentialIterable::iterator, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QSequentialIterable::const_iterator, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QSEQUENTIALITERABLE_H
