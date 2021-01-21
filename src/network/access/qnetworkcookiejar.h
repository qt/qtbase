/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QNETWORKCOOKIEJAR_H
#define QNETWORKCOOKIEJAR_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QObject>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE


class QNetworkCookie;

class QNetworkCookieJarPrivate;
class Q_NETWORK_EXPORT QNetworkCookieJar: public QObject
{
    Q_OBJECT
public:
    explicit QNetworkCookieJar(QObject *parent = nullptr);
    virtual ~QNetworkCookieJar();

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);

    virtual bool insertCookie(const QNetworkCookie &cookie);
    virtual bool updateCookie(const QNetworkCookie &cookie);
    virtual bool deleteCookie(const QNetworkCookie &cookie);

protected:
    QList<QNetworkCookie> allCookies() const;
    void setAllCookies(const QList<QNetworkCookie> &cookieList);
    virtual bool validateCookie(const QNetworkCookie &cookie, const QUrl &url) const;

private:
    Q_DECLARE_PRIVATE(QNetworkCookieJar)
    Q_DISABLE_COPY(QNetworkCookieJar)
};

QT_END_NAMESPACE

#endif
