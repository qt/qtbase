// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qstringlist.h>
#include <qvariant.h>
#include <qmap.h>
#include <qhash.h>
#include <qdebug.h>
#include <qcbormap.h>
#include <qcborarray.h>
#include "qjsonparser_p.h"
#include "qjson_p.h"
#include "qdatastream.h"

QT_BEGIN_NAMESPACE

/*! \class QJsonDocument
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \ingroup qtserialization
    \reentrant
    \since 5.0

    \brief The QJsonDocument class provides a way to read and write JSON documents.

    \compares equality

    QJsonDocument is a class that wraps a complete JSON document and can read
    this document from, and write it to, a UTF-8 encoded text-based
    representation.

    A JSON document can be converted from its text-based representation to a QJsonDocument
    using QJsonDocument::fromJson(). toJson() converts it back to text. The parser is very
    fast and efficient and converts the JSON to the binary representation used by Qt.

    Validity of the parsed document can be queried with !isNull()

    A document can be queried as to whether it contains an array or an object using isArray()
    and isObject(). The array or object contained in the document can be retrieved using
    array() or object() and then read or manipulated.

    \sa {JSON Support in Qt}, {Saving and Loading a Game}
*/


class QJsonDocumentPrivate
{
    Q_DISABLE_COPY_MOVE(QJsonDocumentPrivate);
public:
    QJsonDocumentPrivate() = default;
    QJsonDocumentPrivate(QCborValue data) : value(std::move(data)) {}

    QCborValue value;
};

/*!
 * Constructs an empty and invalid document.
 */
QJsonDocument::QJsonDocument()
    : d(nullptr)
{
}

/*!
 * Creates a QJsonDocument from \a object.
 */
QJsonDocument::QJsonDocument(const QJsonObject &object)
    : d(nullptr)
{
    setObject(object);
}

/*!
 * Constructs a QJsonDocument from \a array.
 */
QJsonDocument::QJsonDocument(const QJsonArray &array)
    : d(nullptr)
{
    setArray(array);
}

/*!
    \internal
 */
QJsonDocument::QJsonDocument(const QCborValue &data)
    : d(std::make_unique<QJsonDocumentPrivate>(data))
{
    Q_ASSERT(d);
}

/*!
 Deletes the document.

 Binary data set with fromRawData is not freed.
 */
QJsonDocument::~QJsonDocument() = default;

/*!
 * Creates a copy of the \a other document.
 */
QJsonDocument::QJsonDocument(const QJsonDocument &other)
{
    if (other.d) {
        if (!d)
            d = std::make_unique<QJsonDocumentPrivate>();
        d->value = other.d->value;
    } else {
        d.reset();
    }
}

QJsonDocument::QJsonDocument(QJsonDocument &&other) noexcept
    : d(std::move(other.d))
{
}

void QJsonDocument::swap(QJsonDocument &other) noexcept
{
    qSwap(d, other.d);
}

/*!
 * Assigns the \a other document to this QJsonDocument.
 * Returns a reference to this object.
 */
QJsonDocument &QJsonDocument::operator =(const QJsonDocument &other)
{
    if (this != &other) {
        if (other.d) {
            if (!d)
                d = std::make_unique<QJsonDocumentPrivate>();
            d->value = other.d->value;
        } else {
            d.reset();
        }
    }
    return *this;
}

/*!
    \fn QJsonDocument::QJsonDocument(QJsonDocument &&other)
    \since 5.10

    Move-constructs a QJsonDocument from \a other.
*/

/*!
    \fn QJsonDocument &QJsonDocument::operator =(QJsonDocument &&other)
    \since 5.10

    Move-assigns \a other to this document.
*/

/*!
    \fn void QJsonDocument::swap(QJsonDocument &other)
    \since 5.10
    \memberswap{document}
*/

#ifndef QT_NO_VARIANT
/*!
 Creates a QJsonDocument from the QVariant \a variant.

 If the \a variant contains any other type than a QVariantMap,
 QVariantHash, QVariantList or QStringList, the returned document is invalid.

 \sa toVariant()
 */
QJsonDocument QJsonDocument::fromVariant(const QVariant &variant)
{
    QJsonDocument doc;

    switch (variant.metaType().id()) {
    case QMetaType::QVariantMap:
        doc.setObject(QJsonObject::fromVariantMap(variant.toMap()));
        break;
    case QMetaType::QVariantHash:
        doc.setObject(QJsonObject::fromVariantHash(variant.toHash()));
        break;
    case QMetaType::QVariantList:
        doc.setArray(QJsonArray::fromVariantList(variant.toList()));
        break;
    case QMetaType::QStringList:
        doc.d = std::make_unique<QJsonDocumentPrivate>();
        doc.d->value = QCborArray::fromStringList(variant.toStringList());
        break;
    default:
        break;
    }
    return doc;
}

/*!
 Returns a QVariant representing the Json document.

 The returned variant will be a QVariantList if the document is
 a QJsonArray and a QVariantMap if the document is a QJsonObject.

 \sa fromVariant(), QJsonValue::toVariant()
 */
QVariant QJsonDocument::toVariant() const
{
    if (!d)
        return QVariant();

    QCborContainerPrivate *container = QJsonPrivate::Value::container(d->value);
    if (d->value.isArray())
        return QJsonArray(container).toVariantList();
    return QJsonObject(container).toVariantMap();
}
#endif // !QT_NO_VARIANT

/*!
\if !defined(qt7)
    \enum QJsonDocument::JsonFormat
    \since 5.1

    This value defines the format of the JSON byte array produced
    when converting to a QJsonDocument using toJson().

    \value Indented Defines human readable output as follows:
        \snippet code/src_corelib_serialization_qjsondocument.cpp 0

    \value Compact Defines a compact output as follows:
        \snippet code/src_corelib_serialization_qjsondocument.cpp 1
\else
    \typealias QJsonDocument::JsonFormat
    \since 5.1

    Same as \l QJsonValue::JsonFormat.
\endif
  */

/*!
    \since 5.1
    Converts the QJsonDocument to a UTF-8 encoded JSON document in the provided \a format.

    \sa fromJson(), JsonFormat
 */
QByteArray QJsonDocument::toJson(JsonFormat format) const
{
    QByteArray json;
    if (!d)
        return json;

    return QJsonPrivate::Value::fromTrustedCbor(d->value).toJson(
            format == JsonFormat::Compact ? QJsonValue::JsonFormat::Compact
                                          : QJsonValue::JsonFormat::Indented);
}

/*!
 Parses \a json as a UTF-8 encoded JSON document, and creates a QJsonDocument
 from it.

 Returns a valid (non-null) QJsonDocument if the parsing succeeds. If it fails,
 the returned document will be null, and the optional \a error variable will contain
 further details about the error.

 \sa toJson(), QJsonParseError, isNull()
 */
QJsonDocument QJsonDocument::fromJson(const QByteArray &json, QJsonParseError *error)
{
    QJsonPrivate::Parser parser(json);
    QJsonDocument result;
    const QCborValue val = parser.parse(error);
    if (val.isArray() || val.isMap()) {
        result.d = std::make_unique<QJsonDocumentPrivate>();
        result.d->value = val;
    } else if (!val.isUndefined() && error) {
        // parsed a valid string/number/bool/null,
        // but QJsonDocument only stores objects and arrays.
        error->error = QJsonParseError::IllegalValue;
        error->offset = 0;
    }
    return result;
}

/*!
    Returns \c true if the document doesn't contain any data.
 */
bool QJsonDocument::isEmpty() const
{
    if (!d)
        return true;

    return false;
}

/*!
    Returns \c true if the document contains an array.

    \sa array(), isObject()
 */
bool QJsonDocument::isArray() const
{
    if (!d)
        return false;

    return d->value.isArray();
}

/*!
    Returns \c true if the document contains an object.

    \sa object(), isArray()
 */
bool QJsonDocument::isObject() const
{
    if (!d)
        return false;

    return d->value.isMap();
}

/*!
    Returns the QJsonObject contained in the document.

    Returns an empty object if the document contains an
    array.

    \sa isObject(), array(), setObject()
 */
QJsonObject QJsonDocument::object() const
{
    if (isObject()) {
        if (auto container = QJsonPrivate::Value::container(d->value))
            return QJsonObject(container);
    }
    return QJsonObject();
}

/*!
    Returns the QJsonArray contained in the document.

    Returns an empty array if the document contains an
    object.

    \sa isArray(), object(), setArray()
 */
QJsonArray QJsonDocument::array() const
{
    if (isArray()) {
        if (auto container = QJsonPrivate::Value::container(d->value))
            return QJsonArray(container);
    }
    return QJsonArray();
}

/*!
    Sets \a object as the main object of this document.

    \sa setArray(), object()
 */
void QJsonDocument::setObject(const QJsonObject &object)
{
    if (!d)
        d = std::make_unique<QJsonDocumentPrivate>();

    d->value = QCborValue::fromJsonValue(object);
}

/*!
    Sets \a array as the main object of this document.

    \sa setObject(), array()
 */
void QJsonDocument::setArray(const QJsonArray &array)
{
    if (!d)
        d = std::make_unique<QJsonDocumentPrivate>();

    d->value = QCborValue::fromJsonValue(array);
}

/*!
    Returns a QJsonValue representing the value for the key \a key.

    Equivalent to calling object().value(key).

    The returned QJsonValue is QJsonValue::Undefined if the key does not exist,
    or if isObject() is false.

    \since 5.10

    \sa QJsonValue, QJsonValue::isUndefined(), QJsonObject
 */
const QJsonValue QJsonDocument::operator[](const QString &key) const
{
    return (*this)[QStringView(key)];
}

/*!
    \overload
    \since 5.14
*/
const QJsonValue QJsonDocument::operator[](QStringView key) const
{
    if (!isObject())
        return QJsonValue(QJsonValue::Undefined);

    return QJsonPrivate::Value::fromTrustedCbor(d->value.toMap().value(key));
}

/*!
    \overload
    \since 5.10
*/
const QJsonValue QJsonDocument::operator[](QLatin1StringView key) const
{
    if (!isObject())
        return QJsonValue(QJsonValue::Undefined);

    return QJsonPrivate::Value::fromTrustedCbor(d->value.toMap().value(key));
}

/*!
    Returns a QJsonValue representing the value for index \a i.

    Equivalent to calling array().at(i).

    The returned QJsonValue is QJsonValue::Undefined, if \a i is out of bounds,
    or if isArray() is false.

    \since 5.10

    \sa QJsonValue, QJsonValue::isUndefined(), QJsonArray
 */
const QJsonValue QJsonDocument::operator[](qsizetype i) const
{
    if (!isArray())
        return QJsonValue(QJsonValue::Undefined);

    return QJsonPrivate::Value::fromTrustedCbor(d->value.toArray().at(i));
}

/*!
    \fn bool QJsonDocument::operator==(const QJsonDocument &lhs, const QJsonDocument &rhs)

    Returns \c true if the \a lhs document is equal to \a rhs document, \c false otherwise.
*/
bool comparesEqual(const QJsonDocument &lhs, const QJsonDocument &rhs) noexcept
{
    if (lhs.d && rhs.d)
        return lhs.d->value == rhs.d->value;
    return !lhs.d == !rhs.d;
}

/*!
    \fn bool QJsonDocument::operator!=(const QJsonDocument &lhs, const QJsonDocument &rhs)

    Returns \c true if the \a lhs document is not equal to \a rhs document, \c false otherwise.
*/

/*!
    returns \c true if this document is null.

    Null documents are documents created through the default constructor.

    Documents created from UTF-8 encoded text or the binary format are
    validated during parsing. If validation fails, the returned document
    will also be null.
 */
bool QJsonDocument::isNull() const
{
    return (d == nullptr);
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, const QJsonDocument &o)
{
    QDebugStateSaver saver(dbg);
    if (!o.d) {
        dbg << "QJsonDocument()";
        return dbg;
    }
    QByteArray json =
        QJsonPrivate::Value::fromTrustedCbor(o.d->value).toJson(QJsonValue::JsonFormat::Compact);
    dbg.nospace() << "QJsonDocument("
                  << json.constData() // print as utf-8 string without extra quotation marks
                  << ')';
    return dbg;
}
#endif

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &stream, const QJsonDocument &doc)
{
    stream << doc.toJson(QJsonDocument::Compact);
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QJsonDocument &doc)
{
    QByteArray buffer;
    stream >> buffer;
    QJsonParseError parseError{};
    doc = QJsonDocument::fromJson(buffer, &parseError);
    if (parseError.error && !buffer.isEmpty())
        stream.setStatus(QDataStream::ReadCorruptData);
    return stream;
}
#endif

QT_END_NAMESPACE
