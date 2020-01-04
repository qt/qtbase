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

#include "qcborstreamwriter.h"

#define CBOR_NO_PARSER_API
#include <private/qcborcommon_p.h>

#include <private/qnumeric_p.h>
#include <qbuffer.h>
#include <qdebug.h>
#include <qstack.h>

QT_BEGIN_NAMESPACE

static CborError qt_cbor_encoder_write_callback(void *token, const void *data, size_t len, CborEncoderAppendType);
#define CBOR_ENCODER_WRITER_CONTROL     1
#define CBOR_ENCODER_WRITE_FUNCTION     qt_cbor_encoder_write_callback
#define CBOR_ENCODER_NO_CHECK_USER

QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4334) // '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)

#include <cborencoder.c>

QT_WARNING_POP

// silence compilers that complain about this being a static function declared
// but never defined
static CborError Q_DECL_UNUSED cbor_encoder_close_container_checked(CborEncoder*, const CborEncoder*)
{
    Q_UNREACHABLE();
    return CborErrorInternalError;
}

static CborError Q_DECL_UNUSED cbor_encode_float_as_half_float(CborEncoder *, float)
{
    Q_UNREACHABLE();
    return CborErrorInternalError;
}

Q_DECLARE_TYPEINFO(CborEncoder, Q_PRIMITIVE_TYPE);

/*!
   \class QCborStreamWriter
   \inmodule QtCore
   \ingroup cbor
   \reentrant
   \since 5.12

   \brief The QCborStreamWriter class is a simple CBOR encoder operating on a
   one-way stream.

   This class can be used to quickly encode a stream of CBOR content directly
   to either a QByteArray or QIODevice. CBOR is the Concise Binary Object
   Representation, a very compact form of binary data encoding that is
   compatible with JSON. It was created by the IETF Constrained RESTful
   Environments (CoRE) WG, which has used it in many new RFCs. It is meant to
   be used alongside the \l{https://tools.ietf.org/html/rfc7252}{CoAP
   protocol}.

   QCborStreamWriter provides a StAX-like API, similar to that of
   \l{QXmlStreamWriter}. It is rather low-level and requires a bit of knowledge
   of CBOR encoding. For a simpler API, see \l{QCborValue} and especially the
   encoding function QCborValue::toCbor().

   The typical use of QCborStreamWriter is to create the object on the target
   QByteArray or QIODevice, then call one of the append() overloads with the
   desired type to be encoded. To create arrays and maps, QCborStreamWriter
   provides startArray() and startMap() overloads, which must be terminated by
   the corresponding endArray() and endMap() functions.

   The following example encodes the equivalent of this JSON content:

   \div{class="pre"}
     {
       "label": "journald",
       "autoDetect": false,
       "condition": "libs.journald",
       "output": [ "privateFeature" ]
     }
   \enddiv

   \snippet code/src_corelib_serialization_qcborstream.cpp 1

   \section1 CBOR support

   QCborStreamWriter supports all CBOR features required to create canonical
   and strict streams. It implements almost all of the features specified in
   \l {https://tools.ietf.org/html/rfc7049}{RFC 7049}.

   The following table lists the CBOR features that QCborStreamWriter supports.

   \table
     \header \li Feature                        \li Support
     \row   \li Unsigned numbers                \li Yes (full range)
     \row   \li Negative numbers                \li Yes (full range)
     \row   \li Byte strings                    \li Yes
     \row   \li Text strings                    \li Yes
     \row   \li Chunked strings                 \li No
     \row   \li Tags                            \li Yes (arbitrary)
     \row   \li Booleans                        \li Yes
     \row   \li Null                            \li Yes
     \row   \li Undefined                       \li Yes
     \row   \li Arbitrary simple values         \li Yes
     \row   \li Half-precision float (16-bit)   \li Yes
     \row   \li Single-precision float (32-bit) \li Yes
     \row   \li Double-precision float (64-bit) \li Yes
     \row   \li Infinities and NaN floating point \li Yes
     \row   \li Determinate-length arrays and maps \li Yes
     \row   \li Indeterminate-length arrays and maps \li Yes
     \row   \li Map key types other than strings and integers \li Yes (arbitrary)
   \endtable

   \section2 Canonical CBOR encoding

   Canonical CBOR encoding is defined by
   \l{https://tools.ietf.org/html/rfc7049#section-3.9}{Section 3.9 of RFC
   7049}. Canonical encoding is not a requirement for Qt's CBOR decoding
   functionality, but it may be required for some protocols. In particular,
   protocols that require the ability to reproduce the same stream identically
   may require this.

   In order to be considered "canonical", a CBOR stream must meet the
   following requirements:

   \list
     \li Integers must be as small as possible. QCborStreamWriter always
         does this (no user action is required and it is not possible
         to write overlong integers).
     \li Array, map and string lengths must be as short as possible. As
         above, QCborStreamWriter automatically does this.
     \li Arrays, maps and strings must use explicit length. QCborStreamWriter
         always does this for strings; for arrays and maps, be sure to call
         startArray() and startMap() overloads with explicit length.
     \li Keys in every map must be sorted in ascending order. QCborStreamWriter
         offers no help in this item: the developer must ensure that before
         calling append() for the map pairs.
     \li Floating point values should be as small as possible. QCborStreamWriter
         will not convert floating point values; it is up to the developer
         to perform this check prior to calling append() (see those functions'
         examples).
   \endlist

   \section2 Strict CBOR mode

   Strict mode is defined by
   \l{https://tools.ietf.org/html/rfc7049#section-3.10}{Section 3.10 of RFC
   7049}. As for Canonical encoding above, QCborStreamWriter makes it possible
   to create strict CBOR streams, but does not require them or validate that
   the output is so.

   \list
     \li Keys in a map must be unique. QCborStreamWriter performs no validation
         of map keys.
     \li Tags may be required to be paired only with the correct types,
         according to their specification. QCborStreamWriter performs no
         validation of tag usage.
     \li Text Strings must be properly-encoded UTF-8. QCborStreamWriter always
         writes proper UTF-8 for strings added with append(), but performs no
         validation for strings added with appendTextString().
   \endlist

   \section2 Invalid CBOR stream

   It is also possible to misuse QCborStreamWriter and produce invalid CBOR
   streams that will fail to be decoded by a receiver. The following actions
   will produce invalid streams:

   \list
     \li Append a tag and not append the corresponding tagged value
         (QCborStreamWriter produces no diagnostic).
     \li Append too many or too few items to an array or map with explicit
         length (endMap() and endArray() will return false and
         QCborStreamWriter will log with qWarning()).
   \endlist

   \sa QCborStreamReader, QCborValue, QXmlStreamWriter
 */

class QCborStreamWriterPrivate
{
public:
    static Q_CONSTEXPR quint64 IndefiniteLength = (std::numeric_limits<quint64>::max)();

    QIODevice *device;
    CborEncoder encoder;
    QStack<CborEncoder> containerStack;
    bool deleteDevice = false;

    QCborStreamWriterPrivate(QIODevice *device)
        : device(device)
    {
        cbor_encoder_init_writer(&encoder, qt_cbor_encoder_write_callback, this);
    }

    ~QCborStreamWriterPrivate()
    {
        if (deleteDevice)
            delete device;
    }

    template <typename... Args> void executeAppend(CborError (*f)(CborEncoder *, Args...), Args... args)
    {
        f(&encoder, std::forward<Args>(args)...);
    }

    void createContainer(CborError (*f)(CborEncoder *, CborEncoder *, size_t), quint64 len = IndefiniteLength)
    {
        Q_STATIC_ASSERT(size_t(IndefiniteLength) == CborIndefiniteLength);
        if (sizeof(len) != sizeof(size_t) && len != IndefiniteLength) {
            if (Q_UNLIKELY(len >= CborIndefiniteLength)) {
                // TinyCBOR can't do this in 32-bit mode
                qWarning("QCborStreamWriter: container of size %llu is too big for a 32-bit build; "
                         "will use indeterminate length instead", len);
                len = CborIndefiniteLength;
            }
        }

        containerStack.push(encoder);
        f(&containerStack.top(), &encoder, len);
    }

    bool closeContainer()
    {
        if (containerStack.isEmpty()) {
            qWarning("QCborStreamWriter: closing map or array that wasn't open");
            return false;
        }

        CborEncoder container = containerStack.pop();
        CborError err = cbor_encoder_close_container(&container, &encoder);
        encoder = container;

        if (Q_UNLIKELY(err)) {
            if (err == CborErrorTooFewItems)
                qWarning("QCborStreamWriter: not enough items added to array or map");
            else if (err == CborErrorTooManyItems)
                qWarning("QCborStreamWriter: too many items added to array or map");
            return false;
        }

        return true;
    }
};

static CborError qt_cbor_encoder_write_callback(void *self, const void *data, size_t len, CborEncoderAppendType)
{
    auto that = static_cast<QCborStreamWriterPrivate *>(self);
    if (!that->device)
        return CborNoError;
    qint64 written = that->device->write(static_cast<const char *>(data), len);
    return (written == qsizetype(len) ? CborNoError : CborErrorIO);
}

/*!
   Creates a QCborStreamWriter object that will write the stream to \a device.
   The device must be opened before the first append() call is made. This
   constructor can be used with any class that derives from QIODevice, such as
   QFile, QProcess or QTcpSocket.

   QCborStreamWriter has no buffering, so every append() call will result in
   one or more calls to the device's \l {QIODevice::}{write()} method.

   The following example writes an empty map to a file:

   \snippet code/src_corelib_serialization_qcborstream.cpp 2

   QCborStreamWriter does not take ownership of \a device.

   \sa device(), setDevice()
 */
QCborStreamWriter::QCborStreamWriter(QIODevice *device)
    : d(new QCborStreamWriterPrivate(device))
{
}

/*!
   Creates a QCborStreamWriter object that will append the stream to \a data.
   All streaming is done immediately to the byte array, without the need for
   flushing any buffers.

   The following example writes a number to a byte array then returns
   it.

   \snippet code/src_corelib_serialization_qcborstream.cpp 3

   QCborStreamWriter does not take ownership of \a data.
 */
QCborStreamWriter::QCborStreamWriter(QByteArray *data)
    : d(new QCborStreamWriterPrivate(new QBuffer(data)))
{
    d->deleteDevice = true;
    d->device->open(QIODevice::WriteOnly | QIODevice::Unbuffered);
}

/*!
   Destroys this QCborStreamWriter object and frees any resources associated.

   QCborStreamWriter does not perform error checking to see if all required
   items were written to the stream prior to the object being destroyed. It is
   the programmer's responsibility to ensure that it was done.
 */
QCborStreamWriter::~QCborStreamWriter()
{
}

/*!
   Replaces the device or byte array that this QCborStreamWriter object is
   writing to with \a device.

   \sa device()
 */
void QCborStreamWriter::setDevice(QIODevice *device)
{
    if (d->deleteDevice)
        delete d->device;
    d->device = device;
    d->deleteDevice = false;
}

/*!
   Returns the QIODevice that this QCborStreamWriter object is writing to. The
   device must have previously been set with either the constructor or with
   setDevice().

   If this object was created by writing to a QByteArray, this function will
   return an internal instance of QBuffer, which is owned by QCborStreamWriter.

   \sa setDevice()
 */
QIODevice *QCborStreamWriter::device() const
{
    return d->device;
}

/*!
   \overload

   Appends the 64-bit unsigned value \a u to the CBOR stream, creating a CBOR
   Unsigned Integer value. In the following example, we write the values 0,
   2\sup{32} and \c UINT64_MAX:

   \snippet code/src_corelib_serialization_qcborstream.cpp 4

   \sa QCborStreamReader::isUnsignedInteger(), QCborStreamReader::toUnsignedInteger()
 */
void QCborStreamWriter::append(quint64 u)
{
    d->executeAppend(cbor_encode_uint, uint64_t(u));
}

/*!
   \overload

   Appends the 64-bit signed value \a i to the CBOR stream. This will create
   either a CBOR Unsigned Integer or CBOR NegativeInteger value based on the
   sign of the parameter. In the following example, we write the values 0, -1,
   2\sup{32} and \c INT64_MAX:

   \snippet code/src_corelib_serialization_qcborstream.cpp 5

   \sa QCborStreamReader::isInteger(), QCborStreamReader::toInteger()
 */
void QCborStreamWriter::append(qint64 i)
{
    d->executeAppend(cbor_encode_int, int64_t(i));
}

/*!
   \overload

   Appends the 64-bit negative value \a n to the CBOR stream.
   QCborNegativeInteger is a 64-bit enum that holds the absolute value of the
   negative number we want to write. If n is zero, the value written will be
   equivalent to 2\sup{64} (that is, -18,446,744,073,709,551,616).

   In the following example, we write the values -1, -2\sup{32} and INT64_MIN:
   \snippet code/src_corelib_serialization_qcborstream.cpp 6

   Note how this function can be used to encode numbers that cannot fit a
   standard computer's 64-bit signed integer like \l qint64. That is, if \a n
   is larger than \c{std::numeric_limits<qint64>::max()} or is 0, this will
   represent a negative number smaller than
   \c{std::numeric_limits<qint64>::min()}.

   \sa QCborStreamReader::isNegativeInteger(), QCborStreamReader::toNegativeInteger()
 */
void QCborStreamWriter::append(QCborNegativeInteger n)
{
    d->executeAppend(cbor_encode_negative_int, uint64_t(n));
}

/*!
   \fn void QCborStreamWriter::append(const QByteArray &ba)
   \overload

   Appends the byte array \a ba to the stream, creating a CBOR Byte String
   value. QCborStreamWriter will attempt to write the entire string in one
   chunk.

   The following example will load and append the contents of a file to the
   stream:

   \snippet code/src_corelib_serialization_qcborstream.cpp 7

   As the example shows, unlike JSON, CBOR requires no escaping for binary
   content.

   \sa appendByteString(), QCborStreamReader::isByteArray(),
       QCborStreamReader::readByteArray()
 */

/*!
   \overload

   Appends the text string \a str to the stream, creating a CBOR Text String
   value. QCborStreamWriter will attempt to write the entire string in one
   chunk.

   The following example appends a simple string to the stream:

   \snippet code/src_corelib_serialization_qcborstream.cpp 8

   \b{Performance note}: CBOR requires that all Text Strings be encoded in
   UTF-8, so this function will iterate over the characters in the string to
   determine whether the contents are US-ASCII or not. If the string is found
   to contain characters outside of US-ASCII, it will allocate memory and
   convert to UTF-8. If this check is unnecessary, use appendTextString()
   instead.

   \sa QCborStreamReader::isString(), QCborStreamReader::readString()
 */
void QCborStreamWriter::append(QLatin1String str)
{
    // We've got Latin-1 but CBOR wants UTF-8, so check if the string is the
    // common subset (US-ASCII).
    if (QtPrivate::isAscii(str)) {
        // it is plain US-ASCII
        appendTextString(str.latin1(), str.size());
    } else {
        // non-ASCII, so we need a pass-through UTF-16
        append(QString(str));
    }
}

/*!
   \overload

   Appends the text string \a str to the stream, creating a CBOR Text String
   value. QCborStreamWriter will attempt to write the entire string in one
   chunk.

   The following example writes an arbitrary QString to the stream:

   \snippet code/src_corelib_serialization_qcborstream.cpp 9

   \sa QCborStreamReader::isString(), QCborStreamReader::readString()
 */
void QCborStreamWriter::append(QStringView str)
{
    QByteArray utf8 = str.toUtf8();
    appendTextString(utf8.constData(), utf8.size());
}

/*!
   \overload

   Appends the CBOR tag \a tag to the stream, creating a CBOR Tag value. All
   tags must be followed by another type which they provide meaning for.

   In the following example, we append a CBOR Tag 36 (Regular Expression) and a
   QRegularExpression's pattern to the stream:

   \snippet code/src_corelib_serialization_qcborstream.cpp 10

   \sa QCborStreamReader::isTag(), QCborStreamReader::toTag()
 */
void QCborStreamWriter::append(QCborTag tag)
{
    d->executeAppend(cbor_encode_tag, CborTag(tag));
}

/*!
   \fn void QCborStreamWriter::append(QCborKnownTags tag)
   \overload

   Appends the CBOR tag \a tag to the stream, creating a CBOR Tag value. All
   tags must be followed by another type which they provide meaning for.

   In the following example, we append a CBOR Tag 1 (Unix \c time_t) and an
   integer representing the current time to the stream, obtained using the \c
   time() function:

   \snippet code/src_corelib_serialization_qcborstream.cpp 11

   \sa QCborStreamReader::isTag(), QCborStreamReader::toTag()
 */

/*!
   \overload

   Appends the CBOR simple type \a st to the stream, creating a CBOR Simple
   Type value. In the following example, we write the simple type for Null as
   well as for type 32, which Qt has no support for.

   \snippet code/src_corelib_serialization_qcborstream.cpp 12

   \note Using Simple Types for which there is no specification can lead to
   validation errors by the remote receiver. In addition, simple type values 24
   through 31 (inclusive) are reserved and must not be used.

   \sa QCborStreamReader::isSimpleType(), QCborStreamReader::toSimpleType()
 */
void QCborStreamWriter::append(QCborSimpleType st)
{
    d->executeAppend(cbor_encode_simple_value, uint8_t(st));
}

#ifndef QT_BOOTSTRAPPED
/*!
   \overload

   Appends the floating point number \a f to the stream, creating a CBOR 16-bit
   Half-Precision Floating Point value. The following code can be used to convert
   a C++ \tt float to \c qfloat16 if there's no loss of precision and append it, or
   instead append the \tt float.

   \snippet code/src_corelib_serialization_qcborstream.cpp 13

   \sa QCborStreamReader::isFloat16(), QCborStreamReader::toFloat16()
 */
void QCborStreamWriter::append(qfloat16 f)
{
    d->executeAppend(cbor_encode_half_float, static_cast<const void *>(&f));
}
#endif // QT_BOOTSTRAPPED

/*!
   \overload

   Appends the floating point number \a f to the stream, creating a CBOR 32-bit
   Single-Precision Floating Point value. The following code can be used to convert
   a C++ \tt double to \tt float if there's no loss of precision and append it, or
   instead append the \tt double.

   \snippet code/src_corelib_serialization_qcborstream.cpp 14

   \sa QCborStreamReader::isFloat(), QCborStreamReader::toFloat()
 */
void QCborStreamWriter::append(float f)
{
    d->executeAppend(cbor_encode_float, f);
}

/*!
   \overload

   Appends the floating point number \a d to the stream, creating a CBOR 64-bit
   Double-Precision Floating Point value. QCborStreamWriter always appends the
   number as-is, performing no check for whether the number is the canonical
   form for NaN, an infinite, whether it is denormal or if it could be written
   with a shorter format.

   The following code performs all those checks, except for the denormal one,
   which is expected to be taken into account by the system FPU or floating
   point emulation directly.

   \snippet code/src_corelib_serialization_qcborstream.cpp 15

   Determining if a double can be converted to an integral with no loss of
   precision is left as an exercise to the reader.

   \sa QCborStreamReader::isDouble(), QCborStreamReader::toDouble()
 */
void QCborStreamWriter::append(double d)
{
    this->d->executeAppend(cbor_encode_double, d);
}

/*!
   Appends \a len bytes of data starting from \a data to the stream, creating a
   CBOR Byte String value. QCborStreamWriter will attempt to write the entire
   string in one chunk.

   Unlike the QByteArray overload of append(), this function is not limited by
   QByteArray's size limits. However, note that neither
   QCborStreamReader::readByteArray() nor QCborValue support reading CBOR
   streams with byte arrays larger than 2 GB.

   \sa append(), appendTextString(),
       QCborStreamReader::isByteArray(), QCborStreamReader::readByteArray()
 */
void QCborStreamWriter::appendByteString(const char *data, qsizetype len)
{
    d->executeAppend(cbor_encode_byte_string, reinterpret_cast<const uint8_t *>(data), size_t(len));
}

/*!
   Appends \a len bytes of text starting from \a utf8 to the stream, creating a
   CBOR Text String value. QCborStreamWriter will attempt to write the entire
   string in one chunk.

   The string pointed to by \a utf8 is expected to be properly encoded UTF-8.
   QCborStreamWriter performs no validation that this is the case.

   Unlike the QLatin1String overload of append(), this function is not limited
   to 2 GB. However, note that neither QCborStreamReader::readString() nor
   QCborValue support reading CBOR streams with text strings larger than 2 GB.

   \sa append(QLatin1String), append(QStringView),
       QCborStreamReader::isString(), QCborStreamReader::readString()
 */
void QCborStreamWriter::appendTextString(const char *utf8, qsizetype len)
{
    d->executeAppend(cbor_encode_text_string, utf8, size_t(len));
}

/*!
   \fn void QCborStreamWriter::append(const char *str, qsizetype size)
   \overload

   Appends \a size bytes of text starting from \a str to the stream, creating a
   CBOR Text String value. QCborStreamWriter will attempt to write the entire
   string in one chunk. If \a size is -1, this function will write \c strlen(\a
   str) bytes.

   The string pointed to by \a str is expected to be properly encoded UTF-8.
   QCborStreamWriter performs no validation that this is the case.

   Unlike the QLatin1String overload of append(), this function is not limited
   to 2 GB. However, note that neither QCborStreamReader nor QCborValue support
   reading CBOR streams with text strings larger than 2 GB.

   \sa append(QLatin1String), append(QStringView),
       QCborStreamReader::isString(), QCborStreamReader::readString()
 */

/*!
   \fn void QCborStreamWriter::append(bool b)
   \overload

   Appends the boolean value \a b to the stream, creating either a CBOR False
   value or a CBOR True value. This function is equivalent to (and implemented
   as):

   \snippet code/src_corelib_serialization_qcborstream.cpp 16

   \sa appendNull(), appendUndefined(),
       QCborStreamReader::isBool(), QCborStreamReader::toBool()
 */

/*!
   \fn void QCborStreamWriter::append(std::nullptr_t)
   \overload

   Appends a CBOR Null value to the stream. This function is equivalent to (and
   implemented as): The parameter is ignored.

   \snippet code/src_corelib_serialization_qcborstream.cpp 17

   \sa appendNull(), append(QCborSimpleType), QCborStreamReader::isNull()
 */

/*!
   \fn void QCborStreamWriter::appendNull()

   Appends a CBOR Null value to the stream. This function is equivalent to (and
   implemented as):

   \snippet code/src_corelib_serialization_qcborstream.cpp 18

   \sa append(std::nullptr_t), append(QCborSimpleType), QCborStreamReader::isNull()
 */

/*!
   \fn void QCborStreamWriter::appendUndefined()

   Appends a CBOR Undefined value to the stream. This function is equivalent to (and
   implemented as):

   \snippet code/src_corelib_serialization_qcborstream.cpp 19

   \sa append(QCborSimpleType), QCborStreamReader::isUndefined()
 */

/*!
   Starts a CBOR Array with indeterminate length in the CBOR stream. Each
   startArray() call must be paired with one endArray() call and the current
   CBOR element extends until the end of the array.

   The array created by this function has no explicit length. Instead, its
   length is implied by the elements contained in it. Note, however, that use
   of indeterminate-length arrays is not compliant with canonical CBOR encoding.

   The following example appends elements from the linked list of strings
   passed as input:

   \snippet code/src_corelib_serialization_qcborstream.cpp 20

   \sa startArray(quint64), endArray(), startMap(), QCborStreamReader::isArray(),
   QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startArray()
{
    d->createContainer(cbor_encoder_create_array);
}

/*!
   \overload

   Starts a CBOR Array with explicit length of \a count items in the CBOR
   stream. Each startArray call must be paired with one endArray() call and the
   current CBOR element extends until the end of the array.

   The array created by this function has an explicit length and therefore
   exactly \a count items must be added to the CBOR stream. Adding fewer or
   more items will result in failure during endArray() and the CBOR stream will
   be corrupt. However, explicit-length arrays are required by canonical CBOR
   encoding.

   The following example appends all strings found in the \l QStringList passed as input:

   \snippet code/src_corelib_serialization_qcborstream.cpp 21

   \b{Size limitations}: The parameter to this function is quint64, which would
   seem to allow up to 2\sup{64}-1 elements in the array. However, both
   QCborStreamWriter and QCborStreamReader are currently limited to 2\sup{32}-2
   items on 32-bit systems and 2\sup{64}-2 items on 64-bit ones. Also note that
   QCborArray is currently limited to 2\sup{27} elements in any platform.

   \sa startArray(), endArray(), startMap(), QCborStreamReader::isArray(),
   QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startArray(quint64 count)
{
    d->createContainer(cbor_encoder_create_array, count);
}

/*!
   Terminates the array started by either overload of startArray() and returns
   true if the correct number of elements was added to the array. This function
   must be called for every startArray() used.

   A return of false indicates error in the application and an unrecoverable
   error in this stream. QCborStreamWriter also writes a warning using
   qWarning() if that happens.

   Calling this function when the current container is not an array is also an
   error, though QCborStreamWriter cannot currently detect this condition.

   \sa startArray(), startArray(quint64), endMap()
 */
bool QCborStreamWriter::endArray()
{
    return d->closeContainer();
}

/*!
   Starts a CBOR Map with indeterminate length in the CBOR stream. Each
   startMap() call must be paired with one endMap() call and the current CBOR
   element extends until the end of the map.

   The map created by this function has no explicit length. Instead, its length
   is implied by the elements contained in it. Note, however, that use of
   indeterminate-length maps is not compliant with canonical CBOR encoding
   (canonical encoding also requires keys to be unique and in sorted order).

   The following example appends elements from the linked list of int and
   string pairs passed as input:

   \snippet code/src_corelib_serialization_qcborstream.cpp 22

   \sa startMap(quint64), endMap(), startArray(), QCborStreamReader::isMap(),
       QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startMap()
{
    d->createContainer(cbor_encoder_create_map);
}

/*!
   \overload

   Starts a CBOR Map with explicit length of \a count items in the CBOR
   stream. Each startMap call must be paired with one endMap() call and the
   current CBOR element extends until the end of the map.

   The map created by this function has an explicit length and therefore
   exactly \a count pairs of items must be added to the CBOR stream. Adding
   fewer or more items will result in failure during endMap() and the CBOR
   stream will be corrupt. However, explicit-length map are required by
   canonical CBOR encoding.

   The following example appends all strings found in the \l QMap passed as input:

   \snippet code/src_corelib_serialization_qcborstream.cpp 23

   \b{Size limitations}: The parameter to this function is quint64, which would
   seem to allow up to 2\sup{64}-1 pairs in the map. However, both
   QCborStreamWriter and QCborStreamReader are currently limited to 2\sup{31}-1
   items on 32-bit systems and 2\sup{63}-1 items on 64-bit ones. Also note that
   QCborMap is currently limited to 2\sup{26} elements in any platform.

   \sa startMap(), endMap(), startArray(), QCborStreamReader::isMap(),
       QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startMap(quint64 count)
{
    d->createContainer(cbor_encoder_create_map, count);
}

/*!
   Terminates the map started by either overload of startMap() and returns
   true if the correct number of elements was added to the array. This function
   must be called for every startMap() used.

   A return of false indicates error in the application and an unrecoverable
   error in this stream. QCborStreamWriter also writes a warning using
   qWarning() if that happens.

   Calling this function when the current container is not a map is also an
   error, though QCborStreamWriter cannot currently detect this condition.

   \sa startMap(), startMap(quint64), endArray()
 */
bool QCborStreamWriter::endMap()
{
    return d->closeContainer();
}

QT_END_NAMESPACE
