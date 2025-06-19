// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qsslkey.h"
#include "qssl_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcSsl, "qt.network.ssl");

/*! \namespace QSsl

    \brief The QSsl namespace declares enums common to all SSL classes in Qt Network.
    \since 4.3

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork
*/

/*!
    \enum QSsl::KeyType

    Describes the two types of keys QSslKey supports.

    \value PrivateKey A private key.
    \value PublicKey A public key.
*/

/*!
    \enum QSsl::KeyAlgorithm

    Describes the different key algorithms supported by QSslKey.

    \value Rsa The RSA algorithm.
    \value Dsa The DSA algorithm.
    \value Ec  The Elliptic Curve algorithm.
    \value Dh  The Diffie-Hellman algorithm.
    \value Opaque A key that should be treated as a 'black box' by QSslKey.

    The opaque key facility allows applications to add support for facilities
    such as PKCS#11 that Qt does not currently offer natively.
*/

/*!
    \enum QSsl::EncodingFormat

    Describes supported encoding formats for certificates and keys.

    \value Pem The PEM format.
    \value Der The DER format.
*/

/*!
    \enum QSsl::AlternativeNameEntryType

    Describes the key types for alternative name entries in QSslCertificate.

    \value EmailEntry An email entry; the entry contains an email address that
    the certificate is valid for.

    \value DnsEntry A DNS host name entry; the entry contains a host name
    entry that the certificate is valid for. The entry may contain wildcards.

    \value IpAddressEntry An IP address entry; the entry contains an IP address
    entry that the certificate is valid for, introduced in Qt 5.13.

    \sa QSslCertificate::subjectAlternativeNames()
*/

/*!
    \enum QSsl::SslProtocol

    Describes the protocol of the cipher.

    \value TlsV1_0 TLSv1.0
    \value TlsV1_0OrLater TLSv1.0 and later versions.
    \value TlsV1_1 TLSv1.1.
    \value TlsV1_1OrLater TLSv1.1 and later versions.
    \value TlsV1_2 TLSv1.2.
    \value TlsV1_2OrLater TLSv1.2 and later versions.
    \value DtlsV1_0 DTLSv1.0
    \value DtlsV1_0OrLater DTLSv1.0 and later versions.
    \value DtlsV1_2 DTLSv1.2
    \value DtlsV1_2OrLater DTLSv1.2 and later versions.
    \value [since 5.12] TlsV1_3 TLSv1.3.
    \value [since 5.12] TlsV1_3OrLater TLSv1.3 and later versions.
    \value UnknownProtocol The cipher's protocol cannot be determined.
    \value AnyProtocol Any supported protocol. This value is used by QSslSocket only.
    \value SecureProtocols The default option, using protocols known to be secure.
*/

/*!
    \enum QSsl::SslOption

    Describes the options that can be used to control the details of
    SSL behaviour. These options are generally used to turn features off
    to work around buggy servers.

    \value SslOptionDisableEmptyFragments Disables the insertion of empty
    fragments into the data when using block ciphers. When enabled, this
    prevents some attacks (such as the BEAST attack), however it is
    incompatible with some servers.
    \value SslOptionDisableSessionTickets Disables the SSL session ticket
    extension. This can cause slower connection setup, however some servers
    are not compatible with the extension.
    \value SslOptionDisableCompression Disables the SSL compression
    extension. When enabled, this allows the data being passed over SSL to
    be compressed, however some servers are not compatible with this
    extension.
    \value SslOptionDisableServerNameIndication Disables the SSL server
    name indication extension. When enabled, this tells the server the virtual
    host being accessed allowing it to respond with the correct certificate.
    \value SslOptionDisableLegacyRenegotiation Disables the older insecure
    mechanism for renegotiating the connection parameters. When enabled, this
    option can allow connections for legacy servers, but it introduces the
    possibility that an attacker could inject plaintext into the SSL session.
    \value SslOptionDisableSessionSharing Disables SSL session sharing via
    the session ID handshake attribute.
    \value SslOptionDisableSessionPersistence Disables storing the SSL session
    in ASN.1 format as returned by QSslConfiguration::sessionTicket(). Enabling
    this feature adds memory overhead of approximately 1K per used session
    ticket.
    \value SslOptionDisableServerCipherPreference Disables selecting the cipher
    chosen based on the servers preferences rather than the order ciphers were
    sent by the client. This option is only relevant to server sockets, and is
    only honored by the OpenSSL backend.

    By default, SslOptionDisableEmptyFragments is turned on since this causes
    problems with a large number of servers. SslOptionDisableLegacyRenegotiation
    is also turned on, since it introduces a security risk.
    SslOptionDisableCompression is turned on to prevent the attack publicised by
    CRIME.
    SslOptionDisableSessionPersistence is turned on to optimize memory usage.
    The other options are turned off.

    \note Availability of above options depends on the version of the SSL
    backend in use.
*/

/*!
    \enum QSsl::AlertLevel
    \brief Describes the level of an alert message
    \relates QSslSocket
    \since 6.0

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    This enum describes the level of an alert message that was sent
    or received.

    \value Warning Non-fatal alert message
    \value Fatal Fatal alert message, the underlying backend will
           handle such an alert properly and close the connection.
    \value Unknown An alert of unknown level of severity.
*/

/*!
    \enum QSsl::AlertType
    \brief Enumerates possible codes that an alert message can have
    \relates QSslSocket
    \since 6.0

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    See \l{RFC 8446, section 6}
    for the possible values and their meaning.

    \value CloseNotify,
    \value UnexpectedMessage
    \value BadRecordMac
    \value RecordOverflow
    \value DecompressionFailure
    \value HandshakeFailure
    \value NoCertificate
    \value BadCertificate
    \value UnsupportedCertificate
    \value CertificateRevoked
    \value CertificateExpired
    \value CertificateUnknown
    \value IllegalParameter
    \value UnknownCa
    \value AccessDenied
    \value DecodeError
    \value DecryptError
    \value ExportRestriction
    \value ProtocolVersion
    \value InsufficientSecurity
    \value InternalError
    \value InappropriateFallback
    \value UserCancelled
    \value NoRenegotiation
    \value MissingExtension
    \value UnsupportedExtension
    \value CertificateUnobtainable
    \value UnrecognizedName
    \value BadCertificateStatusResponse
    \value BadCertificateHashValue
    \value UnknownPskIdentity
    \value CertificateRequired
    \value NoApplicationProtocol
    \value UnknownAlertMessage
*/

/*!
    \enum QSsl::ImplementedClass
    \brief Enumerates classes that a TLS backend implements
    \relates QSslSocket
    \since 6.1

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    In QtNetwork, some classes have backend-specific implementation and thus
    can be left unimplemented. Enumerators in this enum indicate, which class
    has a working implementation in the backend.

    \value Key Class QSslKey.
    \value Certificate Class QSslCertificate.
    \value Socket Class QSslSocket.
    \value DiffieHellman Class QSslDiffieHellmanParameters.
    \value EllipticCurve Class QSslEllipticCurve.
    \value Dtls Class QDtls.
    \value DtlsCookie Class QDtlsClientVerifier.
*/

/*!
    \enum QSsl::SupportedFeature
    \brief Enumerates possible features that a TLS backend supports
    \relates QSslSocket
    \since 6.1

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    In QtNetwork TLS-related classes have public API, that may be left unimplemented
    by some backend, for example, our SecureTransport backend does not support
    server-side ALPN. Enumerators from SupportedFeature enum indicate that a particular
    feature is supported.

    \value CertificateVerification Indicates that QSslCertificate::verify() is
           implemented by the backend.
    \value ClientSideAlpn Client-side ALPN (Application Layer Protocol Negotiation).
    \value ServerSideAlpn Server-side ALPN.
    \value Ocsp OCSP stapling (Online Certificate Status Protocol).
    \value Psk Pre-shared keys.
    \value SessionTicket Session tickets.
    \value Alerts Information about alert messages sent and received.
*/

QT_END_NAMESPACE

#include "moc_qssl.cpp"
