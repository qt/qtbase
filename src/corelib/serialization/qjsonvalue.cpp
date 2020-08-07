/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qurl.h>
#include <quuid.h>
#include <qvariant.h>
#include <qstringlist.h>
#include <qdebug.h>
#include "qdatastream.h"

#include <private/qnumeric_p.h>
#include <private/qcborvalue_p.h>

#include <qcborarray.h>
#include <qcbormap.h>

#include "qjson_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QJsonValue
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The QJsonValue class encapsulates a value in JSON.

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
    \li \l {QJsonArray}::operator[](int i)
    \li \l {QJsonObject}::operator[](const QString & key) const
    \endlist

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    Creates a QJsonValue of type \a type.

    The default is to create a Null value.
 */
QJsonValue::QJsonValue(Type type)
    : d(nullptr), t(QCborValue::Undefined)
{
    switch (type) {
    case Null:
        t = QCborValue::Null;
        break;
    case Bool:
        t = QCborValue::False;
        break;
    case Double:
        t = QCborValue::Double;
        break;
    case String:
        t = QCborValue::String;
        break;
    case Array:
        t = QCborValue::Array;
        break;
    case Object:
        t = QCborValue::Map;
        break;
    case Undefined:
        break;
    }
}

/*!
    Creates a value of type Bool, with value \a b.
 */
QJsonValue::QJsonValue(bool b)
    : t(b ? QCborValue::True : QCborValue::False)
{
}

/*!
    Creates a value of type Double, with value \a v.
 */
QJsonValue::QJsonValue(double v)
    : d(nullptr)
{
    if (convertDoubleTo(v, &n)) {
        t = QCborValue::Integer;
    } else {
        memcpy(&n, &v, sizeof(n));
        t = QCborValue::Double;
    }
}

/*!
    \overload
    Creates a value of type Double, with value \a v.
 */
QJsonValue::QJsonValue(int v)
    : n(v), t(QCborValue::Integer)
{
}

/*!
    \overload
    Creates a value of type Double, with value \a v.
    NOTE: the integer limits for IEEE 754 double precision data is 2^53 (-9007199254740992 to +9007199254740992).
    If you pass in values outside this range expect a loss of precision to occur.
 */
QJsonValue::QJsonValue(qint64 v)
    : n(v), t(QCborValue::Integer)
{
}

/*!
    Creates a value of type String, with value \a s.
 */
QJsonValue::QJsonValue(const QString &s)
    : QJsonValue(QJsonPrivate::Value::fromTrustedCbor(s))
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

// ### Qt6: remove
void QJsonValue::stringDataFromQStringHelper(const QString &string)
{
    *this = QJsonValue(string);
}

/*!
    Creates a value of type String, with value \a s.
 */
QJsonValue::QJsonValue(QLatin1String s)
    : QJsonValue(QJsonPrivate::Value::fromTrustedCbor(s))
{
}

/*!
    Creates a value of type Array, with value \a a.
 */
QJsonValue::QJsonValue(const QJsonArray &a)
    : n(-1), d(a.a), t(QCborValue::Array)
{
}

/*!
    Creates a value of type Object, with value \a o.
 */
QJsonValue::QJsonValue(const QJsonObject &o)
    : n(-1), d(o.o), t(QCborValue::Map)
{
}


/*!
    Destroys the value.
 */
QJsonValue::~QJsonValue() = default;

/*!
    Creates a copy of \a other.
 */
QJsonValue::QJsonValue(const QJsonValue &other)
{
    n = other.n;
    t = other.t;
    d = other.d;
}

/*!
    Assigns the value stored in \a other to this object.
 */
QJsonValue &QJsonValue::operator =(const QJsonValue &other)
{
    QJsonValue copy(other);
    swap(copy);
    return *this;
}

QJsonValue::QJsonValue(QJsonValue &&other) noexcept :
    n(other.n),
    d(other.d),
    t(other.t)
{
    other.n = 0;
    other.d = nullptr;
    other.t = QCborValue::Null;
}

void QJsonValue::swap(QJsonValue &other) noexcept
{
    qSwap(n, other.n);
    qSwap(d, other.d);
    qSwap(t, other.t);
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

    Swaps the value \a other with this. This operation is very fast and never fails.
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
    to pre-encode it to \l {https://www.ietf.org/rfc/rfc4648.txt}{Base64} (or
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
    switch (variant.userType()) {
    case QMetaType::Nullptr:
        return QJsonValue(Null);
    case QMetaType::Bool:
        return QJsonValue(variant.toBool());
    case QMetaType::Int:
    case QMetaType::Float:
    case QMetaType::Double:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::UInt: {
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
    switch (t) {
    case QCborValue::True:
        return true;
    case QCborValue::False:
        return false;
    case QCborValue::Integer:
        return n;
    case QCborValue::Double:
        return toDouble();
    case QCborValue::String:
        return toString();
    case QCborValue::Array:
        return d ?
               QJsonArray(d.data()).toVariantList() :
               QVariantList();
    case QCborValue::Map:
        return d ?
               QJsonObject(d.data()).toVariantMap() :
               QVariantMap();
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
    \value Double   A double. Use toDouble() to convert to a double.
    \value String   A string. Use toString() to convert to a QString.
    \value Array    An array. Use toArray() to convert to a QJsonArray.
    \value Object   An object. Use toObject() to convert to a QJsonObject.
    \value Undefined The value is undefined. This is usually returned as an
                    error condition, when trying to read an out of bounds value
                    in an array or a non existent key in an object.
*/

/*!
    Returns the type of the value.

    \sa QJsonValue::Type
 */
QJsonValue::Type QJsonValue::type() const
{
    switch (t) {
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
    Converts the value to a bool and returns it.

    If type() is not bool, the \a defaultValue will be returned.
 */
bool QJsonValue::toBool(bool defaultValue) const
{
    switch (t) {
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
    switch (t) {
    case QCborValue::Double: {
        const double dbl = toDouble();
        int dblInt;
        convertDoubleTo<int>(dbl, &dblInt);
        return dbl == dblInt ? dblInt : defaultValue;
    }
    case QCborValue::Integer:
        return (n <= qint64(std::numeric_limits<int>::max())
                && n >= qint64(std::numeric_limits<int>::min()))
                ? n : defaultValue;
    default:
        return defaultValue;
    }
}

/*!
    Converts the value to a double and returns it.

    If type() is not Double, the \a defaultValue will be returned.
 */
double QJsonValue::toDouble(double defaultValue) const
{
    switch (t) {
    case QCborValue::Double: {
        double d;
        memcpy(&d, &n, sizeof(d));
        return d;
    }
    case QCborValue::Integer:
        return n;
    default:
        return defaultValue;
    }
}

/*!
    Converts the value to a QString and returns it.

    If type() is not String, the \a defaultValue will be returned.
 */
QString QJsonValue::toString(const QString &defaultValue) const
{
    return (t == QCborValue::String && d) ? d->stringAt(n) : defaultValue;
}

/*!
    Converts the value to a QString and returns it.

    If type() is not String, a null QString will be returned.

    \sa QString::isNull()
 */
QString QJsonValue::toString() const
{
    return (t == QCborValue::String && d) ? d->stringAt(n) : QString();
}

/*!
    Converts the value to an array and returns it.

    If type() is not Array, the \a defaultValue will be returned.
 */
QJsonArray QJsonValue::toArray(const QJsonArray &defaultValue) const
{
    if (t != QCborValue::Array || n >= 0 || !d)
        return defaultValue;

    return QJsonArray(d.data());
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
    if (t != QCborValue::Map || n >= 0 || !d)
        return defaultValue;

    return QJsonObject(d.data());
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

#if QT_STRINGVIEW_LEVEL < 2
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
#endif

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
const QJsonValue QJsonValue::operator[](QLatin1String key) const
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
const QJsonValue QJsonValue::operator[](int i) const
{
    if (!isArray())
        return QJsonValue(QJsonValue::Undefined);

    return toArray().at(i);
}

/*!
    Returns \c true if the value is equal to \a other.
 */
bool QJsonValue::operator==(const QJsonValue &other) const
{
    if (t != other.t)
        return false;

    switch (t) {
    case QCborValue::Undefined:
    case QCborValue::Null:
    case QCborValue::True:
    case QCborValue::False:
        break;
    case QCborValue::Double:
        return toDouble() == other.toDouble();
    case QCborValue::Integer:
        return n == other.n;
    case QCborValue::String:
        return toString() == other.toString();
    case QCborValue::Array:
        if (!d)
            return !other.d || other.d->elements.length() == 0;
        if (!other.d)
            return d->elements.length() == 0;
        return QJsonArray(d.data()) == QJsonArray(other.d.data());
    case QCborValue::Map:
        if (!d)
            return !other.d || other.d->elements.length() == 0;
        if (!other.d)
            return d->elements.length() == 0;
        return QJsonObject(d.data()) == QJsonObject(other.d.data());
    default:
        return false;
    }
    return true;
}

/*!
    Returns \c true if the value is not equal to \a other.
 */
bool QJsonValue::operator!=(const QJsonValue &other) const
{
    return !(*this == other);
}

/*!
    \internal
 */
void QJsonValue::detach()
{
    d.detach();
}


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


QJsonValueRef &QJsonValueRef::operator =(const QJsonValue &val)
{
    if (is_object)
        o->setValueAt(index, val);
    else
        a->replace(index, val);

    return *this;
}

QJsonValueRef &QJsonValueRef::operator =(const QJsonValueRef &ref)
{
    if (is_object)
        o->setValueAt(index, ref);
    else
        a->replace(index, ref);

    return *this;
}

QVariant QJsonValueRef::toVariant() const
{
    return toValue().toVariant();
}

QJsonArray QJsonValueRef::toArray() const
{
    return toValue().toArray();
}

QJsonObject QJsonValueRef::toObject() const
{
    return toValue().toObject();
}

QJsonValue QJsonValueRef::toValue() const
{
    if (!is_object)
        return a->at(index);
    return o->valueAt(index);
}

uint qHash(const QJsonValue &value, uint seed)
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
    Q_UNREACHABLE();
    return 0;
}

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
QDebug operator<<(QDebug dbg, const QJsonValue &o)
{
    QDebugStateSaver saver(dbg);
    switch (o.type()) {
    case QJsonValue::Undefined:
        dbg << "QJsonValue(undefined)";
        break;
    case QJsonValue::Null:
        dbg << "QJsonValue(null)";
        break;
    case QJsonValue::Bool:
        dbg.nospace() << "QJsonValue(bool, " << o.toBool() << ')';
        break;
    case QJsonValue::Double:
        dbg.nospace() << "QJsonValue(double, " << o.toDouble() << ')';
        break;
    case QJsonValue::String:
        dbg.nospace() << "QJsonValue(string, " << o.toString() << ')';
        break;
    case QJsonValue::Array:
        dbg.nospace() << "QJsonValue(array, ";
        dbg << o.toArray();
        dbg << ')';
        break;
    case QJsonValue::Object:
        dbg.nospace() << "QJsonValue(object, ";
        dbg << o.toObject();
        dbg << ')';
        break;
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
