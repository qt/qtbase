// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QX509_ST_P_H
#define QX509_ST_P_H

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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class X509CertificateSecureTransport final : public X509CertificateGeneric
{
public:
    TlsKey *publicKey() const override;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QX509_ST_P_H
