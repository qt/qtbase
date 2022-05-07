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
private:
    const CERT_CONTEXT *certificateContext = nullptr;

    Q_DISABLE_COPY_MOVE(X509CertificateSchannel);
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QX509_SCHANNEL_P_H
