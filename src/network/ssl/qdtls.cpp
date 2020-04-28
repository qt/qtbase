/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qsslconfiguration.h"
#include "qdtls_openssl_p.h"
#include "qudpsocket.h"
#include "qdtls_p.h"
#include "qssl_p.h"
#include "qdtls.h"

#include "qglobal.h"

/*!
    \class QDtlsClientVerifier
    \brief This class implements server-side DTLS cookie generation and verification.
    \since 5.12

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    The QDtlsClientVerifier class implements server-side DTLS cookie generation
    and verification. Datagram security protocols are highly susceptible to a
    variety of Denial-of-Service attacks. According to \l {https://tools.ietf.org/html/rfc6347#section-4.2.1}{RFC 6347, section 4.2.1},
    these are two of the more common types of attack:

    \list
    \li An attacker transmits a series of handshake initiation requests, causing
    a server to allocate excessive resources and potentially perform expensive
    cryptographic operations.
    \li An attacker transmits a series of handshake initiation requests with
    a forged source of the victim, making the server act as an amplifier.
    Normally, the server would reply to the victim machine with a Certificate message,
    which can be quite large, thus flooding the victim machine with datagrams.
    \endlist

    As a countermeasure to these attacks, \l {https://tools.ietf.org/html/rfc6347#section-4.2.1}{RFC 6347, section 4.2.1}
    proposes a stateless cookie technique that a server may deploy:

    \list
    \li In response to the initial ClientHello message, the server sends a HelloVerifyRequest,
    which contains a cookie. This cookie is a cryptographic hash and is generated using the
    client's address, port number, and the server's secret (which is a cryptographically strong
    pseudo-random sequence of bytes).
    \li A reachable DTLS client is expected to reply with a new ClientHello message
    containing this cookie.
    \li When the server receives the ClientHello message with a cookie, it
    generates a new cookie as described above. This new cookie is compared to the
    one found in the ClientHello message.
    \li In the cookies are equal, the client is considered to be real, and the
    server can continue with a TLS handshake procedure.
    \endlist

    \note A DTLS server is not required to use DTLS cookies.

    QDtlsClientVerifier is designed to work in pair with QUdpSocket, as shown in
    the following code-excerpt:

    \snippet code/src_network_ssl_qdtlscookie.cpp 0

    QDtlsClientVerifier does not impose any restrictions on how the application uses
    QUdpSocket. For example, it is possible to have a server with a single QUdpSocket
    in state QAbstractSocket::BoundState, handling multiple DTLS clients
    simultaneously:

    \list
    \li Testing if new clients are real DTLS-capable clients.
    \li Completing TLS handshakes with the verified clients (see QDtls).
    \li Decrypting datagrams coming from the connected clients (see QDtls).
    \li Sending encrypted datagrams to the connected clients (see QDtls).
    \endlist

    This implies that QDtlsClientVerifier does not read directly from a socket,
    instead it expects the application to read an incoming datagram, extract the
    sender's address, and port, and then pass this data to verifyClient().
    To send a HelloVerifyRequest message, verifyClient() can write to the QUdpSocket.

    \note QDtlsClientVerifier does not take ownership of the QUdpSocket object.

    By default QDtlsClientVerifier obtains its secret from a cryptographically
    strong pseudorandom number generator.

    \note The default secret is shared by all objects of the classes QDtlsClientVerifier
    and QDtls. Since this can impose security risks, RFC 6347 recommends to change
    the server's secret frequently. Please see \l {https://tools.ietf.org/html/rfc6347}{RFC 6347, section 4.2.1}
    for hints about possible server implementations. Cookie generator parameters
    can be set using the class QDtlsClientVerifier::GeneratorParameters and
    setCookieGeneratorParameters():

    \snippet code/src_network_ssl_qdtlscookie.cpp 1

    The \l{secureudpserver}{DTLS server} example illustrates how to use
    QDtlsClientVerifier in a server application.

    \sa QUdpSocket, QAbstractSocket::BoundState, QDtls, verifyClient(),
    GeneratorParameters, setCookieGeneratorParameters(), cookieGeneratorParameters(),
    QDtls::setCookieGeneratorParameters(),
    QDtls::cookieGeneratorParameters(),
    QCryptographicHash::Algorithm,
    QDtlsError, dtlsError(), dtlsErrorString()
*/

/*!
    \class QDtlsClientVerifier::GeneratorParameters
    \brief This class defines parameters for DTLS cookie generator.
    \since 5.12

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    An object of this class provides the parameters that QDtlsClientVerifier
    will use to generate DTLS cookies. They include a cryptographic hash
    algorithm and a secret.

    \note An empty secret is considered to be invalid by
    QDtlsClientVerifier::setCookieGeneratorParameters().

    \sa QDtlsClientVerifier::setCookieGeneratorParameters(),
    QDtlsClientVerifier::cookieGeneratorParameters(),
    QDtls::setCookieGeneratorParameters(),
    QDtls::cookieGeneratorParameters(),
    QCryptographicHash::Algorithm
*/

/*!
    \enum QDtlsError
    \brief Describes errors that can be found by QDtls and QDtlsClientVerifier.
    \relates QDtls
    \since 5.12

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    This enum describes general and TLS-specific errors that can be encountered
    by objects of the classes QDtlsClientVerifier and QDtls.

    \value NoError No error occurred, the last operation was successful.
    \value InvalidInputParameters Input parameters provided by a caller were
           invalid.
    \value InvalidOperation An operation was attempted in a state that did not
           permit it.
    \value UnderlyingSocketError QUdpSocket::writeDatagram() failed, QUdpSocket::error()
           and QUdpSocket::errorString() can provide more specific information.
    \value RemoteClosedConnectionError TLS shutdown alert message was received.
    \value PeerVerificationError Peer's identity could not be verified during the
           TLS handshake.
    \value TlsInitializationError An error occurred while initializing an underlying
           TLS backend.
    \value TlsFatalError A fatal error occurred during TLS handshake, other
           than peer verification error or TLS initialization error.
    \value TlsNonFatalError A failure to encrypt or decrypt a datagram, non-fatal,
           meaning QDtls can continue working after this error.
*/

/*!
    \class QDtls
    \brief This class provides encryption for UDP sockets.
    \since 5.12

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    The QDtls class can be used to establish a secure connection with a network
    peer using User Datagram Protocol (UDP). DTLS connection over essentially
    connectionless UDP means that two peers first have to successfully complete
    a TLS handshake by calling doHandshake(). After the handshake has completed,
    encrypted datagrams can be sent to the peer using writeDatagramEncrypted().
    Encrypted datagrams coming from the peer can be decrypted by decryptDatagram().

    QDtls is designed to work with QUdpSocket. Since QUdpSocket can receive
    datagrams coming from different peers, an application must implement
    demultiplexing, forwarding datagrams coming from different peers to their
    corresponding instances of QDtls. An association between a network peer
    and its QDtls object can be established using the peer's address and port
    number. Before starting a handshake, the application must set the peer's
    address and port number using setPeer().

    QDtls does not read datagrams from QUdpSocket, this is expected to be done by
    the application, for example, in a slot attached to the QUdpSocket::readyRead()
    signal. Then, these datagrams must be processed by QDtls.

    \note QDtls does \e not take ownership of the QUdpSocket object.

    Normally, several datagrams are to be received and sent by both peers during
    the handshake phase. Upon reading datagrams, server and client must pass these
    datagrams to doHandshake() until some error is found or handshakeState()
    returns HandshakeComplete:

    \snippet code/src_network_ssl_qdtls.cpp 0

    For a server, the first call to doHandshake() requires a non-empty datagram
    containing a ClientHello message. If the server also deploys QDtlsClientVerifier,
    the first ClientHello message is expected to be the one verified by QDtlsClientVerifier.

    In case the peer's identity cannot be validated during the handshake, the application
    must inspect errors returned by peerVerificationErrors() and then either
    ignore errors by calling ignoreVerificationErrors() or abort the handshake
    by calling abortHandshake(). If errors were ignored, the handshake can be
    resumed by calling resumeHandshake().

    After the handshake has been completed, datagrams can be sent to and received
    from the network peer securely:

    \snippet code/src_network_ssl_qdtls.cpp 2

    A DTLS connection may be closed using shutdown().

    \snippet code/src_network_ssl_qdtls.cpp 3

    \warning It's recommended to call shutdown() before destroying the client's QDtls
    object if you are planning to re-use the same port number to connect to the
    server later. Otherwise, the server may drop incoming ClientHello messages,
    see \l{https://tools.ietf.org/html/rfc6347#page-25}{RFC 6347, section 4.2.8}
    for more details and implementation hints.

    If the server does not use QDtlsClientVerifier, it \e must configure its
    QDtls objects to disable the cookie verification procedure:

    \snippet code/src_network_ssl_qdtls.cpp 4

    A server that uses cookie verification with non-default generator parameters
    \e must set the same parameters for its QDtls object before starting the handshake.

    \note The DTLS protocol leaves Path Maximum Transmission Unit (PMTU) discovery
    to the application. The application may provide QDtls with the MTU using
    setMtuHint(). This hint affects only the handshake phase, since only handshake
    messages can be fragmented and reassembled by the DTLS. All other messages sent
    by the application must fit into a single datagram.
    \note DTLS-specific headers add some overhead to application data further
    reducing the possible message size.
    \warning A server configured to reply with HelloVerifyRequest will drop
    all fragmented ClientHello messages, never starting a handshake.

    The \l{secureudpserver}{DTLS server} and \l{secureudpclient}{DTLS client}
    examples illustrate how to use QDtls in applications.

    \sa QUdpSocket, QDtlsClientVerifier, HandshakeState, QDtlsError, QSslConfiguration
*/

/*!
    \typedef QDtls::GeneratorParameters
*/

/*!
    \fn void QDtls::handshakeTimeout()

    Packet loss can result in timeouts during the handshake phase. In this case
    QDtls emits a handshakeTimeout() signal. Call handleTimeout() to retransmit
    the handshake messages:

    \snippet code/src_network_ssl_qdtls.cpp 1

    \sa handleTimeout()
*/

/*!
    \fn void QDtls::pskRequired(QSslPreSharedKeyAuthenticator *authenticator)

    QDtls emits this signal when it negotiates a PSK ciphersuite, and therefore
    a PSK authentication is then required.

    When using PSK, the client must send to the server a valid identity and a
    valid pre shared key, in order for the TLS handshake to continue.
    Applications can provide this information in a slot connected to this
    signal, by filling in the passed \a authenticator object according to their
    needs.

    \note Ignoring this signal, or failing to provide the required credentials,
    will cause the handshake to fail, and therefore the connection to be aborted.

    \note The \a authenticator object is owned by QDtls and must not be deleted
    by the application.

    \sa QSslPreSharedKeyAuthenticator
*/

/*!
    \enum QDtls::HandshakeState
    \brief Describes the current state of DTLS handshake.
    \since 5.12

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    This enum describes the current state of DTLS handshake for a QDtls
    connection.

    \value HandshakeNotStarted Nothing done yet.
    \value HandshakeInProgress Handshake was initiated and no errors were found so far.
    \value PeerVerificationFailed The identity of the peer can't be established.
    \value HandshakeComplete Handshake completed successfully and encrypted connection
           was established.

    \sa QDtls::doHandshake(), QDtls::handshakeState()
*/


QT_BEGIN_NAMESPACE

QSslConfiguration QDtlsBasePrivate::configuration() const
{
    auto copyPrivate = new QSslConfigurationPrivate(dtlsConfiguration);
    copyPrivate->ref.storeRelaxed(0); // the QSslConfiguration constructor refs up
    QSslConfiguration copy(copyPrivate);
    copyPrivate->sessionCipher = sessionCipher;
    copyPrivate->sessionProtocol = sessionProtocol;

    return copy;
}

void QDtlsBasePrivate::setConfiguration(const QSslConfiguration &configuration)
{
    dtlsConfiguration.localCertificateChain = configuration.localCertificateChain();
    dtlsConfiguration.privateKey = configuration.privateKey();
    dtlsConfiguration.ciphers = configuration.ciphers();
    dtlsConfiguration.ellipticCurves = configuration.ellipticCurves();
    dtlsConfiguration.preSharedKeyIdentityHint = configuration.preSharedKeyIdentityHint();
    dtlsConfiguration.dhParams = configuration.diffieHellmanParameters();
    dtlsConfiguration.caCertificates = configuration.caCertificates();
    dtlsConfiguration.peerVerifyDepth = configuration.peerVerifyDepth();
    dtlsConfiguration.peerVerifyMode = configuration.peerVerifyMode();
    dtlsConfiguration.protocol = configuration.protocol();
    dtlsConfiguration.sslOptions = configuration.d->sslOptions;
    dtlsConfiguration.sslSession = configuration.sessionTicket();
    dtlsConfiguration.sslSessionTicketLifeTimeHint = configuration.sessionTicketLifeTimeHint();
    dtlsConfiguration.nextAllowedProtocols = configuration.allowedNextProtocols();
    dtlsConfiguration.nextNegotiatedProtocol = configuration.nextNegotiatedProtocol();
    dtlsConfiguration.nextProtocolNegotiationStatus = configuration.nextProtocolNegotiationStatus();
    dtlsConfiguration.dtlsCookieEnabled = configuration.dtlsCookieVerificationEnabled();
    dtlsConfiguration.allowRootCertOnDemandLoading = configuration.d->allowRootCertOnDemandLoading;
    dtlsConfiguration.backendConfig = configuration.backendConfiguration();

    clearDtlsError();
}

bool QDtlsBasePrivate::setCookieGeneratorParameters(QCryptographicHash::Algorithm alg,
                                                    const QByteArray &key)
{
    if (!key.size()) {
        setDtlsError(QDtlsError::InvalidInputParameters,
                     QDtls::tr("Invalid (empty) secret"));
        return false;
    }

    clearDtlsError();

    hashAlgorithm = alg;
    secret = key;

    return true;
}

bool QDtlsBasePrivate::isDtlsProtocol(QSsl::SslProtocol protocol)
{
    switch (protocol) {
    case QSsl::DtlsV1_0:
    case QSsl::DtlsV1_0OrLater:
    case QSsl::DtlsV1_2:
    case QSsl::DtlsV1_2OrLater:
        return true;
    default:
        return false;
    }
}

static QString msgUnsupportedMulticastAddress()
{
    return QDtls::tr("Multicast and broadcast addresses are not supported");
}

/*!
    Default constructs GeneratorParameters object with QCryptographicHash::Sha1
    as its algorithm and an empty secret.

    \sa QDtlsClientVerifier::setCookieGeneratorParameters(),
    QDtlsClientVerifier::cookieGeneratorParameters(),
    QDtls::setCookieGeneratorParameters(),
    QDtls::cookieGeneratorParameters()
 */
QDtlsClientVerifier::GeneratorParameters::GeneratorParameters()
{
}

/*!
    Constructs GeneratorParameters object from \a algorithm and \a secret.

    \sa QDtlsClientVerifier::setCookieGeneratorParameters(),
    QDtlsClientVerifier::cookieGeneratorParameters(),
    QDtls::setCookieGeneratorParameters(),
    QDtls::cookieGeneratorParameters()
 */
QDtlsClientVerifier::GeneratorParameters::GeneratorParameters(QCryptographicHash::Algorithm algorithm, const QByteArray &secret)
    : hash(algorithm), secret(secret)
{
}

/*!
    Constructs a QDtlsClientVerifier object, \a parent is passed to QObject's
    constructor.
*/
QDtlsClientVerifier::QDtlsClientVerifier(QObject *parent)
    : QObject(*new QDtlsClientVerifierOpenSSL, parent)
{
    Q_D(QDtlsClientVerifier);

    d->mode = QSslSocket::SslServerMode;
    // The default configuration suffices: verifier never does a full
    // handshake and upon verifying a cookie in a client hello message,
    // it reports success.
    auto conf = QSslConfiguration::defaultDtlsConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    d->setConfiguration(conf);
}

/*!
    Destroys the QDtlsClientVerifier object.
*/
QDtlsClientVerifier::~QDtlsClientVerifier()
{
}

/*!
    Sets the secret and the cryptographic hash algorithm from \a params. This
    QDtlsClientVerifier will use these to generate cookies. If the new secret
    has size zero, this function returns \c false and does not change the
    cookie generator parameters.

    \note The secret is supposed to be a cryptographically secure sequence of bytes.

    \sa QDtlsClientVerifier::GeneratorParameters, cookieGeneratorParameters(),
    QCryptographicHash::Algorithm
*/
bool QDtlsClientVerifier::setCookieGeneratorParameters(const GeneratorParameters &params)
{
    Q_D(QDtlsClientVerifier);

    return d->setCookieGeneratorParameters(params.hash, params.secret);
}

/*!
    Returns the current secret and hash algorithm used to generate cookies.
    The default hash algorithm is QCryptographicHash::Sha256 if Qt was configured
    to support it, QCryptographicHash::Sha1 otherwise. The default secret is
    obtained from the backend-specific cryptographically strong pseudorandom
    number generator.

    \sa QCryptographicHash::Algorithm, QDtlsClientVerifier::GeneratorParameters,
    setCookieGeneratorParameters()
*/
QDtlsClientVerifier::GeneratorParameters QDtlsClientVerifier::cookieGeneratorParameters() const
{
    Q_D(const QDtlsClientVerifier);

    return {d->hashAlgorithm, d->secret};
}

/*!
    \a socket must be a valid pointer, \a dgram must be a non-empty
    datagram, \a address cannot be null, broadcast, or multicast.
    \a port is the remote peer's port. This function returns \c true
    if \a dgram contains a ClientHello message with a valid cookie.
    If no matching cookie is found, verifyClient() will send a
    HelloVerifyRequest message using \a socket and return \c false.

    The following snippet shows how a server application may check for errors:

    \snippet code/src_network_ssl_qdtlscookie.cpp 2

    \sa QHostAddress::isNull(), QHostAddress::isBroadcast(), QHostAddress::isMulticast(),
    setCookieGeneratorParameters(), cookieGeneratorParameters()
*/
bool QDtlsClientVerifier::verifyClient(QUdpSocket *socket, const QByteArray &dgram,
                                       const QHostAddress &address, quint16 port)
{
    Q_D(QDtlsClientVerifier);

    if (!socket || address.isNull() || !dgram.size()) {
        d->setDtlsError(QDtlsError::InvalidInputParameters,
                        tr("A valid UDP socket, non-empty datagram, valid address/port were expected"));
        return false;
    }

    if (address.isBroadcast() || address.isMulticast()) {
        d->setDtlsError(QDtlsError::InvalidInputParameters,
                        msgUnsupportedMulticastAddress());
        return false;
    }

    return d->verifyClient(socket, dgram, address, port);
}

/*!
    Convenience function. Returns the last ClientHello message that was successfully
    verified, or an empty QByteArray if no verification has completed.

    \sa verifyClient()
*/
QByteArray QDtlsClientVerifier::verifiedHello() const
{
    Q_D(const QDtlsClientVerifier);

    return d->verifiedClientHello;
}

/*!
    Returns the last error that occurred or QDtlsError::NoError.

    \sa QDtlsError, dtlsErrorString()
*/
QDtlsError QDtlsClientVerifier::dtlsError() const
{
    Q_D(const QDtlsClientVerifier);

    return d->errorCode;
}

/*!
    Returns a textual description of the last error, or an empty string.

    \sa dtlsError()
 */
QString QDtlsClientVerifier::dtlsErrorString() const
{
    Q_D(const QDtlsBase);

    return d->errorDescription;
}

/*!
    Creates a QDtls object, \a parent is passed to the QObject constructor.
    \a mode is QSslSocket::SslServerMode for a server-side DTLS connection or
    QSslSocket::SslClientMode for a client.

    \sa sslMode(), QSslSocket::SslMode
*/
QDtls::QDtls(QSslSocket::SslMode mode, QObject *parent)
    : QObject(*new QDtlsPrivateOpenSSL, parent)
{
    Q_D(QDtls);

    d->mode = mode;
    setDtlsConfiguration(QSslConfiguration::defaultDtlsConfiguration());
}

/*!
    Destroys the QDtls object.
*/
QDtls::~QDtls()
{
}

/*!
    Sets the peer's address, \a port, and host name and returns \c true
    if successful. \a address must not be null, multicast, or broadcast.
    \a verificationName is the host name used for the certificate validation.

    \sa peerAddress(), peerPort(), peerVerificationName()
 */
bool QDtls::setPeer(const QHostAddress &address, quint16 port,
                    const QString &verificationName)
{
    Q_D(QDtls);

    if (d->handshakeState != HandshakeNotStarted) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot set peer after handshake started"));
        return false;
    }

    if (address.isNull()) {
        d->setDtlsError(QDtlsError::InvalidInputParameters,
                        tr("Invalid address"));
        return false;
    }

    if (address.isBroadcast() || address.isMulticast()) {
        d->setDtlsError(QDtlsError::InvalidInputParameters,
                        msgUnsupportedMulticastAddress());
        return false;
    }

    d->clearDtlsError();

    d->remoteAddress = address;
    d->remotePort = port;
    d->peerVerificationName = verificationName;

    return true;
}

/*!
    Sets the host \a name that will be used for the certificate validation
    and returns \c true if successful.

    \note This function must be called before the handshake starts.

    \sa peerVerificationName(), setPeer()
*/
bool QDtls::setPeerVerificationName(const QString &name)
{
    Q_D(QDtls);

    if (d->handshakeState != HandshakeNotStarted) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot set verification name after handshake started"));
        return false;
    }

    d->clearDtlsError();
    d->peerVerificationName = name;

    return true;
}

/*!
    Returns the peer's address, set by setPeer(), or QHostAddress::Null.

    \sa setPeer()
*/
QHostAddress QDtls::peerAddress() const
{
    Q_D(const QDtls);

    return d->remoteAddress;
}

/*!
    Returns the peer's port number, set by setPeer(), or 0.

    \sa setPeer()
*/
quint16 QDtls::peerPort() const
{
    Q_D(const QDtlsBase);

    return d->remotePort;
}

/*!
    Returns the host name set by setPeer() or setPeerVerificationName().
    The default value is an empty string.

    \sa setPeerVerificationName(), setPeer()
*/
QString QDtls::peerVerificationName() const
{
    Q_D(const QDtls);

    return d->peerVerificationName;
}

/*!
    Returns QSslSocket::SslServerMode for a server-side connection and
    QSslSocket::SslClientMode for a client.

    \sa QDtls(), QSslSocket::SslMode
*/
QSslSocket::SslMode QDtls::sslMode() const
{
    Q_D(const QDtls);

    return d->mode;
}

/*!
    \a mtuHint is the maximum transmission unit (MTU), either discovered or guessed
    by the application. The application is not required to set this value.

    \sa mtuHint(), QAbstractSocket::PathMtuSocketOption
 */
void QDtls::setMtuHint(quint16 mtuHint)
{
    Q_D(QDtls);

    d->mtuHint = mtuHint;
}

/*!
    Returns the value previously set by setMtuHint(). The default value is 0.

    \sa setMtuHint()
 */
quint16 QDtls::mtuHint() const
{
    Q_D(const QDtls);

    return d->mtuHint;
}

/*!
    Sets the cryptographic hash algorithm and the secret from \a params.
    This function is only needed for a server-side QDtls connection.
    Returns \c true if successful.

    \note This function must be called before the handshake starts.

    \sa cookieGeneratorParameters(), doHandshake(), QDtlsClientVerifier,
    QDtlsClientVerifier::cookieGeneratorParameters()
*/
bool QDtls::setCookieGeneratorParameters(const GeneratorParameters &params)
{
    Q_D(QDtls);

    return d->setCookieGeneratorParameters(params.hash, params.secret);
}

/*!
    Returns the current hash algorithm and secret, either default ones or previously
    set by a call to setCookieGeneratorParameters().

    The default hash algorithm is QCryptographicHash::Sha256 if Qt was
    configured to support it, QCryptographicHash::Sha1 otherwise. The default
    secret is obtained from the backend-specific cryptographically strong
    pseudorandom number generator.

    \sa QDtlsClientVerifier, cookieGeneratorParameters()
*/
QDtls::GeneratorParameters QDtls::cookieGeneratorParameters() const
{
    Q_D(const QDtls);

    return {d->hashAlgorithm, d->secret};
}

/*!
    Sets the connection's TLS configuration from \a configuration
    and returns \c true if successful.

    \note This function must be called before the handshake starts.

    \sa dtlsConfiguration(), doHandshake()
*/
bool QDtls::setDtlsConfiguration(const QSslConfiguration &configuration)
{
    Q_D(QDtls);

    if (d->handshakeState != HandshakeNotStarted) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot set configuration after handshake started"));
        return false;
    }

    d->setConfiguration(configuration);
    return true;
}

/*!
    Returns either the default DTLS configuration or the configuration set by an
    earlier call to setDtlsConfiguration().

    \sa setDtlsConfiguration(), QSslConfiguration::defaultDtlsConfiguration()
*/
QSslConfiguration QDtls::dtlsConfiguration() const
{
    Q_D(const QDtls);

    return d->configuration();
}

/*!
    Returns the current handshake state for this QDtls.

    \sa doHandshake(), QDtls::HandshakeState
 */
QDtls::HandshakeState QDtls::handshakeState()const
{
    Q_D(const QDtls);

    return d->handshakeState;
}

/*!
    Starts or continues a DTLS handshake. \a socket must be a valid pointer.
    When starting a server-side DTLS handshake, \a dgram must contain the initial
    ClientHello message read from QUdpSocket. This function returns \c true if
    no error was found. Handshake state can be tested using handshakeState().
    \c false return means some error occurred, use dtlsError() for more
    detailed information.

    \note If the identity of the peer can't be established, the error is set to
    QDtlsError::PeerVerificationError. If you want to ignore verification errors
    and continue connecting, you must call ignoreVerificationErrors() and then
    resumeHandshake(). If the errors cannot be ignored, you must call
    abortHandshake().

    \snippet code/src_network_ssl_qdtls.cpp 5

    \sa handshakeState(), dtlsError(), ignoreVerificationErrors(), resumeHandshake(),
    abortHandshake()
*/
bool QDtls::doHandshake(QUdpSocket *socket, const QByteArray &dgram)
{
    Q_D(QDtls);

    if (d->handshakeState == HandshakeNotStarted)
        return startHandshake(socket, dgram);
    else if (d->handshakeState == HandshakeInProgress)
        return continueHandshake(socket, dgram);

    d->setDtlsError(QDtlsError::InvalidOperation,
                    tr("Cannot start/continue handshake, invalid handshake state"));
    return false;
}

/*!
    \internal
*/
bool QDtls::startHandshake(QUdpSocket *socket, const QByteArray &datagram)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return false;
    }

    if (d->remoteAddress.isNull()) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("To start a handshake you must set peer's address and port first"));
        return false;
    }

    if (sslMode() == QSslSocket::SslServerMode && !datagram.size()) {
        d->setDtlsError(QDtlsError::InvalidInputParameters,
                        tr("To start a handshake, DTLS server requires non-empty datagram (client hello)"));
        return false;
    }

    if (d->handshakeState != HandshakeNotStarted) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot start handshake, already done/in progress"));
        return false;
    }

    return d->startHandshake(socket, datagram);
}

/*!
    If a timeout occures during the handshake, the handshakeTimeout() signal
    is emitted. The application must call handleTimeout() to retransmit handshake
    messages; handleTimeout() returns \c true if a timeout has occurred, false
    otherwise. \a socket must be a valid pointer.

    \sa handshakeTimeout()
*/
bool QDtls::handleTimeout(QUdpSocket *socket)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return false;
    }

    return d->handleTimeout(socket);
}

/*!
    \internal
*/
bool QDtls::continueHandshake(QUdpSocket *socket, const QByteArray &datagram)
{
    Q_D(QDtls);

    if (!socket || !datagram.size()) {
        d->setDtlsError(QDtlsError::InvalidInputParameters,
                        tr("A valid QUdpSocket and non-empty datagram are needed to continue the handshake"));
        return false;
    }

    if (d->handshakeState != HandshakeInProgress) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot continue handshake, not in InProgress state"));
        return false;
    }

    return d->continueHandshake(socket, datagram);
}

/*!
    If peer verification errors were ignored during the handshake,
    resumeHandshake() resumes and completes the handshake and returns
    \c true. \a socket must be a valid pointer. Returns \c false if
    the handshake could not be resumed.

    \sa doHandshake(), abortHandshake() peerVerificationErrors(), ignoreVerificationErrors()
*/
bool QDtls::resumeHandshake(QUdpSocket *socket)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return false;
    }

    if (d->handshakeState != PeerVerificationFailed) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot resume, not in VerificationError state"));
        return false;
    }

    return d->resumeHandshake(socket);
}

/*!
    Aborts the ongoing handshake. Returns true if one was on-going on \a socket;
    otherwise, sets a suitable error and returns false.

    \sa doHandshake(), resumeHandshake()
 */
bool QDtls::abortHandshake(QUdpSocket *socket)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return false;
    }

    if (d->handshakeState != PeerVerificationFailed && d->handshakeState != HandshakeInProgress) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("No handshake in progress, nothing to abort"));
        return false;
    }

    d->abortHandshake(socket);
    return true;
}

/*!
    Sends an encrypted shutdown alert message and closes the DTLS connection.
    Handshake state changes to QDtls::HandshakeNotStarted. \a socket must be a
    valid pointer. This function returns \c true on success.

    \sa doHandshake()
 */
bool QDtls::shutdown(QUdpSocket *socket)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters,
                        tr("Invalid (nullptr) socket"));
        return false;
    }

    if (!d->connectionEncrypted) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot send shutdown alert, not encrypted"));
        return false;
    }

    d->sendShutdownAlert(socket);
    return true;
}

/*!
    Returns \c true if DTLS handshake completed successfully.

    \sa doHandshake(), handshakeState()
 */
bool QDtls::isConnectionEncrypted() const
{
    Q_D(const QDtls);

    return d->connectionEncrypted;
}

/*!
    Returns the cryptographic \l {QSslCipher} {cipher} used by this connection,
    or a null cipher if the connection isn't encrypted. The cipher for the
    session is selected during the handshake phase. The cipher is used to encrypt
    and decrypt data.

    QSslConfiguration provides functions for setting the ordered list of ciphers
    from which the handshake phase will eventually select the session cipher.
    This ordered list must be in place before the handshake phase begins.

    \sa QSslConfiguration, setDtlsConfiguration(), dtlsConfiguration()
*/
QSslCipher QDtls::sessionCipher() const
{
    Q_D(const QDtls);

    return d->sessionCipher;
}

/*!
    Returns the DTLS protocol version used by this connection, or UnknownProtocol
    if the connection isn't encrypted yet. The protocol for the connection is selected
    during the handshake phase.

    setDtlsConfiguration() can set the preferred version before the handshake starts.

    \sa setDtlsConfiguration(), QSslConfiguration, QSslConfiguration::defaultDtlsConfiguration(),
    QSslConfiguration::setProtocol()
*/
QSsl::SslProtocol QDtls::sessionProtocol() const
{
    Q_D(const QDtls);

    return d->sessionProtocol;
}

/*!
    Encrypts \a dgram and writes the encrypted data into \a socket. Returns the
    number of bytes written, or -1 in case of error. The handshake must be completed
    before writing encrypted data. \a socket must be a valid
    pointer.

    \sa doHandshake(), handshakeState(), isConnectionEncrypted(), dtlsError()
*/
qint64 QDtls::writeDatagramEncrypted(QUdpSocket *socket, const QByteArray &dgram)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return -1;
    }

    if (!isConnectionEncrypted()) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot write a datagram, not in encrypted state"));
        return -1;
    }

    return d->writeDatagramEncrypted(socket, dgram);
}

/*!
    Decrypts \a dgram and returns its contents as plain text. The handshake must
    be completed before datagrams can be decrypted. Depending on the type of the
    TLS message the connection may write into \a socket, which must be a valid
    pointer.
*/
QByteArray QDtls::decryptDatagram(QUdpSocket *socket, const QByteArray &dgram)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return {};
    }

    if (!isConnectionEncrypted()) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot read a datagram, not in encrypted state"));
        return {};
    }

    if (!dgram.size())
        return {};

    return d->decryptDatagram(socket, dgram);
}

/*!
    Returns the last error encountered by the connection or QDtlsError::NoError.

    \sa dtlsErrorString(), QDtlsError
*/
QDtlsError QDtls::dtlsError() const
{
    Q_D(const QDtls);

    return d->errorCode;
}

/*!
    Returns a textual description for the last error encountered by the connection
    or empty string.

    \sa dtlsError()
*/
QString QDtls::dtlsErrorString() const
{
    Q_D(const QDtls);

    return d->errorDescription;
}

/*!
    Returns errors found while establishing the identity of the peer.

    If you want to continue connecting despite the errors that have occurred,
    you must call ignoreVerificationErrors().
*/
QVector<QSslError> QDtls::peerVerificationErrors() const
{
    Q_D(const QDtls);

    return d->tlsErrors;
}

/*!
    This method tells QDtls to ignore only the errors given in \a errorsToIgnore.

    If, for instance, you want to connect to a server that uses a self-signed
    certificate, consider the following snippet:

    \snippet code/src_network_ssl_qdtls.cpp 6

    You can also call this function after doHandshake() encountered the
    QDtlsError::PeerVerificationError error, and then resume the handshake by
    calling resumeHandshake().

    Later calls to this function will replace the list of errors that were
    passed in previous calls. You can clear the list of errors you want to ignore
    by calling this function with an empty list.

    \sa doHandshake(), resumeHandshake(), QSslError
*/
void QDtls::ignoreVerificationErrors(const QVector<QSslError> &errorsToIgnore)
{
    Q_D(QDtls);

    d->tlsErrorsToIgnore = errorsToIgnore;
}

QT_END_NAMESPACE
