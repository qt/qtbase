/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
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

#ifndef QSSLPRESHAREDKEYAUTHENTICATOR_H
#define QSSLPRESHAREDKEYAUTHENTICATOR_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QMetaType>

QT_REQUIRE_CONFIG(ssl);

QT_BEGIN_NAMESPACE

class QSslPreSharedKeyAuthenticatorPrivate;

class QSslPreSharedKeyAuthenticator
{
public:
    Q_NETWORK_EXPORT QSslPreSharedKeyAuthenticator();
    Q_NETWORK_EXPORT ~QSslPreSharedKeyAuthenticator();
    Q_NETWORK_EXPORT QSslPreSharedKeyAuthenticator(const QSslPreSharedKeyAuthenticator &authenticator);
    Q_NETWORK_EXPORT QSslPreSharedKeyAuthenticator &operator=(const QSslPreSharedKeyAuthenticator &authenticator);

    QSslPreSharedKeyAuthenticator &operator=(QSslPreSharedKeyAuthenticator &&other) noexcept { swap(other); return *this; }

    void swap(QSslPreSharedKeyAuthenticator &other) noexcept { qSwap(d, other.d); }

    Q_NETWORK_EXPORT QByteArray identityHint() const;

    Q_NETWORK_EXPORT void setIdentity(const QByteArray &identity);
    Q_NETWORK_EXPORT QByteArray identity() const;
    Q_NETWORK_EXPORT int maximumIdentityLength() const;

    Q_NETWORK_EXPORT void setPreSharedKey(const QByteArray &preSharedKey);
    Q_NETWORK_EXPORT QByteArray preSharedKey() const;
    Q_NETWORK_EXPORT int maximumPreSharedKeyLength() const;

private:
    friend Q_NETWORK_EXPORT bool operator==(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs);
    friend class QSslSocketBackendPrivate;
    friend class QDtlsPrivateOpenSSL;

    QSharedDataPointer<QSslPreSharedKeyAuthenticatorPrivate> d;
};

inline bool operator!=(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs)
{
    return !operator==(lhs, rhs);
}

Q_DECLARE_SHARED(QSslPreSharedKeyAuthenticator)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSslPreSharedKeyAuthenticator)
Q_DECLARE_METATYPE(QSslPreSharedKeyAuthenticator*)

#endif // QSSLPRESHAREDKEYAUTHENTICATOR_H
