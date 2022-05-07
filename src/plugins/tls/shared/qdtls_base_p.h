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

#ifndef QDTLS_BASE_P_H
#define QDTLS_BASE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

QT_REQUIRE_CONFIG(dtls);

#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qssl.h>

#include <QtNetwork/qhostaddress.h>

#include <QtCore/qcryptographichash.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

// This class exists to re-implement the shared error/cookie handling
// for both QDtls and QDtlsClientVerifier classes. Use it if/when
// you need it. Backend neutral.
class QDtlsBasePrivate : virtual public QTlsPrivate::DtlsBase
{
public:
    QDtlsBasePrivate(QSslSocket::SslMode m, const QByteArray &s) : mode(m), secret(s) {}
    void setDtlsError(QDtlsError code, const QString &description) override;
    QDtlsError error() const override;
    QString errorString() const override;
    void clearDtlsError() override;

    void setConfiguration(const QSslConfiguration &configuration) override;
    QSslConfiguration configuration() const override;

    bool setCookieGeneratorParameters(const GenParams &) override;
    GenParams cookieGeneratorParameters() const override;

    static bool isDtlsProtocol(QSsl::SslProtocol protocol);

    QHostAddress remoteAddress;
    quint16 remotePort = 0;
    quint16 mtuHint = 0;

    QDtlsError errorCode = QDtlsError::NoError;
    QString errorDescription;
    QSslConfiguration dtlsConfiguration;
    QSslSocket::SslMode mode = QSslSocket::SslClientMode;
    QSslCipher sessionCipher;
    QSsl::SslProtocol sessionProtocol = QSsl::UnknownProtocol;
    QString peerVfyName;
    QByteArray secret;

#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha1;
#else
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha256;
#endif
};

QT_END_NAMESPACE

#endif // QDTLS_BASE_P_H
