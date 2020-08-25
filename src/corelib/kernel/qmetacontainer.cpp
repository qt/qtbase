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

#include "qmetacontainer.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMetaSequence
    \inmodule QtCore
    \since 6.0
    \brief The QMetaSequence class allows type erased access to sequential containers.

    \ingroup objectmodel

    The class provides a number of primitive container operations, using void*
    as operands. This way, you can manipulate a generic container retrieved from
    a Variant without knowing its type.

    The void* arguments to the various methods are typically created by using
    a \l QVariant of the respective container or element type, and calling
    its \l QVariant::data() or \l QVariant::constData() methods. However, you
    can also pass plain pointers to objects of the container or element type.

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
    Returns \c true if the underlying container provides at least a forward
    iterator as defined by std::forward_iterator_tag, otherwise returns
    \c false. Bi-directional iterators and random access iterators are
    specializations of forward iterators. This method will also return
    \c true if the container provides one of those.

    QMetaSequence assumes that const and non-const iterators for the same
    container have the same iterator traits.
 */
bool QMetaSequence::hasForwardIterator() const
{
    if (!d_ptr)
        return false;
    return d_ptr->iteratorCapabilities & QtMetaContainerPrivate::ForwardCapability;
}

/*!
    Returns \c true if the underlying container provides a bi-directional
    iterator or a random access iterator as defined by
    std::bidirectional_iterator_tag and std::random_access_iterator_tag,
    respectively. Otherwise returns \c false.

    QMetaSequence assumes that const and non-const iterators for the same
    container have the same iterator traits.
 */
bool QMetaSequence::hasBidirectionalIterator() const
{
    if (!d_ptr)
        return false;
    return d_ptr->iteratorCapabilities & QtMetaContainerPrivate::BiDirectionalCapability;
}

/*!
    Returns \c true if the underlying container provides a random access
    iterator as defined by std::random_access_iterator_tag, otherwise returns
    \c false.

    QMetaSequence assumes that const and non-const iterators for the same
    container have the same iterator traits.
 */
bool QMetaSequence::hasRandomAccessIterator() const
{
    if (!d_ptr)
        return false;
    return d_ptr->iteratorCapabilities & QtMetaContainerPrivate::RandomAccessCapability;
}

/*!
    Returns the meta type for values stored in the container.
 */
QMetaType QMetaSequence::valueMetaType() const
{
    return d_ptr ? d_ptr->valueMetaType : QMetaType();
}

/*!
    Returns \c true if the underlying container is ordered, otherwise returns
    \c false. A container is considered ordered if elements added to it are
    placed in a defined location. Inserting into or adding to an ordered
    container will always succeed. Inserting into or adding to an unordered
    container may not succeed, for example if the container is a QSet that
    already contains the value being inserted.

    \sa addElement(), insertElementAtIterator(), addsAndRemovesElementsAtBegin(),
        addsAndRemovesElementsAtEnd()
 */
bool QMetaSequence::isOrdered() const
{
    if (!d_ptr)
        return false;
    return d_ptr->addRemovePosition != QtMetaContainerPrivate::QMetaSequenceInterface::Random;
}

/*!
    Returns \c true if elements added using \l addElement() are placed at the
    beginning of the container, otherwise returns \c false. Likewise
    \l removeElement() removes an element from the beginning of the container
    if this method returns \c true.

    \sa addElement(), removeElement(), addsAndRemovesElementsAtEnd()
 */
bool QMetaSequence::addsAndRemovesElementsAtBegin() const
{
    if (!d_ptr)
        return false;
    return d_ptr->addRemovePosition == QtMetaContainerPrivate::QMetaSequenceInterface::AtBegin;
}

/*!
    Returns \c true if elements added using \l addElement() are placed at the
    end of the container, otherwise returns \c false. Likewise
    \l removeElement() removes an element from the end of the container
    if this method returns \c true.

    \sa addElement(), removeElement(), addsAndRemovesElementsAtBegin()
 */
bool QMetaSequence::addsAndRemovesElementsAtEnd() const
{
    if (!d_ptr)
        return false;
    return d_ptr->addRemovePosition == QtMetaContainerPrivate::QMetaSequenceInterface::AtEnd;
}

/*!
    Returns \c true if the container can be queried for its size, \c false
    otherwise.

    \sa size()
 */
bool QMetaSequence::hasSize() const
{
    return d_ptr && d_ptr->sizeFn;
}

/*!
    Returns the number of elements in the given \a container if it can be
    queried for its size. Otherwise returns \c -1.

    \sa hasSize()
 */
qsizetype QMetaSequence::size(const void *container) const
{
    return hasSize() ? d_ptr->sizeFn(container) : -1;
}

/*!
    Returns \c true if the container can be cleared, \c false otherwise.

    \sa clear()
 */
bool QMetaSequence::canClear() const
{
    return d_ptr && d_ptr->clearFn;
}

/*!
    Clears the given \a container if it can be cleared.

    \sa canClear()
 */
void QMetaSequence::clear(void *container) const
{
    if (canClear())
        d_ptr->clearFn(container);
}

/*!
    Returns \c true if elements can be retrieved from the container by index,
    otherwise \c false.

    \sa elementAtIndex()
 */
bool QMetaSequence::canGetElementAtIndex() const
{
    return d_ptr && d_ptr->elementAtIndexFn;
}

/*!
    Retrieves the element at \a index in the \a container and places it in the
    memory location pointed to by \a result, if that is possible.

    \sa canGetElementAtIndex()
 */
void QMetaSequence::elementAtIndex(const void *container, qsizetype index, void *result) const
{
    if (canGetElementAtIndex())
        d_ptr->elementAtIndexFn(container, index, result);
}

/*!
    Returns \c true if an element can be written to the container by index,
    otherwise \c false.

    \sa setElementAtIndex()
*/
bool QMetaSequence::canSetElementAtIndex() const
{
    return d_ptr && d_ptr->setElementAtIndexFn;
}

/*!
    Overwrites the element at \a index in the \a container using the \a element
    passed as parameter if that is possible.

    \sa canSetElementAtIndex()
 */
void QMetaSequence::setElementAtIndex(void *container, qsizetype index, const void *element) const
{
    if (canSetElementAtIndex())
        d_ptr->setElementAtIndexFn(container, index, element);
}

/*!
    Returns \c true if elements can be added to the container, \c false
    otherwise.

    \sa addElement(), isOrdered()
 */
bool QMetaSequence::canAddElement() const
{
    return d_ptr && d_ptr->addElementFn;
}

/*!
    Adds \a element to the \a container if possible. If \l canAddElement()
    returns \c false, the \a element is not added. Else, if
    \l addsAndRemovesElementsAtBegin() returns \c true, the \a element is added
    to the beginning of the \a container. Else, if
    \l addsAndRemovesElementsAtEnd() returns \c true, the \a element is added to
    the end of the container. Else, the element is added in an unspecified
    place or not at all. The latter is the case for adding elements to an
    unordered container, for example \l QSet.

    \sa canAddElement(), addsAndRemovesElementsAtBegin(),
        addsAndRemovesElementsAtEnd(), isOrdered(), removeElement()
 */
void QMetaSequence::addElement(void *container, const void *element) const
{
    if (canAddElement())
        d_ptr->addElementFn(container, element);
}

/*!
    Returns \c true if elements can be removed from the container, \c false
    otherwise.

    \sa removeElement(), isOrdered()
 */
bool QMetaSequence::canRemoveElement() const
{
    return d_ptr && d_ptr->removeElementFn;
}

/*!
    Removes an element from the \a container if possible. If
    \l canRemoveElement() returns \c false, no element is removed. Else, if
    \l addsAndRemovesElementsAtBegin() returns \c true, the first element in
    the \a container is removed. Else, if \l addsAndRemovesElementsAtEnd()
    returns \c true, the last element in the \a container is removed. Else,
    an unspecified element or nothing is removed.

    \sa canRemoveElement(), addsAndRemovesElementsAtBegin(),
        addsAndRemovesElementsAtEnd(), isOrdered(), addElement()
 */
void QMetaSequence::removeElement(void *container) const
{
    if (canRemoveElement())
        d_ptr->removeElementFn(container);
}

/*!
    Returns \c true if the underlying container offers a non-const iterator,
    \c false otherwise.

    \sa begin(), end(), destroyIterator(), compareIterator(), diffIterator(),
        advanceIterator(), copyIterator()
 */
bool QMetaSequence::hasIterator() const
{
    if (!d_ptr || !d_ptr->createIteratorFn)
        return false;
    Q_ASSERT(d_ptr->destroyIteratorFn);
    Q_ASSERT(d_ptr->compareIteratorFn);
    Q_ASSERT(d_ptr->copyIteratorFn);
    Q_ASSERT(d_ptr->advanceIteratorFn);
    Q_ASSERT(d_ptr->diffIteratorFn);
    return true;
}

/*!
    Creates and returns a non-const iterator pointing to the beginning of
    \a container. The iterator is allocated on the heap using new. It has to be
    destroyed using \l destroyIterator eventually, to reclaim the memory.

    Returns \c nullptr if the container doesn't offer any non-const iterators.

    \sa end(), constBegin(), constEnd(), destroyIterator()
 */
void *QMetaSequence::begin(void *container) const
{
    return hasIterator()
            ? d_ptr->createIteratorFn(
                  container, QtMetaContainerPrivate::QMetaSequenceInterface::AtBegin)
            : nullptr;
}

/*!
    Creates and returns a non-const iterator pointing to the end of
    \a container. The iterator is allocated on the heap using new. It has to be
    destroyed using \l destroyIterator eventually, to reclaim the memory.

    Returns \c nullptr if the container doesn't offer any non-const iterators.

    \sa hasIterator(), end(), constBegin(), constEnd(), destroyIterator()
 */
void *QMetaSequence::end(void *container) const
{
    return hasIterator()
            ? d_ptr->createIteratorFn(
                  container, QtMetaContainerPrivate::QMetaSequenceInterface::AtEnd)
            : nullptr;
}

/*!
    Destroys a non-const \a iterator previously created using \l begin() or
    \l end().

    \sa begin(), end(), destroyConstIterator()
 */
void QMetaSequence::destroyIterator(const void *iterator) const
{
    if (hasIterator())
        d_ptr->destroyIteratorFn(iterator);
}

/*!
    Returns \c true if the non-const iterators \a i and \a j point to the same
    element in the container they are iterating over, otherwise returns \c
    false.

    \sa begin(), end()
 */
bool QMetaSequence::compareIterator(const void *i, const void *j) const
{
    return hasIterator() ? d_ptr->compareIteratorFn(i, j) : false;
}

/*!
    Copies the non-const iterator \a source into the non-const iterator
    \a target. Afterwards compareIterator(target, source) returns \c true.

    \sa begin(), end()
 */
void QMetaSequence::copyIterator(void *target, const void *source) const
{
    if (hasIterator())
        d_ptr->copyIteratorFn(target, source);
}

/*!
    Advances the non-const \a iterator by \a step steps. If \a steps is negative
    the \a iterator is moved backwards, towards the beginning of the container.
    The behavior is unspecified for negative values of \a step if
    \l hasBidirectionalIterator() returns false.

    \sa begin(), end()
 */
void QMetaSequence::advanceIterator(void *iterator, qsizetype step) const
{
    if (hasIterator())
        d_ptr->advanceIteratorFn(iterator, step);
}

/*!
    Returns the distance between the non-const iterators \a i and \a j, the
    equivalent of \a i \c - \a j. If \a j is closer to the end of the container
    than \a i, the returned value is negative. The behavior is unspecified in
    this case if \l hasBidirectionalIterator() returns false.

    \sa begin(), end()
 */
qsizetype QMetaSequence::diffIterator(const void *i, const void *j) const
{
    return hasIterator() ? d_ptr->diffIteratorFn(i, j) : 0;
}

/*!
    Returns \c true if the underlying container can retrieve the value pointed
    to by a non-const iterator, \c false otherwise.

    \sa hasIterator(), elementAtIterator()
 */
bool QMetaSequence::canGetElementAtIterator() const
{
    return d_ptr && d_ptr->elementAtIteratorFn;
}

/*!
    Retrieves the element pointed to by the non-const \a iterator and stores it
    in the memory location pointed to by \a result, if possible.

    \sa canGetElementAtIterator(), begin(), end()
 */
void QMetaSequence::elementAtIterator(const void *iterator, void *result) const
{
    if (canGetElementAtIterator())
        d_ptr->elementAtIteratorFn(iterator, result);
}

/*!
    Returns \c true if the underlying container can write to the value pointed
    to by a non-const iterator, \c false otherwise.

    \sa hasIterator(), setElementAtIterator()
 */
bool QMetaSequence::canSetElementAtIterator() const
{
    return d_ptr && d_ptr->setElementAtIteratorFn;
}

/*!
    Writes \a element to the value pointed to by the non-const \a iterator, if
    possible.

    \sa canSetElementAtIterator(), begin(), end()
 */
void QMetaSequence::setElementAtIterator(const void *iterator, const void *element) const
{
    if (canSetElementAtIterator())
        d_ptr->setElementAtIteratorFn(iterator, element);
}

/*!
    Returns \c true if the underlying container can insert a new element, taking
    the location pointed to by a non-const iterator into account.

    \sa hasIterator(), insertElementAtIterator()
 */
bool QMetaSequence::canInsertElementAtIterator() const
{
    return d_ptr && d_ptr->insertElementAtIteratorFn;
}

/*!
    Inserts \a element into the \a container, if possible, taking the non-const
    \a iterator into account. If \l canInsertElementAtIterator() returns
    \c false, the \a element is not inserted. Else if \l isOrdered() returns
    \c true, the element is inserted before the element pointed to by
    \a iterator. Else, the \a element is inserted at an unspecified place or not
    at all. In the latter case, the \a iterator is taken as a hint. If it points
    to the correct place for the \a element, the operation may be faster than a
    \l addElement() without iterator.

    \sa canInsertElementAtIterator(), isOrdered(), begin(), end()
 */
void QMetaSequence::insertElementAtIterator(void *container, const void *iterator,
                                            const void *element) const
{
    if (canInsertElementAtIterator())
        d_ptr->insertElementAtIteratorFn(container, iterator, element);
}

/*!
    Returns \c true if the element pointed to by a non-const iterator can be
    erased, \c false otherwise.

    \sa hasIterator(), eraseElementAtIterator()
 */
bool QMetaSequence::canEraseElementAtIterator() const
{
    return d_ptr && d_ptr->eraseElementAtIteratorFn;
}

/*!
    Erases the element pointed to by the non-const \a iterator from the
    \a container, if possible.

    \sa canEraseElementAtIterator(), begin(), end()
 */
void QMetaSequence::eraseElementAtIterator(void *container, const void *iterator) const
{
    if (canEraseElementAtIterator())
        d_ptr->eraseElementAtIteratorFn(container, iterator);
}

/*!
    Returns \c true if the underlying container offers a const iterator,
    \c false otherwise.

    \sa constBegin(), constEnd(), destroyConstIterator(),
        compareConstIterator(), diffConstIterator(), advanceConstIterator(),
        copyConstIterator()
 */
bool QMetaSequence::hasConstIterator() const
{
    if (!d_ptr || !d_ptr->createConstIteratorFn)
        return false;
    Q_ASSERT(d_ptr->destroyConstIteratorFn);
    Q_ASSERT(d_ptr->compareConstIteratorFn);
    Q_ASSERT(d_ptr->copyConstIteratorFn);
    Q_ASSERT(d_ptr->advanceConstIteratorFn);
    Q_ASSERT(d_ptr->diffConstIteratorFn);
    return true;
}

/*!
    Creates and returns a const iterator pointing to the beginning of
    \a container. The iterator is allocated on the heap using new. It has to be
    destroyed using \l destroyConstIterator eventually, to reclaim the memory.

    Returns \c nullptr if the container doesn't offer any const iterators.

    \sa constEnd(), begin(), end(), destroyConstIterator()
 */
void *QMetaSequence::constBegin(const void *container) const
{
    return hasConstIterator()
            ? d_ptr->createConstIteratorFn(
                  container, QtMetaContainerPrivate::QMetaSequenceInterface::AtBegin)
            : nullptr;
}

/*!
    Creates and returns a const iterator pointing to the end of
    \a container. The iterator is allocated on the heap using new. It has to be
    destroyed using \l destroyConstIterator eventually, to reclaim the memory.

    Returns \c nullptr if the container doesn't offer any const iterators.

    \sa constBegin(), begin(), end(), destroyConstIterator()
 */
void *QMetaSequence::constEnd(const void *container) const
{
    return hasConstIterator()
            ? d_ptr->createConstIteratorFn(
                  container, QtMetaContainerPrivate::QMetaSequenceInterface::AtEnd)
            : nullptr;
}

/*!
    Destroys a const \a iterator previously created using \l constBegin() or
    \l constEnd().

    \sa constBegin(), constEnd(), destroyIterator()
 */
void QMetaSequence::destroyConstIterator(const void *iterator) const
{
    if (hasConstIterator())
        d_ptr->destroyConstIteratorFn(iterator);
}

/*!
    Returns \c true if the const iterators \a i and \a j point to the same
    element in the container they are iterating over, otherwise returns \c
    false.

    \sa constBegin(), constEnd()
 */
bool QMetaSequence::compareConstIterator(const void *i, const void *j) const
{
    return hasConstIterator() ? d_ptr->compareConstIteratorFn(i, j) : false;
}

/*!
    Copies the const iterator \a source into the const iterator
    \a target. Afterwards compareConstIterator(target, source) returns \c true.

    \sa constBegin(), constEnd()
 */
void QMetaSequence::copyConstIterator(void *target, const void *source) const
{
    if (hasConstIterator())
        d_ptr->copyConstIteratorFn(target, source);
}

/*!
    Advances the const \a iterator by \a step steps. If \a steps is negative
    the \a iterator is moved backwards, towards the beginning of the container.
    The behavior is unspecified for negative values of \a step if
    \l hasBidirectionalIterator() returns false.

    \sa constBegin(), constEnd()
 */
void QMetaSequence::advanceConstIterator(void *iterator, qsizetype step) const
{
    if (hasConstIterator())
        d_ptr->advanceConstIteratorFn(iterator, step);
}

/*!
    Returns the distance between the const iterators \a i and \a j, the
    equivalent of \a i \c - \a j. If \a j is closer to the end of the container
    than \a i, the returned value is negative. The behavior is unspecified in
    this case if \l hasBidirectionalIterator() returns false.

    \sa constBegin(), constEnd()
 */
qsizetype QMetaSequence::diffConstIterator(const void *i, const void *j) const
{
    return hasConstIterator() ?  d_ptr->diffConstIteratorFn(i, j) : 0;
}

/*!
    Returns \c true if the underlying container can retrieve the value pointed
    to by a const iterator, \c false otherwise.

    \sa hasConstIterator(), elementAtConstIterator()
 */
bool QMetaSequence::canGetElementAtConstIterator() const
{
    return d_ptr && d_ptr->elementAtConstIteratorFn;
}

/*!
    Retrieves the element pointed to by the const \a iterator and stores it
    in the memory location pointed to by \a result, if possible.

    \sa canGetElementAtConstIterator(), constBegin(), constEnd()
 */
void QMetaSequence::elementAtConstIterator(const void *iterator, void *result) const
{
    if (canGetElementAtConstIterator())
        d_ptr->elementAtConstIteratorFn(iterator, result);
}

/*!
    \fn bool operator==(QMetaSequence a, QMetaSequence b)
    \since 6.0
    \relates QMetaSequence

    Returns \c true if the QMetaSequence \a a represents the same container type
    as the QMetaSequence \a b, otherwise returns \c false.
*/

/*!
    \fn bool operator!=(QMetaSequence a, QMetaSequence b)
    \since 6.0
    \relates QMetaSequence

    Returns \c true if the QMetaSequence \a a represents a different container
    type than the QMetaSequence \a b, otherwise returns \c false.
*/

QT_END_NAMESPACE
