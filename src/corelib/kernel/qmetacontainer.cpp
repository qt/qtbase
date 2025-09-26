// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmetacontainer.h"
#include "qmetatype.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMetaContainer
    \inmodule QtCore
    \since 6.0
    \brief The QMetaContainer class provides common functionality for sequential
        and associative containers.

    QMetaContainer is part of Qt's meta-type system that allows type-erased access
    to container-like types at runtime.

    It serves as a common base for accessing properties of containers in a generic
    way, such as size, iteration, and clearing operations, without knowing the actual
    container type.

    Derived classes, such as QMetaSequence, provide specialized interfaces for
    sequential containers.

    \ingroup objectmodel

    \compares equality
*/

/*!
    Returns \c true if the underlying container provides at least an input
    iterator as defined by std::input_iterator_tag, otherwise returns
    \c false. Forward, Bi-directional, and random access iterators are
    specializations of input iterators. This method will also return
    \c true if the container provides one of those.

    QMetaContainer assumes that const and non-const iterators for the same
    container have the same iterator traits.
 */
bool QMetaContainer::hasInputIterator() const
{
    if (!d_ptr)
        return false;
    return d_ptr->iteratorCapabilities.testAnyFlag(QtMetaContainerPrivate::InputCapability);
}

/*!
    Returns \c true if the underlying container provides at least a forward
    iterator as defined by std::forward_iterator_tag, otherwise returns
    \c false. Bi-directional iterators and random access iterators are
    specializations of forward iterators. This method will also return
    \c true if the container provides one of those.

    QMetaContainer assumes that const and non-const iterators for the same
    container have the same iterator traits.
 */
bool QMetaContainer::hasForwardIterator() const
{
    if (!d_ptr)
        return false;
    return d_ptr->iteratorCapabilities.testAnyFlag(QtMetaContainerPrivate::ForwardCapability);
}

/*!
    Returns \c true if the underlying container provides a bi-directional
    iterator or a random access iterator as defined by
    std::bidirectional_iterator_tag and std::random_access_iterator_tag,
    respectively. Otherwise returns \c false.

    QMetaContainer assumes that const and non-const iterators for the same
    container have the same iterator traits.
 */
bool QMetaContainer::hasBidirectionalIterator() const
{
    if (!d_ptr)
        return false;
    return d_ptr->iteratorCapabilities.testAnyFlag(QtMetaContainerPrivate::BiDirectionalCapability);
}

/*!
    Returns \c true if the underlying container provides a random access
    iterator as defined by std::random_access_iterator_tag, otherwise returns
    \c false.

    QMetaContainer assumes that const and non-const iterators for the same
    container have the same iterator traits.
 */
bool QMetaContainer::hasRandomAccessIterator() const
{
    if (!d_ptr)
        return false;
    return d_ptr->iteratorCapabilities.testAnyFlag(QtMetaContainerPrivate::RandomAccessCapability);
}

/*!
    Returns \c true if the container can be queried for its size, \c false
    otherwise.

    \sa size()
 */
bool QMetaContainer::hasSize() const
{
    return d_ptr && d_ptr->sizeFn;
}

/*!
    Returns the number of values in the given \a container if it can be
    queried for its size. Otherwise returns \c -1.

    \sa hasSize()
 */
qsizetype QMetaContainer::size(const void *container) const
{
    return hasSize() ? d_ptr->sizeFn(container) : -1;
}

/*!
    Returns \c true if the container can be cleared, \c false otherwise.

    \sa clear()
 */
bool QMetaContainer::canClear() const
{
    return d_ptr && d_ptr->clearFn;
}

/*!
    Clears the given \a container if it can be cleared.

    \sa canClear()
 */
void QMetaContainer::clear(void *container) const
{
    if (canClear())
        d_ptr->clearFn(container);
}

/*!
    Returns \c true if the underlying container offers a non-const iterator,
    \c false otherwise.

    \sa begin(), end(), destroyIterator(), compareIterator(), diffIterator(),
        advanceIterator(), copyIterator()
 */
bool QMetaContainer::hasIterator() const
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
void *QMetaContainer::begin(void *container) const
{
    return hasIterator()
            ? d_ptr->createIteratorFn(
                  container, QtMetaContainerPrivate::QMetaContainerInterface::AtBegin)
            : nullptr;
}

/*!
    Creates and returns a non-const iterator pointing to the end of
    \a container. The iterator is allocated on the heap using new. It has to be
    destroyed using \l destroyIterator eventually, to reclaim the memory.

    Returns \c nullptr if the container doesn't offer any non-const iterators.

    \sa hasIterator(), begin(), constBegin(), constEnd(), destroyIterator()
 */
void *QMetaContainer::end(void *container) const
{
    return hasIterator()
            ? d_ptr->createIteratorFn(
                  container, QtMetaContainerPrivate::QMetaContainerInterface::AtEnd)
            : nullptr;
}

/*!
    Destroys a non-const \a iterator previously created using \l begin() or
    \l end().

    \sa begin(), end(), destroyConstIterator()
 */
void QMetaContainer::destroyIterator(const void *iterator) const
{
    if (hasIterator())
        d_ptr->destroyIteratorFn(iterator);
}

/*!
    Returns \c true if the non-const iterators \a i and \a j point to the same
    value in the container they are iterating over, otherwise returns \c
    false.

    \sa begin(), end()
 */
bool QMetaContainer::compareIterator(const void *i, const void *j) const
{
    return hasIterator() ? d_ptr->compareIteratorFn(i, j) : false;
}

/*!
    Copies the non-const iterator \a source into the non-const iterator
    \a target. Afterwards compareIterator(target, source) returns \c true.

    \sa begin(), end()
 */
void QMetaContainer::copyIterator(void *target, const void *source) const
{
    if (hasIterator())
        d_ptr->copyIteratorFn(target, source);
}

/*!
    Advances the non-const \a iterator by \a step steps. If \a step is negative
    the \a iterator is moved backwards, towards the beginning of the container.
    The behavior is unspecified for negative values of \a step if
    \l hasBidirectionalIterator() returns false.

    \sa begin(), end()
 */
void QMetaContainer::advanceIterator(void *iterator, qsizetype step) const
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
qsizetype QMetaContainer::diffIterator(const void *i, const void *j) const
{
    return hasIterator() ? d_ptr->diffIteratorFn(i, j) : 0;
}

/*!
    Returns \c true if the underlying container offers a const iterator,
    \c false otherwise.

    \sa constBegin(), constEnd(), destroyConstIterator(),
        compareConstIterator(), diffConstIterator(), advanceConstIterator(),
        copyConstIterator()
 */
bool QMetaContainer::hasConstIterator() const
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
void *QMetaContainer::constBegin(const void *container) const
{
    return hasConstIterator()
            ? d_ptr->createConstIteratorFn(
                  container, QtMetaContainerPrivate::QMetaContainerInterface::AtBegin)
            : nullptr;
}

/*!
    Creates and returns a const iterator pointing to the end of
    \a container. The iterator is allocated on the heap using new. It has to be
    destroyed using \l destroyConstIterator eventually, to reclaim the memory.

    Returns \c nullptr if the container doesn't offer any const iterators.

    \sa constBegin(), begin(), end(), destroyConstIterator()
 */
void *QMetaContainer::constEnd(const void *container) const
{
    return hasConstIterator()
            ? d_ptr->createConstIteratorFn(
                  container, QtMetaContainerPrivate::QMetaContainerInterface::AtEnd)
            : nullptr;
}

/*!
    Destroys a const \a iterator previously created using \l constBegin() or
    \l constEnd().

    \sa constBegin(), constEnd(), destroyIterator()
 */
void QMetaContainer::destroyConstIterator(const void *iterator) const
{
    if (hasConstIterator())
        d_ptr->destroyConstIteratorFn(iterator);
}

/*!
    Returns \c true if the const iterators \a i and \a j point to the same
    value in the container they are iterating over, otherwise returns \c
    false.

    \sa constBegin(), constEnd()
 */
bool QMetaContainer::compareConstIterator(const void *i, const void *j) const
{
    return hasConstIterator() ? d_ptr->compareConstIteratorFn(i, j) : false;
}

/*!
    Copies the const iterator \a source into the const iterator
    \a target. Afterwards compareConstIterator(target, source) returns \c true.

    \sa constBegin(), constEnd()
 */
void QMetaContainer::copyConstIterator(void *target, const void *source) const
{
    if (hasConstIterator())
        d_ptr->copyConstIteratorFn(target, source);
}

/*!
    Advances the const \a iterator by \a step steps. If \a step is negative
    the \a iterator is moved backwards, towards the beginning of the container.
    The behavior is unspecified for negative values of \a step if
    \l hasBidirectionalIterator() returns false.

    \sa constBegin(), constEnd()
 */
void QMetaContainer::advanceConstIterator(void *iterator, qsizetype step) const
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
qsizetype QMetaContainer::diffConstIterator(const void *i, const void *j) const
{
    return hasConstIterator() ?  d_ptr->diffConstIteratorFn(i, j) : 0;
}

QT_END_NAMESPACE
