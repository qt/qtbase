/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <QtCore/qcryptographichash.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qvector.h>
#include <QtCore/qmutex.h>
#include <QtCore/qdebug.h>

#include <algorithm>
#include <cstddef>

#include <QtCore/private/qcore_mac_p.h>

#ifdef Q_OS_OSX
#include <CoreServices/CoreServices.h>
#endif

QT_BEGIN_NAMESPACE

static SSLContextRef qt_createSecureTransportContext(QSslSocket::SslMode mode)
{
    const bool isServer = mode == QSslSocket::SslServerMode;
    SSLContextRef context = Q_NULLPTR;

#ifndef Q_OS_OSX
    const SSLProtocolSide side = isServer ? kSSLServerSide : kSSLClientSide;
    // We never use kSSLDatagramType, so it's kSSLStreamType unconditionally.
    context = SSLCreateContext(Q_NULLPTR, side, kSSLStreamType);
    if (!context)
        qCWarning(lcSsl) << "SSLCreateContext failed";
#else // Q_OS_OSX

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_8, __IPHONE_NA)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_8) {
        const SSLProtocolSide side = isServer ? kSSLServerSide : kSSLClientSide;
        // We never use kSSLDatagramType, so it's kSSLStreamType unconditionally.
        context = SSLCreateContext(Q_NULLPTR, side, kSSLStreamType);
        if (!context)
            qCWarning(lcSsl) << "SSLCreateContext failed";
    } else {
#else
    {
#endif
        const OSStatus errCode = SSLNewContext(isServer, &context);
        if (errCode != noErr || !context)
            qCWarning(lcSsl) << "SSLNewContext failed with error:" << errCode;
    }
#endif // !Q_OS_OSX

    return context;
}

static void qt_releaseSecureTransportContext(SSLContextRef context)
{
    if (!context)
        return;

#ifndef Q_OS_OSX
    CFRelease(context);
#else

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_8, __IPHONE_NA)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_8) {
        CFRelease(context);
    } else {
#else
    {
#endif // QT_MAC_PLATFORM_...
        const OSStatus errCode = SSLDisposeContext(context);
        if (errCode != noErr)
            qCWarning(lcSsl) << "SSLDisposeContext failed with error:" << errCode;
    }
#endif // !Q_OS_OSX
}

static bool qt_setSessionProtocol(SSLContextRef context, const QSslConfigurationPrivate &configuration,
                                  QTcpSocket *plainSocket)
{
    Q_ASSERT(context);

#ifndef QSSLSOCKET_DEBUG
    Q_UNUSED(plainSocket)
#endif

    OSStatus err = noErr;

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_8, __IPHONE_5_0)
    if (configuration.protocol == QSsl::SslV3) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : SSLv3";
    #endif
        err = SSLSetProtocolVersionMin(context, kSSLProtocol3);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kSSLProtocol3);
    } else if (configuration.protocol == QSsl::TlsV1_0) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.0";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol1);
    } else if (configuration.protocol == QSsl::TlsV1_1) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.1";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol11);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol11);
    } else if (configuration.protocol == QSsl::TlsV1_2) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol12);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol == QSsl::AnyProtocol) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : any";
    #endif
        // kSSLProtocol3, since kSSLProtocol2 is disabled:
        err = SSLSetProtocolVersionMin(context, kSSLProtocol3);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol == QSsl::TlsV1SslV3) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : SSLv3 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kSSLProtocol3);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol == QSsl::SecureProtocols) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol == QSsl::TlsV1_0OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol1);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol == QSsl::TlsV1_1OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol11);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else if (configuration.protocol == QSsl::TlsV1_2OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionMin(context, kTLSProtocol12);
        if (err == noErr)
            err = SSLSetProtocolVersionMax(context, kTLSProtocol12);
    } else {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "no protocol version found in the configuration";
    #endif
        return false;
    }
#endif

    return err == noErr;
}

#ifdef Q_OS_OSX

static bool qt_setSessionProtocolOSX(SSLContextRef context, const QSslConfigurationPrivate &configuration,
                                     QTcpSocket *plainSocket)
{
    // This function works with (now) deprecated API that does not even exist on
    // iOS but is the only API we have on OS X below 10.8

    // Without SSLSetProtocolVersionMin/Max functions it's quite difficult
    // to have the required result:
    // If we use SSLSetProtocolVersion - any constant except the ones with 'Only' suffix -
    // allows a negotiation and we can not set the lower limit.
    // SSLSetProtocolVersionEnabled supports only a limited subset of constants, if you believe their docs:
    // kSSLProtocol2
    // kSSLProtocol3
    // kTLSProtocol1
    // kSSLProtocolAll
    // Here we can only have a look into the SecureTransport's code and hope that what we see there
    // and what we have on 10.7 is similar:
    // SSLSetProtocoLVersionEnabled actually accepts other constants also,
    // called twice with two different protocols it sets a range,
    // called once with a protocol (when all protocols were disabled)
    // - only this protocol is enabled (without a lower limit negotiation).

    Q_ASSERT(context);

#ifndef QSSLSOCKET_DEBUG
    Q_UNUSED(plainSocket)
#endif

    OSStatus err = noErr;

    // First, disable ALL:
    if (SSLSetProtocolVersionEnabled(context, kSSLProtocolAll, false) != noErr)
        return false;

    if (configuration.protocol == QSsl::SslV3) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : SSLv3";
    #endif
        err = SSLSetProtocolVersion(context, kSSLProtocol3Only);
    } else if (configuration.protocol == QSsl::TlsV1_0) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.0";
    #endif
        err = SSLSetProtocolVersion(context, kTLSProtocol1Only);
    } else if (configuration.protocol == QSsl::TlsV1_1) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.1";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kTLSProtocol11, true);
    } else if (configuration.protocol == QSsl::TlsV1_2) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kTLSProtocol12, true);
    } else if (configuration.protocol == QSsl::AnyProtocol) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : any";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kSSLProtocolAll, true);
    } else if (configuration.protocol == QSsl::TlsV1SslV3) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : SSLv3 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kTLSProtocol12, true);
        if (err == noErr)
            err = SSLSetProtocolVersionEnabled(context, kSSLProtocol3, true);
    } else if (configuration.protocol == QSsl::SecureProtocols) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kTLSProtocol12, true);
        if (err == noErr)
            err = SSLSetProtocolVersionEnabled(context, kTLSProtocol1, true);
    } else if (configuration.protocol == QSsl::TlsV1_0OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kTLSProtocol12, true);
        if (err == noErr)
            err = SSLSetProtocolVersionEnabled(context, kTLSProtocol1, true);
    } else if (configuration.protocol == QSsl::TlsV1_1OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.1 - TLSv1.2";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kTLSProtocol12, true);
        if (err == noErr)
            err = SSLSetProtocolVersionEnabled(context, kTLSProtocol11, true);
    } else if (configuration.protocol == QSsl::TlsV1_2OrLater) {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "requesting : TLSv1.2";
    #endif
        err = SSLSetProtocolVersionEnabled(context, kTLSProtocol12, true);
    } else {
    #ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << plainSocket << "no protocol version found in the configuration";
    #endif
        return false;
    }

    return err == noErr;
}

#endif // Q_OS_OSX

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

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, qt_securetransport_mutex, (QMutex::Recursive))

//#define QSSLSOCKET_DEBUG

bool QSslSocketPrivate::s_libraryLoaded = false;
bool QSslSocketPrivate::s_loadedCiphersAndCerts = false;
bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;


#ifndef Q_OS_IOS // dhparam is not used on iOS. (see the SSLSetDiffieHellmanParams call below)
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

// No ioErr on iOS. (defined in MacErrors.h on OS X)
#ifdef Q_OS_IOS
#  define ioErr -36
#endif

static OSStatus _q_SSLRead(QTcpSocket *plainSocket, char *data, size_t *dataLength)
{
    Q_ASSERT(plainSocket);
    Q_ASSERT(data);
    Q_ASSERT(dataLength);

    const qint64 bytes = plainSocket->read(data, *dataLength);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << plainSocket << "read" << bytes;
#endif
    if (bytes < 0) {
        *dataLength = 0;
        return ioErr;
    }

    const OSStatus err = (size_t(bytes) < *dataLength) ? errSSLWouldBlock : noErr;
    *dataLength = bytes;

    return err;
}

static OSStatus _q_SSLWrite(QTcpSocket *plainSocket, const char *data, size_t *dataLength)
{
    Q_ASSERT(plainSocket);
    Q_ASSERT(data);
    Q_ASSERT(dataLength);

    const qint64 bytes = plainSocket->write(data, *dataLength);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << plainSocket << "write" << bytes;
#endif
    if (bytes < 0) {
        *dataLength = 0;
        return ioErr;
    }

    const OSStatus err = (size_t(bytes) < *dataLength) ? errSSLWouldBlock : noErr;
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
            const QSslCipher ciph(QSslSocketBackendPrivate::QSslCipher_from_SSLCipherSuite(cfCiphers[i]));
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
    return QStringLiteral("Secure Transport, ") + QSysInfo::prettyProductName();
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


QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    QList<QSslCertificate> systemCerts;
#ifdef Q_OS_OSX
    // SecTrustSettingsCopyCertificates is not defined on iOS.
    QCFType<CFArrayRef> cfCerts;
    OSStatus status = SecTrustSettingsCopyCertificates(kSecTrustSettingsDomainSystem, &cfCerts);
    if (status == noErr) {
        const CFIndex size = CFArrayGetCount(cfCerts);
        for (CFIndex i = 0; i < size; ++i) {
            SecCertificateRef cfCert = (SecCertificateRef)CFArrayGetValueAtIndex(cfCerts, i);
            QCFType<CFDataRef> derData = SecCertificateCopyData(cfCert);
            systemCerts << QSslCertificate(QByteArray::fromCFData(derData), QSsl::Der);
        }
    } else {
       // no detailed error handling here
       qCWarning(lcSsl) << "SecTrustSettingsCopyCertificates failed:" << status;
    }
#endif
    return systemCerts;
}

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : context(Q_NULLPTR)
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
    if (context && SSLGetNegotiatedCipher(context, &cipher) == noErr)
        return QSslCipher_from_SSLCipherSuite(cipher);

    return QSslCipher();
}

QSsl::SslProtocol QSslSocketBackendPrivate::sessionProtocol() const
{
    if (!context)
        return QSsl::UnknownProtocol;

    SSLProtocol protocol = kSSLProtocolUnknown;
    const OSStatus err = SSLGetNegotiatedProtocolVersion(context, &protocol);
    if (err != noErr) {
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
        setErrorAndEmit(QAbstractSocket::SslInternalError, "Unable to init SSL Context");
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
        setErrorAndEmit(QAbstractSocket::SslInternalError, "Unable to init SSL Context");
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

    if (!connectionEncrypted)
        startHandshake();

    if (connectionEncrypted && !writeBuffer.isEmpty()) {
        qint64 totalBytesWritten = 0;
        while (writeBuffer.nextDataBlockSize() > 0) {
            const size_t nextDataBlockSize = writeBuffer.nextDataBlockSize();
            size_t writtenBytes = 0;
            const OSStatus err = SSLWrite(context, writeBuffer.readPointer(), nextDataBlockSize, &writtenBytes);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << plainSocket << "SSLWrite returned" << err;
#endif
            if (err != noErr && err != errSSLWouldBlock) {
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
        }
    }

    if (connectionEncrypted) {
        QVarLengthArray<char, 4096> data;
        while (true) {
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
            } else if (err != noErr && err != errSSLWouldBlock) {
                setErrorAndEmit(QAbstractSocket::SslInternalError,
                                QStringLiteral("SSLRead failed: %1").arg(err));
                break;
            }

            if (readBytes) {
                char *const ptr = buffer.reserve(readBytes);
                std::copy(data.data(), data.data() + readBytes, ptr);
                if (readyReadEmittedPointer)
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
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

    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("DES-CBC3-SHA");
        break;
    case TLS_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("AES128-SHA");
        break;
    case TLS_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("AES128-SHA256");
        break;
    case TLS_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("AES256-SHA");
        break;
    case TLS_RSA_WITH_AES_256_CBC_SHA256:
        ciph.d->name = QLatin1String("AES256-SHA256");
        break;

    case TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("DHE-RSA-DES-CBC3-SHA");
        break;
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("DHE-RSA-AES128-SHA");
        break;
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("DHE-RSA-AES128-SHA256");
        break;
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("DHE-RSA-AES256-SHA");
        break;
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA256:
        ciph.d->name = QLatin1String("DHE-RSA-AES256-SHA256");
        break;

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
    case TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDH-ECDSA-AES128-SHA256");
        break;
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-ECDSA-AES256-SHA");
        break;
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDH-ECDSA-AES256-SHA384");
        break;

    case TLS_ECDH_RSA_WITH_NULL_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-NULL-SHA");
        break;
    case TLS_ECDH_RSA_WITH_RC4_128_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-AES256-SHA");
        break;
    case TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-DES-CBC3-SHA");
        break;
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-AES128-SHA");
        break;
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDH-RSA-AES128-SHA256");
        break;
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDH-RSA-AES256-SHA");
        break;
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDH-RSA-AES256-SHA384");
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
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-AES128-SHA256");
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-AES256-SHA");
        break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDHE-ECDSA-AES256-SHA384");
        break;

    case TLS_ECDHE_RSA_WITH_NULL_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-NULL-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_RC4_128_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES256-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-DES-CBC3-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES128-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES128-SHA256");
        break;
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES256-SHA");
        break;
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384:
        ciph.d->name = QLatin1String("ECDHE-RSA-AES256-SHA384");
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

    const QStringList bits = ciph.d->name.split('-');
    if (bits.size() >= 2) {
        if (bits.size() == 2 || bits.size() == 3) {
            ciph.d->keyExchangeMethod = QLatin1String("RSA");
        } else if (ciph.d->name.startsWith("DH-") || ciph.d->name.startsWith("DHE-")) {
            ciph.d->keyExchangeMethod = QLatin1String("DH");
        } else if (ciph.d->name.startsWith("ECDH-") || ciph.d->name.startsWith("ECDHE-")) {
            ciph.d->keyExchangeMethod = QLatin1String("ECDH");
        } else {
            qCWarning(lcSsl) << "Unknown Kx" << ciph.d->name;
        }

        if (bits.size() == 2 || bits.size() == 3) {
            ciph.d->authenticationMethod = QLatin1String("RSA");
        } else if (ciph.d->name.contains("-ECDSA-")) {
            ciph.d->authenticationMethod = QLatin1String("ECDSA");
        } else if (ciph.d->name.contains("-RSA-")) {
            ciph.d->authenticationMethod = QLatin1String("RSA");
        } else {
            qCWarning(lcSsl) << "Unknown Au" << ciph.d->name;
        }

        if (ciph.d->name.contains("RC4-")) {
            ciph.d->encryptionMethod = QLatin1String("RC4(128)");
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains("DES-CBC3-")) {
            ciph.d->encryptionMethod = QLatin1String("3DES(168)");
            ciph.d->bits = 168;
            ciph.d->supportedBits = 168;
        } else if (ciph.d->name.contains("AES128-")) {
            ciph.d->encryptionMethod = QLatin1String("AES(128)");
            ciph.d->bits = 128;
            ciph.d->supportedBits = 128;
        } else if (ciph.d->name.contains("AES256-")) {
            ciph.d->encryptionMethod = QLatin1String("AES(256)");
            ciph.d->bits = 256;
            ciph.d->supportedBits = 256;
        } else if (ciph.d->name.contains("NULL-")) {
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
        setErrorAndEmit(QAbstractSocket::SslInternalError, "SSLCreateContext failed");
        return false;
    }

    const OSStatus err = SSLSetIOFuncs(context, reinterpret_cast<SSLReadFunc>(&_q_SSLRead),
                                       reinterpret_cast<SSLWriteFunc>(&_q_SSLWrite));
    if (err != noErr) {
        destroySslContext();
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QStringLiteral("SSLSetIOFuncs failed: %1").arg(err));
        return false;
    }

    SSLSetConnection(context, plainSocket);

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
        setErrorAndEmit(QAbstractSocket::SslInternalError, "Failed to set protocol version");
        return false;
    }

#ifdef Q_OS_OSX
    if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_8) {
        // Starting from OS X 10.8 SSLSetSessionOption with kSSLSessionOptionBreakOnServerAuth/
        // kSSLSessionOptionBreakOnClientAuth disables automatic certificate validation.
        // But for OS X versions below 10.8 we have to do it explicitly:
        const OSStatus err = SSLSetEnableCertVerify(context, false);
        if (err != noErr) {
            destroySslContext();
            setErrorAndEmit(QSslSocket::SslInternalError,
                     QStringLiteral("SSLSetEnableCertVerify failed: %1").arg(err));
            return false;
        }
    }
#endif

    if (mode == QSslSocket::SslClientMode) {
        // enable Server Name Indication (SNI)
        QString tlsHostName(verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName);
        if (tlsHostName.isEmpty())
            tlsHostName = hostName;

        const QByteArray ace(QUrl::toAce(tlsHostName));
        SSLSetPeerDomainName(context, ace.data(), ace.size());
        // tell SecureTransport we handle peer verification ourselves
        OSStatus err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnServerAuth, true);
        if (err == noErr)
            err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnCertRequested, true);

        if (err != noErr) {
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
            if (err == noErr) {
                // We'd like to verify peer ourselves, otherwise handshake will
                // most probably fail before we can do anything.
                err = SSLSetSessionOption(context, kSSLSessionOptionBreakOnClientAuth, true);
            }

            if (err != noErr) {
                destroySslContext();
                setErrorAndEmit(QAbstractSocket::SslInternalError,
                         QStringLiteral("failed to set SSL context option in server mode: %1").arg(err));
                return false;
            }
        }
#ifndef Q_OS_IOS
        // No SSLSetDiffieHellmanParams on iOS; calling it is optional according to docs.
        SSLSetDiffieHellmanParams(context, dhparam, sizeof(dhparam));
#endif
    }
    return true;
}

void QSslSocketBackendPrivate::destroySslContext()
{
    context.reset(Q_NULLPTR);
}

static QByteArray _q_makePkcs12(const QList<QSslCertificate> &certs, const QSslKey &key, const QString &passPhrase);


bool QSslSocketBackendPrivate::setSessionCertificate(QString &errorDescription, QAbstractSocket::SocketError &errorCode)
{
    Q_ASSERT_X(context, Q_FUNC_INFO, "invalid SSL context (null)");

    QSslCertificate localCertificate;
    if (!configuration.localCertificateChain.isEmpty())
        localCertificate = configuration.localCertificateChain[0];

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
        const void *keys[] = { kSecImportExportPassphrase };
        const void *values[] = { password };
        QCFType<CFDictionaryRef> options(CFDictionaryCreate(Q_NULLPTR, keys, values, 1,
                                                            Q_NULLPTR, Q_NULLPTR));
        CFArrayRef items = Q_NULLPTR;
        OSStatus err = SecPKCS12Import(pkcs12, options, &items);
        if (err != noErr) {
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

        QCFType<CFMutableArrayRef> certs = CFArrayCreateMutable(Q_NULLPTR, 0, &kCFTypeArrayCallBacks);
        if (!certs) {
            errorCode = QAbstractSocket::SslInternalError;
            errorDescription = QStringLiteral("Failed to allocate certificates array");
            return false;
        }

        CFArrayAppendValue(certs, identity);

        QCFType<CFArrayRef> chain((CFArrayRef)CFDictionaryGetValue(import, kSecImportItemCertChain));
        if (chain) {
            for (CFIndex i = 1, e = CFArrayGetCount(chain); i < e; ++i)
                CFArrayAppendValue(certs, CFArrayGetValueAtIndex(chain, i));
        }

        err = SSLSetCertificate(context, certs);
        if (err != noErr) {
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

    // QSsl::SslV2 == kSSLProtocol2 is disabled in secure transport and
    // always fails with errSSLIllegalParam:
    // if (version < MINIMUM_STREAM_VERSION || version > MAXIMUM_STREAM_VERSION)
    //     return errSSLIllegalParam;
    // where MINIMUM_STREAM_VERSION is SSL_Version_3_0, MAXIMUM_STREAM_VERSION is TLS_Version_1_2.
    if (configuration.protocol == QSsl::SslV2) {
        qCDebug(lcSsl) << "protocol QSsl::SslV2 is disabled";
        return false;
    }

#ifndef Q_OS_OSX
    return qt_setSessionProtocol(context, configuration, plainSocket);
#else

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_8, __IPHONE_NA)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_8) {
        return qt_setSessionProtocol(context, configuration, plainSocket);
    } else {
#else
    {
#endif
        return qt_setSessionProtocolOSX(context, configuration, plainSocket);
    }
#endif
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
        protocolOk = (sessionProtocol() >= QSsl::SslV3);
    else if (configuration.protocol == QSsl::SecureProtocols)
        protocolOk = (sessionProtocol() >= QSsl::TlsV1_0);
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
    // !trust - SSLCopyPeerTrust can return noErr but null trust.
    if (err != noErr || !trust) {
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
    // store certificates
    const int certCount = SecTrustGetCertificateCount(trust);
    // TODO: why this test depends on configuration.peerCertificateChain not being empty????
    if (configuration.peerCertificateChain.isEmpty()) {
        // Apple's docs say SetTrustEvaluate must be called before
        // SecTrustGetCertificateAtIndex, but this results
        // in 'kSecTrustResultRecoverableTrustFailure', so
        // here we just ignore 'res' (later we'll use SetAnchor etc.
        // and evaluate again).
        SecTrustResultType res = kSecTrustResultInvalid;
        err = SecTrustEvaluate(trust, &res);
        if (err != noErr) {
            // We can not ignore this, it's not even about trust verification
            // probably ...
            setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError,
                            QStringLiteral("SecTrustEvaluate failed: %1").arg(err));
            plainSocket->disconnectFromHost();
            return false;
        }

        for (int i = 0; i < certCount; ++i) {
            SecCertificateRef cert = SecTrustGetCertificateAtIndex(trust, i);
            QCFType<CFDataRef> derData = SecCertificateCopyData(cert);
            configuration.peerCertificateChain << QSslCertificate(QByteArray::fromCFData(derData), QSsl::Der);
        }
    }

    if (certCount > 0) {
        SecCertificateRef cert = SecTrustGetCertificateAtIndex(trust, 0);
        QCFType<CFDataRef> derData = SecCertificateCopyData(cert);
        configuration.peerCertificate = QSslCertificate(QByteArray::fromCFData(derData), QSsl::Der);
    }

    // check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer)
    foreach (const QSslCertificate &cert, configuration.peerCertificateChain) {
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
    QCFType<CFMutableArrayRef> certArray = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    foreach (const QSslCertificate &cert, configuration.caCertificates) {
        QCFType<CFDataRef> certData = cert.d->derData.toCFData();
        QCFType<SecCertificateRef> certRef = SecCertificateCreateWithData(NULL, certData);
        CFArrayAppendValue(certArray, certRef);
    }
    SecTrustSetAnchorCertificates(trust, certArray);
    SecTrustSetAnchorCertificatesOnly(trust, false);

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
        if (!checkSslErrors())
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
                            sslErrors.first().errorString());
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

        setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError,
                        QStringLiteral("SSLHandshake failed: %1").arg(err));
        plainSocket->disconnectFromHost();
        return false;
    }

    // Connection aborted during handshake phase.
    if (q->state() != QAbstractSocket::ConnectedState) {
        qCDebug(lcSsl) << "connection aborted";
        return false;
    }

    // check protocol version ourselves, as Secure Transport does not enforce
    // the requested min / max versions.
    if (!verifySessionProtocol()) {
        setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError,
                 "Protocol version mismatch");
        plainSocket->disconnectFromHost();
        return false;
    }

    if (verifyPeerTrust()) {
        continueHandshake();
        return true;
    } else {
        return false;
    }
}

/*
    PKCS12 helpers.
*/

static QAsn1Element wrap(quint8 type, const QAsn1Element &child)
{
    QByteArray value;
    QDataStream stream(&value, QIODevice::WriteOnly);
    child.write(stream);
    return QAsn1Element(type, value);
}

static QAsn1Element _q_PKCS7_data(const QByteArray &data)
{
    QVector<QAsn1Element> items;
    items << QAsn1Element::fromObjectId("1.2.840.113549.1.7.1");
    items << wrap(QAsn1Element::Context0Type,
                  QAsn1Element(QAsn1Element::OctetStringType, data));
    return QAsn1Element::fromVector(items);
}

/*!
    PKCS #12 key derivation.

    Some test vectors:
    http://www.drh-consultancy.demon.co.uk/test.txt
*/
static QByteArray _q_PKCS12_keygen(char id, const QByteArray &salt, const QString &passPhrase, int n, int r)
{
    const int u = 20;
    const int v = 64;

    // password formatting
    QByteArray passUnicode(passPhrase.size() * 2 + 2, '\0');
    char *p = passUnicode.data();
    for (int i = 0; i < passPhrase.size(); ++i) {
        quint16 ch = passPhrase[i].unicode();
        *(p++) = (ch & 0xff00) >> 8;
        *(p++) = (ch & 0xff);
    }

    // prepare I
    QByteArray D(64, id);
    QByteArray S, P;
    const int sSize = v * ((salt.size() + v - 1) / v);
    S.resize(sSize);
    for (int i = 0; i < sSize; ++i) {
        S[i] = salt[i % salt.size()];
    }
    const int pSize = v * ((passUnicode.size() + v - 1) / v);
    P.resize(pSize);
    for (int i = 0; i < pSize; ++i) {
        P[i] = passUnicode[i % passUnicode.size()];
    }
    QByteArray I = S + P;

    // apply hashing
    const int c = (n + u - 1) / u;
    QByteArray A;
    QByteArray B;
    B.resize(v);
    QCryptographicHash hash(QCryptographicHash::Sha1);
    for (int i = 0; i < c; ++i) {
        // hash r iterations
        QByteArray Ai = D + I;
        for (int j = 0; j < r; ++j) {
            hash.reset();
            hash.addData(Ai);
            Ai = hash.result();
        }

        for (int j = 0; j < v; ++j) {
            B[j] = Ai[j % u];
        }

        // modify I as Ij = (Ij + B + 1) modulo 2^v
        for (int p = 0; p < I.size(); p += v) {
            quint8 carry = 1;
            for (int j = v - 1; j >= 0; --j) {
                quint16 v = quint8(I[p+j]) + quint8(B[j]) + carry;
                I[p+j] = v & 0xff;
                carry = (v & 0xff00) >> 8;
            }
        }
        A += Ai;
    }
    return A.left(n);
}

static QByteArray _q_PKCS12_salt()
{
    QByteArray salt;
    salt.resize(8);
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = (qrand() & 0xff);
    }
    return salt;
}

static QByteArray _q_PKCS12_certBag(const QSslCertificate &cert)
{
    QVector<QAsn1Element> items;
    items << QAsn1Element::fromObjectId("1.2.840.113549.1.12.10.1.3");

    // certificate
    QVector<QAsn1Element> certItems;
    certItems << QAsn1Element::fromObjectId("1.2.840.113549.1.9.22.1");
    certItems << wrap(QAsn1Element::Context0Type,
                      QAsn1Element(QAsn1Element::OctetStringType, cert.toDer()));
    items << wrap(QAsn1Element::Context0Type,
                  QAsn1Element::fromVector(certItems));

    // local key id
    const QByteArray localKeyId = cert.digest(QCryptographicHash::Sha1);
    QVector<QAsn1Element> idItems;
    idItems << QAsn1Element::fromObjectId("1.2.840.113549.1.9.21");
    idItems << wrap(QAsn1Element::SetType,
                    QAsn1Element(QAsn1Element::OctetStringType, localKeyId));
    items << wrap(QAsn1Element::SetType, QAsn1Element::fromVector(idItems));

    // dump
    QAsn1Element root = wrap(QAsn1Element::SequenceType, QAsn1Element::fromVector(items));
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

static QAsn1Element _q_PKCS12_key(const QSslKey &key)
{
    Q_ASSERT(key.algorithm() == QSsl::Rsa || key.algorithm() == QSsl::Dsa);

    QVector<QAsn1Element> keyItems;
    keyItems << QAsn1Element::fromInteger(0);
    QVector<QAsn1Element> algoItems;
    if (key.algorithm() == QSsl::Rsa)
        algoItems << QAsn1Element::fromObjectId(RSA_ENCRYPTION_OID);
    else if (key.algorithm() == QSsl::Dsa)
        algoItems << QAsn1Element::fromObjectId(DSA_ENCRYPTION_OID);
    algoItems << QAsn1Element(QAsn1Element::NullType);
    keyItems << QAsn1Element::fromVector(algoItems);
    keyItems << QAsn1Element(QAsn1Element::OctetStringType, key.toDer());
    return QAsn1Element::fromVector(keyItems);
}

static QByteArray _q_PKCS12_shroudedKeyBag(const QSslKey &key, const QString &passPhrase, const QByteArray &localKeyId)
{
    const int iterations = 2048;
    QByteArray salt = _q_PKCS12_salt();
    QByteArray cKey = _q_PKCS12_keygen(1, salt, passPhrase, 24, iterations);
    QByteArray cIv = _q_PKCS12_keygen(2, salt, passPhrase, 8, iterations);

    // prepare and encrypt data
    QByteArray plain;
    QDataStream plainStream(&plain, QIODevice::WriteOnly);
    _q_PKCS12_key(key).write(plainStream);
    QByteArray crypted = QSslKeyPrivate::encrypt(QSslKeyPrivate::DesEde3Cbc,
        plain, cKey, cIv);

    QVector<QAsn1Element> items;
    items << QAsn1Element::fromObjectId("1.2.840.113549.1.12.10.1.2");

    // key
    QVector<QAsn1Element> keyItems;
    QVector<QAsn1Element> algoItems;
    algoItems << QAsn1Element::fromObjectId("1.2.840.113549.1.12.1.3");
    QVector<QAsn1Element> paramItems;
    paramItems << QAsn1Element(QAsn1Element::OctetStringType, salt);
    paramItems << QAsn1Element::fromInteger(iterations);
    algoItems << QAsn1Element::fromVector(paramItems);
    keyItems << QAsn1Element::fromVector(algoItems);
    keyItems << QAsn1Element(QAsn1Element::OctetStringType, crypted);
    items << wrap(QAsn1Element::Context0Type,
                  QAsn1Element::fromVector(keyItems));

    // local key id
    QVector<QAsn1Element> idItems;
    idItems << QAsn1Element::fromObjectId("1.2.840.113549.1.9.21");
    idItems << wrap(QAsn1Element::SetType,
                    QAsn1Element(QAsn1Element::OctetStringType, localKeyId));
    items << wrap(QAsn1Element::SetType,
                  QAsn1Element::fromVector(idItems));

    // dump
    QAsn1Element root = wrap(QAsn1Element::SequenceType, QAsn1Element::fromVector(items));
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

static QByteArray _q_PKCS12_bag(const QList<QSslCertificate> &certs, const QSslKey &key, const QString &passPhrase)
{
    QVector<QAsn1Element> items;

    // certs
    for (int i = 0; i < certs.size(); ++i)
        items << _q_PKCS7_data(_q_PKCS12_certBag(certs[i]));

    // key
    const QByteArray localKeyId = certs.first().digest(QCryptographicHash::Sha1);
    items << _q_PKCS7_data(_q_PKCS12_shroudedKeyBag(key, passPhrase, localKeyId));

    // dump
    QAsn1Element root = QAsn1Element::fromVector(items);
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

static QAsn1Element _q_PKCS12_mac(const QByteArray &data, const QString &passPhrase)
{
    const int iterations = 2048;

    // salt generation
    QByteArray macSalt = _q_PKCS12_salt();
    QByteArray key = _q_PKCS12_keygen(3, macSalt, passPhrase, 20, iterations);

    // HMAC calculation
    QMessageAuthenticationCode hmac(QCryptographicHash::Sha1, key);
    hmac.addData(data);

    QVector<QAsn1Element> algoItems;
    algoItems << QAsn1Element::fromObjectId("1.3.14.3.2.26");
    algoItems << QAsn1Element(QAsn1Element::NullType);

    QVector<QAsn1Element> digestItems;
    digestItems << QAsn1Element::fromVector(algoItems);
    digestItems << QAsn1Element(QAsn1Element::OctetStringType, hmac.result());

    QVector<QAsn1Element> macItems;
    macItems << QAsn1Element::fromVector(digestItems);
    macItems << QAsn1Element(QAsn1Element::OctetStringType, macSalt);
    macItems << QAsn1Element::fromInteger(iterations);
    return QAsn1Element::fromVector(macItems);
}

QByteArray _q_makePkcs12(const QList<QSslCertificate> &certs, const QSslKey &key, const QString &passPhrase)
{
    QVector<QAsn1Element> items;

    // version
    items << QAsn1Element::fromInteger(3);

    // auth safe
    const QByteArray data = _q_PKCS12_bag(certs, key, passPhrase);
    items << _q_PKCS7_data(data);

    // HMAC
    items << _q_PKCS12_mac(data, passPhrase);

    // dump
    QAsn1Element root = QAsn1Element::fromVector(items);
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    root.write(stream);
    return ba;
}

QT_END_NAMESPACE
