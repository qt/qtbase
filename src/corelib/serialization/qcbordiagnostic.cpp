// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qcborvalue.h"
#include "qcborvalue_p.h"

#include "qcborarray.h"
#include "qcbormap.h"

#include <private/qnumeric_p.h>
#include <qstack.h>
#include <private/qtools_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace  {
class DiagnosticNotation
{
public:
    static QString create(const QCborValue &v, QCborValue::DiagnosticNotationOptions opts)
    {
        DiagnosticNotation dn(opts);
        dn.appendValue(v);
        return dn.result;
    }

private:
    QStack<int> byteArrayFormatStack;
    QString separator;
    QString result;
    QCborValue::DiagnosticNotationOptions opts;
    int nestingLevel = 0;

    struct Nest {
        enum { IndentationWidth = 4 };
        DiagnosticNotation *dn;
        Nest(DiagnosticNotation *that) : dn(that)
        {
            ++dn->nestingLevel;
            static const char indent[IndentationWidth + 1] = "    ";
            if (dn->opts & QCborValue::LineWrapped)
                dn->separator += QLatin1StringView(indent, IndentationWidth);
        }
        ~Nest()
        {
            --dn->nestingLevel;
            if (dn->opts & QCborValue::LineWrapped)
                dn->separator.chop(IndentationWidth);
        }
    };

    DiagnosticNotation(QCborValue::DiagnosticNotationOptions opts_)
        : separator(opts_ & QCborValue::LineWrapped ? "\n"_L1 : ""_L1), opts(opts_)
    {
        byteArrayFormatStack.push(int(QCborKnownTags::ExpectedBase16));
    }

    void appendString(const QString &s);
    void appendArray(const QCborArray &a);
    void appendMap(const QCborMap &m);
    void appendValue(const QCborValue &v);
};
}

static QString makeFpString(double d)
{
    QString s;
    quint64 v;
    if (qt_is_inf(d)) {
        s = (d < 0) ? QStringLiteral("-inf") : QStringLiteral("inf");
    } else if (qt_is_nan(d)) {
        s = QStringLiteral("nan");
    } else if (convertDoubleTo(d, &v)) {
        s = QString::fromLatin1("%1.0").arg(v);
        if (d < 0)
            s.prepend(u'-');
    } else {
        s = QString::number(d, 'g', QLocale::FloatingPointShortest);
        if (!s.contains(u'.') && !s.contains(u'e'))
            s += u'.';
    }
    return s;
}

static bool isByteArrayEncodingTag(QCborTag tag)
{
    switch (quint64(tag)) {
    case quint64(QCborKnownTags::ExpectedBase16):
    case quint64(QCborKnownTags::ExpectedBase64):
    case quint64(QCborKnownTags::ExpectedBase64url):
        return true;
    }
    return false;
}

void DiagnosticNotation::appendString(const QString &s)
{
    result += u'"';

    const QChar *begin = s.begin();
    const QChar *end = s.end();
    while (begin < end) {
        // find the longest span comprising only non-escaped characters
        const QChar *ptr = begin;
        for ( ; ptr < end; ++ptr) {
            ushort uc = ptr->unicode();
            if (uc == '\\' || uc == '"' || uc < ' ' || uc >= 0x7f)
                break;
        }

        if (ptr != begin)
            result.append(begin, ptr - begin);

        if (ptr == end)
            break;

        // there's an escaped character
        static const char escapeMap[16] = {
            // The C escape characters \a \b \t \n \v \f and \r indexed by
            // their ASCII values
            0, 0, 0, 0,
            0, 0, 0, 'a',
            'b', 't', 'n', 'v',
            'f', 'r', 0, 0
        };
        int buflen = 2;
        QChar buf[10];
        buf[0] = u'\\';
        buf[1] = QChar::Null;
        char16_t uc = ptr->unicode();

        if (uc < sizeof(escapeMap))
            buf[1] = QLatin1Char(escapeMap[uc]);
        else if (uc == '"' || uc == '\\')
            buf[1] = QChar(uc);

        if (buf[1] == QChar::Null) {
            const auto toHexUpper = [](char32_t value) -> QChar {
                // QtMiscUtils::toHexUpper() returns char, we need QChar, so wrap
                return char16_t(QtMiscUtils::toHexUpper(value));
            };
            if (ptr->isHighSurrogate() && (ptr + 1) != end && ptr[1].isLowSurrogate()) {
                // properly-paired surrogates
                ++ptr;
                char32_t ucs4 = QChar::surrogateToUcs4(uc, ptr->unicode());
                buf[1] = u'U';
                buf[2] = u'0'; // toHexUpper(ucs4 >> 28);
                buf[3] = u'0'; // toHexUpper(ucs4 >> 24);
                buf[4] = toHexUpper(ucs4 >> 20);
                buf[5] = toHexUpper(ucs4 >> 16);
                buf[6] = toHexUpper(ucs4 >> 12);
                buf[7] = toHexUpper(ucs4 >> 8);
                buf[8] = toHexUpper(ucs4 >> 4);
                buf[9] = toHexUpper(ucs4);
                buflen = 10;
            } else {
                buf[1] = u'u';
                buf[2] = toHexUpper(uc >> 12);
                buf[3] = toHexUpper(uc >> 8);
                buf[4] = toHexUpper(uc >> 4);
                buf[5] = toHexUpper(uc);
                buflen = 6;
            }
        }

        result.append(buf, buflen);
        begin = ptr + 1;
    }

    result += u'"';
}

void DiagnosticNotation::appendArray(const QCborArray &a)
{
    result += u'[';

    // length 2 (including the space) when not line wrapping
    QLatin1StringView commaValue(", ", opts & QCborValue::LineWrapped ? 1 : 2);
    {
        Nest n(this);
        QLatin1StringView comma;
        for (auto v : a) {
            result += comma + separator;
            comma = commaValue;
            appendValue(v);
        }
    }

    result += separator + u']';
}

void DiagnosticNotation::appendMap(const QCborMap &m)
{
    result += u'{';

    // length 2 (including the space) when not line wrapping
    QLatin1StringView commaValue(", ", opts & QCborValue::LineWrapped ? 1 : 2);
    {
        Nest n(this);
        QLatin1StringView comma;
        for (auto v : m) {
            result += comma + separator;
            comma = commaValue;
            appendValue(v.first);
            result += ": "_L1;
            appendValue(v.second);
        }
    }

    result += separator + u'}';
};

void DiagnosticNotation::appendValue(const QCborValue &v)
{
    switch (v.type()) {
    case QCborValue::Integer:
        result += QString::number(v.toInteger());
        return;
    case QCborValue::ByteArray:
        switch (byteArrayFormatStack.top()) {
        case int(QCborKnownTags::ExpectedBase16):
            result += QString::fromLatin1("h'" +
                                          v.toByteArray().toHex(opts & QCborValue::ExtendedFormat ? ' ' : '\0') +
                                          '\'');
            return;
        case int(QCborKnownTags::ExpectedBase64):
            result += QString::fromLatin1("b64'" + v.toByteArray().toBase64() + '\'');
            return;
        default:
        case int(QCborKnownTags::ExpectedBase64url):
            result += QString::fromLatin1("b64'" +
                                          v.toByteArray().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals) +
                                          '\'');
            return;
        }
    case QCborValue::String:
        return appendString(v.toString());
    case QCborValue::Array:
        return appendArray(v.toArray());
    case QCborValue::Map:
        return appendMap(v.toMap());
    case QCborValue::False:
        result += "false"_L1;
        return;
    case QCborValue::True:
        result += "true"_L1;
        return;
    case QCborValue::Null:
        result += "null"_L1;
        return;
    case QCborValue::Undefined:
        result += "undefined"_L1;
        return;
    case QCborValue::Double:
        result += makeFpString(v.toDouble());
        return;
    case QCborValue::Invalid:
        result += QStringLiteral("<invalid>");
        return;

    default:
        // Only tags, extended types, and simple types remain; see below.
        break;
    }

    if (v.isTag()) {
        // We handle all extended types as regular tags, so it won't matter
        // whether we understand that tag or not.
        bool byteArrayFormat = opts & QCborValue::ExtendedFormat && isByteArrayEncodingTag(v.tag());
        if (byteArrayFormat)
            byteArrayFormatStack.push(int(v.tag()));
        result += QString::number(quint64(v.tag())) + u'(';
        appendValue(v.taggedValue());
        result += u')';
        if (byteArrayFormat)
            byteArrayFormatStack.pop();
    } else {
        // must be a simple type
        result += QString::fromLatin1("simple(%1)").arg(quint8(v.toSimpleType()));
    }
}

/*!
    Creates the diagnostic notation equivalent of this CBOR object and returns
    it. The \a opts parameter controls the dialect of the notation. Diagnostic
    notation is useful in debugging, to aid the developer in understanding what
    value is stored in the QCborValue or in a CBOR stream. For that reason, the
    Qt API provides no support for parsing the diagnostic back into the
    in-memory format or CBOR stream, though the representation is unique and it
    would be possible.

    CBOR diagnostic notation is specified by
    \l{RFC 7049, section 6}{section 6} of RFC 7049.
    It is a text representation of the CBOR stream and it is very similar to
    JSON, but it supports the CBOR types not found in JSON. The extended format
    enabled by the \l{DiagnosticNotationOption}{ExtendedFormat} flag is
    currently in some IETF drafts and its format is subject to change.

    This function produces the equivalent representation of the stream that
    toCbor() would produce, without any transformation option provided there.
    This also implies this function may not produce a representation of the
    stream that was used to create the object, if it was created using
    fromCbor(), as that function may have applied transformations. For a
    high-fidelity notation of a stream, without transformation, see the \c
    cbordump example.

    \sa toCbor(), QJsonDocument::toJson()
 */
QString QCborValue::toDiagnosticNotation(DiagnosticNotationOptions opts) const
{
    return DiagnosticNotation::create(*this, opts);
}

QT_END_NAMESPACE
