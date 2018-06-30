/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 Governikus GmbH & Co. KG
** Copyright (C) 2016 Richard J. Moore <rich@kde.org>
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

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

//#define QT_DECRYPT_SSL_TRAFFIC

#include "qssl_p.h"
#include "qsslsocket_openssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslkey.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qmutex.h>
#include <QtCore/qlibrary.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, qt_opensslInitMutex, (QMutex::Recursive))

void QSslSocketPrivate::deinitialize()
{
    // This function exists only for compatibility with the pre-11 code,
    // where deinitialize() actually does some cleanup. To be discarded
    // once we retire < 1.1.
}

bool QSslSocketPrivate::ensureLibraryLoaded()
{
    if (!q_resolveOpenSslSymbols())
        return false;

    const QMutexLocker locker(qt_opensslInitMutex);

    if (!s_libraryLoaded) {
        s_libraryLoaded = true;

        // Initialize OpenSSL.
        if (q_OPENSSL_init_ssl(0, nullptr) != 1)
            return false;
        q_SSL_load_error_strings();
        q_OpenSSL_add_all_algorithms();

        QSslSocketBackendPrivate::s_indexForSSLExtraData
            = q_CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_SSL, 0L, nullptr, nullptr,
                                        nullptr, nullptr);

        // Initialize OpenSSL's random seed.
        if (!q_RAND_status()) {
            qWarning("Random number generator not seeded, disabling SSL support");
            return false;
        }
    }
    return true;
}

void QSslSocketPrivate::ensureCiphersAndCertsLoaded()
{
    const QMutexLocker locker(qt_opensslInitMutex);

    if (s_loadedCiphersAndCerts)
        return;
    s_loadedCiphersAndCerts = true;

    resetDefaultCiphers();
    resetDefaultEllipticCurves();

#if QT_CONFIG(library)
    //load symbols needed to receive certificates from system store
#if defined(Q_OS_WIN)
    HINSTANCE hLib = LoadLibraryW(L"Crypt32");
    if (hLib) {
        ptrCertOpenSystemStoreW = reinterpret_cast<PtrCertOpenSystemStoreW>(
            reinterpret_cast<QFunctionPointer>(GetProcAddress(hLib, "CertOpenSystemStoreW")));
        ptrCertFindCertificateInStore = reinterpret_cast<PtrCertFindCertificateInStore>(
            reinterpret_cast<QFunctionPointer>(GetProcAddress(hLib, "CertFindCertificateInStore")));
        ptrCertCloseStore = reinterpret_cast<PtrCertCloseStore>(
            reinterpret_cast<QFunctionPointer>(GetProcAddress(hLib, "CertCloseStore")));
        if (!ptrCertOpenSystemStoreW || !ptrCertFindCertificateInStore || !ptrCertCloseStore)
            qCWarning(lcSsl, "could not resolve symbols in crypt32 library"); // should never happen
    } else {
        qCWarning(lcSsl, "could not load crypt32 library"); // should never happen
    }
#elif defined(Q_OS_QNX)
    s_loadRootCertsOnDemand = true;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    // check whether we can enable on-demand root-cert loading (i.e. check whether the sym links are there)
    QList<QByteArray> dirs = unixRootCertDirectories();
    QStringList symLinkFilter;
    symLinkFilter << QLatin1String("[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f].[0-9]");
    for (int a = 0; a < dirs.count(); ++a) {
        QDirIterator iterator(QLatin1String(dirs.at(a)), symLinkFilter, QDir::Files);
        if (iterator.hasNext()) {
            s_loadRootCertsOnDemand = true;
            break;
        }
    }
#endif
#endif // QT_CONFIG(library)
    // if on-demand loading was not enabled, load the certs now
    if (!s_loadRootCertsOnDemand)
        setDefaultCaCertificates(systemCaCertificates());
#ifdef Q_OS_WIN
    //Enabled for fetching additional root certs from windows update on windows 6+
    //This flag is set false by setDefaultCaCertificates() indicating the app uses
    //its own cert bundle rather than the system one.
    //Same logic that disables the unix on demand cert loading.
    //Unlike unix, we do preload the certificates from the cert store.
    if ((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_6_0)
        s_loadRootCertsOnDemand = true;
#endif
}

long QSslSocketPrivate::sslLibraryVersionNumber()
{
    if (!supportsSsl())
        return 0;

    return q_OpenSSL_version_num();
}

QString QSslSocketPrivate::sslLibraryVersionString()
{
    if (!supportsSsl())
        return QString();

    const char *versionString = q_OpenSSL_version(OPENSSL_VERSION);
    if (!versionString)
        return QString();

    return QString::fromLatin1(versionString);
}

void QSslSocketBackendPrivate::continueHandshake()
{
    Q_Q(QSslSocket);
    // if we have a max read buffer size, reset the plain socket's to match
    if (readBufferMaxSize)
        plainSocket->setReadBufferSize(readBufferMaxSize);

    if (q_SSL_session_reused(ssl))
        configuration.peerSessionShared = true;

#ifdef QT_DECRYPT_SSL_TRAFFIC
    if (q_SSL_get_session(ssl)) {
        size_t master_key_len = q_SSL_SESSION_get_master_key(q_SSL_get_session(ssl), 0, 0);
        size_t client_random_len = q_SSL_get_client_random(ssl, 0, 0);
        QByteArray masterKey(int(master_key_len), 0); // Will not overflow
        QByteArray clientRandom(int(client_random_len), 0); // Will not overflow

        q_SSL_SESSION_get_master_key(q_SSL_get_session(ssl),
                                     reinterpret_cast<unsigned char*>(masterKey.data()),
                                     masterKey.size());
        q_SSL_get_client_random(ssl, reinterpret_cast<unsigned char *>(clientRandom.data()),
                                clientRandom.size());

        QByteArray debugLineClientRandom("CLIENT_RANDOM ");
        debugLineClientRandom.append(clientRandom.toHex().toUpper());
        debugLineClientRandom.append(" ");
        debugLineClientRandom.append(masterKey.toHex().toUpper());
        debugLineClientRandom.append("\n");

        QString sslKeyFile = QDir::tempPath() + QLatin1String("/qt-ssl-keys");
        QFile file(sslKeyFile);
        if (!file.open(QIODevice::Append))
            qCWarning(lcSsl) << "could not open file" << sslKeyFile << "for appending";
        if (!file.write(debugLineClientRandom))
            qCWarning(lcSsl) << "could not write to file" << sslKeyFile;
        file.close();
    } else {
        qCWarning(lcSsl, "could not decrypt SSL traffic");
    }
#endif

    // Cache this SSL session inside the QSslContext
    if (!(configuration.sslOptions & QSsl::SslOptionDisableSessionSharing)) {
        if (!sslContextPointer->cacheSession(ssl)) {
            sslContextPointer.clear(); // we could not cache the session
        } else {
            // Cache the session for permanent usage as well
            if (!(configuration.sslOptions & QSsl::SslOptionDisableSessionPersistence)) {
                if (!sslContextPointer->sessionASN1().isEmpty())
                    configuration.sslSession = sslContextPointer->sessionASN1();
                configuration.sslSessionTicketLifeTimeHint = sslContextPointer->sessionTicketLifeTimeHint();
            }
        }
    }

#if !defined(OPENSSL_NO_NEXTPROTONEG)

    configuration.nextProtocolNegotiationStatus = sslContextPointer->npnContext().status;
    if (sslContextPointer->npnContext().status == QSslConfiguration::NextProtocolNegotiationUnsupported) {
        // we could not agree -> be conservative and use HTTP/1.1
        configuration.nextNegotiatedProtocol = QByteArrayLiteral("http/1.1");
    } else {
        const unsigned char *proto = 0;
        unsigned int proto_len = 0;

        q_SSL_get0_alpn_selected(ssl, &proto, &proto_len);
        if (proto_len && mode == QSslSocket::SslClientMode) {
            // Client does not have a callback that sets it ...
            configuration.nextProtocolNegotiationStatus = QSslConfiguration::NextProtocolNegotiationNegotiated;
        }

        if (!proto_len) { // Test if NPN was more lucky ...
            q_SSL_get0_next_proto_negotiated(ssl, &proto, &proto_len);
        }

        if (proto_len)
            configuration.nextNegotiatedProtocol = QByteArray(reinterpret_cast<const char *>(proto), proto_len);
        else
            configuration.nextNegotiatedProtocol.clear();
    }
#endif // !defined(OPENSSL_NO_NEXTPROTONEG)

    if (mode == QSslSocket::SslClientMode) {
        EVP_PKEY *key;
        if (q_SSL_get_server_tmp_key(ssl, &key))
            configuration.ephemeralServerKey = QSslKey(key, QSsl::PublicKey);
    }

    connectionEncrypted = true;
    emit q->encrypted();
    if (autoStartHandshake && pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }
}

QT_END_NAMESPACE
