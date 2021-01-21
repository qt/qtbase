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

#ifndef QNETWORKCOOKIE_H
#define QNETWORKCOOKIE_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE


class QByteArray;
class QDateTime;
class QString;
class QUrl;

class QNetworkCookiePrivate;
class Q_NETWORK_EXPORT QNetworkCookie
{
public:
    enum RawForm {
        NameAndValueOnly,
        Full
    };

    explicit QNetworkCookie(const QByteArray &name = QByteArray(), const QByteArray &value = QByteArray());
    QNetworkCookie(const QNetworkCookie &other);
    ~QNetworkCookie();
    QNetworkCookie &operator=(QNetworkCookie &&other) noexcept { swap(other); return *this; }
    QNetworkCookie &operator=(const QNetworkCookie &other);

    void swap(QNetworkCookie &other) noexcept { qSwap(d, other.d); }

    bool operator==(const QNetworkCookie &other) const;
    inline bool operator!=(const QNetworkCookie &other) const
    { return !(*this == other); }

    bool isSecure() const;
    void setSecure(bool enable);
    bool isHttpOnly() const;
    void setHttpOnly(bool enable);

    bool isSessionCookie() const;
    QDateTime expirationDate() const;
    void setExpirationDate(const QDateTime &date);

    QString domain() const;
    void setDomain(const QString &domain);

    QString path() const;
    void setPath(const QString &path);

    QByteArray name() const;
    void setName(const QByteArray &cookieName);

    QByteArray value() const;
    void setValue(const QByteArray &value);

    QByteArray toRawForm(RawForm form = Full) const;

    bool hasSameIdentifier(const QNetworkCookie &other) const;
    void normalize(const QUrl &url);

    static QList<QNetworkCookie> parseCookies(const QByteArray &cookieString);

private:
    QSharedDataPointer<QNetworkCookiePrivate> d;
    friend class QNetworkCookiePrivate;
};

Q_DECLARE_SHARED(QNetworkCookie)

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug, const QNetworkCookie &);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QNetworkCookie)

#endif
