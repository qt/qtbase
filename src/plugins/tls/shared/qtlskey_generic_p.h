// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTLSKEY_GENERIC_P_H
#define QTLSKEY_GENERIC_P_H

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

#include "qtlskey_base_p.h"


#include <QtCore/qnamespace.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

// This class is what previously was known as qsslkey_qt:
// it implements most of functionality needed by QSslKey
// not relying on any TLS implementation. It's used by
// our SecureTransport and Schannel backends.
class TlsKeyGeneric : public TlsKeyBase
{
public:
    using TlsKeyBase::TlsKeyBase;

    void decodeDer(KeyType type, KeyAlgorithm algorithm, const QByteArray &der,
                   const QByteArray &passPhrase, bool deepClear) override;
    void decodePem(KeyType type, KeyAlgorithm algorithm, const QByteArray &pem,
                   const QByteArray &passPhrase, bool deepClear) override;

    QByteArray toPem(const QByteArray &passPhrase) const override;

    QByteArray derFromPem(const QByteArray &pem, QMap<QByteArray,
                          QByteArray> *headers) const override;

    void fromHandle(Qt::HANDLE opaque, KeyType expectedType) override;

    void clear(bool deep) override;

    Qt::HANDLE handle() const override
    {
        return Qt::HANDLE(opaque);
    }

    int length() const override
    {
        return keyLength;
    }

    bool isPkcs8() const override
    {
        return pkcs8;
    }

private:
    QByteArray decryptPkcs8(const QByteArray &encrypted, const QByteArray &passPhrase);

    bool pkcs8 = false;
    Qt::HANDLE opaque = nullptr;
    QByteArray derData;
    int keyLength = -1;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLSKEY_GENERIC_P_H
