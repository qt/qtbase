/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
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

#include "qsslsocket.h"

#include "qssl_p.h"
#include "qsslsocket_mac_p.h"
#include "qasn1element_p.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"
#include "qsslkey_p.h"

#include <QtCore/qmessageauthenticationcode.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qsystemdetection.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qvector.h>
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

namespace
{
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
        qCWarning(lcSsl) << "Failed to create a unique keychain name";
        return;
    }

    const QByteArray uuidAsByteArray = uuid.toByteArray();
    Q_ASSERT(uuidAsByteArray.size() > 2);
    Q_ASSERT(uuidAsByteArray.startsWith('{'));
    Q_ASSERT(uuidAsByteArray.endsWith('}'));
    const auto uuidAsString = QLatin1String(uuidAsByteArray.data(), uuidAsByteArray.size()).mid(1, uuidAsByteArray.size() - 2);

    const QString keychainName
            = QDir::tempPath() + QDir::separator() + uuidAsString + QLatin1String(".keychain");
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
        qCWarning(lcSsl) << "Failed to create a unique keychain name from"
                         << "QDir::tempPath()";
        return;
    }

    std::vector<uint8_t> passUtf8(256);
    if (SecRandomCopyBytes(kSecRandomDefault, passUtf8.size(), &passUtf8[0])) {
        qCWarning(lcSsl) << "SecRandomCopyBytes: failed to create a key";
        return;
    }

    const OSStatus status = SecKeychainCreate(&posixPath[0], passUtf8.size(),
                                              &passUtf8[0], FALSE, nullptr,
                                              &keychain);
    if (status != errSecSuccess || !keychain) {
        qCWarning(lcSsl) << "SecKeychainCreate: failed to create a custom keychain";
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
            qCWarning(lcSsl) << "SecKeychainSettings: failed to disable lock on sleep";
    }

#ifdef QSSLSOCKET_DEBUG
    if (keychain) {
        qCDebug(lcSsl) << "Custom keychain with name" << keychainName << "was created"
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

} // unnamed namespace

static SSLContextRef qt_createSecureTransportContext(QSslSocket::SslMode mode)
{
    const bool isServer = mode == QSslSocket::SslServerMode;
    const SSLProtocolSide side = isServer ? kSSLServerSide : kSSLClientSide;
    // We never use kSSLDatagramType, so it's kSSLStreamType unconditionally.
    SSLContextRef context = SSLCreateContext(nullptr, side, kSSLStreamType);
    if (!context)
        qCWarning(lcSsl) << "SSLCreateContext failed";
    return context;
}

static void qt_releaseSecureTransportContext(SSLContextRef context)
{
    if (context)
        CFRelease(context);
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

Q_GLOBAL_STATIC(QRecursiveMutex, qt_securetransport_mutex)

//#define QSSLSOCKET_DEBUG

bool QSslSocketPrivate::s_libraryLoaded = false;
bool QSslSocketPrivate::s_loadedCiphersAndCerts = false;
bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;


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

OSStatus QSslSocketBackendPrivate::ReadCallback(QSslSocketBackendPrivate *socket,
                                                char *data, size_t *dataLength)
{
    Q_ASSERT(socket);
    Q_ASSERT(data);
    Q_ASSERT(dataLength);

    QTcpSocket *plainSocket = socket->plainSocket;
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
    qCDebug(lcSsl) << plainSocket << "read" << bytes;
#endif
    if (bytes < 0) {
        *dataLength = 0;
        return errSecIO;
    }

    const OSStatus err = (size_t(bytes) < *dataLength) ? errSSLWouldBlock : errSecSuccess;
    *dataLength = bytes;

    return err;
}

OSStatus QSslSocketBackendPrivate::WriteCallback(QSslSocketBackendPrivate *socket,
                                                 const char *data, size_t *dataLength)
{
    Q_ASSERT(socket);
    Q_ASSERT(data);
    Q_ASSERT(dataLength);

    QTcpSocket *plainSocket = socket->plainSocket;
    Q_ASSERT(plainSocket);

    const qint64 bytes = plainSocket->write(data, *dataLength);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << plainSocket << "write" << bytes;
#endif
    if (bytes < 0) {
        *dataLength = 0;
        return errSecIO;
    }

    const OSStatus err = (size_t(bytes) < *dataLength) ? errSSLWouldBlock : errSecSuccess;
    *dataLength = bytes;

    return err;
}

void QSslSocketPrivate::ensureInitialized()
{
    const QMutexLocker locker(qt_securetransport_mutex);
    if (s_loadedCiphersAndCerts)
        return;

    // We have to set it before setDefaultSupportedCiphers,
    // since this function can trigger static (global)'s initialization
    // and as a result - recursive ensureInitialized call
    // from QSslCertificatePrivate's ctor.
    s_loadedCiphersAndCerts = true;

    const QSecureTransportContext context(qt_createSecureTransportContext(QSslSocket::SslClientMode));
    if (context) {
        QList<QSslCipher> ciphers;
        QList<QSslCipher> defaultCiphers;

        size_t numCiphers = 0;
        // Fails only if any of parameters is null.
        SSLGetNumberSupportedCiphers(context, &numCiphers);
        QVector<SSLCipherSuite> cfCiphers(numCiphers);
        // Fails only if any of parameter is null or number of ciphers is wrong.
        SSLGetSupportedCiphers(context, cfCiphers.data(), &numCiphers);

        for (size_t i = 0; i < size_t(cfCiphers.size()); ++i) {
            const QSslCipher ciph(QSslSocketBackendPrivate::QSslCipher_from_SSLCipherSuite(cfCiphers.at(i)));
            if (!ciph.isNull()) {
                ciphers << ciph;
                if (ciph.usedBits() >= 128)
                    defaultCiphers << ciph;
            }
        }

        setDefaultSupportedCiphers(ciphers);
        setDefaultCiphers(defaultCiphers);

        if (!s_loadRootCertsOnDemand)
            setDefaultCaCertificates(systemCaCertificates());
    } else {
        s_loadedCiphersAndCerts = false;
    }

}

long QSslSocketPrivate::sslLibraryVersionNumber()
{
    return 0;
}

QString QSslSocketPrivate::sslLibraryVersionString()
{
    return QLatin1String("Secure Transport, ") + QSysInfo::prettyProductName();
}

long QSslSocketPrivate::sslLibraryBuildVersionNumber()
{
    return 0;
}

QString QSslSocketPrivate::sslLibraryBuildVersionString()
{
    return sslLibraryVersionString();
}

bool QSslSocketPrivate::supportsSsl()
{
    return true;
}

void QSslSocketPrivate::resetDefaultCiphers()
{
    Q_UNIMPLEMENTED();
}

void QSslSocketPrivate::resetDefaultEllipticCurves()
{
    // No public API for this (?).
    Q_UNIMPLEMENTED();
}

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : context(nullptr)
{
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
    destroySslContext();
}

void QSslSocketBackendPrivate::continueHandshake()
{
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << plainSocket << "connection encrypted";
#endif
    Q_Q(QSslSocket);
    connectionEncrypted = true;

#if QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_13_4, __IPHONE_11_0, __TVOS_11_0, __WATCHOS_4_0)
    // Unlike OpenSSL, Secure Transport does not allow to negotiate protocols via
    // a callback during handshake. We can only set our list of preferred protocols
    // (and send it during handshake) and then receive what our peer has sent to us.
    // And here we can finally try to find a match (if any).
    if (__builtin_available(macOS 10.13, iOS 11.0, tvOS 11.0, watchOS 4.0, *)) {
        const auto &requestedProtocols = configuration.nextAllowedProtocols;
        if (const int requestedCount = requestedProtocols.size()) {
            configuration.nextProtocolNegotiationStatus = QSslConfiguration::NextProtocolNegotiationNone;
            configuration.nextNegotiatedProtocol.clear();

            QCFType<CFArrayRef> cfArray;
            const OSStatus result = SSLCopyALPNProtocols(context, &cfArray);
            if (result == errSecSuccess && cfArray && CFArrayGetCount(cfArray)) {
                const int size = CFArrayGetCount(cfArray);
                QVector<QString> peerProtocols(size);
                for (int i = 0; i < size; ++i)
                    peerProtocols[i] = QString::fromCFString((CFStringRef)CFArrayGetValueAtIndex(cfArray, i));

                for (int i = 0; i < requestedCount; ++i) {
                    const auto requestedName = QString::fromLatin1(requestedProtocols[i]);
                    for (int j = 0; j < size; ++j) {
                        if (requestedName == peerProtocols[j]) {
                            configuration.nextNegotiatedProtocol = requestedName.toLatin1();
                            configuration.nextProtocolNegotiationStatus = QSslConfiguration::NextProtocolNegotiationNegotiated;
                            break;
                        }
                    }
                    if (configuration.nextProtocolNegotiationStatus == QSslConfiguration::NextProtocolNegotiationNegotiated)
                        break;
                }
            }
        }
    }
#endif // QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE

    if (!renegotiating)
        emit q->encrypted();

    if (autoStartHandshake && pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }
}

void QSslSocketBackendPrivate::disconnected()
{
    if (plainSocket->bytesAvailable() <= 0)
        destroySslContext();
    // If there is still buffered data in the plain socket, don't destroy the ssl context yet.
    // It will be destroyed when the socket is deleted.
}

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (context) {
        if (!shutdown) {
            SSLClose(context);
            shutdown = true;
        }
    }
    plainSocket->disconnectFromHost();
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    SSLCipherSuite cipher = 0;
    if (context && SSLGetNegotiatedCipher(context, &cipher) == errSecSuccess)
        return QSslCipher_from_SSLCipherSuite(cipher);

    return QSslCipher();
}

QSsl::SslProtocol QSslSocketBackendPrivate::sessionProtocol() const
{
    if (!context)
        return QSsl::UnknownProtocol;

    SSLProtocol protocol = kSSLProtocolUnknown;
    const OSStatus err = SSLGetNegotiatedProtocolVersion(context, &protocol);
    if (err != errSecSuccess) {
        qCWarning(lcSsl) << "SSLGetNegotiatedProtocolVersion failed:" << err;
        return QSsl::UnknownProtocol;
    }

    switch (protocol) {
    case kSSLProtocol2:
        return QSsl::SslV2;
    case kSSLProtocol3:
        return QSsl::SslV3;
    case kTLSProtocol1:
        return QSsl::TlsV1_0;
    case kTLSProtocol11:
        return QSsl::TlsV1_1;
    case kTLSProtocol12:
        return QSsl::TlsV1_2;
    case kTLSProtocol13:
        return QSsl::TlsV1_3;
    default:
        return QSsl::UnknownProtocol;
    }
}

void QSslSocketBackendPrivate::startClientEncryption()
{
    if (!initSslContext()) {
        // Error description/code were set, 'error' emitted
        // by initSslContext, but OpenSSL socket also sets error
        // emits a signal twice, so ...
        setErrorAndEmit(QAbstractSocket::SslInternalError, QStringLiteral("Unable to init SSL Context"));
        return;
    }

    startHandshake();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    if (!initSslContext()) {
        // Error description/code were set, 'error' emitted
        // by initSslContext, but OpenSSL socket also sets error
        // emits a signal twice, so ...
        setErrorAndEmit(QAbstractSocket::SslInternalError, QStringLiteral("Unable to init SSL Context"));
        return;
    }

    startHandshake();
}

void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    // If we don't have any SSL context, don't bother transmitting.
    // Edit: if SSL session closed, don't bother either.
    if (!context || shutdown)
        return;

    if (!isHandshakeComplete())
        startHandshake();

    if (isHandshakeComplete() && !writeBuffer.isEmpty()) {
        qint64 totalBytesWritten = 0;
        while (writeBuffer.nextDataBlockSize() > 0 && context) {
            const size_t nextDataBlockSize = writeBuffer.nextDataBlockSize();
            size_t writtenBytes = 0;
            const OSStatus err = SSLWrite(context, writeBuffer.readPointer(), nextDataBlockSize, &writtenBytes);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << plainSocket << "SSLWrite returned" << err;
#endif
            if (err != errSecSuccess && err != errSSLWouldBlock) {
                setErrorAndEmit(QAbstractSocket::SslInternalError,
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
            if (!emittedBytesWritten) {
                emittedBytesWritten = true;
                emit q->bytesWritten(totalBytesWritten);
                emittedBytesWritten = false;
            }
            emit q->channelBytesWritten(0, totalBytesWritten);
        }
    }

    if (isHandshakeComplete()) {
        QVarLengthArray<char, 4096> data;
        while (context && (!readBufferMaxSize || buffer.size() < readBufferMaxSize)) {
            size_t readBytes = 0;
            data.resize(4096);
            const OSStatus err = SSLRead(context, data.data(), data.size(), &readBytes);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << plainSocket << "SSLRead returned" << err;
#endif
            if (err == errSSLClosedGraceful) {
                shutdown = true; // the other side shut down, make sure we do not send shutdown ourselves
                setErrorAndEmit(QAbstractSocket::RemoteHostClosedError,
                                QSslSocket::tr("The TLS/SSL connection has been closed"));
                break;
            } else if (err != errSecSuccess && err != errSSLWouldBlock) {
                setErrorAndEmit(QAbstractSocket::SslInternalError,
                                QStringLiteral("SSLRead failed: %1").arg(err));
                break;
            }

            if (err == errSSLWouldBlock && renegotiating) {
                startHandshake();
                break;
            }

            if (readBytes) {
                buffer.append(data.constData(), readBytes);
                if (readyReadEmittedPointer)
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
                emit q->channelReadyRead(0);
            }

            if (err == errSSLWouldBlock)
                break;
        }
    }
}


QList<QSslError> (QSslSocketBackendPrivate::verify)(QList<QSslCertificate> certificateChain, const QString &hostName)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(certificateChain)
    Q_UNUSED(hostName)

    QList<QSslError> errors;
    errors << QSslError(QSslError::UnspecifiedError);

    return errors;
}

bool QSslSocketBackendPrivate::importPkcs12(QIODevice *device,
                         QSslKey *key, QSslCertificate *cert,
                         QList<QSslCertificate> *caCertificates,
                         const QByteArray &passPhrase)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(device)
    Q_UNUSED(key)
    Q_UNUSED(cert)
    Q_UNUSED(caCertificates)
    Q_UNUSED(passPhrase)
    return false;
}

QSslCipher QSslSocketBackendPrivate::QSslCipher_from_SSLCipherSuite(SSLCipherSuite cipher)
{
    QSslCipher ciph;
    switch (cipher) {
    // Sorted as in CipherSuite.h (and groupped by their RFC)
    case SSL_RSA_WITH_NULL_MD5:
        ciph.d->name = QLatin1String("NULL-MD5");
        ciph.d->protocol = QSsl::SslV3;
        break;
    case SSL_RSA_WITH_NULL_SHA:
        ciph.d->name = QLatin1String("NULL-SHA");
        ciph.d->protocol = QSsl::SslV3;
        break;
    case SSL_RSA_WITH_RC4_128_MD5:
        ciph.d->name = QLatin1String("RC4-MD5");
        ciph.d->protocol = QSsl::SslV3;
        break;
    case SSL_RSA_WITH_RC4_128_SHA:
        ciph.d->name = QLatin1String("RC4-SHA");
        ciph.d->protocol = QSsl::SslV3;
        break;

    // TLS addenda using AES, per RFC 3268
    case TLS_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("AES128-SHA");
        break;
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("DHE-RSA-AES128-SHA");
        break;
    case TLS_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("AES256-SHA");
        break;
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("DHE-RSA-AES256-SHA");
        break;

    // ECDSA addenda, RFC 4492
    case TLS_ECDH_ECDSA_WITH_NULL_SHA:
        ciph.d->name = QLatin1String("ECDH-ECDSA-NULL-SHA");
        break;
    case TLS_ECDH_ECDSA_WITH_RC4_128_SHA:
        ciph.d->name = QLatin1String("ECDH-ECDSA-RC4-SHA");
        break;
    case TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-ECDSA-DES-CBC3-SHA");
        break;
    case TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-ECDSA-AES128-SHA");
        break;
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-ECDSA-AES256-SHA");
        break;
    case TLS_ECDHE_ECDSA_WITH_NULL_SHA:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-NULL-SHA");
        break;
    case TLS_ECDHE_ECDSA_WITH_RC4_128_SHA:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-RC4-SHA");
        break;
    case TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-DES-CBC3-SHA");
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-AES128-SHA");
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-AES256-SHA");
        break;
    case TLS_ECDH_RSA_WITH_NULL_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-NULL-SHA");
        break;
    case TLS_ECDH_RSA_WITH_RC4_128_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-RC4-SHA");
        break;
    case TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-DES-CBC3-SHA");
        break;
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-AES128-SHA");
        break;
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-AES256-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_NULL_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-NULL-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_RC4_128_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-RC4-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-DES-CBC3-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES128-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES256-SHA");
        break;

    // TLS 1.2 addenda, RFC 5246
    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("DES-CBC3-SHA");
        break;
    case TLS_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("AES128-SHA256");
        break;
    case TLS_RSA_WITH_AES_256_CBC_SHA256:
        ciph.d->name = QLatin1String("AES256-SHA256");
        break;
    case TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("DHE-RSA-DES-CBC3-SHA");
        break;
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("DHE-RSA-AES128-SHA256");
        break;
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA256:
        ciph.d->name = QLatin1String("DHE-RSA-AES256-SHA256");
        break;

    // Addendum from RFC 4279, TLS PSK
    // all missing atm.

    // RFC 4785 - Pre-Shared Key (PSK) Ciphersuites with NULL Encryption
    // all missing atm.

    // Addenda from rfc 5288 AES Galois Counter Mode (CGM) Cipher Suites for TLS
    case TLS_RSA_WITH_AES_256_GCM_SHA384:
        ciph.d->name = QLatin1String("AES256-GCM-SHA384");
        break;

    // RFC 5487 - PSK with SHA-256/384 and AES GCM
    // all missing atm.

    // Addenda from rfc 5289 Elliptic Curve Cipher Suites with HMAC SHA-256/384
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-AES128-SHA256");
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-AES256-SHA384");
        break;
    case TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDH-ECDSA-AES128-SHA256");
        break;
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDH-ECDSA-AES256-SHA384");
        break;
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES128-SHA256");
        break;
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES256-SHA384");
        break;
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDH-RSA-AES128-SHA256");
        break;
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDH-RSA-AES256-SHA384");
        break;

    // Addenda from rfc 5289 Elliptic Curve Cipher Suites
    // with SHA-256/384 and AES Galois Counter Mode (GCM)
    case TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES256-GCM-SHA384");
        break;

    default:
        return ciph;
    }
    ciph.d->isNull = false;

    // protocol
    if (ciph.d->protocol == QSsl::SslV3) {
        ciph.d->protocolString = QLatin1String("SSLv3");
    } else {
        ciph.d->protocol = QSsl::TlsV1_2;
        ciph.d->protocolString = QLatin1String("TLSv1.2");
    }

    const auto bits = ciph.d->name.splitRef(QLatin1Char('-'));
    if (bits.size() >= 2) {
        if (bits.size() == 2 || bits.size() == 3) {
            ciph.d->keyExchangeMethod = QLatin1String("RSA");
        } else if (bits.front() == QLatin1String("DH") || bits.front() == QLatin1String("DHE")) {
            ciph.d->keyExchangeMethod = QLatin1String("DH");
        } else if (bits.front() == QLatin1String("ECDH") || bits.front() == QLatin1String("ECDHE")) {
            ciph.d->keyExchangeMethod = QLatin1String("ECDH");
        } else {
            qCWarning(lcSsl) << "Unknown Kx" << ciph.d->name;
        }

        if (bits.size() == 2 || bits.size() == 3) {
            ciph.d->authenticationMethod = QLatin1String("RSA");
        } else if (ciph.d->name.contains(QLatin1String("-ECDSA-"))) {
            ciph.d->authenticationMethod = QLatin1String("ECDSA");
        } else if (ciph.d->name.contains(QLatin1String("-RSA-"))) {
            ciph.d->authenticationMethod = QLatin1String("RSA");
        } else {
            qCWarning(lcSsl) << "Unknown Au" << ciph.d->name;
        }

        if (ciph.d->name.contains(QLatin1String("RC4-"))) {
            ciph.d->encryptionMethod = QLatin1String("RC4(128)");
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains(QLatin1String("DES-CBC3-"))) {
            ciph.d->encryptionMethod = QLatin1String("3DES(168)");
            ciph.d->bits = 168;
            ciph.d->supportedBits = 168;
        } else if (ciph.d->name.contains(QLatin1String("AES128-"))) {
            ciph.d->encryptionMethod = QLatin1String("AES(128)");
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains(QLatin1String("AES256-GCM"))) {
            ciph.d->encryptionMethod = QLatin1String("AESGCM(256)");
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains(QLatin1String("AES256-"))) {
            ciph.d->encryptionMethod = QLatin1String("AES(256)");
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains(QLatin1String("NULL-"))) {
            ciph.d->encryptionMethod = QLatin1String("NULL");
        } else {
            qCWarning(lcSsl) << "Unknown Enc" << ciph.d->name;
        }
    }
    return ciph;
}

bool QSslSocketBackendPrivate::initSslContext()
{
    Q_Q(QSslSocket);

    Q_ASSERT_X(!context, Q_FUNC_INFO, "invalid socket state, context is not null");
    Q_ASSERT(plainSocket);

    context.reset(qt_createSecureTransportContext(mode));
    if (!context) {
        setErrorAndEmit(QAbstractSocket::SslInternalError, QStringLiteral("SSLCreateContext failed"));
        return false;
    }

    const OSStatus err = SSLSetIOFuncs(context,
                                       reinterpret_cast<SSLReadFunc>(&QSslSocketBackendPrivate::ReadCallback),
                                       reinterpret_cast<SSLWriteFunc>(&QSslSocketBackendPrivate::WriteCallback));
    if (err != errSecSuccess) {
        destroySslContext();
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QStringLiteral("SSLSetIOFuncs failed: %1").arg(err));
        return false;
    }

    SSLSetConnection(context, this);

    if (mode == QSslSocket::SslServerMode
        && !configuration.localCertificateChain.isEmpty()) {
        QString errorDescription;
        QAbstractSocket::SocketError errorCode = QAbstractSocket::UnknownSocketError;
        if (!setSessionCertificate(errorDescription, errorCode)) {
            destroySslContext();
            setErrorAndEmit(errorCode, errorDescription);
            return false;
        }
    }

    if (!setSessionProtocol()) {
        destroySslContext();
        setErrorAndEmit(QAbstractSocket::SslInternalError, QStringLiteral("Failed to set protocol version"));
        return false;
    }

#if QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_13_4, __IPHONE_11_0, __TVOS_11_0, __WATCHOS_4_0)
    if (__builtin_available(macOS 10.13, iOS 11.0, tvOS 11.0, watchOS 4.0, *)) {
        const auto protocolNames = configuration.nextAllowedProtocols;
        QCFType<CFMutableArrayRef> cfNames(CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks));
        if (cfNames) {
            for (const QByteArray &name : protocolNames) {
                if (name.size() > 255) {
                    qCWarning(lcSsl) << "TLS ALPN extension" << name
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
                    qCWarning(lcSsl) << "SSLSetALPNProtocols failed - too long protocol names?";
            }
        } else {
            qCWarning(lcSsl) << "failed to allocate ALPN names array";
        }
    }
#endif // QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE

    if (mode == QSslSocket::SslClientMode) {
        // enable Server Name Indication (SNI)
        QString tlsHostName(verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName);
        if (tlsHostName.isEmpty())
            tlsHostName = hostName;

        const QByteArray ace(QUrl::toAce(tlsHostName));
        SSLSetPeerDomainName(context, ace.data(), ace.size());
        // tell SecureTransport we handle peer verification ourselves
        OSStatus err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnServerAuth, true);
        if (err == errSecSuccess)
            err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnCertRequested, true);

        if (err != errSecSuccess) {
            destroySslContext();
            setErrorAndEmit(QSslSocket::SslInternalError,
                     QStringLiteral("SSLSetSessionOption failed: %1").arg(err));
            return false;
        }
        //
    } else {
        if (configuration.peerVerifyMode != QSslSocket::VerifyNone) {
            // kAlwaysAuthenticate - always fails even if we set break on client auth.
            OSStatus err = SSLSetClientSideAuthenticate(context, kTryAuthenticate);
            if (err == errSecSuccess) {
                // We'd like to verify peer ourselves, otherwise handshake will
                // most probably fail before we can do anything.
                err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnClientAuth, true);
            }

            if (err != errSecSuccess) {
                destroySslContext();
                setErrorAndEmit(QAbstractSocket::SslInternalError,
                         QStringLiteral("failed to set SSL context option in server mode: %1").arg(err));
                return false;
            }
        }
#if !defined(QT_PLATFORM_UIKIT)
        // No SSLSetDiffieHellmanParams on iOS; calling it is optional according to docs.
        SSLSetDiffieHellmanParams(context, dhparam, sizeof(dhparam));
#endif
    }
    return true;
}

void QSslSocketBackendPrivate::destroySslContext()
{
    context.reset(nullptr);
}

bool QSslSocketBackendPrivate::setSessionCertificate(QString &errorDescription, QAbstractSocket::SocketError &errorCode)
{
    Q_ASSERT_X(context, Q_FUNC_INFO, "invalid SSL context (null)");

    QSslCertificate localCertificate;
    if (!configuration.localCertificateChain.isEmpty())
        localCertificate = configuration.localCertificateChain.at(0);

    if (!localCertificate.isNull()) {
        // Require a private key as well.
        if (configuration.privateKey.isNull()) {
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("Cannot provide a certificate with no key");
            return false;
        }

        // import certificates and key
        const QString passPhrase(QString::fromLatin1("foobar"));
        QCFType<CFDataRef> pkcs12 = _q_makePkcs12(configuration.localCertificateChain,
                                                  configuration.privateKey, passPhrase).toCFData();
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
            qCWarning(lcSsl) << plainSocket
                       << QStringLiteral("SecPKCS12Import failed: %1").arg(err);
#endif
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("SecPKCS12Import failed: %1").arg(err);
            return false;
        }

        if (!CFArrayGetCount(items)) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcSsl) << plainSocket << "SecPKCS12Import returned no items";
#endif
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("SecPKCS12Import returned no items");
            return false;
        }

        CFDictionaryRef import = (CFDictionaryRef)CFArrayGetValueAtIndex(items, 0);
        SecIdentityRef identity = (SecIdentityRef)CFDictionaryGetValue(import, kSecImportItemIdentity);
        if (!identity) {
#ifdef QSSLSOCKET_DEBUG
            qCWarning(lcSsl) << plainSocket << "SecPKCS12Import returned no identity";
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
            qCWarning(lcSsl) << plainSocket
                       << QStringLiteral("Cannot set certificate and key: %1").arg(err);
#endif
            errorCode = QAbstractSocket::SslInvalidUserDataError;
            errorDescription = QStringLiteral("Cannot set certificate and key: %1").arg(err);
            return false;
        }
    }

    return true;
}

bool QSslSocketBackendPrivate::setSessionProtocol()
{
    Q_ASSERT_X(context, Q_FUNC_INFO, "invalid SSL context (null)");

    // QSsl::SslV2 == kSSLProtocol2 is disabled in Secure Transport and
    // always fails with errSSLIllegalParam:
    // if (version < MINIMUM_STREAM_VERSION || version > MAXIMUM_STREAM_VERSION)
    //     return errSSLIllegalParam;
    // where MINIMUM_STREAM_VERSION is SSL_Version_3_0, MAXIMUM_STREAM_VERSION is TLS_Version_1_2.
    if (configuration.protocol == QSsl::SslV2) {
        qCDebug(lcSsl) << "protocol QSsl::SslV2 is disabled";
        return false;
    }

    // SslV3 is unsupported.
    if (configuration.protocol == QSsl::SslV3) {
        qCDebug(lcSsl) << "protocol QSsl::SslV3 is disabled";
        return false;
    }

    // SecureTransport has kTLSProtocol13 constant and also, kTLSProtocolMaxSupported.
    // Calling SSLSetProtocolVersionMax/Min with any of these two constants results
    // in errInvalidParam and a failure to set the protocol version. This means
    // no TLS 1.3 on macOS and iOS.
    switch (configuration.protocol) {
    case QSsl::TlsV1_3:
    case QSsl::TlsV1_3OrLater:
        qCWarning(lcSsl) << plainSocket << "SecureTransport does not support TLS 1.3";
        return false;
    default:;
    }

    OSStatus err = errSecSuccess;

    if (configuration.protocol == QSsl::TlsV1_0) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.0";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
        if (err == errSecSuccess)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol1);
    } else if (configuration.protocol == QSsl::TlsV1_1) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.1";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol11);
        if (err == errSecSuccess)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol11);
    } else if (configuration.protocol == QSsl::TlsV1_2) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol12);
        if (err == errSecSuccess)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol == QSsl::AnyProtocol) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : any";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
    } else if (configuration.protocol == QSsl::TlsV1SslV3) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : SSLv3 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
        if (err == errSecSuccess)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol1);
    } else if (configuration.protocol == QSsl::SecureProtocols) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
    } else if (configuration.protocol == QSsl::TlsV1_0OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
    } else if (configuration.protocol == QSsl::TlsV1_1OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol11);
    } else if (configuration.protocol == QSsl::TlsV1_2OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol12);
    } else {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "no protocol version found in the configuration";
    #endif
        return false;
    }

    return err == errSecSuccess;
}

bool QSslSocketBackendPrivate::canIgnoreTrustVerificationFailure() const
{
    const QSslSocket::PeerVerifyMode verifyMode = configuration.peerVerifyMode;
    return mode == QSslSocket::SslServerMode
           && (verifyMode == QSslSocket::QueryPeer
               || verifyMode == QSslSocket::AutoVerifyPeer
               || verifyMode == QSslSocket::VerifyNone);
}

bool QSslSocketBackendPrivate::verifySessionProtocol() const
{
    bool protocolOk = false;
    if (configuration.protocol == QSsl::AnyProtocol)
        protocolOk = true;
    else if (configuration.protocol == QSsl::TlsV1SslV3)
        protocolOk = (sessionProtocol() == QSsl::TlsV1_0);
    else if (configuration.protocol == QSsl::SecureProtocols)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_0);
    else if (configuration.protocol == QSsl::TlsV1_0OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_0);
    else if (configuration.protocol == QSsl::TlsV1_1OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_1);
    else if (configuration.protocol == QSsl::TlsV1_2OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_2);
    else if (configuration.protocol == QSsl::TlsV1_3OrLater)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_3OrLater);
    else
        protocolOk = (sessionProtocol() == configuration.protocol);

    return protocolOk;
}

bool QSslSocketBackendPrivate::verifyPeerTrust()
{
    Q_Q(QSslSocket);

    const QSslSocket::PeerVerifyMode verifyMode = configuration.peerVerifyMode;
    const bool canIgnoreVerify = canIgnoreTrustVerificationFailure();

    Q_ASSERT_X(context, Q_FUNC_INFO, "invalid SSL context (null)");
    Q_ASSERT(plainSocket);

    QCFType<SecTrustRef> trust;
    OSStatus err = SSLCopyPeerTrust(context, &trust);
    // !trust - SSLCopyPeerTrust can return errSecSuccess but null trust.
    if (err != errSecSuccess || !trust) {
        if (!canIgnoreVerify) {
            setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError,
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
        setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError,
                        QStringLiteral("SecTrustEvaluate failed: %1").arg(err));
        plainSocket->disconnectFromHost();
        return false;
    }

    configuration.peerCertificate.clear();
    configuration.peerCertificateChain.clear();

    const CFIndex certCount = SecTrustGetCertificateCount(trust);
    for (CFIndex i = 0; i < certCount; ++i) {
        SecCertificateRef cert = SecTrustGetCertificateAtIndex(trust, i);
        QCFType<CFDataRef> derData = SecCertificateCopyData(cert);
        configuration.peerCertificateChain << QSslCertificate(QByteArray::fromCFData(derData), QSsl::Der);
    }

    if (configuration.peerCertificateChain.size())
        configuration.peerCertificate = configuration.peerCertificateChain.at(0);

    // Check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer):
    for (const QSslCertificate &cert : qAsConst(configuration.peerCertificateChain)) {
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
                                  && mode == QSslSocket::SslClientMode);
    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    if (!configuration.peerCertificate.isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        if (mode == QSslSocket::SslClientMode) {
            const QString peerName(verificationPeerName.isEmpty () ? q->peerName() : verificationPeerName);
            if (!isMatchingHostname(configuration.peerCertificate, peerName) && !canIgnoreVerify) {
                // No matches in common names or alternate names.
                const QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate);
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
    for (const QSslCertificate &cert : qAsConst(configuration.caCertificates)) {
        QCFType<CFDataRef> certData = cert.d->derData.toCFData();
        if (QCFType<SecCertificateRef> secRef = SecCertificateCreateWithData(nullptr, certData))
            CFArrayAppendValue(certArray, secRef);
        else
            qCWarning(lcSsl, "Failed to create SecCertificate from QSslCertificate");
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
            const QSslError error(QSslError::CertificateUntrusted, configuration.peerCertificate);
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
bool QSslSocketBackendPrivate::checkSslErrors()
{
    Q_Q(QSslSocket);
    if (sslErrors.isEmpty())
        return true;

    emit q->sslErrors(sslErrors);

    const bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                              || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                              && mode == QSslSocket::SslClientMode);
    const bool doEmitSslError = !verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            pauseSocketNotifiers(q);
            paused = true;
        } else {
            setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError,
                            sslErrors.constFirst().errorString());
            plainSocket->disconnectFromHost();
        }
        return false;
    }

    return true;
}

bool QSslSocketBackendPrivate::startHandshake()
{
    Q_ASSERT(context);
    Q_Q(QSslSocket);

    OSStatus err = SSLHandshake(context);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << plainSocket << "SSLHandhake returned" << err;
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
            setErrorAndEmit(errorCode, errorDescription);
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
        setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError,
                        QStringLiteral("SSLHandshake failed: %1").arg(err));
        plainSocket->disconnectFromHost();
        return false;
    }

    // Connection aborted during handshake phase.
    if (q->state() != QAbstractSocket::ConnectedState) {
        qCDebug(lcSsl) << "connection aborted";
        renegotiating = false;
        return false;
    }

    // check protocol version ourselves, as Secure Transport does not enforce
    // the requested min / max versions.
    if (!verifySessionProtocol()) {
        setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, QStringLiteral("Protocol version mismatch"));
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

QT_END_NAMESPACE
