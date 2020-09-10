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

#include <QtCore/qiterable.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBaseIterator

    QBaseIterator forms the common base class for all iterators operating on
    subclasses of QIterable.
*/

/*!
    \fn QBaseIterator::QBaseIterator(const QIterable *iterable, void *iterator)

    \internal
    Creates a const QBaseIterator from an \a iterable and an \a iterator.
 */

/*!
    \fn QBaseIterator::QBaseIterator(QIterable *iterable, void *iterator)

    \internal
    Creates a non-const QBaseIterator from an \a iterable and an \a iterator.
 */

/*!
    \fn QBaseIterator::QBaseIterator(QBaseIterator &&other)

    \internal
    Move-constructs a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \fn QBaseIterator::QBaseIterator(const QBaseIterator &other)

    \internal
    Copy-constructs a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \fn QBaseIterator::~QBaseIterator()

    \internal
    Destroys a QBaseIterator.
 */

/*!
    \fn QBaseIterator &QBaseIterator::operator=(const QBaseIterator &other)

    \internal
    Copy-assigns a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \fn void QBaseIterator::initIterator(const void *copy)

    \internal
    Initializes the internal native iterator by duplicating \a copy, if given.
 */

/*!
    \fn void QBaseIterator::clearIterator()

    \internal
    Destroys the internal native iterator.
 */


/*!
    \fn QMetaContainer QBaseIterator::metaContainer() const

    \internal
    Returns the meta sequence.
 */

/*!
    \fn QIterable *QBaseIterator::mutableIterable() const

    \internal
    Returns a non-const pointer to the iterable, if the original iterable was
    non-const. Otherwise returns nullptr.
 */

/*!
    \fn const QIterable *QBaseIterator::constIterable() const

    \internal
    Returns a const pointer to the iterable.
 */

/*!
    \fn void *QBaseIterator::mutableIterator()

    Returns a non-const pointer to the internal native iterator.
 */

/*!
    \fn const void *QBaseIterator::constIterator() const

    Returns a const pointer to the internal native iterator.
 */

/*!
    \fn QBaseIterator &QBaseIterator::operator=(QBaseIterator &&other)

    \internal
    Move-assigns a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \class Qiterator
    \since 6.0
    \inmodule QtCore
    \brief The QIterator allows iteration over a container in a QVariant.

    A QIterator can only be created by a QIterable instance, and can be used
    in a way similar to other stl-style iterators. Generally, QIterator should
    not be used directly, but through its derived classes provided by
    QSequentialIterable and QAssociativeIterable.

    \sa QIterable
*/

/*!
    \fn QIterator::QIterator(QIterable *iterable, void *iterator)

    Creates an iterator from an \a iterable and a pointer to a native \a iterator.
 */

/*!
    \fn bool QIterator::operator==(const QIterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QIterator::operator!=(const QIterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn QIterator &QIterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QSequentialIterable::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QIterator QIterator::operator++(int)
    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/


/*!
    \fn QIterator &QIterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QSequentialIterable::begin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), canReverseIterate()
*/

/*!
    \fn QIterator QIterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa canReverseIterate()
*/

/*!
    \fn QIterator &QIterator::operator+=(qsizetype j)

    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/

/*!
    \fn QIterator &QIterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), canReverseIterate()
*/

/*!
    \fn QIterator QIterator::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/

/*!
    \fn QIterator QIterator::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), canReverseIterate()
*/

/*!
    \fn qsizetype QIterator::operator-(const QIterator &j) const

    Returns the distance between the two iterators.

    \sa operator+(), operator-=(), canReverseIterator()
 */

/*!
    QConstIterator::QConstIterator(const QIterable *iterable, void *iterator)

    Creates a QConstIterator to wrap \a iterator, operating on \a iterable.
 */

/*!
    bool QConstIterator::operator==(const QConstIterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    bool QConstIterator::operator!=(const QConstIterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn QConstIterator &QConstIterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QIterable::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QConstIterator QConstIterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/

/*!
    \fn QConstIterator &QConstIterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QIterable::begin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), canReverseIterate()
*/

/*!
    \fn QConstIterator QConstIterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa canReverseIterate()
*/

/*!
    \fn QConstIterator &QConstIterator::operator+=(qsizetype j)

    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/

/*!
    \fn QConstIterator &QConstIterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), canReverseIterate()
*/

/*!
    \fn QConstIterator QConstIterator::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/

/*!
    \fn QConstIterator QConstIterator::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), canReverseIterate()
*/

/*!
    \fn qsizetype QConstIterator::operator-(const QConstIterator &j) const

    Returns the distance between the two iterators.

    \sa operator+(), operator-=(), canReverseIterator()
 */

/*!
    \class QIterable
    \since 6.0

    QIterable is the base class for QSequentialIterable and QAssociativeIterable.
*/

/*!
    \fn bool QIterable::canInputIterate() const

    Returns whether the container has an input iterator. This corresponds to
    the std::input_iterator_tag iterator trait of the iterator and
    const_iterator of the container.
*/

/*!
    \fn bool QIterable::canForwardIterate() const

    Returns whether it is possible to iterate over the container in forward
    direction. This corresponds to the std::forward_iterator_tag iterator trait
    of the iterator and const_iterator of the container.
*/

/*!
    \fn bool QIterable::canReverseIterate() const

    Returns whether it is possible to iterate over the container in reverse. This
    corresponds to the std::bidirectional_iterator_tag iterator trait of the
    const_iterator of the container.
*/

/*!
    \fn bool QIterable::canRandomAccessIterate() const

    Returns whether it is possible to efficiently skip over multiple values
    using and iterator. This corresponds to the std::random_access_iterator_tag
    iterator trait of the iterator and const_iterator of the container.
*/

/*!
    \fn QConstIterator QIterable::constBegin() const

    Returns a QConstIterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa end()
*/

/*!
    \fn QConstIterator QIterable::constEnd() const

    Returns a Qterable::QConstIterator for the end of the container. This
    can be used in stl-style iteration.

    \sa begin()
*/

/*!
    \fn QIterator QIterable::mutableBegin()

    Returns a QIterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa end()
*/

/*!
    \fn QIterator QIterable::mutableEnd()

    Returns a QSequentialIterable::iterator for the end of the container. This
    can be used in stl-style iteration.

    \sa begin()
*/

/*!
    \fn qsizetype QIterable::size() const

    Returns the number of values in the container.
*/

/*!
    \class QTaggedIterator
    \since 6.0
    \inmodule QtCore
    \brief Wraps an iterator and exposes standard iterator traits.

    In order to use an iterator any of the standard algorithms, it iterator
    traits need to be known. As QSequentialIterable can work with many different
    kinds of containers, we cannot declare the traits in the iterator classes
    themselves. StdIterator gives you a way to explicitly declare a trait for
    a concrete instance of an iterator or QConstIterator.
*/

/*!
    \fn QTaggedIterator::StdIterator(Iterator &&it)

    Constructs a StdIterator from an iterator or QConstIterator \a it. Checks
    whether the IteratorCategory passed as template argument matches the run
    time capabilities of \a it and refuses \a it if not.
*/

/*!
    \fn bool QTaggedIterator::operator==(const StdIterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QTaggedIterator::operator!=(const StdIterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn QTaggedIterator &QTaggedIterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QSequentialIterable::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QTaggedIterator QTaggedIterator::operator++(int)
    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/


/*!
    \fn QTaggedIterator &QTaggedIterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QSequentialIterable::begin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), canReverseIterate()
*/

/*!
    \fn QTaggedIterator QTaggedIterator::operator--(int)
    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa canReverseIterate()
*/


/*!
    \fn QTaggedIterator &QTaggedIterator::operator+=(qsizetype j)

    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/

/*!
    \fn QTaggedIterator &QTaggedIterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), canReverseIterate()
*/

/*!
    \fn QTaggedIterator QTaggedIterator::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/

/*!
    \fn QTaggedIterator QTaggedIterator::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), canReverseIterate()
*/

/*!
    \fn qsizetype QTaggedIterator::operator-(const QTaggedIterator &j) const

    Returns the distance between the two iterators.

    \sa operator+(), operator-=(), canReverseIterator()
*/

QT_END_NAMESPACE
