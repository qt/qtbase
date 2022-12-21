// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtls_st_p.h"
#include "qtlsbackend_st_p.h"
#include "qtlskey_st_p.h"

#include <QtNetwork/private/qssl_p.h>

#include <QtNetwork/private/qsslcertificate_p.h>
#include <QtNetwork/private/qsslcipher_p.h>
#include <QtNetwork/private/qsslkey_p.h>

#include <QtNetwork/qsslsocket.h>

#include <QtCore/qmessageauthenticationcode.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qsystemdetection.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qdebug.h>
#include <QtCore/quuid.h>
#include <QtCore/qdir.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

#include <QtCore/private/qcore_mac_p.h>

#ifdef Q_OS_MACOS
#include <CoreServices/CoreServices.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Defined in qsslsocket_qt.cpp.
QByteArray _q_makePkcs12(const QList<QSslCertificate> &certs, const QSslKey &key,
                         const QString &passPhrase);

namespace QTlsPrivate {

// Defined in qtlsbackend_st.cpp
QSslCipher QSslCipher_from_SSLCipherSuite(SSLCipherSuite cipher);

namespace {

#ifdef Q_OS_MACOS
/*

Our own temporarykeychain is needed only on macOS where SecPKCS12Import changes
the default keychain and where we see annoying pop-ups asking about accessing a
private key.

*/

struct EphemeralSecKeychain
{
    EphemeralSecKeychain();
    ~EphemeralSecKeychain();

    SecKeychainRef keychain = nullptr;
    Q_DISABLE_COPY_MOVE(EphemeralSecKeychain)
};

EphemeralSecKeychain::EphemeralSecKeychain()
{
    const auto uuid = QUuid::createUuid();
    if (uuid.isNull()) {
        qCWarning(lcSecureTransport) << "Failed to create a unique keychain name";
        return;
    }

    const QByteArray uuidAsByteArray = uuid.toByteArray();
    Q_ASSERT(uuidAsByteArray.size() > 2);
    Q_ASSERT(uuidAsByteArray.startsWith('{'));
    Q_ASSERT(uuidAsByteArray.endsWith('}'));
    const auto uuidAsString = QLatin1StringView(uuidAsByteArray.data(), uuidAsByteArray.size()).mid(1, uuidAsByteArray.size() - 2);

    const QString keychainName
            = QDir::tempPath() + QDir::separator() + uuidAsString + ".keychain"_L1;
    // SecKeychainCreate, pathName parameter:
    //
    // "A constant character string representing the POSIX path indicating where
    // to store the keychain."
    //
    // Internally they seem to use std::string, but this does not really help.
    // Fortunately, CFString has a convenient API.
    QCFType<CFStringRef> cfName = keychainName.toCFString();
    std::vector<char> posixPath;
    // "Extracts the contents of a string as a NULL-terminated 8-bit string
    // appropriate for passing to POSIX APIs."
    posixPath.resize(CFStringGetMaximumSizeOfFileSystemRepresentation(cfName));
    const auto ok = CFStringGetFileSystemRepresentation(cfName, &posixPath[0],
                                                        CFIndex(posixPath.size()));
    if (!ok) {
        qCWarning(lcSecureTransport) << "Failed to create a unique keychain name from"
                                     << "QDir::tempPath()";
        return;
    }

    std::vector<uint8_t> passUtf8(256);
    if (SecRandomCopyBytes(kSecRandomDefault, passUtf8.size(), &passUtf8[0])) {
        qCWarning(lcSecureTransport) << "SecRandomCopyBytes: failed to create a key";
        return;
    }

    const OSStatus status = SecKeychainCreate(&posixPath[0], passUtf8.size(),
                                              &passUtf8[0], FALSE, nullptr,
                                              &keychain);
    if (status != errSecSuccess || !keychain) {
        qCWarning(lcSecureTransport) << "SecKeychainCreate: failed to create a custom keychain";
        if (keychain) {
            SecKeychainDelete(keychain);
            CFRelease(keychain);
            keychain = nullptr;
        }
    }

    if (keychain) {
        SecKeychainSettings settings = {};
        settings.version = SEC_KEYCHAIN_SETTINGS_VERS1;
        // Strange, huh? But that's what their docs say to do! With lockOnSleep
        // == false, set interval to INT_MAX to never lock ...
        settings.lockInterval = INT_MAX;
        if (SecKeychainSetSettings(keychain, &settings) != errSecSuccess)
            qCWarning(lcSecureTransport) << "SecKeychainSettings: failed to disable lock on sleep";
    }

#ifdef QSSLSOCKET_DEBUG
    if (keychain) {
        qCDebug(lcSecureTransport) << "Custom keychain with name" << keychainName << "was created"
                                   << "successfully";
    }
#endif
}

EphemeralSecKeychain::~EphemeralSecKeychain()
{
    if (keychain) {
        // clear file off disk
        SecKeychainDelete(keychain);
        CFRelease(keychain);
    }
}

#endif // Q_OS_MACOS

void qt_releaseSecureTransportContext(SSLContextRef context)
{
    if (context)
        CFRelease(context);
}

} // unnamed namespace

// To be also used by qtlsbackend_st.cpp (thus not in unnamed namespace).
SSLContextRef qt_createSecureTransportContext(QSslSocket::SslMode mode)
{
    const bool isServer = mode == QSslSocket::SslServerMode;
    const SSLProtocolSide side = isServer ? kSSLServerSide : kSSLClientSide;
    // We never use kSSLDatagramType, so it's kSSLStreamType unconditionally.
    SSLContextRef context = SSLCreateContext(nullptr, side, kSSLStreamType);
    if (!context)
        qCWarning(lcSecureTransport) << "SSLCreateContext failed";
    return context;
}

QSecureTransportContext::QSecureTransportContext(SSLContextRef c)
    : context(c)
{
}

QSecureTransportContext::~QSecureTransportContext()
{
    qt_releaseSecureTransportContext(context);
}

QSecureTransportContext::operator SSLContextRef()const
{
    return context;
}

void QSecureTransportContext::reset(SSLContextRef newContext)
{
    qt_releaseSecureTransportContext(context);
    context = newContext;
}

#if !defined(QT_PLATFORM_UIKIT) // dhparam is only used on macOS. (see the SSLSetDiffieHellmanParams call below)
static const uint8_t dhparam[] =
    "\x30\x82\x01\x08\x02\x82\x01\x01\x00\x97\xea\xd0\x46\xf7\xae\xa7\x76\x80"
    "\x9c\x74\x56\x98\xd8\x56\x97\x2b\x20\x6c\x77\xe2\x82\xbb\xc8\x84\xbe\xe7"
    "\x63\xaf\xcc\x30\xd0\x67\x97\x7d\x1b\xab\x59\x30\xa9\x13\x67\x21\xd7\xd4"
    "\x0e\x46\xcf\xe5\x80\xdf\xc9\xb9\xba\x54\x9b\x46\x2f\x3b\x45\xfc\x2f\xaf"
    "\xad\xc0\x17\x56\xdd\x52\x42\x57\x45\x70\x14\xe5\xbe\x67\xaa\xde\x69\x75"
    "\x30\x0d\xf9\xa2\xc4\x63\x4d\x7a\x39\xef\x14\x62\x18\x33\x44\xa1\xf9\xc1"
    "\x52\xd1\xb6\x72\x21\x98\xf8\xab\x16\x1b\x7b\x37\x65\xe3\xc5\x11\x00\xf6"
    "\x36\x1f\xd8\x5f\xd8\x9f\x43\xa8\xce\x9d\xbf\x5e\xd6\x2d\xfa\x0a\xc2\x01"
    "\x54\xc2\xd9\x81\x54\x55\xb5\x26\xf8\x88\x37\xf5\xfe\xe0\xef\x4a\x34\x81"
    "\xdc\x5a\xb3\x71\x46\x27\xe3\xcd\x24\xf6\x1b\xf1\xe2\x0f\xc2\xa1\x39\x53"
    "\x5b\xc5\x38\x46\x8e\x67\x4c\xd9\xdd\xe4\x37\x06\x03\x16\xf1\x1d\x7a\xba"
    "\x2d\xc1\xe4\x03\x1a\x58\xe5\x29\x5a\x29\x06\x69\x61\x7a\xd8\xa9\x05\x9f"
    "\xc1\xa2\x45\x9c\x17\xad\x52\x69\x33\xdc\x18\x8d\x15\xa6\x5e\xcd\x94\xf4"
    "\x45\xbb\x9f\xc2\x7b\x85\x00\x61\xb0\x1a\xdc\x3c\x86\xaa\x9f\x5c\x04\xb3"
    "\x90\x0b\x35\x64\xff\xd9\xe3\xac\xf2\xf2\xeb\x3a\x63\x02\x01\x02";
#endif

OSStatus TlsCryptographSecureTransport::ReadCallback(TlsCryptographSecureTransport *socket,
                                                     char *data, size_t *dataLength)
{
    Q_ASSERT(socket);
    Q_ASSERT(data);
    Q_ASSERT(dataLength);

    Q_ASSERT(socket->d);
    QTcpSocket *plainSocket = socket->d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    if (socket->isHandshakeComplete()) {
        // Check if it's a renegotiation attempt, when the handshake is complete, the
        // session state is 'kSSLConnected':
        SSLSessionState currentState = kSSLConnected;
        const OSStatus result = SSLGetSessionState(socket->context, &currentState);
        if (result != noErr) {
            *dataLength = 0;
            return result;
        }

        if (currentState == kSSLHandshake) {
            // Renegotiation detected, don't allow read more yet - 'transmit'
            // will notice this and will call 'startHandshake':
            *dataLength = 0;
            socket->renegotiating = true;
            return errSSLWouldBlock;
        }
    }

    const qint64 bytes = plainSocket->read(data, *dataLength);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSecureTransport) << plainSocket << "read" << bytes;
#endif
    if (bytes < 0) {
        *dataLength = 0;
        return errSecIO;
    }

    const OSStatus err = (size_t(bytes) < *dataLength) ? OSStatus(errSSLWouldBlock) : OSStatus(errSecSuccess);
    *dataLength = bytes;

    return err;
}

OSStatus TlsCryptographSecureTransport::WriteCallback(TlsCryptographSecureTransport *socket,
                                                      const char *data, size_t *dataLength)
{
    Q_ASSERT(socket);
    Q_ASSERT(data);
    Q_ASSERT(dataLength);

    Q_ASSERT(socket->d);
    QTcpSocket *plainSocket = socket->d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    const qint64 bytes = plainSocket->write(data, *dataLength);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSecureTransport) << plainSocket << "write" << bytes;
#endif
    if (bytes < 0) {
        *dataLength = 0;
        return errSecIO;
    }

    const OSStatus err = (size_t(bytes) < *dataLength) ? OSStatus(errSSLWouldBlock) : OSStatus(errSecSuccess);
    *dataLength = bytes;

    return err;
}

TlsCryptographSecureTransport::TlsCryptographSecureTransport()
    : context(nullptr)
{
}

TlsCryptographSecureTransport::~TlsCryptographSecureTransport()
{
    destroySslContext();
}

void TlsCryptographSecureTransport::init(QSslSocket *qObj, QSslSocketPrivate *dObj)
{
    Q_ASSERT(qObj);
    Q_ASSERT(dObj);
    q = qObj;
    d = dObj;

    renegotiating = false;
    shutdown = false;
}

void TlsCryptographSecureTransport::continueHandshake()
{
    Q_ASSERT(q);
    Q_ASSERT(d);
    d->setEncrypted(true);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSecureTransport) << d->plainTcpSocket() << "connection encrypted";
#endif

    // Unlike OpenSSL, Secure Transport does not allow to negotiate protocols via
    // a callback during handshake. We can only set our list of preferred protocols
    // (and send it during handshake) and then receive what our peer has sent to us.
    // And here we can finally try to find a match (if any).
    const auto &configuration = q->sslConfiguration();
    const auto &requestedProtocols = configuration.allowedNextProtocols();
    if (const int requestedCount = requestedProtocols.size()) {
        QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationNone);
        QTlsBackend::setNegotiatedProtocol(d, {});

        QCFType<CFArrayRef> cfArray;
        const OSStatus result = SSLCopyALPNProtocols(context, &cfArray);
        if (result == errSecSuccess && cfArray && CFArrayGetCount(cfArray)) {
            const int size = CFArrayGetCount(cfArray);
            QList<QString> peerProtocols(size);
            for (int i = 0; i < size; ++i)
                peerProtocols[i] = QString::fromCFString((CFStringRef)CFArrayGetValueAtIndex(cfArray, i));

            for (int i = 0; i < requestedCount; ++i) {
                const auto requestedName = QString::fromLatin1(requestedProtocols[i]);
                for (int j = 0; j < size; ++j) {
                    if (requestedName == peerProtocols[j]) {
                        QTlsBackend::setNegotiatedProtocol(d, requestedName.toLatin1());
                        QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationNegotiated);
                        break;
                    }
                }
                if (configuration.nextProtocolNegotiationStatus() == QSslConfiguration::NextProtocolNegotiationNegotiated)
                    break;
            }
        }
    }

    if (!renegotiating)
        emit q->encrypted();

    if (d->isAutoStartingHandshake() && d->isPendingClose()) {
        d->setPendingClose(false);
        q->disconnectFromHost();
    }
}

void TlsCryptographSecureTransport::disconnected()
{
    Q_ASSERT(d && d->plainTcpSocket());
    d->setEncrypted(false);
    if (d->plainTcpSocket()->bytesAvailable() <= 0)
        destroySslContext();
    // If there is still buffered data in the plain socket, don't destroy the ssl context yet.
    // It will be destroyed when the socket is deleted.
}

void TlsCryptographSecureTransport::disconnectFromHost()
{
    Q_ASSERT(d && d->plainTcpSocket());
    if (context) {
        if (!shutdown) {
            SSLClose(context);
            context.reset(nullptr);
            shutdown = true;
        }
    }
    d->plainTcpSocket()->disconnectFromHost();
}

QSslCipher TlsCryptographSecureTransport::sessionCipher() const
{
    SSLCipherSuite cipher = 0;
    if (context && SSLGetNegotiatedCipher(context, &cipher) == errSecSuccess)
        return QSslCipher_from_SSLCipherSuite(cipher);

    return QSslCipher();
}

QSsl::SslProtocol TlsCryptographSecureTransport::sessionProtocol() const
{
    if (!context)
        return QSsl::UnknownProtocol;

    SSLProtocol protocol = kSSLProtocolUnknown;
    const OSStatus err = SSLGetNegotiatedProtocolVersion(context, &protocol);
    if (err != errSecSuccess) {
        qCWarning(lcSecureTransport) << "SSLGetNegotiatedProtocolVersion failed:" << err;
        return QSsl::UnknownProtocol;
    }

    switch (protocol) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case kTLSProtocol1:
        return QSsl::TlsV1_0;
    case kTLSProtocol11:
        return QSsl::TlsV1_1;
QT_WARNING_POP
    case kTLSProtocol12:
        return QSsl::TlsV1_2;
    case kTLSProtocol13:
        return QSsl::TlsV1_3;
    default:
        return QSsl::UnknownProtocol;
    }
}

void TlsCryptographSecureTransport::startClientEncryption()
{
    if (!initSslContext()) {
        Q_ASSERT(d);
        // Error description/code were set, 'error' emitted
        // by initSslContext, but OpenSSL socket also sets error,
        // emits a signal twice, so ...
        setErrorAndEmit(d, QAbstractSocket::SslInternalError, QStringLiteral("Unable to init SSL Context"));
        return;
    }

    startHandshake();
}

void TlsCryptographSecureTransport::startServerEncryption()
{
    if (!initSslContext()) {
        // Error description/code were set, 'error' emitted
        // by initSslContext, but OpenSSL socket also sets error
        // emits a signal twice, so ...
        setErrorAndEmit(d, QAbstractSocket::SslInternalError, QStringLiteral("Unable to init SSL Context"));
        return;
    }

    startHandshake();
}

void TlsCryptographSecureTransport::transmit()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    // If we don't have any SSL context, don't bother transmitting.
    // Edit: if SSL session closed, don't bother either.
    if (!context || shutdown)
        return;

    if (!isHandshakeComplete())
        startHandshake();

    auto &writeBuffer = d->tlsWriteBuffer();
    if (isHandshakeComplete() && !writeBuffer.isEmpty()) {
        qint64 totalBytesWritten = 0;
        while (writeBuffer.nextDataBlockSize() > 0 && context) {
            const size_t nextDataBlockSize = writeBuffer.nextDataBlockSize();
            size_t writtenBytes = 0;
            const OSStatus err = SSLWrite(context, writeBuffer.readPointer(), nextDataBlockSize, &writtenBytes);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSecureTransport) << d->plainTcpSocket() << "SSLWrite returned" << err;
#endif
            if (err != errSecSuccess && err != errSSLWouldBlock) {
                setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                QStringLiteral("SSLWrite failed: %1").arg(err));
                break;
            }

            if (writtenBytes) {
                writeBuffer.free(writtenBytes);
                totalBytesWritten += writtenBytes;
            }

            if (writtenBytes < nextDataBlockSize)
                break;
        }

        if (totalBytesWritten > 0) {
            // Don't emit bytesWritten() recursively.
            auto &emittedBytesWritten = d->tlsEmittedBytesWritten();
            if (!emittedBytesWritten) {
                emittedBytesWritten = true;
                emit q->bytesWritten(totalBytesWritten);
                emittedBytesWritten = false;
            }
            emit q->channelBytesWritten(0, totalBytesWritten);
        }
    }

    auto &buffer = d->tlsBuffer();
    const auto readBufferMaxSize = d->maxReadBufferSize();
    if (isHandshakeComplete()) {
        QVarLengthArray<char, 4096> data;
        while (context && (!readBufferMaxSize || buffer.size() < readBufferMaxSize)) {
            size_t readBytes = 0;
            data.resize(4096);
            const OSStatus err = SSLRead(context, data.data(), data.size(), &readBytes);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSecureTransport) << d->plainTcpSocket() << "SSLRead returned" << err;
#endif
            if (err == errSSLClosedGraceful) {
                shutdown = true; // the other side shut down, make sure we do not send shutdown ourselves
                setErrorAndEmit(d, QAbstractSocket::RemoteHostClosedError,
                                QSslSocket::tr("The TLS/SSL connection has been closed"));
                break;
            } else if (err != errSecSuccess && err != errSSLWouldBlock) {
                setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                QStringLiteral("SSLRead failed: %1").arg(err));
                break;
            }

            if (err == errSSLWouldBlock && renegotiating) {
                startHandshake();
                break;
            }

            if (readBytes) {
                buffer.append(data.constData(), readBytes);
                if (bool *readyReadEmittedPointer = d->readyReadPointer())
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
                emit q->channelReadyRead(0);
            }

            if (err == errSSLWouldBlock)
                break;
        }
    }
}

SSLCipherSuite TlsCryptographSecureTransport::SSLCipherSuite_from_QSslCipher(const QSslCipher &ciph)
{
    if (ciph.name() == "AES128-SHA"_L1)
        return TLS_RSA_WITH_AES_128_CBC_SHA;
    if (ciph.name() == "DHE-RSA-AES128-SHA"_L1)
        return TLS_DHE_RSA_WITH_AES_128_CBC_SHA;
    if (ciph.name() == "AES256-SHA"_L1)
        return TLS_RSA_WITH_AES_256_CBC_SHA;
    if (ciph.name() == "DHE-RSA-AES256-SHA"_L1)
        return TLS_DHE_RSA_WITH_AES_256_CBC_SHA;
    if (ciph.name() == "ECDH-ECDSA-NULL-SHA"_L1)
        return TLS_ECDH_ECDSA_WITH_NULL_SHA;
    if (ciph.name() == "ECDH-ECDSA-RC4-SHA"_L1)
        return TLS_ECDH_ECDSA_WITH_RC4_128_SHA;
    if (ciph.name() == "ECDH-ECDSA-DES-CBC3-SHA"_L1)
        return TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA;
    if (ciph.name() == "ECDH-ECDSA-AES128-SHA"_L1)
        return TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA;
    if (ciph.name() == "ECDH-ECDSA-AES256-SHA"_L1)
        return TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA;
    if (ciph.name() == "ECDH-ECDSA-RC4-SHA"_L1)
        return TLS_ECDHE_ECDSA_WITH_RC4_128_SHA;
    if (ciph.name() == "ECDH-ECDSA-DES-CBC3-SHA"_L1)
        return TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA;
    if (ciph.name() == "ECDH-ECDSA-AES128-SHA"_L1)
        return TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA;
    if (ciph.name() == "ECDH-ECDSA-AES256-SHA"_L1)
        return TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA;
    if (ciph.name() == "ECDH-RSA-NULL-SHA"_L1)
        return TLS_ECDH_RSA_WITH_NULL_SHA;
    if (ciph.name() == "ECDH-RSA-RC4-SHA"_L1)
        return TLS_ECDH_RSA_WITH_RC4_128_SHA;
    if (ciph.name() == "ECDH-RSA-DES-CBC3-SHA"_L1)
        return TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA;
    if (ciph.name() == "ECDH-RSA-AES128-SHA"_L1)
        return TLS_ECDH_RSA_WITH_AES_128_CBC_SHA;
    if (ciph.name() == "ECDH-RSA-AES256-SHA"_L1)
        return TLS_ECDH_RSA_WITH_AES_256_CBC_SHA;
    if (ciph.name() == "ECDH-RSA-RC4-SHA"_L1)
        return TLS_ECDHE_RSA_WITH_RC4_128_SHA;
    if (ciph.name() == "ECDH-RSA-DES-CBC3-SHA"_L1)
        return TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA;
    if (ciph.name() == "ECDH-RSA-AES128-SHA"_L1)
        return TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA;
    if (ciph.name() == "ECDH-RSA-AES256-SHA"_L1)
        return TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
    if (ciph.name() == "DES-CBC3-SHA"_L1)
        return TLS_RSA_WITH_3DES_EDE_CBC_SHA;
    if (ciph.name() == "AES128-SHA256"_L1)
        return TLS_RSA_WITH_AES_128_CBC_SHA256;
    if (ciph.name() == "AES256-SHA256"_L1)
        return TLS_RSA_WITH_AES_256_CBC_SHA256;
    if (ciph.name() == "DHE-RSA-DES-CBC3-SHA"_L1)
        return TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA;
    if (ciph.name() == "DHE-RSA-AES128-SHA256"_L1)
        return TLS_DHE_RSA_WITH_AES_128_CBC_SHA256;
    if (ciph.name() == "DHE-RSA-AES256-SHA256"_L1)
        return TLS_DHE_RSA_WITH_AES_256_CBC_SHA256;
    if (ciph.name() == "AES256-GCM-SHA384"_L1)
        return TLS_RSA_WITH_AES_256_GCM_SHA384;
    if (ciph.name() == "ECDHE-ECDSA-AES128-SHA256"_L1)
        return TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256;
    if (ciph.name() == "ECDHE-ECDSA-AES256-SHA384"_L1)
        return TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384;
    if (ciph.name() == "ECDH-ECDSA-AES128-SHA256"_L1)
        return TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256;
    if (ciph.name() == "ECDH-ECDSA-AES256-SHA384"_L1)
        return TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384;
    if (ciph.name() == "ECDHE-RSA-AES128-SHA256"_L1)
        return TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256;
    if (ciph.name() == "ECDHE-RSA-AES256-SHA384"_L1)
        return TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384;
    if (ciph.name() == "ECDHE-RSA-AES256-SHA384"_L1)
        return TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256;
    if (ciph.name() == "ECDHE-RSA-AES256-GCM-SHA384"_L1)
        return TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384;
    if (ciph.name() == "AES128-GCM-SHA256"_L1)
        return TLS_AES_128_GCM_SHA256;
    if (ciph.name() == "AES256-GCM-SHA384"_L1)
        return TLS_AES_256_GCM_SHA384;
    if (ciph.name() == "CHACHA20-POLY1305-SHA256"_L1)
        return TLS_CHACHA20_POLY1305_SHA256;
    if (ciph.name() == "AES128-CCM-SHA256"_L1)
        return TLS_AES_128_CCM_SHA256;
    if (ciph.name() == "AES128-CCM8-SHA256"_L1)
        return TLS_AES_128_CCM_8_SHA256;
    if (ciph.name() == "ECDHE-ECDSA-AES128-GCM-SHA256"_L1)
        return TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
    if (ciph.name() == "ECDHE-ECDSA-AES256-GCM-SHA384"_L1)
        return TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384;
    if (ciph.name() == "ECDH-ECDSA-AES128-GCM-SHA256"_L1)
        return TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256;
    if (ciph.name() == "ECDH-ECDSA-AES256-GCM-SHA384"_L1)
        return TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384;
    if (ciph.name() == "ECDHE-RSA-AES128-GCM-SHA256"_L1)
        return TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256;
    if (ciph.name() == "ECDH-RSA-AES128-GCM-SHA256"_L1)
        return TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256;
    if (ciph.name() == "ECDH-RSA-AES256-GCM-SHA384"_L1)
        return TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384;
    if (ciph.name() == "ECDHE-RSA-CHACHA20-POLY1305-SHA256"_L1)
        return TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256;
    if (ciph.name() == "ECDHE-ECDSA-CHACHA20-POLY1305-SHA256"_L1)
        return TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256;

    return 0;
}

bool TlsCryptographSecureTransport::initSslContext()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    Q_ASSERT_X(!context, Q_FUNC_INFO, "invalid socket state, context is not null");
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    const auto mode = d->tlsMode();

    context.reset(qt_createSecureTransportContext(mode));
    if (!context) {
        setErrorAndEmit(d, QAbstractSocket::SslInternalError, QStringLiteral("SSLCreateContext failed"));
        return false;
    }

    const OSStatus err = SSLSetIOFuncs(context,
                                       reinterpret_cast<SSLReadFunc>(&TlsCryptographSecureTransport::ReadCallback),
                                       reinterpret_cast<SSLWriteFunc>(&TlsCryptographSecureTransport::WriteCallback));
    if (err != errSecSuccess) {
        destroySslContext();
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QStringLiteral("SSLSetIOFuncs failed: %1").arg(err));
        return false;
    }

    SSLSetConnection(context, this);

    const auto &configuration = q->sslConfiguration();
    if (mode == QSslSocket::SslServerMode
        && !configuration.localCertificateChain().isEmpty()) {
        QString errorDescription;
        QAbstractSocket::SocketError errorCode = QAbstractSocket::UnknownSocketError;
        if (!setSessionCertificate(errorDescription, errorCode)) {
            destroySslContext();
            setErrorAndEmit(d, errorCode, errorDescription);
            return false;
        }
    }

    if (!setSessionProtocol()) {
        destroySslContext();
        setErrorAndEmit(d, QAbstractSocket::SslInternalError, QStringLiteral("Failed to set protocol version"));
        return false;
    }

    const auto protocolNames = configuration.allowedNextProtocols();
    QCFType<CFMutableArrayRef> cfNames(CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks));
    if (cfNames) {
        for (const QByteArray &name : protocolNames) {
            if (name.size() > 255) {
                qCWarning(lcSecureTransport) << "TLS ALPN extension" << name
                                             << "is too long and will be ignored.";
                continue;
            } else if (name.isEmpty()) {
                continue;
            }
            QCFString cfName(QString::fromLatin1(name).toCFString());
            CFArrayAppendValue(cfNames, cfName);
        }

        if (CFArrayGetCount(cfNames)) {
            // Up to the application layer to check that negotiation
            // failed, and handle this non-TLS error, we do not handle
            // the result of this call as an error:
            if (SSLSetALPNProtocols(context, cfNames) != errSecSuccess)
                qCWarning(lcSecureTransport) << "SSLSetALPNProtocols failed - too long protocol names?";
        }
    } else {
        qCWarning(lcSecureTransport) << "failed to allocate ALPN names array";
    }

    if (mode == QSslSocket::SslClientMode) {
        // enable Server Name Indication (SNI)
        const auto verificationPeerName = d->verificationName();
        QString tlsHostName(verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName);
        if (tlsHostName.isEmpty())
            tlsHostName = d->tlsHostName();

        const QByteArray ace(QUrl::toAce(tlsHostName));
        SSLSetPeerDomainName(context, ace.data(), ace.size());
        // tell SecureTransport we handle peer verification ourselves
        OSStatus err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnServerAuth, true);
        if (err == errSecSuccess)
            err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnCertRequested, true);

        if (err != errSecSuccess) {
            destroySslContext();
            setErrorAndEmit(d, QSslSocket::SslInternalError,
                            QStringLiteral("SSLSetSessionOption failed: %1").arg(err));
            return false;
        }
        //
    } else {
        if (configuration.peerVerifyMode() != QSslSocket::VerifyNone) {
            // kAlwaysAuthenticate - always fails even if we set break on client auth.
            OSStatus err = SSLSetClientSideAuthenticate(context, kTryAuthenticate);
            if (err == errSecSuccess) {
                // We'd like to verify peer ourselves, otherwise handshake will
                // most probably fail before we can do anything.
                err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnClientAuth, true);
            }

            if (err != errSecSuccess) {
                destroySslContext();
                setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                QStringLiteral("failed to set SSL context option in server mode: %1").arg(err));
                return false;
            }
        }
#if !defined(QT_PLATFORM_UIKIT)
        // No SSLSetDiffieHellmanParams on iOS; calling it is optional according to docs.
        SSLSetDiffieHellmanParams(context, dhparam, sizeof(dhparam));
#endif
    }
    if (configuration.ciphers().size() > 0) {
        QVector<SSLCipherSuite> cfCiphers;
        for (const QSslCipher &cipher : configuration.ciphers()) {
            if (auto sslCipher = TlsCryptographSecureTransport::SSLCipherSuite_from_QSslCipher(cipher))
                cfCiphers << sslCipher;
        }
        if (cfCiphers.size() == 0) {
            qCWarning(lcSecureTransport) << "failed to add any of the requested ciphers from the configuration";
            return false;
        }
        OSStatus err = SSLSetEnabledCiphers(context, cfCiphers.data(), cfCiphers.size());
        if (err != errSecSuccess) {
            qCWarning(lcSecureTransport) << "failed to set the ciphers from the configuration";
            return false;
        }
    }
    return true;
}

void TlsCryptographSecureTransport::destroySslContext()
{
    context.reset(nullptr);
}

bool TlsCryptographSecureTransport::setSessionCertificate(QString &errorDescription, QAbstractSocket::SocketError &errorCode)
{
    Q_ASSERT_X(context, Q_FUNC_INFO, "invalid SSL context (null)");

    Q_ASSERT(d);
    const auto &configuration = q->sslConfiguration();

#ifdef QSSLSOCKET_DEBUG
    auto *plainSocket = d->plainTcpSocket();
#endif

    QSslCertificate localCertificate;

    if (!configuration.localCertificateChain().isEmpty())
        localCertificate = configuration.localCertificateChain().at(0);

    if (!localCertificate.isNull()) {
        // Require a private key as well.
        if (configuration.privateKey().isNull()) {
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("Cannot provide a certificate with no key");
            return false;
        }

        // import certificates and key
        const QString passPhrase(QString::fromLatin1("foobar"));
        QCFType<CFDataRef> pkcs12 = _q_makePkcs12(configuration.localCertificateChain(),
                                                  configuration.privateKey(), passPhrase).toCFData();
        QCFType<CFStringRef> password = passPhrase.toCFString();
        const void *keys[2] = { kSecImportExportPassphrase };
        const void *values[2] = { password };
        CFIndex nKeys = 1;
#ifdef Q_OS_MACOS
        bool envOk = false;
        const int env = qEnvironmentVariableIntValue("QT_SSL_USE_TEMPORARY_KEYCHAIN", &envOk);
        if (envOk && env) {
            static const EphemeralSecKeychain temporaryKeychain;
            if (temporaryKeychain.keychain) {
                nKeys = 2;
                keys[1] = kSecImportExportKeychain;
                values[1] = temporaryKeychain.keychain;
            }
        }
#endif
        QCFType<CFDictionaryRef> options = CFDictionaryCreate(nullptr, keys, values, nKeys,
                                                              nullptr, nullptr);
        QCFType<CFArrayRef> items;
        OSStatus err = SecPKCS12Import(pkcs12, options, &items);
        if (err != errSecSuccess) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcSecureTransport) << plainSocket
                                         << QStringLiteral("SecPKCS12Import failed: %1").arg(err);
#endif
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("SecPKCS12Import failed: %1").arg(err);
            return false;
        }

        if (!CFArrayGetCount(items)) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcSecureTransport) << plainSocket << "SecPKCS12Import returned no items";
#endif
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("SecPKCS12Import returned no items");
            return false;
        }

        CFDictionaryRef import = (CFDictionaryRef)CFArrayGetValueAtIndex(items, 0);
        SecIdentityRef identity = (SecIdentityRef)CFDictionaryGetValue(import, kSecImportItemIdentity);
        if (!identity) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcSecureTransport) << plainSocket << "SecPKCS12Import returned no identity";
#endif
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("SecPKCS12Import returned no identity");
            return false;
        }

        QCFType<CFMutableArrayRef> certs = CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks);
        if (!certs) {
            errorCode = QAbstractSocket::SslInternalError;
            errorDescription = QStringLiteral("Failed to allocate certificates array");
            return false;
        }

        CFArrayAppendValue(certs, identity);

        CFArrayRef chain = (CFArrayRef)CFDictionaryGetValue(import, kSecImportItemCertChain);
        if (chain) {
            for (CFIndex i = 1, e = CFArrayGetCount(chain); i < e; ++i)
                CFArrayAppendValue(certs, CFArrayGetValueAtIndex(chain, i));
        }

        err = SSLSetCertificate(context, certs);
        if (err != errSecSuccess) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcSecureTransport) << plainSocket
                                         << QStringLiteral("Cannot set certificate and key: %1").arg(err);
#endif
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("Cannot set certificate and key: %1").arg(err);
            return false;
        }
    }

    return true;
}

bool TlsCryptographSecureTransport::setSessionProtocol()
{
    Q_ASSERT_X(context, Q_FUNC_INFO, "invalid SSL context (null)");
    Q_ASSERT(q);
    Q_ASSERT(d);
    // SecureTransport has kTLSProtocol13 constant and also, kTLSProtocolMaxSupported.
    // Calling SSLSetProtocolVersionMax/Min with any of these two constants results
    // in errInvalidParam and a failure to set the protocol version. This means
    // no TLS 1.3 on macOS and iOS.
    const auto &configuration = q->sslConfiguration();
    auto *plainSocket = d->plainTcpSocket();
    switch (configuration.protocol()) {
    case QSsl::TlsV1_3:
    case QSsl::TlsV1_3OrLater:
        qCWarning(lcSecureTransport) << plainSocket << "SecureTransport does not support TLS 1.3";
        return false;
    default:;
    }

    OSStatus err = errSecSuccess;

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    if (configuration.protocol() == QSsl::TlsV1_0) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : TLSv1.0";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
        if (err == errSecSuccess)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol1);
    } else if (configuration.protocol() == QSsl::TlsV1_1) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : TLSv1.1";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol11);
        if (err == errSecSuccess)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol11);
QT_WARNING_POP
    } else if (configuration.protocol() == QSsl::TlsV1_2) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol12);
        if (err == errSecSuccess)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol() == QSsl::AnyProtocol) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : any";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
    } else if (configuration.protocol() == QSsl::SecureProtocols) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol12);
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    } else if (configuration.protocol() == QSsl::TlsV1_0OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : TLSv1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
    } else if (configuration.protocol() == QSsl::TlsV1_1OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : TLSv1.1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol11);
QT_WARNING_POP
    } else if (configuration.protocol() == QSsl::TlsV1_2OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol12);
    } else {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSecureTransport) << plainSocket << "no protocol version found in the configuration";
    #endif
        return false;
    }

    return err == errSecSuccess;
}

bool TlsCryptographSecureTransport::canIgnoreTrustVerificationFailure() const
{
    Q_ASSERT(q);
    Q_ASSERT(d);
    const auto &configuration = q->sslConfiguration();
    const QSslSocket::PeerVerifyMode verifyMode = configuration.peerVerifyMode();
    return d->tlsMode() == QSslSocket::SslServerMode
           && (verifyMode == QSslSocket::QueryPeer
               || verifyMode == QSslSocket::AutoVerifyPeer
               || verifyMode == QSslSocket::VerifyNone);
}

bool TlsCryptographSecureTransport::verifySessionProtocol() const
{
    Q_ASSERT(q);

    const auto &configuration = q->sslConfiguration();
    bool protocolOk = false;
    if (configuration.protocol() == QSsl::AnyProtocol)
        protocolOk = true;
    else if (configuration.protocol() == QSsl::SecureProtocols)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_2);
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    else if (configuration.protocol() == QSsl::TlsV1_0OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_0);
    else if (configuration.protocol() == QSsl::TlsV1_1OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_1);
QT_WARNING_POP
    else if (configuration.protocol() == QSsl::TlsV1_2OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_2);
    else if (configuration.protocol() == QSsl::TlsV1_3OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_3OrLater);
    else
        protocolOk = (sessionProtocol() == configuration.protocol());

    return protocolOk;
}

bool TlsCryptographSecureTransport::verifyPeerTrust()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    const auto mode = d->tlsMode();
    const QSslSocket::PeerVerifyMode verifyMode = q->peerVerifyMode();
    const bool canIgnoreVerify = canIgnoreTrustVerificationFailure();

    Q_ASSERT_X(context, Q_FUNC_INFO, "invalid SSL context (null)");

    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    QCFType<SecTrustRef> trust;
    OSStatus err = SSLCopyPeerTrust(context, &trust);
    // !trust - SSLCopyPeerTrust can return errSecSuccess but null trust.
    if (err != errSecSuccess || !trust) {
        if (!canIgnoreVerify) {
            setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                            QStringLiteral("Failed to obtain peer trust: %1").arg(err));
            plainSocket->disconnectFromHost();
            return false;
        } else {
            return true;
        }
    }

    QList<QSslError> errors;

    // Store certificates.
    // Apple's docs say SetTrustEvaluate must be called before
    // SecTrustGetCertificateAtIndex, but this results
    // in 'kSecTrustResultRecoverableTrustFailure', so
    // here we just ignore 'res' (later we'll use SetAnchor etc.
    // and evaluate again).
    SecTrustResultType res = kSecTrustResultInvalid;
    err = SecTrustEvaluate(trust, &res);
    if (err != errSecSuccess) {
        // We can not ignore this, it's not even about trust verification
        // probably ...
        setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                        QStringLiteral("SecTrustEvaluate failed: %1").arg(err));
        plainSocket->disconnectFromHost();
        return false;
    }

    QTlsBackend::clearPeerCertificates(d);

    QList<QSslCertificate> peerCertificateChain;
    const CFIndex certCount = SecTrustGetCertificateCount(trust);
    for (CFIndex i = 0; i < certCount; ++i) {
        SecCertificateRef cert = SecTrustGetCertificateAtIndex(trust, i);
        QCFType<CFDataRef> derData = SecCertificateCopyData(cert);
        peerCertificateChain << QSslCertificate(QByteArray::fromCFData(derData), QSsl::Der);
    }
    QTlsBackend::storePeerCertificateChain(d, peerCertificateChain);

    if (peerCertificateChain.size())
        QTlsBackend::storePeerCertificate(d, peerCertificateChain.at(0));

    // Check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer):
    for (const QSslCertificate &cert : std::as_const(peerCertificateChain)) {
        if (QSslCertificatePrivate::isBlacklisted(cert) && !canIgnoreVerify) {
            const QSslError error(QSslError::CertificateBlacklisted, cert);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    const bool doVerifyPeer = verifyMode == QSslSocket::VerifyPeer
                              || (verifyMode == QSslSocket::AutoVerifyPeer
                                  && d->tlsMode() == QSslSocket::SslClientMode);
    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    const auto &peerCertificate = q->peerCertificate();
    if (!peerCertificate.isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        const QString verificationPeerName = d->verificationName();
        if (mode == QSslSocket::SslClientMode) {
            const QString peerName(verificationPeerName.isEmpty () ? q->peerName() : verificationPeerName);
            if (!isMatchingHostname(peerCertificate, peerName) && !canIgnoreVerify) {
                // No matches in common names or alternate names.
                const QSslError error(QSslError::HostNameMismatch, peerCertificate);
                errors << error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    } else {
        // No peer certificate presented. Report as error if the socket
        // expected one.
        if (doVerifyPeer && !canIgnoreVerify) {
            const QSslError error(QSslError::NoPeerCertificate);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    // verify certificate chain
    QCFType<CFMutableArrayRef> certArray = CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks);
    const auto &caCertificates = q->sslConfiguration().caCertificates();
    for (const QSslCertificate &cert : caCertificates) {
        QCFType<CFDataRef> certData = cert.toDer().toCFData();
        if (QCFType<SecCertificateRef> secRef = SecCertificateCreateWithData(nullptr, certData))
            CFArrayAppendValue(certArray, secRef);
    }

    SecTrustSetAnchorCertificates(trust, certArray);

    // By default SecTrustEvaluate uses both CA certificates provided in
    // QSslConfiguration and the ones from the system database. This behavior can
    // be unexpected if a user's code tries to limit the trusted CAs to those
    // explicitly set in QSslConfiguration.
    // Since on macOS we initialize the default QSslConfiguration copying the
    // system CA certificates (using SecTrustSettingsCopyCertificates) we can
    // call SecTrustSetAnchorCertificatesOnly(trust, true) to force SecTrustEvaluate
    // to use anchors only from our QSslConfiguration.
    // Unfortunately, SecTrustSettingsCopyCertificates is not available on iOS
    // and the default QSslConfiguration always has an empty list of system CA
    // certificates. This leaves no way to provide client code with access to the
    // actual system CA certificate list (which most use-cases need) other than
    // by letting SecTrustEvaluate fall through to the system list; so, in this case
    // (even though the client code may have provided its own certs), we retain
    // the default behavior. Note, with macOS SDK below 10.12 using 'trust my
    // anchors only' may result in some valid chains rejected, apparently the
    // ones containing intermediated certificates; so we use this functionality
    // on more recent versions only.

    bool anchorsFromConfigurationOnly = false;

#ifdef Q_OS_MACOS
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSierra)
        anchorsFromConfigurationOnly = true;
#endif // Q_OS_MACOS

    SecTrustSetAnchorCertificatesOnly(trust, anchorsFromConfigurationOnly);

    SecTrustResultType trustResult = kSecTrustResultInvalid;
    SecTrustEvaluate(trust, &trustResult);
    switch (trustResult) {
    case kSecTrustResultUnspecified:
    case kSecTrustResultProceed:
        break;
    default:
        if (!canIgnoreVerify) {
            const QSslError error(QSslError::CertificateUntrusted, peerCertificate);
            errors << error;
            emit q->peerVerifyError(error);
        }
    }

    // report errors
    if (!errors.isEmpty() && !canIgnoreVerify) {
        sslErrors = errors;
        // checkSslErrors unconditionally emits sslErrors:
        // a user's slot can abort/close/disconnect on this
        // signal, so we also test the socket's state:
        if (!checkSslErrors() || q->state() != QAbstractSocket::ConnectedState)
            return false;
    } else {
        sslErrors.clear();
    }

    return true;
}

/*
    Copied verbatim from qsslsocket_openssl.cpp
*/
bool TlsCryptographSecureTransport::checkSslErrors()
{
    if (sslErrors.isEmpty())
        return true;

    Q_ASSERT(q);
    Q_ASSERT(d);

    emit q->sslErrors(sslErrors);
    const auto mode = d->tlsMode();
    const auto &configuration = q->sslConfiguration();
    const bool doVerifyPeer = configuration.peerVerifyMode() == QSslSocket::VerifyPeer
                              || (configuration.peerVerifyMode() == QSslSocket::AutoVerifyPeer
                              && mode == QSslSocket::SslClientMode);
    const bool doEmitSslError = !d->verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            QSslSocketPrivate::pauseSocketNotifiers(q);
            d->setPaused(true);
        } else {
            setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                            sslErrors.constFirst().errorString());
            Q_ASSERT(d->plainTcpSocket());
            d->plainTcpSocket()->disconnectFromHost();
        }
        return false;
    }

    return true;
}

bool TlsCryptographSecureTransport::startHandshake()
{
    Q_ASSERT(context);
    Q_ASSERT(q);
    Q_ASSERT(d);

    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    const auto mode = d->tlsMode();

    OSStatus err = SSLHandshake(context);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSecureTransport) << plainSocket << "SSLHandhake returned" << err;
#endif

    if (err == errSSLWouldBlock) {
        // startHandshake has to be called again ... later.
        return false;
    } else if (err == errSSLServerAuthCompleted) {
        // errSSLServerAuthCompleted is a define for errSSLPeerAuthCompleted,
        // it works for both server/client modes.
        // In future we'll evaluate peer's trust at this point,
        // for now we just continue.
        // if (!verifyPeerTrust())
        //      ...
        return startHandshake();
    } else if (err == errSSLClientCertRequested) {
        Q_ASSERT(mode == QSslSocket::SslClientMode);
        QString errorDescription;
        QAbstractSocket::SocketError errorCode = QAbstractSocket::UnknownSocketError;
        // setSessionCertificate does not fail if we have no certificate.
        // Failure means a real error (invalid certificate, no private key, etc).
        if (!setSessionCertificate(errorDescription, errorCode)) {
            setErrorAndEmit(d, errorCode, errorDescription);
            renegotiating = false;
            return false;
        } else {
            // We try to resume a handshake, even if have no
            // local certificates ... (up to server to deal with our failure).
            return startHandshake();
        }
    } else if (err != errSecSuccess) {
        if (err == errSSLBadCert && canIgnoreTrustVerificationFailure()) {
            // We're on the server side and client did not provide any
            // certificate. This is the new 'nice' error returned by
            // Security Framework after it was recently updated.
            return startHandshake();
        }

        renegotiating = false;
        setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                        QStringLiteral("SSLHandshake failed: %1").arg(err));
        plainSocket->disconnectFromHost();
        return false;
    }

    // Connection aborted during handshake phase.
    if (q->state() != QAbstractSocket::ConnectedState) {
        qCDebug(lcSecureTransport) << "connection aborted";
        renegotiating = false;
        return false;
    }

    // check protocol version ourselves, as Secure Transport does not enforce
    // the requested min / max versions.
    if (!verifySessionProtocol()) {
        setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError, QStringLiteral("Protocol version mismatch"));
        plainSocket->disconnectFromHost();
        renegotiating = false;
        return false;
    }

    if (verifyPeerTrust()) {
        continueHandshake();
        renegotiating = false;
        return true;
    } else {
        renegotiating = false;
        return false;
    }
}

bool TlsCryptographSecureTransport::isHandshakeComplete() const
{
    Q_ASSERT(q);
    return q->isEncrypted() && !renegotiating;
}

QList<QSslError> TlsCryptographSecureTransport::tlsErrors() const
{
    return sslErrors;
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
