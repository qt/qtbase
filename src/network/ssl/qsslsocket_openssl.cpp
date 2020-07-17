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

//#define QSSLSOCKET_DEBUG

#include "qssl_p.h"
#include "qsslsocket_openssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"
#include "qsslkey_p.h"
#include "qsslellipticcurve.h"
#include "qsslpresharedkeyauthenticator.h"
#include "qsslpresharedkeyauthenticator_p.h"
#include "qocspresponse_p.h"
#include "qsslkey.h"

#ifdef Q_OS_WIN
#include "qwindowscarootfetcher_p.h"
#endif

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qlibrary.h>
#include <QtCore/qoperatingsystemversion.h>

#if QT_CONFIG(ocsp)
#include "qocsp_p.h"
#endif

#include <algorithm>
#include <memory>

#include <string.h>

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN

namespace {

QSslCertificate findCertificateToFetch(const QList<QSslError> &tlsErrors, bool checkAIA)
{
    QSslCertificate certToFetch;

    for (const auto &tlsError : tlsErrors) {
        switch (tlsError.error()) {
        case QSslError::UnableToGetLocalIssuerCertificate: // site presented intermediate cert, but root is unknown
        case QSslError::SelfSignedCertificateInChain: // site presented a complete chain, but root is unknown
            certToFetch = tlsError.certificate();
            break;
        case QSslError::SelfSignedCertificate:
        case QSslError::CertificateBlacklisted:
            //With these errors, we know it will be untrusted so save time by not asking windows
            return QSslCertificate{};
        default:
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << tlsError.errorString();
#endif
            //TODO - this part is strange.
            break;
        }
    }

    if (checkAIA) {
        const auto extensions = certToFetch.extensions();
        for (const auto &ext : extensions) {
            if (ext.oid() == QStringLiteral("1.3.6.1.5.5.7.1.1")) // See RFC 4325
                return certToFetch;
        }
        //The only reason we check this extensions is because an application set trusted
        //CA certificates explicitly, thus technically disabling CA fetch. So, if it's
        //the case and an intermediate certificate is missing, and no extensions is
        //present on the leaf certificate - we fail the handshake immediately.
        return QSslCertificate{};
    }

    return certToFetch;
}

} // Unnamed namespace

#endif // Q_OS_WIN

Q_GLOBAL_STATIC(QRecursiveMutex, qt_opensslInitMutex)

bool QSslSocketPrivate::s_libraryLoaded = false;
bool QSslSocketPrivate::s_loadedCiphersAndCerts = false;
bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;
int QSslSocketBackendPrivate::s_indexForSSLExtraData = -1;

QString QSslSocketBackendPrivate::getErrorsFromOpenSsl()
{
    QString errorString;
    char buf[256] = {}; // OpenSSL docs claim both 120 and 256; use the larger.
    unsigned long errNum;
    while ((errNum = q_ERR_get_error())) {
        if (!errorString.isEmpty())
            errorString.append(QLatin1String(", "));
        q_ERR_error_string_n(errNum, buf, sizeof buf);
        errorString.append(QString::fromLatin1(buf)); // error is ascii according to man ERR_error_string
    }
    return errorString;
}

void QSslSocketBackendPrivate::logAndClearErrorQueue()
{
    const auto errors = getErrorsFromOpenSsl();
    if (errors.size())
        qCWarning(lcSsl) << "Discarding errors:" << errors;
}

extern "C" {

#ifndef OPENSSL_NO_PSK
static unsigned int q_ssl_psk_client_callback(SSL *ssl,
                                              const char *hint,
                                              char *identity, unsigned int max_identity_len,
                                              unsigned char *psk, unsigned int max_psk_len)
{
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    return d->tlsPskClientCallback(hint, identity, max_identity_len, psk, max_psk_len);
}

static unsigned int q_ssl_psk_server_callback(SSL *ssl,
                                              const char *identity,
                                              unsigned char *psk, unsigned int max_psk_len)
{
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    return d->tlsPskServerCallback(identity, psk, max_psk_len);
}

#ifdef TLS1_3_VERSION
static unsigned int q_ssl_psk_restore_client(SSL *ssl,
                                             const char *hint,
                                             char *identity, unsigned int max_identity_len,
                                             unsigned char *psk, unsigned int max_psk_len)
{
    Q_UNUSED(hint);
    Q_UNUSED(identity);
    Q_UNUSED(max_identity_len);
    Q_UNUSED(psk);
    Q_UNUSED(max_psk_len);

#ifdef QT_DEBUG
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    Q_ASSERT(d->mode == QSslSocket::SslClientMode);
#endif
    q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_client_callback);

    return 0;
}

static int q_ssl_psk_use_session_callback(SSL *ssl, const EVP_MD *md, const unsigned char **id,
                                          size_t *idlen, SSL_SESSION **sess)
{
    Q_UNUSED(ssl);
    Q_UNUSED(md);
    Q_UNUSED(id);
    Q_UNUSED(idlen);
    Q_UNUSED(sess);

#ifdef QT_DEBUG
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    Q_ASSERT(d->mode == QSslSocket::SslClientMode);
#endif

    // Temporarily rebind the psk because it will be called next. The function will restore it.
    q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_restore_client);

    return 1; // need to return 1 or else "the connection setup fails."
}

int q_ssl_sess_set_new_cb(SSL *ssl, SSL_SESSION *session)
{
    if (!ssl) {
        qCWarning(lcSsl, "Invalid SSL (nullptr)");
        return 0;
    }
    if (!session) {
        qCWarning(lcSsl, "Invalid SSL_SESSION (nullptr)");
        return 0;
    }

    auto socketPrivate = static_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl,
                                                                 QSslSocketBackendPrivate::s_indexForSSLExtraData));
    return socketPrivate->handleNewSessionTicket(ssl);
}
#endif // TLS1_3_VERSION

#endif // !OPENSSL_NO_PSK

#if QT_CONFIG(ocsp)

int qt_OCSP_status_server_callback(SSL *ssl, void *ocspRequest)
{
    Q_UNUSED(ocspRequest)
    if (!ssl)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    auto d = static_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    if (!d)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    Q_ASSERT(d->mode == QSslSocket::SslServerMode);
    const QByteArray &response = d->ocspResponseDer;
    Q_ASSERT(response.size());

    unsigned char *derCopy = static_cast<unsigned char *>(q_OPENSSL_malloc(size_t(response.size())));
    if (!derCopy)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    std::copy(response.data(), response.data() + response.size(), derCopy);
    // We don't check the return value: internally OpenSSL simply assignes the
    // pointer (it assumes it now owns this memory btw!) and the length.
    q_SSL_set_tlsext_status_ocsp_resp(ssl, derCopy, response.size());

    return SSL_TLSEXT_ERR_OK;
}

#endif // ocsp

} // extern "C"

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : ssl(nullptr),
      readBio(nullptr),
      writeBio(nullptr),
      session(nullptr)
{
    // Calls SSL_library_init().
    ensureInitialized();
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
    destroySslContext();
}

QSslCipher QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(const SSL_CIPHER *cipher)
{
    QSslCipher ciph;

    char buf [256];
    QString descriptionOneLine = QString::fromLatin1(q_SSL_CIPHER_description(cipher, buf, sizeof(buf)));

    const auto descriptionList = descriptionOneLine.splitRef(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        // ### crude code.
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0).toString();

        QString protoString = descriptionList.at(1).toString();
        ciph.d->protocolString = protoString;
        ciph.d->protocol = QSsl::UnknownProtocol;
        if (protoString == QLatin1String("SSLv3"))
            ciph.d->protocol = QSsl::SslV3;
        else if (protoString == QLatin1String("SSLv2"))
            ciph.d->protocol = QSsl::SslV2;
        else if (protoString == QLatin1String("TLSv1"))
            ciph.d->protocol = QSsl::TlsV1_0;
        else if (protoString == QLatin1String("TLSv1.1"))
            ciph.d->protocol = QSsl::TlsV1_1;
        else if (protoString == QLatin1String("TLSv1.2"))
            ciph.d->protocol = QSsl::TlsV1_2;
        else if (protoString == QLatin1String("TLSv1.3"))
            ciph.d->protocol = QSsl::TlsV1_3;

        if (descriptionList.at(2).startsWith(QLatin1String("Kx=")))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3).toString();
        if (descriptionList.at(3).startsWith(QLatin1String("Au=")))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3).toString();
        if (descriptionList.at(4).startsWith(QLatin1String("Enc=")))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4).toString();
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == QLatin1String("export"));

        ciph.d->bits = q_SSL_CIPHER_get_bits(cipher, &ciph.d->supportedBits);
    }
    return ciph;
}

QSslErrorEntry QSslErrorEntry::fromStoreContext(X509_STORE_CTX *ctx)
{
    return {
        q_X509_STORE_CTX_get_error(ctx),
        q_X509_STORE_CTX_get_error_depth(ctx)
    };
}

#if QT_CONFIG(ocsp)

QSslError qt_OCSP_response_status_to_QSslError(long code)
{
    switch (code) {
    case OCSP_RESPONSE_STATUS_MALFORMEDREQUEST:
        return QSslError::OcspMalformedRequest;
    case OCSP_RESPONSE_STATUS_INTERNALERROR:
        return QSslError::OcspInternalError;
    case OCSP_RESPONSE_STATUS_TRYLATER:
        return QSslError::OcspTryLater;
    case OCSP_RESPONSE_STATUS_SIGREQUIRED:
        return QSslError::OcspSigRequred;
    case OCSP_RESPONSE_STATUS_UNAUTHORIZED:
        return QSslError::OcspUnauthorized;
    case OCSP_RESPONSE_STATUS_SUCCESSFUL:
    default:
        return {};
    }
    Q_UNREACHABLE();
}

QOcspRevocationReason qt_OCSP_revocation_reason(int reason)
{
    switch (reason) {
    case OCSP_REVOKED_STATUS_NOSTATUS:
        return QOcspRevocationReason::None;
    case OCSP_REVOKED_STATUS_UNSPECIFIED:
        return QOcspRevocationReason::Unspecified;
    case OCSP_REVOKED_STATUS_KEYCOMPROMISE:
        return QOcspRevocationReason::KeyCompromise;
    case OCSP_REVOKED_STATUS_CACOMPROMISE:
        return QOcspRevocationReason::CACompromise;
    case OCSP_REVOKED_STATUS_AFFILIATIONCHANGED:
        return QOcspRevocationReason::AffiliationChanged;
    case OCSP_REVOKED_STATUS_SUPERSEDED:
        return QOcspRevocationReason::Superseded;
    case OCSP_REVOKED_STATUS_CESSATIONOFOPERATION:
        return QOcspRevocationReason::CessationOfOperation;
    case OCSP_REVOKED_STATUS_CERTIFICATEHOLD:
        return QOcspRevocationReason::CertificateHold;
    case OCSP_REVOKED_STATUS_REMOVEFROMCRL:
        return QOcspRevocationReason::RemoveFromCRL;
    default:
        return QOcspRevocationReason::None;
    }

    Q_UNREACHABLE();
}

bool qt_OCSP_certificate_match(OCSP_SINGLERESP *singleResponse, X509 *peerCert, X509 *issuer)
{
    // OCSP_basic_verify does verify that the responder is legit, the response is
    // correctly signed, CertID is correct. But it does not know which certificate
    // we were presented with by our peer, so it does not check if it's a response
    // for our peer's certificate.
    Q_ASSERT(singleResponse && peerCert && issuer);

    const OCSP_CERTID *certId = q_OCSP_SINGLERESP_get0_id(singleResponse); // Does not increment refcount.
    if (!certId) {
        qCWarning(lcSsl, "A SingleResponse without CertID");
        return false;
    }

    ASN1_OBJECT *md = nullptr;
    ASN1_INTEGER *reportedSerialNumber = nullptr;
    const int result =  q_OCSP_id_get0_info(nullptr, &md, nullptr, &reportedSerialNumber, const_cast<OCSP_CERTID *>(certId));
    if (result != 1 || !md || !reportedSerialNumber) {
        qCWarning(lcSsl, "Failed to extract a hash and serial number from CertID structure");
        return false;
    }

    if (!q_X509_get_serialNumber(peerCert)) {
        // Is this possible at all? But we have to check this,
        // ASN1_INTEGER_cmp (called from OCSP_id_cmp) dereferences
        // without any checks at all.
        qCWarning(lcSsl, "No serial number in peer's ceritificate");
        return false;
    }

    const int nid = q_OBJ_obj2nid(md);
    if (nid == NID_undef) {
        qCWarning(lcSsl, "Unknown hash algorithm in CertID");
        return false;
    }

    const EVP_MD *digest = q_EVP_get_digestbynid(nid); // Does not increment refcount.
    if (!digest) {
        qCWarning(lcSsl) << "No digest for nid" << nid;
        return false;
    }

    OCSP_CERTID *recreatedId = q_OCSP_cert_to_id(digest, peerCert, issuer);
    if (!recreatedId) {
        qCWarning(lcSsl, "Failed to re-create CertID");
        return false;
    }
    const QSharedPointer<OCSP_CERTID> guard(recreatedId, q_OCSP_CERTID_free);

    if (q_OCSP_id_cmp(const_cast<OCSP_CERTID *>(certId), recreatedId)) {
        qDebug(lcSsl, "Certificate ID mismatch");
        return false;
    }
    // Bingo!
    return true;
}

#endif // ocsp

int q_X509Callback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok) {
        // Store the error and at which depth the error was detected.

        using ErrorListPtr = QVector<QSslErrorEntry>*;
        ErrorListPtr errors = nullptr;

        // Error list is attached to either 'SSL' or 'X509_STORE'.
        if (X509_STORE *store = q_X509_STORE_CTX_get0_store(ctx)) // We try store first:
            errors = ErrorListPtr(q_X509_STORE_get_ex_data(store, 0));

        if (!errors) {
            // Not found on store? Try SSL and its external data then. According to the OpenSSL's
            // documentation:
            //
            // "Whenever a X509_STORE_CTX object is created for the verification of the peers certificate
            // during a handshake, a pointer to the SSL object is stored into the X509_STORE_CTX object
            // to identify the connection affected. To retrieve this pointer the X509_STORE_CTX_get_ex_data()
            // function can be used with the correct index."
            if (SSL *ssl = static_cast<SSL *>(q_X509_STORE_CTX_get_ex_data(ctx, q_SSL_get_ex_data_X509_STORE_CTX_idx())))
                errors = ErrorListPtr(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData + 1));
        }

        if (!errors) {
            qCWarning(lcSsl, "Neither X509_STORE, nor SSL contains error list, handshake failure");
            return 0;
        }

        errors->append(QSslErrorEntry::fromStoreContext(ctx));
    }
    // Always return OK to allow verification to continue. We handle the
    // errors gracefully after collecting all errors, after verification has
    // completed.
    return 1;
}

static void q_loadCiphersForConnection(SSL *connection, QList<QSslCipher> &ciphers,
                                       QList<QSslCipher> &defaultCiphers)
{
    Q_ASSERT(connection);

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(connection);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            QSslCipher ciph = QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(cipher);
            if (!ciph.isNull()) {
                // Unconditionally exclude ADH and AECDH ciphers since they offer no MITM protection
                if (!ciph.name().toLower().startsWith(QLatin1String("adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("exp-adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("aecdh"))) {
                    ciphers << ciph;

                    if (ciph.usedBits() >= 128)
                        defaultCiphers << ciph;
                }
            }
        }
    }
}

// Defined in qsslsocket.cpp
void q_setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers);

long QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SslProtocol protocol, QSsl::SslOptions sslOptions)
{
    long options;
    if (protocol == QSsl::TlsV1SslV3)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
    else if (protocol == QSsl::SecureProtocols)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
    else if (protocol == QSsl::TlsV1_0OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
    else if (protocol == QSsl::TlsV1_1OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1;
    else if (protocol == QSsl::TlsV1_2OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1|SSL_OP_NO_TLSv1_1;
    else if (protocol == QSsl::TlsV1_3OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1|SSL_OP_NO_TLSv1_1|SSL_OP_NO_TLSv1_2;
    else
        options = SSL_OP_ALL;

    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableEmptyFragments)
        options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    else
        options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableLegacyRenegotiation)
        options &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
    else
        options |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
#endif

#ifdef SSL_OP_NO_TICKET
    if (sslOptions & QSsl::SslOptionDisableSessionTickets)
        options |= SSL_OP_NO_TICKET;
#endif
#ifdef SSL_OP_NO_COMPRESSION
    if (sslOptions & QSsl::SslOptionDisableCompression)
        options |= SSL_OP_NO_COMPRESSION;
#endif

    if (!(sslOptions & QSsl::SslOptionDisableServerCipherPreference))
        options |= SSL_OP_CIPHER_SERVER_PREFERENCE;

    return options;
}

bool QSslSocketBackendPrivate::initSslContext()
{
    Q_Q(QSslSocket);

    // If no external context was set (e.g. by QHttpNetworkConnection) we will
    // create a default context
    if (!sslContextPointer) {
        // create a deep copy of our configuration
        QSslConfigurationPrivate *configurationCopy = new QSslConfigurationPrivate(configuration);
        configurationCopy->ref.storeRelaxed(0);              // the QSslConfiguration constructor refs up
        sslContextPointer = QSslContext::sharedFromConfiguration(mode, configurationCopy, allowRootCertOnDemandLoading);
    }

    if (sslContextPointer->error() != QSslError::NoError) {
        setErrorAndEmit(QAbstractSocket::SslInvalidUserDataError, sslContextPointer->errorString());
        sslContextPointer.clear(); // deletes the QSslContext
        return false;
    }

    // Create and initialize SSL session
    if (!(ssl = sslContextPointer->createSsl())) {
        // ### Bad error code
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL session, %1").arg(getErrorsFromOpenSsl()));
        return false;
    }

    if (configuration.protocol != QSsl::SslV2 &&
        configuration.protocol != QSsl::SslV3 &&
        configuration.protocol != QSsl::UnknownProtocol &&
        mode == QSslSocket::SslClientMode) {
        // Set server hostname on TLS extension. RFC4366 section 3.1 requires it in ACE format.
        QString tlsHostName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
        if (tlsHostName.isEmpty())
            tlsHostName = hostName;
        QByteArray ace = QUrl::toAce(tlsHostName);
        // only send the SNI header if the URL is valid and not an IP
        if (!ace.isEmpty()
            && !QHostAddress().setAddress(tlsHostName)
            && !(configuration.sslOptions & QSsl::SslOptionDisableServerNameIndication)) {
            // We don't send the trailing dot from the host header if present see
            // https://tools.ietf.org/html/rfc6066#section-3
            if (ace.endsWith('.'))
                ace.chop(1);
            if (!q_SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, ace.data()))
                qCWarning(lcSsl, "could not set SSL_CTRL_SET_TLSEXT_HOSTNAME, Server Name Indication disabled");
        }
    }

    // Clear the session.
    errorList.clear();

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL session: %1").arg(getErrorsFromOpenSsl()));
        return false;
    }

    // Assign the bios.
    q_SSL_set_bio(ssl, readBio, writeBio);

    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_connect_state(ssl);
    else
        q_SSL_set_accept_state(ssl);

    q_SSL_set_ex_data(ssl, s_indexForSSLExtraData, this);

#ifndef OPENSSL_NO_PSK
    // Set the client callback for PSK
    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_client_callback);
    else if (mode == QSslSocket::SslServerMode)
        q_SSL_set_psk_server_callback(ssl, &q_ssl_psk_server_callback);

#if OPENSSL_VERSION_NUMBER >= 0x10101006L
    // Set the client callback for TLSv1.3 PSK
    if (mode == QSslSocket::SslClientMode
        && QSslSocket::sslLibraryBuildVersionNumber() >= 0x10101006L) {
        q_SSL_set_psk_use_session_callback(ssl, &q_ssl_psk_use_session_callback);
    }
#endif // openssl version >= 0x10101006L

#endif // OPENSSL_NO_PSK


#if QT_CONFIG(ocsp)
    if (configuration.ocspStaplingEnabled) {
        if (mode == QSslSocket::SslServerMode) {
            setErrorAndEmit(QAbstractSocket::SslInvalidUserDataError,
                            QSslSocket::tr("Server-side QSslSocket does not support OCSP stapling"));
            return false;
        }
        if (q_SSL_set_tlsext_status_type(ssl, TLSEXT_STATUSTYPE_ocsp) != 1) {
            setErrorAndEmit(QAbstractSocket::SslInternalError,
                            QSslSocket::tr("Failed to enable OCSP stapling"));
            return false;
        }
    }

    ocspResponseDer.clear();
    auto responsePos = configuration.backendConfig.find("Qt-OCSP-response");
    if (responsePos != configuration.backendConfig.end()) {
        // This is our private, undocumented 'API' we use for the auto-testing of
        // OCSP-stapling. It must be a der-encoded OCSP response, presumably set
        // by tst_QOcsp.
        const QVariant data(responsePos.value());
        if (data.canConvert<QByteArray>())
            ocspResponseDer = data.toByteArray();
    }

    if (ocspResponseDer.size()) {
        if (mode != QSslSocket::SslServerMode) {
            setErrorAndEmit(QAbstractSocket::SslInvalidUserDataError,
                            QSslSocket::tr("Client-side sockets do not send OCSP responses"));
            return false;
        }
    }
#endif // ocsp

    return true;
}

void QSslSocketBackendPrivate::destroySslContext()
{
    if (ssl) {
        if (!q_SSL_in_init(ssl) && !systemOrSslErrorDetected) {
            // We do not send a shutdown alert here. Just mark the session as
            // resumable for qhttpnetworkconnection's "optimization", otherwise
            // OpenSSL won't start a session resumption.
            if (q_SSL_shutdown(ssl) != 1) {
                // Some error may be queued, clear it.
                const auto errors = getErrorsFromOpenSsl();
                Q_UNUSED(errors);
            }
        }
        q_SSL_free(ssl);
        ssl = nullptr;
    }
    sslContextPointer.clear();
}

/*!
    \internal

    Does the minimum amount of initialization to determine whether SSL
    is supported or not.
*/

bool QSslSocketPrivate::supportsSsl()
{
    return ensureLibraryLoaded();
}


/*!
    \internal

    Returns the version number of the SSL library in use. Note that
    this is the version of the library in use at run-time, not compile
    time.
*/
long QSslSocketPrivate::sslLibraryVersionNumber()
{
    if (!supportsSsl())
        return 0;

    return q_OpenSSL_version_num();
}

/*!
    \internal

    Returns the version string of the SSL library in use. Note that
    this is the version of the library in use at run-time, not compile
    time. If no SSL support is available then this will return an empty value.
*/
QString QSslSocketPrivate::sslLibraryVersionString()
{
    if (!supportsSsl())
        return QString();

    const char *versionString = q_OpenSSL_version(OPENSSL_VERSION);
    if (!versionString)
        return QString();

    return QString::fromLatin1(versionString);
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

    ensureCiphersAndCertsLoaded();
}

/*!
    \internal

    Returns the version number of the SSL library in use at compile
    time.
*/
long QSslSocketPrivate::sslLibraryBuildVersionNumber()
{
    return OPENSSL_VERSION_NUMBER;
}

/*!
    \internal

    Returns the version string of the SSL library in use at compile
    time.
*/
QString QSslSocketPrivate::sslLibraryBuildVersionString()
{
    // Using QStringLiteral to store the version string as unicode and
    // avoid false positives from Google searching the playstore for old
    // SSL versions. See QTBUG-46265
    return QStringLiteral(OPENSSL_VERSION_TEXT);
}

/*!
    \internal

    Declared static in QSslSocketPrivate, backend-dependent loading of
    application-wide global ciphers.
*/
void QSslSocketPrivate::resetDefaultCiphers()
{
    SSL_CTX *myCtx = q_SSL_CTX_new(q_TLS_client_method());
    // Note, we assert, not just silently return/bail out early:
    // this should never happen and problems with OpenSSL's initialization
    // must be caught before this (see supportsSsl()).
    Q_ASSERT(myCtx);
    SSL *mySsl = q_SSL_new(myCtx);
    Q_ASSERT(mySsl);

    QList<QSslCipher> ciphers;
    QList<QSslCipher> defaultCiphers;

    q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);

    q_SSL_CTX_free(myCtx);
    q_SSL_free(mySsl);

    setDefaultSupportedCiphers(ciphers);
    setDefaultCiphers(defaultCiphers);

#if QT_CONFIG(dtls)
    ciphers.clear();
    defaultCiphers.clear();
    myCtx = q_SSL_CTX_new(q_DTLS_client_method());
    if (myCtx) {
        mySsl = q_SSL_new(myCtx);
        if (mySsl) {
            q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);
            q_setDefaultDtlsCiphers(defaultCiphers);
            q_SSL_free(mySsl);
        }
        q_SSL_CTX_free(myCtx);
    }
#endif // dtls
}

void QSslSocketPrivate::resetDefaultEllipticCurves()
{
    QVector<QSslEllipticCurve> curves;

#ifndef OPENSSL_NO_EC
    const size_t curveCount = q_EC_get_builtin_curves(nullptr, 0);

    QVarLengthArray<EC_builtin_curve> builtinCurves(static_cast<int>(curveCount));

    if (q_EC_get_builtin_curves(builtinCurves.data(), curveCount) == curveCount) {
        curves.reserve(int(curveCount));
        for (size_t i = 0; i < curveCount; ++i) {
            QSslEllipticCurve curve;
            curve.id = builtinCurves[int(i)].nid;
            curves.append(curve);
        }
    }
#endif // OPENSSL_NO_EC

    // set the list of supported ECs, but not the list
    // of *default* ECs. OpenSSL doesn't like forcing an EC for the wrong
    // ciphersuite, so don't try it -- leave the empty list to mean
    // "the implementation will choose the most suitable one".
    setDefaultSupportedEllipticCurves(curves);
}

#ifndef Q_OS_DARWIN // Apple implementation in qsslsocket_mac_shared.cpp
QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    ensureInitialized();
#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    QList<QSslCertificate> systemCerts;
#if defined(Q_OS_WIN)
    HCERTSTORE hSystemStore;
    hSystemStore = CertOpenSystemStoreW(0, L"ROOT");
    if (hSystemStore) {
        PCCERT_CONTEXT pc = nullptr;
        while (1) {
            pc = CertFindCertificateInStore(hSystemStore, X509_ASN_ENCODING, 0, CERT_FIND_ANY, nullptr, pc);
            if (!pc)
                break;
            QByteArray der(reinterpret_cast<const char *>(pc->pbCertEncoded),
                            static_cast<int>(pc->cbCertEncoded));
            QSslCertificate cert(der, QSsl::Der);
            systemCerts.append(cert);
        }
        CertCloseStore(hSystemStore, 0);
    }
#elif defined(Q_OS_UNIX)
    QSet<QString> certFiles;
    QDir currentDir;
    QStringList nameFilters;
    QList<QByteArray> directories;
    QSsl::EncodingFormat platformEncodingFormat;
# ifndef Q_OS_ANDROID
    directories = unixRootCertDirectories();
    nameFilters << QLatin1String("*.pem") << QLatin1String("*.crt");
    platformEncodingFormat = QSsl::Pem;
# else
    // Q_OS_ANDROID
    QByteArray ministroPath = qgetenv("MINISTRO_SSL_CERTS_PATH"); // Set by Ministro
    directories << ministroPath;
    nameFilters << QLatin1String("*.der");
    platformEncodingFormat = QSsl::Der;
#  ifndef Q_OS_ANDROID_EMBEDDED
    if (ministroPath.isEmpty()) {
        QList<QByteArray> certificateData = fetchSslCertificateData();
        for (int i = 0; i < certificateData.size(); ++i) {
            systemCerts.append(QSslCertificate::fromData(certificateData.at(i), QSsl::Der));
        }
    } else
#  endif //Q_OS_ANDROID_EMBEDDED
# endif //Q_OS_ANDROID
    {
        currentDir.setNameFilters(nameFilters);
        for (int a = 0; a < directories.count(); a++) {
            currentDir.setPath(QLatin1String(directories.at(a)));
            QDirIterator it(currentDir);
            while (it.hasNext()) {
                it.next();
                // use canonical path here to not load the same certificate twice if symlinked
                certFiles.insert(it.fileInfo().canonicalFilePath());
            }
        }
        for (const QString& file : qAsConst(certFiles))
            systemCerts.append(QSslCertificate::fromPath(file, platformEncodingFormat));
# ifndef Q_OS_ANDROID
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/etc/pki/tls/certs/ca-bundle.crt"), QSsl::Pem)); // Fedora, Mandriva
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/usr/local/share/certs/ca-root-nss.crt"), QSsl::Pem)); // FreeBSD's ca_root_nss
# endif
    }
#endif
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "systemCaCertificates retrieval time " << timer.elapsed() << "ms";
    qCDebug(lcSsl) << "imported " << systemCerts.count() << " certificates";
#endif

    return systemCerts;
}
#endif // Q_OS_DARWIN

void QSslSocketBackendPrivate::startClientEncryption()
{
    if (!initSslContext()) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    if (!initSslContext()) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

/*!
    \internal

    Transmits encrypted data between the BIOs and the socket.
*/
void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    using ScopedBool = QScopedValueRollback<bool>;

    if (inSetAndEmitError)
        return;

    // If we don't have any SSL context, don't bother transmitting.
    if (!ssl)
        return;

    bool transmitting;
    do {
        transmitting = false;

        // If the connection is secure, we can transfer data from the write
        // buffer (in plain text) to the write BIO through SSL_write.
        if (connectionEncrypted && !writeBuffer.isEmpty()) {
            qint64 totalBytesWritten = 0;
            int nextDataBlockSize;
            while ((nextDataBlockSize = writeBuffer.nextDataBlockSize()) > 0) {
                int writtenBytes = q_SSL_write(ssl, writeBuffer.readPointer(), nextDataBlockSize);
                if (writtenBytes <= 0) {
                    int error = q_SSL_get_error(ssl, writtenBytes);
                    //write can result in a want_write_error - not an error - continue transmitting
                    if (error == SSL_ERROR_WANT_WRITE) {
                        transmitting = true;
                        break;
                    } else if (error == SSL_ERROR_WANT_READ) {
                        //write can result in a want_read error, possibly due to renegotiation - not an error - stop transmitting
                        transmitting = false;
                        break;
                    } else {
                        // ### Better error handling.
                        const ScopedBool bg(inSetAndEmitError, true);
                        setErrorAndEmit(QAbstractSocket::SslInternalError,
                                        QSslSocket::tr("Unable to write data: %1").arg(
                                            getErrorsFromOpenSsl()));
                        return;
                    }
                }
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encrypted" << writtenBytes << "bytes";
#endif
                writeBuffer.free(writtenBytes);
                totalBytesWritten += writtenBytes;

                if (writtenBytes < nextDataBlockSize) {
                    // break out of the writing loop and try again after we had read
                    transmitting = true;
                    break;
                }
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

        // Check if we've got any data to be written to the socket.
        QVarLengthArray<char, 4096> data;
        int pendingBytes;
        while (plainSocket->isValid() && (pendingBytes = q_BIO_pending(writeBio)) > 0
                && plainSocket->openMode() != QIODevice::NotOpen) {
            // Read encrypted data from the write BIO into a buffer.
            data.resize(pendingBytes);
            int encryptedBytesRead = q_BIO_read(writeBio, data.data(), pendingBytes);

            // Write encrypted data from the buffer to the socket.
            qint64 actualWritten = plainSocket->write(data.constData(), encryptedBytesRead);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: wrote" << encryptedBytesRead << "encrypted bytes to the socket" << actualWritten << "actual.";
#endif
            if (actualWritten < 0) {
                //plain socket write fails if it was in the pending close state.
                const ScopedBool bg(inSetAndEmitError, true);
                setErrorAndEmit(plainSocket->error(), plainSocket->errorString());
                return;
            }
            transmitting = true;
        }

        // Check if we've got any data to be read from the socket.
        if (!connectionEncrypted || !readBufferMaxSize || buffer.size() < readBufferMaxSize)
            while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
                // Read encrypted data from the socket into a buffer.
                data.resize(pendingBytes);
                // just peek() here because q_BIO_write could write less data than expected
                int encryptedBytesRead = plainSocket->peek(data.data(), pendingBytes);

#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: read" << encryptedBytesRead << "encrypted bytes from the socket";
#endif
                // Write encrypted data from the buffer into the read BIO.
                int writtenToBio = q_BIO_write(readBio, data.constData(), encryptedBytesRead);

                // Throw away the results.
                if (writtenToBio > 0) {
                    plainSocket->skip(writtenToBio);
                } else {
                    // ### Better error handling.
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Unable to decrypt data: %1").arg(
                                        getErrorsFromOpenSsl()));
                    return;
                }

                transmitting = true;
            }

        // If the connection isn't secured yet, this is the time to retry the
        // connect / accept.
        if (!connectionEncrypted) {
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: testing encryption";
#endif
            if (startHandshake()) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encryption established";
#endif
                connectionEncrypted = true;
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: connection lost";
#endif
                break;
            } else if (paused) {
                // just wait until the user continues
                return;
            } else {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encryption not done yet";
#endif
            }
        }

        // If the request is small and the remote host closes the transmission
        // after sending, there's a chance that startHandshake() will already
        // have triggered a shutdown.
        if (!ssl)
            continue;

        // We always read everything from the SSL decryption buffers, even if
        // we have a readBufferMaxSize. There's no point in leaving data there
        // just so that readBuffer.size() == readBufferMaxSize.
        int readBytes = 0;
        const int bytesToRead = 4096;
        do {
            if (readChannelCount == 0) {
                // The read buffer is deallocated, don't try resize or write to it.
                break;
            }
            // Don't use SSL_pending(). It's very unreliable.
            readBytes = q_SSL_read(ssl, buffer.reserve(bytesToRead), bytesToRead);
            if (readBytes > 0) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: decrypted" << readBytes << "bytes";
#endif
                buffer.chop(bytesToRead - readBytes);

                if (readyReadEmittedPointer)
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
                emit q->channelReadyRead(0);
                transmitting = true;
                continue;
            }
            buffer.chop(bytesToRead);

            // Error.
            switch (q_SSL_get_error(ssl, readBytes)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                // Out of data.
                break;
            case SSL_ERROR_ZERO_RETURN:
                // The remote host closed the connection.
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: remote disconnect";
#endif
                shutdown = true; // the other side shut down, make sure we do not send shutdown ourselves
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::RemoteHostClosedError,
                                    QSslSocket::tr("The TLS/SSL connection has been closed"));
                }
                return;
            case SSL_ERROR_SYSCALL: // some IO error
            case SSL_ERROR_SSL: // error in the SSL library
                // we do not know exactly what the error is, nor whether we can recover from it,
                // so just return to prevent an endless loop in the outer "while" statement
                systemOrSslErrorDetected = true;
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                }
                return;
            default:
                // SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT: can only happen with a
                // BIO_s_connect() or BIO_s_accept(), which we do not call.
                // SSL_ERROR_WANT_X509_LOOKUP: can only happen with a
                // SSL_CTX_set_client_cert_cb(), which we do not call.
                // So this default case should never be triggered.
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                }
                break;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && transmitting);
}

QSslError _q_OpenSSL_to_QSslError(int errorCode, const QSslCertificate &cert)
{
    QSslError error;
    switch (errorCode) {
    case X509_V_OK:
        // X509_V_OK is also reported if the peer had no certificate.
        break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        error = QSslError(QSslError::UnableToGetIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        error = QSslError(QSslError::UnableToDecryptCertificateSignature, cert); break;
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        error = QSslError(QSslError::UnableToDecodeIssuerPublicKey, cert); break;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        error = QSslError(QSslError::CertificateSignatureFailed, cert); break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
        error = QSslError(QSslError::CertificateNotYetValid, cert); break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
        error = QSslError(QSslError::CertificateExpired, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        error = QSslError(QSslError::InvalidNotBeforeField, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        error = QSslError(QSslError::InvalidNotAfterField, cert); break;
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        error = QSslError(QSslError::SelfSignedCertificate, cert); break;
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        error = QSslError(QSslError::SelfSignedCertificateInChain, cert); break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        error = QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        error = QSslError(QSslError::UnableToVerifyFirstCertificate, cert); break;
    case X509_V_ERR_CERT_REVOKED:
        error = QSslError(QSslError::CertificateRevoked, cert); break;
    case X509_V_ERR_INVALID_CA:
        error = QSslError(QSslError::InvalidCaCertificate, cert); break;
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        error = QSslError(QSslError::PathLengthExceeded, cert); break;
    case X509_V_ERR_INVALID_PURPOSE:
        error = QSslError(QSslError::InvalidPurpose, cert); break;
    case X509_V_ERR_CERT_UNTRUSTED:
        error = QSslError(QSslError::CertificateUntrusted, cert); break;
    case X509_V_ERR_CERT_REJECTED:
        error = QSslError(QSslError::CertificateRejected, cert); break;
    default:
        error = QSslError(QSslError::UnspecifiedError, cert); break;
    }
    return error;
}

QString QSslSocketBackendPrivate::msgErrorsDuringHandshake()
{
    return QSslSocket::tr("Error during SSL handshake: %1")
                         .arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
}

bool QSslSocketBackendPrivate::startHandshake()
{
    Q_Q(QSslSocket);

    // Check if the connection has been established. Get all errors from the
    // verification stage.

    using ScopedBool = QScopedValueRollback<bool>;

    if (inSetAndEmitError)
        return false;

    QVector<QSslErrorEntry> lastErrors;
    q_SSL_set_ex_data(ssl, s_indexForSSLExtraData + 1, &lastErrors);
    int result = (mode == QSslSocket::SslClientMode) ? q_SSL_connect(ssl) : q_SSL_accept(ssl);
    q_SSL_set_ex_data(ssl, s_indexForSSLExtraData + 1, nullptr);

    if (!lastErrors.isEmpty())
        storePeerCertificates();
    for (const auto &currentError : qAsConst(lastErrors)) {
        emit q->peerVerifyError(_q_OpenSSL_to_QSslError(currentError.code,
                                configuration.peerCertificateChain.value(currentError.depth)));
        if (q->state() != QAbstractSocket::ConnectedState)
            break;
    }

    errorList << lastErrors;

    // Connection aborted during handshake phase.
    if (q->state() != QAbstractSocket::ConnectedState)
        return false;

    // Check if we're encrypted or not.
    if (result <= 0) {
        switch (q_SSL_get_error(ssl, result)) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // The handshake is not yet complete.
            break;
        default:
            QString errorString = QSslSocketBackendPrivate::msgErrorsDuringHandshake();
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::startHandshake: error!" << errorString;
#endif
            {
                const ScopedBool bg(inSetAndEmitError, true);
                setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, errorString);
            }
            q->abort();
        }
        return false;
    }

    // store peer certificate chain
    storePeerCertificates();

    // Start translating errors.
    QList<QSslError> errors;

    // check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer)
    for (const QSslCertificate &cert : qAsConst(configuration.peerCertificateChain)) {
        if (QSslCertificatePrivate::isBlacklisted(cert)) {
            QSslError error(QSslError::CertificateBlacklisted, cert);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    const bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                              || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                                  && mode == QSslSocket::SslClientMode);

#if QT_CONFIG(ocsp)
    // For now it's always QSslSocket::SslClientMode - initSslContext() will bail out early,
    // if it's enabled in QSslSocket::SslServerMode. This can change.
    if (!configuration.peerCertificate.isNull() && configuration.ocspStaplingEnabled && doVerifyPeer) {
        if (!checkOcspStatus()) {
            if (ocspErrors.isEmpty()) {
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, ocspErrorDescription);
                }
                q->abort();
                return false;
            }

            for (const QSslError &error : ocspErrors) {
                errors << error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    }
#endif // ocsp

    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    if (!configuration.peerCertificate.isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        if (mode == QSslSocket::SslClientMode) {
            QString peerName = (verificationPeerName.isEmpty () ? q->peerName() : verificationPeerName);

            if (!isMatchingHostname(configuration.peerCertificate, peerName)) {
                // No matches in common names or alternate names.
                QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate);
                errors << error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    } else {
        // No peer certificate presented. Report as error if the socket
        // expected one.
        if (doVerifyPeer) {
            QSslError error(QSslError::NoPeerCertificate);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    // Translate errors from the error list into QSslErrors.
    errors.reserve(errors.size() + errorList.size());
    for (const auto &error : qAsConst(errorList))
        errors << _q_OpenSSL_to_QSslError(error.code, configuration.peerCertificateChain.value(error.depth));

    if (!errors.isEmpty()) {
        sslErrors = errors;

#ifdef Q_OS_WIN
        const bool fetchEnabled = s_loadRootCertsOnDemand
                                  && allowRootCertOnDemandLoading;
        // !fetchEnabled is a special case scenario, when we potentially have a missing
        // intermediate certificate and a recoverable chain, but on demand cert loading
        // was disabled by setCaCertificates call. For this scenario we check if "Authority
        // Information Access" is present - wincrypt can deal with such certificates.
        QSslCertificate certToFetch;
        if (doVerifyPeer && !verifyErrorsHaveBeenIgnored())
            certToFetch = findCertificateToFetch(sslErrors, !fetchEnabled);

        //Skip this if not using system CAs, or if the SSL errors are configured in advance to be ignorable
        if (!certToFetch.isNull()) {
            fetchAuthorityInformation = !fetchEnabled;
            //Windows desktop versions starting from vista ship with minimal set of roots and download on demand
            //from the windows update server CA roots that are trusted by MS. It also can fetch a missing intermediate
            //in case "Authority Information Access" extension is present.
            //
            //However, this is only transparent if using WinINET - we have to trigger it
            //ourselves.
            fetchCaRootForCert(certToFetch);
            return false;
        }
#endif
        if (!checkSslErrors())
            return false;
        // A slot, attached to sslErrors signal can call
        // abort/close/disconnetFromHost/etc; no need to
        // continue handshake then.
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    } else {
        sslErrors.clear();
    }

    continueHandshake();
    return true;
}

void QSslSocketBackendPrivate::storePeerCertificates()
{
    // Store the peer certificate and chain. For clients, the peer certificate
    // chain includes the peer certificate; for servers, it doesn't. Both the
    // peer certificate and the chain may be empty if the peer didn't present
    // any certificate.
    X509 *x509 = q_SSL_get_peer_certificate(ssl);
    configuration.peerCertificate = QSslCertificatePrivate::QSslCertificate_from_X509(x509);
    q_X509_free(x509);
    if (configuration.peerCertificateChain.isEmpty()) {
        configuration.peerCertificateChain = STACKOFX509_to_QSslCertificates(q_SSL_get_peer_cert_chain(ssl));
        if (!configuration.peerCertificate.isNull() && mode == QSslSocket::SslServerMode)
            configuration.peerCertificateChain.prepend(configuration.peerCertificate);
    }
}

int QSslSocketBackendPrivate::handleNewSessionTicket(SSL *connection)
{
    // If we return 1, this means we own the session, but we don't.
    // 0 would tell OpenSSL to deref (but they still have it in the
    // internal cache).
    Q_Q(QSslSocket);

    Q_ASSERT(connection);

    if (q->sslConfiguration().testSslOption(QSsl::SslOptionDisableSessionPersistence)) {
        // We silently ignore, do nothing, remove from cache.
        return 0;
    }

    SSL_SESSION *currentSession = q_SSL_get_session(connection);
    if (!currentSession) {
        qCWarning(lcSsl,
                  "New session ticket callback, the session is invalid (nullptr)");
        return 0;
    }

    if (q_SSL_version(connection) < 0x304) {
        // We only rely on this mechanics with TLS >= 1.3
        return 0;
    }

#ifdef TLS1_3_VERSION
    if (!q_SSL_SESSION_is_resumable(currentSession)) {
        qCDebug(lcSsl, "New session ticket, but the session is non-resumable");
        return 0;
    }
#endif // TLS1_3_VERSION

    const int sessionSize = q_i2d_SSL_SESSION(currentSession, nullptr);
    if (sessionSize <= 0) {
        qCWarning(lcSsl, "could not store persistent version of SSL session");
        return 0;
    }

    // We have somewhat perverse naming, it's not a ticket, it's a session.
    QByteArray sessionTicket(sessionSize, 0);
    auto data = reinterpret_cast<unsigned char *>(sessionTicket.data());
    if (!q_i2d_SSL_SESSION(currentSession, &data)) {
        qCWarning(lcSsl, "could not store persistent version of SSL session");
        return 0;
    }

    configuration.sslSession = sessionTicket;
    configuration.sslSessionTicketLifeTimeHint = int(q_SSL_SESSION_get_ticket_lifetime_hint(currentSession));

    emit q->newSessionTicketReceived();
    return 0;
}

bool QSslSocketBackendPrivate::checkSslErrors()
{
    Q_Q(QSslSocket);
    if (sslErrors.isEmpty())
        return true;

    emit q->sslErrors(sslErrors);

    bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                        || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                            && mode == QSslSocket::SslClientMode);
    bool doEmitSslError = !verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            pauseSocketNotifiers(q);
            paused = true;
        } else {
            setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, sslErrors.constFirst().errorString());
            plainSocket->disconnectFromHost();
        }
        return false;
    }
    return true;
}

unsigned int QSslSocketBackendPrivate::tlsPskClientCallback(const char *hint,
                                                            char *identity, unsigned int max_identity_len,
                                                            unsigned char *psk, unsigned int max_psk_len)
{
    QSslPreSharedKeyAuthenticator authenticator;

    // Fill in some read-only fields (for the user)
    if (hint)
        authenticator.d->identityHint = QByteArray::fromRawData(hint, int(::strlen(hint))); // it's NUL terminated, but do not include the NUL

    authenticator.d->maximumIdentityLength = int(max_identity_len) - 1; // needs to be NUL terminated
    authenticator.d->maximumPreSharedKeyLength = int(max_psk_len);

    // Let the client provide the remaining bits...
    Q_Q(QSslSocket);
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int identityLength = qMin(authenticator.identity().length(), authenticator.maximumIdentityLength());
    ::memcpy(identity, authenticator.identity().constData(), identityLength);
    identity[identityLength] = 0;

    const int pskLength = qMin(authenticator.preSharedKey().length(), authenticator.maximumPreSharedKeyLength());
    ::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

unsigned int QSslSocketBackendPrivate::tlsPskServerCallback(const char *identity,
                                                            unsigned char *psk, unsigned int max_psk_len)
{
    QSslPreSharedKeyAuthenticator authenticator;

    // Fill in some read-only fields (for the user)
    authenticator.d->identityHint = configuration.preSharedKeyIdentityHint;
    authenticator.d->identity = identity;
    authenticator.d->maximumIdentityLength = 0; // user cannot set an identity
    authenticator.d->maximumPreSharedKeyLength = int(max_psk_len);

    // Let the client provide the remaining bits...
    Q_Q(QSslSocket);
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int pskLength = qMin(authenticator.preSharedKey().length(), authenticator.maximumPreSharedKeyLength());
    ::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

#ifdef Q_OS_WIN

void QSslSocketBackendPrivate::fetchCaRootForCert(const QSslCertificate &cert)
{
    Q_Q(QSslSocket);
    //The root certificate is downloaded from windows update, which blocks for 15 seconds in the worst case
    //so the request is done in a worker thread.
    QList<QSslCertificate> customRoots;
    if (fetchAuthorityInformation)
        customRoots = configuration.caCertificates;

    QWindowsCaRootFetcher *fetcher = new QWindowsCaRootFetcher(cert, mode, customRoots, q->peerVerifyName());
    QObject::connect(fetcher, SIGNAL(finished(QSslCertificate,QSslCertificate)), q, SLOT(_q_caRootLoaded(QSslCertificate,QSslCertificate)), Qt::QueuedConnection);
    QMetaObject::invokeMethod(fetcher, "start", Qt::QueuedConnection);
    pauseSocketNotifiers(q);
    paused = true;
}

//This is the callback from QWindowsCaRootFetcher, trustedRoot will be invalid (default constructed) if it failed.
void QSslSocketBackendPrivate::_q_caRootLoaded(QSslCertificate cert, QSslCertificate trustedRoot)
{
    if (fetchAuthorityInformation) {
        if (!configuration.caCertificates.contains(trustedRoot))
            trustedRoot = QSslCertificate{};
        fetchAuthorityInformation = false;
    }

    if (!trustedRoot.isNull() && !trustedRoot.isBlacklisted()) {
        if (s_loadRootCertsOnDemand) {
            //Add the new root cert to default cert list for use by future sockets
            QSslSocket::addDefaultCaCertificate(trustedRoot);
        }
        //Add the new root cert to this socket for future connections
        if (!configuration.caCertificates.contains(trustedRoot))
            configuration.caCertificates += trustedRoot;
        //Remove the broken chain ssl errors (as chain is verified by windows)
        for (int i=sslErrors.count() - 1; i >= 0; --i) {
            if (sslErrors.at(i).certificate() == cert) {
                switch (sslErrors.at(i).error()) {
                case QSslError::UnableToGetLocalIssuerCertificate:
                case QSslError::CertificateUntrusted:
                case QSslError::UnableToVerifyFirstCertificate:
                case QSslError::SelfSignedCertificateInChain:
                    // error can be ignored if OS says the chain is trusted
                    sslErrors.removeAt(i);
                    break;
                default:
                    // error cannot be ignored
                    break;
                }
            }
        }
    }

    // Continue with remaining errors
    if (plainSocket)
        plainSocket->resume();
    paused = false;
    if (checkSslErrors() && ssl) {
        bool willClose = (autoStartHandshake && pendingClose);
        continueHandshake();
        if (!willClose)
            transmit();
    }
}

#endif

#if QT_CONFIG(ocsp)

bool QSslSocketBackendPrivate::checkOcspStatus()
{
    Q_ASSERT(ssl);
    Q_ASSERT(mode == QSslSocket::SslClientMode); // See initSslContext() for SslServerMode
    Q_ASSERT(configuration.peerVerifyMode != QSslSocket::VerifyNone);

    const auto clearErrorQueue = qScopeGuard([] {
        logAndClearErrorQueue();
    });

    ocspResponses.clear();
    ocspErrorDescription.clear();
    ocspErrors.clear();

    const unsigned char *responseData = nullptr;
    const long responseLength = q_SSL_get_tlsext_status_ocsp_resp(ssl, &responseData);
    if (responseLength <= 0 || !responseData) {
        ocspErrors.push_back(QSslError::OcspNoResponseFound);
        return false;
    }

    OCSP_RESPONSE *response = q_d2i_OCSP_RESPONSE(nullptr, &responseData, responseLength);
    if (!response) {
        // Treat this as a fatal SslHandshakeError.
        ocspErrorDescription = QSslSocket::tr("Failed to decode OCSP response");
        return false;
    }
    const QSharedPointer<OCSP_RESPONSE> responseGuard(response, q_OCSP_RESPONSE_free);

    const int ocspStatus = q_OCSP_response_status(response);
    if (ocspStatus != OCSP_RESPONSE_STATUS_SUCCESSFUL) {
        // It's not a definitive response, it's an error message (not signed by the responder).
        ocspErrors.push_back(qt_OCSP_response_status_to_QSslError(ocspStatus));
        return false;
    }

    OCSP_BASICRESP *basicResponse = q_OCSP_response_get1_basic(response);
    if (!basicResponse) {
        // SslHandshakeError.
        ocspErrorDescription = QSslSocket::tr("Failed to extract basic OCSP response");
        return false;
    }
    const QSharedPointer<OCSP_BASICRESP> basicResponseGuard(basicResponse, q_OCSP_BASICRESP_free);

    SSL_CTX *ctx = q_SSL_get_SSL_CTX(ssl); // Does not increment refcount.
    Q_ASSERT(ctx);
    X509_STORE *store = q_SSL_CTX_get_cert_store(ctx); // Does not increment refcount.
    if (!store) {
        // SslHandshakeError.
        ocspErrorDescription = QSslSocket::tr("No certificate verification store, cannot verify OCSP response");
        return false;
    }

    STACK_OF(X509) *peerChain = q_SSL_get_peer_cert_chain(ssl); // Does not increment refcount.
    X509 *peerX509 = q_SSL_get_peer_certificate(ssl);
    Q_ASSERT(peerChain || peerX509);
    const QSharedPointer<X509> peerX509Guard(peerX509, q_X509_free);
    // OCSP_basic_verify with 0 as verificationFlags:
    //
    // 0) Tries to find the OCSP responder's certificate in either peerChain
    // or basicResponse->certs. If not found, verification fails.
    // 1) It checks the signature using the responder's public key.
    // 2) Then it tries to validate the responder's cert (building a chain
    //    etc.)
    // 3) It checks CertID in response.
    // 4) Ensures the responder is authorized to sign the status respond.
    //
    // Note, OpenSSL prior to 1.0.2b would only use bs->certs to
    // verify the responder's chain (see their commit 4ba9a4265bd).
    // Working this around - is too much fuss for ancient versions we
    // are dropping quite soon anyway.
    const unsigned long verificationFlags = 0;
    const int success = q_OCSP_basic_verify(basicResponse, peerChain, store, verificationFlags);
    if (success <= 0)
        ocspErrors.push_back(QSslError::OcspResponseCannotBeTrusted);

    if (q_OCSP_resp_count(basicResponse) != 1) {
        ocspErrors.push_back(QSslError::OcspMalformedResponse);
        return false;
    }

    OCSP_SINGLERESP *singleResponse = q_OCSP_resp_get0(basicResponse, 0);
    if (!singleResponse) {
        ocspErrors.clear();
        // A fatal problem -> SslHandshakeError.
        ocspErrorDescription = QSslSocket::tr("Failed to decode a SingleResponse from OCSP status response");
        return false;
    }

    // Let's make sure the response is for the correct certificate - we
    // can re-create this CertID using our peer's certificate and its
    // issuer's public key.
    ocspResponses.push_back(QOcspResponse());
    QOcspResponsePrivate *dResponse = ocspResponses.back().d.data();
    dResponse->subjectCert = configuration.peerCertificate;
    bool matchFound = false;
    if (configuration.peerCertificate.isSelfSigned()) {
        dResponse->signerCert = configuration.peerCertificate;
        matchFound = qt_OCSP_certificate_match(singleResponse, peerX509, peerX509);
    } else {
        const STACK_OF(X509) *certs = q_SSL_get_peer_cert_chain(ssl);
        if (!certs) // Oh, what a cataclysm! Last try:
            certs = q_OCSP_resp_get0_certs(basicResponse);
        if (certs) {
            // It could be the first certificate in 'certs' is our peer's
            // certificate. Since it was not captured by the 'self-signed' branch
            // above, the CertID will not match and we'll just iterate on to the
            // next certificate. So we start from 0, not 1.
            for (int i = 0, e = q_sk_X509_num(certs); i < e; ++i) {
                X509 *issuer = q_sk_X509_value(certs, i);
                matchFound = qt_OCSP_certificate_match(singleResponse, peerX509, issuer);
                if (matchFound) {
                    if (q_X509_check_issued(issuer, peerX509) == X509_V_OK) {
                        dResponse->signerCert =  QSslCertificatePrivate::QSslCertificate_from_X509(issuer);
                        break;
                    }
                    matchFound = false;
                }
            }
        }
    }

    if (!matchFound) {
        dResponse->signerCert.clear();
        ocspErrors.push_back({QSslError::OcspResponseCertIdUnknown, configuration.peerCertificate});
    }

    // Check if the response is valid time-wise:
    ASN1_GENERALIZEDTIME *revTime = nullptr;
    ASN1_GENERALIZEDTIME *thisUpdate = nullptr;
    ASN1_GENERALIZEDTIME *nextUpdate = nullptr;
    int reason;
    const int certStatus = q_OCSP_single_get0_status(singleResponse, &reason, &revTime, &thisUpdate, &nextUpdate);
    if (!thisUpdate) {
        // This is unexpected, treat as SslHandshakeError, OCSP_check_validity assumes this pointer
        // to be != nullptr.
        ocspErrors.clear();
        ocspResponses.clear();
        ocspErrorDescription = QSslSocket::tr("Failed to extract 'this update time' from the SingleResponse");
        return false;
    }

    // OCSP_check_validity(this, next, nsec, maxsec) does this check:
    // this <= now <= next. They allow some freedom to account
    // for delays/time inaccuracy.
    // this > now + nsec ? -> NOT_YET_VALID
    // if maxsec >= 0:
    //     now - maxsec > this ? -> TOO_OLD
    // now - nsec > next ? -> EXPIRED
    // next < this ? -> NEXT_BEFORE_THIS
    // OK.
    if (!q_OCSP_check_validity(thisUpdate, nextUpdate, 60, -1))
        ocspErrors.push_back({QSslError::OcspResponseExpired, configuration.peerCertificate});

    // And finally, the status:
    switch (certStatus) {
    case V_OCSP_CERTSTATUS_GOOD:
        // This certificate was not found among the revoked ones.
        dResponse->certificateStatus = QOcspCertificateStatus::Good;
        break;
    case V_OCSP_CERTSTATUS_REVOKED:
        dResponse->certificateStatus = QOcspCertificateStatus::Revoked;
        dResponse->revocationReason = qt_OCSP_revocation_reason(reason);
        ocspErrors.push_back({QSslError::CertificateRevoked, configuration.peerCertificate});
        break;
    case V_OCSP_CERTSTATUS_UNKNOWN:
        dResponse->certificateStatus = QOcspCertificateStatus::Unknown;
        ocspErrors.push_back({QSslError::OcspStatusUnknown, configuration.peerCertificate});
    }

    return !ocspErrors.size();
}

#endif // ocsp

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (ssl) {
        if (!shutdown && !q_SSL_in_init(ssl) && !systemOrSslErrorDetected) {
            if (q_SSL_shutdown(ssl) != 1) {
                // Some error may be queued, clear it.
                const auto errors = getErrorsFromOpenSsl();
                Q_UNUSED(errors);
            }
            shutdown = true;
            transmit();
        }
    }
    plainSocket->disconnectFromHost();
}

void QSslSocketBackendPrivate::disconnected()
{
    if (plainSocket->bytesAvailable() <= 0)
        destroySslContext();
    else {
        // Move all bytes into the plain buffer
        qint64 tmpReadBufferMaxSize = readBufferMaxSize;
        readBufferMaxSize = 0; // reset temporarily so the plain socket buffer is completely drained
        transmit();
        readBufferMaxSize = tmpReadBufferMaxSize;
    }
    //if there is still buffered data in the plain socket, don't destroy the ssl context yet.
    //it will be destroyed when the socket is deleted.
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    if (!ssl)
        return QSslCipher();

    const SSL_CIPHER *sessionCipher = q_SSL_get_current_cipher(ssl);
    return sessionCipher ? QSslCipher_from_SSL_CIPHER(sessionCipher) : QSslCipher();
}

QSsl::SslProtocol QSslSocketBackendPrivate::sessionProtocol() const
{
    if (!ssl)
        return QSsl::UnknownProtocol;
    int ver = q_SSL_version(ssl);

    switch (ver) {
    case 0x2:
        return QSsl::SslV2;
    case 0x300:
        return QSsl::SslV3;
    case 0x301:
        return QSsl::TlsV1_0;
    case 0x302:
        return QSsl::TlsV1_1;
    case 0x303:
        return QSsl::TlsV1_2;
    case 0x304:
        return QSsl::TlsV1_3;
    }

    return QSsl::UnknownProtocol;
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
        const unsigned char *proto = nullptr;
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

bool QSslSocketPrivate::ensureLibraryLoaded()
{
    if (!q_resolveOpenSslSymbols())
        return false;

    const QMutexLocker locker(qt_opensslInitMutex);

    if (!s_libraryLoaded) {
        // Initialize OpenSSL.
        if (q_OPENSSL_init_ssl(0, nullptr) != 1)
            return false;

        if (q_OpenSSL_version_num() < 0x10101000L) {
            qCWarning(lcSsl, "QSslSocket: OpenSSL >= 1.1.1 is required; %s was found instead", q_OpenSSL_version(OPENSSL_VERSION));
            return false;
        }

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

        s_libraryLoaded = true;
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
#if defined(Q_OS_QNX)
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
    //Enabled for fetching additional root certs from windows update on windows.
    //This flag is set false by setDefaultCaCertificates() indicating the app uses
    //its own cert bundle rather than the system one.
    //Same logic that disables the unix on demand cert loading.
    //Unlike unix, we do preload the certificates from the cert store.
    s_loadRootCertsOnDemand = true;
#endif
}

QList<QSslCertificate> QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509)
{
    ensureInitialized();
    QList<QSslCertificate> certificates;
    for (int i = 0; i < q_sk_X509_num(x509); ++i) {
        if (X509 *entry = q_sk_X509_value(x509, i))
            certificates << QSslCertificatePrivate::QSslCertificate_from_X509(entry);
    }
    return certificates;
}

QList<QSslError> QSslSocketBackendPrivate::verify(const QList<QSslCertificate> &certificateChain,
                                                  const QString &hostName)
{
    if (s_loadRootCertsOnDemand)
        setDefaultCaCertificates(defaultCaCertificates() + systemCaCertificates());

    return verify(QSslConfiguration::defaultConfiguration().caCertificates(), certificateChain, hostName);
}

QList<QSslError> QSslSocketBackendPrivate::verify(const QList<QSslCertificate> &caCertificates,
                                                  const QList<QSslCertificate> &certificateChain,
                                                  const QString &hostName)
{
    if (certificateChain.count() <= 0)
        return {QSslError(QSslError::UnspecifiedError)};

    QList<QSslError> errors;
    // Setup the store with the default CA certificates
    X509_STORE *certStore = q_X509_STORE_new();
    if (!certStore) {
        qCWarning(lcSsl) << "Unable to create certificate store";
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }
    const std::unique_ptr<X509_STORE, decltype(&q_X509_STORE_free)> storeGuard(certStore, q_X509_STORE_free);

    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const QSslCertificate &caCertificate : caCertificates) {
        // From https://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html:
        //
        // If several CA certificates matching the name, key identifier, and
        // serial number condition are available, only the first one will be
        // examined. This may lead to unexpected results if the same CA
        // certificate is available with different expiration dates. If a
        // ``certificate expired'' verification error occurs, no other
        // certificate will be searched. Make sure to not have expired
        // certificates mixed with valid ones.
        //
        // See also: QSslContext::fromConfiguration()
        if (caCertificate.expiryDate() >= now) {
            q_X509_STORE_add_cert(certStore, reinterpret_cast<X509 *>(caCertificate.handle()));
        }
    }

    QVector<QSslErrorEntry> lastErrors;
    if (!q_X509_STORE_set_ex_data(certStore, 0, &lastErrors)) {
        qCWarning(lcSsl) << "Unable to attach external data (error list) to a store";
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Register a custom callback to get all verification errors.
    q_X509_STORE_set_verify_cb(certStore, q_X509Callback);

    // Build the chain of intermediate certificates
    STACK_OF(X509) *intermediates = nullptr;
    if (certificateChain.length() > 1) {
        intermediates = (STACK_OF(X509) *) q_OPENSSL_sk_new_null();

        if (!intermediates) {
            errors << QSslError(QSslError::UnspecifiedError);
            return errors;
        }

        bool first = true;
        for (const QSslCertificate &cert : certificateChain) {
            if (first) {
                first = false;
                continue;
            }

            q_OPENSSL_sk_push((OPENSSL_STACK *)intermediates, reinterpret_cast<X509 *>(cert.handle()));
        }
    }

    X509_STORE_CTX *storeContext = q_X509_STORE_CTX_new();
    if (!storeContext) {
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }
    std::unique_ptr<X509_STORE_CTX, decltype(&q_X509_STORE_CTX_free)> ctxGuard(storeContext, q_X509_STORE_CTX_free);

    if (!q_X509_STORE_CTX_init(storeContext, certStore, reinterpret_cast<X509 *>(certificateChain[0].handle()), intermediates)) {
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Now we can actually perform the verification of the chain we have built.
    // We ignore the result of this function since we process errors via the
    // callback.
    (void) q_X509_verify_cert(storeContext);
    ctxGuard.reset();
    q_OPENSSL_sk_free((OPENSSL_STACK *)intermediates);

    // Now process the errors

    if (QSslCertificatePrivate::isBlacklisted(certificateChain[0])) {
        QSslError error(QSslError::CertificateBlacklisted, certificateChain[0]);
        errors << error;
    }

    // Check the certificate name against the hostname if one was specified
    if ((!hostName.isEmpty()) && (!isMatchingHostname(certificateChain[0], hostName))) {
        // No matches in common names or alternate names.
        QSslError error(QSslError::HostNameMismatch, certificateChain[0]);
        errors << error;
    }

    // Translate errors from the error list into QSslErrors.
    errors.reserve(errors.size() + lastErrors.size());
    for (const auto &error : qAsConst(lastErrors))
        errors << _q_OpenSSL_to_QSslError(error.code, certificateChain.value(error.depth));

    return errors;
}

bool QSslSocketBackendPrivate::importPkcs12(QIODevice *device,
                                            QSslKey *key, QSslCertificate *cert,
                                            QList<QSslCertificate> *caCertificates,
                                            const QByteArray &passPhrase)
{
    if (!supportsSsl())
        return false;

    // These are required
    Q_ASSERT(device);
    Q_ASSERT(key);
    Q_ASSERT(cert);

    // Read the file into a BIO
    QByteArray pkcs12data = device->readAll();
    if (pkcs12data.size() == 0)
        return false;

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pkcs12data.constData()), pkcs12data.size());

    // Create the PKCS#12 object
    PKCS12 *p12 = q_d2i_PKCS12_bio(bio, nullptr);
    if (!p12) {
        qCWarning(lcSsl, "Unable to read PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), nullptr));
        q_BIO_free(bio);
        return false;
    }

    // Extract the data
    EVP_PKEY *pkey = nullptr;
    X509 *x509;
    STACK_OF(X509) *ca = nullptr;

    if (!q_PKCS12_parse(p12, passPhrase.constData(), &pkey, &x509, &ca)) {
        qCWarning(lcSsl, "Unable to parse PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), nullptr));
        q_PKCS12_free(p12);
        q_BIO_free(bio);
        return false;
    }

    // Convert to Qt types
    if (!key->d->fromEVP_PKEY(pkey)) {
        qCWarning(lcSsl, "Unable to convert private key");
        q_OPENSSL_sk_pop_free(reinterpret_cast<OPENSSL_STACK *>(ca),
                              reinterpret_cast<void (*)(void *)>(q_X509_free));
        q_X509_free(x509);
        q_EVP_PKEY_free(pkey);
        q_PKCS12_free(p12);
        q_BIO_free(bio);

        return false;
    }

    *cert = QSslCertificatePrivate::QSslCertificate_from_X509(x509);

    if (caCertificates)
        *caCertificates = QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(ca);

    // Clean up
    q_OPENSSL_sk_pop_free(reinterpret_cast<OPENSSL_STACK *>(ca),
                          reinterpret_cast<void (*)(void *)>(q_X509_free));

    q_X509_free(x509);
    q_EVP_PKEY_free(pkey);
    q_PKCS12_free(p12);
    q_BIO_free(bio);

    return true;
}


QT_END_NAMESPACE
