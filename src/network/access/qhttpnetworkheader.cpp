// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhttpnetworkheader_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

QHttpNetworkHeaderPrivate::QHttpNetworkHeaderPrivate(const QUrl &newUrl)
    :url(newUrl)
{
}

qint64 QHttpNetworkHeaderPrivate::contentLength() const
{
    bool ok = false;
    // We are not using the headerField() method here because servers might send us multiple content-length
    // headers which is crap (see QTBUG-15311). Therefore just take the first content-length header field.
    QByteArray value = parser.firstHeaderField("content-length");
    qint64 length = value.toULongLong(&ok);
    if (ok)
        return length;
    return -1; // the header field is not set
}

void QHttpNetworkHeaderPrivate::setContentLength(qint64 length)
{
    setHeaderField("Content-Length", QByteArray::number(length));
}

QByteArray QHttpNetworkHeaderPrivate::headerField(const QByteArray &name, const QByteArray &defaultValue) const
{
    QList<QByteArray> allValues = headerFieldValues(name);
    if (allValues.isEmpty())
        return defaultValue;
    else
        return allValues.join(", ");
}

QList<QByteArray> QHttpNetworkHeaderPrivate::headerFieldValues(const QByteArray &name) const
{
    return parser.headerFieldValues(name);
}

void QHttpNetworkHeaderPrivate::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    parser.setHeaderField(name, data);
}

void QHttpNetworkHeaderPrivate::prependHeaderField(const QByteArray &name, const QByteArray &data)
{
    parser.prependHeaderField(name, data);
}

QList<QPair<QByteArray, QByteArray> > QHttpNetworkHeaderPrivate::headers() const
{
    return parser.headers();
}

void QHttpNetworkHeaderPrivate::clearHeaders()
{
    parser.clearHeaders();
}

bool QHttpNetworkHeaderPrivate::operator==(const QHttpNetworkHeaderPrivate &other) const
{
   return (url == other.url);
}


QT_END_NAMESPACE
