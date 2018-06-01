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

#include "qssl_p.h"
#include "qsslsocket_winrt_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSysInfo>
#include <QtCore/qfunctions_winrt.h>
#include <private/qnativesocketengine_winrt_p.h>
#include <private/qeventdispatcher_winrt_p.h>

#include <windows.networking.h>
#include <windows.networking.sockets.h>
#include <windows.security.cryptography.certificates.h>
#include <robuffer.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Sockets;
using namespace ABI::Windows::Security::Cryptography::Certificates;
using namespace ABI::Windows::Storage::Streams;

QT_BEGIN_NAMESPACE

bool QSslSocketPrivate::s_libraryLoaded = true;
bool QSslSocketPrivate::s_loadRootCertsOnDemand = true;
bool QSslSocketPrivate::s_loadedCiphersAndCerts = false;

struct SslSocketGlobal
{
    SslSocketGlobal()
    {
        HRESULT hr;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                                  &hostNameFactory);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<ICertificateStoresStatics> certificateStores;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Certificates_CertificateStores).Get(),
                                  &certificateStores);
        Q_ASSERT_SUCCEEDED(hr);

        hr = certificateStores->get_TrustedRootCertificationAuthorities(&rootStore);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<IAsyncOperation<IVectorView<Certificate *> *>> op;
        hr = certificateStores->FindAllAsync(&op);
        Q_ASSERT_SUCCEEDED(hr);
        ComPtr<IVectorView<Certificate *>> certificates;
        hr = QWinRTFunctions::await(op, certificates.GetAddressOf());
        Q_ASSERT_SUCCEEDED(hr);
        quint32 size;
        hr = certificates->get_Size(&size);
        Q_ASSERT_SUCCEEDED(hr);
        for (quint32 i = 0; i < size; ++i) {
            ComPtr<ICertificate> certificate;
            hr = certificates->GetAt(i, &certificate);
            Q_ASSERT_SUCCEEDED(hr);
            systemCaCertificates.append(QSslCertificatePrivate::QSslCertificate_from_Certificate(certificate.Get()));
        }
    }

    void syncCaCertificates(const QSet<QSslCertificate> &add, const QSet<QSslCertificate> &remove)
    {
        QMutexLocker locker(&certificateMutex);
        for (const QSslCertificate &certificate : add) {
            QHash<QSslCertificate, QAtomicInt>::iterator it = additionalCertificates.find(certificate);
            if (it != additionalCertificates.end()) {
                it.value().ref(); // Add a reference
            } else {
                // install certificate
                HRESULT hr;
                hr = rootStore->Add(static_cast<ICertificate *>(certificate.handle()));
                Q_ASSERT_SUCCEEDED(hr);
                additionalCertificates.insert(certificate, 1);
            }
        }
        for (const QSslCertificate &certificate : remove) {
            QHash<QSslCertificate, QAtomicInt>::iterator it = additionalCertificates.find(certificate);
            if (it != additionalCertificates.end() && !it.value().deref()) {
                // no more references, remove certificate
                HRESULT hr;
                hr = rootStore->Delete(static_cast<ICertificate *>(certificate.handle()));
                Q_ASSERT_SUCCEEDED(hr);
                additionalCertificates.erase(it);
            }
        }
    }

    ComPtr<IHostNameFactory> hostNameFactory;
    QList<QSslCertificate> systemCaCertificates;

private:
    QMutex certificateMutex;
    ComPtr<ICertificateStore> rootStore;
    QHash<QSslCertificate, QAtomicInt> additionalCertificates;
};
Q_GLOBAL_STATIC(SslSocketGlobal, g)

// Called on the socket's thread to avoid cross-thread deletion
void QSslSocketConnectionHelper::disconnectSocketFromHost()
{
    if (d->plainSocket)
        d->plainSocket->disconnectFromHost();
}

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : connectionHelper(new QSslSocketConnectionHelper(this))
{
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
    g->syncCaCertificates(QSet<QSslCertificate>(), previousCaCertificates);
}

void QSslSocketPrivate::deinitialize()
{
    Q_UNIMPLEMENTED();
}

bool QSslSocketPrivate::supportsSsl()
{
    return true;
}

void QSslSocketPrivate::ensureInitialized()
{
    if (s_loadedCiphersAndCerts)
        return;
    s_loadedCiphersAndCerts = true;
    resetDefaultCiphers();
}

long QSslSocketPrivate::sslLibraryVersionNumber()
{
    return QSysInfo::windowsVersion();
}

QString QSslSocketPrivate::sslLibraryVersionString()
{
    return QStringLiteral("Windows Runtime, ") + QSysInfo::prettyProductName();
}

long QSslSocketPrivate::sslLibraryBuildVersionNumber()
{
    Q_UNIMPLEMENTED();
    return 0;
}

QString QSslSocketPrivate::sslLibraryBuildVersionString()
{
    Q_UNIMPLEMENTED();
    return QString::number(sslLibraryBuildVersionNumber());
}

void QSslSocketPrivate::resetDefaultCiphers()
{
    setDefaultSupportedCiphers(QSslSocketBackendPrivate::defaultCiphers());
    setDefaultCiphers(QSslSocketBackendPrivate::defaultCiphers());
}


QList<QSslCipher> QSslSocketBackendPrivate::defaultCiphers()
{
    QList<QSslCipher> ciphers;
    const QString protocolStrings[] = { QStringLiteral("SSLv3"), QStringLiteral("TLSv1"),
                                        QStringLiteral("TLSv1.1"), QStringLiteral("TLSv1.2") };
    const QSsl::SslProtocol protocols[] = { QSsl::SslV3, QSsl::TlsV1_0, QSsl::TlsV1_1, QSsl::TlsV1_2 };
    const int size = static_cast<int>(ARRAYSIZE(protocols));
    ciphers.reserve(size);
    for (int i = 0; i < size; ++i) {
        QSslCipher cipher;
        cipher.d->isNull = false;
        cipher.d->name = QStringLiteral("WINRT");
        cipher.d->protocol = protocols[i];
        cipher.d->protocolString = protocolStrings[i];
        ciphers.append(cipher);
    }
    return ciphers;
}

QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    return g->systemCaCertificates;
}

void QSslSocketBackendPrivate::startClientEncryption()
{
    Q_Q(QSslSocket);

    QSsl::SslProtocol protocol = q->protocol();
    switch (q->protocol()) {
    case QSsl::AnyProtocol:
    case QSsl::SslV3:
    case QSsl::TlsV1SslV3:
        protectionLevel = SocketProtectionLevel_Ssl; // Only use this value if weak cipher support is required
        break;
    case QSsl::TlsV1_0:
        protectionLevel = SocketProtectionLevel_Tls10;
        break;
    case QSsl::TlsV1_1:
        protectionLevel = SocketProtectionLevel_Tls11;
        break;
    case QSsl::TlsV1_2:
        protectionLevel = SocketProtectionLevel_Tls12;
        break;
    case QSsl::TlsV1_0OrLater:
    case QSsl::TlsV1_1OrLater:
    case QSsl::TlsV1_2OrLater:
        // TlsV1_0OrLater, TlsV1_1OrLater and TlsV1_2OrLater are disabled on WinRT
        // because there is no good way to map them to the native API.
        setErrorAndEmit(QAbstractSocket::SslInvalidUserDataError,
                        QStringLiteral("unsupported protocol"));
        return;
    case QSsl::SecureProtocols:
        // SocketProtectionLevel_Tls12 actually means "use TLS1.0, 1.1 or 1.2"
        // https://docs.microsoft.com/en-us/uwp/api/windows.networking.sockets.socketprotectionlevel
        protectionLevel = SocketProtectionLevel_Tls12;
        break;
    default:
        protectionLevel = SocketProtectionLevel_Tls12; // default to highest
        protocol = QSsl::TlsV1_2;
        break;
    }

    // Sync custom certificates
    const QSet<QSslCertificate> caCertificates = configuration.caCertificates.toSet();
    const QSet<QSslCertificate> newCertificates = caCertificates - previousCaCertificates;
    const QSet<QSslCertificate> oldCertificates = previousCaCertificates - caCertificates;
    g->syncCaCertificates(newCertificates, oldCertificates);
    previousCaCertificates = caCertificates;

    continueHandshake();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    Q_UNIMPLEMENTED();
}

void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    if (connectionEncrypted && !writeBuffer.isEmpty()) {
        qint64 totalBytesWritten = 0;
        int nextDataBlockSize;
        while ((nextDataBlockSize = writeBuffer.nextDataBlockSize()) > 0) {
            int writtenBytes = plainSocket->write(writeBuffer.readPointer(), nextDataBlockSize);
            writtenBytes = nextDataBlockSize;

            writeBuffer.free(writtenBytes);
            totalBytesWritten += writtenBytes;

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

    // Check if we've got any data to be read from the socket.
    int pendingBytes;
    bool bytesRead = false;
    while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
        char *ptr = buffer.reserve(pendingBytes);
        int readBytes = plainSocket->read(ptr, pendingBytes);
        buffer.chop(pendingBytes - readBytes);
        bytesRead = true;
    }

    if (bytesRead) {
        if (readyReadEmittedPointer)
            *readyReadEmittedPointer = true;
        emit q->readyRead();
        emit q->channelReadyRead(0);
    }

    if (pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }
}

void QSslSocketBackendPrivate::disconnectFromHost()
{
    QMetaObject::invokeMethod(connectionHelper.data(), "disconnectSocketFromHost", Qt::QueuedConnection);
}

void QSslSocketBackendPrivate::disconnected()
{
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    return configuration.sessionCipher;
}

QSsl::SslProtocol QSslSocketBackendPrivate::sessionProtocol() const
{
    return configuration.sessionCipher.protocol();
}

void QSslSocketBackendPrivate::continueHandshake()
{
    IStreamSocket *socket = reinterpret_cast<IStreamSocket *>(plainSocket->socketDescriptor());
    if (qintptr(socket) == -1) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QStringLiteral("At attempt was made to continue the handshake on an invalid socket."));
        return;
    }

    HRESULT hr;
    ComPtr<IHostName> hostName;
    const QString host = verificationPeerName.isEmpty() ? plainSocket->peerName()
                                                        : verificationPeerName;
    if (host.isEmpty()) {
        ComPtr<IStreamSocketInformation> info;
        hr = socket->get_Information(&info);
        Q_ASSERT_SUCCEEDED(hr);
        hr = info->get_RemoteAddress(&hostName);
    } else {
        HStringReference hostRef(reinterpret_cast<LPCWSTR>(host.utf16()), host.length());
        hr = g->hostNameFactory->CreateHostName(hostRef.Get(), &hostName);
        Q_ASSERT_SUCCEEDED(hr);
    }
    if (FAILED(hr)) {
        setErrorAndEmit(QAbstractSocket::SslInvalidUserDataError, qt_error_string(hr));
        return;
    }

    ComPtr<IStreamSocketControl> control;
    hr = socket->get_Control(&control);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IStreamSocketControl2> control2;
    hr = control.As(&control2);
    ComPtr<IVector<ChainValidationResult>> ignoreList;
    hr = control2->get_IgnorableServerCertificateErrors(&ignoreList);
    Q_ASSERT_SUCCEEDED(hr);

    QSet<QSslError> ignoreErrors = ignoreErrorsList.toSet();
    for (int i = ChainValidationResult_Untrusted; i < ChainValidationResult_OtherErrors + 1; ++i) {
        // Populate the native ignore list - break to add, continue to skip
        switch (i) {
        case ChainValidationResult_Revoked:
        case ChainValidationResult_InvalidSignature:
        case ChainValidationResult_BasicConstraintsError:
        case ChainValidationResult_InvalidCertificateAuthorityPolicy:
        case ChainValidationResult_UnknownCriticalExtension:
        case ChainValidationResult_OtherErrors:
            continue; // The above errors can't be ignored in the handshake
        case ChainValidationResult_Untrusted:
            if (ignoreAllSslErrors || ignoreErrors.contains(QSslError::CertificateUntrusted))
                break;
            continue;
        case ChainValidationResult_Expired:
            if (ignoreAllSslErrors || ignoreErrors.contains(QSslError::CertificateExpired))
                break;
            continue;
        case ChainValidationResult_IncompleteChain:
            if (ignoreAllSslErrors
                    || ignoreErrors.contains(QSslError::InvalidCaCertificate)
                    || ignoreErrors.contains(QSslError::UnableToVerifyFirstCertificate)
                    || ignoreErrors.contains(QSslError::UnableToGetIssuerCertificate)) {
                break;
            }
            continue;
        case ChainValidationResult_WrongUsage:
            if (ignoreAllSslErrors || ignoreErrors.contains(QSslError::InvalidPurpose))
                break;
            continue;
        case ChainValidationResult_InvalidName:
            if (ignoreAllSslErrors
                    || ignoreErrors.contains(QSslError::HostNameMismatch)
                    || ignoreErrors.contains(QSslError::SubjectIssuerMismatch)) {
                break;
            }
            continue;
        case ChainValidationResult_RevocationInformationMissing:
        case ChainValidationResult_RevocationFailure:
        default:
            if (ignoreAllSslErrors)
                break;
            continue;
        }
        hr = ignoreList->Append(static_cast<ChainValidationResult>(i));
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<IAsyncAction> op;
    hr = socket->UpgradeToSslAsync(protectionLevel, hostName.Get(), &op);
    if (FAILED(hr)) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL session: %1").arg(qt_error_string(hr)));
        return;
    }

    hr = QEventDispatcherWinRT::runOnXamlThread([this, op]() {
        HRESULT hr = op->put_Completed(Callback<IAsyncActionCompletedHandler>(
            this, &QSslSocketBackendPrivate::onSslUpgrade).Get());
        return hr;
    });
    Q_ASSERT_SUCCEEDED(hr);
}

HRESULT QSslSocketBackendPrivate::onSslUpgrade(IAsyncAction *action, AsyncStatus)
{
    Q_Q(QSslSocket);

    if (wasDeleted) {
        qCWarning(lcSsl,
                  "SSL upgrade callback received after the delegate was deleted. "
                  "This may be indicative of an internal bug in the WinRT SSL implementation.");
        return S_OK;
    }

    HRESULT hr = action->GetResults();
    QSet<QSslError> errors;
    switch (hr) {
    case SEC_E_INVALID_TOKEN: // Occurs when the server doesn't support the requested protocol
        setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, qt_error_string(hr));
        q->disconnectFromHost();
        return S_OK;
    default:
        if (FAILED(hr))
            qErrnoWarning(hr, "error"); // Unhandled error; let sslErrors take care of it
        break;
    }

    IStreamSocket *socket = reinterpret_cast<IStreamSocket *>(plainSocket->socketDescriptor());
    if (qintptr(socket) == -1) {
        qCWarning(lcSsl,
                  "The underlying TCP socket used by the SSL socket is invalid. "
                  "This may be indicative of an internal bug in the WinRT SSL implementation.");
        return S_OK;
    }

    ComPtr<IStreamSocketInformation> info;
    hr = socket->get_Information(&info);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IStreamSocketInformation2> info2;
    hr = info.As(&info2);
    Q_ASSERT_SUCCEEDED(hr);

    // Cipher
    QSsl::SslProtocol protocol;
    SocketProtectionLevel protectionLevel;
    hr = info->get_ProtectionLevel(&protectionLevel);
    switch (protectionLevel) {
    default:
        protocol = QSsl::UnknownProtocol;
        break;
    case SocketProtectionLevel_Ssl:
        protocol = QSsl::SslV3;
        break;
    case SocketProtectionLevel_Tls10:
        protocol = QSsl::TlsV1_0;
        break;
    case SocketProtectionLevel_Tls11:
        protocol = QSsl::TlsV1_1;
        break;
    case SocketProtectionLevel_Tls12:
        protocol = QSsl::TlsV1_2;
        break;
    }
    configuration.sessionCipher = QSslCipher(QStringLiteral("WINRT"), protocol); // The actual cipher name is not accessible

    // Certificate & chain
    ComPtr<ICertificate> certificate;
    hr = info2->get_ServerCertificate(&certificate);
    Q_ASSERT_SUCCEEDED(hr);

    QList<QSslCertificate> peerCertificateChain;
    if (certificate) {
        ComPtr<IAsyncOperation<CertificateChain *>> op;
        hr = certificate->BuildChainAsync(nullptr, &op);
        Q_ASSERT_SUCCEEDED(hr);
        ComPtr<ICertificateChain> certificateChain;
        hr = QWinRTFunctions::await(op, certificateChain.GetAddressOf());
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<IVectorView<Certificate *>> certificates;
        hr = certificateChain->GetCertificates(true, &certificates);
        Q_ASSERT_SUCCEEDED(hr);
        quint32 certificatesLength;
        hr = certificates->get_Size(&certificatesLength);
        Q_ASSERT_SUCCEEDED(hr);
        for (quint32 i = 0; i < certificatesLength; ++i) {
            ComPtr<ICertificate> chainCertificate;
            hr = certificates->GetAt(i, &chainCertificate);
            Q_ASSERT_SUCCEEDED(hr);
            peerCertificateChain.append(QSslCertificatePrivate::QSslCertificate_from_Certificate(chainCertificate.Get()));
        }
    }

    configuration.peerCertificate = certificate ? QSslCertificatePrivate::QSslCertificate_from_Certificate(certificate.Get())
                                                : QSslCertificate();
    configuration.peerCertificateChain = peerCertificateChain;

    // Errors
    ComPtr<IVectorView<ChainValidationResult>> chainValidationResults;
    hr = info2->get_ServerCertificateErrors(&chainValidationResults);
    Q_ASSERT_SUCCEEDED(hr);
    quint32 size;
    hr = chainValidationResults->get_Size(&size);
    Q_ASSERT_SUCCEEDED(hr);
    for (quint32 i = 0; i < size; ++i) {
        ChainValidationResult result;
        hr = chainValidationResults->GetAt(i, &result);
        Q_ASSERT_SUCCEEDED(hr);
        switch (result) {
        case ChainValidationResult_Success:
            break;
        case ChainValidationResult_Untrusted:
            errors.insert(QSslError::CertificateUntrusted);
            break;
        case ChainValidationResult_Revoked:
            errors.insert(QSslError::CertificateRevoked);
            break;
        case ChainValidationResult_Expired:
            errors.insert(QSslError::CertificateExpired);
            break;
        case ChainValidationResult_IncompleteChain:
            errors.insert(QSslError::UnableToGetIssuerCertificate);
            break;
        case ChainValidationResult_InvalidSignature:
            errors.insert(QSslError::CertificateSignatureFailed);
            break;
        case ChainValidationResult_WrongUsage:
            errors.insert(QSslError::InvalidPurpose);
            break;
        case ChainValidationResult_InvalidName:
            errors.insert(QSslError::HostNameMismatch);
            break;
        case ChainValidationResult_InvalidCertificateAuthorityPolicy:
            errors.insert(QSslError::InvalidCaCertificate);
            break;
        default:
            errors.insert(QSslError::UnspecifiedError);
            break;
        }
    }

    sslErrors = errors.toList();

    // Peer validation
    if (!configuration.peerCertificate.isNull()) {
        const QString peerName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
        if (!isMatchingHostname(configuration.peerCertificate, peerName)) {
            // No matches in common names or alternate names.
            const QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate);
            const int index = sslErrors.indexOf(QSslError::HostNameMismatch);
            if (index >= 0) // Replace the existing error
                sslErrors[index] = error;
            else
                sslErrors.append(error);
            emit q->peerVerifyError(error);
        }

    // Peer validation required, but no certificate is present
    } else if (configuration.peerVerifyMode == QSslSocket::VerifyPeer
               || configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer) {
        QSslError error(QSslError::NoPeerCertificate);
        sslErrors.append(error);
        emit q->peerVerifyError(error);
    }

    // Peer chain validation
    for (const QSslCertificate &certificate : qAsConst(peerCertificateChain)) {
        if (!QSslCertificatePrivate::isBlacklisted(certificate))
            continue;

        QSslError error(QSslError::CertificateBlacklisted, certificate);
        sslErrors.append(error);
        emit q->peerVerifyError(error);
    }

    if (!sslErrors.isEmpty()) {
        emit q->sslErrors(sslErrors);
        setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, sslErrors.constFirst().errorString());

        // Disconnect if there are any non-ignorable errors
        for (const QSslError &error : qAsConst(sslErrors)) {
            if (ignoreErrorsList.contains(error))
                continue;
            q->disconnectFromHost();
            return S_OK;
        }
    }

    if (readBufferMaxSize)
        plainSocket->setReadBufferSize(readBufferMaxSize);

    connectionEncrypted = true;
    emit q->encrypted();

    // The write buffer may already have data written to it, so we need to call transmit.
    // This has to be done in 'q's thread, and not in the current thread (the XAML thread).
    QMetaObject::invokeMethod(q, [this](){ transmit(); });

    if (pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }

    return S_OK;
}

QList<QSslError> QSslSocketBackendPrivate::verify(const QList<QSslCertificate> &certificateChain, const QString &hostName)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(certificateChain)
    Q_UNUSED(hostName)
    QList<QSslError> errors;

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

QT_END_NAMESPACE
