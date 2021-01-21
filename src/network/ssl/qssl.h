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


#ifndef QSSL_H
#define QSSL_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QFlags>

QT_BEGIN_NAMESPACE


namespace QSsl {
    enum KeyType {
        PrivateKey,
        PublicKey
    };

    enum EncodingFormat {
        Pem,
        Der
    };

    enum KeyAlgorithm {
        Opaque,
        Rsa,
        Dsa,
        Ec,
        Dh,
    };

    enum AlternativeNameEntryType {
        EmailEntry,
        DnsEntry,
        IpAddressEntry
    };

#if QT_DEPRECATED_SINCE(5,0)
    typedef AlternativeNameEntryType AlternateNameEntryType;
#endif

    enum SslProtocol {
#if QT_DEPRECATED_SINCE(5, 15)
        SslV3,
        SslV2,
#endif
        TlsV1_0 = 2,
#if QT_DEPRECATED_SINCE(5,0)
        TlsV1 = TlsV1_0,
#endif
        TlsV1_1,
        TlsV1_2,
        AnyProtocol,
#if QT_DEPRECATED_SINCE(5, 15)
        TlsV1SslV3,
#endif
        SecureProtocols = AnyProtocol + 2,

        TlsV1_0OrLater,
        TlsV1_1OrLater,
        TlsV1_2OrLater,

        DtlsV1_0,
        DtlsV1_0OrLater,
        DtlsV1_2,
        DtlsV1_2OrLater,

        TlsV1_3,
        TlsV1_3OrLater,

        UnknownProtocol = -1
    };

    enum SslOption {
        SslOptionDisableEmptyFragments = 0x01,
        SslOptionDisableSessionTickets = 0x02,
        SslOptionDisableCompression = 0x04,
        SslOptionDisableServerNameIndication = 0x08,
        SslOptionDisableLegacyRenegotiation = 0x10,
        SslOptionDisableSessionSharing = 0x20,
        SslOptionDisableSessionPersistence = 0x40,
        SslOptionDisableServerCipherPreference = 0x80
    };
    Q_DECLARE_FLAGS(SslOptions, SslOption)
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QSsl::SslOptions)

QT_END_NAMESPACE

#endif // QSSL_H
