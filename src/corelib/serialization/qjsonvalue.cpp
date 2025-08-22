// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qurl.h>
#include <quuid.h>
#include <qvariant.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qhash.h>
#include <qdebug.h>
#include "qdatastream.h"
#include "qjsonparser_p.h"
#include "qjsonwriter_p.h"

#include <private/qnumeric_p.h>
#include <private/qcborvalue_p.h>

#include <qcborarray.h>
#include <qcbormap.h>

#include "qjson_p.h"

QT_BEGIN_NAMESPACE

static QJsonValue::Type convertFromCborType(QCborValue::Type type) noexcept
{
    switch (type) {
    case QCborValue::Null:
        return QJsonValue::Null;
    case QCborValue::True:
    case QCborValue::False:
        return QJsonValue::Bool;
    case QCborValue::Double:
    case QCborValue::Integer:
        return QJsonValue::Double;
    case QCborValue::String:
        return QJsonValue::String;
    case QCborValue::Array:
        return QJsonValue::Array;
    case QCborValue::Map:
        return QJsonValue::Object;
    case QCborValue::Undefined:
    default:
        return QJsonValue::Undefined;
    }
}

/*!
    \class QJsonValue
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \ingroup qtserialization
    \reentrant
    \since 5.0

    \brief The QJsonValue class encapsulates a value in JSON.

    \compares equality
    \compareswith equality QJsonValueConstRef QJsonValueRef
    \endcompareswith

    A value in JSON can be one of 6 basic types:

    JSON is a format to store structured data. It has 6 basic data types:

    \list
    \li bool QJsonValue::Bool
    \li double QJsonValue::Double
    \li string QJsonValue::String
    \li array QJsonValue::Array
    \li object QJsonValue::Object
    \li null QJsonValue::Null
    \endlist

    A value can represent any of the above data types. In addition, QJsonValue has one special
    flag to represent undefined values. This can be queried with isUndefined().

    The type of the value can be queried with type() or accessors like isBool(), isString(), and so on.
    Likewise, the value can be converted to the type stored in it using the toBool(), toString() and so on.

    Values are strictly typed internally and contrary to QVariant will not attempt to do any implicit type
    conversions. This implies that converting to a type that is not stored in the value will return a default
    constructed return value.

    \section1 QJsonValueRef

    QJsonValueRef is a helper class for QJsonArray and QJsonObject.
    When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the element in the QJsonArray or QJsonObject
    from which you got the reference.

    The following methods return QJsonValueRef:
    \list
    \li \l {QJsonArray}::operator[](qsizetype i)
    \li \l {QJsonObject}::operator[](const QString & key) const
    \endlist

    \sa {JSON Support in Qt}, {Saving and Loading a Game}
*/

/*!
    Creates a QJsonValue of type \a type.

    The default is to create a Null value.
 */
QJsonValue::QJsonValue(Type type)
{
    switch (type) {
    case Null:
        value = QCborValue::Null;
        break;
    case Bool:
        value = QCborValue::False;
        break;
    case Double:
        value = QCborValue::Double;
        break;
    case String:
        value = QCborValue::String;
        break;
    case Array:
        value = QCborValue::Array;
        break;
    case Object:
        value = QCborValue::Map;
        break;
    case Undefined:
        break;
    }
}

/*!
    Creates a value of type Bool, with value \a b.
 */
QJsonValue::QJsonValue(bool b)
    : value(b)
{
}

static inline QCborValue doubleValueHelper(double v)
{
    qint64 n = 0;
    // Convert to integer if the number is an integer and changing wouldn't
    // introduce additional digit precision not present in the double.
    if (convertDoubleTo<qint64>(v, &n, false /* allow_precision_upgrade */))
        return n;
    else
        return v;
}

/*!
    Creates a value of type Double, with value \a v.
 */
QJsonValue::QJsonValue(double v)
    : value(doubleValueHelper(v))
{
}

/*!
    \overload
    Creates a value of type Double, with value \a v.
 */
QJsonValue::QJsonValue(int v)
    : value(v)
{
}

/*!
    \overload
    Creates a value of type Double, with value \a v.

    This is stored internally as a 64-bit integer, so retains its full
    precision, as long as it is retrieved with \l toInteger(). However,
    retrieving its value with \l toDouble() will lose precision unless the value
    lies between ±2^53.

    \sa toInteger(), toDouble()
*/
QJsonValue::QJsonValue(qint64 v)
    : value(v)
{
}

/*!
    Creates a value of type String, with value \a s.
 */
QJsonValue::QJsonValue(const QString &s)
    : value(s)
{
}

/*!
    \fn QJsonValue::QJsonValue(const char *s)

    Creates a value of type String with value \a s, assuming
    UTF-8 encoding of the input.

    You can disable this constructor by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications.

    \since 5.3
 */

/*!
    Creates a value of type String, with the Latin-1 string viewed by \a s.
 */
QJsonValue::QJsonValue(QLatin1StringView s)
    : value(s)
{
}

/*!
    Creates a value of type Array, with value \a a.
 */
QJsonValue::QJsonValue(const QJsonArray &a)
    : value(QCborArray::fromJsonArray(a))
{
}

/*!
    \overload
    \since 6.3
 */
QJsonValue::QJsonValue(QJsonArray &&a) noexcept
    : value(QCborArray::fromJsonArray(std::move(a)))
{
}

/*!
    Creates a value of type Object, with value \a o.
 */
QJsonValue::QJsonValue(const QJsonObject &o)
    : value(QCborMap::fromJsonObject(o))
{
}

/*!
    \overload
    \since 6.3
 */
QJsonValue::QJsonValue(QJsonObject &&o) noexcept
    : value(QCborMap::fromJsonObject(std::move(o)))
{
}


/*!
    Destroys the value.
 */
QJsonValue::~QJsonValue() = default;

/*!
    Creates a copy of \a other.
 */
QJsonValue::QJsonValue(const QJsonValue &other) noexcept = default;

/*!
    Assigns the value stored in \a other to this object.
 */
QJsonValue &QJsonValue::operator =(const QJsonValue &other) noexcept
{
    QJsonValue copy(other);
    swap(copy);
    return *this;
}

QJsonValue::QJsonValue(QJsonValue &&other) noexcept
    : value(std::move(other.value))
{
    other.value = QCborValue(nullptr);
}

void QJsonValue::swap(QJsonValue &other) noexcept
{
    value.swap(other.value);
}

/*!
    \fn QJsonValue::QJsonValue(QJsonValue &&other)
    \since 5.10

    Move-constructs a QJsonValue from \a other.
*/

/*!
    \fn QJsonValue &QJsonValue::operator =(QJsonValue &&other)
    \since 5.10

    Move-assigns \a other to this value.
*/

/*!
    \fn void QJsonValue::swap(QJsonValue &other)
    \since 5.10
    \memberswap{value}
*/

/*!
    \fn bool QJsonValue::isNull() const

    Returns \c true if the value is null.
*/

/*!
    \fn bool QJsonValue::isBool() const

    Returns \c true if the value contains a boolean.

    \sa toBool()
 */

/*!
    \fn bool QJsonValue::isDouble() const

    Returns \c true if the value contains a double.

    \sa toDouble()
 */

/*!
    \fn bool QJsonValue::isString() const

    Returns \c true if the value contains a string.

    \sa toString()
 */

/*!
    \fn bool QJsonValue::isArray() const

    Returns \c true if the value contains an array.

    \sa toArray()
 */

/*!
    \fn bool QJsonValue::isObject() const

    Returns \c true if the value contains an object.

    \sa toObject()
 */

/*!
    \fn bool QJsonValue::isUndefined() const

    Returns \c true if the value is undefined. This can happen in certain
    error cases as e.g. accessing a non existing key in a QJsonObject.
 */

#ifndef QT_NO_VARIANT
/*!
    Converts \a variant to a QJsonValue and returns it.

    The conversion will convert QVariant types as follows:

    \table
    \header
        \li Source type
        \li Destination type
    \row
        \li
            \list
                \li QMetaType::Nullptr
            \endlist
        \li QJsonValue::Null
    \row
        \li
            \list
                \li QMetaType::Bool
            \endlist
        \li QJsonValue::Bool
    \row
        \li
            \list
                \li QMetaType::Int
                \li QMetaType::UInt
                \li QMetaType::LongLong
                \li QMetaType::ULongLong
                \li QMetaType::Float
                \li QMetaType::Double
            \endlist
        \li QJsonValue::Double
    \row
        \li
            \list
                \li QMetaType::QString
            \endlist
        \li QJsonValue::String
    \row
        \li
            \list
                \li QMetaType::QStringList
                \li QMetaType::QVariantList
            \endlist
        \li QJsonValue::Array
    \row
        \li
            \list
                \li QMetaType::QVariantMap
                \li QMetaType::QVariantHash
            \endlist
        \li QJsonValue::Object

    \row
        \li
            \list
                \li QMetaType::QUrl
            \endlist
        \li QJsonValue::String. The conversion will use QUrl::toString() with flag
            QUrl::FullyEncoded, so as to ensure maximum compatibility in parsing the URL
    \row
        \li
            \list
                \li QMetaType::QUuid
            \endlist
        \li QJsonValue::String. Since Qt 5.11, the resulting string will not include braces
    \row
        \li
            \list
                \li QMetaType::QCborValue
            \endlist
        \li Whichever type QCborValue::toJsonValue() returns.
    \row
        \li
            \list
                \li QMetaType::QCborArray
            \endlist
        \li QJsonValue::Array. See QCborValue::toJsonValue() for conversion restrictions.
    \row
        \li
            \list
                \li QMetaType::QCborMap
            \endlist
        \li QJsonValue::Map. See QCborValue::toJsonValue() for conversion restrictions and the
            "stringification" of map keys.
    \endtable

    \section2 Loss of information and other types

    QVariant can carry more information than is representable in JSON. If the
    QVariant is not one of the types above, the conversion is not guaranteed
    and is subject to change in future versions of Qt, as the UUID one did.
    Code should strive not to use any other types than those listed above.

    If QVariant::isNull() returns true, a null QJsonValue is returned or
    inserted into the list or object, regardless of the type carried by
    QVariant. Note the behavior change in Qt 6.0 affecting QVariant::isNull()
    also affects this function.

    A floating point value that is either an infinity or NaN will be converted
    to a null JSON value. Since Qt 6.0, QJsonValue can store the full precision
    of any 64-bit signed integer without loss, but in previous versions values
    outside the range of ±2^53 may lose precision. Unsigned 64-bit values
    greater than or equal to 2^63 will either lose precision or alias to
    negative values, so QMetaType::ULongLong should be avoided.

    For other types not listed above, a conversion to string will be attempted,
    usually but not always by calling QVariant::toString(). If the conversion
    fails the value is replaced by a null JSON value. Note that
    QVariant::toString() is also lossy for the majority of types. For example,
    if the passed QVariant is representing raw byte array data, it is recommended
    to pre-encode it to \l {RFC 4686}{Base64} (or
    another lossless encoding), otherwise a lossy conversion using QString::fromUtf8()
    will be used.

    Please note that the conversions via QVariant::toString() are subject to
    change at any time. Both QVariant and QJsonValue may be extended in the
    future to support more types, which will result in a change in how this
    function performs conversions.

    \sa toVariant(), QCborValue::fromVariant()
 */
QJsonValue QJsonValue::fromVariant(const QVariant &variant)
{
    switch (variant.metaType().id()) {
    case QMetaType::Nullptr:
        return QJsonValue(Null);
    case QMetaType::Bool:
        return QJsonValue(variant.toBool());
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::LongLong:
        return QJsonValue(variant.toLongLong());
    case QMetaType::ULong:
    case QMetaType::ULongLong:
        if (variant.toULongLong() <= static_cast<uint64_t>(std::numeric_limits<qint64>::max()))
            return QJsonValue(variant.toLongLong());
        Q_FALLTHROUGH();
    case QMetaType::Float16:
    case QMetaType::Float:
    case QMetaType::Double: {
        double v = variant.toDouble();
        return qt_is_finite(v) ? QJsonValue(v) : QJsonValue();
    }
    case QMetaType::QString:
        return QJsonValue(variant.toString());
    case QMetaType::QStringList:
        return QJsonValue(QJsonArray::fromStringList(variant.toStringList()));
    case QMetaType::QVariantList:
        return QJsonValue(QJsonArray::fromVariantList(variant.toList()));
    case QMetaType::QVariantMap:
        return QJsonValue(QJsonObject::fromVariantMap(variant.toMap()));
    case QMetaType::QVariantHash:
        return QJsonValue(QJsonObject::fromVariantHash(variant.toHash()));
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QUrl:
        return QJsonValue(variant.toUrl().toString(QUrl::FullyEncoded));
    case QMetaType::QUuid:
        return variant.toUuid().toString(QUuid::WithoutBraces);
    case QMetaType::QJsonValue:
        return variant.toJsonValue();
    case QMetaType::QJsonObject:
        return variant.toJsonObject();
    case QMetaType::QJsonArray:
        return variant.toJsonArray();
    case QMetaType::QJsonDocument: {
        QJsonDocument doc = variant.toJsonDocument();
        return doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    }
    case QMetaType::QCborValue:
        return qvariant_cast<QCborValue>(variant).toJsonValue();
    case QMetaType::QCborArray:
        return qvariant_cast<QCborArray>(variant).toJsonArray();
    case QMetaType::QCborMap:
        return qvariant_cast<QCborMap>(variant).toJsonObject();
#endif
    default:
        break;
    }
    QString string = variant.toString();
    if (string.isEmpty())
        return QJsonValue();
    return QJsonValue(string);
}

/*!
    Converts the value to a \l {QVariant::}{QVariant()}.

    The QJsonValue types will be converted as follows:

    \value Null     QMetaType::Nullptr
    \value Bool     QMetaType::Bool
    \value Double   QMetaType::Double or QMetaType::LongLong
    \value String   QString
    \value Array    QVariantList
    \value Object   QVariantMap
    \value Undefined \l {QVariant::}{QVariant()}

    \sa fromVariant()
 */
QVariant QJsonValue::toVariant() const
{
    switch (value.type()) {
    case QCborValue::True:
        return true;
    case QCborValue::False:
        return false;
    case QCborValue::Integer:
        return toInteger();
    case QCborValue::Double:
        return toDouble();
    case QCborValue::String:
        return toString();
    case QCborValue::Array:
        return toArray().toVariantList();
    case QCborValue::Map:
        return toObject().toVariantMap();
    case QCborValue::Null:
        return QVariant::fromValue(nullptr);
    case QCborValue::Undefined:
    default:
        break;
    }
    return QVariant();
}

/*!
    \enum QJsonValue::Type

    This enum describes the type of the JSON value.

    \value Null     A Null value
    \value Bool     A boolean value. Use toBool() to convert to a bool.
    \value Double   A number value. Use toDouble() to convert to a double,
                    or toInteger() to convert to a qint64.
    \value String   A string. Use toString() to convert to a QString.
    \value Array    An array. Use toArray() to convert to a QJsonArray.
    \value Object   An object. Use toObject() to convert to a QJsonObject.
    \value Undefined The value is undefined. This is usually returned as an
                    error condition, when trying to read an out of bounds value
                    in an array or a non existent key in an object.
*/
#endif // !QT_NO_VARIANT

/*!
    \since 6.9
    Parses \a json as a UTF-8 encoded JSON value, and creates a QJsonValue
    from it.

    Returns a valid QJsonValue if the parsing succeeds. If it fails, the
    returned value will be \l {QJsonValue::isUndefined} {undefined}, and
    the optional \a error variable will contain further details about the
    error.

    \sa QJsonParseError, isUndefined(), toJson()
 */
QJsonValue QJsonValue::fromJson(QByteArrayView json, QJsonParseError *error)
{
    QJsonPrivate::Parser parser(json);
    QJsonValue result;
    result.value = parser.parse(error);
    return result;
}

/*!
\if defined(qt7)
    \enum QJsonValue::JsonFormat
    \since 6.9

    This value defines the format of the JSON byte array produced
    when converting to a QJsonValue using toJson().

    \value Indented Defines human readable output as follows:
        \snippet code/src_corelib_serialization_qjsondocument.cpp 0

    \value Compact Defines a compact output as follows:
        \snippet code/src_corelib_serialization_qjsondocument.cpp 1
\else
    \typealias QJsonValue::JsonFormat
    \since 6.9

    Same as \l QJsonDocument::JsonFormat.
\endif
*/

/*!
    \since 6.9
    Converts the QJsonValue to a UTF-8 encoded JSON value in the provided \a format.

    \sa fromJson(), JsonFormat
 */
QByteArray QJsonValue::toJson(JsonFormat format) const
{
    QByteArray json;

    QJsonPrivate::Writer::valueToJson(value, json, 0, (format == JsonFormat::Compact));

    return json;
}

/*!
    Returns the type of the value.

    \sa QJsonValue::Type
 */
QJsonValue::Type QJsonValue::type() const
{
    return convertFromCborType(value.type());
}

/*!
    Converts the value to a bool and returns it.

    If type() is not bool, the \a defaultValue will be returned.
 */
bool QJsonValue::toBool(bool defaultValue) const
{
    switch (value.type()) {
    case QCborValue::True:
        return true;
    case QCborValue::False:
        return false;
    default:
        return defaultValue;
    }
}

/*!
    \since 5.2
    Converts the value to an int and returns it.

    If type() is not Double or the value is not a whole number,
    the \a defaultValue will be returned.
 */
int QJsonValue::toInt(int defaultValue) const
{
    switch (value.type()) {
    case QCborValue::Double: {
        int dblInt;
        if (convertDoubleTo<int>(toDouble(), &dblInt))
            return dblInt;
        break;
    }
    case QCborValue::Integer: {
        const auto n = value.toInteger();
        if (qint64(int(n)) == n)
            return int(n);
        break;
    }
    default:
        break;
    }
    return defaultValue;
}

/*!
    \since 6.0
    Converts the value to an integer and returns it.

    If type() is not Double or the value is not a whole number
    representable as qint64, the \a defaultValue will be returned.
 */
qint64 QJsonValue::toInteger(qint64 defaultValue) const
{
    switch (value.type()) {
    case QCborValue::Integer:
        return value.toInteger();
    case QCborValue::Double: {
        qint64 dblInt;
        if (convertDoubleTo<qint64>(toDouble(), &dblInt))
            return dblInt;
        break;
    }
    default:
        break;
    }
    return defaultValue;
}

/*!
    Converts the value to a double and returns it.

    If type() is not Double, the \a defaultValue will be returned.
 */
double QJsonValue::toDouble(double defaultValue) const
{
    return value.toDouble(defaultValue);
}

/*!
    Converts the value to a QString and returns it.

    If type() is not String, the \a defaultValue will be returned.

    \sa toStringView()
 */
QString QJsonValue::toString(const QString &defaultValue) const
{
    return value.toString(defaultValue);
}

/*!
    \since 6.10

    Returns the string value stored in this QJsonValue, if it is of the
    \l{String}{string} type. Otherwise, it returns \a defaultValue. Since
    QJsonValue stores strings in either US-ASCII, UTF-8 or UTF-16, the returned
    QAnyStringView may be in any of these encodings.

    This function does not allocate memory. The return value is valid until the
    next call to a non-const member function on this object. If this object goes
    out of scope, the return value is valid until the next call to a non-const
    member function on the parent JSON object or array.

    \sa toString()
*/
QAnyStringView QJsonValue::toStringView(QAnyStringView defaultValue) const
{
    return value.toStringView(defaultValue);
}

/*!
    Converts the value to a QString and returns it.

    If type() is not String, a null QString will be returned.

    \sa QString::isNull()
 */
QString QJsonValue::toString() const
{
    return value.toString();
}

/*!
    Converts the value to an array and returns it.

    If type() is not Array, the \a defaultValue will be returned.
 */
QJsonArray QJsonValue::toArray(const QJsonArray &defaultValue) const
{
    if (!isArray())
        return defaultValue;
    QCborContainerPrivate *dd = nullptr;
    const auto n = QJsonPrivate::Value::valueHelper(value);
    const auto container = QJsonPrivate::Value::container(value);
    Q_ASSERT(n == -1 || container == nullptr);
    if (n < 0)
        dd = container;
    return QJsonArray(dd);
}

/*!
    \overload

    Converts the value to an array and returns it.

    If type() is not Array, a \l{QJsonArray::}{QJsonArray()} will be returned.
 */
QJsonArray QJsonValue::toArray() const
{
    return toArray(QJsonArray());
}

/*!
    Converts the value to an object and returns it.

    If type() is not Object, the \a defaultValue will be returned.
 */
QJsonObject QJsonValue::toObject(const QJsonObject &defaultValue) const
{
    if (!isObject())
        return defaultValue;
    QCborContainerPrivate *dd = nullptr;
    const auto container = QJsonPrivate::Value::container(value);
    const auto n = QJsonPrivate::Value::valueHelper(value);
    Q_ASSERT(n == -1 || container == nullptr);
    if (n < 0)
        dd = container;
    return QJsonObject(dd);
}

/*!
    \overload

    Converts the value to an object and returns it.

    If type() is not Object, the \l {QJsonObject::}{QJsonObject()} will be returned.
*/
QJsonObject QJsonValue::toObject() const
{
    return toObject(QJsonObject());
}

/*!
    Returns a QJsonValue representing the value for the key \a key.

    Equivalent to calling toObject().value(key).

    The returned QJsonValue is QJsonValue::Undefined if the key does not exist,
    or if isObject() is false.

    \since 5.10

    \sa QJsonValue, QJsonValue::isUndefined(), QJsonObject
 */
const QJsonValue QJsonValue::operator[](const QString &key) const
{
    return (*this)[QStringView(key)];
}

/*!
    \overload
    \since 5.14
*/
const QJsonValue QJsonValue::operator[](QStringView key) const
{
    if (!isObject())
        return QJsonValue(QJsonValue::Undefined);

    return toObject().value(key);
}

/*!
    \overload
    \since 5.10
*/
const QJsonValue QJsonValue::operator[](QLatin1StringView key) const
{
    if (!isObject())
        return QJsonValue(QJsonValue::Undefined);

    return toObject().value(key);
}

/*!
    Returns a QJsonValue representing the value for index \a i.

    Equivalent to calling toArray().at(i).

    The returned QJsonValue is QJsonValue::Undefined, if \a i is out of bounds,
    or if isArray() is false.

    \since 5.10

    \sa QJsonValue, QJsonValue::isUndefined(), QJsonArray
 */
const QJsonValue QJsonValue::operator[](qsizetype i) const
{
    if (!isArray())
        return QJsonValue(QJsonValue::Undefined);

    return toArray().at(i);
}

/*!
    \fn bool QJsonValue::operator==(const QJsonValue &lhs, const QJsonValue &rhs)

    Returns \c true if the \a lhs value is equal to \a rhs value, \c false otherwise.
*/
bool comparesEqual(const QJsonValue &lhs, const QJsonValue &rhs)
{
    if (lhs.value.type() != rhs.value.type()) {
        if (lhs.isDouble() && rhs.isDouble()) {
            // One value Cbor integer, one Cbor double, should interact as doubles.
            return lhs.toDouble() == rhs.toDouble();
        }
        return false;
    }

    switch (lhs.value.type()) {
    case QCborValue::Undefined:
    case QCborValue::Null:
    case QCborValue::True:
    case QCborValue::False:
        break;
    case QCborValue::Double:
        return lhs.toDouble() == rhs.toDouble();
    case QCborValue::Integer:
        return QJsonPrivate::Value::valueHelper(lhs.value)
                == QJsonPrivate::Value::valueHelper(rhs.value);
    case QCborValue::String:
        return lhs.toString() == rhs.toString();
    case QCborValue::Array:
        return lhs.toArray() == rhs.toArray();
    case QCborValue::Map:
        return lhs.toObject() == rhs.toObject();
    default:
        return false;
    }
    return true;
}

/*!
    \fn bool QJsonValue::operator!=(const QJsonValue &lhs, const QJsonValue &rhs)

    Returns \c true if the \a lhs value is not equal to \a rhs value, \c false otherwise.
*/

/*!
    \class QJsonValueRef
    \inmodule QtCore
    \reentrant
    \brief The QJsonValueRef class is a helper class for QJsonValue.

    \internal

    \ingroup json

    When you get an object of type QJsonValueRef, if you can assign to it,
    the assignment will apply to the character in the string from
    which you got the reference. That is its whole purpose in life.

    You can use it exactly in the same way as a reference to a QJsonValue.

    The QJsonValueRef becomes invalid once modifications are made to the
    string: if you want to keep the character, copy it into a QJsonValue.

    Most of the QJsonValue member functions also exist in QJsonValueRef.
    However, they are not explicitly documented here.
*/

void QJsonValueRef::detach()
{
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
    QCborContainerPrivate *d = QJsonPrivate::Value::container(*this);
    d = QCborContainerPrivate::detach(d, d->elements.size());

    if (is_object)
        o->o.reset(d);
    else
        a->a.reset(d);
#else
    d = QCborContainerPrivate::detach(d, d->elements.size());
#endif
}

static QJsonValueRef &assignToRef(QJsonValueRef &ref, const QCborValue &value, bool is_object)
{
    QCborContainerPrivate *d = QJsonPrivate::Value::container(ref);
    qsizetype index = QJsonPrivate::Value::indexHelper(ref);
    if (is_object && value.isUndefined()) {
        d->removeAt(index);
        d->removeAt(index - 1);
    } else {
        d->replaceAt(index, value);
    }

    return ref;
}

QJsonValueRef &QJsonValueRef::operator =(const QJsonValue &val)
{
    detach();
    return assignToRef(*this, QCborValue::fromJsonValue(val), is_object);
}

QJsonValueRef &QJsonValueRef::operator =(const QJsonValueRef &ref)
{
    // ### optimize more?
    const QCborContainerPrivate *d = QJsonPrivate::Value::container(ref);
    qsizetype index = QJsonPrivate::Value::indexHelper(ref);

    if (d == QJsonPrivate::Value::container(*this) &&
            index == QJsonPrivate::Value::indexHelper(*this))
        return *this;     // self assignment

    detach();
    return assignToRef(*this, d->valueAt(index), is_object);
}

#ifndef QT_NO_VARIANT
QVariant QJsonValueConstRef::toVariant() const
{
    return concrete(*this).toVariant();
}
#endif // !QT_NO_VARIANT

QJsonArray QJsonValueConstRef::toArray() const
{
    return concrete(*this).toArray();
}

QJsonObject QJsonValueConstRef::toObject() const
{
    return concrete(*this).toObject();
}

QJsonValue::Type QJsonValueConstRef::concreteType(QJsonValueConstRef self) noexcept
{
    return convertFromCborType(QJsonPrivate::Value::elementHelper(self).type);
}

bool QJsonValueConstRef::concreteBool(QJsonValueConstRef self, bool defaultValue) noexcept
{
    auto &e = QJsonPrivate::Value::elementHelper(self);
    if (e.type == QCborValue::False)
        return false;
    if (e.type == QCborValue::True)
        return true;
    return defaultValue;
}

qint64 QJsonValueConstRef::concreteInt(QJsonValueConstRef self, qint64 defaultValue, bool clamp) noexcept
{
    auto &e = QJsonPrivate::Value::elementHelper(self);
    qint64 v = defaultValue;
    if (e.type == QCborValue::Double) {
        // convertDoubleTo modifies the output even on returning false
        if (!convertDoubleTo<qint64>(e.fpvalue(), &v))
            v = defaultValue;
    } else if (e.type == QCborValue::Integer) {
        v = e.value;
    }
    if (clamp && qint64(int(v)) != v)
        return defaultValue;
    return v;
}

double QJsonValueConstRef::concreteDouble(QJsonValueConstRef self, double defaultValue) noexcept
{
    auto &e = QJsonPrivate::Value::elementHelper(self);
    if (e.type == QCborValue::Double)
        return e.fpvalue();
    if (e.type == QCborValue::Integer)
        return e.value;
    return defaultValue;
}

QString QJsonValueConstRef::concreteString(QJsonValueConstRef self, const QString &defaultValue)
{
    const QCborContainerPrivate *d = QJsonPrivate::Value::container(self);
    qsizetype index = QJsonPrivate::Value::indexHelper(self);
    if (d->elements.at(index).type != QCborValue::String)
        return defaultValue;
    return d->stringAt(index);
}

QAnyStringView QJsonValueConstRef::concreteStringView(QJsonValueConstRef self, QAnyStringView defaultValue)
{
    const QCborContainerPrivate *d = QJsonPrivate::Value::container(self);
    const qsizetype index = QJsonPrivate::Value::indexHelper(self);
    if (d->elements.at(index).type != QCborValue::String)
        return defaultValue;
    return d->anyStringViewAt(index);
}

QJsonValue QJsonValueConstRef::concrete(QJsonValueConstRef self) noexcept
{
    const QCborContainerPrivate *d = QJsonPrivate::Value::container(self);
    qsizetype index = QJsonPrivate::Value::indexHelper(self);
    return QJsonPrivate::Value::fromTrustedCbor(d->valueAt(index));
}

QAnyStringView QJsonValueConstRef::objectKeyView(QJsonValueConstRef self)
{
    Q_ASSERT(self.is_object);
    const QCborContainerPrivate *d = QJsonPrivate::Value::container(self);
    const qsizetype index = QJsonPrivate::Value::indexHelper(self);

    Q_ASSERT(d);
    Q_ASSERT(index < d->elements.size());
    return d->anyStringViewAt(index - 1);
}

QString QJsonValueConstRef::objectKey(QJsonValueConstRef self)
{
    Q_ASSERT(self.is_object);
    const QCborContainerPrivate *d = QJsonPrivate::Value::container(self);
    qsizetype index = QJsonPrivate::Value::indexHelper(self);

    Q_ASSERT(d);
    Q_ASSERT(index < d->elements.size());
    return d->stringAt(index - 1);
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
QVariant QJsonValueRef::toVariant() const
{
    return QJsonValueConstRef::toVariant();
}

QJsonArray QJsonValueRef::toArray() const
{
    return QJsonValueConstRef::toArray();
}

QJsonObject QJsonValueRef::toObject() const
{
    return QJsonValueConstRef::toObject();
}

QJsonValue QJsonValueRef::toValue() const
{
    return concrete(*this);
}
#else
QJsonValueRef QJsonValueRef::operator[](qsizetype key)
{
    if (d->elements.at(index).type != QCborValue::Array)
        d->replaceAt(index, QCborValue::Array);

    auto &e = d->elements[index];
    e.container = QCborContainerPrivate::grow(e.container, key);    // detaches
    e.flags |= QtCbor::Element::IsContainer;

    return QJsonValueRef(e.container, key, false);
}

QJsonValueRef QJsonValueRef::operator[](QAnyStringView key)
{
    // must go through QJsonObject because some of the machinery is non-static
    // member or file-static in qjsonobject.cpp
    QJsonObject o = QJsonPrivate::Value::fromTrustedCbor(d->valueAt(index)).toObject();
    QJsonValueRef ret = key.visit([&](auto v) {
        if constexpr (std::is_same_v<decltype(v), QUtf8StringView>)
            return o[QString::fromUtf8(v)];
        else
            return o[v];
    });

    // ### did the QJsonObject::operator[] above detach?
    QCborContainerPrivate *x = o.o.take();
    Q_ASSERT(x->ref.loadRelaxed() == 1);

    auto &e = d->elements[index];
    if (e.flags & QtCbor::Element::IsContainer && e.container != x)
        o.o.reset(e.container);     // might not an object!

    e.flags |= QtCbor::Element::IsContainer;
    e.container = x;

    return ret;
}
#endif

size_t qHash(const QJsonValue &value, size_t seed)
{
    switch (value.type()) {
    case QJsonValue::Null:
        return qHash(nullptr, seed);
    case QJsonValue::Bool:
        return qHash(value.toBool(), seed);
    case QJsonValue::Double:
        return qHash(value.toDouble(), seed);
    case QJsonValue::String:
        return qHash(value.toString(), seed);
    case QJsonValue::Array:
        return qHash(value.toArray(), seed);
    case QJsonValue::Object:
        return qHash(value.toObject(), seed);
    case QJsonValue::Undefined:
        return seed;
    }
    Q_UNREACHABLE_RETURN(0);
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, const QJsonValue &o)
{
    QDebugStateSaver saver(dbg);
    switch (o.value.type()) {
    case QCborValue::Undefined:
        dbg << "QJsonValue(undefined)";
        break;
    case QCborValue::Null:
        dbg << "QJsonValue(null)";
        break;
    case QCborValue::True:
    case QCborValue::False:
        dbg.nospace() << "QJsonValue(bool, " << o.toBool() << ')';
        break;
    case QCborValue::Integer:
        dbg.nospace() << "QJsonValue(double, " << o.toInteger() << ')';
        break;
    case QCborValue::Double:
        dbg.nospace() << "QJsonValue(double, " << o.toDouble() << ')';
        break;
    case QCborValue::String:
        dbg.nospace() << "QJsonValue(string, " << o.toString() << ')';
        break;
    case QCborValue::Array:
        dbg.nospace() << "QJsonValue(array, ";
        dbg << o.toArray();
        dbg << ')';
        break;
    case QCborValue::Map:
        dbg.nospace() << "QJsonValue(object, ";
        dbg << o.toObject();
        dbg << ')';
        break;
    default:
        Q_UNREACHABLE();
    }
    return dbg;
}
#endif

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &stream, const QJsonValue &v)
{
    quint8 type = v.type();
    stream << type;
    switch (type) {
    case QJsonValue::Undefined:
    case QJsonValue::Null:
        break;
    case QJsonValue::Bool:
        stream << v.toBool();
        break;
    case QJsonValue::Double:
        stream << v.toDouble();
        break;
    case QJsonValue::String:
        stream << v.toString();
        break;
    case QJsonValue::Array:
        stream << v.toArray();
        break;
    case QJsonValue::Object:
        stream << v.toObject();
        break;
    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QJsonValue &v)
{
    quint8 type;
    stream >> type;
    switch (type) {
    case QJsonValue::Undefined:
    case QJsonValue::Null:
        v = QJsonValue{QJsonValue::Type(type)};
        break;
    case QJsonValue::Bool: {
        bool b;
        stream >> b;
        v = QJsonValue(b);
        break;
    } case QJsonValue::Double: {
        double d;
        stream >> d;
        v = QJsonValue{d};
        break;
    } case QJsonValue::String: {
        QString s;
        stream >> s;
        v = QJsonValue{s};
        break;
    }
    case QJsonValue::Array: {
        QJsonArray a;
        stream >> a;
        v = QJsonValue{a};
        break;
    }
    case QJsonValue::Object: {
        QJsonObject o;
        stream >> o;
        v = QJsonValue{o};
        break;
    }
    default: {
        stream.setStatus(QDataStream::ReadCorruptData);
        v = QJsonValue{QJsonValue::Undefined};
    }
    }
    return stream;
}
#endif

QT_END_NAMESPACE
