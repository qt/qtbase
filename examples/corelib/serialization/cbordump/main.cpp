// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCborStreamReader>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QStack>

#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

using namespace Qt::StringLiterals;

/*
 * To regenerate:
 *  curl -O https://www.iana.org/assignments/cbor-tags/cbor-tags.xml
 *  ./cbortag.py cbor-tags.xml
 *
 * The XHTML URL mentioned in the comment below is a human-readable version of
 * the same resource.
 */

// GENERATED CODE
struct CborTagDescription
{
    QCborTag tag;
    const char *description; // with space and parentheses
};

// Concise Binary Object Representation (CBOR) Tags
static const CborTagDescription tagDescriptions[] = {
    // from https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
    { QCborTag(0), " (Standard date/time string; see Section 3.4.1 [RFC8949])" },
    { QCborTag(1), " (Epoch-based date/time; see Section 3.4.2 [RFC8949])" },
    { QCborTag(2), " (Positive bignum; see Section 3.4.3 [RFC8949])" },
    { QCborTag(3), " (Negative bignum; see Section 3.4.3 [RFC8949])" },
    { QCborTag(4), " (Decimal fraction; see Section 3.4.4 [RFC8949])" },
    { QCborTag(5), " (Bigfloat; see Section 3.4.4 [RFC8949])" },
    { QCborTag(16), " (COSE Single Recipient Encrypted Data Object [RFC9052])" },
    { QCborTag(17), " (COSE Mac w/o Recipients Object [RFC9052])" },
    { QCborTag(18), " (COSE Single Signer Data Object [RFC9052])" },
    { QCborTag(19), " (COSE standalone V2 countersignature [RFC9338])" },
    { QCborTag(21),
      " (Expected conversion to base64url encoding; see Section 3.4.5.2 [RFC8949])" },
    { QCborTag(22), " (Expected conversion to base64 encoding; see Section 3.4.5.2 [RFC8949])" },
    { QCborTag(23), " (Expected conversion to base16 encoding; see Section 3.4.5.2 [RFC8949])" },
    { QCborTag(24), " (Encoded CBOR data item; see Section 3.4.5.1 [RFC8949])" },
    { QCborTag(25), " (reference the nth previously seen string)" },
    { QCborTag(26), " (Serialised Perl object with classname and constructor arguments)" },
    { QCborTag(27),
      " (Serialised language-independent object with type name and constructor arguments)" },
    { QCborTag(28), " (mark value as (potentially) shared)" },
    { QCborTag(29), " (reference nth marked value)" },
    { QCborTag(30), " (Rational number)" },
    { QCborTag(31), " (Absent value in a CBOR Array)" },
    { QCborTag(32), " (URI; see Section 3.4.5.3 [RFC8949])" },
    { QCborTag(33), " (base64url; see Section 3.4.5.3 [RFC8949])" },
    { QCborTag(34), " (base64; see Section 3.4.5.3 [RFC8949])" },
    { QCborTag(35), " (Regular expression; see Section 2.4.4.3 [RFC7049])" },
    { QCborTag(36), " (MIME message; see Section 3.4.5.3 [RFC8949])" },
    { QCborTag(37), " (Binary UUID [RFC4122, Section 4.1.2])" },
    { QCborTag(38), " (Language-tagged string [RFC9290, Appendix A])" },
    { QCborTag(39), " (Identifier)" },
    { QCborTag(40), " (Multi-dimensional Array, row-major order [RFC8746])" },
    { QCborTag(41), " (Homogeneous Array [RFC8746])" },
    { QCborTag(42), " (IPLD content identifier)" },
    { QCborTag(43), " (YANG bits datatype; see Section 6.7. [RFC9254])" },
    { QCborTag(44), " (YANG enumeration datatype; see Section 6.6. [RFC9254])" },
    { QCborTag(45), " (YANG identityref datatype; see Section 6.10. [RFC9254])" },
    { QCborTag(46), " (YANG instance-identifier datatype; see Section 6.13. [RFC9254])" },
    { QCborTag(47), " (YANG Schema Item iDentifier (sid); see Section 3.2. [RFC9254])" },
    { QCborTag(52), " (IPv4, [prefixlen,IPv4], [IPv4,prefixpart] [RFC9164])" },
    { QCborTag(54), " (IPv6, [prefixlen,IPv6], [IPv6,prefixpart] [RFC9164])" },
    { QCborTag(61), " (CBOR Web Token (CWT) [RFC8392])" },
    { QCborTag(63), " (Encoded CBOR Sequence [RFC8742])" },
    { QCborTag(64), " (uint8 Typed Array [RFC8746])" },
    { QCborTag(65), " (uint16, big endian, Typed Array [RFC8746])" },
    { QCborTag(66), " (uint32, big endian, Typed Array [RFC8746])" },
    { QCborTag(67), " (uint64, big endian, Typed Array [RFC8746])" },
    { QCborTag(68), " (uint8 Typed Array, clamped arithmetic [RFC8746])" },
    { QCborTag(69), " (uint16, little endian, Typed Array [RFC8746])" },
    { QCborTag(70), " (uint32, little endian, Typed Array [RFC8746])" },
    { QCborTag(71), " (uint64, little endian, Typed Array [RFC8746])" },
    { QCborTag(72), " (sint8 Typed Array [RFC8746])" },
    { QCborTag(73), " (sint16, big endian, Typed Array [RFC8746])" },
    { QCborTag(74), " (sint32, big endian, Typed Array [RFC8746])" },
    { QCborTag(75), " (sint64, big endian, Typed Array [RFC8746])" },
    { QCborTag(76), " ((reserved) [RFC8746])" },
    { QCborTag(77), " (sint16, little endian, Typed Array [RFC8746])" },
    { QCborTag(78), " (sint32, little endian, Typed Array [RFC8746])" },
    { QCborTag(79), " (sint64, little endian, Typed Array [RFC8746])" },
    { QCborTag(80), " (IEEE 754 binary16, big endian, Typed Array [RFC8746])" },
    { QCborTag(81), " (IEEE 754 binary32, big endian, Typed Array [RFC8746])" },
    { QCborTag(82), " (IEEE 754 binary64, big endian, Typed Array [RFC8746])" },
    { QCborTag(83), " (IEEE 754 binary128, big endian, Typed Array [RFC8746])" },
    { QCborTag(84), " (IEEE 754 binary16, little endian, Typed Array [RFC8746])" },
    { QCborTag(85), " (IEEE 754 binary32, little endian, Typed Array [RFC8746])" },
    { QCborTag(86), " (IEEE 754 binary64, little endian, Typed Array [RFC8746])" },
    { QCborTag(87), " (IEEE 754 binary128, little endian, Typed Array [RFC8746])" },
    { QCborTag(96), " (COSE Encrypted Data Object [RFC9052])" },
    { QCborTag(97), " (COSE MACed Data Object [RFC9052])" },
    { QCborTag(98), " (COSE Signed Data Object [RFC9052])" },
    { QCborTag(100), " (Number of days since the epoch date 1970-01-01 [RFC8943])" },
    { QCborTag(101), " (alternatives as given by the uint + 128; see Section 9.1)" },
    { QCborTag(103), " (Geographic Coordinates)" },
    { QCborTag(104), " (Geographic Coordinate Reference System WKT or EPSG number)" },
    { QCborTag(110), " (relative object identifier (BER encoding); SDNV sequence [RFC9090])" },
    { QCborTag(111), " (object identifier (BER encoding) [RFC9090])" },
    { QCborTag(112), " (object identifier (BER encoding), relative to 1.3.6.1.4.1 [RFC9090])" },
    { QCborTag(120), " (Internet of Things Data Point)" },
    { QCborTag(260),
      " (Network Address (IPv4 or IPv6 or MAC Address) (DEPRECATED in favor of 52 and 54 for IP"
      " addresses) [RFC9164])" },
    { QCborTag(261),
      " (Network Address Prefix (IPv4 or IPv6 Address + Mask Length) (DEPRECATED in favor of 52"
      " and 54 for IP addresses) [RFC9164])" },
    { QCborTag(271),
      " (DDoS Open Threat Signaling (DOTS) signal channel object, as defined in [RFC9132])" },
    { QCborTag(1004), " (full-date string [RFC8943])" },
    { QCborTag(1040), " (Multi-dimensional Array, column-major order [RFC8746])" },
    { QCborTag(55799), " (Self-described CBOR; see Section 3.4.6 [RFC8949])" },
    { QCborTag(55800), " (indicates that the file contains CBOR Sequences [RFC9277])" },
    { QCborTag(55801),
      " (indicates that the file starts with a CBOR-Labeled Non-CBOR Data label. [RFC9277])" },
    { QCborTag(-1), nullptr }
};
// END GENERATED CODE

enum {
    // See RFC 7049 section 2.
    SmallValueBitLength = 5,
    SmallValueMask = (1 << SmallValueBitLength) - 1, /* 0x1f */
    Value8Bit = 24,
    Value16Bit = 25,
    Value32Bit = 26,
    Value64Bit = 27
};

//! [0]
struct CborDumper
{
    enum DumpOption { ShowCompact = 0x01, ShowWidthIndicators = 0x02, ShowAnnotated = 0x04 };
    Q_DECLARE_FLAGS(DumpOptions, DumpOption)

    CborDumper(QFile *f, DumpOptions opts_);
    QCborError dump();

private:
    void dumpOne(int nestingLevel);
    void dumpOneDetailed(int nestingLevel);

    void printByteArray(const QByteArray &ba);
    void printWidthIndicator(quint64 value, char space = '\0');
    void printStringWidthIndicator(quint64 value);

    QCborStreamReader reader;
    QByteArray data;
    QStack<quint8> byteArrayEncoding;
    qint64 offset = 0;
    DumpOptions opts;
};
//! [0]
Q_DECLARE_OPERATORS_FOR_FLAGS(CborDumper::DumpOptions)

static int cborNumberSize(quint64 value)
{
    int normalSize = 1;
    if (value > std::numeric_limits<quint32>::max())
        normalSize += 8;
    else if (value > std::numeric_limits<quint16>::max())
        normalSize += 4;
    else if (value > std::numeric_limits<quint8>::max())
        normalSize += 2;
    else if (value >= Value8Bit)
        normalSize += 1;
    return normalSize;
}

CborDumper::CborDumper(QFile *f, DumpOptions opts_) : opts(opts_)
{
    // try to mmap the file, this is faster
    char *ptr = reinterpret_cast<char *>(f->map(0, f->size(), QFile::MapPrivateOption));
    if (ptr) {
        // worked
        data = QByteArray::fromRawData(ptr, f->size());
        reader.addData(data);
    } else if ((opts & ShowAnnotated) || f->isSequential()) {
        // details requires full contents, so allocate memory
        data = f->readAll();
        reader.addData(data);
    } else {
        // just use the QIODevice
        reader.setDevice(f);
    }
}

QCborError CborDumper::dump()
{
    byteArrayEncoding << quint8(QCborKnownTags::ExpectedBase16);
    if (!reader.lastError()) {
        if (opts & ShowAnnotated)
            dumpOneDetailed(0);
        else
            dumpOne(0);
    }

    QCborError err = reader.lastError();
    offset = reader.currentOffset();
    if (err) {
        fflush(stdout);
        fprintf(stderr, "cbordump: decoding failed at %lld: %s\n",
                offset, qPrintable(err.toString()));
        if (!data.isEmpty())
            fprintf(stderr, " bytes at %lld: %s\n", offset,
                    data.mid(offset, 9).toHex(' ').constData());
    } else {
        if (!opts.testFlag(ShowAnnotated))
            printf("\n");
        if (offset < data.size() || (reader.device() && reader.device()->bytesAvailable()))
            fprintf(stderr, "Warning: bytes remaining at the end of the CBOR stream\n");
    }

    return err;
}

template<typename T>
static inline bool canConvertTo(double v)
{
    using TypeInfo = std::numeric_limits<T>;
    // The [conv.fpint] (7.10 Floating-integral conversions) section of the
    // standard says only exact conversions are guaranteed. Converting
    // integrals to floating-point with loss of precision has implementation-
    // defined behavior whether the next higher or next lower is returned;
    // converting FP to integral is UB if it can't be represented.;
    static_assert(TypeInfo::is_integer);

    double supremum = ldexp(1, TypeInfo::digits);
    if (v >= supremum)
        return false;

    if (v < TypeInfo::min()) // either zero or a power of two, so it's exact
        return false;

    // we're in range
    return v == floor(v);
}

static QString fpToString(double v, QLatin1StringView suffix = ""_L1)
{
    if (qIsInf(v))
        return v < 0 ? "-inf"_L1 : "inf"_L1;
    if (qIsNaN(v))
        return "nan"_L1;
    if (canConvertTo<qint64>(v))
        return QString::number(qint64(v)) + ".0"_L1 + suffix;
    if (canConvertTo<quint64>(v))
        return QString::number(quint64(v)) + ".0"_L1 + suffix;

    QString s = QString::number(v, 'g', QLocale::FloatingPointShortest);
    if (!s.contains(u'.') && !s.contains(u'e'))
        s += u'.';
    if (suffix.size())
        s += suffix;
    return s;
};

void CborDumper::dumpOne(int nestingLevel)
{
    QString indent(1, u' ');
    QString indented = indent;
    if (!opts.testFlag(ShowCompact)) {
        indent = u'\n' + QString(4 * nestingLevel, u' ');
        indented = u'\n' + QString(4 + 4 * nestingLevel, u' ');
    }

    switch (reader.type()) {
    case QCborStreamReader::UnsignedInteger: {
        quint64 u = reader.toUnsignedInteger();
        printf("%llu", u);
        reader.next();
        printWidthIndicator(u);
        return;
    }

    case QCborStreamReader::NegativeInteger: {
        quint64 n = quint64(reader.toNegativeInteger());
        if (n == 0) // -2^64 (wrapped around)
            printf("-18446744073709551616");
        else
            printf("-%llu", n);
        reader.next();
        printWidthIndicator(n);
        return;
    }

    case QCborStreamReader::ByteArray:
    case QCborStreamReader::String: {
        bool isLengthKnown = reader.isLengthKnown();
        if (!isLengthKnown) {
            printf("(_ ");
            ++offset;
        }

        QString comma;
        if (reader.isByteArray()) {
            auto r = reader.readByteArray();
            while (r.status == QCborStreamReader::Ok) {
                printf("%s", qPrintable(comma));
                printByteArray(r.data);
                printStringWidthIndicator(r.data.size());

                r = reader.readByteArray();
                comma = u',' + indented;
            }
        } else {
            auto r = reader.readString();
            while (r.status == QCborStreamReader::Ok) {
                printf("%s\"%s\"", qPrintable(comma), qPrintable(r.data));
                printStringWidthIndicator(r.data.toUtf8().size());

                r = reader.readString();
                comma = u',' + indented;
            }
        }

        if (!isLengthKnown && !reader.lastError())
            printf(")");
        break;
    }

    case QCborStreamReader::Array:
    case QCborStreamReader::Map: {
        const char *delimiters = (reader.isArray() ? "[]" : "{}");
        printf("%c", delimiters[0]);

        if (reader.isLengthKnown()) {
            quint64 len = reader.length();
            reader.enterContainer();
            printWidthIndicator(len, ' ');
        } else {
            reader.enterContainer();
            offset = reader.currentOffset();
            printf("_ ");
        }

        const char *comma = "";
        while (!reader.lastError() && reader.hasNext()) {
            printf("%s%s", comma, qPrintable(indented));
            comma = ",";
            dumpOne(nestingLevel + 1);

            if (reader.parentContainerType() != QCborStreamReader::Map)
                continue;
            if (reader.lastError())
                break;
            printf(": ");
            dumpOne(nestingLevel + 1);
        }

        if (!reader.lastError()) {
            reader.leaveContainer();
            printf("%s%c", qPrintable(indent), delimiters[1]);
        }
        break;
    }

    case QCborStreamReader::Tag: {
        QCborTag tag = reader.toTag();
        printf("%llu", quint64(tag));

        if (tag == QCborKnownTags::ExpectedBase16 || tag == QCborKnownTags::ExpectedBase64
                || tag == QCborKnownTags::ExpectedBase64url)
            byteArrayEncoding.push(quint8(tag));

        if (reader.next()) {
            printWidthIndicator(quint64(tag));
            printf("(");
            dumpOne(nestingLevel); // same level!
            printf(")");
        }

        if (tag == QCborKnownTags::ExpectedBase16 || tag == QCborKnownTags::ExpectedBase64
                || tag == QCborKnownTags::ExpectedBase64url)
            byteArrayEncoding.pop();
        break;
    }

    case QCborStreamReader::SimpleType:
        switch (reader.toSimpleType()) {
        case QCborSimpleType::False:
            printf("false");
            break;
        case QCborSimpleType::True:
            printf("true");
            break;
        case QCborSimpleType::Null:
            printf("null");
            break;
        case QCborSimpleType::Undefined:
            printf("undefined");
            break;
        default:
            printf("simple(%u)", quint8(reader.toSimpleType()));
            break;
        }
        reader.next();
        break;

    case QCborStreamReader::Float16:
        printf("%s", qPrintable(fpToString(reader.toFloat16(), "f16"_L1)));
        reader.next();
        break;
    case QCborStreamReader::Float:
        printf("%s", qPrintable(fpToString(reader.toFloat(), "f"_L1)));
        reader.next();
        break;
    case QCborStreamReader::Double:
        printf("%s", qPrintable(fpToString(reader.toDouble())));
        reader.next();
        break;
    case QCborStreamReader::Invalid:
        return;
    }

    offset = reader.currentOffset();
}

void CborDumper::dumpOneDetailed(int nestingLevel)
{
    auto tagDescription = [](QCborTag tag) {
        for (auto entry : tagDescriptions) {
            if (entry.tag == tag)
                return entry.description;
            if (entry.tag > tag)
                break;
        }
        return "";
    };
    auto printOverlong = [](int actualSize, quint64 value) {
        if (cborNumberSize(value) != actualSize)
            printf(" (overlong)");
    };
    auto print = [&](const char *descr, const char *fmt, ...) {
        qint64 prevOffset = offset;
        offset = reader.currentOffset();
        if (prevOffset == offset)
            return;

        QByteArray bytes = data.mid(prevOffset, offset - prevOffset);
        QByteArray indent(nestingLevel * 2, ' ');
        printf("%-50s # %s ", (indent + bytes.toHex(' ')).constData(), descr);

        va_list va;
        va_start(va, fmt);
        vprintf(fmt, va);
        va_end(va);

        if (strstr(fmt, "%ll")) {
            // Only works because all callers below that use %ll, use it as the
            // first arg
            va_start(va, fmt);
            quint64 value = va_arg(va, quint64);
            va_end(va);
            printOverlong(bytes.size(), value);
        }

        puts("");
    };

    auto printFp = [=](const char *descr, double d) {
        QString s = fpToString(d);
        if (s.size() <= 6)
            return print(descr, "%s", qPrintable(s));
        return print(descr, "%a", d);
    };

    auto printString = [&](const char *descr) {
        constexpr qsizetype ChunkSizeLimit = std::numeric_limits<int>::max();
        QByteArray indent(nestingLevel * 2, ' ');
        const char *chunkStr = (reader.isLengthKnown() ? "" : "chunk ");
        int width = 48 - indent.size();
        int bytesPerLine = qMax(width / 3, 5);

        qsizetype size = reader.currentStringChunkSize();
        if (size < 0)
            return; // error
        if (size >= ChunkSizeLimit) {
            fprintf(stderr, "String length too big, %lli\n", qint64(size));
            exit(EXIT_FAILURE);
        }

        // if asking for the current string chunk changes the offset, then it
        // was chunked
        print(descr, "(indeterminate length)");

        QByteArray bytes(size, Qt::Uninitialized);
        auto r = reader.readStringChunk(bytes.data(), bytes.size());
        while (r.status == QCborStreamReader::Ok) {
            // We'll have to decode the length's width directly from CBOR...
            const char *lenstart = data.constData() + offset;
            const char *lenend = lenstart + 1;
            quint8 additionalInformation = (*lenstart & SmallValueMask);

            // Decode this number directly from CBOR (see RFC 7049 section 2)
            if (additionalInformation >= Value8Bit) {
                if (additionalInformation == Value8Bit)
                    lenend += 1;
                else if (additionalInformation == Value16Bit)
                    lenend += 2;
                else if (additionalInformation == Value32Bit)
                    lenend += 4;
                else
                    lenend += 8;
            }

            {
                QByteArray lenbytes = QByteArray::fromRawData(lenstart, lenend - lenstart);
                printf("%-50s # %s %slength %llu",
                       (indent + lenbytes.toHex(' ')).constData(), descr, chunkStr, quint64(size));
                printOverlong(lenbytes.size(), size);
                puts("");
            }

            offset = reader.currentOffset();

            for (int i = 0; i < r.data; i += bytesPerLine) {
                QByteArray section = bytes.mid(i, bytesPerLine);
                printf("  %s%s", indent.constData(), section.toHex(' ').constData());

                // print the decode
                QByteArray spaces(width > 0 ? width - section.size() * 3 + 1 : 0, ' ');
                printf("%s # \"", spaces.constData());
                auto ptr = reinterpret_cast<const uchar *>(section.constData());
                for (int j = 0; j < section.size(); ++j)
                    printf("%c", ptr[j] >= 0x80 || ptr[j] < 0x20 ? '.' : ptr[j]);

                puts("\"");
            }

            // get the next chunk
            size = reader.currentStringChunkSize();
            if (size < 0)
                return; // error
            if (size >= ChunkSizeLimit) {
                fprintf(stderr, "String length too big, %lli\n", qint64(size));
                exit(EXIT_FAILURE);
            }
            bytes.resize(size);
            r = reader.readStringChunk(bytes.data(), bytes.size());
        }
    };

    if (reader.lastError())
        return;

    switch (reader.type()) {
    case QCborStreamReader::UnsignedInteger: {
        quint64 u = reader.toUnsignedInteger();
        reader.next();
        if (u < 65536 || (u % 100000) == 0)
            print("Unsigned integer", "%llu", u);
        else
            print("Unsigned integer", "0x%llx", u);
        return;
    }

    case QCborStreamReader::NegativeInteger: {
        quint64 n = quint64(reader.toNegativeInteger());
        reader.next();
        print("Negative integer", n == 0 ? "-18446744073709551616" : "-%llu", n);
        return;
    }

    case QCborStreamReader::ByteArray:
    case QCborStreamReader::String: {
        bool isLengthKnown = reader.isLengthKnown();
        const char *descr = (reader.isString() ? "Text string" : "Byte string");
        if (!isLengthKnown)
            ++nestingLevel;

        printString(descr);
        if (reader.lastError())
            return;

        if (!isLengthKnown) {
            --nestingLevel;
            print("Break", "");
        }
        break;
    }

    case QCborStreamReader::Array:
    case QCborStreamReader::Map: {
        const char *descr = (reader.isArray() ? "Array" : "Map");
        if (reader.isLengthKnown()) {
            quint64 len = reader.length();
            reader.enterContainer();
            print(descr, "length %llu", len);
        } else {
            reader.enterContainer();
            print(descr, "(indeterminate length)");
        }

        while (!reader.lastError() && reader.hasNext())
            dumpOneDetailed(nestingLevel + 1);

        if (!reader.lastError()) {
            reader.leaveContainer();
            print("Break", "");
        }
        break;
    }

    case QCborStreamReader::Tag: {
        QCborTag tag = reader.toTag();
        reader.next();
        print("Tag", "%llu%s", quint64(tag), tagDescription(tag));
        dumpOneDetailed(nestingLevel + 1);
        break;
    }

    case QCborStreamReader::SimpleType: {
        QCborSimpleType st = reader.toSimpleType();
        reader.next();
        switch (st) {
        case QCborSimpleType::False:
            print("Simple Type", "false");
            break;
        case QCborSimpleType::True:
            print("Simple Type", "true");
            break;
        case QCborSimpleType::Null:
            print("Simple Type", "null");
            break;
        case QCborSimpleType::Undefined:
            print("Simple Type", "undefined");
            break;
        default:
            print("Simple Type", "%u", quint8(st));
            break;
        }
        break;
    }

    case QCborStreamReader::Float16: {
        double d = reader.toFloat16();
        reader.next();
        printFp("Float16", d);
        break;
    }
    case QCborStreamReader::Float: {
        double d = reader.toFloat();
        reader.next();
        printFp("Float", d);
        break;
    }
    case QCborStreamReader::Double: {
        double d = reader.toDouble();
        reader.next();
        printFp("Double", d);
        break;
    }
    case QCborStreamReader::Invalid:
        return;
    }

    offset = reader.currentOffset();
}

void CborDumper::printByteArray(const QByteArray &ba)
{
    switch (byteArrayEncoding.top()) {
    default:
        printf("h'%s'", ba.toHex(' ').constData());
        break;

    case quint8(QCborKnownTags::ExpectedBase64):
        printf("b64'%s'", ba.toBase64().constData());
        break;

    case quint8(QCborKnownTags::ExpectedBase64url):
        printf("b64'%s'",
               ba.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
                       .constData());
        break;
    }
}

void printIndicator(quint64 value, qint64 previousOffset, qint64 offset, char space)
{
    int normalSize = cborNumberSize(value);
    int actualSize = offset - previousOffset;

    if (actualSize != normalSize) {
        Q_ASSERT(actualSize > 1);
        actualSize -= 2;
        printf("_%d", qPopulationCount(uint(actualSize)));
        if (space)
            printf("%c", space);
    }
}

void CborDumper::printWidthIndicator(quint64 value, char space)
{
    qint64 previousOffset = offset;
    offset = reader.currentOffset();
    if (opts & ShowWidthIndicators)
        printIndicator(value, previousOffset, offset, space);
}

void CborDumper::printStringWidthIndicator(quint64 value)
{
    qint64 previousOffset = offset;
    offset = reader.currentOffset();
    if (opts & ShowWidthIndicators)
        printIndicator(value, previousOffset, offset - uint(value), '\0');
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    setlocale(LC_ALL, "C");

    QCommandLineParser parser;
    parser.setApplicationDescription("CBOR Dumper tool"_L1);
    parser.addHelpOption();

    QCommandLineOption compact({"c"_L1, "compact"_L1}, "Use compact form (no line breaks)"_L1);
    parser.addOption(compact);

    QCommandLineOption showIndicators({ "i"_L1, "indicators"_L1 },
                                      "Show indicators for width of lengths and integrals"_L1);
    parser.addOption(showIndicators);

    QCommandLineOption verbose({"a"_L1, "annotated"_L1}, "Show bytes and annotated decoding"_L1);
    parser.addOption(verbose);

    parser.addPositionalArgument("[source]"_L1, "CBOR file to read from"_L1);

    parser.process(app);

    CborDumper::DumpOptions opts;
    if (parser.isSet(compact))
        opts |= CborDumper::ShowCompact;
    if (parser.isSet(showIndicators))
        opts |= CborDumper::ShowWidthIndicators;
    if (parser.isSet(verbose))
        opts |= CborDumper::ShowAnnotated;

    QStringList files = parser.positionalArguments();
    if (files.isEmpty())
        files << "-";
    for (const QString &file : std::as_const(files)) {
        QFile f(file);
        if (file == "-" ? f.open(stdin, QIODevice::ReadOnly) : f.open(QIODevice::ReadOnly)) {
            if (files.size() > 1)
                printf("/ From \"%s\" /\n", qPrintable(file));

            CborDumper dumper(&f, opts);
            QCborError err = dumper.dump();
            if (err)
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
