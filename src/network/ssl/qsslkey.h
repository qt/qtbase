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


#ifndef QSSLKEY_H
#define QSSLKEY_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qsharedpointer.h>
#include <QtNetwork/qssl.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_SSL

template <typename A, typename B> struct QPair;

class QIODevice;

class QSslKeyPrivate;
class Q_NETWORK_EXPORT QSslKey
{
public:
    QSslKey();
    QSslKey(const QByteArray &encoded, QSsl::KeyAlgorithm algorithm,
            QSsl::EncodingFormat format = QSsl::Pem,
            QSsl::KeyType type = QSsl::PrivateKey,
            const QByteArray &passPhrase = QByteArray());
    QSslKey(QIODevice *device, QSsl::KeyAlgorithm algorithm,
            QSsl::EncodingFormat format = QSsl::Pem,
            QSsl::KeyType type = QSsl::PrivateKey,
            const QByteArray &passPhrase = QByteArray());
    explicit QSslKey(Qt::HANDLE handle, QSsl::KeyType type = QSsl::PrivateKey);
    QSslKey(const QSslKey &other);
    QSslKey(QSslKey &&other) noexcept;
    QSslKey &operator=(QSslKey &&other) noexcept;
    QSslKey &operator=(const QSslKey &other);
    ~QSslKey();

    void swap(QSslKey &other) noexcept { qSwap(d, other.d); }

    bool isNull() const;
    void clear();

    int length() const;
    QSsl::KeyType type() const;
    QSsl::KeyAlgorithm algorithm() const;

    QByteArray toPem(const QByteArray &passPhrase = QByteArray()) const;
    QByteArray toDer(const QByteArray &passPhrase = QByteArray()) const;

    Qt::HANDLE handle() const;

    bool operator==(const QSslKey &key) const;
    inline bool operator!=(const QSslKey &key) const { return !operator==(key); }

private:
    QExplicitlySharedDataPointer<QSslKeyPrivate> d;
    friend class QSslCertificate;
    friend class QSslSocketBackendPrivate;
};

Q_DECLARE_SHARED(QSslKey)

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QSslKey &key);
#endif

#endif // QT_NO_SSL

QT_END_NAMESPACE

#endif
