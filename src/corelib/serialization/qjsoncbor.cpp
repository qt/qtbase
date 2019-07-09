/****************************************************************************
**
** Copyright (C) 2019 Intel Corporation.
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

#include "qcborvalue.h"
#include "qcborvalue_p.h"

#include "qcborarray.h"
#include "qcbormap.h"
#include "qjson_p.h"

#include <private/qnumeric_p.h>
#include <quuid.h>

QT_BEGIN_NAMESPACE

using namespace QtCbor;

static QJsonValue fpToJson(double v)
{
    return qt_is_finite(v) ? QJsonValue(v) : QJsonValue();
}

static QString simpleTypeString(QCborValue::Type t)
{
    int simpleType = t - QCborValue::SimpleType;
    if (unsigned(simpleType) < 0x100)
        return QString::fromLatin1("simple(%1)").arg(simpleType);

    // if we got here, we got an unknown type
    qWarning("QCborValue: found unknown type 0x%x", t);
    return QString();

}

static QString encodeByteArray(const QCborContainerPrivate *d, qsizetype idx, QCborTag encoding)
{
    const ByteData *b = d->byteData(idx);
    if (!b)
        return QString();

    QByteArray data = QByteArray::fromRawData(b->byte(), b->len);
    if (encoding == QCborKnownTags::ExpectedBase16)
        data = data.toHex();
    else if (encoding == QCborKnownTags::ExpectedBase64)
        data = data.toBase64();
    else
        data = data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    return QString::fromLatin1(data, data.size());
}

static QString makeString(const QCborContainerPrivate *d, qsizetype idx);

static QString maybeEncodeTag(const QCborContainerPrivate *d)
{
    qint64 tag = d->elements.at(0).value;
    const Element &e = d->elements.at(1);
    const ByteData *b = d->byteData(e);

    switch (tag) {
    case qint64(QCborKnownTags::DateTimeString):
    case qint64(QCborKnownTags::Url):
        if (e.type == QCborValue::String)
            return makeString(d, 1);
        break;

    case qint64(QCborKnownTags::ExpectedBase64url):
    case qint64(QCborKnownTags::ExpectedBase64):
    case qint64(QCborKnownTags::ExpectedBase16):
        if (e.type == QCborValue::ByteArray)
            return encodeByteArray(d, 1, QCborTag(tag));
        break;

    case qint64(QCborKnownTags::Uuid):
        if (e.type == QCborValue::ByteArray && b->len == sizeof(QUuid))
            return QUuid::fromRfc4122(b->asByteArrayView()).toString(QUuid::WithoutBraces);
    }

    // don't know what to do, bail out
    return QString();
}

static QString encodeTag(const QCborContainerPrivate *d)
{
    QString s;
    if (!d || d->elements.size() != 2)
        return s;               // invalid (incomplete?) tag state

    s = maybeEncodeTag(d);
    if (s.isNull()) {
        // conversion failed, ignore the tag and convert the tagged value
        s = makeString(d, 1);
    }
    return s;
}

static Q_NEVER_INLINE QString makeString(const QCborContainerPrivate *d, qsizetype idx)
{
    const auto &e = d->elements.at(idx);

    switch (e.type) {
    case QCborValue::Integer:
        return QString::number(qint64(e.value));

    case QCborValue::Double:
        return QString::number(e.fpvalue());

    case QCborValue::ByteArray:
        return encodeByteArray(d, idx, QCborTag(QCborKnownTags::ExpectedBase64url));

    case QCborValue::String:
        return d->stringAt(idx);

    case QCborValue::Array:
    case QCborValue::Map:
        return d->valueAt(idx).toDiagnosticNotation(QCborValue::Compact);

    case QCborValue::SimpleType:
        break;

    case QCborValue::False:
        return QStringLiteral("false");

    case QCborValue::True:
        return QStringLiteral("true");

    case QCborValue::Null:
        return QStringLiteral("null");

    case QCborValue::Undefined:
        return QStringLiteral("undefined");

    case QCborValue::Invalid:
        return QString();

    case QCborValue::Tag:
    case QCborValue::DateTime:
    case QCborValue::Url:
    case QCborValue::RegularExpression:
    case QCborValue::Uuid:
        return encodeTag(e.flags & Element::IsContainer ? e.container : nullptr);
    }

    // maybe it's a simple type
    return simpleTypeString(e.type);
}

static QJsonValue convertToJson(const QCborContainerPrivate *d, qsizetype idx);

static QJsonArray convertToJsonArray(const QCborContainerPrivate *d)
{
    QJsonArray a;
    if (d) {
        for (qsizetype idx = 0; idx < d->elements.size(); ++idx)
            a.append(convertToJson(d, idx));
    }
    return a;
}

static QJsonObject convertToJsonObject(const QCborContainerPrivate *d)
{
    QJsonObject o;
    if (d) {
        for (qsizetype idx = 0; idx < d->elements.size(); idx += 2)
            o.insert(makeString(d, idx), convertToJson(d, idx + 1));
    }
    return o;
}

static QJsonValue convertExtendedTypeToJson(const QCborContainerPrivate *d)
{
    qint64 tag = d->elements.at(0).value;

    switch (tag) {
    case qint64(QCborKnownTags::Url):
        // use the fullly-encoded URL form
        if (d->elements.at(1).type == QCborValue::String)
            return QUrl::fromEncoded(d->byteData(1)->asByteArrayView()).toString(QUrl::FullyEncoded);
        Q_FALLTHROUGH();

    case qint64(QCborKnownTags::DateTimeString):
    case qint64(QCborKnownTags::ExpectedBase64url):
    case qint64(QCborKnownTags::ExpectedBase64):
    case qint64(QCborKnownTags::ExpectedBase16):
    case qint64(QCborKnownTags::Uuid): {
        // use the string conversion
        QString s = maybeEncodeTag(d);
        if (!s.isNull())
            return s;
    }
    }

    // for all other tags, ignore it and return the converted tagged item
    return convertToJson(d, 1);
}

static QJsonValue convertToJson(const QCborContainerPrivate *d, qsizetype idx)
{
    // encoding the container itself
    if (idx == -QCborValue::Array)
        return convertToJsonArray(d);
    if (idx == -QCborValue::Map)
        return convertToJsonObject(d);
    if (idx < 0) {
        // tag-like type
        if (!d || d->elements.size() != 2)
            return QJsonValue::Undefined;   // invalid state
        return convertExtendedTypeToJson(d);
    }

    // an element in the container
    const auto &e = d->elements.at(idx);
    switch (e.type) {
    case QCborValue::Integer:
        return qint64(e.value);

    case QCborValue::ByteArray:
    case QCborValue::String:
    case QCborValue::SimpleType:
        // make string
        break;

    case QCborValue::Array:
    case QCborValue::Map:
    case QCborValue::Tag:
    case QCborValue::DateTime:
    case QCborValue::Url:
    case QCborValue::RegularExpression:
    case QCborValue::Uuid:
        // recurse
        return convertToJson(e.flags & Element::IsContainer ? e.container : nullptr, -e.type);

    case QCborValue::Null:
        return QJsonValue();

    case QCborValue::Undefined:
    case QCborValue::Invalid:
        return QJsonValue(QJsonValue::Undefined);

    case QCborValue::False:
        return false;

    case QCborValue::True:
        return true;

    case QCborValue::Double:
        return fpToJson(e.fpvalue());
    }

    return makeString(d, idx);
}

/*!
    Converts this QCborValue object to an equivalent representation in JSON and
    returns it as a QJsonValue.

    Please note that CBOR contains a richer and wider type set than JSON, so
    some information may be lost in this conversion. The following table
    compares CBOR types to JSON types and indicates whether information may be
    lost or not.

    \table
      \header \li CBOR Type     \li JSON Type   \li Comments
      \row  \li Bool            \li Bool        \li No data loss possible
      \row  \li Double          \li Number      \li Infinities and NaN will be converted to Null;
                                                    no data loss for other values
      \row  \li Integer         \li Number      \li Data loss possible in the conversion if the
                                                    integer is larger than 2\sup{53} or smaller
                                                    than -2\sup{53}.
      \row  \li Null            \li Null        \li No data loss possible
      \row  \li Undefined       \li Null        \li Type information lost
      \row  \li String          \li String      \li No data loss possible
      \row  \li Byte Array      \li String      \li Converted to a lossless encoding like Base64url,
                                                    but the distinction between strings and byte
                                                    arrays is lost
      \row  \li Other simple types \li String   \li Type information lost
      \row  \li Array           \li Array       \li Conversion applies to each contained value
      \row  \li Map             \li Object      \li Keys are converted to string; values converted
                                                    according to this table
      \row  \li Tags and extended types \li Special \li The tag number itself is lost and the tagged
                                                    value is converted to JSON
    \endtable

    For information on the conversion of CBOR map keys to string, see
    QCborMap::toJsonObject().

    If this QCborValue contains the undefined value, this function will return
    an undefined QJsonValue too. Note that JSON does not support undefined
    values and undefined QJsonValues are an extension to the specification.
    They cannot be held in a QJsonArray or QJsonObject, but can be returned
    from functions to indicate a failure. For all other intents and purposes,
    they are the same as null.

    \section3 Special handling of tags and extended types

    Some tags are handled specially and change the transformation of the tagged
    value from CBOR to JSON. The following table lists those special cases:

    \table
      \header \li Tag           \li CBOR type       \li Transformation
      \row  \li ExpectedBase64url \li Byte array    \li Encodes the byte array as Base64url
      \row  \li ExpectedBase64  \li Byte array      \li Encodes the byte array as Base64
      \row  \li ExpectedBase16  \li Byte array      \li Encodes the byte array as hex
      \row  \li Url             \li Url and String  \li Uses QUrl::toEncoded() to normalize the
                                                    encoding to the URL's fully encoded format
      \row  \li Uuid            \li Uuid and Byte array \li Uses QUuid::toString() to create
                                                    the string representation
    \endtable

    \sa fromJsonValue(), toVariant(), QCborArray::toJsonArray(), QCborMap::toJsonObject()
 */
QJsonValue QCborValue::toJsonValue() const
{
    if (container)
        return convertToJson(container, n < 0 ? -type() : n);

    // simple values
    switch (type()) {
    case Integer:
        return n;

    case Null:
        return QJsonValue();

    case False:
        return false;

    case True:
        return true;

    case Double:
        return fpToJson(fp_helper());

    case SimpleType:
        break;

    case Undefined:
    case Invalid:
        return QJsonValue(QJsonValue::Undefined);

    case ByteArray:
    case String:
        // empty strings
        return QString();

    case Array:
        // empty array
        return QJsonArray();

    case Map:
        // empty map
        return QJsonObject();

    case Tag:
    case DateTime:
    case Url:
    case RegularExpression:
    case Uuid:
        Q_UNREACHABLE();
        return QJsonValue::Undefined;
    }

    return simpleTypeString(type());
}

QJsonValue QCborValueRef::toJsonValue() const
{
    return convertToJson(d, i);
}

/*!
    Recursively converts every \l QCborValue element in this array to JSON
    using QCborValue::toJsonValue() and returns the corresponding QJsonArray
    composed of those elements.

    Please note that CBOR contains a richer and wider type set than JSON, so
    some information may be lost in this conversion. For more details on what
    conversions are applied, see QCborValue::toJsonValue().

    \sa fromJsonArray(), QCborValue::toJsonValue(), QCborMap::toJsonObject(), toVariantList()
 */
QJsonArray QCborArray::toJsonArray() const
{
    return convertToJsonArray(d.data());
}

/*!
    Recursively converts every \l QCborValue value in this map to JSON using
    QCborValue::toJsonValue() and creates a string key for all keys that aren't
    strings, then returns the corresponding QJsonObject composed of those
    associations.

    Please note that CBOR contains a richer and wider type set than JSON, so
    some information may be lost in this conversion. For more details on what
    conversions are applied, see QCborValue::toJsonValue().

    \section3 Map key conversion to string

    JSON objects are defined as having string keys, unlike CBOR, so the
    conversion of a QCborMap to QJsonObject will imply a step of
    "stringification" of the key values. The conversion will use the special
    handling of tags and extended types from above and will also convert the
    rest of the types as follows:

    \table
      \header \li Type              \li Transformation
      \row  \li Bool                \li "true" and "false"
      \row  \li Null                \li "null"
      \row  \li Undefined           \li "undefined"
      \row  \li Integer             \li The decimal string form of the number
      \row  \li Double              \li The decimal string form of the number
      \row  \li Byte array          \li Unless tagged differently (see above), encoded as
                                        Base64url
      \row  \li Array               \li Replaced by the compact form of its
                                        \l{QCborValue::toDiagnosticNotation()}{Diagnostic notation}
      \row  \li Map                 \li Replaced by the compact form of its
                                        \l{QCborValue::toDiagnosticNotation()}{Diagnostic notation}
      \row  \li Tags and extended types \li Tag number is dropped and the tagged value is converted
                                        to string
    \endtable

    \sa fromJsonObject(), QCborValue::toJsonValue(), QCborArray::toJsonArray(), toVariantMap()
 */
QJsonObject QCborMap::toJsonObject() const
{
    return convertToJsonObject(d.data());
}

/*!
    Converts this value to a native Qt type and returns the corresponding QVariant.

    The following table lists the mapping performed between \l{Type}{QCborValue
    types} and \l{QMetaType::Type}{Qt meta types}.

    \table
      \header \li CBOR Type         \li Qt or C++ type          \li Notes
      \row  \li Integer             \li \l qint64               \li
      \row  \li Double              \li \c double               \li
      \row  \li Bool                \li \c bool                 \li
      \row  \li Null                \li \c std::nullptr_t       \li
      \row  \li Undefined           \li no type (QVariant())    \li
      \row  \li Byte array          \li \l QByteArray           \li
      \row  \li String              \li \l QString              \li
      \row  \li Array               \li \l QVariantList         \li Recursively converts all values
      \row  \li Map                 \li \l QVariantMap          \li Key types are "stringified"
      \row  \li Other simple types  \li \l QCborSimpleType      \li
      \row  \li DateTime            \li \l QDateTime            \li
      \row  \li Url                 \li \l QUrl                 \li
      \row  \li RegularExpression   \li \l QRegularExpression   \li
      \row  \li Uuid                \li \l QUuid                \li
      \row  \li Other tags          \li Special                 \li The tag is ignored and the tagged
                                                                    value is converted using this
                                                                    function
    \endtable

    Note that values in both CBOR Maps and Arrays are converted recursively
    using this function too and placed in QVariantMap and QVariantList instead.
    You will not find QCborMap and QCborArray stored inside the QVariants.

    QVariantMaps have string keys, unlike CBOR, so the conversion of a QCborMap
    to QVariantMap will imply a step of "stringification" of the key values.
    See QCborMap::toJsonObject() for details.

    \sa fromVariant(), toJsonValue(), QCborArray::toVariantList(), QCborMap::toVariantMap()
 */
QVariant QCborValue::toVariant() const
{
    switch (type()) {
    case Integer:
        return toInteger();

    case Double:
        return toDouble();

    case SimpleType:
        break;

    case False:
    case True:
        return isTrue();

    case Null:
        return QVariant::fromValue(nullptr);

    case Undefined:
        return QVariant();

    case ByteArray:
        return toByteArray();

    case String:
        return toString();

    case Array:
        return toArray().toVariantList();

    case Map:
        return toMap().toVariantMap();

    case Tag:
        // ignore tags
        return taggedValue().toVariant();

    case DateTime:
        return toDateTime();

    case Url:
        return toUrl();

#if QT_CONFIG(regularexpression)
    case RegularExpression:
        return toRegularExpression();
#endif

    case Uuid:
        return toUuid();

    case Invalid:
        return QVariant();

    default:
        break;
    }

    if (isSimpleType())
        return QVariant::fromValue(toSimpleType());

    Q_UNREACHABLE();
    return QVariant();
}

/*!
    Converts the JSON value contained in \a v into its corresponding CBOR value
    and returns it. There is no data loss in converting from JSON to CBOR, as
    the CBOR type set is richer than JSON's. Additionally, values converted to
    CBOR using this function can be converted back to JSON using toJsonValue()
    with no data loss.

    The following table lists the mapping of JSON types to CBOR types:

    \table
      \header \li JSON Type     \li CBOR Type
      \row  \li Bool            \li Bool
      \row  \li Number          \li Integer (if the number has no fraction and is in the \l qint64
                                    range) or Double
      \row  \li String          \li String
      \row  \li Array           \li Array
      \row  \li Object          \li Map
      \row  \li Null            \li Null
    \endtable

    \l QJsonValue can also be undefined, indicating a previous operation that
    failed to complete (for example, searching for a key not present in an
    object). Undefined values are not JSON types and may not appear in JSON
    arrays and objects, but this function does return the QCborValue undefined
    value if the corresponding QJsonValue is undefined.

    \sa toJsonValue(), fromVariant(), QCborArray::fromJsonArray(), QCborMap::fromJsonObject()
 */
QCborValue QCborValue::fromJsonValue(const QJsonValue &v)
{
    switch (v.type()) {
    case QJsonValue::Bool:
        return v.b;
    case QJsonValue::Double: {
        qint64 i;
        if (convertDoubleTo(v.dbl, &i))
            return i;
        return v.dbl;
    }
    case QJsonValue::String:
        return v.toString();
    case QJsonValue::Array:
        return QCborArray::fromJsonArray(v.toArray());
    case QJsonValue::Object:
        return QCborMap::fromJsonObject(v.toObject());
    case QJsonValue::Null:
        return nullptr;
    case QJsonValue::Undefined:
        break;
    }
    return QCborValue();
}

static void appendVariant(QCborContainerPrivate *d, const QVariant &variant)
{
    // Handle strings and byte arrays directly, to avoid creating a temporary
    // dummy container to hold their data.
    int type = variant.userType();
    if (type == QVariant::String) {
        d->append(variant.toString());
    } else if (type == QVariant::ByteArray) {
        QByteArray ba = variant.toByteArray();
        d->appendByteData(ba.constData(), ba.size(), QCborValue::ByteArray);
    } else {
        // For everything else, use the function below.
        d->append(QCborValue::fromVariant(variant));
    }
}

/*!
    Converts the QVariant \a variant into QCborValue and returns it.

    QVariants may contain a large list of different meta types, many of which
    have no corresponding representation in CBOR. That includes all
    user-defined meta types. When preparing transmission using CBOR, it is
    suggested to encode carefully each value to prevent loss of representation.

    The following table lists the conversion this function will apply:

    \table
      \header \li Qt (C++) type             \li CBOR type
      \row  \li invalid (QVariant())        \li Undefined
      \row  \li \c bool                     \li Bool
      \row  \li \c std::nullptr_t           \li Null
      \row  \li \c short, \c ushort, \c int, \c uint, \l qint64  \li Integer
      \row  \li \l quint64                  \li Integer, but they are cast to \c qint64 first so
                                                values higher than 2\sup{63}-1 (\c INT64_MAX) will
                                                be wrapped to negative
      \row  \li \c float, \c double         \li Double
      \row  \li \l QByteArray               \li ByteArray
      \row  \li \l QDateTime                \li DateTime
      \row  \li \l QCborSimpleType          \li Simple type
      \row  \li \l QJsonArray               \li Array, converted using QCborArray::formJsonArray()
      \row  \li \l QJsonDocument            \li Array or Map
      \row  \li \l QJsonObject              \li Map, converted using QCborMap::fromJsonObject()
      \row  \li \l QJsonValue               \li converted using fromJsonValue()
      \row  \li \l QRegularExpression       \li RegularExpression
      \row  \li \l QString                  \li String
      \row  \li \l QStringList              \li Array
      \row  \li \l QVariantHash             \li Map
      \row  \li \l QVariantList             \li Array
      \row  \li \l QVariantMap              \li Map
      \row  \li \l QUrl                     \li Url
      \row  \li \l QUuid                    \li Uuid
    \endtable

    For any other types, this function will return Null if the QVariant itself
    is null, and otherwise will try to convert to string using
    QVariant::toString(). If the conversion to string fails, this function
    returns Undefined.

    Please note that the conversions via QVariant::toString() are subject to
    change at any time. QCborValue may be extended in the future to support
    more types, which will result in a change in how this function performs
    conversions.

    \sa toVariant(), fromJsonValue(), QCborArray::toVariantList(), QCborMap::toVariantMap()
 */
QCborValue QCborValue::fromVariant(const QVariant &variant)
{
    switch (variant.userType()) {
    case QVariant::Invalid:
        return {};
    case QMetaType::Nullptr:
        return nullptr;
    case QVariant::Bool:
        return variant.toBool();
    case QMetaType::Short:
    case QMetaType::UShort:
    case QVariant::Int:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::UInt:
        return variant.toLongLong();
    case QMetaType::Float:
    case QVariant::Double:
        return variant.toDouble();
    case QVariant::String:
        return variant.toString();
    case QVariant::StringList:
        return QCborArray::fromStringList(variant.toStringList());
    case QVariant::ByteArray:
        return variant.toByteArray();
    case QVariant::DateTime:
        return QCborValue(variant.toDateTime());
    case QVariant::Url:
        return QCborValue(variant.toUrl());
    case QVariant::Uuid:
        return QCborValue(variant.toUuid());
    case QVariant::List:
        return QCborArray::fromVariantList(variant.toList());
    case QVariant::Map:
        return QCborMap::fromVariantMap(variant.toMap());
    case QVariant::Hash:
        return QCborMap::fromVariantHash(variant.toHash());
#ifndef QT_BOOTSTRAPPED
#if QT_CONFIG(regularexpression)
    case QVariant::RegularExpression:
        return QCborValue(variant.toRegularExpression());
#endif
    case QMetaType::QJsonValue:
        return fromJsonValue(variant.toJsonValue());
    case QMetaType::QJsonObject:
        return QCborMap::fromJsonObject(variant.toJsonObject());
    case QMetaType::QJsonArray:
        return QCborArray::fromJsonArray(variant.toJsonArray());
    case QMetaType::QJsonDocument: {
        QJsonDocument doc = variant.toJsonDocument();
        if (doc.isArray())
            return QCborArray::fromJsonArray(doc.array());
        return QCborMap::fromJsonObject(doc.object());
    }
    case QMetaType::QCborValue:
        return variant.value<QCborValue>();
    case QMetaType::QCborArray:
        return variant.value<QCborArray>();
    case QMetaType::QCborMap:
        return variant.value<QCborMap>();
    case QMetaType::QCborSimpleType:
        return variant.value<QCborSimpleType>();
#endif
    default:
        break;
    }

    if (variant.isNull())
        return QCborValue(nullptr);

    QString string = variant.toString();
    if (string.isNull())
        return QCborValue();        // undefined
    return string;
}

/*!
    Recursively converts each \l QCborValue in this array using
    QCborValue::toVariant() and returns the QVariantList composed of the
    converted items.

    Conversion to \l QVariant is not completely lossless. Please see the
    documentation in QCborValue::toVariant() for more information.

    \sa fromVariantList(), fromStringList(), toJsonArray(),
        QCborValue::toVariant(), QCborMap::toVariantMap()
 */
QVariantList QCborArray::toVariantList() const
{
    QVariantList retval;
    retval.reserve(size());
    for (qsizetype i = 0; i < size(); ++i)
        retval.append(d->valueAt(i).toVariant());
    return retval;
}

/*!
    Returns a QCborArray containing all the strings found in the \a list list.

    \sa fromVariantList(), fromJsonArray()
 */
QCborArray QCborArray::fromStringList(const QStringList &list)
{
    QCborArray a;
    a.detach(list.size());
    for (const QString &s : list)
        a.d->append(s);
    return a;
}

/*!
    Converts all the items in the \a list to CBOR using
    QCborValue::fromVariant() and returns the array composed of those elements.

    Conversion from \l QVariant is not completely lossless. Please see the
    documentation in QCborValue::fromVariant() for more information.

    \sa toVariantList(), fromStringList(), fromJsonArray(), QCborMap::fromVariantMap()
 */
QCborArray QCborArray::fromVariantList(const QVariantList &list)
{
    QCborArray a;
    a.detach(list.size());
    for (const QVariant &v : list)
        appendVariant(a.d.data(), v);
    return a;
}

/*!
    Converts all JSON items found in the \a array array to CBOR using
    QCborValue::fromJson(), and returns the CBOR array composed of those
    elements.

    This conversion is lossless, as the CBOR type system is a superset of
    JSON's. Moreover, the array returned by this function can be converted back
    to the original \a array by using toJsonArray().

    \sa toJsonArray(), toVariantList(), QCborValue::fromJsonValue(), QCborMap::fromJsonObject()
 */
QCborArray QCborArray::fromJsonArray(const QJsonArray &array)
{
    QCborArray a;
    a.detach(array.size());
    for (const QJsonValue &v : array) {
        if (v.isString())
            a.d->append(v.toString());
        else
            a.d->append(QCborValue::fromJsonValue(v));
    }
    return a;
}

/*!
    Converts the CBOR values to QVariant using QCborValue::toVariant() and
    "stringifies" all the CBOR keys in this map, returning the QVariantMap that
    results from that association list.

    QVariantMaps have string keys, unlike CBOR, so the conversion of a QCborMap
    to QVariantMap will imply a step of "stringification" of the key values.
    See QCborMap::toJsonObject() for details.

    In addition, the conversion to \l QVariant is not completely lossless.
    Please see the documentation in QCborValue::toVariant() for more
    information.

    \sa fromVariantMap(), toVariantHash(), toJsonObject(), QCborValue::toVariant(),
        QCborArray::toVariantList()
 */
QVariantMap QCborMap::toVariantMap() const
{
    QVariantMap retval;
    for (qsizetype i = 0; i < 2 * size(); i += 2)
        retval.insert(makeString(d.data(), i), d->valueAt(i + 1).toVariant());
    return retval;
}

/*!
    Converts the CBOR values to QVariant using QCborValue::toVariant() and
    "stringifies" all the CBOR keys in this map, returning the QVariantHash that
    results from that association list.

    QVariantMaps have string keys, unlike CBOR, so the conversion of a QCborMap
    to QVariantMap will imply a step of "stringification" of the key values.
    See QCborMap::toJsonObject() for details.

    In addition, the conversion to \l QVariant is not completely lossless.
    Please see the documentation in QCborValue::toVariant() for more
    information.

    \sa fromVariantHash(), toVariantMap(), toJsonObject(), QCborValue::toVariant(),
        QCborArray::toVariantList()
 */
QVariantHash QCborMap::toVariantHash() const
{
    QVariantHash retval;
    retval.reserve(size());
    for (qsizetype i = 0; i < 2 * size(); i += 2)
        retval.insert(makeString(d.data(), i), d->valueAt(i + 1).toVariant());
    return retval;
}

/*!
    Converts all the items in \a map to CBOR using QCborValue::fromVariant()
    and returns the map composed of those elements.

    Conversion from \l QVariant is not completely lossless. Please see the
    documentation in QCborValue::fromVariant() for more information.

    \sa toVariantMap(), fromVariantHash(), fromJsonObject(), QCborValue::fromVariant()
 */
QCborMap QCborMap::fromVariantMap(const QVariantMap &map)
{
    QCborMap m;
    m.detach(map.size());
    QCborContainerPrivate *d = m.d.data();

    auto it = map.begin();
    auto end = map.end();
    for ( ; it != end; ++it) {
        d->append(it.key());
        appendVariant(d, it.value());
    }
    return m;
}

/*!
    Converts all the items in \a hash to CBOR using QCborValue::fromVariant()
    and returns the map composed of those elements.

    Conversion from \l QVariant is not completely lossless. Please see the
    documentation in QCborValue::fromVariant() for more information.

    \sa toVariantHash(), fromVariantMap(), fromJsonObject(), QCborValue::fromVariant()
 */
QCborMap QCborMap::fromVariantHash(const QVariantHash &hash)
{
    QCborMap m;
    m.detach(hash.size());
    QCborContainerPrivate *d = m.d.data();

    auto it = hash.begin();
    auto end = hash.end();
    for ( ; it != end; ++it) {
        d->append(it.key());
        appendVariant(d, it.value());
    }
    return m;
}

/*!
    Converts all JSON items found in the \a obj object to CBOR using
    QCborValue::fromJson(), and returns the map composed of those elements.

    This conversion is lossless, as the CBOR type system is a superset of
    JSON's. Moreover, the map returned by this function can be converted back
    to the original \a obj by using toJsonObject().

    \sa toJsonObject(), toVariantMap(), QCborValue::fromJsonValue(), QCborArray::fromJsonArray()
 */
QCborMap QCborMap::fromJsonObject(const QJsonObject &obj)
{
    QCborMap m;
    m.detach(obj.size());
    QCborContainerPrivate *d = m.d.data();

    auto it = obj.begin();
    auto end = obj.end();
    for ( ; it != end; ++it) {
        d->append(it.key());
        if (it.value().isString())
            d->append(it.value().toString());
        else
            d->append(QCborValue::fromJsonValue(it.value()));
    }
    return m;
}

QT_END_NAMESPACE
