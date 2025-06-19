// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTLSBACKEND_OPENSSL_P_H
#define QTLSBACKEND_OPENSSL_P_H

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

#include <QtNetwork/qssldiffiehellmanparameters.h>
#include <QtNetwork/qsslcertificate.h>

#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>

#include <openssl/ssl.h>

QT_BEGIN_NAMESPACE

class QTlsBackendOpenSSL final : public QTlsBackend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QTlsBackend_iid)
    Q_INTERFACES(QTlsBackend)

public:

    static QString getErrorsFromOpenSsl();
    static void logAndClearErrorQueue();
    static void clearErrorQueue();

    // Index used in SSL_get_ex_data to get the matching TlsCryptographerOpenSSL:
    static int s_indexForSSLExtraData;

    static QString msgErrorsDuringHandshake();
    static QSslCipher qt_OpenSSL_cipher_to_QSslCipher(const SSL_CIPHER *cipher);
private:
    static bool ensureLibraryLoaded();
    QString backendName() const override;
    bool isValid() const override;
    long tlsLibraryVersionNumber() const override;
    QString tlsLibraryVersionString() const override;
    long tlsLibraryBuildVersionNumber() const override;
    QString tlsLibraryBuildVersionString() const override;

    void ensureInitialized() const override;
    void ensureCiphersAndCertsLoaded() const;
    static void resetDefaultCiphers();

    QList<QSsl::SslProtocol> supportedProtocols() const override;
    QList<QSsl::SupportedFeature> supportedFeatures() const override;
    QList<QSsl::ImplementedClass> implementedClasses() const override;

    // QSslKey:
    QTlsPrivate::TlsKey *createKey() const override;

    // QSslCertificate:
    QTlsPrivate::X509Certificate *createCertificate() const override;
    QList<QSslCertificate> systemCaCertificates() const override;

    QTlsPrivate::TlsCryptograph *createTlsCryptograph() const override;
    QTlsPrivate::DtlsCookieVerifier *createDtlsCookieVerifier() const override;
    QTlsPrivate::DtlsCryptograph *createDtlsCryptograph(QDtls *q, int mode) const override;

    QTlsPrivate::X509ChainVerifyPtr X509Verifier() const override;
    QTlsPrivate::X509PemReaderPtr X509PemReader() const override;
    QTlsPrivate::X509DerReaderPtr X509DerReader() const override;
    QTlsPrivate::X509Pkcs12ReaderPtr X509Pkcs12Reader() const override;

    // Elliptic curves:
    QList<int> ellipticCurvesIds() const override;
    int curveIdFromShortName(const QString &name) const override;
    int curveIdFromLongName(const QString &name) const override;
    QString shortNameForId(int cid) const override;
    QString longNameForId(int cid) const override;
    bool isTlsNamedCurve(int cid) const override;

    // DH parameters:
    using DHParams = QSslDiffieHellmanParameters;
    int dhParametersFromDer(const QByteArray &derData, QByteArray *data) const override;
    int dhParametersFromPem(const QByteArray &pemData, QByteArray *data) const override;

    void forceAutotestSecurityLevel() override;
};

Q_DECLARE_LOGGING_CATEGORY(lcTlsBackend)

QT_END_NAMESPACE

#endif // QTLSBACKEND_OPENSSL_P_H


