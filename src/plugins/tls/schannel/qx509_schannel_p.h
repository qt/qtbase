// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QX509_SCHANNEL_P_H
#define QX509_SCHANNEL_P_H

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

#include "../shared/qx509_generic_p.h"
#include "../shared/qwincrypt_p.h"

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class X509CertificateSchannel final : public X509CertificateGeneric
{
public:
    X509CertificateSchannel();
    ~X509CertificateSchannel();

    TlsKey *publicKey() const override;
    Qt::HANDLE handle() const override;

    static QSslCertificate QSslCertificate_from_CERT_CONTEXT(const CERT_CONTEXT *certificateContext);

    static bool importPkcs12(QIODevice *device, QSslKey *key, QSslCertificate *cert,
                             QList<QSslCertificate> *caCertificates,
                             const QByteArray &passPhrase);
private:
    const CERT_CONTEXT *certificateContext = nullptr;

    Q_DISABLE_COPY_MOVE(X509CertificateSchannel);
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QX509_SCHANNEL_P_H
