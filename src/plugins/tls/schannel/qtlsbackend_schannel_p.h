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

#ifndef QTLSBACKEND_ST_P_H
#define QTLSBACKEND_ST_P_H

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

#include <private/qtnetworkglobal_p.h>

#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtCore/qglobal.h>


QT_BEGIN_NAMESPACE

class QSchannelBackend : public QTlsBackend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QTlsBackend_iid)
    Q_INTERFACES(QTlsBackend)

public:
    static void ensureInitializedImplementation();

private:
    long tlsLibraryVersionNumber() const override;
    QString tlsLibraryVersionString() const override;
    long tlsLibraryBuildVersionNumber() const override;
    QString tlsLibraryBuildVersionString() const override;
    void ensureInitialized() const override;

    static void resetDefaultCiphers();

    QString backendName() const override;
    QList<QSsl::SslProtocol> supportedProtocols() const override;
    QList<QSsl::SupportedFeature> supportedFeatures() const override;
    QList<QSsl::ImplementedClass> implementedClasses() const override;

    QTlsPrivate::TlsKey *createKey() const override;
    QTlsPrivate::X509Certificate *createCertificate() const override;

    QTlsPrivate::TlsCryptograph * createTlsCryptograph() const override;

    QList<QSslCertificate> systemCaCertificates() const override;
    static QList<QSslCertificate> systemCaCertificatesImplementation();

    QTlsPrivate::X509PemReaderPtr X509PemReader() const override;
    QTlsPrivate::X509DerReaderPtr X509DerReader() const override;

    static bool s_loadedCiphersAndCerts;
};

Q_DECLARE_LOGGING_CATEGORY(lcTlsBackendSchannel)

QT_END_NAMESPACE

#endif // QTLSBACKEND_ST_P_H


