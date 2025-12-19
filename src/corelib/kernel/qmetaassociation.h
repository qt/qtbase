// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMETAASSOCIATION_H
#define QMETAASSOCIATION_H

#if 0
#pragma qt_class(QMetaAssociation)
#endif

#include <QtCore/qiterable.h>
#include <QtCore/qiterable_impl.h>
#include <QtCore/qmetacontainer.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

namespace QtMetaContainerPrivate {

class AssociativeIterator : public QIterator<QMetaAssociation>
{
public:
    using key_type = QVariant;
    using mapped_type = QVariant;
    using reference = QVariant::Reference<AssociativeIterator>;
    using pointer = QVariant::Pointer<AssociativeIterator>;

    static constexpr bool CanNoexceptAssignQVariant = false;
    static constexpr bool CanNoexceptConvertToQVariant = false;

    AssociativeIterator(QIterator &&it) : QIterator(std::move(it)) {}

    key_type key() const
    {
        const QMetaAssociation meta = metaContainer();
        return QtIterablePrivate::retrieveElement(meta.keyMetaType(), [&](void *dataPtr) {
            meta.keyAtIterator(constIterator(), dataPtr);
        });
    }
    reference value() const { return operator*(); }

    reference operator*() const { return reference(*this); }
    pointer operator->() const { return pointer(*this); }
};

class AssociativeConstIterator : public QConstIterator<QMetaAssociation>
{
public:
    using key_type = QVariant;
    using mapped_type = QVariant;
    using reference = QVariant::ConstReference<AssociativeConstIterator>;
    using pointer = QVariant::ConstPointer<AssociativeConstIterator>;

    static constexpr bool CanNoexceptConvertToQVariant = false;

    AssociativeConstIterator(QConstIterator &&it) : QConstIterator(std::move(it)) {}

    key_type key() const
    {
        const QMetaAssociation meta = metaContainer();
        return QtIterablePrivate::retrieveElement(meta.keyMetaType(), [&](void *dataPtr) {
            meta.keyAtConstIterator(constIterator(), dataPtr);
        });
    }

    mapped_type value() const { return operator*(); }

    mapped_type operator*() const;
    pointer operator->() const { return pointer(*this); }
};

} // namespace QtMetaContainerPrivate

namespace QtPrivate {

template<typename Referred>
QVariant associativeIteratorToVariant(const Referred &referred)
{
    const auto metaAssociation = referred.metaContainer();
    const QMetaType metaType(metaAssociation.mappedMetaType());
    if (metaType.isValid(QT6_CALL_NEW_OVERLOAD)) {
        return QtIterablePrivate::retrieveElement(metaType, [&](void *dataPtr) {
            metaAssociation.mappedAtConstIterator(referred.constIterator(), dataPtr);
        });
    }

    return QtIterablePrivate::retrieveElement(metaType, [&](void *dataPtr) {
        metaAssociation.keyAtConstIterator(referred.constIterator(), dataPtr);
    });
}

} // namespace QtPrivate

template<>
inline QVariant::Reference<QtMetaContainerPrivate::AssociativeIterator>::operator QVariant() const
{
    return QtPrivate::associativeIteratorToVariant(m_referred);
}

template<>
inline QVariant::Reference<QtMetaContainerPrivate::AssociativeIterator> &
QVariant::Reference<QtMetaContainerPrivate::AssociativeIterator>::operator=(const QVariant &value)
{
    const auto metaAssociation = m_referred.metaContainer();
    const QMetaType metaType(metaAssociation.mappedMetaType());
    if (!metaType.isValid(QT6_CALL_NEW_OVERLOAD))
        return *this;

    QtPrivate::QVariantTypeCoercer coercer;
    metaAssociation.setMappedAtIterator(
            m_referred.constIterator(), coercer.coerce(value, metaType));
    return *this;
}

template<>
inline QVariant::ConstReference<QtMetaContainerPrivate::AssociativeConstIterator>::operator QVariant() const
{
    return QtPrivate::associativeIteratorToVariant(m_referred);
}

namespace QtMetaContainerPrivate {
inline AssociativeConstIterator::mapped_type AssociativeConstIterator::operator*() const
{
    return reference(*this);
}

class Association : public QIterable<QMetaAssociation>
{
public:
    using Iterator
            = QTaggedIterator<AssociativeIterator, void>;
    using RandomAccessIterator
            = QTaggedIterator<AssociativeIterator, std::random_access_iterator_tag>;
    using BidirectionalIterator
            = QTaggedIterator<AssociativeIterator, std::bidirectional_iterator_tag>;
    using ForwardIterator
            = QTaggedIterator<AssociativeIterator, std::forward_iterator_tag>;
    using InputIterator
            = QTaggedIterator<AssociativeIterator, std::input_iterator_tag>;

    using ConstIterator
            = QTaggedIterator<AssociativeConstIterator, void>;
    using RandomAccessConstIterator
            = QTaggedIterator<AssociativeConstIterator, std::random_access_iterator_tag>;
    using BidirectionalConstIterator
            = QTaggedIterator<AssociativeConstIterator, std::bidirectional_iterator_tag>;
    using ForwardConstIterator
            = QTaggedIterator<AssociativeConstIterator, std::forward_iterator_tag>;
    using InputConstIterator
            = QTaggedIterator<AssociativeConstIterator, std::input_iterator_tag>;

    using iterator = Iterator;
    using const_iterator = ConstIterator;

    template<class T>
    Association(const T *p) : QIterable(QMetaAssociation::fromContainer<T>(), p) {}

    template<class T>
    Association(T *p) : QIterable(QMetaAssociation::fromContainer<T>(), p) {}

    Association() : QIterable(QMetaAssociation(), nullptr) {}

    template<typename Pointer>
    Association(const QMetaAssociation &metaAssociation, Pointer iterable)
        : QIterable(metaAssociation, iterable)
    {
    }

    Association(const QMetaAssociation &metaAssociation, QMetaType metaType, void *iterable)
        : QIterable(metaAssociation, metaType.alignOf(), iterable)
    {
    }

    Association(const QMetaAssociation &metaAssociation, QMetaType metaType, const void *iterable)
        : QIterable(metaAssociation, metaType.alignOf(), iterable)
    {
    }

    Association(QIterable<QMetaAssociation> &&other)
        : QIterable(std::move(other))
    {}

    Association &operator=(QIterable<QMetaAssociation> &&other)
    {
        QIterable::operator=(std::move(other));
        return *this;
    }

    ConstIterator begin() const { return constBegin(); }
    ConstIterator end() const { return constEnd(); }

    ConstIterator constBegin() const { return ConstIterator(QIterable::constBegin()); }
    ConstIterator constEnd() const { return ConstIterator(QIterable::constEnd()); }

    Iterator mutableBegin() { return Iterator(QIterable::mutableBegin()); }
    Iterator mutableEnd() { return Iterator(QIterable::mutableEnd()); }

    ConstIterator find(const QVariant &key) const
    {
        const QMetaAssociation meta = metaContainer();
        QtPrivate::QVariantTypeCoercer coercer;
        if (const void *keyData = coercer.convert(key, meta.keyMetaType())) {
            return ConstIterator(QConstIterator<QMetaAssociation>(
                    this, meta.createConstIteratorAtKey(constIterable(), keyData)));
        }
        return constEnd();
    }

    ConstIterator constFind(const QVariant &key) const { return find(key); }

    Iterator mutableFind(const QVariant &key)
    {
        const QMetaAssociation meta = metaContainer();
        QtPrivate::QVariantTypeCoercer coercer;
        if (const void *keyData = coercer.convert(key, meta.keyMetaType()))
            return Iterator(QIterator(this, meta.createIteratorAtKey(mutableIterable(), keyData)));
        return mutableEnd();
    }

    bool containsKey(const QVariant &key) const
    {
        const QMetaAssociation meta = metaContainer();
        QtPrivate::QVariantTypeCoercer keyCoercer;
        if (const void *keyData = keyCoercer.convert(key, meta.keyMetaType()))
            return meta.containsKey(constIterable(), keyData);
        return false;
    }

    void insertKey(const QVariant &key)
    {
        const QMetaAssociation meta = metaContainer();
        QtPrivate::QVariantTypeCoercer keyCoercer;
        meta.insertKey(mutableIterable(), keyCoercer.coerce(key, meta.keyMetaType()));
    }

    void removeKey(const QVariant &key)
    {
        const QMetaAssociation meta = metaContainer();
        QtPrivate::QVariantTypeCoercer keyCoercer;
        meta.removeKey(mutableIterable(), keyCoercer.coerce(key, meta.keyMetaType()));
    }

    QVariant value(const QVariant &key) const
    {
        const QMetaAssociation meta = metaContainer();
        return QtIterablePrivate::retrieveElement(meta.mappedMetaType(), [&](void *dataPtr) {
            QtPrivate::QVariantTypeCoercer coercer;
            meta.mappedAtKey(constIterable(), coercer.coerce(key, meta.keyMetaType()), dataPtr);
        });
    }

    void setValue(const QVariant &key, const QVariant &mapped)
    {
        const QMetaAssociation meta = metaContainer();
        QtPrivate::QVariantTypeCoercer keyCoercer;
        QtPrivate::QVariantTypeCoercer mappedCoercer;
        meta.setMappedAtKey(mutableIterable(), keyCoercer.coerce(key, meta.keyMetaType()),
                            mappedCoercer.coerce(mapped, meta.mappedMetaType()));
    }
};

} // namespace QtMetaContainerPrivate

QT_END_NAMESPACE

#endif // QMETAASSOCIATION_H
