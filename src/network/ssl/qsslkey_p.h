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


#ifndef QSSLKEY_OPENSSL_P_H
#define QSSLKEY_OPENSSL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qsslcertificate.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qsslkey.h"
#include "qssl_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {
class TlsKey;
}

class QSslKeyPrivate
{
public:
    QSslKeyPrivate();
    ~QSslKeyPrivate();

    using Cipher = QTlsPrivate::Cipher;

    Q_NETWORK_EXPORT static QByteArray decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv);
    Q_NETWORK_EXPORT static QByteArray encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv);

    std::unique_ptr<QTlsPrivate::TlsKey> backend;
    QAtomicInt ref;

private:
    Q_DISABLE_COPY_MOVE(QSslKeyPrivate)
};

QT_END_NAMESPACE

#endif // QSSLKEY_OPENSSL_P_H
