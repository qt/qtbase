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

#include "qcbormap.h"
#include "qcborvalue_p.h"

QT_BEGIN_NAMESPACE

using namespace QtCbor;

/*!
    \class QCborMap
    \inmodule QtCore
    \ingroup cbor
    \reentrant
    \since 5.12

    \brief The QCborMap class is used to hold an associative container representable in CBOR.

    This class can be used to hold an associative container in CBOR, a map
    between a key and a value type. CBOR is the Concise Binary Object
    Representation, a very compact form of binary data encoding that is a
    superset of JSON. It was created by the IETF Constrained RESTful
    Environments (CoRE) WG, which has used it in many new RFCs. It is meant to
    be used alongside the \l{https://tools.ietf.org/html/rfc7252}{CoAP
    protocol}.

    Unlike JSON and \l QVariantMap, CBOR map keys can be of any type, not just
    strings. For that reason, QCborMap is effectively a map between QCborValue
    keys to QCborValue value elements.

    However, for all member functions that take a key parameter, QCborMap
    provides overloads that will work efficiently with integers and strings. In
    fact, the use of integer keys is encouraged, since they occupy fewer bytes
    to transmit and are simpler to encode and decode. Newer protocols designed
    by the IETF CoRE WG to work specifically with CBOR are known to use them.

    QCborMap is not sorted, because of that, searching for keys has linear
    complexity (O(n)). QCborMap actually keeps the elements in the order that
    they were inserted, which means that it is possible to make sorted
    QCborMaps by carefully inserting elements in sorted order. CBOR does not
    require sorting, but recommends it.

    QCborMap can also be converted to and from QVariantMap and QJsonObject.
    However, when performing the conversion, any non-string keys will be
    stringified using a one-way method that the conversion back to QCborMap
    will not undo.

    \sa QCborArray, QCborValue, QJsonDocument, QVariantMap
 */

/*!
    \typedef QCborMap::value_type

    The value that is stored in this container: a pair of QCborValues
 */

/*!
    \typedef QCborMap::key_type

    The key type for this map. Since QCborMap keys can be any CBOR type, this
    is a QCborValue.
 */

/*!
    \typedef QCborMap::mapped_type

    The type that is mapped to (the value), that is, a QCborValue.
 */

/*!
    \typedef QCborMap::size_type

    The type that QCborMap uses for sizes.
 */

/*!
    \typedef QCborMap::iterator

    A synonym for QCborMap::Iterator.
 */

/*!
    \typedef QCborMap::const_iterator

    A synonym for QCborMap::ConstIterator
 */

/*!
    \fn QCborMap::iterator QCborMap::begin()

    Returns a map iterator pointing to the first key-value pair of this map. If
    this map is empty, the returned iterator will be the same as end().

    \sa constBegin(), end()
 */

/*!
    \fn QCborMap::const_iterator QCborMap::constBegin() const

    Returns a map iterator pointing to the first key-value pair of this map. If
    this map is empty, the returned iterator will be the same as constEnd().

    \sa begin(), constEnd()
 */

/*!
    \fn QCborMap::const_iterator QCborMap::begin() const

    Returns a map iterator pointing to the first key-value pair of this map. If
    this map is empty, the returned iterator will be the same as constEnd().

    \sa begin(), constEnd()
 */

/*!
    \fn QCborMap::const_iterator QCborMap::cbegin() const

    Returns a map iterator pointing to the first key-value pair of this map. If
    this map is empty, the returned iterator will be the same as constEnd().

    \sa begin(), constEnd()
 */

/*!
    \fn QCborMap::iterator QCborMap::end()

    Returns a map iterator representing an element just past the last element
    in the map.

    \sa begin(), constBegin(), find(), constFind()
 */

/*!
    \fn QCborMap::iterator QCborMap::constEnd() const

    Returns a map iterator representing an element just past the last element
    in the map.

    \sa begin(), constBegin(), find(), constFind()
 */

/*!
    \fn QCborMap::iterator QCborMap::end() const

    Returns a map iterator representing an element just past the last element
    in the map.

    \sa begin(), constBegin(), find(), constFind()
 */

/*!
    \fn QCborMap::iterator QCborMap::cend() const

    Returns a map iterator representing an element just past the last element
    in the map.

    \sa begin(), constBegin(), find(), constFind()
 */

/*!
    Constructs an empty CBOR Map object.

    \sa isEmpty()
 */
QCborMap::QCborMap() noexcept
    : d(nullptr)
{
}

/*!
    Creates a QCborMap object that is a copy of \a other.
 */
QCborMap::QCborMap(const QCborMap &other) noexcept
    : d(other.d)
{
}

/*!
    \fn QCborMap::QCborMap(std::initializer_list<value_type> args)

    Constructs a QCborMap with items from a brace-initialization list found in
    \a args, as in the following example:

    \code
        QCborMap map = {
            {0, "Hello"},
            {1, "World"},
            {"foo", nullptr},
            {"bar", QCborArray{0, 1, 2, 3, 4}}
        };
    \endcode
 */

/*!
    Destroys this QCborMap object and frees any associated resources it owns.
 */
QCborMap::~QCborMap()
{
}

/*!
    Replaces the contents of this object with a copy of \a other, then returns
    a reference to this object.
 */
QCborMap &QCborMap::operator=(const QCborMap &other) noexcept
{
    d = other.d;
    return *this;
}

/*!
    \fn void QCborMap::swap(QCborMap &other)

    Swaps the contents of this map and \a other.
 */

/*!
    \fn QCborValue QCborMap::toCborValue() const

    Explicitly constructs a \l QCborValue object that represents this map.
    This function is usually not necessary since QCborValue has a constructor
    for QCborMap, so the conversion is implicit.

    Converting QCborMap to QCborValue allows it to be used in any context where
    QCborValues can be used, including as keys and mapped types in QCborMap, as
    well as QCborValue::toCbor().

    \sa QCborValue::QCborValue(const QCborMap &)
 */

/*!
    \fn bool QCborMap::isEmpty() const

    Returns true if this map is empty (that is, size() is 0).

    \sa size(), clear()
 */

/*!
    Returns the number of elements in this map.

    \sa isEmpty()
 */
qsizetype QCborMap::size() const noexcept
{
    return d ? d->elements.size() / 2 : 0;
}

/*!
    Empties this map.

    \sa isEmpty()
 */
void QCborMap::clear()
{
    d.reset();
}

/*!
    Returns a list of all keys in this map.

    \sa QMap::keys(), QHash::keys()
 */
QVector<QCborValue> QCborMap::keys() const
{
    QVector<QCborValue> result;
    if (d) {
        result.reserve(size());
        for (qsizetype i = 0; i < d->elements.size(); i += 2)
            result << d->valueAt(i);
    }
    return result;
}

/*!
    \fn QCborValue QCborMap::value(qint64 key) const

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one. CBOR recommends using integer keys, since they occupy less
    space and are simpler to encode and decode.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one the return from function will reference. QCborMap does not allow
    inserting duplicate keys, but it is possible to create such a map by
    decoding a CBOR stream with them. They are usually not permitted and having
    duplicate keys is usually an indication of a problem in the sender.

    \sa operator[](qint64), find(qint64), constFind(qint64), remove(qint64), contains(qint64)
        value(QLatin1String), value(const QString &), value(const QCborValue &)
 */

/*!
    \fn QCborValue QCborMap::operator[](qint64 key) const

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one. CBOR recommends using integer keys, since they occupy less
    space and are simpler to encode and decode.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will return. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(qint64), find(qint64), constFind(qint64), remove(qint64), contains(qint64)
        operator[](QLatin1String), operator[](const QString &), operator[](const QCborOperator[] &)
 */

/*!
    \fn QCborValue QCborMap::take(qint64 key)

    Removes the key \a key and the corresponding value from the map and returns
    the value, if it is found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(qint64), operator[](qint64), find(qint64), contains(qint64),
      take(QLatin1String), take(const QString &), take(const QCborValue &), insert()
 */

/*!
    \fn void QCborMap::remove(qint64 key)

    Removes the key \a key and the corresponding value from the map, if it is
    found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(qint64), operator[](qint64), find(qint64), contains(qint64)
        remove(QLatin1String), remove(const QString &), remove(const QCborValue &)
 */

/*!
    \fn bool QCborMap::contains(qint64 key) const

    Returns true if this map contains a key-value pair identified by key \a
    key. CBOR recommends using integer keys, since they occupy less space and
    are simpler to encode and decode.

    \sa value(qint64), operator[](qint64), find(qint64), remove(qint64),
        contains(QLatin1String), remove(const QString &), remove(const QCborValue &)
 */

/*!
    Returns a QCborValueRef to the value in this map that corresponds to key \a
    key. CBOR recommends using integer keys, since they occupy less space and
    are simpler to encode and decode.

    QCborValueRef has the exact same API as \l QCborValue, with one important
    difference: if you assign new values to it, this map will be updated with
    that new value.

    If the map did not have a key equal to \a key, one is inserted and this
    function returns a reference to the new value, which will be a QCborValue
    with an undefined value. For that reason, it is not possible with this
    function to tell apart the situation where the key was not present from the
    situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one the return will reference. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(qint64), find(qint64), contains(qint64), remove(qint64),
        operator[](QLatin1String), operator[](const QString &), operator[](const QCborValue &)
 */
QCborValueRef QCborMap::operator[](qint64 key)
{
    auto it = find(key);
    if (it == constEnd()) {
        // insert element
        detach(it.item.i + 2);
        d->append(key);
        d->append(Undefined{});
    }
    return { d.data(), it.item.i };
}

/*!
    \fn QCborValue QCborMap::value(QLatin1String key) const
    \overload

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will return. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa operator[](QLatin1String), find(QLatin1String), constFind(QLatin1String),
        remove(QLatin1String), contains(QLatin1String)
        value(qint64), value(const QString &), value(const QCborValue &)
 */

/*!
    \fn QCborValue QCborMap::operator[](QLatin1String key) const
    \overload

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will return. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(QLatin1String), find(QLatin1String), constFind(QLatin1String),
        remove(QLatin1String), contains(QLatin1String)
        operator[](qint64), operator[](const QString &), operator[](const QCborOperator[] &)
 */

/*!
    \fn QCborValue QCborMap::take(QLatin1String key)

    Removes the key \a key and the corresponding value from the map and returns
    the value, if it is found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(QLatin1String), operator[](QLatin1String), find(QLatin1String), contains(QLatin1String),
      take(qint64), take(const QString &), take(const QCborValue &), insert()
 */

/*!
    \fn void QCborMap::remove(QLatin1String key)
    \overload

    Removes the key \a key and the corresponding value from the map, if it is
    found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(QLatin1String), operator[](QLatin1String), find(QLatin1String), contains(QLatin1String)
        remove(qint64), remove(const QString &), remove(const QCborValue &)
 */

/*!
    \fn bool QCborMap::contains(QLatin1String key) const
    \overload

    Returns true if this map contains a key-value pair identified by key \a
    key.

    \sa value(QLatin1String), operator[](QLatin1String), find(QLatin1String), remove(QLatin1String),
        contains(qint64), remove(const QString &), remove(const QCborValue &)
 */

/*!
    \overload

    Returns a QCborValueRef to the value in this map that corresponds to key \a
    key.

    QCborValueRef has the exact same API as \l QCborValue, with one important
    difference: if you assign new values to it, this map will be updated with
    that new value.

    If the map did not have a key equal to \a key, one is inserted and this
    function returns a reference to the new value, which will be a QCborValue
    with an undefined value. For that reason, it is not possible with this
    function to tell apart the situation where the key was not present from the
    situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one the return will reference. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(QLatin1String), find(QLatin1String), contains(QLatin1String), remove(QLatin1String),
        operator[](qint64), operator[](const QString &), operator[](const QCborValue &)
 */
QCborValueRef QCborMap::operator[](QLatin1String key)
{
    auto it = find(key);
    if (it == constEnd()) {
        // insert element
        detach(it.item.i + 2);
        d->append(key);
        d->append(Undefined{});
    }
    return { d.data(), it.item.i };
}

/*!
    \fn QCborValue QCborMap::value(const QString &key) const
    \overload

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will return. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa operator[](const QString &), find(const QString &), constFind(const QString &),
        remove(const QString &), contains(const QString &)
        value(qint64), value(QLatin1String), value(const QCborValue &)
 */

/*!
    \fn QCborValue QCborMap::operator[](const QString &key) const
    \overload

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will return. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QString &), find(const QString &), constFind(const QString &),
        remove(const QString &), contains(const QString &)
        operator[](qint64), operator[](QLatin1String), operator[](const QCborOperator[] &)
 */

/*!
    \fn QCborValue QCborMap::take(const QString &key)

    Removes the key \a key and the corresponding value from the map and returns
    the value, if it is found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QString &), operator[](const QString &), find(const QString &), contains(const QString &),
      take(QLatin1String), take(qint64), take(const QCborValue &), insert()
 */

/*!
    \fn void QCborMap::remove(const QString &key)
    \overload

    Removes the key \a key and the corresponding value from the map, if it is
    found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QString &), operator[](const QString &), find(const QString &),
        contains(const QString &)
        remove(qint64), remove(QLatin1String), remove(const QCborValue &)
 */

/*!
    \fn bool QCborMap::contains(const QString &key) const
    \overload

    Returns true if this map contains a key-value pair identified by key \a
    key.

    \sa value(const QString &), operator[](const QString &), find(const QString &),
        remove(const QString &),
        contains(qint64), remove(QLatin1String), remove(const QCborValue &)
 */

/*!
    \overload

    Returns a QCborValueRef to the value in this map that corresponds to key \a
    key.

    QCborValueRef has the exact same API as \l QCborValue, with one important
    difference: if you assign new values to it, this map will be updated with
    that new value.

    If the map did not have a key equal to \a key, one is inserted and this
    function returns a reference to the new value, which will be a QCborValue
    with an undefined value. For that reason, it is not possible with this
    function to tell apart the situation where the key was not present from the
    situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one the return will reference. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QString &), find(const QString &), contains(const QString &), remove(const QString &),
        operator[](qint64), operator[](QLatin1String), operator[](const QCborValue &)
 */
QCborValueRef QCborMap::operator[](const QString & key)
{
    auto it = find(key);
    if (it == constEnd()) {
        // insert element
        detach(it.item.i + 2);
        d->append(key);
        d->append(Undefined{});
    }
    return { d.data(), it.item.i };
}

/*!
    \fn QCborValue QCborMap::value(const QCborValue &key) const

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will return. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa operator[](const QCborValue &), find(const QCborValue &), constFind(const QCborValue &),
        remove(const QCborValue &), contains(const QCborValue &)
        value(qint64), value(QLatin1String), value(const QString &)
 */

/*!
    \fn QCborValue QCborMap::operator[](const QCborValue &key) const

    Returns the QCborValue element in this map that corresponds to key \a key,
    if there is one.

    If the map does not contain key \a key, this function returns a QCborValue
    containing an undefined value. For that reason, it is not possible with
    this function to tell apart the situation where the key was not present
    from the situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will return. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QCborValue &), find(const QCborValue &), constFind(const QCborValue &),
        remove(const QCborValue &), contains(const QCborValue &)
        operator[](qint64), operator[](QLatin1String), operator[](const QCborOperator[] &)
 */

/*!
    \fn QCborValue QCborMap::take(const QCborValue &key)

    Removes the key \a key and the corresponding value from the map and returns
    the value, if it is found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QCborValue &), operator[](const QCborValue &), find(const QCborValue &), contains(const QCborValue &),
      take(QLatin1String), take(const QString &), take(qint64), insert()
 */

/*!
    \fn void QCborMap::remove(const QCborValue &key)

    Removes the key \a key and the corresponding value from the map, if it is
    found. If the map contains no such key, this function does nothing.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will remove. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QCborValue &), operator[](const QCborValue &), find(const QCborValue &),
        contains(const QCborValue &)
        remove(qint64), remove(QLatin1String), remove(const QString &)
 */

/*!
    \fn bool QCborMap::contains(const QCborValue &key) const

    Returns true if this map contains a key-value pair identified by key \a
    key.

    \sa value(const QCborValue &), operator[](const QCborValue &), find(const QCborValue &),
        remove(const QCborValue &),
        contains(qint64), remove(QLatin1String), remove(const QString &)
 */

/*!
    \overload

    Returns a QCborValueRef to the value in this map that corresponds to key \a
    key.

    QCborValueRef has the exact same API as \l QCborValue, with one important
    difference: if you assign new values to it, this map will be updated with
    that new value.

    If the map did not have a key equal to \a key, one is inserted and this
    function returns a reference to the new value, which will be a QCborValue
    with an undefined value. For that reason, it is not possible with this
    function to tell apart the situation where the key was not present from the
    situation where the key was mapped to an undefined value.

    If the map contains more than one key equal to \a key, it is undefined
    which one the return will reference. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QCborValue &), find(const QCborValue &), contains(const QCborValue &), remove(const QCborValue &),
        operator[](qint64), operator[](QLatin1String), operator[](const QString &)
 */
QCborValueRef QCborMap::operator[](const QCborValue &key)
{
    auto it = find(key);
    if (it == constEnd()) {
        // insert element
        detach(it.item.i + 2);
        d->append(key);
        d->append(Undefined{});
    }
    return { d.data(), it.item.i };
}

/*!
    \fn QCborMap::iterator QCborMap::find(qint64 key)
    \fn QCborMap::const_iterator QCborMap::find(qint64 key) const

    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns end().

    CBOR recommends using integer keys, since they occupy less
    space and are simpler to encode and decode.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(qint64), operator[](qint64), constFind(qint64), remove(qint64), contains(qint64)
        value(QLatin1String), value(const QString &), value(const QCborValue &)
 */
QCborMap::iterator QCborMap::find(qint64 key)
{
    detach();
    auto it = constFind(key);
    return { d.data(), it.item.i };
}

/*!
    \fn QCborMap::iterator QCborMap::find(QLatin1String key)
    \fn QCborMap::const_iterator QCborMap::find(QLatin1String key) const
    \overload

    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns end().

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(QLatin1String), operator[](QLatin1String), constFind(QLatin1String),
        remove(QLatin1String), contains(QLatin1String)
        value(qint64), value(const QString &), value(const QCborValue &)
 */
QCborMap::iterator QCborMap::find(QLatin1String key)
{
    detach();
    auto it = constFind(key);
    return { d.data(), it.item.i };
}

/*!
    \fn QCborMap::iterator QCborMap::find(const QString & key)
    \fn QCborMap::const_iterator QCborMap::find(const QString & key) const
    \overload

    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns end().

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QString &), operator[](const QString &), constFind(const QString &),
        remove(const QString &), contains(const QString &)
        value(qint64), value(QLatin1String), value(const QCborValue &)
 */
QCborMap::iterator QCborMap::find(const QString & key)
{
    detach();
    auto it = constFind(key);
    return { d.data(), it.item.i };
}

/*!
    \fn QCborMap::iterator QCborMap::find(const QCborValue &key)
    \fn QCborMap::const_iterator QCborMap::find(const QCborValue &key) const
    \overload

    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns end().

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QCborValue &), operator[](const QCborValue &), constFind(const QCborValue &),
        remove(const QCborValue &), contains(const QCborValue &)
        value(qint64), value(QLatin1String), value(const QString &)
 */
QCborMap::iterator QCborMap::find(const QCborValue &key)
{
    detach();
    auto it = constFind(key);
    return { d.data(), it.item.i };
}

/*!
    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns constEnd().

    CBOR recommends using integer keys, since they occupy less
    space and are simpler to encode and decode.

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(qint64), operator[](qint64), find(qint64), remove(qint64), contains(qint64)
        value(QLatin1String), value(const QString &), value(const QCborValue &)
 */
QCborMap::const_iterator QCborMap::constFind(qint64 key) const
{
    for (qsizetype i = 0; i < 2 * size(); i += 2) {
        const auto &e = d->elements.at(i);
        if (e.type == QCborValue::Integer && e.value == key)
            return { d.data(), i + 1 };
    }
    return constEnd();
}

/*!
    \overload

    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns constEnd().

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(QLatin1String), operator[](QLatin1String), find(QLatin1String),
        remove(QLatin1String), contains(QLatin1String)
        value(qint64), value(const QString &), value(const QCborValue &)
 */
QCborMap::const_iterator QCborMap::constFind(QLatin1String key) const
{
    for (qsizetype i = 0; i < 2 * size(); i += 2) {
        if (d->stringEqualsElement(i, key))
            return { d.data(), i + 1 };
    }
    return constEnd();
}

/*!
    \overload

    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns constEnd().

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QString &), operator[](const QString &), find(const QString &),
        remove(const QString &), contains(const QString &)
        value(qint64), value(QLatin1String), value(const QCborValue &)
 */
QCborMap::const_iterator QCborMap::constFind(const QString & key) const
{
    for (qsizetype i = 0; i < 2 * size(); i += 2) {
        if (d->stringEqualsElement(i, key))
            return { d.data(), i + 1 };
    }
    return constEnd();
}

/*!
    \overload

    Returns a map iterator to the key-value pair whose key is \a key, if the
    map contains such a pair. If it doesn't, this function returns constEnd().

    If the map contains more than one key equal to \a key, it is undefined
    which one this function will find. QCborMap does not allow inserting
    duplicate keys, but it is possible to create such a map by decoding a CBOR
    stream with them. They are usually not permitted and having duplicate keys
    is usually an indication of a problem in the sender.

    \sa value(const QCborValue &), operator[](const QCborValue &), find(const QCborValue &),
        remove(const QCborValue &), contains(const QCborValue &),
        value(qint64), value(QLatin1String), value(const QString &)
 */
QCborMap::const_iterator QCborMap::constFind(const QCborValue &key) const
{
    for (qsizetype i = 0; i < 2 * size(); i += 2) {
        int cmp = d->compareElement(i, key);
        if (cmp == 0)
            return { d.data(), i + 1 };
    }
    return constEnd();
}

/*!
    \fn QCborMap::iterator QCborMap::insert(qint64 key, const QCborValue &value)
    \overload

    Inserts the key \a key and value \a value into this map and returns a map
    iterator pointing to the newly inserted pair.

    If the map already had a key equal to \a key, its value will be overwritten
    by \a value.

    \sa erase(), remove(qint64), value(qint64), operator[](qint64), find(qint64),
        contains(qint64), take(qint64), extract()
 */

/*!
    \fn QCborMap::iterator QCborMap::insert(QLatin1String key, const QCborValue &value)
    \overload

    Inserts the key \a key and value \a value into this map and returns a map
    iterator pointing to the newly inserted pair.

    If the map already had a key equal to \a key, its value will be overwritten
    by \a value.

    \sa erase(), remove(QLatin1String), value(QLatin1String), operator[](QLatin1String),
        find(QLatin1String), contains(QLatin1String), take(QLatin1String), extract()
 */

/*!
    \fn QCborMap::iterator QCborMap::insert(const QString &key, const QCborValue &value)
    \overload

    Inserts the key \a key and value \a value into this map and returns a map
    iterator pointing to the newly inserted pair.

    If the map already had a key equal to \a key, its value will be overwritten
    by \a value.

    \sa erase(), remove(const QString &), value(const QString &), operator[](const QString &),
        find(const QString &), contains(const QString &), take(const QString &), extract()
 */

/*!
    \fn QCborMap::iterator QCborMap::insert(const QCborValue &key, const QCborValue &value)
    \overload

    Inserts the key \a key and value \a value into this map and returns a map
    iterator pointing to the newly inserted pair.

    If the map already had a key equal to \a key, its value will be overwritten
    by \a value.

    \sa erase(), remove(const QCborValue &), value(const QCborValue &), operator[](const QCborValue &),
        find(const QCborValue &), contains(const QCborValue &), take(const QCborValue &), extract()
 */

/*!
    \fn QCborMap::iterator QCborMap::insert(value_type v)
    \overload

    Inserts the key-value pair in \a v into this map and returns a map iterator
    pointing to the newly inserted pair.

    If the map already had a key equal to \c{v.first}, its value will be
    overwritten by \c{v.second}.

    \sa operator[], erase(), extract()
 */


/*!
    \fn QCborMap::iterator QCborMap::erase(const_iterator it)

    Removes the key-value pair pointed to by the map iterator \a it and returns a
    pointer to the next element, after removal.

    \sa remove(), begin(), end(), insert(), extract()
 */

/*!
    \overload

    Removes the key-value pair pointed to by the map iterator \a it and returns a
    pointer to the next element, after removal.

    \sa remove(), begin(), end(), insert()
 */
QCborMap::iterator QCborMap::erase(QCborMap::iterator it)
{
    detach();

    // remove both key and value
    // ### optimize?
    d->removeAt(it.item.i - 1);
    d->removeAt(it.item.i - 1);
    return it;
}

/*!
    \fn QCborValue QCborMap::extract(iterator it)
    \fn QCborValue QCborMap::extract(const_iterator it)

    Extracts a value from the map at the position indicated by iterator \a it
    and returns the value so extracted.

    \sa insert(), erase(), take(), remove()
 */
QCborValue QCborMap::extract(iterator it)
{
    detach();
    QCborValue v = d->extractAt(it.item.i);
    // remove both key and value
    // ### optimize?
    d->removeAt(it.item.i - 1);
    d->removeAt(it.item.i - 1);

    return v;
}

/*!
    \fn bool QCborMap::empty() const

    Synonym for isEmpty(). This function is provided for compatibility with
    generic code that uses the Standard Library API.

    Returns true if this map is empty (size() == 0).

    \sa isEmpty(), size()
 */

/*!
    \fn int QCborMap::compare(const QCborMap &other) const

    Compares this map and \a other, comparing each element in sequence, and
    returns an integer that indicates whether this map should be sorted prior
    to (if the result is negative) or after \a other (if the result is
    positive). If this function returns 0, the two maps are equal and contain
    the same elements.

    Note that CBOR maps are unordered, which means that two maps containing the
    very same pairs but in different order will still compare differently. To
    avoid this, it is recommended to insert elements into the map in a
    predictable order, such as by ascending key value. In fact, maps with keys
    in sorted order are required for Canonical CBOR representation.

    For more information on CBOR sorting order, see QCborValue::compare().

    \sa QCborValue::compare(), QCborArray::compare(), operator==()
 */

/*!
    \fn bool QCborMap::operator==(const QCborMap &other) const

    Compares this map and \a other, comparing each element in sequence, and
    returns true if the two maps contains the same elements in the same order,
    false otherwise.

    Note that CBOR maps are unordered, which means that two maps containing the
    very same pairs but in different order will still compare differently. To
    avoid this, it is recommended to insert elements into the map in a
    predictable order, such as by ascending key value. In fact, maps with keys
    in sorted order are required for Canonical CBOR representation.

    For more information on CBOR equality in Qt, see, QCborValue::compare().

    \sa compare(), QCborValue::operator==(), QCborMap::operator==(),
        operator!=(), operator<()
 */

/*!
    \fn bool QCborMap::operator!=(const QCborMap &other) const

    Compares this map and \a other, comparing each element in sequence, and
    returns true if the two maps contains any different elements or elements in
    different orders, false otherwise.

    Note that CBOR maps are unordered, which means that two maps containing the
    very same pairs but in different order will still compare differently. To
    avoid this, it is recommended to insert elements into the map in a
    predictable order, such as by ascending key value. In fact, maps with keys
    in sorted order are required for Canonical CBOR representation.

    For more information on CBOR equality in Qt, see, QCborValue::compare().

    \sa compare(), QCborValue::operator==(), QCborMap::operator==(),
        operator==(), operator<()
 */

/*!
    \fn bool QCborMap::operator<(const QCborMap &other) const

    Compares this map and \a other, comparing each element in sequence, and
    returns true if this map should be sorted before \a other, false
    otherwise.

    Note that CBOR maps are unordered, which means that two maps containing the
    very same pairs but in different order will still compare differently. To
    avoid this, it is recommended to insert elements into the map in a
    predictable order, such as by ascending key value. In fact, maps with keys
    in sorted order are required for Canonical CBOR representation.

    For more information on CBOR sorting order, see QCborValue::compare().

    \sa compare(), QCborValue::operator==(), QCborMap::operator==(),
        operator==(), operator!=()
 */

void QCborMap::detach(qsizetype reserved)
{
    d = QCborContainerPrivate::detach(d.data(), reserved ? reserved : size() * 2);
}

/*!
    \class QCborMap::Iterator
    \inmodule QtCore
    \ingroup cbor
    \reentrant
    \since 5.12

    \brief The QCborMap::Iterator class provides an STL-style non-const iterator for QCborMap.

    QCborMap::Iterator allows you to iterate over a QCborMap and to modify the
    value (but not the key) stored under a particular key. If you want to
    iterate over a const QCborMap, you should use QCborMap::ConstIterator. It
    is generally good practice to use QCborMap::ConstIterator on a non-const
    QCborMap as well, unless you need to change the QCborMap through the
    iterator. Const iterators are slightly faster, and improve code
    readability.

    You must initialize the iterator using a QCborMap function like
    QCborMap::begin(), QCborMap::end(), or QCborMap::find() before you can
    start iterating..

    Multiple iterators can be used on the same object. Existing iterators will however
    become dangling once the object gets modified.

    \sa QCborMap::ConstIterator
*/

/*!
    \typedef QCborMap::Iterator::difference_type
    \internal
*/

/*!
    \typedef QCborMap::Iterator::iterator_category

    A synonym for \e {std::random_access_iterator_tag} indicating
    this iterator is a random-access iterator.
*/

/*!
    \typedef QCborMap::Iterator::reference
    \internal
*/

/*!
    \typedef QCborMap::Iterator::value_type
    \internal
*/

/*!
    \typedef QCborMap::Iterator::pointer
    \internal
*/

/*!
    \fn QCborMap::Iterator::Iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QCborMap::begin(), QCborMap::end()
*/

/*!
    \fn QCborMap::Iterator::Iterator(const Iterator &other)

    Constructs an iterator as a copy of \a other.
 */

/*!
    \fn QCborMap::Iterator &QCborMap::Iterator::operator=(const Iterator &other)

    Makes this iterator a copy of \a other and returns a reference to this
    iterator.
 */

/*!
    \fn QCborValue QCborMap::Iterator::key() const

    Returns the current item's key.

    There is no direct way of changing an item's key through an iterator,
    although it can be done by calling QCborMap::erase() followed by
    QCborMap::insert().

    \sa value()
*/

/*!
    \fn QCborValueRef QCborMap::Iterator::value() const

    Returns a modifiable reference to the current item's value.

    You can change the value for a key by using value() on the left side of an
    assignment.

    The return value is of type QCborValueRef, a helper class for QCborArray
    and QCborMap. When you get an object of type QCborValueRef, you can use it
    as if it were a reference to a QCborValue. If you assign to it, the
    assignment will apply to the element in the QCborArray or QCborMap from
    which you got the reference.

    \sa key(), operator*()
*/

/*!
    \fn QCborMap::Iterator::value_type QCborMap::Iterator::operator*() const

    Returns a pair containing the current item's key and a modifiable reference
    to the current item's value.

    The second element of the pair is of type QCborValueRef, a helper class for
    QCborArray and QCborMap. When you get an object of type QCborValueRef, you
    can use it as if it were a reference to a QCborValue. If you assign to it,
    the assignment will apply to the element in the QCborArray or QCborMap from
    which you got the reference.

    \sa key(), value()
*/

/*!
    \fn QCborValueRef *QCborMap::Iterator::operator->() const

    Returns a pointer to a modifiable reference to the current pair's value.
*/

/*!
    \fn bool QCborMap::Iterator::operator==(const Iterator &other) const
    \fn bool QCborMap::Iterator::operator==(const ConstIterator &other) const

    Returns \c true if \a other points to the same entry in the map as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QCborMap::Iterator::operator!=(const Iterator &other) const
    \fn bool QCborMap::Iterator::operator!=(const ConstIterator &other) const

    Returns \c true if \a other points to a different entry in the map than
    this iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QCborMap::Iterator::operator<(const Iterator& other) const
    \fn bool QCborMap::Iterator::operator<(const ConstIterator& other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs before the entry pointed to by the \a other iterator.
*/

/*!
    \fn bool QCborMap::Iterator::operator<=(const Iterator& other) const
    \fn bool QCborMap::Iterator::operator<=(const ConstIterator& other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs before or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn bool QCborMap::Iterator::operator>(const Iterator& other) const
    \fn bool QCborMap::Iterator::operator>(const ConstIterator& other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs after the entry pointed to by the \a other iterator.
 */

/*!
    \fn bool QCborMap::Iterator::operator>=(const Iterator& other) const
    \fn bool QCborMap::Iterator::operator>=(const ConstIterator& other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs after or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn QCborMap::Iterator &QCborMap::Iterator::operator++()

    The prefix ++ operator, \c{++i}, advances the iterator to the next item in
    the map and returns this iterator.

    Calling this function on QCborMap::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QCborMap::Iterator QCborMap::Iterator::operator++(int)
    \overload

    The postfix ++ operator, \c{i++}, advances the iterator to the next item in
    the map and returns an iterator to the previously current item.
*/

/*!
    \fn QCborMap::Iterator QCborMap::Iterator::operator--()

    The prefix -- operator, \c{--i}, makes the preceding item current and
    returns this iterator.

    Calling this function on QCborMap::begin() leads to undefined results.

    \sa operator++()
*/

/*!
    \fn QCborMap::Iterator QCborMap::Iterator::operator--(int)
    \overload

    The postfix -- operator, \c{i--}, makes the preceding item current and
    returns an iterator pointing to the previously current item.
*/

/*!
    \fn QCborMap::Iterator QCborMap::Iterator::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from this
    iterator. If \a j is negative, the iterator goes backward.

    \sa operator-()
*/

/*!
    \fn QCborMap::Iterator QCborMap::Iterator::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from this
    iterator. If \a j is negative, the iterator goes forward.

    \sa operator+()
*/

/*!
    \fn qsizetype QCborMap::Iterator::operator-(QCborMap::Iterator j) const

    Returns the position of the item at iterator \a j relative to the item
    at this iterator. If the item at \a j is forward of this time, the returned
    value is negative.

    \sa operator+()
*/

/*!
    \fn QCborMap::Iterator &QCborMap::Iterator::operator+=(qsizetype j)

    Advances the iterator by \a j items. If \a j is negative, the iterator goes
    backward. Returns a reference to this iterator.

    \sa operator-=(), operator+()
*/

/*!
    \fn QCborMap::Iterator &QCborMap::Iterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j items. If \a j is negative, the iterator
    goes forward. Returns a reference to this iterator.

    \sa operator+=(), operator-()
*/

/*!
    \class QCborMap::ConstIterator
    \inmodule QtCore
    \ingroup cbor
    \since 5.12

    \brief The QCborMap::ConstIterator class provides an STL-style const iterator for QCborMap.

    QCborMap::ConstIterator allows you to iterate over a QCborMap. If you want
    to modify the QCborMap as you iterate over it, you must use
    QCborMap::Iterator instead. It is generally good practice to use
    QCborMap::ConstIterator, even on a non-const QCborMap, when you don't need
    to change the QCborMap through the iterator. Const iterators are slightly
    faster and improve code readability.

    You must initialize the iterator using a QCborMap function like
    QCborMap::begin(), QCborMap::end(), or QCborMap::find() before you can
    start iterating..

    Multiple iterators can be used on the same object. Existing iterators
    will however become dangling if the object gets modified.

    \sa QCborMap::Iterator
*/

/*!
    \typedef QCborMap::ConstIterator::difference_type
    \internal
*/

/*!
    \typedef QCborMap::ConstIterator::iterator_category

    A synonym for \e {std::random_access_iterator_tag} indicating
    this iterator is a random-access iterator.
*/

/*!
    \typedef QCborMap::ConstIterator::reference
    \internal
*/

/*!
    \typedef QCborMap::ConstIterator::value_type
    \internal
*/

/*!
    \typedef QCborMap::ConstIterator::pointer
    \internal
*/

/*!
    \fn QCborMap::ConstIterator::ConstIterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QCborMap::constBegin(), QCborMap::constEnd()
*/

/*!
    \fn QCborMap::ConstIterator::ConstIterator(const ConstIterator &other)

    Constructs an iterator as a copy of \a other.
 */

/*!
    \fn QCborMap::ConstIterator &QCborMap::ConstIterator::operator=(const ConstIterator &other)

    Makes this iterator a copy of \a other and returns a reference to this
    iterator.
 */

/*!
    \fn QString QCborMap::ConstIterator::key() const

    Returns the current item's key.

    \sa value()
*/

/*!
    \fn QCborValue QCborMap::ConstIterator::value() const

    Returns the current item's value.

    \sa key(), operator*()
*/

/*!
    \fn QCborMap::ConstIterator::value_type QCborMap::ConstIterator::operator*() const

    Returns a pair containing the curent item's key and value.

    \sa key(), value()
 */

/*!
    \fn const QCborValueRef *QCborMap::ConstIterator::operator->() const

    Returns a pointer to the current pair's value.
 */

/*!
    \fn bool QCborMap::ConstIterator::operator==(const ConstIterator &other) const
    \fn bool QCborMap::ConstIterator::operator==(const Iterator &other) const

    Returns \c true if \a other points to the same entry in the map as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QCborMap::ConstIterator::operator!=(const ConstIterator &other) const
    \fn bool QCborMap::ConstIterator::operator!=(const Iterator &other) const

    Returns \c true if \a other points to a different entry in the map than
    this iterator; otherwise returns \c false.

    \sa operator==()
 */

/*!
    \fn bool QCborMap::ConstIterator::operator<(const Iterator &other) const
    \fn bool QCborMap::ConstIterator::operator<(const ConstIterator &other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs before the entry pointed to by the \a other iterator.
*/

/*!
    \fn bool QCborMap::ConstIterator::operator<=(const Iterator &other) const
    \fn bool QCborMap::ConstIterator::operator<=(const ConstIterator &other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs before or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn bool QCborMap::ConstIterator::operator>(const Iterator &other) const
    \fn bool QCborMap::ConstIterator::operator>(const ConstIterator &other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs after the entry pointed to by the \a other iterator.
*/

/*!
    \fn bool QCborMap::ConstIterator::operator>=(const Iterator &other) const
    \fn bool QCborMap::ConstIterator::operator>=(const ConstIterator &other) const

    Returns \c true if the entry in the map pointed to by this iterator
    occurs after or is the same entry as is pointed to by the \a other
    iterator.
*/

/*!
    \fn QCborMap::ConstIterator &QCborMap::ConstIterator::operator++()

    The prefix ++ operator, \c{++i}, advances the iterator to the next item in
    the map and returns this iterator.

    Calling this function on QCborMap::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QCborMap::ConstIterator QCborMap::ConstIterator::operator++(int)
    \overload

    The postfix ++ operator, \c{i++}, advances the iterator to the next item in
    the map and returns an iterator to the previously current item.
 */

/*!
    \fn QCborMap::ConstIterator &QCborMap::ConstIterator::operator--()

    The prefix -- operator, \c{--i}, makes the preceding item current and
    returns this iterator.

    Calling this function on QCborMap::begin() leads to undefined results.

    \sa operator++()
*/

/*!
    \fn QCborMap::ConstIterator QCborMap::ConstIterator::operator--(int)
    \overload

    The postfix -- operator, \c{i--}, makes the preceding item current and
    returns an iterator pointing to the previously current item.
 */

/*!
    \fn QCborMap::ConstIterator QCborMap::ConstIterator::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from this
    iterator. If \a j is negative, the iterator goes backward.

    \sa operator-()
*/

/*!
    \fn QCborMap::ConstIterator QCborMap::ConstIterator::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from this
    iterator. If \a j is negative, the iterator goes forward.

    \sa operator+()
*/

/*!
    \fn qsizetype QCborMap::ConstIterator::operator-(QCborMap::ConstIterator j) const

    Returns the position of the item at iterator \a j relative to the item
    at this iterator. If the item at \a j is forward of this time, the returned
    value is negative.

    \sa operator+()
*/

/*!
    \fn QCborMap::ConstIterator &QCborMap::ConstIterator::operator+=(qsizetype j)

    Advances the iterator by \a j items. If \a j is negative, the iterator goes
    backward. Returns a reference to this iterator.

    \sa operator-=(), operator+()
*/

/*!
    \fn QCborMap::ConstIterator &QCborMap::ConstIterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j items. If \a j is negative, the iterator
    goes forward. Returns a reference to this iterator.

    \sa operator+=(), operator-()
*/

uint qHash(const QCborMap &map, uint seed)
{
    return qHashRange(map.begin(), map.end(), seed);
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, const QCborMap &m)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QCborMap{";
    const char *open = "{";
    for (auto pair : m) {
        dbg << open << pair.first <<  ", " << pair.second << '}';
        open = ", {";
    }
    return dbg << '}';
}
#endif

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &stream, const QCborMap &value)
{
    stream << value.toCborValue().toCbor();
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QCborMap &value)
{
    QByteArray buffer;
    stream >> buffer;
    QCborParserError parseError{};
    value = QCborValue::fromCbor(buffer, &parseError).toMap();
    if (parseError.error)
        stream.setStatus(QDataStream::ReadCorruptData);
    return stream;
}
#endif

QT_END_NAMESPACE
