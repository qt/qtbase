/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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

    enum SslProtocol {
        TlsV1_0,
        TlsV1_1,
        TlsV1_2,
        AnyProtocol,
        SecureProtocols,

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

    enum class AlertLevel {
        Warning,
        Fatal,
        Unknown
    };

    enum class AlertType {
        CloseNotify,
        UnexpectedMessage = 10,
        BadRecordMac = 20,
        RecordOverflow = 22,
        DecompressionFailure = 30, // reserved
        HandshakeFailure = 40,
        NoCertificate = 41, // reserved
        BadCertificate = 42,
        UnsupportedCertificate = 43,
        CertificateRevoked = 44,
        CertificateExpired = 45,
        CertificateUnknown = 46,
        IllegalParameter = 47,
        UnknownCa = 48,
        AccessDenied = 49,
        DecodeError = 50,
        DecryptError = 51,
        ExportRestriction = 60, // reserved
        ProtocolVersion = 70,
        InsufficientSecurity = 71,
        InternalError = 80,
        InappropriateFallback = 86,
        UserCancelled = 90,
        NoRenegotiation = 100,
        MissingExtension = 109,
        UnsupportedExtension = 110,
        CertificateUnobtainable = 111, // reserved
        UnrecognizedName = 112,
        BadCertificateStatusResponse = 113,
        BadCertificateHashValue = 114, // reserved
        UnknownPskIdentity = 115,
        CertificateRequired = 116,
        NoApplicationProtocol = 120,
        UnknownAlertMessage = 255
    };

    enum class ImplementedClass
    {
        Key,
        Certificate,
        Socket,
        DiffieHellman,
        EllipticCurve,
        Dtls
    };

    enum class SupportedFeature
    {
        CertificateVerification,
        ClientSideAlpn,
        ServerSideAlpn,
        Ocsp,
        Psk,
        SessionTicket,
        Alerts
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QSsl::SslOptions)

QT_END_NAMESPACE

#endif // QSSL_H
