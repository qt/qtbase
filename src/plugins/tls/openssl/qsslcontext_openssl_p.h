// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QSSLCONTEXT_OPENSSL_P_H
#define QSSLCONTEXT_OPENSSL_P_H

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
#include <QtCore/qvariant.h>
#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qsslconfiguration.h>
#include <openssl/ssl.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SSL

class QSslContext
{
public:

    ~QSslContext();

    static std::shared_ptr<QSslContext> sharedFromConfiguration(QSslSocket::SslMode mode, const QSslConfiguration &configuration,
                                                               bool allowRootCertOnDemandLoading);
    static std::shared_ptr<QSslContext> sharedFromPrivateConfiguration(QSslSocket::SslMode mode, QSslConfigurationPrivate *privConfiguration,
                                                                      bool allowRootCertOnDemandLoading);

    static qssloptions setupOpenSslOptions(QSsl::SslProtocol protocol, QSsl::SslOptions sslOptions);

    QSslError::SslError error() const;
    QString errorString() const;

    SSL* createSsl();
    bool cacheSession(SSL*); // should be called when handshake completed

    QByteArray sessionASN1() const;
    void setSessionASN1(const QByteArray &sessionASN1);
    int sessionTicketLifeTimeHint() const;

    static void forceAutoTestSecurityLevel();

#ifndef OPENSSL_NO_NEXTPROTONEG
    // must be public because we want to use it from an OpenSSL callback
    struct NPNContext {
        NPNContext() : data(nullptr),
            len(0),
            status(QSslConfiguration::NextProtocolNegotiationNone)
        { }
        unsigned char *data;
        unsigned short len;
        QSslConfiguration::NextProtocolNegotiationStatus status;
    };
    NPNContext npnContext() const;
#endif // !OPENSSL_NO_NEXTPROTONEG

protected:
    QSslContext();

private:
    static void initSslContext(QSslContext* sslContext, QSslSocket::SslMode mode, const QSslConfiguration &configuration,
                               bool allowRootCertOnDemandLoading);
    static void applyBackendConfig(QSslContext *sslContext);

private:
    SSL_CTX* ctx;
    EVP_PKEY *pkey;
    SSL_SESSION *session;
    QByteArray m_sessionASN1;
    int m_sessionTicketLifeTimeHint;
    QSslError::SslError errorCode;
    QString errorStr;
    QSslConfiguration sslConfiguration;
#ifndef OPENSSL_NO_NEXTPROTONEG
    QByteArray m_supportedNPNVersions;
    NPNContext m_npnContext;
#endif // !OPENSSL_NO_NEXTPROTONEG
};

#endif // QT_NO_SSL

QT_END_NAMESPACE

#endif // QSSLCONTEXT_OPENSSL_P_H
