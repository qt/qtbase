// Copyright (c) 2023 LLC «V Kontakte»
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_NETWORK_BUILD_REMOVED_API

#include "qtnetworkglobal.h"

QT_USE_NAMESPACE

#if QT_NETWORK_REMOVED_SINCE(6, 7)

#include "qhostinfo.h"

// static
int QHostInfo::lookupHost(const QString &name, QObject *receiver, const char *member)
{
    const auto *r = receiver;
    return lookupHost(name, r, member);
}


#include "qnetworkreply.h"

QByteArray QNetworkReply::rawHeader(const QByteArray &headerName) const
{
    return rawHeader(qToByteArrayViewIgnoringNull(headerName));
}

bool QNetworkReply::hasRawHeader(const QByteArray &headerName) const
{
    return hasRawHeader(qToByteArrayViewIgnoringNull(headerName));
}

#include "qnetworkrequest.h"

QByteArray QNetworkRequest::rawHeader(const QByteArray &headerName) const
{
    return rawHeader(qToByteArrayViewIgnoringNull(headerName));
}

bool QNetworkRequest::hasRawHeader(const QByteArray &headerName) const
{
    return hasRawHeader(qToByteArrayViewIgnoringNull(headerName));
}

#include "qnetworkcookie.h"

QList<QNetworkCookie> QNetworkCookie::parseCookies(const QByteArray &cookieString)
{
    return parseCookies(qToByteArrayViewIgnoringNull(cookieString));
}

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically

#endif // QT_NETWORK_REMOVED_SINCE(6, 7)

#if QT_NETWORK_REMOVED_SINCE(6, 8)

#if QT_CONFIG(dnslookup)
#  include "qdnslookup.h" // inlined API
#endif
#include "qnetworkrequest.h" // inlined API

#include "qsslerror.h"

#ifndef QT_NO_DEBUG_STREAM
#if QT_CONFIG(ssl)

#include "qdebug.h"

QDebug operator<<(QDebug debug, const QSslError::SslError &error)
{
    return print(std::move(debug), error);
}
#endif
#endif

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically

#endif // QT_NETWORK_REMOVED_SINCE(6, 8)
