/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 Governikus GmbH & Co. KG
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
#include <QtCore/qthread.h>
#include <QtCore/qfile.h>
#include <QtCore/qmutex.h>
#include <QtCore/qlibrary.h>

QT_BEGIN_NAMESPACE

/* \internal

    From OpenSSL's thread(3) manual page:

    OpenSSL can safely be used in multi-threaded applications provided that at
    least two callback functions are set.

    locking_function(int mode, int n, const char *file, int line) is needed to
    perform locking on shared data structures.  (Note that OpenSSL uses a
    number of global data structures that will be implicitly shared
    whenever multiple threads use OpenSSL.)  Multi-threaded
    applications will crash at random if it is not set.  ...
    ...
    id_function(void) is a function that returns a thread ID. It is not
    needed on Windows nor on platforms where getpid() returns a different
    ID for each thread (most notably Linux)
*/

class QOpenSslLocks
{
public:
    QOpenSslLocks()
        : initLocker(QMutex::Recursive),
          locksLocker(QMutex::Recursive)
    {
        QMutexLocker locker(&locksLocker);
        int numLocks = q_CRYPTO_num_locks();
        locks = new QMutex *[numLocks];
        memset(locks, 0, numLocks * sizeof(QMutex *));
    }
    ~QOpenSslLocks()
    {
        QMutexLocker locker(&locksLocker);
        for (int i = 0; i < q_CRYPTO_num_locks(); ++i)
            delete locks[i];
        delete [] locks;

        QSslSocketPrivate::deinitialize();
    }
    QMutex *lock(int num)
    {
        QMutexLocker locker(&locksLocker);
        QMutex *tmp = locks[num];
        if (!tmp)
            tmp = locks[num] = new QMutex(QMutex::Recursive);
        return tmp;
    }

    QMutex *globalLock()
    {
        return &locksLocker;
    }

    QMutex *initLock()
    {
        return &initLocker;
    }

private:
    QMutex initLocker;
    QMutex locksLocker;
    QMutex **locks;
};

Q_GLOBAL_STATIC(QOpenSslLocks, openssl_locks)

extern "C" {
static void locking_function(int mode, int lockNumber, const char *, int)
{
    QMutex *mutex = openssl_locks()->lock(lockNumber);

    // Lock or unlock it
    if (mode & CRYPTO_LOCK)
        mutex->lock();
    else
        mutex->unlock();
}
static unsigned long id_function()
{
    return (quintptr)QThread::currentThreadId();
}

} // extern "C"

static void q_OpenSSL_add_all_algorithms_safe()
{
#ifdef Q_OS_WIN
    // Prior to version 1.0.1m an attempt to call OpenSSL_add_all_algorithms on
    // Windows could result in 'exit' call from OPENSSL_config (QTBUG-43843).
    // We can predict this and avoid OPENSSL_add_all_algorithms call.
    // From OpenSSL docs:
    // "An application does not need to add algorithms to use them explicitly,
    // for example by EVP_sha1(). It just needs to add them if it (or any of
    // the functions it calls) needs to lookup algorithms.
    // The cipher and digest lookup functions are used in many parts of the
    // library. If the table is not initialized several functions will
    // misbehave and complain they cannot find algorithms. This includes the
    // PEM, PKCS#12, SSL and S/MIME libraries. This is a common query in
    // the OpenSSL mailing lists."
    //
    // Anyway, as a result, we chose not to call this function if it would exit.

    if (q_SSLeay() < 0x100010DFL)
    {
        // Now, before we try to call it, check if an attempt to open config file
        // will result in exit:
        if (char *confFileName = q_CONF_get1_default_config_file()) {
            BIO *confFile = q_BIO_new_file(confFileName, "r");
            const auto lastError = q_ERR_peek_last_error();
            q_CRYPTO_free(confFileName);
            if (confFile) {
                q_BIO_free(confFile);
            } else {
                q_ERR_clear_error();
                if (ERR_GET_REASON(lastError) == ERR_R_SYS_LIB) {
                    qCWarning(lcSsl, "failed to open openssl.conf file");
                    return;
                }
            }
        }
    }
#endif // Q_OS_WIN

    q_OpenSSL_add_all_algorithms();
}


void QSslSocketPrivate::deinitialize()
{
    q_CRYPTO_set_id_callback(0);
    q_CRYPTO_set_locking_callback(0);
    q_ERR_free_strings();
}


bool QSslSocketPrivate::ensureLibraryLoaded()
{
    if (!q_resolveOpenSslSymbols())
        return false;

    // Check if the library itself needs to be initialized.
    QMutexLocker locker(openssl_locks()->initLock());

    if (!s_libraryLoaded) {
        s_libraryLoaded = true;

        // Initialize OpenSSL.
        q_CRYPTO_set_id_callback(id_function);
        q_CRYPTO_set_locking_callback(locking_function);
        if (q_SSL_library_init() != 1)
            return false;
        q_SSL_load_error_strings();
        q_OpenSSL_add_all_algorithms_safe();

#if OPENSSL_VERSION_NUMBER >= 0x10001000L
        if (q_SSLeay() >= 0x10001000L)
            QSslSocketBackendPrivate::s_indexForSSLExtraData = q_SSL_get_ex_new_index(0L, NULL, NULL, NULL, NULL);
#endif

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
    QMutexLocker locker(openssl_locks()->initLock());
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
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
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
    s_loadRootCertsOnDemand = true;
#endif
}

long QSslSocketPrivate::sslLibraryVersionNumber()
{
    if (!supportsSsl())
        return 0;

    return q_SSLeay();
}

QString QSslSocketPrivate::sslLibraryVersionString()
{
    if (!supportsSsl())
        return QString();

    const char *versionString = q_SSLeay_version(SSLEAY_VERSION);
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

    if (q_SSL_ctrl((ssl), SSL_CTRL_GET_SESSION_REUSED, 0, NULL))
        configuration.peerSessionShared = true;

#ifdef QT_DECRYPT_SSL_TRAFFIC
    if (ssl->session && ssl->s3) {
        const char *mk = reinterpret_cast<const char *>(ssl->session->master_key);
        QByteArray masterKey(mk, ssl->session->master_key_length);
        const char *random = reinterpret_cast<const char *>(ssl->s3->client_random);
        QByteArray clientRandom(random, SSL3_RANDOM_SIZE);

        // different format, needed for e.g. older Wireshark versions:
//        const char *sid = reinterpret_cast<const char *>(ssl->session->session_id);
//        QByteArray sessionID(sid, ssl->session->session_id_length);
//        QByteArray debugLineRSA("RSA Session-ID:");
//        debugLineRSA.append(sessionID.toHex().toUpper());
//        debugLineRSA.append(" Master-Key:");
//        debugLineRSA.append(masterKey.toHex().toUpper());
//        debugLineRSA.append("\n");

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

#if OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_NEXTPROTONEG)

    configuration.nextProtocolNegotiationStatus = sslContextPointer->npnContext().status;
    if (sslContextPointer->npnContext().status == QSslConfiguration::NextProtocolNegotiationUnsupported) {
        // we could not agree -> be conservative and use HTTP/1.1
        configuration.nextNegotiatedProtocol = QByteArrayLiteral("http/1.1");
    } else {
        const unsigned char *proto = 0;
        unsigned int proto_len = 0;
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
        if (q_SSLeay() >= 0x10002000L) {
            q_SSL_get0_alpn_selected(ssl, &proto, &proto_len);
            if (proto_len && mode == QSslSocket::SslClientMode) {
                // Client does not have a callback that sets it ...
                configuration.nextProtocolNegotiationStatus = QSslConfiguration::NextProtocolNegotiationNegotiated;
            }
        }

        if (!proto_len) { // Test if NPN was more lucky ...
#else
        {
#endif
            q_SSL_get0_next_proto_negotiated(ssl, &proto, &proto_len);
        }

        if (proto_len)
            configuration.nextNegotiatedProtocol = QByteArray(reinterpret_cast<const char *>(proto), proto_len);
        else
            configuration.nextNegotiatedProtocol.clear();
    }
#endif // OPENSSL_VERSION_NUMBER >= 0x1000100fL ...

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    if (q_SSLeay() >= 0x10002000L && mode == QSslSocket::SslClientMode) {
        EVP_PKEY *key;
        if (q_SSL_get_server_tmp_key(ssl, &key))
            configuration.ephemeralServerKey = QSslKey(key, QSsl::PublicKey);
    }
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L ...

    connectionEncrypted = true;
    emit q->encrypted();
    if (autoStartHandshake && pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }
}

QT_END_NAMESPACE
