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

#include <QtCore/qsequentialiterable.h>
#include <QtCore/qvariant.h>

#include <QtCore/private/qiterable_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSequentialIterable
    \since 5.2
    \inmodule QtCore
    \brief The QSequentialIterable class is an iterable interface for a container in a QVariant.

    This class allows several methods of accessing the values of a container held within
    a QVariant. An instance of QSequentialIterable can be extracted from a QVariant if it can
    be converted to a QVariantList.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \typedef QSequentialIterable::RandomAccessIterator
    Exposes an iterator using std::random_access_iterator_tag.
*/

/*!
    \typedef QSequentialIterable::BidirectionalIterator
    Exposes an iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typedef QSequentialIterable::ForwardIterator
    Exposes an iterator using std::forward_iterator_tag.
*/

/*!
    \typedef QSequentialIterable::InputIterator
    Exposes an iterator using std::input_iterator_tag.
*/

/*!
    \typedef QSequentialIterable::RandomAccessConstIterator
    Exposes a const_iterator using std::random_access_iterator_tag.
*/

/*!
    \typedef QSequentialIterable::BidirectionalConstIterator
    Exposes a const_iterator using std::bidirectional_iterator_tag.
*/

/*!
    \typedef QSequentialIterable::ForwardConstIterator
    Exposes a const_iterator using std::forward_iterator_tag.
*/

/*!
    \typedef QSequentialIterable::InputConstIterator
    Exposes a const_iterator using std::input_iterator_tag.
*/

/*!
    Adds \a value to the container, at \a position, if possible.
 */
void QSequentialIterable::addValue(const QVariant &value, Position position)
{
    QtPrivate::QVariantTypeCoercer coercer;
    const void *valuePtr = coercer.coerce(value, metaContainer().valueMetaType());

    switch (position) {
    case AtBegin:
        if (metaContainer().canAddValueAtBegin())
            metaContainer().addValueAtBegin(mutableIterable(), valuePtr);
        break;
    case AtEnd:
        if (metaContainer().canAddValueAtEnd())
            metaContainer().addValueAtEnd(mutableIterable(), valuePtr);
        break;
    case Unspecified:
        if (metaContainer().canAddValue())
            metaContainer().addValue(mutableIterable(), valuePtr);
        break;
    }
}

/*!
    Removes a value from the container, at \a position, if possible.
 */
void QSequentialIterable::removeValue(Position position)
{
    switch (position) {
    case AtBegin:
        if (metaContainer().canRemoveValueAtBegin())
            metaContainer().removeValueAtBegin(mutableIterable());
        break;
    case AtEnd:
        if (metaContainer().canRemoveValueAtEnd())
            metaContainer().removeValueAtEnd(mutableIterable());
        break;
    case Unspecified:
        if (metaContainer().canRemoveValue())
            metaContainer().removeValue(mutableIterable());
        break;
    }
}

QMetaType QSequentialIterable::valueMetaType() const
{
    return QMetaType(metaContainer().valueMetaType());
}

/*!
    Returns the value at position \a idx in the container.
*/
QVariant QSequentialIterable::at(qsizetype idx) const
{
    QVariant v(valueMetaType());
    void *dataPtr;
    if (valueMetaType() == QMetaType::fromType<QVariant>())
        dataPtr = &v;
    else
        dataPtr = v.data();

    const QMetaSequence meta = metaContainer();
    if (meta.canGetValueAtIndex()) {
        meta.valueAtIndex(m_iterable.constPointer(), idx, dataPtr);
    } else if (meta.canGetValueAtConstIterator()) {
        void *iterator = meta.constBegin(m_iterable.constPointer());
        meta.advanceConstIterator(iterator, idx);
        meta.valueAtConstIterator(iterator, dataPtr);
        meta.destroyConstIterator(iterator);
    }

    return v;
}

/*!
    Sets the element at position \a idx in the container to \a value.
*/
void QSequentialIterable::set(qsizetype idx, const QVariant &value)
{
    QtPrivate::QVariantTypeCoercer coercer;
    const void *dataPtr = coercer.coerce(value, metaContainer().valueMetaType());

    const QMetaSequence meta = metaContainer();
    if (meta.canSetValueAtIndex()) {
        meta.setValueAtIndex(m_iterable.mutablePointer(), idx, dataPtr);
    } else if (meta.canSetValueAtIterator()) {
        void *iterator = meta.begin(m_iterable.mutablePointer());
        meta.advanceIterator(iterator, idx);
        meta.setValueAtIterator(iterator, dataPtr);
        meta.destroyIterator(iterator);
    }
}

/*!
    \typealias QSequentialIterable::const_iterator
    \brief The QSequentialIterable::const_iterator allows iteration over a container in a QVariant.

    A QSequentialIterable::const_iterator can only be created by a QSequentialIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 9
*/

/*!
    \typealias QSequentialIterable::iterator
    \since 6.0
    \brief The QSequentialIterable::iterator allows iteration over a container in a QVariant.

    A QSequentialIterable::iterator can only be created by a QSequentialIterable instance,
    and can be used in a way similar to other stl-style iterators.
*/

/*!
    Returns the current item, converted to a QVariantRef.
*/
QVariantRef<QSequentialIterator> QSequentialIterator::operator*() const
{
    return QVariantRef<QSequentialIterator>(this);
}

/*!
    Returns the current item, converted to a QVariantPointer.
*/
QVariantPointer<QSequentialIterator> QSequentialIterator::operator->() const
{
    return QVariantPointer<QSequentialIterator>(this);
}

/*!
    Returns the current item, converted to a QVariant.
*/
QVariant QSequentialConstIterator::operator*() const
{
    return QIterablePrivate::retrieveElement(metaContainer().valueMetaType(), [this](void *dataPtr) {
        metaContainer().valueAtConstIterator(constIterator(), dataPtr);
    });
}

/*!
    Returns the current item, converted to a QVariantConstPointer.
*/
QVariantConstPointer QSequentialConstIterator::operator->() const
{
    return QVariantConstPointer(operator*());
}

QT_END_NAMESPACE
