// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qstringlist.h>
#include <qdebug.h>
#include <qvariant.h>
#include <qcbormap.h>
#include <qmap.h>
#include <qhash.h>

#include <private/qcborvalue_p.h>
#include "qjsonwriter_p.h"
#include "qjson_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \class QJsonObject
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \ingroup qtserialization
    \reentrant
    \since 5.0

    \brief The QJsonObject class encapsulates a JSON object.

    \compares equality
    \compareswith equality QJsonValue QJsonValueConstRef
    \endcompareswith

    A JSON object is a list of key value pairs, where the keys are unique strings
    and the values are represented by a QJsonValue.

    A QJsonObject can be converted to and from a QVariantMap. You can query the
    number of (key, value) pairs with size(), insert(), and remove() entries from it
    and iterate over its content using the standard C++ iterator pattern.

    QJsonObject is an implicitly shared class, and shares the data with the document
    it has been created from as long as it is not being modified.

    You can convert the object to and from text based JSON through QJsonDocument.

    \sa {JSON Support in Qt}, {Saving and Loading a Game}
*/

/*!
    \typedef QJsonObject::Iterator

    Qt-style synonym for QJsonObject::iterator.
*/

/*!
    \typedef QJsonObject::ConstIterator

    Qt-style synonym for QJsonObject::const_iterator.
*/

/*!
    \typedef QJsonObject::key_type

    Typedef for QString. Provided for STL compatibility.
*/

/*!
    \typedef QJsonObject::mapped_type

    Typedef for QJsonValue. Provided for STL compatibility.
*/

/*!
    \typedef QJsonObject::size_type

    Typedef for qsizetype. Provided for STL compatibility.
*/


/*!
    Constructs an empty JSON object.

    \sa isEmpty()
 */
QJsonObject::QJsonObject() = default;

/*!
    \fn QJsonObject::QJsonObject(std::initializer_list<std::pair<QString, QJsonValue> > args)
    \since 5.4
    Constructs a QJsonObject instance initialized from \a args initialization list.
    For example:
    \code
    QJsonObject object
    {
        {"property1", 1},
        {"property2", 2}
    };
    \endcode
*/

/*!
    \internal
 */
QJsonObject::QJsonObject(QCborContainerPrivate *object)
    : o(object)
{
}

/*!
    Destroys the object.
 */
QJsonObject::~QJsonObject() = default;

QJsonObject::QJsonObject(std::initializer_list<std::pair<QString, QJsonValue> > args)
{
    for (const auto &arg : args)
        insert(arg.first, arg.second);
}

/*!
    Creates a copy of \a other.

    Since QJsonObject is implicitly shared, the copy is shallow
    as long as the object does not get modified.
 */
QJsonObject::QJsonObject(const QJsonObject &other) noexcept = default;

/*!
    \since 5.10

    Move-constructs a QJsonObject from \a other.
*/
QJsonObject::QJsonObject(QJsonObject &&other) noexcept
    : o(other.o)
{
    other.o = nullptr;
}

/*!
    Assigns \a other to this object.
 */
QJsonObject &QJsonObject::operator =(const QJsonObject &other) noexcept = default;


/*!
    \fn QJsonObject &QJsonObject::operator =(QJsonObject &&other)
    \since 5.10

    Move-assigns \a other to this object.
*/

/*!
    \fn void QJsonObject::swap(QJsonObject &other)
    \since 5.10
    \memberswap{object}
*/

#ifndef QT_NO_VARIANT
/*!
    Converts the variant map \a map to a QJsonObject.

    The keys in \a map will be used as the keys in the JSON object,
    and the QVariant values will be converted to JSON values.

    \note Conversion from \l QVariant is not completely lossless. Please see
    the documentation in QJsonValue::fromVariant() for more information.

    \sa fromVariantHash(), toVariantMap(), QJsonValue::fromVariant()
 */
QJsonObject QJsonObject::fromVariantMap(const QVariantMap &map)
{
    return QJsonPrivate::Variant::toJsonObject(map);
}

/*!
    Converts this object to a QVariantMap.

    Returns the created map.

    \sa toVariantHash()
 */
QVariantMap QJsonObject::toVariantMap() const
{
    return QCborMap::fromJsonObject(*this).toVariantMap();
}

/*!
    Converts the variant hash \a hash to a QJsonObject.
    \since 5.5

    The keys in \a hash will be used as the keys in the JSON object,
    and the QVariant values will be converted to JSON values.

    \note Conversion from \l QVariant is not completely lossless. Please see
    the documentation in QJsonValue::fromVariant() for more information.

    \sa fromVariantMap(), toVariantHash(), QJsonValue::fromVariant()
 */
QJsonObject QJsonObject::fromVariantHash(const QVariantHash &hash)
{
    // ### this is implemented the trivial way, not the most efficient way

    QJsonObject object;
    for (QVariantHash::const_iterator it = hash.constBegin(); it != hash.constEnd(); ++it)
        object.insert(it.key(), QJsonValue::fromVariant(it.value()));
    return object;
}

/*!
    Converts this object to a QVariantHash.
    \since 5.5

    Returns the created hash.

    \sa toVariantMap()
 */
QVariantHash QJsonObject::toVariantHash() const
{
    return QCborMap::fromJsonObject(*this).toVariantHash();
}
#endif // !QT_NO_VARIANT

/*!
    Returns a list of all keys in this object.

    The list is sorted alphabetically.
 */
QStringList QJsonObject::keys() const
{
    QStringList keys;
    if (o) {
        keys.reserve(o->elements.size() / 2);
        for (qsizetype i = 0, end = o->elements.size(); i < end; i += 2)
            keys.append(o->stringAt(i));
    }
    return keys;
}

/*!
    Returns the number of (key, value) pairs stored in the object.
 */
qsizetype QJsonObject::size() const
{
    return o ? o->elements.size() / 2 : 0;
}

/*!
    Returns \c true if the object is empty. This is the same as size() == 0.

    \sa size()
 */
bool QJsonObject::isEmpty() const
{
    return !o || o->elements.isEmpty();
}

template<typename String>
static qsizetype indexOf(const QExplicitlySharedDataPointer<QCborContainerPrivate> &o,
                         String key, bool *keyExists)
{
    const auto begin = QJsonPrivate::ConstKeyIterator(o->elements.constBegin());
    const auto end = QJsonPrivate::ConstKeyIterator(o->elements.constEnd());

    const auto it = std::lower_bound(
                begin, end, key,
                [&](const QJsonPrivate::ConstKeyIterator::value_type &e, const String &key) {
        return o->stringCompareElement(e.key(), key, QtCbor::Comparison::ForOrdering) < 0;
    });

    *keyExists = (it != end) && o->stringEqualsElement((*it).key(), key);
    return it.it - begin.it;
}

/*!
    Returns a QJsonValue representing the value for the key \a key.

    The returned QJsonValue is QJsonValue::Undefined if the key does not exist.

    \sa QJsonValue, QJsonValue::isUndefined()
 */
QJsonValue QJsonObject::value(const QString &key) const
{
    return value(QStringView(key));
}

/*!
    \overload
    \since 5.14
*/
QJsonValue QJsonObject::value(QStringView key) const
{
    return valueImpl(key);
}

/*!
    \overload
    \since 5.7
*/
QJsonValue QJsonObject::value(QLatin1StringView key) const
{
    return valueImpl(key);
}

/*!
    \internal
*/
template <typename T>
QJsonValue QJsonObject::valueImpl(T key) const
{
    if (!o)
        return QJsonValue(QJsonValue::Undefined);

    bool keyExists;
    auto i = indexOf(o, key, &keyExists);
    if (!keyExists)
        return QJsonValue(QJsonValue::Undefined);
    return QJsonPrivate::Value::fromTrustedCbor(o->valueAt(i + 1));
}

/*!
    Returns a QJsonValue representing the value for the key \a key.

    This does the same as value().

    The returned QJsonValue is QJsonValue::Undefined if the key does not exist.

    \sa value(), QJsonValue, QJsonValue::isUndefined()
 */
QJsonValue QJsonObject::operator [](const QString &key) const
{
    return (*this)[QStringView(key)];
}

/*!
    \fn QJsonValue QJsonObject::operator [](QStringView key) const

    \overload
    \since 5.14
*/

/*!
    \fn QJsonValue QJsonObject::operator [](QLatin1StringView key) const

    \overload
    \since 5.7
*/

/*!
    Returns a reference to the value for \a key. If there is no value with key
    \a key in the object, one is created with a QJsonValue::Null value and then
    returned.

    The return value is of type QJsonValueRef, a helper class for QJsonArray
    and QJsonObject. When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the element in the QJsonArray or QJsonObject
    from which you got the reference.

    \sa value()
 */
QJsonValueRef QJsonObject::operator [](const QString &key)
{
    return (*this)[QStringView(key)];
}

/*!
    \overload
    \since 5.14
*/
QJsonValueRef QJsonObject::operator [](QStringView key)
{
    return atImpl(key);
}

/*!
    \overload
    \since 5.7
*/
QJsonValueRef QJsonObject::operator [](QLatin1StringView key)
{
    return atImpl(key);
}

/*!
    \internal
*/
template <typename T>
QJsonValueRef QJsonObject::atImpl(T key)
{
    if (!o)
        o = new QCborContainerPrivate;

    bool keyExists = false;
    auto index = indexOf(o, key, &keyExists);
    if (!keyExists) {
        detach(o->elements.size() / 2 + 1);
        o->insertAt(index, key);
        o->insertAt(index + 1, QCborValue::fromJsonValue(QJsonValue()));
    }
    // detaching will happen if and when this QJsonValueRef is assigned to
    return QJsonValueRef(this, index / 2);
}

/*!
    Inserts a new item with the key \a key and a value of \a value.

    If there is already an item with the key \a key, then that item's value
    is replaced with \a value.

    Returns an iterator pointing to the inserted item.

    If the value is QJsonValue::Undefined, it will cause the key to get removed
    from the object. The returned iterator will then point to end().

    \sa remove(), take(), QJsonObject::iterator, end()
 */
QJsonObject::iterator QJsonObject::insert(const QString &key, const QJsonValue &value)
{
    return insert(QStringView(key), value);
}

/*!
    \overload
    \since 5.14
*/
QJsonObject::iterator QJsonObject::insert(QStringView key, const QJsonValue &value)
{
    return insertImpl(key, value);
}

/*!
    \overload
    \since 5.14
*/
QJsonObject::iterator QJsonObject::insert(QLatin1StringView key, const QJsonValue &value)
{
    return insertImpl(key, value);
}

/*!
    \internal
*/
template <typename T>
QJsonObject::iterator QJsonObject::insertImpl(T key, const QJsonValue &value)
{
    if (value.type() == QJsonValue::Undefined) {
        remove(key);
        return end();
    }
    bool keyExists = false;
    auto pos = o ? indexOf(o, key, &keyExists) : 0;
    return insertAt(pos, key, value, keyExists);
}

/*!
    \internal
 */
template <typename T>
QJsonObject::iterator QJsonObject::insertAt(qsizetype pos, T key, const QJsonValue &value, bool keyExists)
{
    if (o)
        detach(o->elements.size() / 2 + (keyExists ? 0 : 1));
    else
        o = new QCborContainerPrivate;

    if (keyExists) {
        o->replaceAt(pos + 1, QCborValue::fromJsonValue(value));
    } else {
        o->insertAt(pos, key);
        o->insertAt(pos + 1, QCborValue::fromJsonValue(value));
    }
    return {this, pos / 2};
}

/*!
    Removes \a key from the object.

    \sa insert(), take()
 */
void QJsonObject::remove(const QString &key)
{
    remove(QStringView(key));
}

/*!
    \overload
    \since 5.14
*/
void QJsonObject::remove(QStringView key)
{
    removeImpl(key);
}

/*!
    \overload
    \since 5.14
*/
void QJsonObject::remove(QLatin1StringView key)
{
    removeImpl(key);
}

/*!
    \internal
*/
template <typename T>
void QJsonObject::removeImpl(T key)
{
    if (!o)
        return;

    bool keyExists;
    auto index = indexOf(o, key, &keyExists);
    if (!keyExists)
        return;

    removeAt(index);
}

/*!
    Removes \a key from the object.

    Returns a QJsonValue containing the value referenced by \a key.
    If \a key was not contained in the object, the returned QJsonValue
    is QJsonValue::Undefined.

    \sa insert(), remove(), QJsonValue
 */
QJsonValue QJsonObject::take(const QString &key)
{
    return take(QStringView(key));
}

/*!
    \overload
    \since 5.14
*/
QJsonValue QJsonObject::take(QStringView key)
{
    return takeImpl(key);
}

/*!
    \overload
    \since 5.14
*/
QJsonValue QJsonObject::take(QLatin1StringView key)
{
    return takeImpl(key);
}

/*!
    \internal
*/
template <typename T>
QJsonValue QJsonObject::takeImpl(T key)
{
    if (!o)
        return QJsonValue(QJsonValue::Undefined);

    bool keyExists;
    auto index = indexOf(o, key, &keyExists);
    if (!keyExists)
        return QJsonValue(QJsonValue::Undefined);

    detach();
    const QJsonValue v = QJsonPrivate::Value::fromTrustedCbor(o->extractAt(index + 1));
    removeAt(index);
    return v;
}

/*!
    Returns \c true if the object contains key \a key.

    \sa insert(), remove(), take()
 */
bool QJsonObject::contains(const QString &key) const
{
    return contains(QStringView(key));
}

/*!
    \overload
    \since 5.14
*/
bool QJsonObject::contains(QStringView key) const
{
    return containsImpl(key);
}

/*!
    \overload
    \since 5.7
*/
bool QJsonObject::contains(QLatin1StringView key) const
{
    return containsImpl(key);
}

/*!
    \internal
*/
template <typename T>
bool QJsonObject::containsImpl(T key) const
{
    if (!o)
        return false;

    bool keyExists;
    indexOf(o, key, &keyExists);
    return keyExists;
}

/*!
    \fn bool QJsonObject::operator==(const QJsonObject &lhs, const QJsonObject &rhs)

    Returns \c true if \a lhs object is equal to \a rhs, \c false otherwise.
*/
bool comparesEqual(const QJsonObject &lhs, const QJsonObject &rhs)
{
    if (lhs.o == rhs.o)
        return true;

    if (!lhs.o)
        return !rhs.o->elements.size();
    if (!rhs.o)
        return !lhs.o->elements.size();
    if (lhs.o->elements.size() != rhs.o->elements.size())
        return false;

    for (qsizetype i = 0, end = lhs.o->elements.size(); i < end; ++i) {
        if (lhs.o->valueAt(i) != rhs.o->valueAt(i))
            return false;
    }

    return true;
}

/*!
    \fn bool QJsonObject::operator!=(const QJsonObject &lhs, const QJsonObject &rhs)

    Returns \c true if \a lhs object is not equal to \a rhs, \c false otherwise.
*/

/*!
    Removes the (key, value) pair pointed to by the iterator \a it
    from the map, and returns an iterator to the next item in the
    map.

    \sa remove()
 */
QJsonObject::iterator QJsonObject::erase(QJsonObject::iterator it)
{
    removeAt(it.item.index * 2);

    // index hasn't changed; the container pointer shouldn't have changed
    // because we shouldn't have detached (detaching happens on obtaining a
    // non-const iterator). But just in case we did, reload the pointer.
    return { this, qsizetype(it.item.index) };
}

/*!
    Returns an iterator pointing to the item with key \a key in the
    map.

    If the map contains no item with key \a key, the function
    returns end().
 */
QJsonObject::iterator QJsonObject::find(const QString &key)
{
    return find(QStringView(key));
}

/*!
    \overload
    \since 5.14
*/
QJsonObject::iterator QJsonObject::find(QStringView key)
{
    return findImpl(key);
}

/*!
    \overload
    \since 5.7
*/
QJsonObject::iterator QJsonObject::find(QLatin1StringView key)
{
    return findImpl(key);
}

/*!
    \internal
*/
template <typename T>
QJsonObject::iterator QJsonObject::findImpl(T key)
{
    bool keyExists = false;
    auto index = o ? indexOf(o, key, &keyExists) : 0;
    if (!keyExists)
        return end();
    detach();
    return {this, index / 2};
}

/*! \fn QJsonObject::const_iterator QJsonObject::find(const QString &key) const

    \overload
*/

/*! \fn QJsonObject::const_iterator QJsonObject::find(QStringView key) const

    \overload
    \since 5.14
*/

/*! \fn QJsonObject::const_iterator QJsonObject::find(QLatin1StringView key) const

    \overload
    \since 5.7
*/

/*!
    Returns a const iterator pointing to the item with key \a key in the
    map.

    If the map contains no item with key \a key, the function
    returns constEnd().
 */
QJsonObject::const_iterator QJsonObject::constFind(const QString &key) const
{
    return constFind(QStringView(key));
}

/*!
    \overload
    \since 5.14
*/
QJsonObject::const_iterator QJsonObject::constFind(QStringView key) const
{
    return constFindImpl(key);
}

/*!
    \overload
    \since 5.7
*/
QJsonObject::const_iterator QJsonObject::constFind(QLatin1StringView key) const
{
    return constFindImpl(key);
}

/*!
    \internal
*/
template <typename T>
QJsonObject::const_iterator QJsonObject::constFindImpl(T key) const
{
    bool keyExists = false;
    auto index = o ? indexOf(o, key, &keyExists) : 0;
    if (!keyExists)
        return end();
    return {this, index / 2};
}

/*! \fn qsizetype QJsonObject::count() const

    \overload

    Same as size().
*/

/*! \fn qsizetype QJsonObject::length() const

    \overload

    Same as size().
*/

/*! \fn QJsonObject::iterator QJsonObject::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the object.

    \sa constBegin(), end()
*/

/*! \fn QJsonObject::const_iterator QJsonObject::begin() const

    \overload
*/

/*! \fn QJsonObject::const_iterator QJsonObject::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the object.

    \sa begin(), constEnd()
*/

/*! \fn QJsonObject::iterator QJsonObject::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the object.

    \sa begin(), constEnd()
*/

/*! \fn QJsonObject::const_iterator QJsonObject::end() const

    \overload
*/

/*! \fn QJsonObject::const_iterator QJsonObject::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the object.

    \sa constBegin(), end()
*/

/*!
    \fn bool QJsonObject::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty(), returning \c true if the object is empty; otherwise
    returning \c false.
*/

/*! \typedef QJsonObject::const_key_value_iterator
    \inmodule QtCore
    \since 6.10
    \brief The QJsonObject::const_key_value_iterator typedef provides an STL-style iterator for
   QJsonObject.

    QJsonObject::const_key_value_iterator is essentially the same as QJsonObject::const_iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \typedef QJsonObject::key_value_iterator
    \inmodule QtCore
    \since 6.10
    \brief The QJsonObject::key_value_iterator typedef provides an STL-style iterator for
   QJsonObject.

    QJsonObject::key_value_iterator is essentially the same as QJsonObject::iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \fn QJsonObject::key_value_iterator QJsonObject::keyValueBegin()
    \since 6.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the object.

    \sa keyValueEnd()
*/

/*! \fn QJsonObject::key_value_iterator QJsonObject::keyValueEnd()
    \since 6.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the object.

    \sa keyValueBegin()
*/

/*! \fn QJsonObject::const_key_value_iterator QJsonObject::keyValueBegin() const
    \since 6.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the object.

    \sa keyValueEnd()
*/

/*! \fn QJsonObject::const_key_value_iterator QJsonObject::constKeyValueBegin() const
    \since 6.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the object.

    \sa keyValueBegin()
*/

/*! \fn QJsonObject::const_key_value_iterator QJsonObject::keyValueEnd() const
    \since 6.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the object.

    \sa keyValueBegin()
*/

/*! \fn QJsonObject::const_key_value_iterator QJsonObject::constKeyValueEnd() const
    \since 6.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the ibject.

    \sa constKeyValueBegin()
*/

/*! \fn auto QJsonObject::asKeyValueRange() &
    \fn auto QJsonObject::asKeyValueRange() const &
    \fn auto QJsonObject::asKeyValueRange() &&
    \fn auto QJsonObject::asKeyValueRange() const &&
    \since 6.10

    Returns a range object that allows iteration over this object as
    key/value pairs. For instance, this range object can be used in a
    range-based for loop, in combination with a structured binding declaration:

    \snippet code/src_corelib_serialization_qjsonobject.cpp 1

    Note that the value obtained this way is a reference into the one in the
    object. Specifically, mutating the value will modify the object itself.

    When calling this method on rvalues (e.g. on a temporary created in the
    inializer of a ranged for-loop), the object will be captured in this range.

    \sa QKeyValueIterator
*/

/*! \class QJsonObject::iterator
    \inmodule QtCore
    \ingroup json
    \reentrant
    \since 5.0

    \brief The QJsonObject::iterator class provides an STL-style non-const iterator for QJsonObject.

    \compares strong
    \compareswith strong QJsonObject::const_iterator
    \endcompareswith

    QJsonObject::iterator allows you to iterate over a QJsonObject
    and to modify the value (but not the key) stored under
    a particular key. If you want to iterate over a const QJsonObject, you
    should use QJsonObject::const_iterator. It is generally good practice to
    use QJsonObject::const_iterator on a non-const QJsonObject as well, unless you
    need to change the QJsonObject through the iterator. Const iterators are
    slightly faster, and improve code readability.

    The default QJsonObject::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QJsonObject function like
    QJsonObject::begin(), QJsonObject::end(), or QJsonObject::find() before you can
    start iterating.

    Multiple iterators can be used on the same object. Existing iterators will however
    become dangling once the object gets modified.

    \sa QJsonObject::const_iterator, {JSON Support in Qt}, {Saving and Loading a Game}
*/

/*! \typedef QJsonObject::iterator::difference_type

    \internal
*/

/*! \typedef QJsonObject::iterator::iterator_category

    A synonym for \e {std::random_access_iterator_tag} indicating
    this iterator is a random-access iterator.

    \note In Qt versions before 5.6, this was set by mistake to
    \e {std::bidirectional_iterator_tag}.
*/

/*! \typedef QJsonObject::iterator::reference

    \internal
*/

/*! \typedef QJsonObject::iterator::value_type

    \internal
*/

/*! \typedef QJsonObject::iterator::pointer

    \internal
*/

/*! \fn QJsonObject::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QJsonObject::begin(), QJsonObject::end()
*/

/*! \fn QJsonObject::iterator::iterator(QJsonObject *obj, qsizetype index)
    \internal
*/

/*! \fn QString QJsonObject::iterator::key() const

    Returns the current item's key.

    There is no direct way of changing an item's key through an
    iterator, although it can be done by calling QJsonObject::erase()
    followed by QJsonObject::insert().

    \sa value(), keyView()
*/

/*!
    \fn QAnyStringView QJsonObject::iterator::keyView() const
    \since 6.10

    Returns the current item's key as a QAnyStringView. This function does not
    allocate memory.

    Since QJsonObject stores keys in US-ASCII, UTF-8 or UTF-16, the returned
    QAnyStringView may be in any of these encodings.

    There is no direct way of changing an item's key through an
    iterator, although it can be done by calling QJsonObject::erase()
    followed by QJsonObject::insert().

    \sa key(), value()
*/

/*! \fn QJsonValueRef QJsonObject::iterator::value() const

    Returns a modifiable reference to the current item's value.

    You can change the value of an item by using value() on
    the left side of an assignment.

    The return value is of type QJsonValueRef, a helper class for QJsonArray
    and QJsonObject. When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the element in the QJsonArray or QJsonObject
    from which you got the reference.

    \sa key(), keyView(), operator*()
*/

/*! \fn QJsonValueRef QJsonObject::iterator::operator*() const

    Returns a modifiable reference to the current item's value.

    Same as value().

    The return value is of type QJsonValueRef, a helper class for QJsonArray
    and QJsonObject. When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the element in the QJsonArray or QJsonObject
    from which you got the reference.

    \sa key(), keyView()
*/

/*! \fn QJsonValueRef *QJsonObject::iterator::operator->()

    Returns a pointer to a modifiable reference to the current item.
*/

/*! \fn const QJsonValueConstRef *QJsonObject::iterator::operator->() const

    Returns a pointer to a constant reference to the current item.
*/

/*! \fn const QJsonValueRef QJsonObject::iterator::operator[](qsizetype j) const

    Returns a modifiable reference to the item at offset \a j from the
    item pointed to by this iterator (the item at position \c{*this + j}).

    This function is provided to make QJsonObject iterators behave like C++
    pointers.

    The return value is of type QJsonValueRef, a helper class for QJsonArray
    and QJsonObject. When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the element in the QJsonArray or QJsonObject
    from which you got the reference.

    \sa operator+()
*/

/*!
    \fn bool QJsonObject::iterator::operator==(const iterator &lhs, const iterator &rhs)
    \fn bool QJsonObject::iterator::operator==(const iterator &lhs, const const_iterator &rhs)

    Returns \c true if \a lhs points to the same item as \a rhs
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QJsonObject::iterator::operator!=(const iterator &lhs, const iterator &rhs)
    \fn bool QJsonObject::iterator::operator!=(const iterator &lhs, const const_iterator &rhs)

    Returns \c true if \a lhs points to a different item than \a rhs
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QJsonObject::iterator::operator<(const iterator &lhs, const iterator &rhs)
    \fn bool QJsonObject::iterator::operator<(const iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is less than
    the item pointed to by the \a rhs iterator.
*/

/*!
    \fn bool QJsonObject::iterator::operator<=(const iterator &lhs, const iterator &rhs)
    \fn bool QJsonObject::iterator::operator<=(const iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is less than
    or equal to the item pointed to by the \a rhs iterator.
*/

/*!
    \fn bool QJsonObject::iterator::operator>(const iterator &lhs, const iterator &rhs)
    \fn bool QJsonObject::iterator::operator>(const iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is greater
    than the item pointed to by the \a rhs iterator.
*/

/*!
    \fn bool QJsonObject::iterator::operator>=(const iterator &lhs, const iterator &rhs)
    \fn bool QJsonObject::iterator::operator>=(const iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is greater
    than or equal to the item pointed to by the \a rhs iterator.
*/

/*! \fn QJsonObject::iterator QJsonObject::iterator::operator++()

    The prefix \c{++} operator, \c{++i}, advances the iterator to the
    next item in the object and returns an iterator to the new current
    item.

    Calling this function on QJsonObject::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QJsonObject::iterator QJsonObject::iterator::operator++(int)

    \overload

    The postfix \c{++} operator, \c{i++}, advances the iterator to the
    next item in the object and returns an iterator to the previously
    current item.
*/

/*! \fn QJsonObject::iterator QJsonObject::iterator::operator--()

    The prefix \c{--} operator, \c{--i}, makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on QJsonObject::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QJsonObject::iterator QJsonObject::iterator::operator--(int)

    \overload

    The postfix \c{--} operator, \c{i--}, makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn QJsonObject::iterator QJsonObject::iterator::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    \sa operator-()

*/

/*! \fn QJsonObject::iterator QJsonObject::iterator::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    \sa operator+()
*/

/*! \fn QJsonObject::iterator &QJsonObject::iterator::operator+=(qsizetype j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    \sa operator-=(), operator+()
*/

/*! \fn QJsonObject::iterator &QJsonObject::iterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*! \fn qsizetype QJsonObject::iterator::operator-(iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/

/*!
    \class QJsonObject::const_iterator
    \inmodule QtCore
    \ingroup json
    \since 5.0
    \brief The QJsonObject::const_iterator class provides an STL-style const iterator for QJsonObject.

    \compares strong
    \compareswith strong QJsonObject::iterator
    \endcompareswith

    QJsonObject::const_iterator allows you to iterate over a QJsonObject.
    If you want to modify the QJsonObject as you iterate
    over it, you must use QJsonObject::iterator instead. It is generally
    good practice to use QJsonObject::const_iterator on a non-const QJsonObject as
    well, unless you need to change the QJsonObject through the iterator.
    Const iterators are slightly faster and improve code
    readability.

    The default QJsonObject::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QJsonObject
    function like QJsonObject::constBegin(), QJsonObject::constEnd(), or
    QJsonObject::find() before you can start iterating.

    Multiple iterators can be used on the same object. Existing iterators
    will however become dangling if the object gets modified.

    \sa QJsonObject::iterator, {JSON Support in Qt}, {Saving and Loading a Game}
*/

/*! \typedef QJsonObject::const_iterator::difference_type

    \internal
*/

/*! \typedef QJsonObject::const_iterator::iterator_category

    A synonym for \e {std::random_access_iterator_tag} indicating
    this iterator is a random-access iterator.

    \note In Qt versions before 5.6, this was set by mistake to
    \e {std::bidirectional_iterator_tag}.
*/

/*! \typedef QJsonObject::const_iterator::reference

    \internal
*/

/*! \typedef QJsonObject::const_iterator::value_type

    \internal
*/

/*! \typedef QJsonObject::const_iterator::pointer

    \internal
*/

/*! \fn QJsonObject::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QJsonObject::constBegin(), QJsonObject::constEnd()
*/

/*! \fn QJsonObject::const_iterator::const_iterator(const QJsonObject *obj, qsizetype index)
    \internal
*/

/*! \fn QJsonObject::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn QString QJsonObject::const_iterator::key() const

    Returns the current item's key.

    \sa value(), keyView()
*/

/*!
    \fn QAnyStringView QJsonObject::const_iterator::keyView() const
    \since 6.10

    Returns the current item's key as a QAnyStringView. This function does not
    allocate.

    Since QJsonObject stores keys in US-ASCII, UTF-8 or UTF-16, the returned
    QAnyStringView may be in any of these encodings.

    \sa value(), key()
*/

/*! \fn QJsonValueConstRef QJsonObject::const_iterator::value() const

    Returns the current item's value.

    \sa key(), keyView(), operator*()
*/

/*! \fn const QJsonValueConstRef QJsonObject::const_iterator::operator*() const

    Returns the current item's value.

    Same as value().

    \sa key(), keyView()
*/

/*! \fn const QJsonValueConstRef *QJsonObject::const_iterator::operator->() const

    Returns a pointer to the current item.
*/

/*! \fn const QJsonValueConstRef QJsonObject::const_iterator::operator[](qsizetype j) const

    Returns the item at offset \a j from the item pointed to by this iterator (the item at
    position \c{*this + j}).

    This function is provided to make QJsonObject iterators behave like C++
    pointers.

    \sa operator+()
*/


/*! \fn bool QJsonObject::const_iterator::operator==(const const_iterator &lhs, const const_iterator &rhs)

    Returns \c true if \a lhs points to the same item as \a rhs
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn bool QJsonObject::const_iterator::operator!=(const const_iterator &lhs, const const_iterator &rhs)

    Returns \c true if \a lhs points to a different item than \a rhs
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QJsonObject::const_iterator::operator<(const const_iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is less than
    the item pointed to by the \a rhs iterator.
*/

/*!
    \fn bool QJsonObject::const_iterator::operator<=(const const_iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is less than
    or equal to the item pointed to by the \a rhs iterator.
*/

/*!
    \fn bool QJsonObject::const_iterator::operator>(const const_iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is greater
    than the item pointed to by the \a rhs iterator.
*/

/*!
    \fn bool QJsonObject::const_iterator::operator>=(const const_iterator &lhs, const const_iterator &rhs)

    Returns \c true if the item pointed to by \a lhs iterator is greater
    than or equal to the item pointed to by the \a rhs iterator.
*/

/*! \fn QJsonObject::const_iterator QJsonObject::const_iterator::operator++()

    The prefix \c{++} operator, \c{++i}, advances the iterator to the
    next item in the object and returns an iterator to the new current
    item.

    Calling this function on QJsonObject::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QJsonObject::const_iterator QJsonObject::const_iterator::operator++(int)

    \overload

    The postfix \c{++} operator, \c{i++}, advances the iterator to the
    next item in the object and returns an iterator to the previously
    current item.
*/

/*! \fn QJsonObject::const_iterator &QJsonObject::const_iterator::operator--()

    The prefix \c{--} operator, \c{--i}, makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on QJsonObject::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QJsonObject::const_iterator QJsonObject::const_iterator::operator--(int)

    \overload

    The postfix \c{--} operator, \c{i--}, makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn QJsonObject::const_iterator QJsonObject::const_iterator::operator+(qsizetype j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    This operation can be slow for large \a j values.

    \sa operator-()
*/

/*! \fn QJsonObject::const_iterator QJsonObject::const_iterator::operator-(qsizetype j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn QJsonObject::const_iterator &QJsonObject::const_iterator::operator+=(qsizetype j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    This operation can be slow for large \a j values.

    \sa operator-=(), operator+()
*/

/*! \fn QJsonObject::const_iterator &QJsonObject::const_iterator::operator-=(qsizetype j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    This operation can be slow for large \a j values.

    \sa operator+=(), operator-()
*/

/*! \fn qsizetype QJsonObject::const_iterator::operator-(const_iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/


/*!
    \internal
 */
bool QJsonObject::detach(qsizetype reserve)
{
    if (!o)
        return true;
    o = QCborContainerPrivate::detach(o.data(), reserve ? reserve * 2 : o->elements.size());
    return o;
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
/*!
    \internal
 */
QString QJsonObject::keyAt(qsizetype i) const
{
    Q_ASSERT(o && i >= 0 && i * 2 < o->elements.size());
    return o->stringAt(i * 2);
}

/*!
    \internal
 */
QJsonValue QJsonObject::valueAt(qsizetype i) const
{
    if (!o || i < 0 || 2 * i + 1 >= o->elements.size())
        return QJsonValue(QJsonValue::Undefined);
    return QJsonPrivate::Value::fromTrustedCbor(o->valueAt(2 * i + 1));
}

/*!
    \internal
 */
void QJsonObject::setValueAt(qsizetype i, const QJsonValue &val)
{
    Q_ASSERT(o && i >= 0 && 2 * i + 1 < o->elements.size());
    detach();
    if (val.isUndefined()) {
        o->removeAt(2 * i + 1);
        o->removeAt(2 * i);
    } else {
        o->replaceAt(2 * i + 1, QCborValue::fromJsonValue(val));
    }
}
#endif // Qt 7

/*!
    \internal
 */
void QJsonObject::removeAt(qsizetype index)
{
    detach();
    o->removeAt(index + 1);
    o->removeAt(index);
}

size_t qHash(const QJsonObject &object, size_t seed)
{
    QtPrivate::QHashCombine hash(seed);
    for (auto it = object.begin(), end = object.end(); it != end; ++it) {
        const QString key = it.key();
        const QJsonValue value = it.value();
        seed = hash(seed, std::pair<const QString&, const QJsonValue&>(key, value));
    }
    return seed;
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, const QJsonObject &o)
{
    QDebugStateSaver saver(dbg);
    if (!o.o) {
        dbg << "QJsonObject()";
        return dbg;
    }
    QByteArray json;
    QJsonPrivate::Writer::objectToJson(o.o.data(), json, 0, true);
    dbg.nospace() << "QJsonObject("
                  << json.constData() // print as utf-8 string without extra quotation marks
                  << ")";
    return dbg;
}
#endif

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &stream, const QJsonObject &object)
{
    QJsonDocument doc{object};
    stream << doc.toJson(QJsonDocument::Compact);
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QJsonObject &object)
{
    QJsonDocument doc;
    stream >> doc;
    object = doc.object();
    return stream;
}
#endif

QT_END_NAMESPACE
