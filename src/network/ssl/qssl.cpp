/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

    \note In Qt 4, this enum was called \c {AlternateNameEntryType}. That name
    is deprecated in Qt 5.

    \sa QSslCertificate::subjectAlternativeNames()
*/

/*!
  \typedef QSsl::AlternateNameEntryType
  \obsolete

  Use QSsl::AlternativeNameEntryType instead.
*/

/*!
    \enum QSsl::SslProtocol

    Describes the protocol of the cipher.

    \value SslV3 SSLv3; not supported by QSslSocket.
    \value SslV2 SSLv2; not supported by QSslSocket.
    \value TlsV1_0 TLSv1.0
    \value TlsV1_0OrLater TLSv1.0 and later versions. This option is not available when using the WinRT backend due to platform limitations.
    \value TlsV1 Obsolete, means the same as TlsV1_0
    \value TlsV1_1 TLSv1.1. When using the WinRT backend this option will also enable TLSv1.0.
    \value TlsV1_1OrLater TLSv1.1 and later versions. This option is not available when using the WinRT backend due to platform limitations.
    \value TlsV1_2 TLSv1.2. When using the WinRT backend this option will also enable TLSv1.0 and TLSv1.1.
    \value TlsV1_2OrLater TLSv1.2 and later versions. This option is not available when using the WinRT backend due to platform limitations.
    \value DtlsV1_0 DTLSv1.0
    \value DtlsV1_0OrLater DTLSv1.0 and later versions.
    \value DtlsV1_2 DTLSv1.2
    \value DtlsV1_2OrLater DTLSv1.2 and later versions.
    \value TlsV1_3 TLSv1.3. (Since Qt 5.12)
    \value TlsV1_3OrLater TLSv1.3 and later versions. (Since Qt 5.12)
    \value UnknownProtocol The cipher's protocol cannot be determined.
    \value AnyProtocol Any supported protocol. This value is used by QSslSocket only.
    \value TlsV1SslV3 Same as TlsV1_0. This enumerator is deprecated, use TlsV1_0 instead.
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


QT_END_NAMESPACE
