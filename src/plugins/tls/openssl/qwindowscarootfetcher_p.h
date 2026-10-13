// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSCAROOTFETCHER_P_H
#define QWINDOWSCAROOTFETCHER_P_H

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qsslsocket.h>

#include <QtCore/QtGlobal>
#include <QtCore/QObject>

#include "../shared/qwincrypt_p.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QWindowsCaRootFetcher : public QObject
{
    Q_OBJECT
public:
    QWindowsCaRootFetcher(const QSslCertificate &certificate, QSslSocket::SslMode sslMode,
                          const QList<QSslCertificate> &caCertificates = {},
                          const QString &hostName = {});
    ~QWindowsCaRootFetcher();
public slots:
    void start();
signals:
    void finished(QSslCertificate brokenChain, QSslCertificate caroot);
private:
    QHCertStorePointer createAdditionalStore() const;

    QSslCertificate cert;
    QSslSocket::SslMode mode;
    // In case the application set CA certificates in the configuration,
    // in the past we did not load missing certs. But this disables
    // recoverable case when a certificate has Authority Information Access
    // extension. So we try to fetch in this scenario also, but in case
    // explicitly trusted root was not in a system store, we'll do
    // additional checks, thus we need 'peerVerifyName':
    QList<QSslCertificate> explicitlyTrustedCAs;
    QString peerVerifyName;
};

QT_END_NAMESPACE

#endif // QWINDOWSCAROOTFETCHER_P_H
