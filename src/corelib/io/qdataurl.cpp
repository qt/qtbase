/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qplatformdefs.h"
#include "qurl.h"
#include "private/qdataurl_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    Decode a data: URL into its mimetype and payload. Returns a null string if
    the URL could not be decoded.
*/
Q_CORE_EXPORT bool qDecodeDataUrl(const QUrl &uri, QString &mimeType, QByteArray &payload)
{
    if (uri.scheme() != QLatin1String("data") || !uri.host().isEmpty())
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
        if (QLatin1String{data}.endsWith(QLatin1String(";base64"), Qt::CaseInsensitive)) {
            payload = QByteArray::fromBase64(payload);
            data.chop(7);
        }

        if (QLatin1String{data}.startsWith(QLatin1String("charset"), Qt::CaseInsensitive)) {
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
