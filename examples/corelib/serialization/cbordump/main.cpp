/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

/*
 * To regenerate:
 *  curl -O https://www.iana.org/assignments/cbor-tags/cbor-tags.xml
 *  xsltproc tag-transform.xslt cbor-tags.xml
 */

// GENERATED CODE
struct CborTagDescription
{
    QCborTag tag;
    const char *description;    // with space and parentheses
};

// CBOR Tags
static const CborTagDescription tagDescriptions[] = {
    // from https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
    { QCborTag(0), " (Standard date/time string; see Section 2.4.1 [RFC7049])" },
    { QCborTag(1), " (Epoch-based date/time; see Section 2.4.1 [RFC7049])" },
    { QCborTag(2), " (Positive bignum; see Section 2.4.2 [RFC7049])" },
    { QCborTag(3), " (Negative bignum; see Section 2.4.2 [RFC7049])" },
    { QCborTag(4), " (Decimal fraction; see Section 2.4.3 [RFC7049])" },
    { QCborTag(5), " (Bigfloat; see Section 2.4.3 [RFC7049])" },
    { QCborTag(16), " (COSE Single Recipient Encrypted Data Object [RFC8152])" },
    { QCborTag(17), " (COSE Mac w/o Recipients Object [RFC8152])" },
    { QCborTag(18), " (COSE Single Signer Data Object [RFC8152])" },
    { QCborTag(21), " (Expected conversion to base64url encoding; see Section 2.4.4.2 [RFC7049])" },
    { QCborTag(22), " (Expected conversion to base64 encoding; see Section 2.4.4.2 [RFC7049])" },
    { QCborTag(23), " (Expected conversion to base16 encoding; see Section 2.4.4.2 [RFC7049])" },
    { QCborTag(24), " (Encoded CBOR data item; see Section 2.4.4.1 [RFC7049])" },
    { QCborTag(25), " (reference the nth previously seen string)" },
    { QCborTag(26), " (Serialised Perl object with classname and constructor arguments)" },
    { QCborTag(27), " (Serialised language-independent object with type name and constructor arguments)" },
    { QCborTag(28), " (mark value as (potentially) shared)" },
    { QCborTag(29), " (reference nth marked value)" },
    { QCborTag(30), " (Rational number)" },
    { QCborTag(32), " (URI; see Section 2.4.4.3 [RFC7049])" },
    { QCborTag(33), " (base64url; see Section 2.4.4.3 [RFC7049])" },
    { QCborTag(34), " (base64; see Section 2.4.4.3 [RFC7049])" },
    { QCborTag(35), " (Regular expression; see Section 2.4.4.3 [RFC7049])" },
    { QCborTag(36), " (MIME message; see Section 2.4.4.3 [RFC7049])" },
    { QCborTag(37), " (Binary UUID ( section 4.1.2))" },
    { QCborTag(38), " (Language-tagged string)" },
    { QCborTag(39), " (Identifier)" },
    { QCborTag(61), " (CBOR Web Token (CWT))" },
    { QCborTag(96), " (COSE Encrypted Data Object [RFC8152])" },
    { QCborTag(97), " (COSE MACed Data Object [RFC8152])" },
    { QCborTag(98), " (COSE Signed Data Object [RFC8152])" },
    { QCborTag(256), " (mark value as having string references)" },
    { QCborTag(257), " (Binary MIME message)" },
    { QCborTag(258), " (Mathematical finite set)" },
    { QCborTag(260), " (Network Address (IPv4 or IPv6 or MAC Address))" },
    { QCborTag(264), " (Decimal fraction with arbitrary exponent)" },
    { QCborTag(265), " (Bigfloat with arbitrary exponent)" },
    { QCborTag(1001), " (extended time)" },
    { QCborTag(1002), " (duration)" },
    { QCborTag(1003), " (period)" },
    { QCborTag(22098), " (hint that indicates an additional level of indirection)" },
    { QCborTag(55799), " (Self-describe CBOR; see Section 2.4.5 [RFC7049])" },
    { QCborTag(15309736), " (RAINS Message)" },
    { QCborTag(-1), nullptr }
};
// END GENERATED CODE

enum {
    // See RFC 7049 section 2.
    SmallValueBitLength     = 5,
    SmallValueMask          = (1 << SmallValueBitLength) - 1,      /* 0x1f */
    Value8Bit               = 24,
    Value16Bit              = 25,
    Value32Bit              = 26,
    Value64Bit              = 27
};

struct CborDumper
{
    enum DumpOption {
        ShowCompact             = 0x01,
        ShowWidthIndicators     = 0x02,
        ShowAnnotated           = 0x04
    };
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

CborDumper::CborDumper(QFile *f, DumpOptions opts_)
    : opts(opts_)
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

template <typename T> static inline bool canConvertTo(double v)
{
    // The [conv.fpint] (7.10 Floating-integral conversions) section of the
    // standard says only exact conversions are guaranteed. Converting
    // integrals to floating-point with loss of precision has implementation-
    // defined behavior whether the next higher or next lower is returned;
    // converting FP to integral is UB if it can't be represented.;
    Q_STATIC_ASSERT(std::numeric_limits<T>::is_integer);

    double supremum = ldexp(1, std::numeric_limits<T>::digits);
    if (v >= supremum)
        return false;

    if (v < std::numeric_limits<T>::min()) // either zero or a power of two, so it's exact
        return false;

    // we're in range
    return v == floor(v);
}

static QString fpToString(double v, const char *suffix)
{
    if (qIsInf(v))
        return v < 0 ? QStringLiteral("-inf") : QStringLiteral("inf");
    if (qIsNaN(v))
        return QStringLiteral("nan");
    if (canConvertTo<qint64>(v))
        return QString::number(qint64(v)) + ".0" + suffix;
    if (canConvertTo<quint64>(v))
        return QString::number(quint64(v)) + ".0" + suffix;

    QString s = QString::number(v, 'g', QLocale::FloatingPointShortest);
    if (!s.contains('.') && !s.contains('e'))
        s += '.';
    s += suffix;
    return s;
};

void CborDumper::dumpOne(int nestingLevel)
{
    QString indent(1, QLatin1Char(' '));
    QString indented = indent;
    if (!opts.testFlag(ShowCompact)) {
        indent = QLatin1Char('\n') + QString(4 * nestingLevel, QLatin1Char(' '));
        indented = QLatin1Char('\n') + QString(4 + 4 * nestingLevel, QLatin1Char(' '));
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
                comma = QLatin1Char(',') + indented;
            }
        } else {
            auto r = reader.readString();
            while (r.status == QCborStreamReader::Ok) {
                printf("%s\"%s\"", qPrintable(comma), qPrintable(r.data));
                printStringWidthIndicator(r.data.toUtf8().size());

                r = reader.readString();
                comma = QLatin1Char(',') + indented;
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
            dumpOne(nestingLevel);  // same level!
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
        printf("%s", qPrintable(fpToString(reader.toFloat16(), "f16")));
        reader.next();
        break;
    case QCborStreamReader::Float:
        printf("%s", qPrintable(fpToString(reader.toFloat(), "f")));
        reader.next();
        break;
    case QCborStreamReader::Double:
        printf("%s", qPrintable(fpToString(reader.toDouble(), "")));
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
    auto print = [=](const char *descr, const char *fmt, ...) {
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
        QString s = fpToString(d, "");
        if (s.size() <= 6)
            return print(descr, "%s", qPrintable(s));
        return print(descr, "%a", d);
    };

    auto printString = [=](const char *descr) {
        QByteArray indent(nestingLevel * 2, ' ');
        const char *chunkStr = (reader.isLengthKnown() ? "" : "chunk ");
        int width = 48 - indent.size();
        int bytesPerLine = qMax(width / 3, 5);

        qsizetype size = reader.currentStringChunkSize();
        if (size < 0)
            return;         // error
        if (size >= std::numeric_limits<int>::max()) {
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
                QByteArray spaces(width > 0 ? width - section.size() * 3 + 1: 0, ' ');
                printf("%s # \"", spaces.constData());
                auto ptr = reinterpret_cast<const uchar *>(section.constData());
                for (int j = 0; j < section.size(); ++j)
                    printf("%c", ptr[j] >= 0x80 || ptr[j] < 0x20 ? '.' : ptr[j]);

                puts("\"");
            }

            // get the next chunk
            size = reader.currentStringChunkSize();
            if (size < 0)
                return;         // error
            if (size >= std::numeric_limits<int>::max()) {
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
        printf("b64'%s'", ba.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals).constData());
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
    parser.setApplicationDescription(QStringLiteral("CBOR Dumper tool"));
    parser.addHelpOption();

    QCommandLineOption compact({QStringLiteral("c"), QStringLiteral("compact")},
                               QStringLiteral("Use compact form (no line breaks)"));
    parser.addOption(compact);

    QCommandLineOption showIndicators({QStringLiteral("i"), QStringLiteral("indicators")},
                                      QStringLiteral("Show indicators for width of lengths and integrals"));
    parser.addOption(showIndicators);

    QCommandLineOption verbose({QStringLiteral("a"), QStringLiteral("annotated")},
                               QStringLiteral("Show bytes and annotated decoding"));
    parser.addOption(verbose);

    parser.addPositionalArgument(QStringLiteral("[source]"),
                                 QStringLiteral("CBOR file to read from"));

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
    for (const QString &file : qAsConst(files)) {
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
