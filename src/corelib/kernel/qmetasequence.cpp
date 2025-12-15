// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qmetasequence.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMetaSequence
    \inmodule QtCore
    \since 6.0
    \brief The QMetaSequence class allows type erased access to sequential containers.

    \ingroup objectmodel

    \compares equality

    The class provides a number of primitive container operations, using void*
    as operands. This way, you can manipulate a generic container retrieved from
    a Variant without knowing its type.

    The void* arguments to the various methods are typically created by using
    a \l QVariant of the respective container or value type, and calling
    its \l QVariant::data() or \l QVariant::constData() methods. However, you
    can also pass plain pointers to objects of the container or value type.

    Iterator invalidation follows the rules given by the underlying containers
    and is not expressed in the API. Therefore, for a truly generic container,
    any iterators should be considered invalid after any write operation.
*/

/*!
    \fn template<typename C> QMetaSequence QMetaSequence::fromContainer()
    \since 6.0

    Returns the QMetaSequence corresponding to the type given as template parameter.
*/

/*!
    Returns the meta type for values stored in the container.
 */
QMetaType QMetaSequence::valueMetaType() const
{
    if (auto iface = d())
        return QMetaType(iface->valueMetaType);
    return QMetaType();
}

/*!
    Returns \c true if the underlying container is sortable, otherwise returns
    \c false. A container is considered sortable if values added to it are
    placed in a defined location. Inserting into or adding to a sortable
    container will always succeed. Inserting into or adding to an unsortable
    container may not succeed, for example if the container is a QSet that
    already contains the value being inserted.

    \sa addValue(), insertValueAtIterator(), canAddValueAtBegin(),
        canAddValueAtEnd(), canRemoveValueAtBegin(), canRemoveValueAtEnd()
 */
bool QMetaSequence::isSortable() const
{
    if (auto iface = d()) {
        return (iface->addRemoveCapabilities
                & (QtMetaContainerPrivate::CanAddAtBegin | QtMetaContainerPrivate::CanAddAtEnd))
                && (iface->addRemoveCapabilities
                    & (QtMetaContainerPrivate::CanRemoveAtBegin
                       | QtMetaContainerPrivate::CanRemoveAtEnd));
    }
    return false;
}

/*!
    Returns \c true if values added using \l addValue() can be placed at the
    beginning of the container, otherwise returns \c false.

    \sa addValueAtBegin(), canAddValueAtEnd()
 */
bool QMetaSequence::canAddValueAtBegin() const
{
    if (auto iface = d()) {
        return iface->addValueFn
                && iface->addRemoveCapabilities & QtMetaContainerPrivate::CanAddAtBegin;
    }
    return false;
}

/*!
    Adds \a value to the beginning of \a container if possible. If
    \l canAddValueAtBegin() returns \c false, the \a value is not added.

    \sa canAddValueAtBegin(), isSortable(), removeValueAtBegin()
 */
void QMetaSequence::addValueAtBegin(void *container, const void *value) const
{
    if (canAddValueAtBegin())
        d()->addValueFn(container, value, QtMetaContainerPrivate::QMetaSequenceInterface::AtBegin);
}

/*!
    Returns \c true if values can be removed from the beginning of the container
    using \l removeValue(), otherwise returns \c false.

    \sa removeValueAtBegin(), canRemoveValueAtEnd()
 */
bool QMetaSequence::canRemoveValueAtBegin() const
{
    if (auto iface = d()) {
        return iface->removeValueFn
                && iface->addRemoveCapabilities & QtMetaContainerPrivate::CanRemoveAtBegin;
    }
    return false;
}

/*!
    Removes a value from the beginning of \a container if possible. If
    \l canRemoveValueAtBegin() returns \c false, the value is not removed.

    \sa canRemoveValueAtBegin(), isSortable(), addValueAtBegin()
 */
void QMetaSequence::removeValueAtBegin(void *container) const
{
    if (canRemoveValueAtBegin())
        d()->removeValueFn(container, QtMetaContainerPrivate::QMetaSequenceInterface::AtBegin);
}

/*!
    Returns \c true if values added using \l addValue() can be placed at the
    end of the container, otherwise returns \c false.

    \sa addValueAtEnd(), canAddValueAtBegin()
 */
bool QMetaSequence::canAddValueAtEnd() const
{
    if (auto iface = d()) {
        return iface->addValueFn
                && iface->addRemoveCapabilities & QtMetaContainerPrivate::CanAddAtEnd;
    }
    return false;
}

/*!
    Adds \a value to the end of \a container if possible. If
    \l canAddValueAtEnd() returns \c false, the \a value is not added.

    \sa canAddValueAtEnd(), isSortable(), removeValueAtEnd()
 */
void QMetaSequence::addValueAtEnd(void *container, const void *value) const
{
    if (canAddValueAtEnd())
        d()->addValueFn(container, value, QtMetaContainerPrivate::QMetaSequenceInterface::AtEnd);
}

/*!
    Returns \c true if values can be removed from the end of the container
    using \l removeValue(), otherwise returns \c false.

    \sa removeValueAtEnd(), canRemoveValueAtBegin()
 */
bool QMetaSequence::canRemoveValueAtEnd() const
{
    if (auto iface = d()) {
        return iface->removeValueFn
                && iface->addRemoveCapabilities & QtMetaContainerPrivate::CanRemoveAtEnd;
    }
    return false;
}

/*!
    Removes a value from the end of \a container if possible. If
    \l canRemoveValueAtEnd() returns \c false, the value is not removed.

    \sa canRemoveValueAtEnd(), isSortable(), addValueAtEnd()
 */
void QMetaSequence::removeValueAtEnd(void *container) const
{
    if (canRemoveValueAtEnd())
        d()->removeValueFn(container, QtMetaContainerPrivate::QMetaSequenceInterface::AtEnd);
}

/*!
    Returns \c true if values can be retrieved from the container by index,
    otherwise \c false.

    \sa valueAtIndex()
 */
bool QMetaSequence::canGetValueAtIndex() const
{
    if (auto iface = d())
        return iface->valueAtIndexFn;
    return false;
}

/*!
    Retrieves the value at \a index in the \a container and places it in the
    memory location pointed to by \a result, if that is possible.

    \sa canGetValueAtIndex()
 */
void QMetaSequence::valueAtIndex(const void *container, qsizetype index, void *result) const
{
    if (canGetValueAtIndex())
        d()->valueAtIndexFn(container, index, result);
}

/*!
    Returns \c true if a value can be written to the container by index,
    otherwise \c false.

    \sa setValueAtIndex()
*/
bool QMetaSequence::canSetValueAtIndex() const
{
    if (auto iface = d())
        return iface->setValueAtIndexFn;
    return false;
}

/*!
    Overwrites the value at \a index in the \a container using the \a value
    passed as parameter if that is possible.

    \sa canSetValueAtIndex()
 */
void QMetaSequence::setValueAtIndex(void *container, qsizetype index, const void *value) const
{
    if (canSetValueAtIndex())
        d()->setValueAtIndexFn(container, index, value);
}

/*!
    Returns \c true if values can be added to the container, \c false
    otherwise.

    \sa addValue(), isSortable()
 */
bool QMetaSequence::canAddValue() const
{
    if (auto iface = d())
        return iface->addValueFn;
    return false;
}

/*!
    Adds \a value to the \a container if possible. If \l canAddValue()
    returns \c false, the \a value is not added. Else, if
    \l canAddValueAtEnd() returns \c true, the \a value is added
    to the end of the \a container. Else, if
    \l canAddValueAtBegin() returns \c true, the \a value is added to
    the beginning of the container. Else, the value is added in an unspecified
    place or not at all. The latter is the case for adding values to an
    unordered container, for example \l QSet.

    \sa canAddValue(), canAddValueAtBegin(),
        canAddValueAtEnd(), isSortable(), removeValue()
 */
void QMetaSequence::addValue(void *container, const void *value) const
{
    if (canAddValue()) {
        d()->addValueFn(container, value,
                        QtMetaContainerPrivate::QMetaSequenceInterface::Unspecified);
    }
}

/*!
    Returns \c true if values can be removed from the container, \c false
    otherwise.

    \sa removeValue(), isSortable()
 */
bool QMetaSequence::canRemoveValue() const
{
    if (auto iface = d())
        return iface->removeValueFn;
    return false;
}

/*!
    Removes a value from the \a container if possible. If
    \l canRemoveValue() returns \c false, no value is removed. Else, if
    \l canRemoveValueAtEnd() returns \c true, the last value in
    the \a container is removed. Else, if \l canRemoveValueAtBegin()
    returns \c true, the first value in the \a container is removed. Else,
    an unspecified value or nothing is removed.

    \sa canRemoveValue(), canRemoveValueAtBegin(),
        canRemoveValueAtEnd(), isSortable(), addValue()
 */
void QMetaSequence::removeValue(void *container) const
{
    if (canRemoveValue()) {
        d()->removeValueFn(container,
                           QtMetaContainerPrivate::QMetaSequenceInterface::Unspecified);
    }
}


/*!
    Returns \c true if the underlying container can retrieve the value pointed
    to by a non-const iterator, \c false otherwise.

    \sa hasIterator(), valueAtIterator()
 */
bool QMetaSequence::canGetValueAtIterator() const
{
    if (auto iface = d())
        return iface->valueAtIteratorFn;
    return false;
}

/*!
    Retrieves the value pointed to by the non-const \a iterator and stores it
    in the memory location pointed to by \a result, if possible.

    \sa canGetValueAtIterator(), begin(), end()
 */
void QMetaSequence::valueAtIterator(const void *iterator, void *result) const
{
    if (canGetValueAtIterator())
        d()->valueAtIteratorFn(iterator, result);
}

/*!
    Returns \c true if the underlying container can write to the value pointed
    to by a non-const iterator, \c false otherwise.

    \sa hasIterator(), setValueAtIterator()
 */
bool QMetaSequence::canSetValueAtIterator() const
{
    if (auto iface = d())
        return iface->setValueAtIteratorFn;
    return false;
}

/*!
    Writes \a value to the value pointed to by the non-const \a iterator, if
    possible.

    \sa canSetValueAtIterator(), begin(), end()
 */
void QMetaSequence::setValueAtIterator(const void *iterator, const void *value) const
{
    if (canSetValueAtIterator())
        d()->setValueAtIteratorFn(iterator, value);
}

/*!
    Returns \c true if the underlying container can insert a new value, taking
    the location pointed to by a non-const iterator into account.

    \sa hasIterator(), insertValueAtIterator()
 */
bool QMetaSequence::canInsertValueAtIterator() const
{
    if (auto iface = d())
        return iface->insertValueAtIteratorFn;
    return false;
}

/*!
    Inserts \a value into the \a container, if possible, taking the non-const
    \a iterator into account. If \l canInsertValueAtIterator() returns
    \c false, the \a value is not inserted. Else if \l isSortable() returns
    \c true, the value is inserted before the value pointed to by
    \a iterator. Else, the \a value is inserted at an unspecified place or not
    at all. In the latter case, the \a iterator is taken as a hint. If it points
    to the correct place for the \a value, the operation may be faster than a
    \l addValue() without iterator.

    \sa canInsertValueAtIterator(), isSortable(), begin(), end()
 */
void QMetaSequence::insertValueAtIterator(void *container, const void *iterator,
                                          const void *value) const
{
    if (canInsertValueAtIterator())
        d()->insertValueAtIteratorFn(container, iterator, value);
}

/*!
    Returns \c true if the value pointed to by a non-const iterator can be
    erased, \c false otherwise.

    \sa hasIterator(), eraseValueAtIterator()
 */
bool QMetaSequence::canEraseValueAtIterator() const
{
    if (auto iface = d())
        return iface->eraseValueAtIteratorFn;
    return false;
}

/*!
    Erases the value pointed to by the non-const \a iterator from the
    \a container, if possible.

    \sa canEraseValueAtIterator(), begin(), end()
 */
void QMetaSequence::eraseValueAtIterator(void *container, const void *iterator) const
{
    if (canEraseValueAtIterator())
        d()->eraseValueAtIteratorFn(container, iterator);
}

/*!
    Returns \c true if a range between two iterators can be erased from the
    container, \c false otherwise.
 */
bool QMetaSequence::canEraseRangeAtIterator() const
{
    if (auto iface = d())
        return iface->eraseRangeAtIteratorFn;
    return false;
}

/*!
    Erases the range of values between the iterators \a iterator1 and
    \a iterator2 from the \a container, if possible.

    \sa canEraseValueAtIterator(), begin(), end()
 */
void QMetaSequence::eraseRangeAtIterator(void *container, const void *iterator1,
                                         const void *iterator2) const
{
    if (canEraseRangeAtIterator())
        d()->eraseRangeAtIteratorFn(container, iterator1, iterator2);
}


/*!
    Returns \c true if the underlying container can retrieve the value pointed
    to by a const iterator, \c false otherwise.

    \sa hasConstIterator(), valueAtConstIterator()
 */
bool QMetaSequence::canGetValueAtConstIterator() const
{
    if (auto iface = d())
        return iface->valueAtConstIteratorFn;
    return false;
}

/*!
    Retrieves the value pointed to by the const \a iterator and stores it
    in the memory location pointed to by \a result, if possible.

    \sa canGetValueAtConstIterator(), constBegin(), constEnd()
 */
void QMetaSequence::valueAtConstIterator(const void *iterator, void *result) const
{
    if (canGetValueAtConstIterator())
        d()->valueAtConstIteratorFn(iterator, result);
}

/*!
    \fn bool QMetaSequence::operator==(const QMetaSequence &lhs, const QMetaSequence &rhs)
    \since 6.0

    Returns \c true if the QMetaSequence \a lhs represents the same container type
    as the QMetaSequence \a rhs, otherwise returns \c false.
*/

/*!
    \fn bool QMetaSequence::operator!=(const QMetaSequence &lhs, const QMetaSequence &rhs)
    \since 6.0

    Returns \c true if the QMetaSequence \a lhs represents a different container
    type than the QMetaSequence \a rhs, otherwise returns \c false.
*/

/*!
    \class QMetaSequence::Iterable
    \inherits QIterable
    \since 6.11
    \inmodule QtCore
    \brief The QMetaSequence::Iterable class is an iterable interface for a container in a QVariant.

    This class allows several methods of accessing the values of a container
    held within a QVariant. An instance of QMetaSequence::Iterable can be
    extracted from a QVariant if it can be converted to a QVariantList, or if
    the container it contains is registered using
    Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE. Most sequential containers found
    in Qt and some found in the C++ standard library are automatically
    registered.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \typealias QMetaSequence::Iterable::RandomAccessIterator
    Exposes an iterator using std::random_access_iterator_tag.
*/

/*!
    \typealias QMetaSequence::Iterable::BidirectionalIterator
    Exposes an iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typealias QMetaSequence::Iterable::ForwardIterator
    Exposes an iterator using std::forward_iterator_tag.
*/

/*!
    \typealias QMetaSequence::Iterable::InputIterator
    Exposes an iterator using std::input_iterator_tag.
*/

/*!
    \typealias QMetaSequence::Iterable::RandomAccessConstIterator
    Exposes a const_iterator using std::random_access_iterator_tag.
*/

/*!
    \typealias QMetaSequence::Iterable::BidirectionalConstIterator
    Exposes a const_iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typealias QMetaSequence::Iterable::ForwardConstIterator
    Exposes a const_iterator using std::forward_iterator_tag.
*/

/*!
    \typealias QMetaSequence::Iterable::InputConstIterator
    Exposes a const_iterator using std::input_iterator_tag.
*/

/*!
    \enum QMetaSequence::Iterable::Position
    \deprecated [6.11] Use append(), prepend(), removeFirst(), or removeLast()

    Specifies the position at which an element shall be added to or removed from
    the iterable.

    \value AtBegin
        Add or remove at the beginning of the iterable.
    \value AtEnd
        Add or remove at the end of the iterable.
    \value Unspecified
        Add or remove at an unspecified position in the iterable.
 */

/*!
    \fn void QMetaSequence::Iterable::addValue(const QVariant &value, Position position)
    \deprecated [6.11] Use append() or prepend()
    Adds \a value to the container, at \a position, if possible.
 */

/*!
    \deprecated [6.11] Use removeFirst() or removeLast()
    \fn void QMetaSequence::Iterable::removeValue(Position position)
    Removes a value from the container, at \a position, if possible.
 */

/*!
    \deprecated [6.11] Use QMetaSequence::valueMetaType()
    \fn QMetaType QMetaSequence::Iterable::valueMetaType() const
    Returns the meta type for values stored in the underlying container.
 */

/*!
    \fn QVariant QMetaSequence::Iterable::at(qsizetype idx) const
    Returns the value at position \a idx in the container.

    \note If the underlying container does not provide a native way to retrieve
          an element at an index, this method will synthesize the access using
          iterators. This behavior is deprecated and will be removed in a future
          version of Qt.
*/

/*!
    \fn void QMetaSequence::Iterable::setAt(qsizetype idx, const QVariant &value)
    Sets the element at position \a idx in the container to \a value.
*/

/*!
    \class QMetaSequence::Iterable::ConstIterator
    \inmodule QtCore
    \inherits QConstIterator
    \since 6.11
    \brief QMetaSequence::Iterable::ConstIterator allows iteration over a container in a QVariant.

    A QMetaSequence::Iterable::ConstIterator can only be created by a
    QMetaSequence::Iterable instance, and can be used in a way similar to other
    stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 9
*/

/*!
    \class QMetaSequence::Iterable::Iterator
    \inmodule QtCore
    \inherits QIterator
    \since 6.11
    \brief QMetaSequence::Iterable::Iterator allows iteration over a container in a QVariant.

    A QMetaSequence::Iterable::Iterator can only be created by a QMetaSequence::Iterable
    instance, and can be used in a way similar to other stl-style iterators.
*/

/*!
    \fn QVariant::Reference<QMetaSequence::Iterable::Iterator> QMetaSequence::Iterable::Iterator::operator*() const
    Returns the current item, converted to a QVariant::Reference.
*/

/*!
    \fn QVariant::Pointer<QMetaSequence::Iterable::Iterator> QMetaSequence::Iterable::Iterator::operator->() const
    Returns the current item, converted to a QVariant::Pointer.
*/

/*!
    \fn QVariant::Reference<QMetaSequence::Iterable::Iterator> QMetaSequence::Iterable::Iterator::operator[](qsizetype n) const
    Returns the item offset from the current one by \a n, converted to a
    QVariant::Reference.
*/

/*!
    \fn QVariant QMetaSequence::Iterable::ConstIterator::operator*() const
    Returns the current item, converted to a QVariant.
*/

/*!
    \fn QVariant::ConstPointer<QMetaSequence::Iterable::ConstIterator> QMetaSequence::Iterable::ConstIterator::operator->() const
    Returns the current item, converted to a QVariant::ConstPointer.
*/

/*!
    \fn QVariant QMetaSequence::Iterable::ConstIterator::operator[](qsizetype n) const
    Returns the item offset from the current one by \a n, converted to a
    QVariant.
*/

QT_END_NAMESPACE
