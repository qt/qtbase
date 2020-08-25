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
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSequentialIterable
    \since 5.2
    \inmodule QtCore
    \brief The QSequentialIterable class is an iterable interface for a container in a QVariant.

    This class allows several methods of accessing the elements of a container held within
    a QVariant. An instance of QSequentialIterable can be extracted from a QVariant if it can
    be converted to a QVariantList.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

QSequentialIterable::const_iterator::const_iterator(const QSequentialIterable *iterable, void *iterator)
  : m_iterable(iterable), m_iterator(iterator), m_ref(new QAtomicInt(0))
{
    m_ref->ref();
}

/*! \fn QSequentialIterable::const_iterator QSequentialIterable::begin() const

    Returns a QSequentialIterable::const_iterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa end()
*/
QSequentialIterable::const_iterator QSequentialIterable::begin() const
{
    return const_iterator(this, m_metaSequence.constBegin(m_iterable));
}

/*!
    Returns a QSequentialIterable::const_iterator for the end of the container. This
    can be used in stl-style iteration.

    \sa begin()
*/
QSequentialIterable::const_iterator QSequentialIterable::end() const
{
    return const_iterator(this, m_metaSequence.constEnd(m_iterable));
}

/*!
    Returns the element at position \a idx in the container.
*/
QVariant QSequentialIterable::at(qsizetype idx) const
{
    QVariant v(m_metaSequence.valueMetaType());
    void *dataPtr;
    if (m_metaSequence.valueMetaType() == QMetaType::fromType<QVariant>())
        dataPtr = &v;
    else
        dataPtr = v.data();

    const QMetaSequence metaSequence = m_metaSequence;
    if (metaSequence.canGetElementAtIndex()) {
        metaSequence.elementAtIndex(m_iterable, idx, dataPtr);
    } else if (metaSequence.canGetElementAtConstIterator()) {
        void *iterator = metaSequence.constBegin(m_iterable);
        metaSequence.advanceConstIterator(iterator, idx);
        metaSequence.elementAtConstIterator(iterator, dataPtr);
        metaSequence.destroyConstIterator(iterator);
    }

    return v;
}

/*!
    Returns the number of elements in the container.
*/
qsizetype QSequentialIterable::size() const
{
    const QMetaSequence metaSequence = m_metaSequence;
    const void *container = m_iterable;
    if (metaSequence.hasSize())
        return metaSequence.size(container);
    if (!metaSequence.hasConstIterator())
        return -1;

    const void *begin = metaSequence.constBegin(container);
    const void *end = metaSequence.constEnd(container);
    const qsizetype size = metaSequence.diffConstIterator(end, begin);
    metaSequence.destroyConstIterator(begin);
    metaSequence.destroyConstIterator(end);
    return size;
}

/*!
    Returns whether it is possible to iterate over the container in reverse. This
    corresponds to the std::bidirectional_iterator_tag iterator trait of the
    const_iterator of the container.
*/
bool QSequentialIterable::canReverseIterate() const
{
    return m_metaSequence.hasBidirectionalIterator();
}

/*!
    \class QSequentialIterable::const_iterator
    \since 5.2
    \inmodule QtCore
    \brief The QSequentialIterable::const_iterator allows iteration over a container in a QVariant.

    A QSequentialIterable::const_iterator can only be created by a QSequentialIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    \sa QSequentialIterable
*/


/*!
    Destroys the QSequentialIterable::const_iterator.
*/
QSequentialIterable::const_iterator::~const_iterator() {
    if (!m_ref->deref()) {
        m_iterable->m_metaSequence.destroyConstIterator(m_iterator);
        delete m_ref;
    }
}

/*!
    Creates a copy of \a other.
*/
QSequentialIterable::const_iterator::const_iterator(const const_iterator &other)
  : m_iterable(other.m_iterable), m_iterator(other.m_iterator), m_ref(other.m_ref)
{
    m_ref->ref();
}

/*!
    Assigns \a other to this.
*/
QSequentialIterable::const_iterator&
QSequentialIterable::const_iterator::operator=(const const_iterator &other)
{
    if (this == &other)
        return *this;
    other.m_ref->ref();
    if (!m_ref->deref()) {
        m_iterable->m_metaSequence.destroyConstIterator(m_iterator);
        delete m_ref;
    }
    m_iterable = other.m_iterable;
    m_iterator = other.m_iterator;
    m_ref = other.m_ref;
    return *this;
}

/*!
    Returns the current item, converted to a QVariant.
*/
const QVariant QSequentialIterable::const_iterator::operator*() const
{
    QVariant v(m_iterable->m_metaSequence.valueMetaType());
    void *dataPtr;
    if (m_iterable->m_metaSequence.valueMetaType() == QMetaType::fromType<QVariant>())
        dataPtr = &v;
    else
        dataPtr = v.data();
    m_iterable->m_metaSequence.elementAtConstIterator(m_iterator, dataPtr);
    return v;
}

/*!
    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/
bool QSequentialIterable::const_iterator::operator==(const const_iterator &other) const
{
    return m_iterable->m_metaSequence.compareConstIterator(
                m_iterator, other.m_iterator);
}

/*!
    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/
bool QSequentialIterable::const_iterator::operator!=(const const_iterator &other) const
{
    return !m_iterable->m_metaSequence.compareConstIterator(
                m_iterator, other.m_iterator);
}

/*!
    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QSequentialIterable::end() leads to undefined results.

    \sa operator--()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator++()
{
    m_iterable->m_metaSequence.advanceConstIterator(m_iterator, 1);
    return *this;
}

/*!
    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator++(int)
{
    const_iterator result(
                m_iterable,
                m_iterable->m_metaSequence.constBegin(m_iterable->m_iterable));
    m_iterable->m_metaSequence.copyConstIterator(result.m_iterator, m_iterator);
    m_iterable->m_metaSequence.advanceConstIterator(m_iterator, 1);
    return result;
}

/*!
    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QSequentialIterable::begin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), canReverseIterate()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator--()
{
    m_iterable->m_metaSequence.advanceConstIterator(m_iterator, -1);
    return *this;
}

/*!
    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa canReverseIterate()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator--(int)
{
    const_iterator result(
                m_iterable,
                m_iterable->m_metaSequence.constBegin(m_iterable->m_iterable));
    m_iterable->m_metaSequence.copyConstIterator(result.m_iterator, m_iterator);
    m_iterable->m_metaSequence.advanceConstIterator(m_iterator, -1);
    return result;
}

/*!
    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator+=(int j)
{
    m_iterable->m_metaSequence.advanceConstIterator(m_iterator, j);
    return *this;
}

/*!
    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), canReverseIterate()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator-=(int j)
{
    m_iterable->m_metaSequence.advanceConstIterator(m_iterator, -j);
    return *this;
}

/*!
    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator+(int j) const
{
    const_iterator result(
                m_iterable,
                m_iterable->m_metaSequence.constBegin(m_iterable->m_iterable));
    m_iterable->m_metaSequence.copyConstIterator(result.m_iterator, m_iterator);
    m_iterable->m_metaSequence.advanceConstIterator(result.m_iterator, j);
    return result;
}

/*!
    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), canReverseIterate()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator-(int j) const
{
    const_iterator result(
                m_iterable,
                m_iterable->m_metaSequence.constBegin(m_iterable->m_iterable));
    m_iterable->m_metaSequence.copyConstIterator(result.m_iterator, m_iterator);
    m_iterable->m_metaSequence.advanceConstIterator(result.m_iterator, -j);
    return result;
}

QT_END_NAMESPACE
