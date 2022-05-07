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

#ifndef QTLSKEY_BASE_P_H
#define QTLSKEY_BASE_P_H

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

#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtNetwork/qssl.h>

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class TlsKeyBase : public TlsKey
{
public:
    TlsKeyBase(KeyType type = QSsl::PublicKey, KeyAlgorithm algorithm = QSsl::Opaque)
        : keyType(type),
          keyAlgorithm(algorithm)
    {
    }

    bool isNull() const override
    {
        return keyIsNull;
    }
    KeyType type() const override
    {
        return keyType;
    }
    KeyAlgorithm algorithm() const override
    {
        return keyAlgorithm;
    }
    bool isPkcs8 () const override
    {
        return false;
    }

    QByteArray pemFromDer(const QByteArray &der, const QMap<QByteArray, QByteArray> &headers) const override;

protected:
    static QByteArray pkcs8Header(bool encrypted);
    static QByteArray pkcs8Footer(bool encrypted);
    static bool isEncryptedPkcs8(const QByteArray &der);

    bool keyIsNull = true;
    KeyType keyType = QSsl::PublicKey;
    KeyAlgorithm keyAlgorithm = QSsl::Opaque;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLSKEY_BASE_P_H
