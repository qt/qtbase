// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qiterable.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBaseIterator
    \inmodule QtCore
    QBaseIterator forms the common base class for all iterators operating on
    subclasses of QIterable.
*/

/*!
    \fn template<class Container> QBaseIterator<Container>::QBaseIterator(const QIterable<Container> *iterable, void *iterator)

    \internal
    Creates a const QBaseIterator from an \a iterable and an \a iterator.
 */

/*!
    \fn template<class Container> QBaseIterator<Container>::QBaseIterator(QIterable<Container> *iterable, void *iterator)

    \internal
    Creates a non-const QBaseIterator from an \a iterable and an \a iterator.
 */

/*!
    \fn template<class Container> QBaseIterator<Container>::QBaseIterator(QBaseIterator<Container> &&other)

    \internal
    Move-constructs a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \fn template<class Container> QBaseIterator<Container>::QBaseIterator(const QBaseIterator<Container> &other)

    \internal
    Copy-constructs a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \fn template<class Container> QBaseIterator<Container>::~QBaseIterator()

    \internal
    Destroys a QBaseIterator.
 */

/*!
    \fn template<class Container> QBaseIterator<Container> &QBaseIterator<Container>::operator=(const QBaseIterator<Container> &other)

    \internal
    Copy-assigns a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \fn template<class Container> void QBaseIterator<Container>::initIterator(const void *copy)

    \internal
    Initializes the internal native iterator by duplicating \a copy, if given.
 */

/*!
    \fn template<class Container> void QBaseIterator<Container>::clearIterator()

    \internal
    Destroys the internal native iterator.
 */


/*!
    \fn QMetaContainer QBaseIterator<Container>::metaContainer() const

    \internal
    Returns the meta sequence.
 */

/*!
    \fn template<class Container> QIterable *QBaseIterator<Container>::mutableIterable() const

    \internal
    Returns a non-const pointer to the iterable, if the original iterable was
    non-const. Otherwise returns nullptr.
 */

/*!
    \fn template<class Container> const QIterable *QBaseIterator<Container>::constIterable() const

    \internal
    Returns a const pointer to the iterable.
 */

/*!
    \fn template<class Container> void *QBaseIterator<Container>::mutableIterator()

    Returns a non-const pointer to the internal native iterator.
 */

/*!
    \fn template<class Container> const void *QBaseIterator<Container>::constIterator() const

    Returns a const pointer to the internal native iterator.
 */

/*!
    \fn template<class Container> QBaseIterator &QBaseIterator<Container>::operator=(QBaseIterator<Container> &&other)

    \internal
    Move-assigns a QBaseIterator from \a other, preserving its const-ness.
 */

/*!
    \class QIterator
    \since 6.0
    \inmodule QtCore
    \brief The QIterator is a template class that allows iteration over a container in a QVariant.

    A QIterator can only be created by a QIterable instance, and can be used
    in a way similar to other stl-style iterators. Generally, QIterator should
    not be used directly, but through its derived classes provided by
    QSequentialIterable and QAssociativeIterable.

    \sa QIterable
*/

/*!
    \fn template<class Container> QIterator<Container>::QIterator(QIterable<Container> *iterable, void *iterator)

    Creates an iterator from an \a iterable and a pointer to a native \a iterator.
 */

/*!
    \fn template<class Container> bool QIterator<Container>::operator==(const QIterator<Container> &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template<class Container> bool QIterator<Container>::operator!=(const QIterator<Container> &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template<class Container> QIterator<Container> &QIterator<Container>::operator++()

    The prefix \c{++} operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QSequentialIterable::constEnd() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn template<class Container> QIterator<Container> QIterator<Container>::operator++(int)
    \overload

    The postfix \c{++} operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/


/*!
    \fn template<class Container> QIterator<Container> &QIterator<Container>::operator--()

    The prefix \c{--} operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QSequentialIterable::constBegin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), QIterable::canReverseIterate()
*/

/*!
    \fn template<class Container> QIterator<Container> QIterator<Container>::operator--(int)

    \overload

    The postfix \c{--} operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa QIterable::canReverseIterate()
*/

/*!
    \fn template<class Container> QIterator<Container> &QIterator<Container>::operator+=(qsizetype j)

    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/

/*!
    \fn template<class Container> QIterator<Container> &QIterator<Container>::operator-=(qsizetype j)

    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), QIterable::canReverseIterate()
*/

/*!
    \fn template<class Container> QIterator<Container> QIterator<Container>::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/

/*!
    \fn template<class Container> QIterator<Container> QIterator<Container>::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), QIterable::canReverseIterate()
*/

/*!
    \fn template<class Container> qsizetype QIterator<Container>::operator-(const QIterator<Container> &j) const
    \overload

    Returns the distance between the two iterators.

    \sa operator+(), operator-=(), QIterable::canReverseIterate()
 */

/*!
    \fn template <class Container> QIterator<Container> QIterator<Container>::operator+(qsizetype j, const QIterator<Container> &k)

    Returns an iterator to the item at \a j positions forward from iterator \a k.
*/

/*!
    \struct QConstIterator
    \since 6.0
    \inmodule QtCore
    \brief The QConstIterator allows iteration over a container in a QVariant.
    \sa QIterator, QIterable
*/

/*!
    \fn template <class Container> QConstIterator<Container>::QConstIterator(const QIterable<Container> *iterable, void *iterator)

    Creates a QConstIterator to wrap \a iterator, operating on \a iterable.
 */

/*!
    \fn template<class Container> bool QConstIterator<Container>::operator==(const QConstIterator<Container> &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template<class Container> bool QConstIterator<Container>::operator!=(const QConstIterator<Container> &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template<class Container> QConstIterator<Container> &QConstIterator<Container>::operator++()

    The prefix \c{++} operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QIterable<Container>::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn template<class Container> QConstIterator<Container> QConstIterator<Container>::operator++(int)

    \overload

    The postfix \c{++} operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/

/*!
    \fn template<class Container> QConstIterator<Container> &QConstIterator<Container>::operator--()

    The prefix \c{--} operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QIterable<Container>::begin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), QIterable::canReverseIterate()
*/

/*!
    \fn template<class Container> QConstIterator<Container> QConstIterator<Container>::operator--(int)

    \overload

    The postfix \c{--} operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa QIterable::canReverseIterate()
*/

/*!
    \fn template<class Container> QConstIterator<Container> &QConstIterator<Container>::operator+=(qsizetype j)

    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/

/*!
    \fn template<class Container> QConstIterator<Container> &QConstIterator<Container>::operator-=(qsizetype j)

    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), QIterable::canReverseIterate()
*/

/*!
    \fn template<class Container> QConstIterator<Container> QConstIterator<Container>::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/

/*!
    \fn template<class Container> QConstIterator<Container> QConstIterator<Container>::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), QIterable::canReverseIterate()
*/

/*!
    \fn template <class Container> qsizetype QConstIterator<Container>::operator-(const QConstIterator<Container> &j) const

    \overload

    Returns the distance between the two iterators.

    \sa operator+(), operator-=(), QIterable::canReverseIterate()
 */

/*!
    \class QIterable
    \inmodule QtCore
    \since 6.0
    \brief QIterable is a template class that is the base class for QSequentialIterable and QAssociativeIterable.
*/

/*!
    \fn template <class Container> bool QIterable<Container>::canInputIterate() const

    Returns whether the container has an input iterator. This corresponds to
    the std::input_iterator_tag iterator trait of the iterator and
    const_iterator of the container.
*/

/*!
    \fn template<class Container> bool QIterable<Container>::canForwardIterate() const

    Returns whether it is possible to iterate over the container in forward
    direction. This corresponds to the std::forward_iterator_tag iterator trait
    of the iterator and const_iterator of the container.
*/

/*!
    \fn template<class Container> bool QIterable<Container>::canReverseIterate() const

    Returns whether it is possible to iterate over the container in reverse. This
    corresponds to the std::bidirectional_iterator_tag iterator trait of the
    const_iterator of the container.
*/

/*!
    \fn template<class Container> bool QIterable<Container>::canRandomAccessIterate() const

    Returns whether it is possible to efficiently skip over multiple values
    using and iterator. This corresponds to the std::random_access_iterator_tag
    iterator trait of the iterator and const_iterator of the container.
*/

/*!
    \fn template<class Container> QConstIterator<Container> QIterable<Container>::constBegin() const

    Returns a QConstIterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa constEnd(), mutableBegin()
*/

/*!
    \fn template<class Container> QConstIterator<Container> QIterable<Container>::constEnd() const

    Returns a Qterable::QConstIterator for the end of the container. This
    can be used in stl-style iteration.

    \sa constBegin(), mutableEnd()
*/

/*!
    \fn template<class Container> QIterator<Container> QIterable<Container>::mutableBegin()

    Returns a QIterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa mutableEnd(), constBegin()
*/

/*!
    \fn template<class Container> QIterator<Container> QIterable<Container>::mutableEnd()

    Returns a QSequentialIterable::iterator for the end of the container. This
    can be used in stl-style iteration.

    \sa mutableBegin(), constEnd()
*/

/*!
    \fn template<class Container> qsizetype QIterable<Container>::size() const

    Returns the number of values in the container.
*/

/*!
    \class QTaggedIterator
    \since 6.0
    \inmodule QtCore
    \brief QTaggedIterator is a template class that wraps an iterator and exposes standard iterator traits.

    In order to use an iterator any of the standard algorithms, its iterator
    traits need to be known. As QSequentialIterable can work with many different
    kinds of containers, we cannot declare the traits in the iterator classes
    themselves. A QTaggedIterator gives you a way to explicitly declare a trait for
    a concrete instance of an iterator or QConstIterator.
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory>::QTaggedIterator(Iterator &&it)

    Constructs a QTaggedIterator from an iterator or QConstIterator \a it. Checks
    whether the IteratorCategory passed as template argument matches the run
    time capabilities of \a it; if there's no match, \a it is refused.
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> bool QTaggedIterator<Iterator, IteratorCategory>::operator==(const QTaggedIterator<Iterator, IteratorCategory> &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> bool QTaggedIterator<Iterator, IteratorCategory>::operator!=(const QTaggedIterator<Iterator, IteratorCategory> &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> &QTaggedIterator<Iterator, IteratorCategory>::operator++()

    The prefix \c{++} operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QSequentialIterable::constEnd() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> QTaggedIterator<Iterator, IteratorCategory>::operator++(int)
    \overload

    The postfix \c{++} operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/


/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> &QTaggedIterator<Iterator, IteratorCategory>::operator--()

    The prefix \c{--} operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QSequentialIterable::constBegin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), QIterable::canReverseIterate()
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> QTaggedIterator<Iterator, IteratorCategory>::operator--(int)
    \overload

    The postfix \c{--} operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa QIterable::canReverseIterate()
*/


/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> &QTaggedIterator<Iterator, IteratorCategory>::operator+=(qsizetype j)

    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> &QTaggedIterator<Iterator, IteratorCategory>::operator-=(qsizetype j)

    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), QIterable::canReverseIterate()
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> QTaggedIterator<Iterator, IteratorCategory>::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> QTaggedIterator<Iterator, IteratorCategory>::operator+(qsizetype j, const QTaggedIterator &k)

    Returns an iterator to the item at \a j positions forward from iterator \a k.
*/

/*!
    \fn template<class Iterator, typename IteratorCategory> QTaggedIterator<Iterator, IteratorCategory> QTaggedIterator<Iterator, IteratorCategory>::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), QIterable::canReverseIterate()
*/

/*!
    \fn template <class Iterator, typename IteratorCategory> qsizetype QTaggedIterator<Iterator, IteratorCategory>::operator-(const QTaggedIterator<Iterator, IteratorCategory> &j) const

    Returns the distance between this iterator and \a j.

    \sa operator+(), operator-=(), QIterable::canReverseIterate()
*/

QT_END_NAMESPACE
