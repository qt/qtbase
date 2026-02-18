// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qmetaassociation.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMetaAssociation
    \inmodule QtCore
    \since 6.0
    \brief The QMetaAssociation class allows type erased access to associative containers.

    \ingroup objectmodel

    \compares equality

    The class provides a number of primitive container operations, using void*
    as operands. This way, you can manipulate a generic container retrieved from
    a Variant without knowing its type.

    QMetaAssociation covers both, containers with mapped values such as QMap or
    QHash, and containers that only hold keys such as QSet.

    The void* arguments to the various methods are typically created by using
    a \l QVariant of the respective container or value type, and calling
    its \l QVariant::data() or \l QVariant::constData() methods. However, you
    can also pass plain pointers to objects of the container or value type.

    Iterator invalidation follows the rules given by the underlying containers
    and is not expressed in the API. Therefore, for a truly generic container,
    any iterators should be considered invalid after any write operation.

    \sa QMetaContainer, QMetaSequence, QIterable, QIterator
*/

/*!
    \fn template<typename C> QMetaAssociation QMetaAssociation::fromContainer()
    \since 6.0

    Returns the QMetaAssociation corresponding to the type given as template parameter.
*/

/*!
    Returns the meta type for keys in the container.
 */
QMetaType QMetaAssociation::keyMetaType() const
{
    if (auto iface = d())
        return QMetaType(iface->keyMetaType);
    return QMetaType();
}

/*!
    Returns the meta type for mapped values in the container.
 */
QMetaType QMetaAssociation::mappedMetaType() const
{
    if (auto iface = d())
        return QMetaType(iface->mappedMetaType);
    return QMetaType();
}

/*!
    \fn bool QMetaAssociation::canInsertKey() const

    Returns \c true if keys can be added to the container using \l insertKey(),
    otherwise returns \c false.

    \sa insertKey()
 */

/*!
    \fn void QMetaAssociation::insertKey(void *container, const void *key) const

    Inserts the \a key into the \a container if possible. If the container has
    mapped values a default-create mapped value is associated with the \a key.

    \sa canInsertKey()
 */

/*!
    \fn bool QMetaAssociation::canRemoveKey() const

    Returns \c true if keys can be removed from the container using
    \l removeKey(), otherwise returns \c false.

    \sa removeKey()
 */

/*!
    \fn void QMetaAssociation::removeKey(void *container, const void *key) const

    Removes the \a key and its associated mapped value from the \a container if
    possible.

    \sa canRemoveKey()
 */

/*!
    \fn bool QMetaAssociation::canContainsKey() const

    Returns \c true if the container can be queried for keys using
    \l containsKey(), otherwise returns \c false.
 */

/*!
    \fn bool QMetaAssociation::containsKey(const void *container, const void *key) const

    Returns \c true if the \a container can be queried for keys and contains the
    \a key, otherwise returns \c false.

    \sa canContainsKey()
 */

/*!
    \fn bool QMetaAssociation::canGetMappedAtKey() const

    Returns \c true if the container can be queried for values using
    \l mappedAtKey(), otherwise returns \c false.
 */

/*!
    \fn void QMetaAssociation::mappedAtKey(const void *container, const void *key, void *mapped) const

    Retrieves the mapped value associated with the \a key in the \a container
    and places it in the memory location pointed to by \a mapped, if that is
    possible.

    \sa canGetMappedAtKey()
 */

/*!
    \fn bool QMetaAssociation::canSetMappedAtKey() const

    Returns \c true if mapped values can be modified in the container using
    \l setMappedAtKey(), otherwise returns \c false.

    \sa setMappedAtKey()
 */

/*!
    \fn void QMetaAssociation::setMappedAtKey(void *container, const void *key, const void *mapped) const

    Overwrites the value associated with the \a key in the \a container using
    the \a mapped value passed as argument if that is possible.

    \sa canSetMappedAtKey()
 */

/*!
    \fn bool QMetaAssociation::canGetKeyAtIterator() const

    Returns \c true if a key can be retrieved from a non-const iterator using
    \l keyAtIterator(), otherwise returns \c false.

    \sa keyAtIterator()
 */

/*!
    \fn void QMetaAssociation::keyAtIterator(const void *iterator, void *key) const

    Retrieves the key pointed to by the non-const \a iterator and stores it
    in the memory location pointed to by \a key, if possible.

    \sa canGetKeyAtIterator(), begin(), end(), createIteratorAtKey()
 */

/*!
    \fn bool QMetaAssociation::canGetKeyAtConstIterator() const

    Returns \c true if a key can be retrieved from a const iterator using
    \l keyAtConstIterator(), otherwise returns \c false.

    \sa keyAtConstIterator()
 */

/*!
    \fn void QMetaAssociation::keyAtConstIterator(const void *iterator, void *key) const

    Retrieves the key pointed to by the const \a iterator and stores it
    in the memory location pointed to by \a key, if possible.

    \sa canGetKeyAtConstIterator(), constBegin(), constEnd(), createConstIteratorAtKey()
 */

/*!
    \fn bool QMetaAssociation::canGetMappedAtIterator() const

    Returns \c true if a mapped value can be retrieved from a non-const
    iterator using \l mappedAtIterator(), otherwise returns \c false.

    \sa mappedAtIterator()
 */

/*!
    \fn void QMetaAssociation::mappedAtIterator(const void *iterator, void *mapped) const

    Retrieves the mapped value pointed to by the non-const \a iterator and
    stores it in the memory location pointed to by \a mapped, if possible.

    \sa canGetMappedAtIterator(), begin(), end(), createIteratorAtKey()
 */

/*!
    \fn bool QMetaAssociation::canGetMappedAtConstIterator() const

    Returns \c true if a mapped value can be retrieved from a const iterator
    using \l mappedAtConstIterator(), otherwise returns \c false.

    \sa mappedAtConstIterator()
 */

/*!
    \fn void QMetaAssociation::mappedAtConstIterator(const void *iterator, void *mapped) const

    Retrieves the mapped value pointed to by the const \a iterator and
    stores it in the memory location pointed to by \a mapped, if possible.

    \sa canGetMappedAtConstIterator(), constBegin(), constEnd(), createConstIteratorAtKey()
 */

/*!
    \fn bool QMetaAssociation::canSetMappedAtIterator() const

    Returns \c true if a mapped value can be set via a non-const iterator using
    \l setMappedAtIterator(), otherwise returns \c false.

    \sa setMappedAtIterator()
 */

/*!
    \fn void QMetaAssociation::setMappedAtIterator(const void *iterator, const void *mapped) const

    Writes the \a mapped value to the container location pointed to by the
    non-const \a iterator, if possible.

    \sa canSetMappedAtIterator(), begin(), end(), createIteratorAtKey()
 */

/*!
    \fn bool QMetaAssociation::canCreateIteratorAtKey() const

    Returns \c true if an iterator pointing to an entry in the container can be
    created using \l createIteratorAtKey(), otherwise returns false.

    \sa createIteratorAtKey()
 */

/*!
    \fn void *QMetaAssociation::createIteratorAtKey(void *container, const void *key) const

    Returns a non-const iterator pointing to the entry of \a key in the
    \a container, if possible. If the entry doesn't exist, creates a non-const
    iterator pointing to the end of the \a container. If no non-const iterator
    can be created, returns \c nullptr.

    The non-const iterator has to be destroyed using destroyIterator().

    \sa canCreateIteratorAtKey(), begin(), end(), destroyIterator()
 */

/*!
    \fn bool QMetaAssociation::canCreateConstIteratorAtKey() const

    Returns \c true if a const iterator pointing to an entry in the container
    can be created using \l createConstIteratorAtKey(), otherwise returns false.
 */

/*!
    \fn void *QMetaAssociation::createConstIteratorAtKey(const void *container, const void *key) const

    Returns a const iterator pointing to the entry of \a key in the
    \a container, if possible. If the entry doesn't exist, creates a const
    iterator pointing to the end of the \a container. If no const iterator can
    be created, returns \c nullptr.

    The const iterator has to be destroyed using destroyConstIterator().

    \sa canCreateConstIteratorAtKey(), constBegin(), constEnd(), destroyConstIterator()
 */

/*!
    \fn bool QMetaAssociation::operator==(const QMetaAssociation &lhs, const QMetaAssociation &rhs)

    Returns \c true if the QMetaAssociation \a lhs represents the same container type
    as the QMetaAssociation \a rhs, otherwise returns \c false.
*/
/*!
    \fn bool QMetaAssociation::operator!=(const QMetaAssociation &lhs, const QMetaAssociation &rhs)

    Returns \c true if the QMetaAssociation \a lhs represents a different container
    type than the QMetaAssociation \a rhs, otherwise returns \c false.
*/

/*!
    \class QMetaAssociation::Iterable
    \inherits QIterable
    \since 6.11
    \inmodule QtCore
    \brief QMetaAssociation::Iterable is an iterable interface for an associative container in a QVariant.

    This class allows several methods of accessing the elements of an
    associative container held within a QVariant. An instance of
    QMetaAssociation::Iterable can be extracted from a QVariant if it can be
    converted to a QVariantHash or QVariantMap or if a custom mutable view has
    been registered.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \typealias QMetaAssociation::Iterable::RandomAccessIterator
    Exposes an iterator using std::random_access_iterator_tag.
*/

/*!
    \typealias QMetaAssociation::Iterable::BidirectionalIterator
    Exposes an iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typealias QMetaAssociation::Iterable::ForwardIterator
    Exposes an iterator using std::forward_iterator_tag.
*/

/*!
    \typealias QMetaAssociation::Iterable::InputIterator
    Exposes an iterator using std::input_iterator_tag.
*/

/*!
    \typealias QMetaAssociation::Iterable::RandomAccessConstIterator
    Exposes a const_iterator using std::random_access_iterator_tag.
*/

/*!
    \typealias QMetaAssociation::Iterable::BidirectionalConstIterator
    Exposes a const_iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typealias QMetaAssociation::Iterable::ForwardConstIterator
    Exposes a const_iterator using std::forward_iterator_tag.
*/

/*!
    \typealias QMetaAssociation::Iterable::InputConstIterator
    Exposes a const_iterator using std::input_iterator_tag.
*/

/*!
    \class QMetaAssociation::Iterable::ConstIterator
    \inherits QConstIterator
    \since 6.11
    \inmodule QtCore
    \brief QMetaAssociation::Iterable::ConstIterator allows iteration over a container in a QVariant.

    A QMetaAssociation::Iterable::ConstIterator can only be created by a
    QMetaAssociation::Iterable instance, and can be used in a way similar to
    other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    \sa QMetaAssociation::Iterable
*/

/*!
    \class QMetaAssociation::Iterable::Iterator
    \inherits QIterator
    \since 6.11
    \inmodule QtCore
    \brief The QMetaAssociation::Iterable::Iterator allows iteration over a container in a QVariant.

    A QMetaAssociation::Iterable::Iterator can only be created by a
    QMetaAssociation::Iterable instance, and can be used in a way similar to
    other stl-style iterators.

    \sa QMetaAssociation::Iterable
*/

/*!
    \fn QMetaAssociation::Iterable::ConstIterator QMetaAssociation::Iterable::find(const QVariant &key) const
    Retrieves a ConstIterator pointing to the element at the given \a key, or
    the end of the container if that key does not exist. If the \a key isn't
    convertible to the expected type, the end of the container is returned.
 */

/*!
    \fn QMetaAssociation::Iterable::Iterator QMetaAssociation::Iterable::mutableFind(const QVariant &key)
    Retrieves an iterator pointing to the element at the given \a key, or
    the end of the container if that key does not exist. If the \a key isn't
    convertible to the expected type, the end of the container is returned.
 */

/*!
    \fn bool QMetaAssociation::Iterable::containsKey(const QVariant &key) const
    Returns \c true if the container has an entry with the given \a key, or
    \c false otherwise. If the \a key isn't convertible to the expected type,
    \c false is returned.
 */

/*!
    \fn void QMetaAssociation::Iterable::insertKey(const QVariant &key)
    Inserts a new entry with the given \a key, or resets the mapped value of
    any existing entry with the given \a key to the default constructed
    mapped value. The \a key is coerced to the expected type: If it isn't
    convertible, a default value is inserted.
 */

/*!
    \fn void QMetaAssociation::Iterable::removeKey(const QVariant &key)
    Removes the entry with the given \a key from the container. The \a key is
    coerced to the expected type: If it isn't convertible, the default value
    is removed.
 */

/*!
    \fn QVariant QMetaAssociation::Iterable::value(const QVariant &key) const
    Retrieves the mapped value at the given \a key, or a QVariant of a
    default-constructed instance of the mapped type, if the key does not
    exist. If the \a key is not convertible to the key type, the mapped value
    associated with the default-constructed key is returned.
 */

/*!
    \fn void QMetaAssociation::Iterable::setValue(const QVariant &key, const QVariant &mapped)
    Sets the mapped value associated with \a key to \a mapped, if possible.
    Inserts a new entry if none exists yet, for the given \a key. If the
    \a key is not convertible to the key type, the value for the
    default-constructed key type is overwritten.
 */


/*!
    \fn QVariant QMetaAssociation::Iterable::Iterator::key() const
    Returns the key this iterator points to.
*/

/*!
    \fn QVariant::Reference<QMetaAssociation::Iterable::Iterator> QMetaAssociation::Iterable::Iterator::value() const
    Returns the mapped value this iterator points to. If the container does not
    provide a mapped value (for example a set), returns an invalid
    QVariant::Reference.
*/

/*!
    \fn QVariant::Reference<QMetaAssociation::Iterable::Iterator> QMetaAssociation::Iterable::Iterator::operator*() const
    Returns the current item, converted to a QVariant::Reference. The resulting
    QVariant::Reference resolves to the mapped value if there is one, or to the
    key value if not.
*/

/*!
    \fn QVariant::Pointer<QMetaAssociation::Iterable::Iterator> QMetaAssociation::Iterable::Iterator::operator->() const
    Returns the current item, converted to a QVariant::Pointer. The resulting
    QVariant::Pointer resolves to the mapped value if there is one, or to the
    key value if not.
*/

/*!
    \fn QVariant QMetaAssociation::Iterable::ConstIterator::key() const
    Returns the key this iterator points to.
*/

/*!
    \fn QVariant QMetaAssociation::Iterable::ConstIterator::value() const
    Returns the mapped value this iterator points to, or an invalid QVariant if
    there is no mapped value.
*/

/*!
    \fn QVariant QMetaAssociation::Iterable::ConstIterator::operator*() const
    Returns the current item, converted to a QVariant. The returned value is the
    mapped value at the current iterator if there is one, or otherwise the key.
*/

/*!
    \fn QVariant::ConstPointer<QMetaAssociation::Iterable::ConstIterator> QMetaAssociation::Iterable::ConstIterator::operator->() const
    Returns the current item, converted to a QVariant::ConstPointer. The
    QVariant::ConstPointer will resolve to the mapped value at the current
    iterator if there is one, or otherwise the key.
*/

QT_END_NAMESPACE
