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

#ifndef QWINDOWSCAROOTFETCHER_P_H
#define QWINDOWSCAROOTFETCHER_P_H

#include <QtCore/QtGlobal>
#include <QtCore/QObject>

#include "qsslsocket_p.h"

#include "qsslsocket.h"
#include "qsslcertificate.h"

#include <memory>

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
