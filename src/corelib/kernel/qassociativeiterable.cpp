// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qassociativeiterable.h>
#include <QtCore/qvariant.h>

#include <QtCore/private/qiterable_p.h>

QT_BEGIN_NAMESPACE

/*!
    Returns the key this iterator points to.
*/
QVariant QAssociativeIterator::key() const
{
    return QIterablePrivate::retrieveElement(
                metaContainer().keyMetaType(), [this](void *dataPtr) {
        metaContainer().keyAtIterator(constIterator(), dataPtr);
    });
}

/*!
    Returns the mapped value this iterator points to. If the container does not
    provide a mapped value (for example a set), returns an invalid QVariantRef.
*/
QVariantRef<QAssociativeIterator> QAssociativeIterator::value() const
{
    const QMetaType mappedMetaType(metaContainer().mappedMetaType());
    return QVariantRef<QAssociativeIterator>(mappedMetaType.isValid() ? this : nullptr);
}

/*!
    Returns the current item, converted to a QVariantRef. The resulting
    QVariantRef resolves to the mapped value if there is one, or to the key
    value if not.
*/
QVariantRef<QAssociativeIterator> QAssociativeIterator::operator*() const
{
    return QVariantRef<QAssociativeIterator>(this);
}

/*!
    Returns the current item, converted to a QVariantPointer. The resulting
    QVariantPointer resolves to the mapped value if there is one, or to the key
    value if not.
*/
QVariantPointer<QAssociativeIterator> QAssociativeIterator::operator->() const
{
    return QVariantPointer<QAssociativeIterator>(this);
}

/*!
    Returns the key this iterator points to.
*/
QVariant QAssociativeConstIterator::key() const
{
    return QIterablePrivate::retrieveElement(
                metaContainer().keyMetaType(), [this](void *dataPtr) {
        metaContainer().keyAtConstIterator(constIterator(), dataPtr);
    });
}

/*!
    Returns the mapped value this iterator points to, or an invalid QVariant if
    there is no mapped value.
*/
QVariant QAssociativeConstIterator::value() const
{
    return QIterablePrivate::retrieveElement(
                metaContainer().mappedMetaType(), [this](void *dataPtr) {
        metaContainer().mappedAtConstIterator(constIterator(), dataPtr);
    });
}

/*!
    Returns the current item, converted to a QVariant. The returned value is the
    mapped value at the current iterator if there is one, or otherwise the key.
*/
QVariant QAssociativeConstIterator::operator*() const
{
    const QMetaType mappedMetaType(metaContainer().mappedMetaType());
    return mappedMetaType.isValid() ? value() : key();
}

/*!
    Returns the current item, converted to a QVariantConstPointer. The
    QVariantConstPointer will resolve to the mapped value at the current
    iterator if there is one, or otherwise the key.
*/
QVariantConstPointer QAssociativeConstIterator::operator->() const
{
    return QVariantConstPointer(operator*());
}

/*!
    \class QAssociativeIterable
    \since 5.2
    \inmodule QtCore
    \brief The QAssociativeIterable class is an iterable interface for an associative container in a QVariant.

    This class allows several methods of accessing the elements of an associative container held within
    a QVariant. An instance of QAssociativeIterable can be extracted from a QVariant if it can
    be converted to a QVariantHash or QVariantMap or if a custom mutable view has been registered.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \typedef QAssociativeIterable::RandomAccessIterator
    Exposes an iterator using std::random_access_iterator_tag.
*/

/*!
    \typedef QAssociativeIterable::BidirectionalIterator
    Exposes an iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typedef QAssociativeIterable::ForwardIterator
    Exposes an iterator using std::forward_iterator_tag.
*/

/*!
    \typedef QAssociativeIterable::InputIterator
    Exposes an iterator using std::input_iterator_tag.
*/

/*!
    \typedef QAssociativeIterable::RandomAccessConstIterator
    Exposes a const_iterator using std::random_access_iterator_tag.
*/

/*!
    \typedef QAssociativeIterable::BidirectionalConstIterator
    Exposes a const_iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typedef QAssociativeIterable::ForwardConstIterator
    Exposes a const_iterator using std::forward_iterator_tag.
*/

/*!
    \typedef QAssociativeIterable::InputConstIterator
    Exposes a const_iterator using std::input_iterator_tag.
*/

/*!
    Retrieves a const_iterator pointing to the element at the given \a key, or
    the end of the container if that key does not exist. If the \a key isn't
    convertible to the expected type, the end of the container is returned.
 */
QAssociativeIterable::const_iterator QAssociativeIterable::find(const QVariant &key) const
{
    const QMetaAssociation meta = metaContainer();
    QtPrivate::QVariantTypeCoercer coercer;
    if (const void *keyData = coercer.convert(key, meta.keyMetaType())) {
        return const_iterator(QConstIterator(this, meta.createConstIteratorAtKey(
                                                 constIterable(), keyData)));
    }
    return constEnd();
}

/*!
    Retrieves an iterator pointing to the element at the given \a key, or
    the end of the container if that key does not exist. If the \a key isn't
    convertible to the expected type, the end of the container is returned.
 */
QAssociativeIterable::iterator QAssociativeIterable::mutableFind(const QVariant &key)
{
    const QMetaAssociation meta = metaContainer();
    QtPrivate::QVariantTypeCoercer coercer;
    if (const void *keyData = coercer.convert(key, meta.keyMetaType()))
        return iterator(QIterator(this, meta.createIteratorAtKey(mutableIterable(), keyData)));
    return mutableEnd();
}

/*!
    Returns \c true if the container has an entry with the given \a key, or
    \c false otherwise. If the \a key isn't convertible to the expected type,
    \c false is returned.
 */
bool QAssociativeIterable::containsKey(const QVariant &key)
{
    QtPrivate::QVariantTypeCoercer keyCoercer;
    QMetaAssociation meta = metaContainer();
    if (const void *keyData = keyCoercer.convert(key, meta.keyMetaType()))
        return meta.containsKey(constIterable(), keyData);
    return false;
}

/*!
    Inserts a new entry with the given \a key, or resets the mapped value of
    any existing entry with the given \a key to the default constructed
    mapped value. The \a key is coerced to the expected type: If it isn't
    convertible, a default value is inserted.
 */
void QAssociativeIterable::insertKey(const QVariant &key)
{
    QMetaAssociation meta = metaContainer();
    QtPrivate::QVariantTypeCoercer keyCoercer;
    meta.insertKey(mutableIterable(), keyCoercer.coerce(key, meta.keyMetaType()));
}

/*!
    Removes the entry with the given \a key from the container. The \a key is
    coerced to the expected type: If it isn't convertible, the default value
    is removed.
 */
void QAssociativeIterable::removeKey(const QVariant &key)
{
    QMetaAssociation meta = metaContainer();
    QtPrivate::QVariantTypeCoercer keyCoercer;
    meta.removeKey(mutableIterable(), keyCoercer.coerce(key, meta.keyMetaType()));
}


/*!
    Retrieves the mapped value at the given \a key, or a default-constructed
    QVariant of the mapped type, if the key does not exist. If the \a key is not
    convertible to the key type, the mapped value associated with the
    default-constructed key is returned.
 */
QVariant QAssociativeIterable::value(const QVariant &key) const
{
    const QMetaAssociation meta = metaContainer();
    const QMetaType mappedMetaType = meta.mappedMetaType();

    QtPrivate::QVariantTypeCoercer coercer;
    const void *keyData = coercer.coerce(key, meta.keyMetaType());

    if (mappedMetaType == QMetaType::fromType<QVariant>()) {
        QVariant result;
        meta.mappedAtKey(constIterable(), keyData, &result);
        return result;
    }

    QVariant result(mappedMetaType);
    meta.mappedAtKey(constIterable(), keyData, result.data());
    return result;
}

/*!
    Sets the mapped value associated with \a key to \a mapped, if possible.
    Inserts a new entry if none exists yet, for the given \a key. If the \a key
    is not convertible to the key type, the value for the default-constructed
    key type is overwritten.
 */
void QAssociativeIterable::setValue(const QVariant &key, const QVariant &mapped)
{
    QtPrivate::QVariantTypeCoercer keyCoercer;
    QtPrivate::QVariantTypeCoercer mappedCoercer;
    QMetaAssociation meta = metaContainer();
    meta.setMappedAtKey(mutableIterable(), keyCoercer.coerce(key, meta.keyMetaType()),
                        mappedCoercer.coerce(mapped, meta.mappedMetaType()));
}

/*!
    \typealias QAssociativeIterable::const_iterator
    \inmodule QtCore
    \brief The QAssociativeIterable::const_iterator allows iteration over a container in a QVariant.

    A QAssociativeIterable::const_iterator can only be created by a QAssociativeIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    \sa QAssociativeIterable
*/

/*!
    \typealias QAssociativeIterable::iterator
    \since 6.0
    \inmodule QtCore
    \brief The QAssociativeIterable::iterator allows iteration over a container in a QVariant.

    A QAssociativeIterable::iterator can only be created by a QAssociativeIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \sa QAssociativeIterable
*/

QT_END_NAMESPACE
