// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qhttpheaderparser_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

QHttpHeaderParser::QHttpHeaderParser()
    : statusCode(100) // Required by tst_QHttpNetworkConnection::ignoresslerror(failure)
    , majorVersion(0)
    , minorVersion(0)
{
}

void QHttpHeaderParser::clear()
{
    statusCode = 100;
    majorVersion = 0;
    minorVersion = 0;
    reasonPhrase.clear();
    fields.clear();
}

static bool fieldNameCheck(QByteArrayView name)
{
    const auto fieldNameChar = [](char c) {
        constexpr QByteArrayView otherCharacters("!#$%&'*+-.^_`|~");
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')
                || otherCharacters.contains(c);
    };

    return !name.empty() && std::all_of(name.begin(), name.end(), fieldNameChar);
}

bool QHttpHeaderParser::parseHeaders(QByteArrayView header)
{
    // see rfc2616, sec 4 for information about HTTP/1.1 headers.
    // allows relaxed parsing here, accepts both CRLF & LF line endings
    Q_ASSERT(fields.isEmpty());
    const auto hSpaceStart = [](QByteArrayView h) {
        return h.startsWith(' ') || h.startsWith('\t');
    };
    // Headers, if non-empty, start with a non-space and end with a newline:
    if (hSpaceStart(header) || (!header.empty() && !header.endsWith('\n')))
        return false;

    while (int tail = header.endsWith("\n\r\n") ? 2 : header.endsWith("\n\n") ? 1 : 0)
        header.chop(tail);

    if (header.size() - (header.endsWith("\r\n") ? 2 : 1) > maxTotalSize)
        return false;

    QHttpHeaders result;
    while (!header.empty()) {
        const qsizetype colon = header.indexOf(':');
        if (colon == -1) // if no colon check if empty headers
            return result.isEmpty() && (header == "\n" || header == "\r\n");
        if (result.size() >= maxFieldCount)
            return false;
        QByteArrayView name = header.first(colon);
        if (!fieldNameCheck(name))
            return false;
        header = header.sliced(colon + 1);
        QByteArray value;
        qsizetype valueSpace = maxFieldSize - name.size() - 1;
        do {
            const qsizetype endLine = header.indexOf('\n');
            Q_ASSERT(endLine != -1);
            auto line = header.first(endLine); // includes space
            valueSpace -= line.size() - (line.endsWith('\r') ? 1 : 0);
            if (valueSpace < 0)
                return false;
            line = line.trimmed();
            if (!line.empty()) {
                if (value.size())
                    value += ' ' + line;
                else
                    value = line.toByteArray();
            }
            header = header.sliced(endLine + 1);
        } while (hSpaceStart(header));
        Q_ASSERT(name.size() + 1 + value.size() <= maxFieldSize);
        result.append(name, value);
    }

    fields = result;
    return true;
}

bool QHttpHeaderParser::parseStatus(QByteArrayView status)
{
    // from RFC 2616:
    //        Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    //        HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
    // that makes: 'HTTP/n.n xxx Message'
    // byte count:  0123456789012

    static const int minLength = 11;
    static const int dotPos = 6;
    static const int spacePos = 8;
    static const char httpMagic[] = "HTTP/";

    if (status.size() < minLength
        || !status.startsWith(httpMagic)
        || status.at(dotPos) != '.'
        || status.at(spacePos) != ' ') {
        // I don't know how to parse this status line
        return false;
    }

    // optimize for the valid case: defer checking until the end
    majorVersion = status.at(dotPos - 1) - '0';
    minorVersion = status.at(dotPos + 1) - '0';

    int i = spacePos;
    qsizetype j = status.indexOf(' ', i + 1);
    const QByteArrayView code = j > i ? status.sliced(i + 1, j - i - 1)
                                      : status.sliced(i + 1);

    bool ok = false;
    statusCode = code.toInt(&ok);

    reasonPhrase = j > i ? QString::fromLatin1(status.sliced(j + 1))
                         : QString();

    return ok && uint(majorVersion) <= 9 && uint(minorVersion) <= 9;
}

const QHttpHeaders& QHttpHeaderParser::headers() const &
{
    return fields;
}

QHttpHeaders QHttpHeaderParser::headers() &&
{
    return std::move(fields);
}

QByteArray QHttpHeaderParser::firstHeaderField(QByteArrayView name,
                                               const QByteArray &defaultValue) const
{
    return fields.value(name, defaultValue).toByteArray();
}

QByteArray QHttpHeaderParser::combinedHeaderValue(QByteArrayView name, const QByteArray &defaultValue) const
{
    const QList<QByteArray> allValues = headerFieldValues(name);
    if (allValues.isEmpty())
        return defaultValue;
    return allValues.join(", ");
}

QList<QByteArray> QHttpHeaderParser::headerFieldValues(QByteArrayView name) const
{
    return fields.values(name);
}

void QHttpHeaderParser::removeHeaderField(QByteArrayView name)
{
    fields.removeAll(name);
}

void QHttpHeaderParser::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    removeHeaderField(name);
    fields.append(name, data);
}

void QHttpHeaderParser::prependHeaderField(const QByteArray &name, const QByteArray &data)
{
    fields.insert(0, name, data);
}

void QHttpHeaderParser::appendHeaderField(const QByteArray &name, const QByteArray &data)
{
    fields.append(name, data);
}

void QHttpHeaderParser::clearHeaders()
{
    fields.clear();
}

int QHttpHeaderParser::getStatusCode() const
{
    return statusCode;
}

void QHttpHeaderParser::setStatusCode(int code)
{
    statusCode = code;
}

int QHttpHeaderParser::getMajorVersion() const
{
    return majorVersion;
}

void QHttpHeaderParser::setMajorVersion(int version)
{
    majorVersion = version;
}

int QHttpHeaderParser::getMinorVersion() const
{
    return minorVersion;
}

void QHttpHeaderParser::setMinorVersion(int version)
{
    minorVersion = version;
}

QString QHttpHeaderParser::getReasonPhrase() const
{
    return reasonPhrase;
}

void QHttpHeaderParser::setReasonPhrase(const QString &reason)
{
    reasonPhrase = reason;
}

QT_END_NAMESPACE
