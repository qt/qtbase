/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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

class QSslContextPrivate;

class QSslContext
{
public:

    ~QSslContext();

    static QSslContext* fromConfiguration(QSslSocket::SslMode mode, const QSslConfiguration &configuration,
                                          bool allowRootCertOnDemandLoading);
    static QSharedPointer<QSslContext> sharedFromConfiguration(QSslSocket::SslMode mode, const QSslConfiguration &configuration,
                                                               bool allowRootCertOnDemandLoading);

    QSslError::SslError error() const;
    QString errorString() const;

    SSL* createSsl();
    bool cacheSession(SSL*); // should be called when handshake completed

    QByteArray sessionASN1() const;
    void setSessionASN1(const QByteArray &sessionASN1);
    int sessionTicketLifeTimeHint() const;

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
    friend class QSharedPointer<QSslContext>;

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
