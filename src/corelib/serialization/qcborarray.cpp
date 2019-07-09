/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
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

#include "qcborarray.h"
#include "qcborvalue_p.h"
#include "qdatastream.h"

QT_BEGIN_NAMESPACE

using namespace QtCbor;

/*!
    \class QCborArray
    \inmodule QtCore
    \ingroup cbor
    \reentrant
    \since 5.12

    \brief The QCborArray class is used to hold an array of CBOR elements.

    This class can be used to hold one sequential container in CBOR (an array).
    CBOR is the Concise Binary Object Representation, a very compact form of
    binary data encoding that is a superset of JSON. It was created by the IETF
    Constrained RESTful Environments (CoRE) WG, which has used it in many new
    RFCs. It is meant to be used alongside the
    \l{https://tools.ietf.org/html/rfc7252}{CoAP protocol}.

    QCborArray is very similar to \l QVariantList and \l QJsonArray and its API
    is almost identical to those two classes. It can also be converted to and
    from those two, though there may be loss of information in some
    conversions.

    \sa QCborValue, QCborMap, QJsonArray, QList, QVector
 */

/*!
    \typedef QCborArray::size_type

    A typedef to qsizetype.
 */

/*!
    \typedef QCborArray::difference_type

    A typedef to qsizetype.
 */

/*!
    \typedef QCborArray::value_type

    The type of values that can be held in a QCborArray: that is, QCborValue.
 */

/*!
    \typedef QCborArray::pointer

    A typedef to \c{QCborValue *}, for compatibility with generic algorithms.
 */

/*!
    \typedef QCborArray::const_pointer

    A typedef to \c{const QCborValue *}, for compatibility with generic algorithms.
 */

/*!
    \typedef QCborArray::reference

    A typedef to \c{QCborValue &}, for compatibility with generic algorithms.
 */

/*!
    \typedef QCborArray::const_reference

    A typedef to \c{const QCborValue &}, for compatibility with generic algorithms.
 */

/*!
    Constructs an empty QCborArray.
 */
QCborArray::QCborArray() noexcept
    : d(nullptr)
{
}

/*!
    Copies the contents of \a other into this object.
 */
QCborArray::QCborArray(const QCborArray &other) noexcept
    : d(other.d)
{
}

/*!
    \fn QCborArray::QCborArray(std::initializer_list<QCborValue> args)

    Initializes this QCborArray from the C++ brace-enclosed list found in \a
    args, as in the following example:

    \code
        QCborArray a = { null, 0, 1, 1.5, 2, "Hello", QByteArray("World") };
    \endcode
 */

/*!
    Destroys this QCborArray and frees any associated resources.
 */
QCborArray::~QCborArray()
{
}

/*!
    Replaces the contents of this array with that found in \a other, then
    returns a reference to this object.
 */
QCborArray &QCborArray::operator=(const QCborArray &other) noexcept
{
    d = other.d;
    return *this;
}

/*!
    \fn void QCborArray::swap(QCborArray &other)

    Swaps the contents of this object and \a other.
 */

/*!
    \fn QCborValue QCborArray::toCborValue() const

    Explicitly construcuts a \l QCborValue object that represents this array.
    This function is usually not necessary since QCborValue has a constructor
    for QCborArray, so the conversion is implicit.

    Converting QCborArray to QCborValue allows it to be used in any context
    where QCborValues can be used, including as items in QCborArrays and as keys
    and mapped types in QCborMap. Converting an array to QCborValue allows
    access to QCborValue::toCbor().

    \sa QCborValue::QCborValue(const QCborArray &)
 */

/*!
    Returns the size of this array.

    \sa isEmpty()
 */
qsizetype QCborArray::size() const noexcept
{
    return d ? d->elements.size() : 0;
}

/*!
    Empties this array.

    \sa isEmpty()
 */
void QCborArray::clear()
{
    d.reset();
}

/*!
    \fn bool QCborArray::isEmpty() const

    Returns true if this QCborArray is empty (that is if size() is 0).

    \sa size(), clear()
 */

/*!
    Returns the QCborValue element at position \a i in the array.

    If the array is smaller than \a i elements, this function returns a
    QCborValue containing an undefined value. For that reason, it is not
    possible with this function to tell apart the situation where the array is
    not large enough from the case where the array starts with an undefined
    value.

    \sa operator[](), first(), last(), insert(), prepend(), append(),
        removeAt(), takeAt()
 */
QCborValue QCborArray::at(qsizetype i) const
{
    if (!d || size_t(i) >= size_t(size()))
        return QCborValue();
    return d->valueAt(i);
}

/*!
    \fn QCborValue QCborArray::first() const

    Returns the first QCborValue of this array.

    If the array is empty, this function returns a QCborValue containing an
    undefined value. For that reason, it is not possible with this function to
    tell apart the situation where the array is not large enough from the case
    where the array ends with an undefined value.

    \sa operator[](), at(), last(), insert(), prepend(), append(),
        removeAt(), takeAt()
 */

/*!
    \fn QCborValue QCborArray::last() const

    Returns the last QCborValue of this array.

    If the array is empty, this function returns a QCborValue containing an
    undefined value. For that reason, it is not possible with this function to
    tell apart the situation where the array is not large enough from the case
    where the array ends with an undefined value.

    \sa operator[](), at(), first(), insert(), prepend(), append(),
        removeAt(), takeAt()
 */

/*!
    \fn QCborValue QCborArray::operator[](qsizetype i) const

    Returns the QCborValue element at position \a i in the array.

    If the array is smaller than \a i elements, this function returns a
    QCborValue containing an undefined value. For that reason, it is not
    possible with this function to tell apart the situation where the array is
    not large enough from the case where the array contains an undefined value
    at position \a i.

    \sa at(), first(), last(), insert(), prepend(), append(),
        removeAt(), takeAt()
 */

/*!
    \fn QCborValueRef QCborArray::first()

    Returns a reference to the first QCborValue of this array. The array must
    not be empty.

    QCborValueRef has the exact same API as \l QCborValue, with one important
    difference: if you assign new values to it, this array will be updated with
    that new value.

    \sa operator[](), at(), last(), insert(), prepend(), append(),
        removeAt(), takeAt()
 */

/*!
    \fn QCborValueRef QCborArray::last()

    Returns a reference to the last QCborValue of this array. The array must
    not be empty.

    QCborValueRef has the exact same API as \l QCborValue, with one important
    difference: if you assign new values to it, this array will be updated with
    that new value.

    \sa operator[](), at(), first(), insert(), prepend(), append(),
        removeAt(), takeAt()
 */

/*!
    \fn QCborValueRef QCborArray::operator[](qsizetype i)

    Returns a reference to the QCborValue element at position \a i in the
    array.  Indices beyond the end of the array will grow the array, filling
    with undefined entries, until it has an entry at the specified index.

    QCborValueRef has the exact same API as \l QCborValue, with one important
    difference: if you assign new values to it, this array will be updated with
    that new value.

    \sa at(), first(), last(), insert(), prepend(), append(),
        removeAt(), takeAt()
 */

/*!
    \fn void QCborArray::insert(qsizetype i, const QCborValue &value)
    \fn void QCborArray::insert(qsizetype i, QCborValue &&value)

    Inserts \a value into the array at position \a i in this array. If \a i is
    -1, the entry is appended to the array. Pads the array with invalid entries
    if \a i is greater than the prior size of the array.

    \sa at(), operator[](), first(), last(), prepend(), append(),
        removeAt(), takeAt(), extract()
 */
void QCborArray::insert(qsizetype i, const QCborValue &value)
{
    if (i < 0) {
        Q_ASSERT(i == -1);
        i = size();
        detach(i + 1);
    } else {
        d = QCborContainerPrivate::grow(d.data(), i); // detaches
    }
    d->insertAt(i, value);
}

void QCborArray::insert(qsizetype i, QCborValue &&value)
{
    if (i < 0) {
        Q_ASSERT(i == -1);
        i = size();
        detach(i + 1);
    } else {
        d = QCborContainerPrivate::grow(d.data(), i); // detaches
    }
    d->insertAt(i, value, QCborContainerPrivate::MoveContainer);
    QCborContainerPrivate::resetValue(value);
}

/*!
    \fn QCborValue QCborArray::extract(Iterator it)
    \fn QCborValue QCborArray::extract(ConstIterator it)

    Extracts a value from the array at the position indicated by iterator \a it
    and returns the value so extracted.

    \sa insert(), erase(), takeAt(), removeAt()
 */
QCborValue QCborArray::extract(iterator it)
{
    detach();

    QCborValue v = d->extractAt(it.item.i);
    d->removeAt(it.item.i);
    return v;
}

/*!
    \fn void QCborArray::prepend(const QCborValue &value)
    \fn void QCborArray::prepend(QCborValue &&value)

    Prepends \a value into the array before any other elements it may already
    contain.

    \sa at(), operator[](), first(), last(), insert(), append(),
        removeAt(), takeAt()
 */

/*!
    \fn void QCborArray::append(const QCborValue &value)
    \fn void QCborArray::append(QCborValue &&value)

    Appends \a value into the array after all other elements it may already
    contain.

    \sa at(), operator[](), first(), last(), insert(), prepend(),
        removeAt(), takeAt()
 */

/*!
    Removes the item at position \a i from the array. The array must have more
    than \a i elements before the removal.

    \sa takeAt(), removeFirst(), removeLast(), at(), operator[](), insert(),
        prepend(), append()
 */
void QCborArray::removeAt(qsizetype i)
{
    detach(size());
    d->removeAt(i);
}

/*!
    \fn QCborValue QCborArray::takeAt(qsizetype i)

    Removes the item at position \a i from the array and returns it. The array
    must have more than \a i elements before the removal.

    \sa removeAt(), removeFirst(), removeLast(), at(), operator[](), insert(),
        prepend(), append()
 */

/*!
    \fn void QCborArray::removeFirst()

    Removes the first item in the array, making the second element become the
    first. The array must not be empty before this call.

    \sa removeAt(), takeFirst(), removeLast(), at(), operator[](), insert(),
        prepend(), append()
 */

/*!
    \fn void QCborArray::removeLast()

    Removes the last item in the array. The array must not be empty before this
    call.

    \sa removeAt(), takeLast(), removeFirst(), at(), operator[](), insert(),
        prepend(), append()
 */

/*!
    \fn void QCborArray::takeFirst()

    Removes the first item in the array and returns it, making the second
    element become the first. The array must not be empty before this call.

    \sa takeAt(), removeFirst(), removeLast(), at(), operator[](), insert(),
        prepend(), append()
 */

/*!
    \fn void QCborArray::takeLast()

    Removes the last item in the array and returns it. The array must not be
    empty before this call.

    \sa takeAt(), removeLast(), removeFirst(), at(), operator[](), insert(),
        prepend(), append()
 */

/*!
    Returns true if this array contains an element that is equal to \a value.
 */
bool QCborArray::contains(const QCborValue &value) const
{
    for (qsizetype i = 0; i < size(); ++i) {
        int cmp = d->compareElement(i, value);
        if (cmp == 0)
            return true;
    }
    return false;
}

/*!
    \fn int QCborArray::compare(const QCborArray &other) const

    Compares this array and \a other, comparing each element in sequence, and
    returns an integer that indicates whether this array should be sorted
    before (if the result is negative) or after \a other (if the result is
    positive). If this function returns 0, the two arrays are equal and contain
    the same elements.

    For more information on CBOR sorting order, see QCborValue::compare().

    \sa QCborValue::compare(), QCborMap::compare(), operator==()
 */

/*!
    \fn bool QCborArray::operator==(const QCborArray &other) const

    Compares this array and \a other, comparing each element in sequence, and
    returns true if both arrays contains the same elements, false otherwise.

    For more information on CBOR equality in Qt, see, QCborValue::compare().

    \sa compare(), QCborValue::operator==(), QCborMap::operator==(),
        operator!=(), operator<()
 */

/*!
    \fn bool QCborArray::operator!=(const QCborArray &other) const

    Compares this array and \a other, comparing each element in sequence, and
    returns true if the two arrays' contents are different, false otherwise.

    For more information on CBOR equality in Qt, see, QCborValue::compare().

    \sa compare(), QCborValue::operator==(), QCborMap::operator==(),
        operator==(), operator<()
 */

/*!
    \fn bool QCborArray::operator<(const QCborArray &other) const

    Compares this array and \a other, comparing each element in sequence, and
    returns true if this array should be sorted before \a other, false
    otherwise.

    For more information on CBOR sorting order, see QCborValue::compare().

    \sa compare(), QCborValue::operator==(), QCborMap::operator==(),
        operator==(), operator!=()
 */

/*!
    \typedef QCborArray::iterator

    A synonym to QCborArray::Iterator.
 */

/*!
    \typedef QCborArray::const_iterator

    A synonym to QCborArray::ConstIterator.
 */

/*!
    \fn QCborArray::iterator QCborArray::begin()

    Returns an array iterator pointing to the first item in this array. If the
    array is empty, then this function returns the same as end().

    \sa constBegin(), end()
 */

/*!
    \fn QCborArray::const_iterator QCborArray::begin() const

    Returns an array iterator pointing to the first item in this array. If the
    array is empty, then this function returns the same as end().

    \sa constBegin(), constEnd()
 */

/*!
    \fn QCborArray::const_iterator QCborArray::cbegin() const

    Returns an array iterator pointing to the first item in this array. If the
    array is empty, then this function returns the same as end().

    \sa constBegin(), constEnd()
 */

/*!
    \fn QCborArray::const_iterator QCborArray::constBegin() const

    Returns an array iterator pointing to the first item in this array. If the
    array is empty, then this function returns the same as end().

    \sa begin(), constEnd()
 */

/*!
    \fn QCborArray::iterator QCborArray::end()

    Returns an array iterator pointing to just after the last element in this
    array.

    \sa begin(), constEnd()
 */

/*!
    \fn QCborArray::const_iterator QCborArray::end() const

    Returns an array iterator pointing to just after the last element in this
    array.

    \sa constBegin(), constEnd()
 */

/*!
    \fn QCborArray::const_iterator QCborArray::cend() const

    Returns an array iterator pointing to just after the last element in this
    array.

    \sa constBegin(), constEnd()
 */

/*!
    \fn QCborArray::const_iterator QCborArray::constEnd() const

    Returns an array iterator pointing to just after the last element in this
    array.

    \sa constBegin(), end()
 */

/*!
    \fn QCborArray::iterator QCborArray::insert(iterator before, const QCborValue &value)
    \fn QCborArray::iterator QCborArray::insert(const_iterator before, const QCborValue &value)
    \overload

    Inserts \a value into this array before element \a before and returns an
    array iterator pointing to the just-inserted element.

    \sa erase(), removeAt(), prepend(), append()
 */

/*!
    \fn QCborArray::iterator QCborArray::erase(iterator it)
    \fn QCborArray::iterator QCborArray::erase(const_iterator it)

    Removes the element pointed to by the array iterator \a it from this array,
    then returns an iterator to the next element (the one that took the same
    position in the array that \a it used to occupy).

    \sa insert(), removeAt(), takeAt(), takeFirst(), takeLast()
 */

/*!
    \fn void QCborArray::push_back(const QCborValue &t)

    Synonym for append(). This function is provided for compatibility with
    generic code that uses the Standard Library API.

    Appends the element \a t to this array.

    \sa append(), push_front(), pop_back(), prepend(), insert()
 */

/*!
    \fn void QCborArray::push_front(const QCborValue &t)

    Synonym for prepend(). This function is provided for compatibility with
    generic code that uses the Standard Library API.

    Prepends the element \a t to this array.

    \sa prepend(), push_back(), pop_front(), append(), insert()
 */

/*!
    \fn void QCborArray::pop_front()

    Synonym for removeFirst(). This function is provided for compatibility with
    generic code that uses the Standard Library API.

    Removes the first element of this array. The array must not be empty before
    the removal

    \sa removeFirst(), takeFirst(), pop_back(), push_front(), prepend(), insert()
 */

/*!
    \fn void QCborArray::pop_back()

    Synonym for removeLast(). This function is provided for compatibility with
    generic code that uses the Standard Library API.

    Removes the last element of this array. The array must not be empty before
    the removal

    \sa removeLast(), takeLast(), pop_front(), push_back(), append(), insert()
 */

/*!
    \fn bool QCborArray::empty() const

    Synonym for isEmpty(). This function is provided for compatibility with
    generic code that uses the Standard Library API.

    Returns true if this array is empty (size() == 0).

    \sa isEmpty(), size()
 */

/*!
    \fn QCborArray QCborArray::operator+(const QCborValue &v) const

    Returns a new QCborArray containing the same elements as this array, plus
    \a v appended as the last element.

    \sa operator+=(), operator<<(), append()
 */

/*!
    \fn QCborArray &QCborArray::operator+=(const QCborValue &v)

    Appends \a v to this array and returns a reference to this array.

    \sa append(), insert(), operator+(), operator<<()
 */

/*!
    \fn QCborArray &QCborArray::operator<<(const QCborValue &v)

    Appends \a v to this array and returns a reference to this array.

    \sa append(), insert(), operator+(), operator+=()
 */

void QCborArray::detach(qsizetype reserved)
{
    d = QCborContainerPrivate::detach(d.data(), reserved ? reserved : size());
}

/*!
    \class QCborArray::Iterator
    \inmodule QtCore
    \ingroup cbor
    \since 5.12

    \brief The QCborArray::Iterator class provides an STL-style non-const iterator for QCborArray.

    QCborArray::Iterator allows you to iterate over a QCborArray and to modify
    the array item associated with the iterator. If you want to iterate over a
    const QCborArray, use QCborArray::ConstIterator instead. It is generally a
    good practice to use QCborArray::ConstIterator on a non-const QCborArray as
    well, unless you need to change the QCborArray through the iterator. Const
    iterators are slightly faster and improve code readability.

    Iterators are initialized by using a QCborArray function like
    QCborArray::begin(), QCborArray::end(), or QCborArray::insert(). Iteration
    is only possible after that.

    Most QCborArray functions accept an integer index rather than an iterator.
    For that reason, iterators are rarely useful in connection with QCborArray.
    One place where STL-style iterators do make sense is as arguments to
    \l{generic algorithms}.

    Multiple iterators can be used on the same array. However, be aware that
    any non-const function call performed on the QCborArray will render all
    existing iterators undefined.

    \sa QCborArray::ConstIterator
*/

/*!
    \typedef QCborArray::Iterator::iterator_category

    A synonym for \e {std::random_access_iterator_tag} indicating this iterator
    is a random access iterator.
 */

/*!
    \typedef QCborArray::Iterator::difference_type
    \internal
*/

/*!
    \typedef QCborArray::Iterator::value_type
    \internal
*/

/*!
    \typedef QCborArray::Iterator::reference
    \internal
*/

/*!
    \typedef QCborArray::Iterator::pointer
    \internal
*/

/*!
    \fn QCborArray::Iterator::Iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called on an
    uninitialized iterator. Use operator=() to assign a value to it before
    using it.

    \sa QCborArray::begin(), QCborArray::end()
*/

/*!
    \fn QCborArray::Iterator::Iterator(const Iterator &other)

    Makes a copy of \a other.
 */

/*!
    \fn QCborArray::Iterator &QCborArray::Iterator::operator=(const Iterator &other)

    Makes this iterator a copy of \a other and returns a reference to this iterator.
 */

/*!
    \fn QCborValueRef QCborArray::Iterator::operator*() const

    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the left side
    of an assignment.

    The return value is of type QCborValueRef, a helper class for QCborArray
    and QCborMap. When you get an object of type QCborValueRef, you can use it
    as if it were a reference to a QCborValue. If you assign to it, the
    assignment will apply to the element in the QCborArray or QCborMap from
    which you got the reference.
*/

/*!
    \fn QCborValueRef *QCborArray::Iterator::operator->() const

    Returns a pointer to a modifiable reference to the current item.
*/

/*!
    \fn QCborValueRef QCborArray::Iterator::operator[](qsizetype j)

    Returns a modifiable reference to the item at a position \a j steps forward
    from the item pointed to by this iterator.

    This function is provided to make QCborArray iterators behave like C++
    pointers.

    The return value is of type QCborValueRef, a helper class for QCborArray
    and QCborMap. When you get an object of type QCborValueRef, you can use it
    as if it were a reference to a QCborValue. If you assign to it, the
    assignment will apply to the element in the QCborArray or QCborMap from
    which you got the reference.

    \sa operator+()
*/

/*!
    \fn bool QCborArray::Iterator::operator==(const Iterator &other) const
    \fn bool QCborArray::Iterator::operator==(const ConstIterator &other) const

    Returns \c true if \a other points to the same entry in the array as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QCborArray::Iterator::operator!=(const Iterator &other) const
    \fn bool QCborArray::Iterator::operator!=(const ConstIterator &other) const

    Returns \c true if \a other points to a different entry in the array than
    this iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QCborArray::Iterator::operator<(const Iterator& other) const
    \fn bool QCborArray::Iterator::operator<(const ConstIterator& other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs before the entry pointed to by the \a other iterator.
*/

/*!
    \fn bool QCborArray::Iterator::operator<=(const Iterator& other) const
    \fn bool QCborArray::Iterator::operator<=(const ConstIterator& other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs before or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn bool QCborArray::Iterator::operator>(const Iterator& other) const
    \fn bool QCborArray::Iterator::operator>(const ConstIterator& other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs after the entry pointed to by the \a other iterator.
 */

/*!
    \fn bool QCborArray::Iterator::operator>=(const Iterator& other) const
    \fn bool QCborArray::Iterator::operator>=(const ConstIterator& other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs after or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn QCborArray::Iterator &QCborArray::Iterator::operator++()

    The prefix ++ operator, \c{++it}, advances the iterator to the next item in
    the array and returns this iterator.

    Calling this function on QCborArray::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QCborArray::Iterator QCborArray::Iterator::operator++(int)
    \overload

    The postfix ++ operator, \c{it++}, advances the iterator to the next item
    in the array and returns an iterator to the previously current item.
*/

/*!
    \fn QCborArray::Iterator &QCborArray::Iterator::operator--()

    The prefix -- operator, \c{--it}, makes the preceding item current and
    returns this iterator.

    Calling this function on QCborArray::begin() leads to undefined results.

    \sa operator++()
*/

/*!
    \fn QCborArray::Iterator QCborArray::Iterator::operator--(int)
    \overload

    The postfix -- operator, \c{it--}, makes the preceding item current and
    returns an iterator to the previously current item.
*/

/*!
    \fn QCborArray::Iterator &QCborArray::Iterator::operator+=(qsizetype j)

    Advances the iterator by \a j positions. If \a j is negative, the iterator
    goes backward. Returns a reference to this iterator.

    \sa operator-=(), operator+()
*/

/*!
    \fn QCborArray::Iterator &QCborArray::Iterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j positions. If \a j is negative, the
    iterator goes forward. Returns a reference to this iterator.

    \sa operator+=(), operator-()
*/

/*!
    \fn QCborArray::Iterator QCborArray::Iterator::operator+(qsizetype j) const

    Returns an iterator to the item at position \a j steps forward from this
    iterator. If \a j is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*!
    \fn QCborArray::Iterator QCborArray::Iterator::operator-(qsizetype j) const

    Returns an iterator to the item at position \a j steps backward from this
    iterator. If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*!
    \fn qsizetype QCborArray::Iterator::operator-(Iterator other) const

    Returns the offset of this iterator relative to \a other.
*/

/*!
    \class QCborArray::ConstIterator
    \inmodule QtCore
    \ingroup cbor
    \since 5.12

    \brief The QCborArray::ConstIterator class provides an STL-style const iterator for QCborArray.

    QCborArray::ConstIterator allows you to iterate over a QCborArray. If you
    want to modify the QCborArray as you iterate over it, use
    QCborArray::Iterator instead. It is generally good practice to use
    QCborArray::ConstIterator, even on a non-const QCborArray, when you don't
    need to change the QCborArray through the iterator. Const iterators are
    slightly faster and improves code readability.

    Iterators are initialized by using a QCborArray function like
    QCborArray::begin() or QCborArray::end(). Iteration is only possible after
    that.

    Most QCborArray functions accept an integer index rather than an iterator.
    For that reason, iterators are rarely useful in connection with QCborArray.
    One place where STL-style iterators do make sense is as arguments to
    \l{generic algorithms}.

    Multiple iterators can be used on the same array. However, be aware that
    any non-const function call performed on the QCborArray will render all
    existing iterators undefined.

    \sa QCborArray::Iterator
*/

/*!
    \fn QCborArray::ConstIterator::ConstIterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called on an
    uninitialized iterator. Use operator=() to assign a value to it before
    using it.

    \sa QCborArray::constBegin(), QCborArray::constEnd()
*/

/*!
    \fn QCborArray::ConstIterator &QCborArray::ConstIterator::operator=(const ConstIterator &other)

    Makes this iterator a copy of \a other and returns a reference to this iterator.
*/

/*!
    \typedef QCborArray::ConstIterator::iterator_category

    A synonym for \e {std::random_access_iterator_tag} indicating this iterator
    is a random access iterator.
*/

/*!
    \typedef QCborArray::ConstIterator::difference_type
    \internal
*/

/*!
    \typedef QCborArray::ConstIterator::value_type
    \internal
*/

/*!
    \typedef QCborArray::ConstIterator::reference
    \internal
*/

/*!
    \typedef QCborArray::ConstIterator::pointer
    \internal
*/

/*!
    \fn QCborArray::ConstIterator::ConstIterator(const ConstIterator &other)

    Constructs a copy of \a other.
*/

/*!
    \fn QCborValue QCborArray::ConstIterator::operator*() const

    Returns the current item.
*/

/*!
    \fn const QCborValue *QCborArray::ConstIterator::operator->() const

    Returns a pointer to the current item.
*/

/*!
    \fn const QCborValueRef QCborArray::ConstIterator::operator[](qsizetype j)

    Returns the item at a position \a j steps forward from the item pointed to
    by this iterator.

    This function is provided to make QCborArray iterators behave like C++
    pointers.

    \sa operator+()
*/

/*!
    \fn bool QCborArray::ConstIterator::operator==(const Iterator &other) const
    \fn bool QCborArray::ConstIterator::operator==(const ConstIterator &other) const

    Returns \c true if \a other points to the same entry in the array as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QCborArray::ConstIterator::operator!=(const Iterator &o) const
    \fn bool QCborArray::ConstIterator::operator!=(const ConstIterator &o) const

    Returns \c true if \a o points to a different entry in the array than
    this iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QCborArray::ConstIterator::operator<(const Iterator &other) const
    \fn bool QCborArray::ConstIterator::operator<(const ConstIterator &other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs before the entry pointed to by the \a other iterator.
*/

/*!
    \fn bool QCborArray::ConstIterator::operator<=(const Iterator &other) const
    \fn bool QCborArray::ConstIterator::operator<=(const ConstIterator &other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs before or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn bool QCborArray::ConstIterator::operator>(const Iterator &other) const
    \fn bool QCborArray::ConstIterator::operator>(const ConstIterator &other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs after the entry pointed to by the \a other iterator.
*/

/*!
    \fn bool QCborArray::ConstIterator::operator>=(const Iterator &other) const
    \fn bool QCborArray::ConstIterator::operator>=(const ConstIterator &other) const

    Returns \c true if the entry in the array pointed to by this iterator
    occurs after or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn QCborArray::ConstIterator &QCborArray::ConstIterator::operator++()

    The prefix ++ operator, \c{++it}, advances the iterator to the next item in
    the array and returns this iterator.

    Calling this function on QCborArray::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QCborArray::ConstIterator QCborArray::ConstIterator::operator++(int)
    \overload

    The postfix ++ operator, \c{it++}, advances the iterator to the next item
    in the array and returns an iterator to the previously current item.
*/

/*!
    \fn QCborArray::ConstIterator &QCborArray::ConstIterator::operator--()

    The prefix -- operator, \c{--it}, makes the preceding item current and
    returns this iterator.

    Calling this function on QCborArray::begin() leads to undefined results.

    \sa operator++()
*/

/*!
    \fn QCborArray::ConstIterator QCborArray::ConstIterator::operator--(int)
    \overload

    The postfix -- operator, \c{it--}, makes the preceding item current and
    returns an iterator to the previously current item.
*/

/*!
    \fn QCborArray::ConstIterator &QCborArray::ConstIterator::operator+=(qsizetype j)

    Advances the iterator by \a j positions. If \a j is negative, the iterator
    goes backward. Returns a reference to this iterator.

    \sa operator-=(), operator+()
*/

/*!
    \fn QCborArray::ConstIterator &QCborArray::ConstIterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j positions. If \a j is negative, the
    iterator goes forward. Returns a reference to this iterator.

    \sa operator+=(), operator-()
*/

/*!
    \fn QCborArray::ConstIterator QCborArray::ConstIterator::operator+(qsizetype j) const

    Returns an iterator to the item at a position \a j steps forward from this
    iterator. If \a j is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*!
    \fn QCborArray::ConstIterator QCborArray::ConstIterator::operator-(qsizetype j) const

    Returns an iterator to the item at a position \a j steps backward from this
    iterator. If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*!
    \fn qsizetype QCborArray::ConstIterator::operator-(ConstIterator other) const

    Returns the offset of this iterator relative to \a other.
*/

uint qHash(const QCborArray &array, uint seed)
{
    return qHashRange(array.begin(), array.end(), seed);
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, const QCborArray &a)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QCborArray{";
    const char *comma = "";
    for (auto v : a) {
        dbg << comma << v;
        comma = ", ";
    }
    return dbg << '}';
}
#endif

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &stream, const QCborArray &value)
{
    stream << value.toCborValue().toCbor();
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QCborArray &value)
{
    QByteArray buffer;
    stream >> buffer;
    QCborParserError parseError{};
    value = QCborValue::fromCbor(buffer, &parseError).toArray();
    if (parseError.error)
        stream.setStatus(QDataStream::ReadCorruptData);
    return stream;
}
#endif

QT_END_NAMESPACE
