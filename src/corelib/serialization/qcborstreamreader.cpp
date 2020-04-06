/****************************************************************************
**
** Copyright (C) 2020 Intel Corporation.
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

#include "qcborstreamreader.h"

#define CBOR_NO_ENCODER_API
#include <private/qcborcommon_p.h>

#include <private/qbytearray_p.h>
#include <private/qnumeric_p.h>
#include <private/qutfcodec_p.h>
#include <qdebug.h>
#include <qstack.h>

QT_BEGIN_NAMESPACE

static bool qt_cbor_decoder_can_read(void *token, size_t len);
static void qt_cbor_decoder_advance(void *token, size_t len);
static void *qt_cbor_decoder_read(void *token, void *userptr, size_t offset, size_t len);
static CborError qt_cbor_decoder_transfer_string(void *token, const void **userptr, size_t offset, size_t len);

#define CBOR_PARSER_READER_CONTROL              1
#define CBOR_PARSER_CAN_READ_BYTES_FUNCTION     qt_cbor_decoder_can_read
#define CBOR_PARSER_ADVANCE_BYTES_FUNCTION      qt_cbor_decoder_advance
#define CBOR_PARSER_TRANSFER_STRING_FUNCTION    qt_cbor_decoder_transfer_string
#define CBOR_PARSER_READ_BYTES_FUNCTION         qt_cbor_decoder_read

QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4334) // '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)

#include <cborparser.c>

QT_WARNING_POP

static CborError _cbor_value_dup_string(const CborValue *, void **, size_t *, CborValue *)
{
    Q_UNREACHABLE();
    return CborErrorInternalError;
}
static CborError Q_DECL_UNUSED cbor_value_get_half_float_as_float(const CborValue *, float *)
{
    Q_UNREACHABLE();
    return CborErrorInternalError;
}

// confirm our constants match TinyCBOR's
Q_STATIC_ASSERT(int(QCborStreamReader::UnsignedInteger) == CborIntegerType);
Q_STATIC_ASSERT(int(QCborStreamReader::ByteString) == CborByteStringType);
Q_STATIC_ASSERT(int(QCborStreamReader::TextString) == CborTextStringType);
Q_STATIC_ASSERT(int(QCborStreamReader::Array) == CborArrayType);
Q_STATIC_ASSERT(int(QCborStreamReader::Map) == CborMapType);
Q_STATIC_ASSERT(int(QCborStreamReader::Tag) == CborTagType);
Q_STATIC_ASSERT(int(QCborStreamReader::SimpleType) == CborSimpleType);
Q_STATIC_ASSERT(int(QCborStreamReader::HalfFloat) == CborHalfFloatType);
Q_STATIC_ASSERT(int(QCborStreamReader::Float) == CborFloatType);
Q_STATIC_ASSERT(int(QCborStreamReader::Double) == CborDoubleType);
Q_STATIC_ASSERT(int(QCborStreamReader::Invalid) == CborInvalidType);

/*!
   \class QCborStreamReader
   \inmodule QtCore
   \ingroup cbor
   \reentrant
   \since 5.12

   \brief The QCborStreamReader class is a simple CBOR stream decoder, operating
   on either a QByteArray or QIODevice.

   This class can be used to decode a stream of CBOR content directly from
   either a QByteArray or a QIODevice. CBOR is the Concise Binary Object
   Representation, a very compact form of binary data encoding that is
   compatible with JSON. It was created by the IETF Constrained RESTful
   Environments (CoRE) WG, which has used it in many new RFCs. It is meant to
   be used alongside the \l{https://tools.ietf.org/html/rfc7252}{CoAP
   protocol}.

   QCborStreamReader provides a StAX-like API, similar to that of
   \l{QXmlStreamReader}. Using it requires a bit of knowledge of CBOR encoding.
   For a simpler API, see \l{QCborValue} and especially the decoding function
   QCborValue::fromCbor().

   Typically, one creates a QCborStreamReader by passing the source QByteArray
   or QIODevice as a parameter to the constructor, then pop elements off the
   stream if there were no errors in decoding. There are three kinds of CBOR
   types:

   \table
     \header \li Kind       \li Types       \li Behavior
     \row   \li Fixed-width \li Integers, Tags, Simple types, Floating point
            \li Value is pre-parsed by QCborStreamReader, so accessor functions
                are \c const. Must call next() to advance.
     \row   \li Strings     \li Byte arrays, Text strings
            \li Length (if known) is pre-parsed, but the string itself is not.
                The accessor functions are not const and may allocate memory.
                Once called, the accessor functions automatically advance to
                the next element.
     \row   \li Containers  \li Arrays, Maps
            \li Length (if known) is pre-parsed. To access the elements, you
                must call enterContainer(), read all elements, then call
                leaveContainer(). That function advances to the next element.
   \endtable

   So a processor function typically looks like this:

   \snippet code/src_corelib_serialization_qcborstream.cpp 24

   \section1 CBOR support

   The following table lists the CBOR features that QCborStreamReader supports.

   \table
     \header \li Feature                        \li Support
     \row   \li Unsigned numbers                \li Yes (full range)
     \row   \li Negative numbers                \li Yes (full range)
     \row   \li Byte strings                    \li Yes
     \row   \li Text strings                    \li Yes
     \row   \li Chunked strings                 \li Yes
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

   \section1 Dealing with invalid or incomplete CBOR streams

   QCborStreamReader is capable of detecting corrupt input on its own. The
   library it uses has been extensively tested against invalid input of any
   kind and is quite able to report errors. If any is detected,
   QCborStreamReader will set lastError() to a value besides
   QCborError::NoError, indicating which situation was detected.

   Most errors detected by QCborStreamReader during normal item parsing are not
   recoverable. The code using QCborStreamReader may opt to handle the data
   that was properly decoded or it can opt to discard the entire data.

   The only recoverable error is QCborError::EndOfFile, which indicates that
   more data is required in order to complete the parsing. This situation is
   useful when data is being read from an asynchronous source, such as a pipe
   (QProcess) or a socket (QTcpSocket, QUdpSocket, QNetworkReply, etc.). When
   more data arrives, the surrounding code needs to call either addData(), if
   parsing from a QByteArray, or reparse(), if it is instead reading directly
   a the QIDOevice that now has more data available (see setDevice()).

   \sa QCborStreamWriter, QCborValue, QXmlStreamReader
 */

/*!
   \enum QCborStreamReader::Type

   This enumeration contains all possible CBOR types as decoded by
   QCborStreamReader. CBOR has 7 major types, plus a number of simple types
   carrying no value, and floating point values.

   \value UnsignedInteger       (Major type 0) Ranges from 0 to 2\sup{64} - 1
                                (18,446,744,073,709,551,616)
   \value NegativeInteger       (Major type 1) Ranges from -1 to -2\sup{64}
                                (-18,446,744,073,709,551,616)
   \value ByteArray             (Major type 2) Arbitrary binary data.
   \value ByteString            An alias to ByteArray.
   \value String                (Major type 3) Unicode text, possibly containing NULs.
   \value TextString            An alias to String
   \value Array                 (Major type 4) Array of heterogeneous items.
   \value Map                   (Major type 5) Map/dictionary of heterogeneous items.
   \value Tag                   (Major type 6) Numbers giving further semantic value
                                to generic CBOR items. See \l QCborTag for more information.
   \value SimpleType            (Major type 7) Types carrying no further value. Includes
                                booleans (true and false), null, undefined.
   \value Float16               IEEE 754 half-precision floating point (\c qfloat16).
   \value HalfFloat             An alias to Float16.
   \value Float                 IEEE 754 single-precision floating point (\tt float).
   \value Double                IEEE 754 double-precision floating point (\tt double).
   \value Invalid               Not a valid type, either due to parsing error or due to
                                reaching the end of an array or map.
 */

/*!
   \enum QCborStreamReader::StringResultCode

   This enum is returned by readString() and readByteArray() and is used to
   indicate what the status of the parsing is.

   \value EndOfString           The parsing for the string is complete, with no error.
   \value Ok                    The function returned data; there was no error.
   \value Error                 Parsing failed with an error.
 */

/*!
   \class QCborStreamReader::StringResult
   \inmodule QtCore

   This class is returned by readString() and readByteArray(), with either the
   contents of the string that was read or an indication that the parsing is
   done or found an error.

   The contents of \l data are valid only if \l status is
   \l{StringResultCode}{Ok}. Otherwise, it should be null.
 */

/*!
   \variable QCborStreamReader::StringResult::data

   Contains the actual data from the string if \l status is \c Ok.
 */

/*!
   \variable QCborStreamReader::StringResult::status

   Contains the status of the attempt of reading the string from the stream.
 */

/*!
   \fn QCborStreamReader::Type QCborStreamReader::type() const

   Returns the type of the current element. It is one of the valid types or
   Invalid.

   \sa isValid(), isUnsignedInteger(), isNegativeInteger(), isInteger(),
       isByteArray(), isString(), isArray(), isMap(), isTag(), isSimpleType(),
       isBool(), isFalse(), isTrue(), isNull(), isUndefined(), isFloat16(),
       isFloat(), isDouble()
 */

/*!
   \fn bool QCborStreamReader::isValid() const

   Returns true if the current element is valid, false otherwise. The current
   element may be invalid if there was a decoding error or we've just parsed
   the last element in an array or map.

   \note This function is not the opposite of isNull(). Null is a normal CBOR
   type that must be handled by the application.

   \sa type(), isInvalid()
 */

/*!
    \fn bool QCborStreamReader::isInvalid() const

    Returns true if the current element is invalid, false otherwise. The current
   element may be invalid if there was a decoding error or we've just parsed
   the last element in an array or map.

   \note This function is not to be confused with isNull(). Null is a normal
   CBOR type that must be handled by the application.

   \sa type(), isValid()
 */

/*!
   \fn bool QCborStreamReader::isUnsignedInteger() const

   Returns true if the type of the current element is an unsigned integer (that
   is if type() returns QCborStreamReader::UnsignedInteger). If this function
   returns true, you may call toUnsignedInteger() or toInteger() to read that value.

   \sa type(), toUnsignedInteger(), toInteger(), isInteger(), isNegativeInteger()
 */

/*!
   \fn bool QCborStreamReader::isNegativeInteger() const

   Returns true if the type of the current element is a negative integer (that
   is if type() returns QCborStreamReader::NegativeInteger). If this function
   returns true, you may call toNegativeInteger() or toInteger() to read that value.

   \sa type(), toNegativeInteger(), toInteger(), isInteger(), isUnsignedInteger()
 */

/*!
   \fn bool QCborStreamReader::isInteger() const

   Returns true if the type of the current element is either an unsigned
   integer or a negative one (that is, if type() returns
   QCborStreamReader::UnsignedInteger or QCborStreamReader::NegativeInteger).
   If this function returns true, you may call toInteger() to read that
   value.

   \sa type(), toInteger(), toUnsignedInteger(), toNegativeInteger(),
   isUnsignedInteger(), isNegativeInteger()
 */

/*!
   \fn bool QCborStreamReader::isByteArray() const

   Returns true if the type of the current element is a byte array (that is,
   if type() returns QCborStreamReader::ByteArray). If this function returns
   true, you may call readByteArray() to read that data.

   \sa type(), readByteArray(), isString()
 */

/*!
   \fn bool QCborStreamReader::isString() const

   Returns true if the type of the current element is a text string (that is,
   if type() returns QCborStreamReader::String). If this function returns
   true, you may call readString() to read that data.

   \sa type(), readString(), isByteArray()
 */

/*!
   \fn bool QCborStreamReader::isArray() const

   Returns true if the type of the current element is an array (that is,
   if type() returns QCborStreamReader::Array). If this function returns
   true, you may call enterContainer() to begin parsing that container.

   When the current element is an array, you may also call isLengthKnown() to
   find out if the array's size is explicit in the CBOR stream. If it is, that
   size can be obtained by calling length().

   The following example pre-allocates a QVariantList given the array's size
   for more efficient decoding:

   \snippet code/src_corelib_serialization_qcborstream.cpp 25

   \note The code above does not validate that the length is a sensible value.
   If the input stream reports that the length is 1 billion elements, the above
   function will try to allocate some 16 GB or more of RAM, which can lead to a
   crash.

   \sa type(), isMap(), isLengthKnown(), length(), enterContainer(), leaveContainer()
 */

/*!
   \fn bool QCborStreamReader::isMap() const

   Returns true if the type of the current element is a map (that is, if type()
   returns QCborStreamReader::Map). If this function returns true, you may call
   enterContainer() to begin parsing that container.

   When the current element is a map, you may also call isLengthKnown() to
   find out if the map's size is explicit in the CBOR stream. If it is, that
   size can be obtained by calling length().

   The following example pre-allocates a QVariantMap given the map's size
   for more efficient decoding:

   \snippet code/src_corelib_serialization_qcborstream.cpp 26

   The example above uses a function called \c readElementAsString to read the
   map's keys and obtain a string. That is because CBOR maps may contain any
   type as keys, not just strings. User code needs to either perform this
   conversion, reject non-string keys, or instead use a different container
   besides \l QVariantMap and \l QVariantHash. For example, if the map is
   expected to contain integer keys, which is recommended as it reduces stream
   size and parsing, the correct container would be \c{\l{QMap}<int, QVariant>}
   or \c{\l{QHash}<int, QVariant>}.

   \note The code above does not validate that the length is a sensible value.
   If the input stream reports that the length is 1 billion elements, the above
   function will try to allocate some 24 GB or more of RAM, which can lead to a
   crash.

   \sa type(), isArray(), isLengthKnown(), length(), enterContainer(), leaveContainer()
 */

/*!
   \fn bool QCborStreamReader::isTag() const

   Returns true if the type of the current element is a CBOR tag (that is,
   if type() returns QCborStreamReader::Tag). If this function returns
   true, you may call toTag() to read that data.

   \sa type(), toTag()
 */

/*!
   \fn bool QCborStreamReader::isFloat16() const

   Returns true if the type of the current element is an IEEE 754
   half-precision floating point (that is, if type() returns
   QCborStreamReader::Float16). If this function returns true, you may call
   toFloat16() to read that data.

   \sa type(), toFloat16(), isFloat(), isDouble()
 */

/*!
   \fn bool QCborStreamReader::isFloat() const

   Returns true if the type of the current element is an IEEE 754
   single-precision floating point (that is, if type() returns
   QCborStreamReader::Float). If this function returns true, you may call
   toFloat() to read that data.

   \sa type(), toFloat(), isFloat16(), isDouble()
 */

/*!
   \fn bool QCborStreamReader::isDouble() const

   Returns true if the type of the current element is an IEEE 754
   double-precision floating point (that is, if type() returns
   QCborStreamReader::Double). If this function returns true, you may call
   toDouble() to read that data.

   \sa type(), toDouble(), isFloat16(), isFloat()
 */

/*!
   \fn bool QCborStreamReader::isSimpleType() const

   Returns true if the type of the current element is any CBOR simple type,
   including a boolean value (true and false) as well as null and undefined. To
   find out which simple type this is, call toSimpleType(). Alternatively, to
   test for one specific simple type, call the overload that takes a
   QCborSimpleType parameter.

   CBOR simple types are types that do not carry extra value. There are 255
   possibilities, but there are currently only four values that have defined
   meaning. Code is not expected to cope with unknown simple types and may
   simply discard the stream as invalid if it finds an unknown one.

   \sa QCborSimpleType, type(), isSimpleType(QCborSimpleType), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isSimpleType(QCborSimpleType st) const

   Returns true if the type of the current element is the simple type \a st,
   false otherwise. If this function returns true, then toSimpleType() will
   return \a st.

   CBOR simple types are types that do not carry extra value. There are 255
   possibilities, but there are currently only four values that have defined
   meaning. Code is not expected to cope with unknown simple types and may
   simply discard the stream as invalid if it finds an unknown one.

   \sa QCborSimpleType, type(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isFalse() const

   Returns true if the current element is the \c false value, false if it is
   anything else.

   \sa type(), isTrue(), isBool(), toBool(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isTrue() const

   Returns true if the current element is the \c true value, false if it is
   anything else.

   \sa type(), isFalse(), isBool(), toBool(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isBool() const

   Returns true if the current element is a boolean value (\c true or \c
   false), false if it is anything else. If this function returns true, you may
   call toBool() to retrieve the value of the boolean. You may also call
   toSimpleType() and compare to either QCborSimpleValue::True or
   QCborSimpleValue::False.

   \sa type(), isFalse(), isTrue(), toBool(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isNull() const

   Returns true if the current element is the \c null value, false if it is
   anything else. Null values may be used to indicate the absence of some
   optional data.

   \note This function is not the opposite of isValid(). A Null value is a
   valid CBOR value.

   \sa type(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isUndefined() const

   Returns true if the current element is the \c undefined value, false if it
   is anything else. Undefined values may be encoded to indicate that some
   conversion failed or was not possible when creating the stream.
   QCborStreamReader never performs any replacement and this function will only
   return true if the stream contains an explicit undefined value.

   \sa type(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isContainer() const

   Returns true if the current element is a container (that is, an array or a
   map), false if it is anything else. If the current element is a container,
   the isLengthKnown() function may be used to find out if the container's size
   is explicit in the stream and, if so, length() can be used to get that size.

   More importantly, for a container, the enterContainer() function is
   available to begin iterating through the elements contained therein.

   \sa type(), isArray(), isMap(), isLengthKnown(), length(), enterContainer(),
       leaveContainer(), containerDepth()
 */

class QCborStreamReaderPrivate
{
public:
    enum {
        // 9 bytes is the maximum size for any integer, floating point or
        // length in CBOR.
        MaxCborIndividualSize = 9,
        IdealIoBufferSize = 256
    };

    QIODevice *device;
    QByteArray buffer;
    QStack<CborValue> containerStack;

    CborParser parser;
    CborValue currentElement;
    QCborError lastError = {};

    QByteArray::size_type bufferStart;
    bool corrupt = false;

    QCborStreamReaderPrivate(const QByteArray &data)
        : device(nullptr), buffer(data)
    {
        initDecoder();
    }

    QCborStreamReaderPrivate(QIODevice *device)
    {
        setDevice(device);
    }

    ~QCborStreamReaderPrivate()
    {
    }

    void setDevice(QIODevice *dev)
    {
        buffer.clear();
        device = dev;
        initDecoder();
    }

    void initDecoder()
    {
        containerStack.clear();
        bufferStart = 0;
        if (device) {
            buffer.clear();
            buffer.reserve(IdealIoBufferSize);      // sets the CapacityReserved flag
        }

        preread();
        if (CborError err = cbor_parser_init_reader(nullptr, &parser, &currentElement, this))
            handleError(err);
        else
            lastError = { QCborError::NoError };
    }

    char *bufferPtr()
    {
        Q_ASSERT(buffer.isDetached());
        return const_cast<char *>(buffer.constData()) + bufferStart;
    }

    void preread()
    {
        if (device && buffer.size() - bufferStart < MaxCborIndividualSize) {
            // load more, but only if there's more to be read
            qint64 avail = device->bytesAvailable();
            Q_ASSERT(avail >= buffer.size());
            if (avail == buffer.size())
                return;

            if (bufferStart)
                device->skip(bufferStart);  // skip what we've already parsed

            if (buffer.size() != IdealIoBufferSize)
                buffer.resize(IdealIoBufferSize);

            bufferStart = 0;
            qint64 read = device->peek(bufferPtr(), IdealIoBufferSize);
            if (read < 0)
                buffer.clear();
            else if (read != IdealIoBufferSize)
                buffer.truncate(read);
        }
    }

    void handleError(CborError err) noexcept
    {
        Q_ASSERT(err);

        // is the error fatal?
        if (err != CborErrorUnexpectedEOF)
            corrupt = true;

        lastError = QCborError { QCborError::Code(int(err)) };
    }

    void updateBufferAfterString(qsizetype offset, qsizetype size)
    {
        Q_ASSERT(device);

        bufferStart += offset;
        qsizetype newStart = bufferStart + size;
        qsizetype remainingInBuffer = buffer.size() - newStart;

        if (remainingInBuffer <= 0) {
            // We've read from the QIODevice more than what was in the buffer.
            buffer.truncate(0);
        } else {
            // There's still data buffered, but we need to move it around.
            char *ptr = buffer.data();
            memmove(ptr, ptr + newStart, remainingInBuffer);
            buffer.truncate(remainingInBuffer);
        }

        bufferStart = 0;
    }

    bool ensureStringIteration();
};

void qt_cbor_stream_set_error(QCborStreamReaderPrivate *d, QCborError error)
{
    d->handleError(CborError(error.c));
}

static inline bool qt_cbor_decoder_can_read(void *token, size_t len)
{
    Q_ASSERT(len <= QCborStreamReaderPrivate::MaxCborIndividualSize);
    auto self = static_cast<QCborStreamReaderPrivate *>(token);

    qint64 avail = self->buffer.size() - self->bufferStart;
    return len <= quint64(avail);
}

static void qt_cbor_decoder_advance(void *token, size_t len)
{
    Q_ASSERT(len <= QCborStreamReaderPrivate::MaxCborIndividualSize);
    auto self = static_cast<QCborStreamReaderPrivate *>(token);
    Q_ASSERT(len <= size_t(self->buffer.size() - self->bufferStart));

    self->bufferStart += int(len);
    self->preread();
}

static void *qt_cbor_decoder_read(void *token, void *userptr, size_t offset, size_t len)
{
    Q_ASSERT(len == 1 || len == 2 || len == 4 || len == 8);
    Q_ASSERT(offset == 0 || offset == 1);
    auto self = static_cast<const QCborStreamReaderPrivate *>(token);

    // we must have pre-read the data
    Q_ASSERT(len + offset <= size_t(self->buffer.size() - self->bufferStart));
    return memcpy(userptr, self->buffer.constData() + self->bufferStart + offset, len);
}

static CborError qt_cbor_decoder_transfer_string(void *token, const void **userptr, size_t offset, size_t len)
{
    auto self = static_cast<QCborStreamReaderPrivate *>(token);
    Q_ASSERT(offset <= size_t(self->buffer.size()));
    Q_STATIC_ASSERT(sizeof(size_t) >= sizeof(QByteArray::size_type));
    Q_STATIC_ASSERT(sizeof(size_t) == sizeof(qsizetype));

    // check that we will have enough data from the QIODevice before we advance
    // (otherwise, we'd lose the length information)
    qsizetype total;
    if (len > size_t(std::numeric_limits<QByteArray::size_type>::max())
            || add_overflow<qsizetype>(offset, len, &total))
        return CborErrorDataTooLarge;

    // our string transfer is just saving the offset to the userptr
    *userptr = reinterpret_cast<void *>(offset);

    qint64 avail = (self->device ? self->device->bytesAvailable() : self->buffer.size()) -
            self->bufferStart;
    return total > avail ? CborErrorUnexpectedEOF : CborNoError;
}

bool QCborStreamReaderPrivate::ensureStringIteration()
{
    if (currentElement.flags & CborIteratorFlag_IteratingStringChunks)
        return true;

    CborError err = cbor_value_begin_string_iteration(&currentElement);
    if (!err)
        return true;
    handleError(err);
    return false;
}

/*!
   \internal
 */
inline void QCborStreamReader::preparse()
{
    if (lastError() == QCborError::NoError) {
        type_ = cbor_value_get_type(&d->currentElement);

        if (type_ == CborInvalidType) {
            // We may have reached the end.
            if (d->device && d->containerStack.isEmpty()) {
                d->buffer.clear();
                if (d->bufferStart)
                    d->device->skip(d->bufferStart);
                d->bufferStart = 0;
            }
        } else {
            d->lastError = {};
            // Undo the type mapping that TinyCBOR does (we have an explicit type
            // for negative integer and we don't have separate types for Boolean,
            // Null and Undefined).
            if (type_ == CborBooleanType || type_ == CborNullType || type_ == CborUndefinedType) {
                type_ = CborSimpleType;
                value64 = quint8(d->buffer.at(d->bufferStart)) - CborSimpleType;
            } else {
                // Using internal TinyCBOR API!
                value64 = _cbor_value_extract_int64_helper(&d->currentElement);

                if (cbor_value_is_negative_integer(&d->currentElement))
                    type_ = quint8(QCborStreamReader::NegativeInteger);
            }
        }
    } else {
        type_ = Invalid;
    }
}

/*!
   Creates a QCborStreamReader object with no source data. After construction,
   QCborStreamReader will report an error parsing.

   You can add more data by calling addData() or by setting a different source
   device using setDevice().

   \sa addData(), isValid()
 */
QCborStreamReader::QCborStreamReader()
    : QCborStreamReader(QByteArray())
{
}

/*!
   \overload

   Creates a QCborStreamReader object with \a len bytes of data starting at \a
   data. The pointer must remain valid until QCborStreamReader is destroyed.
 */
QCborStreamReader::QCborStreamReader(const char *data, qsizetype len)
    : QCborStreamReader(QByteArray::fromRawData(data, len))
{
}

/*!
   \overload

   Creates a QCborStreamReader object with \a len bytes of data starting at \a
   data. The pointer must remain valid until QCborStreamReader is destroyed.
 */
QCborStreamReader::QCborStreamReader(const quint8 *data, qsizetype len)
    : QCborStreamReader(QByteArray::fromRawData(reinterpret_cast<const char *>(data), len))
{
}

/*!
   \overload

   Creates a QCborStreamReader object that will parse the CBOR stream found in
   \a data.
 */
QCborStreamReader::QCborStreamReader(const QByteArray &data)
    : d(new QCborStreamReaderPrivate(data))
{
    preparse();
}

/*!
   \overload

   Creates a QCborStreamReader object that will parse the CBOR stream found by
   reading from \a device. QCborStreamReader does not take ownership of \a
   device, so it must remain valid until this oject is destroyed.
 */
QCborStreamReader::QCborStreamReader(QIODevice *device)
    : d(new QCborStreamReaderPrivate(device))
{
    preparse();
}

/*!
   Destroys this QCborStreamReader object and frees any associated resources.
 */
QCborStreamReader::~QCborStreamReader()
{
}

/*!
   Sets the source of data to \a device, resetting the decoder to its initial
   state.
 */
void QCborStreamReader::setDevice(QIODevice *device)
{
    d->setDevice(device);
    preparse();
}

/*!
   Returns the QIODevice that was set with either setDevice() or the
   QCborStreamReader constructor. If this object was reading from a QByteArray,
   this function returns nullptr instead.
 */
QIODevice *QCborStreamReader::device() const
{
    return d->device;
}

/*!
   Adds \a data to the CBOR stream and reparses the current element. This
   function is useful if the end of the data was previously reached while
   processing the stream, but now more data is available.
 */
void QCborStreamReader::addData(const QByteArray &data)
{
    addData(data.constData(), data.size());
}

/*!
   \fn void QCborStreamReader::addData(const quint8 *data, qsizetype len)
   \overload

   Adds \a len bytes of data starting at \a data to the CBOR stream and
   reparses the current element. This function is useful if the end of the data
   was previously reached while processing the stream, but now more data is
   available.
 */

/*!
   \overload

   Adds \a len bytes of data starting at \a data to the CBOR stream and
   reparses the current element. This function is useful if the end of the data
   was previously reached while processing the stream, but now more data is
   available.
 */
void QCborStreamReader::addData(const char *data, qsizetype len)
{
    if (!d->device) {
        if (len > 0)
            d->buffer.append(data, len);
        reparse();
    } else {
        qWarning("QCborStreamReader: addData() with device()");
    }
}

/*!
   Reparses the current element. This function must be called when more data
   becomes available in the source QIODevice after parsing failed due to
   reaching the end of the input data before the end of the CBOR stream.

   When reading from QByteArray(), the addData() function automatically calls
   this function. Calling it when the reading had not failed is a no-op.
 */
void QCborStreamReader::reparse()
{
    d->lastError = {};
    d->preread();
    if (CborError err = cbor_value_reparse(&d->currentElement))
        d->handleError(err);
    else
        preparse();
}

/*!
   Clears the decoder state and resets the input source data to an empty byte
   array. After this function is called, QCborStreamReader will be indicating
   an error parsing.

   Call addData() to add more data to be parsed.

   \sa reset(), setDevice()
 */
void QCborStreamReader::clear()
{
    setDevice(nullptr);
}

/*!
   Resets the source back to the beginning and clears the decoder state. If the
   source data was a QByteArray, QCborStreamReader will restart from the
   beginning of the array.

   If the source data is a QIODevice, this function will call
   QIODevice::reset(), which will seek to byte position 0. If the CBOR stream
   is not found at the beginning of the device (e.g., beginning of a file),
   then this function will likely do the wrong thing. Instead, position the
   QIODevice to the right offset and call setDevice().

   \sa clear(), setDevice()
 */
void QCborStreamReader::reset()
{
    if (d->device)
        d->device->reset();
    d->lastError = {};
    d->initDecoder();
    preparse();
}

/*!
   Returns the last error in decoding the stream, if any. If no error
   was encountered, this returns an QCborError::NoError.

   \sa isValid()
 */
QCborError QCborStreamReader::lastError()
{
    return d->lastError;
}

/*!
   Returns the offset in the input stream of the item currently being decoded.
   The current offset is the number of decoded bytes so far only if the source
   data is a QByteArray or it is a QIODevice that was positioned at its
   beginning when decoding started.

   \sa reset(), clear(), device()
 */
qint64 QCborStreamReader::currentOffset() const
{
    return (d->device ? d->device->pos() : 0) + d->bufferStart;
}

/*!
   Returns the number of containers that this stream has entered with
   enterContainer() but not yet left.

   \sa enterContainer(), leaveContainer()
 */
int QCborStreamReader::containerDepth() const
{
    return d->containerStack.size();
}

/*!
   Returns either QCborStreamReader::Array or QCborStreamReader::Map,
   indicating whether the container that contains the current item was an array
   or map, respectively. If we're currently parsing the root element, this
   function returns QCborStreamReader::Invalid.

   \sa containerDepth(), enterContainer()
 */
QCborStreamReader::Type QCborStreamReader::parentContainerType() const
{
    if (d->containerStack.isEmpty())
        return Invalid;
    return Type(cbor_value_get_type(&qAsConst(d->containerStack).top()));
}

/*!
   Returns true if there are more items to be decoded in the current container
   or false of we've reached its end. If we're parsing the root element,
   hasNext() returning false indicates the parsing is complete; otherwise, if
   the container depth is non-zero, then the outer code needs to call
   leaveContainer().

   \sa parentContainerType(), containerDepth(), leaveContainer()
 */
bool QCborStreamReader::hasNext() const noexcept
{
    return cbor_value_is_valid(&d->currentElement) &&
            !cbor_value_at_end(&d->currentElement);
}

/*!
   Advance the CBOR stream decoding one element. You should usually call this
   function when parsing fixed-width basic elements (that is, integers, simple
   values, tags and floating point values). But this function can be called
   when the current item is a string, array or map too and it will skip over
   that entire element, including all contained elements.

   This function returns true if advancing was successful, false otherwise. It
   may fail if the stream is corrupt, incomplete or if the nesting level of
   arrays and maps exceeds \a maxRecursion. Calling this function when
   hasNext() has returned false is also an error. If this function returns
   false, lastError() will return the error code detailing what the failure
   was.

   \sa lastError(), isValid(), hasNext()
 */
bool QCborStreamReader::next(int maxRecursion)
{
    if (lastError() != QCborError::NoError)
        return false;

    if (!hasNext()) {
        d->handleError(CborErrorAdvancePastEOF);
    } else if (maxRecursion < 0) {
        d->handleError(CborErrorNestingTooDeep);
    } else if (isContainer()) {
        // iterate over each element
        enterContainer();
        while (lastError() == QCborError::NoError && hasNext())
            next(maxRecursion - 1);
        if (lastError() == QCborError::NoError)
            leaveContainer();
    } else if (isString() || isByteArray()) {
        auto r = _readByteArray_helper();
        while (r.status == Ok) {
            if (isString() && r.data.size() > MaxStringSize) {
                d->handleError(CborErrorDataTooLarge);
                break;
            }
            if (isString() && !QUtf8::isValidUtf8(r.data, r.data.size()).isValidUtf8) {
                d->handleError(CborErrorInvalidUtf8TextString);
                break;
            }
            r = _readByteArray_helper();
        }
    } else {
        // fixed types
        CborError err = cbor_value_advance_fixed(&d->currentElement);
        if (err)
            d->handleError(err);
    }

    preparse();
    return d->lastError == QCborError::NoError;
}

/*!
   Returns true if the length of the current array, map, byte array or string
   is known (explicit in the CBOR stream), false otherwise. This function
   should only be called if the element is one of those.

   If the length is known, it may be obtained by calling length().

   If the length of a map or an array is not known, it is implied by the number
   of elements present in the stream. QCborStreamReader has no API to calculate
   the length in that condition.

   Strings and byte arrays may also have indeterminate length (that is, they
   may be transmitted in multiple chunks). Those cannot currently be created
   with QCborStreamWriter, but they could be with other encoders, so
   QCborStreamReader supports them.

   \sa length(), QCborStreamWriter::startArray(), QCborStreamWriter::startMap()
 */
bool QCborStreamReader::isLengthKnown() const noexcept
{
    return cbor_value_is_length_known(&d->currentElement);
}

/*!
   Returns the length of the string or byte array, or the number of items in an
   array or the number, of item pairs in a map, if known. This function must
   not be called if the length is unknown (that is, if isLengthKnown() returned
   false). It is an error to do that and it will cause QCborStreamReader to
   stop parsing the input stream.

   \sa isLengthKnown(), QCborStreamWriter::startArray(), QCborStreamWriter::startMap()
 */
quint64 QCborStreamReader::length() const
{
    CborError err;
    switch (type()) {
    case String:
    case ByteArray:
    case Map:
    case Array:
        if (isLengthKnown())
            return value64;
        err = CborErrorUnknownLength;
        break;

    default:
        err = CborErrorIllegalType;
        break;
    }

    d->handleError(err);
    return quint64(-1);
}

/*!
   \fn bool QCborStreamReader::enterContainer()

   Enters the array or map that is the current item and prepares for iterating
   the elements contained in the container. Returns true if entering the
   container succeeded, false otherwise (usually, a parsing error). Each call
   to enterContainer() must be paired with a call to leaveContainer().

   This function may only be called if the current item is an array or a map
   (that is, if isArray(), isMap() or isContainer() is true). Calling it in any
   other condition is an error.

   \sa leaveContainer(), isContainer(), isArray(), isMap()
 */
bool QCborStreamReader::_enterContainer_helper()
{
    d->containerStack.push(d->currentElement);
    CborError err = cbor_value_enter_container(&d->containerStack.top(), &d->currentElement);
    if (!err) {
        preparse();
        return true;
    }
    d->handleError(err);
    return false;
}

/*!
   Leaves the array or map whose items were being processed and positions the
   decoder at the next item after the end of the container. Returns true if
   leaving the container succeeded, false otherwise (usually, a parsing error).
   Each call to enterContainer() must be paired with a call to
   leaveContainer().

   This function may only be called if hasNext() has returned false and
   containerDepth() is not zero. Calling it in any other condition is an error.

   \sa enterContainer(), parentContainerType(), containerDepth()
 */
bool QCborStreamReader::leaveContainer()
{
    if (d->containerStack.isEmpty()) {
        qWarning("QCborStreamReader::leaveContainer: trying to leave top-level element");
        return false;
    }
    if (d->corrupt)
        return false;

    CborValue container = d->containerStack.pop();
    CborError err = cbor_value_leave_container(&container, &d->currentElement);
    d->currentElement = container;
    if (err) {
        d->handleError(err);
        return false;
    }

    preparse();
    return true;
}

/*!
   \fn bool QCborStreamReader::toBool() const

   Returns the boolean value of the current element.

   This function does not perform any type conversions, including from integer.
   Therefore, it may only be called if isTrue(), isFalse() or isBool() returned
   true; calling it in any other condition is an error.

   \sa isBool(), isTrue(), isFalse(), toInteger()
 */

/*!
   \fn QCborTag QCborStreamReader::toTag() const

   Returns the tag value of the current element.

   This function does not perform any type conversions, including from integer.
   Therefore, it may only be called if isTag() is true; calling it in any other
   condition is an error.

   Tags are 64-bit numbers attached to generic CBOR types that give them
   further meaning. For a list of known tags, see the \l QCborKnownTags
   enumeration.

   \sa isTag(), toInteger(), QCborKnownTags
 */

/*!
   \fn quint64 QCborStreamReader::toUnsignedInteger() const

   Returns the unsigned integer value of the current element.

   This function does not perform any type conversions, including from boolean
   or CBOR tag. Therefore, it may only be called if isUnsignedInteger() is
   true; calling it in any other condition is an error.

   This function may be used to obtain numbers beyond the range of the return
   type of toInteger().

   \sa type(), toInteger(), isUnsignedInteger(), isNegativeInteger()
 */

/*!
   \fn QCborNegativeValue QCborStreamReader::toNegativeInteger() const

   Returns the negative integer value of the current element.
   QCborNegativeValue is a 64-bit unsigned integer containing the absolute
   value of the negative number that was stored in the CBOR stream.
   Additionally, QCborNegativeValue(0) represents the number -2\sup{64}.

   This function does not perform any type conversions, including from boolean
   or CBOR tag. Therefore, it may only be called if isNegativeInteger() is
   true; calling it in any other condition is an error.

   This function may be used to obtain numbers beyond the range of the return
   type of toInteger(). However, use of negative numbers smaller than -2\sup{63}
   is extremely discouraged.

   \sa type(), toInteger(), isNegativeInteger(), isUnsignedInteger()
 */

/*!
   \fn qint64 QCborStreamReader::toInteger() const

   Returns the integer value of the current element, be it negative, positive
   or zero. If the value is larger than 2\sup{63} - 1 or smaller than
   -2\sup{63}, the returned value will overflow and will have an incorrect
   sign. If handling those values is required, use toUnsignedInteger() or
   toNegativeInteger() instead.

   This function does not perform any type conversions, including from boolean
   or CBOR tag. Therefore, it may only be called if isInteger() is true;
   calling it in any other condition is an error.

   \sa isInteger(), toUnsignedInteger(), toNegativeInteger()
 */

/*!
   \fn QCborSimpleType QCborStreamReader::toSimpleType() const

   Returns value of the current simple type.

   This function does not perform any type conversions, including from integer.
   Therefore, it may only be called if isSimpleType() is true; calling it in
   any other condition is an error.

   \sa isSimpleType(), isTrue(), isFalse(), isBool(), isNull(), isUndefined()
 */

/*!
   \fn qfloat16 QCborStreamReader::toFloat16() const

   Returns the 16-bit half-precision floating point value of the current element.

   This function does not perform any type conversions, including from other
   floating point types or from integer values. Therefore, it may only be
   called if isFloat16() is true; calling it in any other condition is an
   error.

   \sa isFloat16(), toFloat(), toDouble()
 */

/*!
   \fn float QCborStreamReader::toFloat() const

   Returns the 32-bit single-precision floating point value of the current
   element.

   This function does not perform any type conversions, including from other
   floating point types or from integer values. Therefore, it may only be
   called if isFloat() is true; calling it in any other condition is an error.

   \sa isFloat(), toFloat16(), toDouble()
 */

/*!
   \fn double QCborStreamReader::toDouble() const

   Returns the 64-bit double-precision floating point value of the current
   element.

   This function does not perform any type conversions, including from other
   floating point types or from integer values. Therefore, it may only be
   called if isDouble() is true; calling it in any other condition is an error.

   \sa isDouble(), toFloat16(), toFloat()
 */

/*!
   \fn QCborStreamReader::StringResult<QString> QCborStreamReader::readString()

   Decodes one string chunk from the CBOR string and returns it. This function
   is used for both regular and chunked string contents, so the caller must
   always loop around calling this function, even if isLengthKnown() has
   is true. The typical use of this function is as follows:

   \snippet code/src_corelib_serialization_qcborstream.cpp 27

   This function does not perform any type conversions, including from integers
   or from byte arrays. Therefore, it may only be called if isString() returned
   true; calling it in any other condition is an error.

   \sa readByteArray(), isString(), readStringChunk()
 */
QCborStreamReader::StringResult<QString> QCborStreamReader::_readString_helper()
{
    auto r = _readByteArray_helper();
    QCborStreamReader::StringResult<QString> result;
    result.status = r.status;

    if (r.status == Ok) {
        // See QUtf8::convertToUnicode() a detailed explanation of why this
        // conversion uses the same number of words or less.
        CborError err = CborNoError;
        if (r.data.size() > MaxStringSize) {
            err = CborErrorDataTooLarge;
        } else {
            QTextCodec::ConverterState cs;
            result.data = QUtf8::convertToUnicode(r.data, r.data.size(), &cs);
            if (cs.invalidChars != 0 || cs.remainingChars != 0)
                err = CborErrorInvalidUtf8TextString;
        }

        if (err) {
            d->handleError(err);
            result.data.clear();
            result.status = Error;
        }
    }
    return result;
}

/*!
   \fn QCborStreamReader::StringResult<QString> QCborStreamReader::readByteArray()

   Decodes one byte array chunk from the CBOR string and returns it. This
   function is used for both regular and chunked contents, so the caller must
   always loop around calling this function, even if isLengthKnown() has
   is true. The typical use of this function is as follows:

   \snippet code/src_corelib_serialization_qcborstream.cpp 28

   This function does not perform any type conversions, including from integers
   or from strings. Therefore, it may only be called if isByteArray() is true;
   calling it in any other condition is an error.

   \sa readString(), isByteArray(), readStringChunk()
 */
QCborStreamReader::StringResult<QByteArray> QCborStreamReader::_readByteArray_helper()
{
    QCborStreamReader::StringResult<QByteArray> result;
    result.status = Error;
    qsizetype len = _currentStringChunkSize();
    if (len < 0)
        return result;
    if (len > MaxByteArraySize) {
        d->handleError(CborErrorDataTooLarge);
        return result;
    }

    result.data.resize(len);
    auto r = readStringChunk(result.data.data(), len);
    Q_ASSERT(r.status != Ok || r.data == len);
    result.status = r.status;
    return result;
}

/*!
    \fn qsizetype QCborStreamReader::currentStringChunkSize() const

    Returns the size of the current text or byte string chunk. If the CBOR
    stream contains a non-chunked string (that is, if isLengthKnown() returns
    \c true), this function returns the size of the entire string, the same as
    length().

    This function is useful to pre-allocate the buffer whose pointer can be passed
    to readStringChunk() later.

    \sa readString(), readByteArray(), readStringChunk()
 */
qsizetype QCborStreamReader::_currentStringChunkSize() const
{
    if (!d->ensureStringIteration())
        return -1;

    size_t len;
    CborError err = cbor_value_get_string_chunk_size(&d->currentElement, &len);
    if (err == CborErrorNoMoreStringChunks)
        return 0;           // not a real error
    else if (err)
        d->handleError(err);
    else if (qsizetype(len) < 0)
        d->handleError(CborErrorDataTooLarge);
    else
        return qsizetype(len);
    return -1;
}

/*!
    Reads the current string chunk into the buffer pointed to by \a ptr, whose
    size is \a maxlen. This function returns a \l StringResult object, with the
    number of bytes copied into \a ptr saved in the \c \l StringResult::data
    member. The \c \l StringResult::status member indicates whether there was
    an error reading the string, whether data was copied or whether this was
    the last chunk.

    This function can be called for both \l String and \l ByteArray types.
    For the latter, this function will read the same data that readByteArray()
    would have returned. For strings, it returns the UTF-8 equivalent of the \l
    QString that would have been returned.

    This function is usually used alongside currentStringChunkSize() in a loop.
    For example:

   \snippet code/src_corelib_serialization_qcborstream.cpp 29

    Unlike readByteArray() and readString(), this function is not limited by
    implementation limits of QByteArray and QString.

    \note This function does not perform verification that the UTF-8 contents
    are properly formatted. That means this function does not produce the
    QCborError::InvalidUtf8String error, even when readString() does.

    \sa currentStringChunkSize(), readString(), readByteArray(),
        isString(), isByteArray()
 */
QCborStreamReader::StringResult<qsizetype>
QCborStreamReader::readStringChunk(char *ptr, qsizetype maxlen)
{
    CborError err;
    size_t len;
    const void *content = nullptr;
    QCborStreamReader::StringResult<qsizetype> result;
    result.data = 0;
    result.status = Error;

    d->lastError = {};
    if (!d->ensureStringIteration())
        return result;

#if 1
    // Using internal TinyCBOR API!
    err = _cbor_value_get_string_chunk(&d->currentElement, &content, &len, &d->currentElement);
#else
    // the above is effectively the same as:
    if (cbor_value_is_byte_string(&currentElement))
        err = cbor_value_get_byte_string_chunk(&d->currentElement, reinterpret_cast<const uint8_t **>(&content),
                                               &len, &d->currentElement);
    else
        err = cbor_value_get_text_string_chunk(&d->currentElement, reinterpret_cast<const char **>(&content),
                                               &len, &d->currentElement);
#endif

    // Range check: using implementation-defined behavior in converting an
    // unsigned value out of range of the destination signed type (same as
    // "len > size_t(std::numeric_limits<qsizetype>::max())", but generates
    // better code with ICC and MSVC).
    if (!err && qsizetype(len) < 0)
        err = CborErrorDataTooLarge;

    if (err) {
        if (err == CborErrorNoMoreStringChunks) {
            d->preread();
            err = cbor_value_finish_string_iteration(&d->currentElement);
            result.status = EndOfString;
        }
        if (err)
            d->handleError(err);
        else
            preparse();
        return result;
    }

    // Read the chunk into the user's buffer.
    qint64 actuallyRead;
    qptrdiff offset = qptrdiff(content);
    qsizetype toRead = qsizetype(len);
    qsizetype left = toRead - maxlen;
    if (left < 0)
        left = 0;               // buffer bigger than string
    else
        toRead = maxlen;        // buffer smaller than string

    if (d->device) {
        // This first skip can't fail because we've already read this many bytes.
        d->device->skip(d->bufferStart + qptrdiff(content));
        actuallyRead = d->device->read(ptr, toRead);

        if (actuallyRead != toRead)  {
            actuallyRead = -1;
        } else if (left) {
            qint64 skipped = d->device->skip(left);
            if (skipped != left)
                actuallyRead = -1;
        }

        if (actuallyRead < 0) {
            d->handleError(CborErrorIO);
            return result;
        }

        d->updateBufferAfterString(offset, len);
    } else {
        actuallyRead = toRead;
        memcpy(ptr, d->buffer.constData() + d->bufferStart + offset, toRead);
        d->bufferStart += QByteArray::size_type(offset + len);
    }

    d->preread();
    result.data = actuallyRead;
    result.status = Ok;
    return result;
}

QT_END_NAMESPACE

#include "moc_qcborstreamreader.cpp"
