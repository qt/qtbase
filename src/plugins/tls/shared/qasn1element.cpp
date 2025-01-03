// Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qasn1element_p.h"

#include <QtCore/qdatastream.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlist.h>
#include <QDebug>
#include <private/qtools_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;

typedef QMap<QByteArray, QByteArray> OidNameMap;
static OidNameMap createOidMap()
{
    OidNameMap oids;
    // used by unit tests
    oids.insert(oids.cend(), QByteArrayLiteral("0.9.2342.19200300.100.1.5"), QByteArrayLiteral("favouriteDrink"));
    oids.insert(oids.cend(), QByteArrayLiteral("1.2.840.113549.1.9.1"), QByteArrayLiteral("emailAddress"));
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.6.1.5.5.7.1.1"), QByteArrayLiteral("authorityInfoAccess"));
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.6.1.5.5.7.48.1"), QByteArrayLiteral("OCSP"));
    oids.insert(oids.cend(), QByteArrayLiteral("1.3.6.1.5.5.7.48.2"), QByteArrayLiteral("caIssuers"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.29.14"), QByteArrayLiteral("subjectKeyIdentifier"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.29.15"), QByteArrayLiteral("keyUsage"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.29.17"), QByteArrayLiteral("subjectAltName"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.29.19"), QByteArrayLiteral("basicConstraints"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.29.35"), QByteArrayLiteral("authorityKeyIdentifier"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.10"), QByteArrayLiteral("O"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.11"), QByteArrayLiteral("OU"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.12"), QByteArrayLiteral("title"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.13"), QByteArrayLiteral("description"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.17"), QByteArrayLiteral("postalCode"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.3"), QByteArrayLiteral("CN"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.4"), QByteArrayLiteral("SN"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.41"), QByteArrayLiteral("name"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.42"), QByteArrayLiteral("GN"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.43"), QByteArrayLiteral("initials"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.46"), QByteArrayLiteral("dnQualifier"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.5"), QByteArrayLiteral("serialNumber"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.6"), QByteArrayLiteral("C"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.7"), QByteArrayLiteral("L"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.8"), QByteArrayLiteral("ST"));
    oids.insert(oids.cend(), QByteArrayLiteral("2.5.4.9"), QByteArrayLiteral("street"));
    return oids;
}
Q_GLOBAL_STATIC_WITH_ARGS(OidNameMap, oidNameMap, (createOidMap()))

QAsn1Element::QAsn1Element(quint8 type, const QByteArray &value)
    : mType(type)
    , mValue(value)
{
}

bool QAsn1Element::read(QDataStream &stream)
{
    // type
    quint8 tmpType;
    stream >> tmpType;
    if (!tmpType)
        return false;

    // length
    quint64 length = 0;
    quint8 first;
    stream >> first;
    if (first & 0x80) {
        // long form
        const quint8 bytes = (first & 0x7f);
        if (bytes > 7)
            return false;

        quint8 b;
        for (int i = 0; i < bytes; i++) {
            stream >> b;
            length = (length << 8) | b;
        }
    } else {
        // short form
        length = (first & 0x7f);
    }

    if (length > quint64(std::numeric_limits<int>::max()))
        return false;

    // read value in blocks to avoid being fooled by incorrect length
    const int BUFFERSIZE = 4 * 1024;
    QByteArray tmpValue;
    int remainingLength = length;
    while (remainingLength) {
        char readBuffer[BUFFERSIZE];
        const int bytesToRead = qMin(remainingLength, BUFFERSIZE);
        const int count = stream.readRawData(readBuffer, bytesToRead);
        if (count != int(bytesToRead))
            return false;
        tmpValue.append(readBuffer, bytesToRead);
        remainingLength -= bytesToRead;
    }

    mType = tmpType;
    mValue.swap(tmpValue);
    return true;
}

bool QAsn1Element::read(const QByteArray &data)
{
    QDataStream stream(data);
    return read(stream);
}

void QAsn1Element::write(QDataStream &stream) const
{
    // type
    stream << mType;

    // length
    qint64 length = mValue.size();
    if (length >= 128) {
        // long form
        quint8 encodedLength = 0x80;
        QByteArray ba;
        while (length) {
            ba.prepend(quint8((length & 0xff)));
            length >>= 8;
            encodedLength += 1;
        }
        stream << encodedLength;
        stream.writeRawData(ba.data(), ba.size());
    } else {
        // short form
        stream << quint8(length);
    }

    // value
    stream.writeRawData(mValue.data(), mValue.size());
}

QAsn1Element QAsn1Element::fromBool(bool val)
{
    return QAsn1Element(QAsn1Element::BooleanType,
        QByteArray(1, val ? 0xff : 0x00));
}

QAsn1Element QAsn1Element::fromInteger(unsigned int val)
{
    QAsn1Element elem(QAsn1Element::IntegerType);
    while (val > 127) {
        elem.mValue.prepend(val & 0xff);
        val >>= 8;
    }
    elem.mValue.prepend(val & 0x7f);
    return elem;
}

QAsn1Element QAsn1Element::fromVector(const QList<QAsn1Element> &items)
{
    QAsn1Element seq;
    seq.mType = SequenceType;
    QDataStream stream(&seq.mValue, QDataStream::WriteOnly);
    for (auto it = items.cbegin(), end = items.cend(); it != end; ++it)
        it->write(stream);
    return seq;
}

QAsn1Element QAsn1Element::fromObjectId(const QByteArray &id)
{
    QAsn1Element elem;
    elem.mType = ObjectIdentifierType;
    const QList<QByteArray> bits = id.split('.');
    Q_ASSERT(bits.size() > 2);
    elem.mValue += quint8((bits[0].toUInt() * 40 + bits[1].toUInt()));
    for (int i = 2; i < bits.size(); ++i) {
        char buffer[std::numeric_limits<unsigned int>::digits / 7 + 2];
        char *pBuffer = buffer + sizeof(buffer);
        *--pBuffer = '\0';
        unsigned int node = bits[i].toUInt();
        *--pBuffer = quint8((node & 0x7f));
        node >>= 7;
        while (node) {
            *--pBuffer = quint8(((node & 0x7f) | 0x80));
            node >>= 7;
        }
        elem.mValue += pBuffer;
    }
    return elem;
}

bool QAsn1Element::toBool(bool *ok) const
{
    if (*this == fromBool(true)) {
        if (ok)
            *ok = true;
        return true;
    } else if (*this == fromBool(false)) {
        if (ok)
            *ok = true;
        return false;
    } else {
        if (ok)
            *ok = false;
        return false;
    }
}

QDateTime QAsn1Element::toDateTime() const
{
    QDateTime result;

    if (mValue.size() != 13 && mValue.size() != 15)
        return result;

    // QDateTime::fromString is lenient and accepts +- signs in front
    // of the year; but ASN.1 doesn't allow them.
    if (!isAsciiDigit(mValue[0]))
        return result;

    // Timezone must be present, and UTC
    if (mValue.back() != 'Z')
        return result;

    if (mType == UtcTimeType && mValue.size() == 13) {
        result = QDateTime::fromString(QString::fromLatin1(mValue),
                                       QStringLiteral("yyMMddHHmmsst"));
        if (!result.isValid())
            return result;

        Q_ASSERT(result.timeSpec() == Qt::UTC);

        QDate date = result.date();

        // RFC 2459:
        //   Where YY is greater than or equal to 50, the year shall be
        //   interpreted as 19YY; and
        //
        //   Where YY is less than 50, the year shall be interpreted as 20YY.
        //
        // QDateTime interprets the 'yy' format as 19yy, so we may need to adjust
        // the year (bring it in the [1950, 2049] range).
        if (date.year() < 1950)
            result.setDate(date.addYears(100));

        Q_ASSERT(result.date().year() >= 1950);
        Q_ASSERT(result.date().year() <= 2049);
    } else if (mType == GeneralizedTimeType && mValue.size() == 15) {
        result = QDateTime::fromString(QString::fromLatin1(mValue),
                                       QStringLiteral("yyyyMMddHHmmsst"));
    }

    return result;
}

QMultiMap<QByteArray, QString> QAsn1Element::toInfo() const
{
    QMultiMap<QByteArray, QString> info;
    QAsn1Element elem;
    QDataStream issuerStream(mValue);
    while (elem.read(issuerStream) && elem.mType == QAsn1Element::SetType) {
        QAsn1Element issuerElem;
        QDataStream setStream(elem.mValue);
        if (issuerElem.read(setStream) && issuerElem.mType == QAsn1Element::SequenceType) {
            const auto elems = issuerElem.toList();
            if (elems.size() == 2) {
                const QByteArray key = elems.front().toObjectName();
                if (!key.isEmpty())
                    info.insert(key, elems.back().toString());
            }
        }
    }
    return info;
}

qint64 QAsn1Element::toInteger(bool *ok) const
{
    if (mType != QAsn1Element::IntegerType || mValue.isEmpty()) {
        if (ok)
            *ok = false;
        return 0;
    }

    // NOTE: - negative numbers are not handled
    //       - greater sizes would overflow
    if (mValue.at(0) & 0x80 || mValue.size() > 8) {
        if (ok)
            *ok = false;
        return 0;
    }

    qint64 value = mValue.at(0) & 0x7f;
    for (int i = 1; i < mValue.size(); ++i)
        value = (value << 8) | quint8(mValue.at(i));

    if (ok)
        *ok = true;
    return value;
}

QList<QAsn1Element> QAsn1Element::toList() const
{
    QList<QAsn1Element> items;
    if (mType == SequenceType) {
        QAsn1Element elem;
        QDataStream stream(mValue);
        while (elem.read(stream))
            items << elem;
    }
    return items;
}

QByteArray QAsn1Element::toObjectId() const
{
    QByteArray key;
    if (mType == ObjectIdentifierType && !mValue.isEmpty()) {
        quint8 b = mValue.at(0);
        key += QByteArray::number(b / 40) + '.' + QByteArray::number (b % 40);
        unsigned int val = 0;
        for (int i = 1; i < mValue.size(); ++i) {
            b = mValue.at(i);
            val = (val << 7) | (b & 0x7f);
            if (!(b & 0x80)) {
                key += '.' + QByteArray::number(val);
                val = 0;
            }
        }
    }
    return key;
}

QByteArray QAsn1Element::toObjectName() const
{
    QByteArray key = toObjectId();
    return oidNameMap->value(key, key);
}

QString QAsn1Element::toString() const
{
    // Detect embedded NULs and reject
    if (qstrlen(mValue) < uint(mValue.size()))
        return QString();

    if (mType == PrintableStringType || mType == TeletexStringType
        || mType == Rfc822NameType || mType == DnsNameType
        || mType == UniformResourceIdentifierType)
        return QString::fromLatin1(mValue, mValue.size());
    if (mType == Utf8StringType)
        return QString::fromUtf8(mValue, mValue.size());

    return QString();
}

QT_END_NAMESPACE
