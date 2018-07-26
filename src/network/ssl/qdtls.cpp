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
#include "qudpsocket.h"
#include "qdtls_p.h"
#include "qssl_p.h"
#include "qdtls.h"

#include "qglobal.h"

#if QT_CONFIG(openssl)
#include "qdtls_openssl_p.h"
#endif // QT_CONFIG

QT_BEGIN_NAMESPACE

namespace
{

bool isDtlsProtocol(QSsl::SslProtocol protocol)
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

}

QSslConfiguration QDtlsBasePrivate::configuration() const
{
    auto copyPrivate = new QSslConfigurationPrivate(dtlsConfiguration);
    copyPrivate->ref.store(0); // the QSslConfiguration constructor refs up
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
    Q_ASSERT(isDtlsProtocol(configuration.protocol()));
    dtlsConfiguration.protocol = configuration.protocol();
    dtlsConfiguration.sslOptions = configuration.d->sslOptions;
    dtlsConfiguration.sslSession = configuration.sessionTicket();
    dtlsConfiguration.sslSessionTicketLifeTimeHint = configuration.sessionTicketLifeTimeHint();
    dtlsConfiguration.nextAllowedProtocols = configuration.allowedNextProtocols();
    dtlsConfiguration.nextNegotiatedProtocol = configuration.nextNegotiatedProtocol();
    dtlsConfiguration.nextProtocolNegotiationStatus = configuration.nextProtocolNegotiationStatus();
    dtlsConfiguration.dtlsCookieEnabled = configuration.dtlsCookieVerificationEnabled();

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

QDtlsClientVerifier::QDtlsClientVerifier(QObject *parent)
#if QT_CONFIG(openssl)
    : QObject(*new QDtlsClientVerifierOpenSSL, parent)
#endif // openssl
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

bool QDtlsClientVerifier::setCookieGeneratorParameters(const GeneratorParameters &params)
{
    Q_D(QDtlsClientVerifier);

    return d->setCookieGeneratorParameters(params.hash, params.secret);
}

QDtlsClientVerifier::GeneratorParameters QDtlsClientVerifier::cookieGeneratorParameters() const
{
    Q_D(const QDtlsClientVerifier);

    return {d->hashAlgorithm, d->secret};
}

static QString msgUnsupportedMulticastAddress()
{
    return QDtls::tr("Multicast and broadcast addresses are not supported");
}

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

QByteArray QDtlsClientVerifier::verifiedHello() const
{
    Q_D(const QDtlsClientVerifier);

    return d->verifiedClientHello;
}

QDtlsError QDtlsClientVerifier::dtlsError() const
{
    Q_D(const QDtlsClientVerifier);

    return d->errorCode;
}

QString QDtlsClientVerifier::dtlsErrorString() const
{
    Q_D(const QDtlsBase);

    return d->errorDescription;
}

QDtls::QDtls(QSslSocket::SslMode mode, QObject *parent)
#if QT_CONFIG(openssl)
    : QObject(*new QDtlsPrivateOpenSSL, parent)
#endif
{
    Q_D(QDtls);

    d->mode = mode;
    setDtlsConfiguration(QSslConfiguration::defaultDtlsConfiguration());
}

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

QHostAddress QDtls::peerAddress() const
{
    Q_D(const QDtls);

    return d->remoteAddress;
}

quint16 QDtls::peerPort() const
{
    Q_D(const QDtlsBase);

    return d->remotePort;
}

QString QDtls::peerVerificationName() const
{
    Q_D(const QDtls);

    return d->peerVerificationName;
}

QSslSocket::SslMode QDtls::sslMode() const
{
    Q_D(const QDtls);

    return d->mode;
}

void QDtls::setMtuHint(quint16 mtuHint)
{
    Q_D(QDtls);

    d->mtuHint = mtuHint;
}

quint16 QDtls::mtuHint() const
{
    Q_D(const QDtls);

    return d->mtuHint;
}

bool QDtls::setCookieGeneratorParameters(const GeneratorParameters &params)
{
    Q_D(QDtls);

    return d->setCookieGeneratorParameters(params.hash, params.secret);
}

QDtls::GeneratorParameters QDtls::cookieGeneratorParameters() const
{
    Q_D(const QDtls);

    return {d->hashAlgorithm, d->secret};
}

bool QDtls::setDtlsConfiguration(const QSslConfiguration &configuration)
{
    Q_D(QDtls);

    if (d->handshakeState != HandshakeNotStarted) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Cannot set configuration after handshake started"));
        return false;
    }

    if (isDtlsProtocol(configuration.protocol())) {
        d->setConfiguration(configuration);
        return true;
    }

    d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Unsupported protocol"));
    return false;
}

QSslConfiguration QDtls::dtlsConfiguration() const
{
    Q_D(const QDtls);

    return d->configuration();
}

QDtls::HandshakeState QDtls::handshakeState()const
{
    Q_D(const QDtls);

    return d->handshakeState;
}

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

bool QDtls::handleTimeout(QUdpSocket *socket)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return false;
    }

    return d->handleTimeout(socket);
}

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

bool QDtls::abortHandshake(QUdpSocket *socket)
{
    Q_D(QDtls);

    if (!socket) {
        d->setDtlsError(QDtlsError::InvalidInputParameters, tr("Invalid (nullptr) socket"));
        return false;
    }

    if (d->handshakeState != PeerVerificationFailed) {
        d->setDtlsError(QDtlsError::InvalidOperation,
                        tr("Not in VerificationError state, nothing to abort"));
        return false;
    }

    d->abortHandshake(socket);
    return true;
}

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

bool QDtls::isConnectionEncrypted() const
{
    Q_D(const QDtls);

    return d->connectionEncrypted;
}

QSslCipher QDtls::sessionCipher() const
{
    Q_D(const QDtls);

    return d->sessionCipher;
}

QSsl::SslProtocol QDtls::sessionProtocol() const
{
    Q_D(const QDtls);

    return d->sessionProtocol;
}

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

QDtlsError QDtls::dtlsError() const
{
    Q_D(const QDtls);

    return d->errorCode;
}

QString QDtls::dtlsErrorString() const
{
    Q_D(const QDtls);

    return d->errorDescription;
}

QVector<QSslError> QDtls::peerVerificationErrors() const
{
    Q_D(const QDtls);

    return d->tlsErrors;
}

void QDtls::ignoreVerificationErrors(const QVector<QSslError> &errorsToIgnore)
{
    Q_D(QDtls);

    d->tlsErrorsToIgnore = errorsToIgnore;
}

QT_END_NAMESPACE
