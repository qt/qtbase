// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


//#define QSSLSOCKET_DEBUG

/*!
    \class QSslSocket
    \brief The QSslSocket class provides an SSL encrypted socket for both
    clients and servers.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    QSslSocket establishes a secure, encrypted TCP connection you can
    use for transmitting encrypted data. It can operate in both client
    and server mode, and it supports modern TLS protocols, including
    TLS 1.3. By default, QSslSocket uses only TLS protocols
    which are considered to be secure (QSsl::SecureProtocols), but you can
    change the TLS protocol by calling setProtocol() as long as you do
    it before the handshake has started.

    SSL encryption operates on top of the existing TCP stream after
    the socket enters the ConnectedState. There are two simple ways to
    establish a secure connection using QSslSocket: With an immediate
    SSL handshake, or with a delayed SSL handshake occurring after the
    connection has been established in unencrypted mode.

    The most common way to use QSslSocket is to construct an object
    and start a secure connection by calling connectToHostEncrypted().
    This method starts an immediate SSL handshake once the connection
    has been established.

    \snippet code/src_network_ssl_qsslsocket.cpp 0

    As with a plain QTcpSocket, QSslSocket enters the HostLookupState,
    ConnectingState, and finally the ConnectedState, if the connection
    is successful. The handshake then starts automatically, and if it
    succeeds, the encrypted() signal is emitted to indicate the socket
    has entered the encrypted state and is ready for use.

    Note that data can be written to the socket immediately after the
    return from connectToHostEncrypted() (i.e., before the encrypted()
    signal is emitted). The data is queued in QSslSocket until after
    the encrypted() signal is emitted.

    An example of using the delayed SSL handshake to secure an
    existing connection is the case where an SSL server secures an
    incoming connection. Suppose you create an SSL server class as a
    subclass of QTcpServer. You would override
    QTcpServer::incomingConnection() with something like the example
    below, which first constructs an instance of QSslSocket and then
    calls setSocketDescriptor() to set the new socket's descriptor to
    the existing one passed in. It then initiates the SSL handshake
    by calling startServerEncryption().

    \snippet code/src_network_ssl_qsslsocket.cpp 1

    If an error occurs, QSslSocket emits the sslErrors() signal. In this
    case, if no action is taken to ignore the error(s), the connection
    is dropped. To continue, despite the occurrence of an error, you
    can call ignoreSslErrors(), either from within this slot after the
    error occurs, or any time after construction of the QSslSocket and
    before the connection is attempted. This will allow QSslSocket to
    ignore the errors it encounters when establishing the identity of
    the peer. Ignoring errors during an SSL handshake should be used
    with caution, since a fundamental characteristic of secure
    connections is that they should be established with a successful
    handshake.

    Once encrypted, you use QSslSocket as a regular QTcpSocket. When
    readyRead() is emitted, you can call read(), canReadLine() and
    readLine(), or getChar() to read decrypted data from QSslSocket's
    internal buffer, and you can call write() or putChar() to write
    data back to the peer. QSslSocket will automatically encrypt the
    written data for you, and emit encryptedBytesWritten() once
    the data has been written to the peer.

    As a convenience, QSslSocket supports QTcpSocket's blocking
    functions waitForConnected(), waitForReadyRead(),
    waitForBytesWritten(), and waitForDisconnected(). It also provides
    waitForEncrypted(), which will block the calling thread until an
    encrypted connection has been established.

    \snippet code/src_network_ssl_qsslsocket.cpp 2

    QSslSocket provides an extensive, easy-to-use API for handling
    cryptographic ciphers, private keys, and local, peer, and
    Certification Authority (CA) certificates. It also provides an API
    for handling errors that occur during the handshake phase.

    The following features can also be customized:

    \list
    \li The socket's cryptographic cipher suite can be customized before
    the handshake phase with QSslConfiguration::setCiphers().
    \li The socket's local certificate and private key can be customized
    before the handshake phase with setLocalCertificate() and
    setPrivateKey().
    \li The CA certificate database can be extended and customized with
    QSslConfiguration::addCaCertificate(),
    QSslConfiguration::addCaCertificates().
    \endlist

    To extend the list of \e default CA certificates used by the SSL sockets
    during the SSL handshake you must update the default configuration, as
    in the snippet below:

    \code
        QList<QSslCertificate> certificates = getCertificates();
        QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
        configuration.addCaCertificates(certificates);
        QSslConfiguration::setDefaultConfiguration(configuration);
    \endcode

    \note If available, root certificates on Unix (excluding \macos) will be
    loaded on demand from the standard certificate directories. If you do not
    want to load root certificates on demand, you need to call either
    QSslConfiguration::defaultConfiguration().setCaCertificates() before the first
    SSL handshake is made in your application (for example, via passing
    QSslSocket::systemCaCertificates() to it), or call
    QSslConfiguration::defaultConfiguration()::setCaCertificates() on your QSslSocket instance
    prior to the SSL handshake.

    For more information about ciphers and certificates, refer to QSslCipher and
    QSslCertificate.

    This product includes software developed by the OpenSSL Project
    for use in the OpenSSL Toolkit (\l{http://www.openssl.org/}).

    \note Be aware of the difference between the bytesWritten() signal and
    the encryptedBytesWritten() signal. For a QTcpSocket, bytesWritten()
    will get emitted as soon as data has been written to the TCP socket.
    For a QSslSocket, bytesWritten() will get emitted when the data
    is being encrypted and encryptedBytesWritten()
    will get emitted as soon as data has been written to the TCP socket.

    \sa QSslCertificate, QSslCipher, QSslError
*/

/*!
    \enum QSslSocket::SslMode

    Describes the connection modes available for QSslSocket.

    \value UnencryptedMode The socket is unencrypted. Its
    behavior is identical to QTcpSocket.

    \value SslClientMode The socket is a client-side SSL socket.
    It is either already encrypted, or it is in the SSL handshake
    phase (see QSslSocket::isEncrypted()).

    \value SslServerMode The socket is a server-side SSL socket.
    It is either already encrypted, or it is in the SSL handshake
    phase (see QSslSocket::isEncrypted()).
*/

/*!
    \enum QSslSocket::PeerVerifyMode
    \since 4.4

    Describes the peer verification modes for QSslSocket. The default mode is
    AutoVerifyPeer, which selects an appropriate mode depending on the
    socket's QSocket::SslMode.

    \value VerifyNone QSslSocket will not request a certificate from the
    peer. You can set this mode if you are not interested in the identity of
    the other side of the connection. The connection will still be encrypted,
    and your socket will still send its local certificate to the peer if it's
    requested.

    \value QueryPeer QSslSocket will request a certificate from the peer, but
    does not require this certificate to be valid. This is useful when you
    want to display peer certificate details to the user without affecting the
    actual SSL handshake. This mode is the default for servers.
    Note: In Schannel this value acts the same as VerifyNone.

    \value VerifyPeer QSslSocket will request a certificate from the peer
    during the SSL handshake phase, and requires that this certificate is
    valid. On failure, QSslSocket will emit the QSslSocket::sslErrors()
    signal. This mode is the default for clients.

    \value AutoVerifyPeer QSslSocket will automatically use QueryPeer for
    server sockets and VerifyPeer for client sockets.

    \sa QSslSocket::peerVerifyMode()
*/

/*!
    \fn void QSslSocket::encrypted()

    This signal is emitted when QSslSocket enters encrypted mode. After this
    signal has been emitted, QSslSocket::isEncrypted() will return true, and
    all further transmissions on the socket will be encrypted.

    \sa QSslSocket::connectToHostEncrypted(), QSslSocket::isEncrypted()
*/

/*!
    \fn void QSslSocket::modeChanged(QSslSocket::SslMode mode)

    This signal is emitted when QSslSocket changes from \l
    QSslSocket::UnencryptedMode to either \l QSslSocket::SslClientMode or \l
    QSslSocket::SslServerMode. \a mode is the new mode.

    \sa QSslSocket::mode()
*/

/*!
    \fn void QSslSocket::encryptedBytesWritten(qint64 written)
    \since 4.4

    This signal is emitted when QSslSocket writes its encrypted data to the
    network. The \a written parameter contains the number of bytes that were
    successfully written.

    \sa QIODevice::bytesWritten()
*/

/*!
    \fn void QSslSocket::peerVerifyError(const QSslError &error)
    \since 4.4

    QSslSocket can emit this signal several times during the SSL handshake,
    before encryption has been established, to indicate that an error has
    occurred while establishing the identity of the peer. The \a error is
    usually an indication that QSslSocket is unable to securely identify the
    peer.

    This signal provides you with an early indication when something's wrong.
    By connecting to this signal, you can manually choose to tear down the
    connection from inside the connected slot before the handshake has
    completed. If no action is taken, QSslSocket will proceed to emitting
    QSslSocket::sslErrors().

    \sa sslErrors()
*/

/*!
    \fn void QSslSocket::sslErrors(const QList<QSslError> &errors);

    QSslSocket emits this signal after the SSL handshake to indicate that one
    or more errors have occurred while establishing the identity of the
    peer. The errors are usually an indication that QSslSocket is unable to
    securely identify the peer. Unless any action is taken, the connection
    will be dropped after this signal has been emitted.

    If you want to continue connecting despite the errors that have occurred,
    you must call QSslSocket::ignoreSslErrors() from inside a slot connected to
    this signal. If you need to access the error list at a later point, you
    can call sslHandshakeErrors().

    \a errors contains one or more errors that prevent QSslSocket from
    verifying the identity of the peer.

    \note You cannot use Qt::QueuedConnection when connecting to this signal,
    or calling QSslSocket::ignoreSslErrors() will have no effect.

    \sa peerVerifyError()
*/

/*!
    \fn void QSslSocket::preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator)
    \since 5.5

    QSslSocket emits this signal when it negotiates a PSK ciphersuite, and
    therefore a PSK authentication is then required.

    When using PSK, the client must send to the server a valid identity and a
    valid pre shared key, in order for the SSL handshake to continue.
    Applications can provide this information in a slot connected to this
    signal, by filling in the passed \a authenticator object according to their
    needs.

    \note Ignoring this signal, or failing to provide the required credentials,
    will cause the handshake to fail, and therefore the connection to be aborted.

    \note The \a authenticator object is owned by the socket and must not be
    deleted by the application.

    \sa QSslPreSharedKeyAuthenticator
*/

/*!
    \fn void QSslSocket::alertSent(QSsl::AlertLevel level, QSsl::AlertType type, const QString &description)

    QSslSocket emits this signal if an alert message was sent to a peer. \a level
    describes if it was a warning or a fatal error. \a type gives the code
    of the alert message. When a textual description of the alert message is
    available, it is supplied in \a description.

    \note This signal is mostly informational and can be used for debugging
    purposes, normally it does not require any actions from the application.
    \note Not all backends support this functionality.

    \sa alertReceived(), QSsl::AlertLevel, QSsl::AlertType
*/

/*!
    \fn void QSslSocket::alertReceived(QSsl::AlertLevel level, QSsl::AlertType type, const QString &description)

    QSslSocket emits this signal if an alert message was received from a peer.
    \a level tells if the alert was fatal or it was a warning. \a type is the
    code explaining why the alert was sent. When a textual description of
    the alert message is available, it is supplied in \a description.

    \note The signal is mostly for informational and debugging purposes and does not
    require any handling in the application. If the alert was fatal, underlying
    backend will handle it and close the connection.
    \note Not all backends support this functionality.

    \sa alertSent(), QSsl::AlertLevel, QSsl::AlertType
*/

/*!
    \fn void QSslSocket::handshakeInterruptedOnError(const QSslError &error)

    QSslSocket emits this signal if a certificate verification error was
    found and if early error reporting was enabled in QSslConfiguration.
    An application is expected to inspect the \a error and decide if
    it wants to continue the handshake, or abort it and send an alert message
    to the peer. The signal-slot connection must be direct.

    \sa continueInterruptedHandshake(), sslErrors(), QSslConfiguration::setHandshakeMustInterruptOnError()
*/

/*!
    \fn void QSslSocket::newSessionTicketReceived()
    \since 5.15

    If TLS 1.3 protocol was negotiated during a handshake, QSslSocket
    emits this signal after receiving NewSessionTicket message. Session
    and session ticket's lifetime hint are updated in the socket's
    configuration. The session can be used for session resumption (and
    a shortened handshake) in future TLS connections.

    \note This functionality enabled only with OpenSSL backend and requires
    OpenSSL v 1.1.1 or above.

    \sa QSslSocket::sslConfiguration(), QSslConfiguration::sessionTicket(), QSslConfiguration::sessionTicketLifeTimeHint()
*/

#include "qssl_p.h"
#include "qsslsocket.h"
#include "qsslcipher.h"
#include "qocspresponse.h"
#include "qtlsbackend_p.h"
#include "qsslconfiguration_p.h"
#include "qsslsocket_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qhostinfo.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifdef Q_OS_VXWORKS
constexpr auto isVxworks = true;
#else
constexpr auto isVxworks = false;
#endif

class QSslSocketGlobalData
{
public:
    QSslSocketGlobalData()
        : config(new QSslConfigurationPrivate),
          dtlsConfig(new QSslConfigurationPrivate)
    {
#if QT_CONFIG(dtls)
        dtlsConfig->protocol = QSsl::DtlsV1_2OrLater;
#endif // dtls
    }

    QMutex mutex;
    QList<QSslCipher> supportedCiphers;
    QList<QSslEllipticCurve> supportedEllipticCurves;
    QExplicitlySharedDataPointer<QSslConfigurationPrivate> config;
    QExplicitlySharedDataPointer<QSslConfigurationPrivate> dtlsConfig;
};
Q_GLOBAL_STATIC(QSslSocketGlobalData, globalData)

/*!
    Constructs a QSslSocket object. \a parent is passed to QObject's
    constructor. The new socket's \l {QSslCipher} {cipher} suite is
    set to the one returned by the static method defaultCiphers().
*/
QSslSocket::QSslSocket(QObject *parent)
    : QTcpSocket(*new QSslSocketPrivate, parent)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::QSslSocket(" << parent << "), this =" << (void *)this;
#endif
    d->q_ptr = this;
    d->init();
}

/*!
    Destroys the QSslSocket.
*/
QSslSocket::~QSslSocket()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::~QSslSocket(), this =" << (void *)this;
#endif
    delete d->plainSocket;
    d->plainSocket = nullptr;
}

/*!
    \reimp

    \since 5.0

    Continues data transfer on the socket after it has been paused. If
    "setPauseMode(QAbstractSocket::PauseOnSslErrors);" has been called on
    this socket and a sslErrors() signal is received, calling this method
    is necessary for the socket to continue.

    \sa QAbstractSocket::pauseMode(), QAbstractSocket::setPauseMode()
*/
void QSslSocket::resume()
{
    Q_D(QSslSocket);
    if (!d->paused)
        return;
    // continuing might emit signals, rather do this through the event loop
    QMetaObject::invokeMethod(this, "_q_resumeImplementation", Qt::QueuedConnection);
}

/*!
    Starts an encrypted connection to the device \a hostName on \a
    port, using \a mode as the \l OpenMode. This is equivalent to
    calling connectToHost() to establish the connection, followed by a
    call to startClientEncryption(). The \a protocol parameter can be
    used to specify which network protocol to use (eg. IPv4 or IPv6).

    QSslSocket first enters the HostLookupState. Then, after entering
    either the event loop or one of the waitFor...() functions, it
    enters the ConnectingState, emits connected(), and then initiates
    the SSL client handshake. At each state change, QSslSocket emits
    signal stateChanged().

    After initiating the SSL client handshake, if the identity of the
    peer can't be established, signal sslErrors() is emitted. If you
    want to ignore the errors and continue connecting, you must call
    ignoreSslErrors(), either from inside a slot function connected to
    the sslErrors() signal, or prior to entering encrypted mode. If
    ignoreSslErrors() is not called, the connection is dropped, signal
    disconnected() is emitted, and QSslSocket returns to the
    UnconnectedState.

    If the SSL handshake is successful, QSslSocket emits encrypted().

    \snippet code/src_network_ssl_qsslsocket.cpp 3

    \note The example above shows that text can be written to
    the socket immediately after requesting the encrypted connection,
    before the encrypted() signal has been emitted. In such cases, the
    text is queued in the object and written to the socket \e after
    the connection is established and the encrypted() signal has been
    emitted.

    The default for \a mode is \l ReadWrite.

    If you want to create a QSslSocket on the server side of a connection, you
    should instead call startServerEncryption() upon receiving the incoming
    connection through QTcpServer.

    \sa connectToHost(), startClientEncryption(), waitForConnected(), waitForEncrypted()
*/
void QSslSocket::connectToHostEncrypted(const QString &hostName, quint16 port, OpenMode mode, NetworkLayerProtocol protocol)
{
    Q_D(QSslSocket);
    if (d->state == ConnectedState || d->state == ConnectingState) {
        qCWarning(lcSsl,
                  "QSslSocket::connectToHostEncrypted() called when already connecting/connected");
        return;
    }

    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::connectToHostEncrypted: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }

    if (!d->verifyProtocolSupported("QSslSocket::connectToHostEncrypted:"))
        return;

    d->init();
    d->autoStartHandshake = true;
    d->initialized = true;

    // Note: When connecting to localhost, some platforms (e.g., HP-UX and some BSDs)
    // establish the connection immediately (i.e., first attempt).
    connectToHost(hostName, port, mode, protocol);
}

/*!
    \since 4.6
    \overload

    In addition to the original behaviour of connectToHostEncrypted,
    this overloaded method enables the usage of a different hostname
    (\a sslPeerName) for the certificate validation instead of
    the one used for the TCP connection (\a hostName).

    \sa connectToHostEncrypted()
*/
void QSslSocket::connectToHostEncrypted(const QString &hostName, quint16 port,
                                        const QString &sslPeerName, OpenMode mode,
                                        NetworkLayerProtocol protocol)
{
    Q_D(QSslSocket);
    if (d->state == ConnectedState || d->state == ConnectingState) {
        qCWarning(lcSsl,
                  "QSslSocket::connectToHostEncrypted() called when already connecting/connected");
        return;
    }

    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::connectToHostEncrypted: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }

    d->init();
    d->autoStartHandshake = true;
    d->initialized = true;
    d->verificationPeerName = sslPeerName;

    // Note: When connecting to localhost, some platforms (e.g., HP-UX and some BSDs)
    // establish the connection immediately (i.e., first attempt).
    connectToHost(hostName, port, mode, protocol);
}

/*!
    Initializes QSslSocket with the native socket descriptor \a
    socketDescriptor. Returns \c true if \a socketDescriptor is accepted
    as a valid socket descriptor; otherwise returns \c false.
    The socket is opened in the mode specified by \a openMode, and
    enters the socket state specified by \a state.

    \note It is not possible to initialize two sockets with the same
    native socket descriptor.

    \sa socketDescriptor()
*/
bool QSslSocket::setSocketDescriptor(qintptr socketDescriptor, SocketState state, OpenMode openMode)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::setSocketDescriptor(" << socketDescriptor << ','
             << state << ',' << openMode << ')';
#endif
    if (!d->plainSocket)
        d->createPlainSocket(openMode);
    bool retVal = d->plainSocket->setSocketDescriptor(socketDescriptor, state, openMode);
    d->cachedSocketDescriptor = d->plainSocket->socketDescriptor();
    d->setError(d->plainSocket->error(), d->plainSocket->errorString());
    setSocketState(state);
    setOpenMode(openMode);
    setLocalPort(d->plainSocket->localPort());
    setLocalAddress(d->plainSocket->localAddress());
    setPeerPort(d->plainSocket->peerPort());
    setPeerAddress(d->plainSocket->peerAddress());
    setPeerName(d->plainSocket->peerName());
    d->readChannelCount = d->plainSocket->readChannelCount();
    d->writeChannelCount = d->plainSocket->writeChannelCount();
    return retVal;
}

/*!
    \since 4.6
    Sets the given \a option to the value described by \a value.

    \sa socketOption()
*/
void QSslSocket::setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value)
{
    Q_D(QSslSocket);
    if (d->plainSocket)
        d->plainSocket->setSocketOption(option, value);
}

/*!
    \since 4.6
    Returns the value of the \a option option.

    \sa setSocketOption()
*/
QVariant QSslSocket::socketOption(QAbstractSocket::SocketOption option)
{
    Q_D(QSslSocket);
    if (d->plainSocket)
        return d->plainSocket->socketOption(option);
    else
        return QVariant();
}

/*!
    Returns the current mode for the socket; either UnencryptedMode, where
    QSslSocket behaves identially to QTcpSocket, or one of SslClientMode or
    SslServerMode, where the client is either negotiating or in encrypted
    mode.

    When the mode changes, QSslSocket emits modeChanged()

    \sa SslMode
*/
QSslSocket::SslMode QSslSocket::mode() const
{
    Q_D(const QSslSocket);
    return d->mode;
}

/*!
    Returns \c true if the socket is encrypted; otherwise, false is returned.

    An encrypted socket encrypts all data that is written by calling write()
    or putChar() before the data is written to the network, and decrypts all
    incoming data as the data is received from the network, before you call
    read(), readLine() or getChar().

    QSslSocket emits encrypted() when it enters encrypted mode.

    You can call sessionCipher() to find which cryptographic cipher is used to
    encrypt and decrypt your data.

    \sa mode()
*/
bool QSslSocket::isEncrypted() const
{
    Q_D(const QSslSocket);
    return d->connectionEncrypted;
}

/*!
    Returns the socket's SSL protocol. By default, \l QSsl::SecureProtocols is used.

    \sa setProtocol()
*/
QSsl::SslProtocol QSslSocket::protocol() const
{
    Q_D(const QSslSocket);
    return d->configuration.protocol;
}

/*!
    Sets the socket's SSL protocol to \a protocol. This will affect the next
    initiated handshake; calling this function on an already-encrypted socket
    will not affect the socket's protocol.
*/
void QSslSocket::setProtocol(QSsl::SslProtocol protocol)
{
    Q_D(QSslSocket);
    d->configuration.protocol = protocol;
}

/*!
    \since 4.4

    Returns the socket's verify mode. This mode decides whether
    QSslSocket should request a certificate from the peer (i.e., the client
    requests a certificate from the server, or a server requesting a
    certificate from the client), and whether it should require that this
    certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients and QueryPeer for servers.

    \sa setPeerVerifyMode(), peerVerifyDepth(), mode()
*/
QSslSocket::PeerVerifyMode QSslSocket::peerVerifyMode() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerVerifyMode;
}

/*!
    \since 4.4

    Sets the socket's verify mode to \a mode. This mode decides whether
    QSslSocket should request a certificate from the peer (i.e., the client
    requests a certificate from the server, or a server requesting a
    certificate from the client), and whether it should require that this
    certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients and QueryPeer for servers.

    Setting this mode after encryption has started has no effect on the
    current connection.

    \sa peerVerifyMode(), setPeerVerifyDepth(), mode()
*/
void QSslSocket::setPeerVerifyMode(QSslSocket::PeerVerifyMode mode)
{
    Q_D(QSslSocket);
    d->configuration.peerVerifyMode = mode;
}

/*!
    \since 4.4

    Returns the maximum number of certificates in the peer's certificate chain
    to be checked during the SSL handshake phase, or 0 (the default) if no
    maximum depth has been set, indicating that the whole certificate chain
    should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa setPeerVerifyDepth(), peerVerifyMode()
*/
int QSslSocket::peerVerifyDepth() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerVerifyDepth;
}

/*!
    \since 4.4

    Sets the maximum number of certificates in the peer's certificate chain to
    be checked during the SSL handshake phase, to \a depth. Setting a depth of
    0 means that no maximum depth is set, indicating that the whole
    certificate chain should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa peerVerifyDepth(), setPeerVerifyMode()
*/
void QSslSocket::setPeerVerifyDepth(int depth)
{
    Q_D(QSslSocket);
    if (depth < 0) {
        qCWarning(lcSsl, "QSslSocket::setPeerVerifyDepth: cannot set negative depth of %d", depth);
        return;
    }
    d->configuration.peerVerifyDepth = depth;
}

/*!
    \since 4.8

    Returns the different hostname for the certificate validation, as set by
    setPeerVerifyName or by connectToHostEncrypted.

    \sa setPeerVerifyName(), connectToHostEncrypted()
*/
QString QSslSocket::peerVerifyName() const
{
    Q_D(const QSslSocket);
    return d->verificationPeerName;
}

/*!
    \since 4.8

    Sets a different host name, given by \a hostName, for the certificate
    validation instead of the one used for the TCP connection.

    \sa connectToHostEncrypted()
*/
void QSslSocket::setPeerVerifyName(const QString &hostName)
{
    Q_D(QSslSocket);
    d->verificationPeerName = hostName;
}

/*!
    \reimp

    Returns the number of decrypted bytes that are immediately available for
    reading.
*/
qint64 QSslSocket::bytesAvailable() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QAbstractSocket::bytesAvailable() + (d->plainSocket ? d->plainSocket->bytesAvailable() : 0);
    return QAbstractSocket::bytesAvailable();
}

/*!
    \reimp

    Returns the number of unencrypted bytes that are waiting to be encrypted
    and written to the network.
*/
qint64 QSslSocket::bytesToWrite() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return d->plainSocket ? d->plainSocket->bytesToWrite() : 0;
    return d->writeBuffer.size();
}

/*!
    \since 4.4

    Returns the number of encrypted bytes that are awaiting decryption.
    Normally, this function will return 0 because QSslSocket decrypts its
    incoming data as soon as it can.
*/
qint64 QSslSocket::encryptedBytesAvailable() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return 0;
    return d->plainSocket->bytesAvailable();
}

/*!
    \since 4.4

    Returns the number of encrypted bytes that are waiting to be written to
    the network.
*/
qint64 QSslSocket::encryptedBytesToWrite() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return 0;
    return d->plainSocket->bytesToWrite();
}

/*!
    \reimp

    Returns \c true if you can read one while line (terminated by a single ASCII
    '\\n' character) of decrypted characters; otherwise, false is returned.
*/
bool QSslSocket::canReadLine() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QAbstractSocket::canReadLine() || (d->plainSocket && d->plainSocket->canReadLine());
    return QAbstractSocket::canReadLine();
}

/*!
    \reimp
*/
void QSslSocket::close()
{
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::close()";
#endif
    Q_D(QSslSocket);

    // On Windows, CertGetCertificateChain is probably still doing its
    // job, if the socket is re-used, we want to ignore its reported
    // root CA.
    if (auto *backend = d->backend.get())
        backend->cancelCAFetch();

    if (!d->abortCalled && (encryptedBytesToWrite() || !d->writeBuffer.isEmpty()))
        flush();
    if (d->plainSocket) {
        if (d->abortCalled)
            d->plainSocket->abort();
        else
            d->plainSocket->close();
    }
    QTcpSocket::close();

    // must be cleared, reading/writing not possible on closed socket:
    d->buffer.clear();
    d->writeBuffer.clear();
}

/*!
    \reimp
*/
bool QSslSocket::atEnd() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QAbstractSocket::atEnd() && (!d->plainSocket || d->plainSocket->atEnd());
    return QAbstractSocket::atEnd();
}

/*!
    \since 4.4

    Sets the size of QSslSocket's internal read buffer to be \a size bytes.
*/
void QSslSocket::setReadBufferSize(qint64 size)
{
    Q_D(QSslSocket);
    d->readBufferMaxSize = size;

    if (d->plainSocket)
        d->plainSocket->setReadBufferSize(size);
}

/*!
    \since 4.4

    Returns the socket's SSL configuration state. The default SSL
    configuration of a socket is to use the default ciphers,
    default CA certificates, no local private key or certificate.

    The SSL configuration also contains fields that can change with
    time without notice.

    \sa localCertificate(), peerCertificate(), peerCertificateChain(),
        sessionCipher(), privateKey(), QSslConfiguration::ciphers(),
        QSslConfiguration::caCertificates()
*/
QSslConfiguration QSslSocket::sslConfiguration() const
{
    Q_D(const QSslSocket);

    // create a deep copy of our configuration
    QSslConfigurationPrivate *copy = new QSslConfigurationPrivate(d->configuration);
    copy->ref.storeRelaxed(0);              // the QSslConfiguration constructor refs up
    copy->sessionCipher = d->sessionCipher();
    copy->sessionProtocol = d->sessionProtocol();

    return QSslConfiguration(copy);
}

/*!
    \since 4.4

    Sets the socket's SSL configuration to be the contents of \a configuration.
    This function sets the local certificate, the ciphers, the private key and the CA
    certificates to those stored in \a configuration.

    It is not possible to set the SSL-state related fields.

    \sa setLocalCertificate(), setPrivateKey(), QSslConfiguration::setCaCertificates(),
        QSslConfiguration::setCiphers()
*/
void QSslSocket::setSslConfiguration(const QSslConfiguration &configuration)
{
    Q_D(QSslSocket);
    d->configuration.localCertificateChain = configuration.localCertificateChain();
    d->configuration.privateKey = configuration.privateKey();
    d->configuration.ciphers = configuration.ciphers();
    d->configuration.ellipticCurves = configuration.ellipticCurves();
    d->configuration.preSharedKeyIdentityHint = configuration.preSharedKeyIdentityHint();
    d->configuration.dhParams = configuration.diffieHellmanParameters();
    d->configuration.caCertificates = configuration.caCertificates();
    d->configuration.peerVerifyDepth = configuration.peerVerifyDepth();
    d->configuration.peerVerifyMode = configuration.peerVerifyMode();
    d->configuration.protocol = configuration.protocol();
    d->configuration.backendConfig = configuration.backendConfiguration();
    d->configuration.sslOptions = configuration.d->sslOptions;
    d->configuration.sslSession = configuration.sessionTicket();
    d->configuration.sslSessionTicketLifeTimeHint = configuration.sessionTicketLifeTimeHint();
    d->configuration.nextAllowedProtocols = configuration.allowedNextProtocols();
    d->configuration.nextNegotiatedProtocol = configuration.nextNegotiatedProtocol();
    d->configuration.nextProtocolNegotiationStatus = configuration.nextProtocolNegotiationStatus();
#if QT_CONFIG(ocsp)
    d->configuration.ocspStaplingEnabled = configuration.ocspStaplingEnabled();
#endif
#if QT_CONFIG(openssl)
    d->configuration.reportFromCallback = configuration.handshakeMustInterruptOnError();
    d->configuration.missingCertIsFatal = configuration.missingCertificateIsFatal();
#endif // openssl
    // if the CA certificates were set explicitly (either via
    // QSslConfiguration::setCaCertificates() or QSslSocket::setCaCertificates(),
    // we cannot load the certificates on demand
    if (!configuration.d->allowRootCertOnDemandLoading) {
        d->allowRootCertOnDemandLoading = false;
        d->configuration.allowRootCertOnDemandLoading = false;
    }
}

/*!
    Sets the certificate chain to be presented to the peer during the
    SSL handshake to be \a localChain.

    \sa QSslConfiguration::setLocalCertificateChain()
    \since 5.1
 */
void QSslSocket::setLocalCertificateChain(const QList<QSslCertificate> &localChain)
{
    Q_D(QSslSocket);
    d->configuration.localCertificateChain = localChain;
}

/*!
    Returns the socket's local \l {QSslCertificate} {certificate} chain,
    or an empty list if no local certificates have been assigned.

    \sa setLocalCertificateChain()
    \since 5.1
*/
QList<QSslCertificate> QSslSocket::localCertificateChain() const
{
    Q_D(const QSslSocket);
    return d->configuration.localCertificateChain;
}

/*!
    Sets the socket's local certificate to \a certificate. The local
    certificate is necessary if you need to confirm your identity to the
    peer. It is used together with the private key; if you set the local
    certificate, you must also set the private key.

    The local certificate and private key are always necessary for server
    sockets, but are also rarely used by client sockets if the server requires
    the client to authenticate.

    \note Secure Transport SSL backend on macOS may update the default keychain
    (the default is probably your login keychain) by importing your local certificates
    and keys. This can also result in system dialogs showing up and asking for
    permission when your application is using these private keys. If such behavior
    is undesired, set the QT_SSL_USE_TEMPORARY_KEYCHAIN environment variable to a
    non-zero value; this will prompt QSslSocket to use its own temporary keychain.

    \sa localCertificate(), setPrivateKey()
*/
void QSslSocket::setLocalCertificate(const QSslCertificate &certificate)
{
    Q_D(QSslSocket);
    d->configuration.localCertificateChain = QList<QSslCertificate>();
    d->configuration.localCertificateChain += certificate;
}

/*!
    \overload

    Sets the socket's local \l {QSslCertificate} {certificate} to the
    first one found in file \a path, which is parsed according to the
    specified \a format.
*/
void QSslSocket::setLocalCertificate(const QString &path,
                                     QSsl::EncodingFormat format)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        setLocalCertificate(QSslCertificate(file.readAll(), format));

}

/*!
    Returns the socket's local \l {QSslCertificate} {certificate}, or
    an empty certificate if no local certificate has been assigned.

    \sa setLocalCertificate(), privateKey()
*/
QSslCertificate QSslSocket::localCertificate() const
{
    Q_D(const QSslSocket);
    if (d->configuration.localCertificateChain.isEmpty())
        return QSslCertificate();
    return d->configuration.localCertificateChain[0];
}

/*!
    Returns the peer's digital certificate (i.e., the immediate
    certificate of the host you are connected to), or a null
    certificate, if the peer has not assigned a certificate.

    The peer certificate is checked automatically during the
    handshake phase, so this function is normally used to fetch
    the certificate for display or for connection diagnostic
    purposes. It contains information about the peer, including
    its host name, the certificate issuer, and the peer's public
    key.

    Because the peer certificate is set during the handshake phase, it
    is safe to access the peer certificate from a slot connected to
    the sslErrors() signal or the encrypted() signal.

    If a null certificate is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to check the peer's complete chain of certificates,
    use peerCertificateChain() to get them all at once.

    \sa peerCertificateChain()
*/
QSslCertificate QSslSocket::peerCertificate() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerCertificate;
}

/*!
    Returns the peer's chain of digital certificates, or an empty list
    of certificates.

    Peer certificates are checked automatically during the handshake
    phase. This function is normally used to fetch certificates for
    display, or for performing connection diagnostics. Certificates
    contain information about the peer and the certificate issuers,
    including host name, issuer names, and issuer public keys.

    The peer certificates are set in QSslSocket during the handshake
    phase, so it is safe to call this function from a slot connected
    to the sslErrors() signal or the encrypted() signal.

    If an empty list is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to get only the peer's immediate certificate, use
    peerCertificate().

    \sa peerCertificate()
*/
QList<QSslCertificate> QSslSocket::peerCertificateChain() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerCertificateChain;
}

/*!
    Returns the socket's cryptographic \l {QSslCipher} {cipher}, or a
    null cipher if the connection isn't encrypted. The socket's cipher
    for the session is set during the handshake phase. The cipher is
    used to encrypt and decrypt data transmitted through the socket.

    QSslSocket also provides functions for setting the ordered list of
    ciphers from which the handshake phase will eventually select the
    session cipher. This ordered list must be in place before the
    handshake phase begins.

    \sa QSslConfiguration::ciphers(), QSslConfiguration::setCiphers(),
        QSslConfiguration::setCiphers(),
        QSslConfiguration::ciphers(),
        QSslConfiguration::supportedCiphers()
*/
QSslCipher QSslSocket::sessionCipher() const
{
    Q_D(const QSslSocket);
    return d->sessionCipher();
}

/*!
    Returns the socket's SSL/TLS protocol or UnknownProtocol if the
    connection isn't encrypted. The socket's protocol for the session
    is set during the handshake phase.

    \sa protocol(), setProtocol()
    \since 5.4
*/
QSsl::SslProtocol QSslSocket::sessionProtocol() const
{
    Q_D(const QSslSocket);
    return d->sessionProtocol();
}

/*!
    \since 5.13

    This function returns Online Certificate Status Protocol responses that
    a server may send during a TLS handshake using OCSP stapling. The list
    is empty if no definitive response or no response at all was received.

    \sa QSslConfiguration::setOcspStaplingEnabled()
*/
QList<QOcspResponse> QSslSocket::ocspResponses() const
{
    Q_D(const QSslSocket);
    if (const auto *backend = d->backend.get())
        return backend->ocsps();
    return {};
}

/*!
    Sets the socket's private \l {QSslKey} {key} to \a key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to
    SSL peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.

    \sa privateKey(), setLocalCertificate()
*/
void QSslSocket::setPrivateKey(const QSslKey &key)
{
    Q_D(QSslSocket);
    d->configuration.privateKey = key;
}

/*!
    \overload

    Reads the string in file \a fileName and decodes it using
    a specified \a algorithm and encoding \a format to construct
    an \l {QSslKey} {SSL key}. If the encoded key is encrypted,
    \a passPhrase is used to decrypt it.

    The socket's private key is set to the constructed key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to SSL
    peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.

    \sa privateKey(), setLocalCertificate()
*/
void QSslSocket::setPrivateKey(const QString &fileName, QSsl::KeyAlgorithm algorithm,
                               QSsl::EncodingFormat format, const QByteArray &passPhrase)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcSsl, "QSslSocket::setPrivateKey: Couldn't open file for reading");
        return;
    }

    QSslKey key(file.readAll(), algorithm, format, QSsl::PrivateKey, passPhrase);
    if (key.isNull()) {
        qCWarning(lcSsl, "QSslSocket::setPrivateKey: "
                         "The specified file does not contain a valid key");
        return;
    }

    Q_D(QSslSocket);
    d->configuration.privateKey = key;
}

/*!
    Returns this socket's private key.

    \sa setPrivateKey(), localCertificate()
*/
QSslKey QSslSocket::privateKey() const
{
    Q_D(const QSslSocket);
    return d->configuration.privateKey;
}

/*!
    Waits until the socket is connected, or \a msecs milliseconds,
    whichever happens first. If the connection has been established,
    this function returns \c true; otherwise it returns \c false.

    \sa QAbstractSocket::waitForConnected()
*/
bool QSslSocket::waitForConnected(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    bool retVal = d->plainSocket->waitForConnected(msecs);
    if (!retVal) {
        setSocketState(d->plainSocket->state());
        d->setError(d->plainSocket->error(), d->plainSocket->errorString());
    }
    return retVal;
}

/*!
    Waits until the socket has completed the SSL handshake and has
    emitted encrypted(), or \a msecs milliseconds, whichever comes
    first. If encrypted() has been emitted, this function returns
    true; otherwise (e.g., the socket is disconnected, or the SSL
    handshake fails), false is returned.

    The following example waits up to one second for the socket to be
    encrypted:

    \snippet code/src_network_ssl_qsslsocket.cpp 5

    If msecs is -1, this function will not time out.

    \sa startClientEncryption(), startServerEncryption(), encrypted(), isEncrypted()
*/
bool QSslSocket::waitForEncrypted(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket || d->connectionEncrypted)
        return false;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return false;
    if (!d->verifyProtocolSupported("QSslSocket::waitForEncrypted:"))
        return false;

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (d->plainSocket->state() != QAbstractSocket::ConnectedState) {
        // Wait until we've entered connected state.
        if (!d->plainSocket->waitForConnected(msecs))
            return false;
    }

    while (!d->connectionEncrypted) {
        // Start the handshake, if this hasn't been started yet.
        if (d->mode == UnencryptedMode)
            startClientEncryption();
        // Loop, waiting until the connection has been encrypted or an error
        // occurs.
        if (!d->plainSocket->waitForReadyRead(qt_subtract_from_timeout(msecs, stopWatch.elapsed())))
            return false;
    }
    return d->connectionEncrypted;
}

/*!
    \reimp
*/
bool QSslSocket::waitForReadyRead(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->waitForReadyRead(msecs);

    // This function must return true if and only if readyRead() *was* emitted.
    // So we initialize "readyReadEmitted" to false and check if it was set to true.
    // waitForReadyRead() could be called recursively, so we can't use the same variable
    // (the inner waitForReadyRead() may fail, but the outer one still succeeded)
    bool readyReadEmitted = false;
    bool *previousReadyReadEmittedPointer = d->readyReadEmittedPointer;
    d->readyReadEmittedPointer = &readyReadEmitted;

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs)) {
            d->readyReadEmittedPointer = previousReadyReadEmittedPointer;
            return false;
        }
    }

    if (!d->writeBuffer.isEmpty()) {
        // empty our cleartext write buffer first
        d->transmit();
    }

    // test readyReadEmitted first because either operation above
    // (waitForEncrypted or transmit) may have set it
    while (!readyReadEmitted &&
           d->plainSocket->waitForReadyRead(qt_subtract_from_timeout(msecs, stopWatch.elapsed()))) {
    }

    d->readyReadEmittedPointer = previousReadyReadEmittedPointer;
    return readyReadEmitted;
}

/*!
    \reimp
*/
bool QSslSocket::waitForBytesWritten(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    if (d->mode == UnencryptedMode)
        return d->plainSocket->waitForBytesWritten(msecs);

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs))
            return false;
    }
    if (!d->writeBuffer.isEmpty()) {
        // empty our cleartext write buffer first
        d->transmit();
    }

    return d->plainSocket->waitForBytesWritten(qt_subtract_from_timeout(msecs, stopWatch.elapsed()));
}

/*!
    Waits until the socket has disconnected or \a msecs milliseconds,
    whichever comes first. If the connection has been disconnected,
    this function returns \c true; otherwise it returns \c false.

    \sa QAbstractSocket::waitForDisconnected()
*/
bool QSslSocket::waitForDisconnected(int msecs)
{
    Q_D(QSslSocket);

    // require calling connectToHost() before waitForDisconnected()
    if (state() == UnconnectedState) {
        qCWarning(lcSsl, "QSslSocket::waitForDisconnected() is not allowed in UnconnectedState");
        return false;
    }

    if (!d->plainSocket)
        return false;
    // Forward to the plain socket unless the connection is secure.
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->waitForDisconnected(msecs);

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs))
            return false;
    }
    // We are delaying the disconnect, if the write buffer is not empty.
    // So, start the transmission.
    if (!d->writeBuffer.isEmpty())
        d->transmit();

    // At this point, the socket might be disconnected, if disconnectFromHost()
    // was called just after the connectToHostEncrypted() call. Also, we can
    // lose the connection as a result of the transmit() call.
    if (state() == UnconnectedState)
        return true;

    bool retVal = d->plainSocket->waitForDisconnected(qt_subtract_from_timeout(msecs, stopWatch.elapsed()));
    if (!retVal) {
        setSocketState(d->plainSocket->state());
        d->setError(d->plainSocket->error(), d->plainSocket->errorString());
    }
    return retVal;
}

/*!
    \since 5.15

    Returns a list of the last SSL errors that occurred. This is the
    same list as QSslSocket passes via the sslErrors() signal. If the
    connection has been encrypted with no errors, this function will
    return an empty list.

    \sa connectToHostEncrypted()
*/
QList<QSslError> QSslSocket::sslHandshakeErrors() const
{
    Q_D(const QSslSocket);
    if (const auto *backend = d->backend.get())
        return backend->tlsErrors();
    return {};
}

/*!
    Returns \c true if this platform supports SSL; otherwise, returns
    false. If the platform doesn't support SSL, the socket will fail
    in the connection phase.
*/
bool QSslSocket::supportsSsl()
{
    return QSslSocketPrivate::supportsSsl();
}

/*!
    \since 5.0
    Returns the version number of the SSL library in use. Note that
    this is the version of the library in use at run-time not compile
    time. If no SSL support is available then this will return -1.
*/
long QSslSocket::sslLibraryVersionNumber()
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        return tlsBackend->tlsLibraryVersionNumber();

    return -1;
}

/*!
    \since 5.0
    Returns the version string of the SSL library in use. Note that
    this is the version of the library in use at run-time not compile
    time. If no SSL support is available then this will return an empty value.
*/
QString QSslSocket::sslLibraryVersionString()
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        return tlsBackend->tlsLibraryVersionString();
    return {};
}

/*!
    \since 5.4
    Returns the version number of the SSL library in use at compile
    time. If no SSL support is available then this will return -1.

    \sa sslLibraryVersionNumber()
*/
long QSslSocket::sslLibraryBuildVersionNumber()
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        return tlsBackend->tlsLibraryBuildVersionNumber();
    return -1;
}

/*!
    \since 5.4
    Returns the version string of the SSL library in use at compile
    time. If no SSL support is available then this will return an
    empty value.

    \sa sslLibraryVersionString()
*/
QString QSslSocket::sslLibraryBuildVersionString()
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        return tlsBackend->tlsLibraryBuildVersionString();

    return {};
}

/*!
    \since 6.1
    Returns the names of the currently available backends. These names
    are in lower case, e.g. "openssl", "securetransport", "schannel"
    (similar to the already existing feature names for TLS backends in Qt).

    \sa activeBackend()
*/
QList<QString> QSslSocket::availableBackends()
{
    return QTlsBackend::availableBackendNames();
}

/*!
    \since 6.1
    Returns the name of the backend that QSslSocket and related classes
    use. If the active backend was not set explicitly, this function
    returns the name of a default backend that QSslSocket selects implicitly
    from the list of available backends.

    \note When selecting a default backend implicitly, QSslSocket prefers
    the OpenSSL backend if available. If it's not available, the Schannel backend
    is implicitly selected on Windows, and Secure Transport on Darwin platforms.
    Failing these, if a custom TLS backend is found, it is used.
    If no other backend is found, the "certificate only" backend is selected.
    For more information about TLS plugins, please see
    \l {Enabling and Disabling SSL Support when Building Qt from Source}.

    \sa setActiveBackend(), availableBackends()
*/
QString QSslSocket::activeBackend()
{
    const QMutexLocker locker(&QSslSocketPrivate::backendMutex);

    if (!QSslSocketPrivate::activeBackendName.size())
        QSslSocketPrivate::activeBackendName = QTlsBackend::defaultBackendName();

    return QSslSocketPrivate::activeBackendName;
}

/*!
    \since 6.1
    Returns true if a backend with name \a backendName was set as
    active backend. \a backendName must be one of names returned
    by availableBackends().

    \note An application cannot mix different backends simultaneously.
    This implies that a non-default backend must be selected prior
    to any use of QSslSocket or related classes, e.g. QSslCertificate
    or QSslKey.

    \sa activeBackend(), availableBackends()
*/
bool QSslSocket::setActiveBackend(const QString &backendName)
{
    if (!backendName.size()) {
        qCWarning(lcSsl, "Invalid parameter (backend name cannot be an empty string)");
        return false;
    }

    QMutexLocker locker(&QSslSocketPrivate::backendMutex);
    if (QSslSocketPrivate::tlsBackend) {
        qCWarning(lcSsl) << "Cannot set backend named" << backendName
                         << "as active, another backend is already in use";
        locker.unlock();
        return activeBackend() == backendName;
    }

    if (!QTlsBackend::availableBackendNames().contains(backendName)) {
        qCWarning(lcSsl) << "Cannot set unavailable backend named" << backendName
                         << "as active";
        return false;
    }

    QSslSocketPrivate::activeBackendName = backendName;

    return true;
}

/*!
    \since 6.1
    If a backend with name \a backendName is available, this function returns the
    list of TLS protocol versions supported by this backend. An empty \a backendName
    is understood as a query about the currently active backend. Otherwise, this
    function returns an empty list.

    \sa availableBackends(), activeBackend(), isProtocolSupported()
*/
QList<QSsl::SslProtocol> QSslSocket::supportedProtocols(const QString &backendName)
{
    return QTlsBackend::supportedProtocols(backendName.size() ? backendName : activeBackend());
}

/*!
    \since 6.1
    Returns true if \a protocol is supported by a backend named \a backendName. An empty
    \a backendName is understood as a query about the currently active backend.

    \sa supportedProtocols()
*/
bool QSslSocket::isProtocolSupported(QSsl::SslProtocol protocol, const QString &backendName)
{
    const auto versions = supportedProtocols(backendName);
    return versions.contains(protocol);
}

/*!
    \since 6.1
    This function returns backend-specific classes implemented by the backend named
    \a backendName.  An empty \a backendName is understood as a query about the
    currently active backend.

    \sa QSsl::ImplementedClass, activeBackend(), isClassImplemented()
*/
QList<QSsl::ImplementedClass> QSslSocket::implementedClasses(const QString &backendName)
{
    return QTlsBackend::implementedClasses(backendName.size() ? backendName : activeBackend());
}

/*!
    \since 6.1
    Returns true if a class \a cl is implemented by the backend named \a backendName. An empty
    \a backendName is understood as a query about the currently active backend.

    \sa implementedClasses()
*/

bool QSslSocket::isClassImplemented(QSsl::ImplementedClass cl, const QString &backendName)
{
    return implementedClasses(backendName).contains(cl);
}

/*!
    \since 6.1
    This function returns features supported by a backend named \a backendName.
    An empty \a backendName is understood as a query about the currently active backend.

    \sa QSsl::SupportedFeature, activeBackend()
*/
QList<QSsl::SupportedFeature> QSslSocket::supportedFeatures(const QString &backendName)
{
    return QTlsBackend::supportedFeatures(backendName.size() ? backendName : activeBackend());
}

/*!
    \since 6.1
    Returns true if a feature \a ft is supported by a backend named \a backendName. An empty
    \a backendName is understood as a query about the currently active backend.

    \sa QSsl::SupportedFeature, supportedFeatures()
*/
bool QSslSocket::isFeatureSupported(QSsl::SupportedFeature ft, const QString &backendName)
{
    return supportedFeatures(backendName).contains(ft);
}

/*!
    Starts a delayed SSL handshake for a client connection. This
    function can be called when the socket is in the \l ConnectedState
    but still in the \l UnencryptedMode. If it is not yet connected,
    or if it is already encrypted, this function has no effect.

    Clients that implement STARTTLS functionality often make use of
    delayed SSL handshakes. Most other clients can avoid calling this
    function directly by using connectToHostEncrypted() instead, which
    automatically performs the handshake.

    \sa connectToHostEncrypted(), startServerEncryption()
*/
void QSslSocket::startClientEncryption()
{
    Q_D(QSslSocket);
    if (d->mode != UnencryptedMode) {
        qCWarning(lcSsl,
                  "QSslSocket::startClientEncryption: cannot start handshake on non-plain connection");
        return;
    }
    if (state() != ConnectedState) {
        qCWarning(lcSsl,
                  "QSslSocket::startClientEncryption: cannot start handshake when not connected");
        return;
    }

    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::startClientEncryption: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }

    if (!d->verifyProtocolSupported("QSslSocket::startClientEncryption:"))
        return;

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::startClientEncryption()";
#endif
    d->mode = SslClientMode;
    emit modeChanged(d->mode);
    d->startClientEncryption();
}

/*!
    Starts a delayed SSL handshake for a server connection. This
    function can be called when the socket is in the \l ConnectedState
    but still in \l UnencryptedMode. If it is not connected or it is
    already encrypted, the function has no effect.

    For server sockets, calling this function is the only way to
    initiate the SSL handshake. Most servers will call this function
    immediately upon receiving a connection, or as a result of having
    received a protocol-specific command to enter SSL mode (e.g, the
    server may respond to receiving the string "STARTTLS\\r\\n" by
    calling this function).

    The most common way to implement an SSL server is to create a
    subclass of QTcpServer and reimplement
    QTcpServer::incomingConnection(). The returned socket descriptor
    is then passed to QSslSocket::setSocketDescriptor().

    \sa connectToHostEncrypted(), startClientEncryption()
*/
void QSslSocket::startServerEncryption()
{
    Q_D(QSslSocket);
    if (d->mode != UnencryptedMode) {
        qCWarning(lcSsl, "QSslSocket::startServerEncryption: cannot start handshake on non-plain connection");
        return;
    }
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::startServerEncryption()";
#endif
    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::startServerEncryption: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }
    if (!d->verifyProtocolSupported("QSslSocket::startServerEncryption"))
        return;

    d->mode = SslServerMode;
    emit modeChanged(d->mode);
    d->startServerEncryption();
}

/*!
    This slot tells QSslSocket to ignore errors during QSslSocket's
    handshake phase and continue connecting. If you want to continue
    with the connection even if errors occur during the handshake
    phase, then you must call this slot, either from a slot connected
    to sslErrors(), or before the handshake phase. If you don't call
    this slot, either in response to errors or before the handshake,
    the connection will be dropped after the sslErrors() signal has
    been emitted.

    If there are no errors during the SSL handshake phase (i.e., the
    identity of the peer is established with no problems), QSslSocket
    will not emit the sslErrors() signal, and it is unnecessary to
    call this function.

    \warning Be sure to always let the user inspect the errors
    reported by the sslErrors() signal, and only call this method
    upon confirmation from the user that proceeding is ok.
    If there are unexpected errors, the connection should be aborted.
    Calling this method without inspecting the actual errors will
    most likely pose a security risk for your application. Use it
    with great care!

    \sa sslErrors()
*/
void QSslSocket::ignoreSslErrors()
{
    Q_D(QSslSocket);
    d->ignoreAllSslErrors = true;
}

/*!
    \overload
    \since 4.6

    This method tells QSslSocket to ignore only the errors given in \a
    errors.

    \note Because most SSL errors are associated with a certificate, for most
    of them you must set the expected certificate this SSL error is related to.
    If, for instance, you want to connect to a server that uses
    a self-signed certificate, consider the following snippet:

    \snippet code/src_network_ssl_qsslsocket.cpp 6

    Multiple calls to this function will replace the list of errors that
    were passed in previous calls.
    You can clear the list of errors you want to ignore by calling this
    function with an empty list.

    \sa sslErrors(), sslHandshakeErrors()
*/
void QSslSocket::ignoreSslErrors(const QList<QSslError> &errors)
{
    Q_D(QSslSocket);
    d->ignoreErrorsList = errors;
}


/*!
    \since 6.0

    If an application wants to conclude a handshake even after receiving
    handshakeInterruptedOnError() signal, it must call this function.
    This call must be done from a slot function attached to the signal.
    The signal-slot connection must be direct.

    \sa handshakeInterruptedOnError(), QSslConfiguration::setHandshakeMustInterruptOnError()
*/
void QSslSocket::continueInterruptedHandshake()
{
    Q_D(QSslSocket);
    if (auto *backend = d->backend.get())
        backend->enableHandshakeContinuation();
}

/*!
    \internal
*/
void QSslSocket::connectToHost(const QString &hostName, quint16 port, OpenMode openMode, NetworkLayerProtocol protocol)
{
    Q_D(QSslSocket);
    d->preferredNetworkLayerProtocol = protocol;
    if (!d->initialized)
        d->init();
    d->initialized = false;

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::connectToHost("
             << hostName << ',' << port << ',' << openMode << ')';
#endif
    if (!d->plainSocket) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "\tcreating internal plain socket";
#endif
        d->createPlainSocket(openMode);
    }
#ifndef QT_NO_NETWORKPROXY
    d->plainSocket->setProtocolTag(d->protocolTag);
    d->plainSocket->setProxy(proxy());
#endif
    QIODevice::open(openMode);
    d->readChannelCount = d->writeChannelCount = 0;
    d->plainSocket->connectToHost(hostName, port, openMode, d->preferredNetworkLayerProtocol);
    d->cachedSocketDescriptor = d->plainSocket->socketDescriptor();
}

/*!
    \internal
*/
void QSslSocket::disconnectFromHost()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::disconnectFromHost()";
#endif
    if (!d->plainSocket)
        return;
    if (d->state == UnconnectedState)
        return;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake) {
        d->plainSocket->disconnectFromHost();
        return;
    }
    if (d->state <= ConnectingState) {
        d->pendingClose = true;
        return;
    }
    // Make sure we don't process any signal from the CA fetcher
    // (Windows):
    if (auto *backend = d->backend.get())
        backend->cancelCAFetch();

    // Perhaps emit closing()
    if (d->state != ClosingState) {
        d->state = ClosingState;
        emit stateChanged(d->state);
    }

    if (!d->writeBuffer.isEmpty()) {
        d->pendingClose = true;
        return;
    }

    if (d->mode == UnencryptedMode) {
        d->plainSocket->disconnectFromHost();
    } else {
        d->disconnectFromHost();
    }
}

/*!
    \reimp
*/
qint64 QSslSocket::readData(char *data, qint64 maxlen)
{
    Q_D(QSslSocket);
    qint64 readBytes = 0;

    if (d->mode == UnencryptedMode && !d->autoStartHandshake) {
        readBytes = d->plainSocket->read(data, maxlen);
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "QSslSocket::readData(" << (void *)data << ',' << maxlen << ") =="
                 << readBytes;
#endif
    } else {
        // possibly trigger another transmit() to decrypt more data from the socket
        if (d->plainSocket->bytesAvailable() || d->hasUndecryptedData())
            QMetaObject::invokeMethod(this, "_q_flushReadBuffer", Qt::QueuedConnection);
        else if (d->state != QAbstractSocket::ConnectedState)
            return maxlen ? qint64(-1) : qint64(0);
    }

    return readBytes;
}

/*!
    \reimp
*/
qint64 QSslSocket::writeData(const char *data, qint64 len)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::writeData(" << (void *)data << ',' << len << ')';
#endif
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->write(data, len);

    d->write(data, len);

    // make sure we flush to the plain socket's buffer
    if (!d->flushTriggered) {
        d->flushTriggered = true;
        QMetaObject::invokeMethod(this, "_q_flushWriteBuffer", Qt::QueuedConnection);
    }

    return len;
}

bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;

/*!
    \internal
*/
QSslSocketPrivate::QSslSocketPrivate()
    : initialized(false)
    , mode(QSslSocket::UnencryptedMode)
    , autoStartHandshake(false)
    , connectionEncrypted(false)
    , ignoreAllSslErrors(false)
    , readyReadEmittedPointer(nullptr)
    , allowRootCertOnDemandLoading(true)
    , plainSocket(nullptr)
    , paused(false)
    , flushTriggered(false)
{
    QSslConfigurationPrivate::deepCopyDefaultConfiguration(&configuration);
    // If the global configuration doesn't allow root certificates to be loaded
    // on demand then we have to disable it for this socket as well.
    if (!configuration.allowRootCertOnDemandLoading)
        allowRootCertOnDemandLoading = false;

    const auto *tlsBackend = tlsBackendInUse();
    if (!tlsBackend) {
        qCWarning(lcSsl, "No TLS backend is available");
        return;
    }
    backend.reset(tlsBackend->createTlsCryptograph());
    if (!backend.get()) {
        qCWarning(lcSsl) << "The backend named" << tlsBackend->backendName()
                         << "does not support TLS";
    }
}

/*!
    \internal
*/
QSslSocketPrivate::~QSslSocketPrivate()
{
}

/*!
    \internal
*/
bool QSslSocketPrivate::supportsSsl()
{
    if (const auto *tlsBackend = tlsBackendInUse())
        return tlsBackend->implementedClasses().contains(QSsl::ImplementedClass::Socket);
    return false;
}

/*!
    \internal

    Declared static in QSslSocketPrivate, makes sure the SSL libraries have
    been initialized.
*/
void QSslSocketPrivate::ensureInitialized()
{
    if (!supportsSsl())
        return;

    const auto *tlsBackend = tlsBackendInUse();
    Q_ASSERT(tlsBackend);
    tlsBackend->ensureInitialized();
}

/*!
    \internal
*/
void QSslSocketPrivate::init()
{
    // TLSTODO: delete those data members.
    mode = QSslSocket::UnencryptedMode;
    autoStartHandshake = false;
    connectionEncrypted = false;
    ignoreAllSslErrors = false;
    abortCalled = false;
    pendingClose = false;
    flushTriggered = false;
    // We don't want to clear the ignoreErrorsList, so
    // that it is possible setting it before connecting.

    buffer.clear();
    writeBuffer.clear();
    configuration.peerCertificate.clear();
    configuration.peerCertificateChain.clear();

    if (backend.get()) {
        Q_ASSERT(q_ptr);
        backend->init(static_cast<QSslSocket *>(q_ptr), this);
    }
}

/*!
    \internal
*/
bool QSslSocketPrivate::verifyProtocolSupported(const char *where)
{
    auto protocolName = "DTLS"_L1;
    switch (configuration.protocol) {
    case QSsl::UnknownProtocol:
        // UnknownProtocol, according to our docs, is for cipher whose protocol is unknown.
        // Should not be used when configuring QSslSocket.
        protocolName = "UnknownProtocol"_L1;
        Q_FALLTHROUGH();
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::DtlsV1_0:
    case QSsl::DtlsV1_2:
    case QSsl::DtlsV1_0OrLater:
    case QSsl::DtlsV1_2OrLater:
        qCWarning(lcSsl) << where << "QSslConfiguration with unexpected protocol" << protocolName;
        setErrorAndEmit(QAbstractSocket::SslInvalidUserDataError,
                        QSslSocket::tr("Attempted to use an unsupported protocol."));
        return false;
QT_WARNING_POP
    default:
        return true;
    }
}

/*!
    \internal
*/
QList<QSslCipher> QSslSocketPrivate::defaultCiphers()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->config->ciphers;
}

/*!
    \internal
*/
QList<QSslCipher> QSslSocketPrivate::supportedCiphers()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->supportedCiphers;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->ciphers = ciphers;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->supportedCiphers = ciphers;
}

/*!
    \internal
*/
void QSslSocketPrivate::resetDefaultEllipticCurves()
{
    const auto *tlsBackend = tlsBackendInUse();
    if (!tlsBackend)
        return;

    auto ids = tlsBackend->ellipticCurvesIds();
    if (!ids.size())
        return;

    QList<QSslEllipticCurve> curves;
    curves.reserve(ids.size());
    for (int id : ids) {
        QSslEllipticCurve curve;
        curve.id = id;
        curves.append(curve);
    }

    // Set the list of supported ECs, but not the list
    // of *default* ECs. OpenSSL doesn't like forcing an EC for the wrong
    // ciphersuite, so don't try it -- leave the empty list to mean
    // "the implementation will choose the most suitable one".
    setDefaultSupportedEllipticCurves(curves);
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->ciphers = ciphers;
}

/*!
    \internal
*/
QList<QSslCipher> QSslSocketPrivate::defaultDtlsCiphers()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->dtlsConfig->ciphers;
}

/*!
    \internal
*/
QList<QSslEllipticCurve> QSslSocketPrivate::supportedEllipticCurves()
{
    QSslSocketPrivate::ensureInitialized();
    const QMutexLocker locker(&globalData()->mutex);
    return globalData()->supportedEllipticCurves;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultSupportedEllipticCurves(const QList<QSslEllipticCurve> &curves)
{
    const QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->dtlsConfig.detach();
    globalData()->supportedEllipticCurves = curves;
}

/*!
    \internal
*/
QList<QSslCertificate> QSslSocketPrivate::defaultCaCertificates()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->config->caCertificates;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->caCertificates = certs;
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->caCertificates = certs;
    // when the certificates are set explicitly, we do not want to
    // load the system certificates on demand
    s_loadRootCertsOnDemand = false;
}

/*!
    \internal
*/
void QSslSocketPrivate::addDefaultCaCertificate(const QSslCertificate &cert)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    if (globalData()->config->caCertificates.contains(cert))
        return;
    globalData()->config.detach();
    globalData()->config->caCertificates += cert;
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->caCertificates += cert;
}

/*!
    \internal
*/
void QSslSocketPrivate::addDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->caCertificates += certs;
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->caCertificates += certs;
}

/*!
    \internal
*/
QSslConfiguration QSslConfigurationPrivate::defaultConfiguration()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return QSslConfiguration(globalData()->config.data());
}

/*!
    \internal
*/
void QSslConfigurationPrivate::setDefaultConfiguration(const QSslConfiguration &configuration)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    if (globalData()->config == configuration.d)
        return;                 // nothing to do

    globalData()->config = const_cast<QSslConfigurationPrivate*>(configuration.d.constData());
}

/*!
    \internal
*/
void QSslConfigurationPrivate::deepCopyDefaultConfiguration(QSslConfigurationPrivate *ptr)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    const QSslConfigurationPrivate *global = globalData()->config.constData();

    if (!global)
        return;

    ptr->ref.storeRelaxed(1);
    ptr->peerCertificate = global->peerCertificate;
    ptr->peerCertificateChain = global->peerCertificateChain;
    ptr->localCertificateChain = global->localCertificateChain;
    ptr->privateKey = global->privateKey;
    ptr->sessionCipher = global->sessionCipher;
    ptr->sessionProtocol = global->sessionProtocol;
    ptr->ciphers = global->ciphers;
    ptr->caCertificates = global->caCertificates;
    ptr->allowRootCertOnDemandLoading = global->allowRootCertOnDemandLoading;
    ptr->protocol = global->protocol;
    ptr->peerVerifyMode = global->peerVerifyMode;
    ptr->peerVerifyDepth = global->peerVerifyDepth;
    ptr->sslOptions = global->sslOptions;
    ptr->ellipticCurves = global->ellipticCurves;
    ptr->backendConfig = global->backendConfig;
#if QT_CONFIG(dtls)
    ptr->dtlsCookieEnabled = global->dtlsCookieEnabled;
#endif
#if QT_CONFIG(ocsp)
    ptr->ocspStaplingEnabled = global->ocspStaplingEnabled;
#endif
#if QT_CONFIG(openssl)
    ptr->reportFromCallback = global->reportFromCallback;
    ptr->missingCertIsFatal = global->missingCertIsFatal;
#endif
}

/*!
    \internal
*/
QSslConfiguration QSslConfigurationPrivate::defaultDtlsConfiguration()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);

    return QSslConfiguration(globalData()->dtlsConfig.data());
}

/*!
    \internal
*/
void QSslConfigurationPrivate::setDefaultDtlsConfiguration(const QSslConfiguration &configuration)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    if (globalData()->dtlsConfig == configuration.d)
        return;                 // nothing to do

    globalData()->dtlsConfig = const_cast<QSslConfigurationPrivate*>(configuration.d.constData());
}

/*!
    \internal
*/
void QSslSocketPrivate::createPlainSocket(QIODevice::OpenMode openMode)
{
    Q_Q(QSslSocket);
    q->setOpenMode(openMode); // <- from QIODevice
    q->setSocketState(QAbstractSocket::UnconnectedState);
    q->setSocketError(QAbstractSocket::UnknownSocketError);
    q->setLocalPort(0);
    q->setLocalAddress(QHostAddress());
    q->setPeerPort(0);
    q->setPeerAddress(QHostAddress());
    q->setPeerName(QString());

    plainSocket = new QTcpSocket(q);
    q->connect(plainSocket, SIGNAL(connected()),
               q, SLOT(_q_connectedSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(hostFound()),
               q, SLOT(_q_hostFoundSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(disconnected()),
               q, SLOT(_q_disconnectedSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
               q, SLOT(_q_stateChangedSlot(QAbstractSocket::SocketState)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
               q, SLOT(_q_errorSlot(QAbstractSocket::SocketError)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(readyRead()),
               q, SLOT(_q_readyReadSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(channelReadyRead(int)),
               q, SLOT(_q_channelReadyReadSlot(int)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(bytesWritten(qint64)),
               q, SLOT(_q_bytesWrittenSlot(qint64)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(channelBytesWritten(int,qint64)),
               q, SLOT(_q_channelBytesWrittenSlot(int,qint64)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(readChannelFinished()),
               q, SLOT(_q_readChannelFinishedSlot()),
               Qt::DirectConnection);
#ifndef QT_NO_NETWORKPROXY
    q->connect(plainSocket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
               q, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
#endif

    buffer.clear();
    writeBuffer.clear();
    connectionEncrypted = false;
    configuration.peerCertificate.clear();
    configuration.peerCertificateChain.clear();
    mode = QSslSocket::UnencryptedMode;
    q->setReadBufferSize(readBufferMaxSize);
}

void QSslSocketPrivate::pauseSocketNotifiers(QSslSocket *socket)
{
    if (!socket->d_func()->plainSocket)
        return;
    QAbstractSocketPrivate::pauseSocketNotifiers(socket->d_func()->plainSocket);
}

void QSslSocketPrivate::resumeSocketNotifiers(QSslSocket *socket)
{
    if (!socket->d_func()->plainSocket)
        return;
    QAbstractSocketPrivate::resumeSocketNotifiers(socket->d_func()->plainSocket);
}

bool QSslSocketPrivate::isPaused() const
{
    return paused;
}

void QSslSocketPrivate::setPaused(bool p)
{
    paused = p;
}

bool QSslSocketPrivate::bind(const QHostAddress &address, quint16 port, QAbstractSocket::BindMode mode)
{
    // this function is called from QAbstractSocket::bind
    if (!initialized)
        init();
    initialized = false;

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::bind(" << address << ',' << port << ',' << mode << ')';
#endif
    if (!plainSocket) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "\tcreating internal plain socket";
#endif
        createPlainSocket(QIODevice::ReadWrite);
    }
    bool ret = plainSocket->bind(address, port, mode);
    localPort = plainSocket->localPort();
    localAddress = plainSocket->localAddress();
    cachedSocketDescriptor = plainSocket->socketDescriptor();
    readChannelCount = writeChannelCount = 0;
    return ret;
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_connectedSlot()
{
    Q_Q(QSslSocket);
    q->setLocalPort(plainSocket->localPort());
    q->setLocalAddress(plainSocket->localAddress());
    q->setPeerPort(plainSocket->peerPort());
    q->setPeerAddress(plainSocket->peerAddress());
    q->setPeerName(plainSocket->peerName());
    cachedSocketDescriptor = plainSocket->socketDescriptor();
    readChannelCount = plainSocket->readChannelCount();
    writeChannelCount = plainSocket->writeChannelCount();

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_connectedSlot()";
    qCDebug(lcSsl) << "\tstate =" << q->state();
    qCDebug(lcSsl) << "\tpeer =" << q->peerName() << q->peerAddress() << q->peerPort();
    qCDebug(lcSsl) << "\tlocal =" << QHostInfo::fromName(q->localAddress().toString()).hostName()
             << q->localAddress() << q->localPort();
#endif

    if (autoStartHandshake)
        q->startClientEncryption();

    emit q->connected();

    if (pendingClose && !autoStartHandshake) {
        pendingClose = false;
        q->disconnectFromHost();
    }
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_hostFoundSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_hostFoundSlot()";
    qCDebug(lcSsl) << "\tstate =" << q->state();
#endif
    emit q->hostFound();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_disconnectedSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_disconnectedSlot()";
    qCDebug(lcSsl) << "\tstate =" << q->state();
#endif
    disconnected();
    emit q->disconnected();

    q->setLocalPort(0);
    q->setLocalAddress(QHostAddress());
    q->setPeerPort(0);
    q->setPeerAddress(QHostAddress());
    q->setPeerName(QString());
    cachedSocketDescriptor = -1;
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_stateChangedSlot(QAbstractSocket::SocketState state)
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_stateChangedSlot(" << state << ')';
#endif
    q->setSocketState(state);
    emit q->stateChanged(state);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_errorSlot(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
#ifdef QSSLSOCKET_DEBUG
    Q_Q(QSslSocket);
    qCDebug(lcSsl) << "QSslSocket::_q_errorSlot(" << error << ')';
    qCDebug(lcSsl) << "\tstate =" << q->state();
    qCDebug(lcSsl) << "\terrorString =" << q->errorString();
#endif
    // this moves encrypted bytes from plain socket into our buffer
    if (plainSocket->bytesAvailable() && mode != QSslSocket::UnencryptedMode) {
        qint64 tmpReadBufferMaxSize = readBufferMaxSize;
        readBufferMaxSize = 0; // reset temporarily so the plain sockets completely drained drained
        transmit();
        readBufferMaxSize = tmpReadBufferMaxSize;
    }

    setErrorAndEmit(plainSocket->error(), plainSocket->errorString());
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_readyReadSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_readyReadSlot() -" << plainSocket->bytesAvailable() << "bytes available";
#endif
    if (mode == QSslSocket::UnencryptedMode) {
        if (readyReadEmittedPointer)
            *readyReadEmittedPointer = true;
        emit q->readyRead();
        return;
    }

    transmit();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_channelReadyReadSlot(int channel)
{
    Q_Q(QSslSocket);
    if (mode == QSslSocket::UnencryptedMode)
        emit q->channelReadyRead(channel);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_bytesWrittenSlot(qint64 written)
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_bytesWrittenSlot(" << written << ')';
#endif

    if (mode == QSslSocket::UnencryptedMode)
        emit q->bytesWritten(written);
    else
        emit q->encryptedBytesWritten(written);
    if (state == QAbstractSocket::ClosingState && writeBuffer.isEmpty())
        q->disconnectFromHost();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_channelBytesWrittenSlot(int channel, qint64 written)
{
    Q_Q(QSslSocket);
    if (mode == QSslSocket::UnencryptedMode)
        emit q->channelBytesWritten(channel, written);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_readChannelFinishedSlot()
{
    Q_Q(QSslSocket);
    emit q->readChannelFinished();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_flushWriteBuffer()
{
    Q_Q(QSslSocket);

    // need to notice if knock-on effects of this flush (e.g. a readReady() via transmit())
    // make another necessary, so clear flag before calling:
    flushTriggered = false;
    if (!writeBuffer.isEmpty())
        q->flush();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_flushReadBuffer()
{
    // trigger a read from the plainSocket into SSL
    if (mode != QSslSocket::UnencryptedMode)
        transmit();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_resumeImplementation()
{
    if (plainSocket)
        plainSocket->resume();
    paused = false;
    if (!connectionEncrypted) {
        if (verifyErrorsHaveBeenIgnored()) {
            continueHandshake();
        } else {
            const auto sslErrors = backend->tlsErrors();
            Q_ASSERT(!sslErrors.isEmpty());
            setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, sslErrors.constFirst().errorString());
            plainSocket->disconnectFromHost();
            return;
        }
    }
    transmit();
}

/*!
    \internal
*/
bool QSslSocketPrivate::verifyErrorsHaveBeenIgnored()
{
    Q_ASSERT(backend.get());

    bool doEmitSslError;
    if (!ignoreErrorsList.empty()) {
        // check whether the errors we got are all in the list of expected errors
        // (applies only if the method QSslSocket::ignoreSslErrors(const QList<QSslError> &errors)
        // was called)
        const auto &sslErrors = backend->tlsErrors();
        doEmitSslError = false;
        for (int a = 0; a < sslErrors.size(); a++) {
            if (!ignoreErrorsList.contains(sslErrors.at(a))) {
                doEmitSslError = true;
                break;
            }
        }
    } else {
        // if QSslSocket::ignoreSslErrors(const QList<QSslError> &errors) was not called and
        // we get an SSL error, emit a signal unless we ignored all errors (by calling
        // QSslSocket::ignoreSslErrors() )
        doEmitSslError = !ignoreAllSslErrors;
    }
    return !doEmitSslError;
}

/*!
    \internal
*/
bool QSslSocketPrivate::isAutoStartingHandshake() const
{
    return autoStartHandshake;
}

/*!
    \internal
*/
bool QSslSocketPrivate::isPendingClose() const
{
    return pendingClose;
}

/*!
    \internal
*/
void QSslSocketPrivate::setPendingClose(bool pc)
{
    pendingClose = pc;
}

/*!
    \internal
*/
qint64 QSslSocketPrivate::maxReadBufferSize() const
{
    return readBufferMaxSize;
}

/*!
    \internal
*/
void QSslSocketPrivate::setMaxReadBufferSize(qint64 maxSize)
{
    readBufferMaxSize = maxSize;
}

/*!
    \internal
*/
void QSslSocketPrivate::setEncrypted(bool enc)
{
    connectionEncrypted = enc;
}

/*!
    \internal
*/
QIODevicePrivate::QRingBufferRef &QSslSocketPrivate::tlsWriteBuffer()
{
    return writeBuffer;
}

/*!
    \internal
*/
QIODevicePrivate::QRingBufferRef &QSslSocketPrivate::tlsBuffer()
{
    return buffer;
}

/*!
    \internal
*/
bool &QSslSocketPrivate::tlsEmittedBytesWritten()
{
    return emittedBytesWritten;
}

/*!
    \internal
*/
bool *QSslSocketPrivate::readyReadPointer()
{
    return readyReadEmittedPointer;
}

bool QSslSocketPrivate::hasUndecryptedData() const
{
    return backend.get() && backend->hasUndecryptedData();
}

/*!
    \internal
*/
qint64 QSslSocketPrivate::peek(char *data, qint64 maxSize)
{
    if (mode == QSslSocket::UnencryptedMode && !autoStartHandshake) {
        //unencrypted mode - do not use QIODevice::peek, as it reads ahead data from the plain socket
        //peek at data already in the QIODevice buffer (from a previous read)
        qint64 r = buffer.peek(data, maxSize, transactionPos);
        if (r == maxSize)
            return r;
        data += r;
        //peek at data in the plain socket
        if (plainSocket) {
            qint64 r2 = plainSocket->peek(data, maxSize - r);
            if (r2 < 0)
                return (r > 0 ? r : r2);
            return r + r2;
        }

        return -1;
    } else {
        //encrypted mode - the socket engine will read and decrypt data into the QIODevice buffer
        return QTcpSocketPrivate::peek(data, maxSize);
    }
}

/*!
    \internal
*/
QByteArray QSslSocketPrivate::peek(qint64 maxSize)
{
    if (mode == QSslSocket::UnencryptedMode && !autoStartHandshake) {
        //unencrypted mode - do not use QIODevice::peek, as it reads ahead data from the plain socket
        //peek at data already in the QIODevice buffer (from a previous read)
        QByteArray ret;
        ret.reserve(maxSize);
        ret.resize(buffer.peek(ret.data(), maxSize, transactionPos));
        if (ret.size() == maxSize)
            return ret;
        //peek at data in the plain socket
        if (plainSocket)
            return ret + plainSocket->peek(maxSize - ret.size());

        return QByteArray();
    } else {
        //encrypted mode - the socket engine will read and decrypt data into the QIODevice buffer
        return QTcpSocketPrivate::peek(maxSize);
    }
}

/*!
    \reimp
*/
qint64 QSslSocket::skipData(qint64 maxSize)
{
    Q_D(QSslSocket);

    if (d->mode == QSslSocket::UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->skip(maxSize);

    // In encrypted mode, the SSL backend writes decrypted data directly into the
    // QIODevice's read buffer. As this buffer is always emptied by the caller,
    // we need to wait for more incoming data.
    return (d->state == QAbstractSocket::ConnectedState) ? Q_INT64_C(0) : Q_INT64_C(-1);
}

/*!
    \internal
*/
bool QSslSocketPrivate::flush()
{
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocketPrivate::flush()";
#endif
    if (mode != QSslSocket::UnencryptedMode) {
        // encrypt any unencrypted bytes in our buffer
        transmit();
    }

    return plainSocket && plainSocket->flush();
}

/*!
    \internal
*/
void QSslSocketPrivate::startClientEncryption()
{
    if (backend.get())
        backend->startClientEncryption();
}

/*!
    \internal
*/
void QSslSocketPrivate::startServerEncryption()
{
    if (backend.get())
        backend->startServerEncryption();
}

/*!
    \internal
*/
void QSslSocketPrivate::transmit()
{
    if (backend.get())
        backend->transmit();
}

/*!
    \internal
*/
void QSslSocketPrivate::disconnectFromHost()
{
    if (backend.get())
        backend->disconnectFromHost();
}

/*!
    \internal
*/
void QSslSocketPrivate::disconnected()
{
    if (backend.get())
        backend->disconnected();
}

/*!
    \internal
*/
QSslCipher QSslSocketPrivate::sessionCipher() const
{
    if (backend.get())
        return backend->sessionCipher();

    return {};
}

/*!
    \internal
*/
QSsl::SslProtocol QSslSocketPrivate::sessionProtocol() const
{
    if (backend.get())
        return backend->sessionProtocol();

    return QSsl::UnknownProtocol;
}

/*!
    \internal
*/
void QSslSocketPrivate::continueHandshake()
{
    if (backend.get())
        backend->continueHandshake();
}

/*!
    \internal
*/
bool QSslSocketPrivate::rootCertOnDemandLoadingSupported()
{
    return s_loadRootCertsOnDemand;
}

/*!
    \internal
*/
void QSslSocketPrivate::setRootCertOnDemandLoadingSupported(bool supported)
{
    s_loadRootCertsOnDemand = supported;
}

/*!
    \internal
*/
QList<QByteArray> QSslSocketPrivate::unixRootCertDirectories()
{
    const auto ba = [](const auto &cstr) constexpr {
        return QByteArray::fromRawData(std::begin(cstr), std::size(cstr) - 1);
    };
    static const QByteArray dirs[] = {
        ba("/etc/ssl/certs/"), // (K)ubuntu, OpenSUSE, Mandriva ...
        ba("/usr/lib/ssl/certs/"), // Gentoo, Mandrake
        ba("/usr/share/ssl/"), // Red Hat pre-2004, SuSE
        ba("/etc/pki/ca-trust/extracted/pem/directory-hash/"), // Red Hat 2021+
        ba("/usr/local/ssl/"), // Normal OpenSSL Tarball
        ba("/var/ssl/certs/"), // AIX
        ba("/usr/local/ssl/certs/"), // Solaris
        ba("/etc/openssl/certs/"), // BlackBerry
        ba("/opt/openssl/certs/"), // HP-UX
        ba("/etc/ssl/"), // OpenBSD
    };
    QList<QByteArray> result = QList<QByteArray>::fromReadOnlyData(dirs);
    if constexpr (isVxworks) {
        static QByteArray vxworksCertsDir = qgetenv("VXWORKS_CERTS_DIR");
        if (!vxworksCertsDir.isEmpty())
            result.push_back(vxworksCertsDir);
    }
    return result;
}

/*!
    \internal
*/
void QSslSocketPrivate::checkSettingSslContext(QSslSocket* socket, std::shared_ptr<QSslContext> tlsContext)
{
    if (!socket)
        return;

    if (auto *backend = socket->d_func()->backend.get())
        backend->checkSettingSslContext(tlsContext);
}

/*!
    \internal
*/
std::shared_ptr<QSslContext> QSslSocketPrivate::sslContext(QSslSocket *socket)
{
    if (!socket)
        return {};

    if (const auto *backend = socket->d_func()->backend.get())
        return backend->sslContext();

    return {};
}

bool QSslSocketPrivate::isMatchingHostname(const QSslCertificate &cert, const QString &peerName)
{
    QHostAddress hostAddress(peerName);
    if (!hostAddress.isNull()) {
        const auto subjectAlternativeNames = cert.subjectAlternativeNames();
        const auto ipAddresses = subjectAlternativeNames.equal_range(QSsl::AlternativeNameEntryType::IpAddressEntry);

        for (auto it = ipAddresses.first; it != ipAddresses.second; it++) {
            if (QHostAddress(*it).isEqual(hostAddress, QHostAddress::StrictConversion))
                return true;
        }
    }

    const QString lowerPeerName = QString::fromLatin1(QUrl::toAce(peerName));
    const QStringList commonNames = cert.subjectInfo(QSslCertificate::CommonName);

    for (const QString &commonName : commonNames) {
        if (isMatchingHostname(commonName, lowerPeerName))
            return true;
    }

    const auto subjectAlternativeNames = cert.subjectAlternativeNames();
    const auto altNames = subjectAlternativeNames.equal_range(QSsl::DnsEntry);
    for (auto it = altNames.first; it != altNames.second; ++it) {
        if (isMatchingHostname(*it, lowerPeerName))
            return true;
    }

    return false;
}

/*! \internal
   Checks if the certificate's name \a cn matches the \a hostname.
   \a hostname must be normalized in ASCII-Compatible Encoding, but \a cn is not normalized
 */
bool QSslSocketPrivate::isMatchingHostname(const QString &cn, const QString &hostname)
{
    qsizetype wildcard = cn.indexOf(u'*');

    // Check this is a wildcard cert, if not then just compare the strings
    if (wildcard < 0)
        return QLatin1StringView(QUrl::toAce(cn)) == hostname;

    qsizetype firstCnDot = cn.indexOf(u'.');
    qsizetype secondCnDot = cn.indexOf(u'.', firstCnDot+1);

    // Check at least 3 components
    if ((-1 == secondCnDot) || (secondCnDot+1 >= cn.size()))
        return false;

    // Check * is last character of 1st component (ie. there's a following .)
    if (wildcard+1 != firstCnDot)
        return false;

    // Check only one star
    if (cn.lastIndexOf(u'*') != wildcard)
        return false;

    // Reject wildcard character embedded within the A-labels or U-labels of an internationalized
    // domain name (RFC6125 section 7.2)
    if (cn.startsWith("xn--"_L1, Qt::CaseInsensitive))
        return false;

    // Check characters preceding * (if any) match
    if (wildcard && QStringView{hostname}.left(wildcard).compare(QStringView{cn}.left(wildcard), Qt::CaseInsensitive) != 0)
        return false;

    // Check characters following first . match
    qsizetype hnDot = hostname.indexOf(u'.');
    if (QStringView{hostname}.mid(hnDot + 1) != QStringView{cn}.mid(firstCnDot + 1)
        && QStringView{hostname}.mid(hnDot + 1) != QLatin1StringView(QUrl::toAce(cn.mid(firstCnDot + 1)))) {
        return false;
    }

    // Check if the hostname is an IP address, if so then wildcards are not allowed
    QHostAddress addr(hostname);
    if (!addr.isNull())
        return false;

    // Ok, I guess this was a wildcard CN and the hostname matches.
    return true;
}

/*!
    \internal
*/
QTlsBackend *QSslSocketPrivate::tlsBackendInUse()
{
    const QMutexLocker locker(&backendMutex);
    if (tlsBackend)
        return tlsBackend;

    if (!activeBackendName.size())
        activeBackendName = QTlsBackend::defaultBackendName();

    if (!activeBackendName.size()) {
        qCWarning(lcSsl, "No functional TLS backend was found");
        return nullptr;
    }

    tlsBackend = QTlsBackend::findBackend(activeBackendName);
    if (tlsBackend) {
        QObject::connect(tlsBackend, &QObject::destroyed, tlsBackend, [] {
            const QMutexLocker locker(&backendMutex);
            tlsBackend = nullptr;
        },
        Qt::DirectConnection);
    }
    return tlsBackend;
}

/*!
    \internal
*/
QSslSocket::SslMode QSslSocketPrivate::tlsMode() const
{
    return mode;
}

/*!
    \internal
*/
bool QSslSocketPrivate::isRootsOnDemandAllowed() const
{
    return allowRootCertOnDemandLoading;
}

/*!
    \internal
*/
QString QSslSocketPrivate::verificationName() const
{
    return verificationPeerName;
}

/*!
    \internal
*/
QString QSslSocketPrivate::tlsHostName() const
{
    return hostName;
}

QTcpSocket *QSslSocketPrivate::plainTcpSocket() const
{
    return plainSocket;
}

/*!
    \internal
*/
QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    if (const auto *tlsBackend = tlsBackendInUse())
        return tlsBackend->systemCaCertificates();
    return {};
}

QT_END_NAMESPACE

#include "moc_qsslsocket.cpp"
