/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
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

#define CBOR_NO_ENCODER_API
#define CBOR_NO_PARSER_API
#include "qcborcommon_p.h"

#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

#include <cborerrorstrings.c>

/*!
   \headerfile <QtCborCommon>

   \brief The <QtCborCommon> header contains definitions common to both the
   streaming classes (QCborStreamReader and QCborStreamWriter) and to
   QCborValue.

   \sa QCborError
 */

/*!
   \enum QCborSimpleType
   \relates <QtCborCommon>

   This enum contains the possible "Simple Types" for CBOR. Simple Types range
   from 0 to 255 and are types that carry no further value.

   The following values are currently known:

   \value False             A "false" boolean.
   \value True              A "true" boolean.
   \value Null              Absence of value (null).
   \value Undefined         Missing or deleted value, usually an error.

   Qt CBOR API supports encoding and decoding any Simple Type, whether one of
   those above or any other value.

   Applications should only use further values if a corresponding specification
   has been published, otherwise interpretation and validation by the remote
   may fail. Values 24 to 31 are reserved and must not be used.

   The current authoritative list is maintained by IANA in the
   \l{https://www.iana.org/assignments/cbor-simple-values/cbor-simple-values.xml}{Simple
   Values registry}.

   \sa QCborStreamWriter::append(QCborSimpleType), QCborStreamReader::isSimpleType(),
       QCborStreamReader::toSimpleType(), QCborValue::isSimpleType(), QCborValue::toSimpleType()
 */

#if !defined(QT_NO_DATASTREAM)
QDataStream &operator<<(QDataStream &ds, QCborSimpleType st)
{
    return ds << quint8(st);
}

QDataStream &operator>>(QDataStream &ds, QCborSimpleType &st)
{
    quint8 v;
    ds >> v;
    st = QCborSimpleType(v);
    return ds;
}
#endif

/*!
   \enum QCborTag
   \relates <QtCborCommon>

   This enum contains no enumeration and is used only to provide type-safe
   access to a CBOR tag.

   CBOR tags are 64-bit numbers that are attached to generic CBOR types to
   provide further semantic meaning. QCborTag may be constructed from an
   enumeration found in QCborKnownTags or directly by providing the numeric
   representation.

   For example, the following creates a QCborValue containing a byte array
   tagged with a tag 2.

   \snippet code/src_corelib_serialization_qcborstream.cpp 0

   \sa QCborKnownTags, QCborStreamWriter::append(QCborTag),
       QCborStreamReader::isTag(), QCborStreamReader::toTag(),
       QCborValue::isTag(), QCborValue::tag()
 */

/*!
   \enum QCborKnownTags
   \relates <QtCborCommon>

   This enum contains a list of CBOR tags, known at the time of the Qt
   implementation. This list is not meant to be complete and contains only
   tags that are either backed by an RFC or specifically used by the Qt
   implementation.

   The authoritative list is maintained by IANA in the
   \l{https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml}{CBOR tag
   registry}.

   \value DateTimeString        A date and time string, formatted according to RFC 3339, as refined
                                by RFC 4287. It is the same format as Qt::ISODate and
                                Qt::ISODateWithMs.
   \value UnixTime_t            A numerical representation of seconds elapsed since
                                1970-01-01T00:00Z.
   \value PositiveBignum        A positive number of arbitrary length, encoded as a byte array in
                                network byte order. For example, the number 2\sup{64} is represented by
                                a byte array containing the byte value 0x01 followed by 8 zero bytes.
   \value NegativeBignum        A negative number of arbirary length, encoded as the absolute value
                                of that number, minus one. For example, a byte array containing
                                byte value 0x02 followed by 8 zero bytes represents the number
                                -2\sup{65} - 1.
   \value Decimal               A decimal fraction, encoded as an array of two integers: the first
                                is the exponent of the power of 10, the second the integral
                                mantissa. The value 273.15 would be encoded as array \c{[-2, 27315]}.
   \value Bigfloat              Similar to Decimal, but the exponent is a power of 2 instead.
   \value COSE_Encrypt0         An \c Encrypt0 map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Mac0             A \c Mac0 map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Sign1            A \c Sign1 map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value ExpectedBase64url     Indicates that the byte array should be encoded using Base64url
                                if the stream is converted to JSON.
   \value ExpectedBase64        Indicates that the byte array should be encoded using Base64
                                if the stream is converted to JSON.
   \value ExpectedBase16        Indicates that the byte array should be encoded using Base16 (hex)
                                if the stream is converted to JSON.
   \value EncodedCbor           Indicates that the byte array contains a CBOR stream.
   \value Url                   Indicates that the string contains a URL.
   \value Base64url             Indicates that the string contains data encoded using Base64url.
   \value Base64                Indicates that the string contains data encoded using Base64.
   \value RegularExpression     Indicates that the string contains a Perl-Compatible Regular
                                Expression pattern.
   \value MimeMessage           Indicates that the string contains a MIME message (according to
                                \l{https://tools.ietf.org/html/rfc2045}){RFC 2045}.
   \value Uuid                  Indicates that the byte array contains a UUID.
   \value COSE_Encrypt          An \c Encrypt map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Mac              A \c Mac map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Sign             A \c Sign map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value Signature             No change in interpretation; this tag can be used as the outermost
                                tag in a CBOR stream as the file header.

   The following tags are interpreted by QCborValue during decoding and will
   produce objects with extended Qt types, and it will use those tags when
   encoding the same extended types.

   \value DateTimeString        \l QDateTime
   \value UnixTime_t            \l QDateTime (only in decoding)
   \value Url                   \l QUrl
   \value Uuid                  \l QUuid

   Additionally, if a QCborValue containing a QByteArray is tagged using one of
   \c ExpectedBase64url, \c ExpectedBase64 or \c ExpectedBase16, QCborValue
   will use the expected encoding when converting to JSON (see
   QCborValue::toJsonValue).

   \sa QCborTag, QCborStreamWriter::append(QCborTag),
       QCborStreamReader::isTag(), QCborStreamReader::toTag(),
       QCborValue::isTag(), QCborValue::tag()
 */

/*!
   \class QCborError
   \inmodule QtCore
   \inheaderfile QtCborCommon
   \reentrant
   \since 5.12

   \brief The QCborError class holds the error condition found while parsing or
   validating a CBOR stream.

   \sa QCborStreamReader, QCborValue, QCborParserError
 */

/*!
   \enum QCborError::Code

   This enum contains the possible error condition codes.

   \value NoError           No error was detected.
   \value UnknownError      An unknown error occurred and no further details are available.
   \value AdvancePastEnd    QCborStreamReader::next() was called but there are no more elements in
                            the current context.
   \value InputOutputError  An I/O error with the QIODevice occurred.
   \value GarbageAtEnd      Data was found in the input stream after the last element.
   \value EndOfFile         The end of the input stream was unexpectedly reached while processing an
                            element.
   \value UnexpectedBreak   The CBOR stream contains a Break where it is not allowed (data is
                            corrupt and the error is not recoverable).
   \value UnknownType       The CBOR stream contains an unknown/unparseable Type (data is corrupt
                            and the and the error is not recoverable).
   \value IllegalType       The CBOR stream contains a known type in a position it is not allowed
                            to exist (data is corrupt and the error is not recoverable).
   \value IllegalNumber     The CBOR stream appears to be encoding a number larger than 64-bit
                            (data is corrupt and the error is not recoverable).
   \value IllegalSimpleType The CBOR stream contains a Simple Type encoded incorrectly (data is
                            corrupt and the error is not recoverable).
   \value InvalidUtf8String The CBOR stream contains a text string that does not decode properly
                            as UTF-8 (data is corrupt and the error is not recoverable).
   \value DataTooLarge      CBOR string, map or array is too big and cannot be parsed by Qt
                            (internal limitation, but the error is not recoverable).
   \value NestingTooDeep    Too many levels of arrays or maps encountered while processing the
                            input (internal limitation, but the error is not recoverable).
   \value UnsupportedType   The CBOR stream contains a known type that the implementation does not
                            support (internal limitation, but the error is not recoverable).
 */

/*!
   \variable QCborError::c
   \internal
 */

/*!
   \fn QCborError::operator Code() const

   Returns the error code that this QCborError object stores.
 */

/*!
   Returns a text string that matches the error code in this QCborError object.

   Note: the string is not translated. Applications whose interface allow users
   to parse CBOR streams need to provide their own, translated strings.

   \sa QCborError::Code
 */
QString QCborError::toString() const
{
    switch (c) {
    case NoError:
        Q_STATIC_ASSERT(int(NoError) == int(CborNoError));
        return QString();

    case UnknownError:
        Q_STATIC_ASSERT(int(UnknownError) == int(CborUnknownError));
        return QStringLiteral("Unknown error");
    case AdvancePastEnd:
        Q_STATIC_ASSERT(int(AdvancePastEnd) == int(CborErrorAdvancePastEOF));
        return QStringLiteral("Read past end of buffer (more bytes needed)");
    case InputOutputError:
        Q_STATIC_ASSERT(int(InputOutputError) == int(CborErrorIO));
        return QStringLiteral("Input/Output error");
    case GarbageAtEnd:
        Q_STATIC_ASSERT(int(GarbageAtEnd) == int(CborErrorGarbageAtEnd));
        return QStringLiteral("Data found after the end of the stream");
    case EndOfFile:
        Q_STATIC_ASSERT(int(EndOfFile) == int(CborErrorUnexpectedEOF));
        return QStringLiteral("Unexpected end of input data (more bytes needed)");
    case UnexpectedBreak:
        Q_STATIC_ASSERT(int(UnexpectedBreak) == int(CborErrorUnexpectedBreak));
        return QStringLiteral("Invalid CBOR stream: unexpected 'break' byte");
    case UnknownType:
        Q_STATIC_ASSERT(int(UnknownType) == int(CborErrorUnknownType));
        return QStringLiteral("Invalid CBOR stream: unknown type");
    case IllegalType:
        Q_STATIC_ASSERT(int(IllegalType) == int(CborErrorIllegalType));
        return QStringLiteral("Invalid CBOR stream: illegal type found");
    case IllegalNumber:
        Q_STATIC_ASSERT(int(IllegalNumber) == int(CborErrorIllegalNumber));
        return QStringLiteral("Invalid CBOR stream: illegal number encoding (future extension)");
    case IllegalSimpleType:
        Q_STATIC_ASSERT(int(IllegalSimpleType) == int(CborErrorIllegalSimpleType));
        return QStringLiteral("Invalid CBOR stream: illegal simple type");
    case InvalidUtf8String:
        Q_STATIC_ASSERT(int(InvalidUtf8String) == int(CborErrorInvalidUtf8TextString));
        return QStringLiteral("Invalid CBOR stream: invalid UTF-8 text string");
    case DataTooLarge:
        Q_STATIC_ASSERT(int(DataTooLarge) == int(CborErrorDataTooLarge));
        return QStringLiteral("Internal limitation: data set too large");
    case NestingTooDeep:
        Q_STATIC_ASSERT(int(NestingTooDeep) == int(CborErrorNestingTooDeep));
        return QStringLiteral("Internal limitation: data nesting too deep");
    case UnsupportedType:
        Q_STATIC_ASSERT(int(UnsupportedType) == int(CborErrorUnsupportedType));
        return QStringLiteral("Internal limitation: unsupported type");
    }

    // get the error string from TinyCBOR
    CborError err = CborError(int(c));
    return QString::fromLatin1(cbor_error_string(err));
}

QT_END_NAMESPACE

#ifndef QT_BOOTSTRAPPED
#include "moc_qcborcommon.cpp"
#endif
