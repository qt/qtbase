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

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qstringlist.h>
#include <qvariant.h>
#include <qdebug.h>
#include <qcbormap.h>
#include <qcborarray.h>
#include "qcborvalue_p.h"
#include "qjsonwriter_p.h"
#include "qjsonparser_p.h"
#include "qjson_p.h"
#include "qdatastream.h"

#if QT_CONFIG(binaryjson)
#include "qbinaryjson_p.h"
#include "qbinaryjsonobject_p.h"
#include "qbinaryjsonarray_p.h"
#endif

#include <private/qmemory_p.h>

QT_BEGIN_NAMESPACE

/*! \class QJsonDocument
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The QJsonDocument class provides a way to read and write JSON documents.

    QJsonDocument is a class that wraps a complete JSON document and can read and
    write this document both from a UTF-8 encoded text based representation as well
    as Qt's own binary format.

    A JSON document can be converted from its text-based representation to a QJsonDocument
    using QJsonDocument::fromJson(). toJson() converts it back to text. The parser is very
    fast and efficient and converts the JSON to the binary representation used by Qt.

    Validity of the parsed document can be queried with !isNull()

    A document can be queried as to whether it contains an array or an object using isArray()
    and isObject(). The array or object contained in the document can be retrieved using
    array() or object() and then read or manipulated.

    A document can also be created from a stored binary representation using fromBinaryData() or
    fromRawData().

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/


class QJsonDocumentPrivate
{
    Q_DISABLE_COPY_MOVE(QJsonDocumentPrivate);
public:
    QJsonDocumentPrivate() = default;
    QJsonDocumentPrivate(QCborValue data) : value(std::move(data)) {}
    ~QJsonDocumentPrivate()
    {
        if (rawData)
            free(rawData);
    }

    QCborValue value;
    char *rawData = nullptr;
    uint rawDataSize = 0;

    void clearRawData()
    {
        if (rawData) {
            free(rawData);
            rawData = nullptr;
            rawDataSize = 0;
        }
    }
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
    : d(qt_make_unique<QJsonDocumentPrivate>(data))
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
            d = qt_make_unique<QJsonDocumentPrivate>();
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
                d = qt_make_unique<QJsonDocumentPrivate>();
            else
                d->clearRawData();
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

    Swaps the document \a other with this. This operation is very fast and never fails.
*/


/*! \enum QJsonDocument::DataValidation

  This value is used to tell QJsonDocument whether to validate the binary data
  when converting to a QJsonDocument using fromBinaryData() or fromRawData().

  \value Validate Validate the data before using it. This is the default.
  \value BypassValidation Bypasses data validation. Only use if you received the
  data from a trusted place and know it's valid, as using of invalid data can crash
  the application.
  */

#if QT_CONFIG(binaryjson) && QT_DEPRECATED_SINCE(5, 15)
/*!
 \deprecated

 Creates a QJsonDocument that uses the first \a size bytes from
 \a data. It assumes \a data contains a binary encoded JSON document.
 The created document does not take ownership of \a data. The data is
 copied into a different data structure, and the original data can be
 deleted or modified afterwards.

 \a data has to be aligned to a 4 byte boundary.

 \a validation decides whether the data is checked for validity before being used.
 By default the data is validated. If the \a data is not valid, the method returns
 a null document.

 Returns a QJsonDocument representing the data.

 \note Deprecated in Qt 5.15. The binary JSON encoding is only retained for backwards
 compatibility. It is undocumented and restrictive in the maximum size of JSON
 documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
 which can in turn be serialized into the CBOR binary format and vice versa. The
 CBOR format is a well-defined and less restrictive binary representation for a
 superset of JSON.

 \note Before Qt 5.15, the caller had to guarantee that \a data would not be
 deleted or modified as long as any QJsonDocument, QJsonObject or QJsonArray
 still referenced the data. From Qt 5.15 on, this is not necessary anymore.

 \sa rawData(), fromBinaryData(), isNull(), DataValidation, QCborValue
 */
QJsonDocument QJsonDocument::fromRawData(const char *data, int size, DataValidation validation)
{
    if (quintptr(data) & 3) {
        qWarning("QJsonDocument::fromRawData: data has to have 4 byte alignment");
        return QJsonDocument();
    }

    if (size < 0 || uint(size) < sizeof(QBinaryJsonPrivate::Header) + sizeof(QBinaryJsonPrivate::Base))
        return QJsonDocument();

    std::unique_ptr<QBinaryJsonPrivate::ConstData> binaryData
            = qt_make_unique<QBinaryJsonPrivate::ConstData>(data, size);

    return (validation == BypassValidation || binaryData->isValid())
            ? binaryData->toJsonDocument()
            : QJsonDocument();
}

/*!
  \deprecated

  Returns the raw binary representation of the data
  \a size will contain the size of the returned data.

  This method is useful to e.g. stream the JSON document
  in its binary form to a file.

  \note Deprecated in Qt 5.15. The binary JSON encoding is only retained for backwards
  compatibility. It is undocumented and restrictive in the maximum size of JSON
  documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
  which can in turn be serialized into the CBOR binary format and vice versa. The
  CBOR format is a well-defined and less restrictive binary representation for a
  superset of JSON.

  \sa QCborValue
 */
const char *QJsonDocument::rawData(int *size) const
{
    if (!d) {
        *size = 0;
        return nullptr;
    }

    if (!d->rawData) {
        if (isObject()) {
            QBinaryJsonObject o = QBinaryJsonObject::fromJsonObject(object());
            d->rawData = o.takeRawData(&(d->rawDataSize));
        } else {
            QBinaryJsonArray a = QBinaryJsonArray::fromJsonArray(array());
            d->rawData = a.takeRawData(&(d->rawDataSize));
        }
    }

    // It would be quite miraculous if not, as we should have hit the 128MB limit then.
    Q_ASSERT(d->rawDataSize <= uint(std::numeric_limits<int>::max()));

    *size = d->rawDataSize;
    return d->rawData;
}

/*!
 \deprecated
 Creates a QJsonDocument from \a data.

 \a validation decides whether the data is checked for validity before being used.
 By default the data is validated. If the \a data is not valid, the method returns
 a null document.

 \note Deprecated in Qt 5.15. The binary JSON encoding is only retained for backwards
 compatibility. It is undocumented and restrictive in the maximum size of JSON
 documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
 which can in turn be serialized into the CBOR binary format and vice versa. The
 CBOR format is a well-defined and less restrictive binary representation for a
 superset of JSON.

 \sa toBinaryData(), fromRawData(), isNull(), DataValidation, QCborValue
 */
QJsonDocument QJsonDocument::fromBinaryData(const QByteArray &data, DataValidation validation)
{
    if (uint(data.size()) < sizeof(QBinaryJsonPrivate::Header) + sizeof(QBinaryJsonPrivate::Base))
        return QJsonDocument();

    QBinaryJsonPrivate::Header h;
    memcpy(&h, data.constData(), sizeof(QBinaryJsonPrivate::Header));
    QBinaryJsonPrivate::Base root;
    memcpy(&root, data.constData() + sizeof(QBinaryJsonPrivate::Header),
           sizeof(QBinaryJsonPrivate::Base));

    const uint size = sizeof(QBinaryJsonPrivate::Header) + root.size;
    if (h.tag != QJsonDocument::BinaryFormatTag || h.version != 1U || size > uint(data.size()))
        return QJsonDocument();

    std::unique_ptr<QBinaryJsonPrivate::ConstData> d
            = qt_make_unique<QBinaryJsonPrivate::ConstData>(data.constData(), size);

    return (validation == BypassValidation || d->isValid())
            ? d->toJsonDocument()
            : QJsonDocument();
}

/*!
 \deprecated
 Returns a binary representation of the document.

 The binary representation is also the native format used internally in Qt,
 and is very efficient and fast to convert to and from.

 The binary format can be stored on disk and interchanged with other applications
 or computers. fromBinaryData() can be used to convert it back into a
 JSON document.

 \note Deprecated in Qt 5.15. The binary JSON encoding is only retained for backwards
 compatibility. It is undocumented and restrictive in the maximum size of JSON
 documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
 which can in turn be serialized into the CBOR binary format and vice versa. The
 CBOR format is a well-defined and less restrictive binary representation for a
 superset of JSON.

 \sa fromBinaryData(), QCborValue
 */
QByteArray QJsonDocument::toBinaryData() const
{
    int size = 0;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    const char *raw = rawData(&size);
QT_WARNING_POP
    return QByteArray(raw, size);
}
#endif // QT_CONFIG(binaryjson) && QT_DEPRECATED_SINCE(5, 15)

/*!
 Creates a QJsonDocument from the QVariant \a variant.

 If the \a variant contains any other type than a QVariantMap,
 QVariantHash, QVariantList or QStringList, the returned document is invalid.

 \sa toVariant()
 */
QJsonDocument QJsonDocument::fromVariant(const QVariant &variant)
{
    QJsonDocument doc;

    switch (variant.userType()) {
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
        doc.d = qt_make_unique<QJsonDocumentPrivate>();
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

/*!
 Converts the QJsonDocument to an indented, UTF-8 encoded JSON document.

 \sa fromJson()
 */
#if !defined(QT_JSON_READONLY) || defined(Q_CLANG_QDOC)
QByteArray QJsonDocument::toJson() const
{
    return toJson(Indented);
}
#endif

/*!
    \enum QJsonDocument::JsonFormat
    \since 5.1

    This value defines the format of the JSON byte array produced
    when converting to a QJsonDocument using toJson().

    \value Indented Defines human readable output as follows:
        \snippet code/src_corelib_serialization_qjsondocument.cpp 0

    \value Compact Defines a compact output as follows:
        \snippet code/src_corelib_serialization_qjsondocument.cpp 1
  */

/*!
    \since 5.1
    Converts the QJsonDocument to a UTF-8 encoded JSON document in the provided \a format.

    \sa fromJson(), JsonFormat
 */
#if !defined(QT_JSON_READONLY) || defined(Q_CLANG_QDOC)
QByteArray QJsonDocument::toJson(JsonFormat format) const
{
    QByteArray json;
    if (!d)
        return json;

    const QCborContainerPrivate *container = QJsonPrivate::Value::container(d->value);
    if (d->value.isArray())
        QJsonPrivate::Writer::arrayToJson(container, json, 0, (format == Compact));
    else
        QJsonPrivate::Writer::objectToJson(container, json, 0, (format == Compact));

    return json;
}
#endif

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
    QJsonPrivate::Parser parser(json.constData(), json.length());
    QJsonDocument result;
    const QCborValue val = parser.parse(error);
    if (val.isArray() || val.isMap()) {
        result.d = qt_make_unique<QJsonDocumentPrivate>();
        result.d->value = val;
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
        d = qt_make_unique<QJsonDocumentPrivate>();
    else
        d->clearRawData();

    d->value = QCborValue::fromJsonValue(object);
}

/*!
    Sets \a array as the main object of this document.

    \sa setObject(), array()
 */
void QJsonDocument::setArray(const QJsonArray &array)
{
    if (!d)
        d = qt_make_unique<QJsonDocumentPrivate>();
    else
        d->clearRawData();

    d->value = QCborValue::fromJsonValue(array);
}

#if QT_STRINGVIEW_LEVEL < 2
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
#endif

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
const QJsonValue QJsonDocument::operator[](QLatin1String key) const
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
const QJsonValue QJsonDocument::operator[](int i) const
{
    if (!isArray())
        return QJsonValue(QJsonValue::Undefined);

    return QJsonPrivate::Value::fromTrustedCbor(d->value.toArray().at(i));
}

/*!
    Returns \c true if the \a other document is equal to this document.
 */
bool QJsonDocument::operator==(const QJsonDocument &other) const
{
    if (d && other.d)
        return d->value == other.d->value;
    return !d == !other.d;
}

/*!
 \fn bool QJsonDocument::operator!=(const QJsonDocument &other) const

    returns \c true if \a other is not equal to this document
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

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
QDebug operator<<(QDebug dbg, const QJsonDocument &o)
{
    QDebugStateSaver saver(dbg);
    if (!o.d) {
        dbg << "QJsonDocument()";
        return dbg;
    }
    QByteArray json;
    const QCborContainerPrivate *container = QJsonPrivate::Value::container(o.d->value);
    if (o.d->value.isArray())
        QJsonPrivate::Writer::arrayToJson(container, json, 0, true);
    else
        QJsonPrivate::Writer::objectToJson(container, json, 0, true);
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
