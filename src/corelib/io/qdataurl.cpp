// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "qurl.h"
#include "private/qdataurl_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

/*!
    \internal

    Decode a data: URL into its mimetype and payload. Returns a null string if
    the URL could not be decoded.
*/
Q_CORE_EXPORT bool qDecodeDataUrl(const QUrl &uri, QString &mimeType, QByteArray &payload)
{
    if (uri.scheme() != "data"_L1 || !uri.host().isEmpty())
        return false;

    mimeType = QStringLiteral("text/plain;charset=US-ASCII");

    // the following would have been the correct thing, but
    // reality often differs from the specification. People have
    // data: URIs with ? and #
    //QByteArray data = QByteArray::fromPercentEncoding(uri.path(QUrl::FullyEncoded).toLatin1());
    QByteArray data = QByteArray::fromPercentEncoding(uri.url(QUrl::FullyEncoded | QUrl::RemoveScheme).toLatin1());

    // parse it:
    const qsizetype pos = data.indexOf(',');
    if (pos != -1) {
        payload = data.mid(pos + 1);
        data.truncate(pos);
        data = data.trimmed();

        // find out if the payload is encoded in Base64
        if (QLatin1StringView{data}.endsWith(";base64"_L1, Qt::CaseInsensitive)) {
            payload = QByteArray::fromBase64(payload);
            data.chop(7);
        }

        if (QLatin1StringView{data}.startsWith("charset"_L1, Qt::CaseInsensitive)) {
            qsizetype i = 7;      // strlen("charset")
            while (data.at(i) == ' ')
                ++i;
            if (data.at(i) == '=')
                data.prepend("text/plain;");
        }

        if (!data.isEmpty())
            mimeType = QString::fromLatin1(data.trimmed());

    }

    return true;
}

QT_END_NAMESPACE
