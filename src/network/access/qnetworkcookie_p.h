/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QNETWORKCOOKIE_P_H
#define QNETWORKCOOKIE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access framework.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtCore/qdatetime.h"
#include "QtNetwork/qnetworkcookie.h"

QT_BEGIN_NAMESPACE

class QNetworkCookiePrivate: public QSharedData
{
public:
    QNetworkCookiePrivate() = default;
    static QList<QNetworkCookie> parseSetCookieHeaderLine(const QByteArray &cookieString);

    QDateTime expirationDate;
    QString domain;
    QString path;
    QString comment;
    QByteArray name;
    QByteArray value;
    QNetworkCookie::SameSite sameSite = QNetworkCookie::SameSite::Default;
    bool secure = false;
    bool httpOnly = false;
};

static inline bool isLWS(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int nextNonWhitespace(const QByteArray &text, int from)
{
    // RFC 2616 defines linear whitespace as:
    //  LWS = [CRLF] 1*( SP | HT )
    // We ignore the fact that CRLF must come as a pair at this point
    // It's an invalid HTTP header if that happens.
    while (from < text.length()) {
        if (isLWS(text.at(from)))
            ++from;
        else
            return from;        // non-whitespace
    }

    // reached the end
    return text.length();
}

QT_END_NAMESPACE

#endif
