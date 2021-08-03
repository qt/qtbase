/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qhttpheaderparser_p.h"

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

bool QHttpHeaderParser::parseHeaders(QByteArrayView header)
{
    // see rfc2616, sec 4 for information about HTTP/1.1 headers.
    // allows relaxed parsing here, accepts both CRLF & LF line endings
    int i = 0;
    while (i < header.size()) {
        int j = header.indexOf(':', i); // field-name
        if (j == -1)
            break;
        QByteArrayView field = header.sliced(i, j - i).trimmed();
        j++;
        // any number of LWS is allowed before and after the value
        QByteArray value;
        do {
            i = header.indexOf('\n', j);
            if (i == -1)
                break;
            if (!value.isEmpty())
                value += ' ';
            // check if we have CRLF or only LF
            bool hasCR = i && header[i - 1] == '\r';
            int length = i - (hasCR ? 1: 0) - j;
            value += header.sliced(j, length).trimmed();
            j = ++i;
        } while (i < header.size() && (header.at(i) == ' ' || header.at(i) == '\t'));
        if (i == -1)
            return false; // something is wrong

        fields.append(qMakePair(field.toByteArray(), value));
    }
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

    if (status.length() < minLength
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
    int j = status.indexOf(' ', i + 1);
    const QByteArrayView code = j > i ? status.sliced(i + 1, j - i - 1)
                                      : status.sliced(i + 1);

    bool ok;
    statusCode = code.toInt(&ok);

    reasonPhrase = j > i ? QString::fromLatin1(status.sliced(j + 1))
                         : QString();

    return ok && uint(majorVersion) <= 9 && uint(minorVersion) <= 9;
}

const QList<QPair<QByteArray, QByteArray> >& QHttpHeaderParser::headers() const
{
    return fields;
}

QByteArray QHttpHeaderParser::firstHeaderField(const QByteArray &name,
                                               const QByteArray &defaultValue) const
{
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        if (name.compare(it->first, Qt::CaseInsensitive) == 0)
            return it->second;
    }
    return defaultValue;
}

QByteArray QHttpHeaderParser::combinedHeaderValue(const QByteArray &name, const QByteArray &defaultValue) const
{
    const QList<QByteArray> allValues = headerFieldValues(name);
    if (allValues.isEmpty())
        return defaultValue;
    else
        return allValues.join(", ");
}

QList<QByteArray> QHttpHeaderParser::headerFieldValues(const QByteArray &name) const
{
    QList<QByteArray> result;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it)
        if (name.compare(it->first, Qt::CaseInsensitive) == 0)
            result += it->second;

    return result;
}

void QHttpHeaderParser::removeHeaderField(const QByteArray &name)
{
    auto firstEqualsName = [&name](const QPair<QByteArray, QByteArray> &header) {
        return name.compare(header.first, Qt::CaseInsensitive) == 0;
    };
    fields.removeIf(firstEqualsName);
}

void QHttpHeaderParser::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    removeHeaderField(name);
    fields.append(qMakePair(name, data));
}

void QHttpHeaderParser::prependHeaderField(const QByteArray &name, const QByteArray &data)
{
    fields.prepend(qMakePair(name, data));
}

void QHttpHeaderParser::appendHeaderField(const QByteArray &name, const QByteArray &data)
{
    fields.append(qMakePair(name, data));
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
