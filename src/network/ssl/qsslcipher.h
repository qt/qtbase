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


#ifndef QSSLCIPHER_H
#define QSSLCIPHER_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qssl.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_SSL

class QSslCipherPrivate;
class Q_NETWORK_EXPORT QSslCipher
{
public:
    QSslCipher();
    explicit QSslCipher(const QString &name);
    QSslCipher(const QString &name, QSsl::SslProtocol protocol);
    QSslCipher(const QSslCipher &other);
    QSslCipher &operator=(QSslCipher &&other) noexcept { swap(other); return *this; }
    QSslCipher &operator=(const QSslCipher &other);
    ~QSslCipher();

    void swap(QSslCipher &other) noexcept
    { qSwap(d, other.d); }

    bool operator==(const QSslCipher &other) const;
    inline bool operator!=(const QSslCipher &other) const { return !operator==(other); }

    bool isNull() const;
    QString name() const;
    int supportedBits() const;
    int usedBits() const;

    QString keyExchangeMethod() const;
    QString authenticationMethod() const;
    QString encryptionMethod() const;
    QString protocolString() const;
    QSsl::SslProtocol protocol() const;

private:
    QScopedPointer<QSslCipherPrivate> d;
    friend class QSslSocketBackendPrivate;
};

Q_DECLARE_SHARED(QSslCipher)

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QSslCipher &cipher);
#endif

#endif // QT_NO_SSL

QT_END_NAMESPACE

#endif

