// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsslsocket_openssl_symbols_p.h"
#include "qx509_openssl_p.h"
#include "qtls_openssl_p.h"

#ifdef Q_OS_WIN
#include "qwindowscarootfetcher_p.h"
#endif

#include <QtNetwork/private/qsslpresharedkeyauthenticator_p.h>
#include <QtNetwork/private/qsslcertificate_p.h>
#include <QtNetwork/private/qocspresponse_p.h>
#include <QtNetwork/private/qsslsocket_p.h>

#include <QtNetwork/qsslpresharedkeyauthenticator.h>

#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qscopeguard.h>

#include <algorithm>
#include <cstring>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace  {

QSsl::AlertLevel tlsAlertLevel(int value)
{
    using QSsl::AlertLevel;

    if (const char *typeString = q_SSL_alert_type_string(value)) {
        // Documented to return 'W' for warning, 'F' for fatal,
        // 'U' for unknown.
        switch (typeString[0]) {
        case 'W':
            return AlertLevel::Warning;
        case 'F':
            return AlertLevel::Fatal;
        default:;
        }
    }

    return AlertLevel::Unknown;
}

QString tlsAlertDescription(int value)
{
    QString description = QLatin1StringView(q_SSL_alert_desc_string_long(value));
    if (!description.size())
        description = "no description provided"_L1;
    return description;
}

QSsl::AlertType tlsAlertType(int value)
{
    // In case for some reason openssl gives us a value,
    // which is not in our enum actually, we leave it to
    // an application to handle (supposedly they have
    // if or switch-statements).
    return QSsl::AlertType(value & 0xff);
}

#ifdef Q_OS_WIN

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
            qCDebug(lcTlsBackend) << tlsError.errorString();
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

#endif // Q_OS_WIN

} // unnamed namespace

namespace QTlsPrivate {

extern "C" {

int q_X509Callback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok) {
        // Store the error and at which depth the error was detected.

        using ErrorListPtr = QList<QSslErrorEntry> *;
        ErrorListPtr errors = nullptr;

        // Error list is attached to either 'SSL' or 'X509_STORE'.
        if (X509_STORE *store = q_X509_STORE_CTX_get0_store(ctx)) // We try store first:
            errors = ErrorListPtr(q_X509_STORE_get_ex_data(store, 0));

        if (!errors) {
            // Not found on store? Try SSL and its external data then. According to the OpenSSL's
            // documentation:
            //
            // "Whenever a X509_STORE_CTX object is created for the verification of the
            // peer's certificate during a handshake, a pointer to the SSL object is
            // stored into the X509_STORE_CTX object to identify the connection affected.
            // To retrieve this pointer the X509_STORE_CTX_get_ex_data() function can be
            // used with the correct index."
            const auto offset = QTlsBackendOpenSSL::s_indexForSSLExtraData
                    + TlsCryptographOpenSSL::errorOffsetInExData;
            if (SSL *ssl = static_cast<SSL *>(q_X509_STORE_CTX_get_ex_data(
                        ctx, q_SSL_get_ex_data_X509_STORE_CTX_idx()))) {

                // We may be in a renegotiation, check if we are inside a call to SSL_read:
                const auto tlsOffset = QTlsBackendOpenSSL::s_indexForSSLExtraData
                        + TlsCryptographOpenSSL::socketOffsetInExData;
                auto tls = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, tlsOffset));
                Q_ASSERT(tls);
                if (tls->isInSslRead()) {
                    // We are in a renegotiation, make a note of this for later.
                    // We'll check that the certificate is the same as the one we got during
                    // the initial handshake
                    tls->setRenegotiated(true);
                    return 1;
                }

                errors = ErrorListPtr(q_SSL_get_ex_data(ssl, offset));
            }
        }

        if (!errors) {
            qCWarning(lcTlsBackend, "Neither X509_STORE, nor SSL contains error list, handshake failure");
            return 0;
        }

        errors->append(X509CertificateOpenSSL::errorEntryFromStoreContext(ctx));
    }
    // Always return OK to allow verification to continue. We handle the
    // errors gracefully after collecting all errors, after verification has
    // completed.
    return 1;
}

int q_X509CallbackDirect(int ok, X509_STORE_CTX *ctx)
{
    // Passed to SSL_CTX_set_verify()
    // https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_verify.html
    // Returns 0 to abort verification, 1 to continue.

    // This is a new, experimental verification callback, reporting
    // errors immediately and returning 0 or 1 depending on an application
    // either ignoring or not ignoring verification errors as they come.
    if (!ctx) {
        qCWarning(lcTlsBackend, "Invalid store context (nullptr)");
        return 0;
    }

    if (!ok) {
        // "Whenever a X509_STORE_CTX object is created for the verification of the
        // peer's certificate during a handshake, a pointer to the SSL object is
        // stored into the X509_STORE_CTX object to identify the connection affected.
        // To retrieve this pointer the X509_STORE_CTX_get_ex_data() function can be
        // used with the correct index."
        SSL *ssl = static_cast<SSL *>(q_X509_STORE_CTX_get_ex_data(ctx, q_SSL_get_ex_data_X509_STORE_CTX_idx()));
        if (!ssl) {
            qCWarning(lcTlsBackend, "No external data (SSL) found in X509 store object");
            return 0;
        }

        const auto offset = QTlsBackendOpenSSL::s_indexForSSLExtraData
                            + TlsCryptographOpenSSL::socketOffsetInExData;
        auto crypto = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, offset));
        if (!crypto) {
            qCWarning(lcTlsBackend, "No external data (TlsCryptographOpenSSL) found in SSL object");
            return 0;
        }

        return crypto->emitErrorFromCallback(ctx);
    }
    return 1;
}

#ifndef OPENSSL_NO_PSK
static unsigned q_ssl_psk_client_callback(SSL *ssl, const char *hint, char *identity, unsigned max_identity_len,
                                          unsigned char *psk, unsigned max_psk_len)
{
    auto *tls = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData));
    return tls->pskClientTlsCallback(hint, identity, max_identity_len, psk, max_psk_len);
}

static unsigned int q_ssl_psk_server_callback(SSL *ssl, const char *identity, unsigned char *psk,
                                              unsigned int max_psk_len)
{
    auto *tls = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData));
    Q_ASSERT(tls);
    return tls->pskServerTlsCallback(identity, psk, max_psk_len);
}

#ifdef TLS1_3_VERSION
static unsigned q_ssl_psk_restore_client(SSL *ssl, const char *hint, char *identity, unsigned max_identity_len,
                                         unsigned char *psk, unsigned max_psk_len)
{
    Q_UNUSED(hint);
    Q_UNUSED(identity);
    Q_UNUSED(max_identity_len);
    Q_UNUSED(psk);
    Q_UNUSED(max_psk_len);

#ifdef QT_DEBUG
    auto tls = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData));
    Q_ASSERT(tls);
    Q_ASSERT(tls->d);
    Q_ASSERT(tls->d->tlsMode() == QSslSocket::SslClientMode);
#endif
    unsigned retVal = 0;

    // Let developers opt-in to having the normal PSK callback get called for TLS 1.3
    // PSK (which works differently in a few ways, and is called at the start of every connection).
    // When they do opt-in we just call the old callback from here.
    if (qEnvironmentVariableIsSet("QT_USE_TLS_1_3_PSK"))
        retVal = q_ssl_psk_client_callback(ssl, hint, identity, max_identity_len, psk, max_psk_len);

    q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_client_callback);

    return retVal;
}

static int q_ssl_psk_use_session_callback(SSL *ssl, const EVP_MD *md, const unsigned char **id,
                                          size_t *idlen, SSL_SESSION **sess)
{
    Q_UNUSED(md);
    Q_UNUSED(id);
    Q_UNUSED(idlen);
    Q_UNUSED(sess);

#ifdef QT_DEBUG
    auto *tls = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData));
    Q_ASSERT(tls);
    Q_ASSERT(tls->d);
    Q_ASSERT(tls->d->tlsMode() == QSslSocket::SslClientMode);
#endif

    // Temporarily rebind the psk because it will be called next. The function will restore it.
    q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_restore_client);

    return 1; // need to return 1 or else "the connection setup fails."
}

int q_ssl_sess_set_new_cb(SSL *ssl, SSL_SESSION *session)
{
    if (!ssl) {
        qCWarning(lcTlsBackend, "Invalid SSL (nullptr)");
        return 0;
    }
    if (!session) {
        qCWarning(lcTlsBackend, "Invalid SSL_SESSION (nullptr)");
        return 0;
    }

    auto *tls = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData));
    Q_ASSERT(tls);
    return tls->handleNewSessionTicket(ssl);
}
#endif // TLS1_3_VERSION

#endif // !OPENSSL_NO_PSK

#if QT_CONFIG(ocsp)

int qt_OCSP_status_server_callback(SSL *ssl, void *ocspRequest)
{
    Q_UNUSED(ocspRequest);
    if (!ssl)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    auto crypto = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData));
    if (!crypto)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    Q_ASSERT(crypto->d);
    Q_ASSERT(crypto->d->tlsMode() == QSslSocket::SslServerMode);
    const QByteArray &response = crypto->ocspResponseDer;
    Q_ASSERT(response.size());

    unsigned char *derCopy = static_cast<unsigned char *>(q_OPENSSL_malloc(size_t(response.size())));
    if (!derCopy)
        return SSL_TLSEXT_ERR_ALERT_FATAL;

    std::copy(response.data(), response.data() + response.size(), derCopy);
    // We don't check the return value: internally OpenSSL simply assigns the
    // pointer (it assumes it now owns this memory btw!) and the length.
    q_SSL_set_tlsext_status_ocsp_resp(ssl, derCopy, response.size());

    return SSL_TLSEXT_ERR_OK;
}

#endif // ocsp

void qt_AlertInfoCallback(const SSL *connection, int from, int value)
{
    // Passed to SSL_set_info_callback()
    // https://www.openssl.org/docs/man1.1.1/man3/SSL_set_info_callback.html

    if (!connection) {
#ifdef QSSLSOCKET_DEBUG
        qCWarning(lcTlsBackend, "Invalid 'connection' parameter (nullptr)");
#endif // QSSLSOCKET_DEBUG
        return;
    }

    const auto offset = QTlsBackendOpenSSL::s_indexForSSLExtraData
                        + TlsCryptographOpenSSL::socketOffsetInExData;
    auto crypto = static_cast<TlsCryptographOpenSSL *>(q_SSL_get_ex_data(connection, offset));
    if (!crypto) {
        // SSL_set_ex_data can fail:
#ifdef QSSLSOCKET_DEBUG
        qCWarning(lcTlsBackend, "No external data (socket backend) found for parameter 'connection'");
#endif // QSSLSOCKET_DEBUG
        return;
    }

    if (!(from & SSL_CB_ALERT)) {
        // We only want to know about alerts (at least for now).
        return;
    }

    if (from & SSL_CB_WRITE)
        crypto->alertMessageSent(value);
    else
        crypto->alertMessageReceived(value);
}

} // extern "C"

#if QT_CONFIG(ocsp)
namespace {

QSslError::SslError qt_OCSP_response_status_to_SslError(long code)
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
        qCWarning(lcTlsBackend, "A SingleResponse without CertID");
        return false;
    }

    ASN1_OBJECT *md = nullptr;
    ASN1_INTEGER *reportedSerialNumber = nullptr;
    const int result =  q_OCSP_id_get0_info(nullptr, &md, nullptr, &reportedSerialNumber, const_cast<OCSP_CERTID *>(certId));
    if (result != 1 || !md || !reportedSerialNumber) {
        qCWarning(lcTlsBackend, "Failed to extract a hash and serial number from CertID structure");
        return false;
    }

    if (!q_X509_get_serialNumber(peerCert)) {
        // Is this possible at all? But we have to check this,
        // ASN1_INTEGER_cmp (called from OCSP_id_cmp) dereferences
        // without any checks at all.
        qCWarning(lcTlsBackend, "No serial number in peer's ceritificate");
        return false;
    }

    const int nid = q_OBJ_obj2nid(md);
    if (nid == NID_undef) {
        qCWarning(lcTlsBackend, "Unknown hash algorithm in CertID");
        return false;
    }

    const EVP_MD *digest = q_EVP_get_digestbynid(nid); // Does not increment refcount.
    if (!digest) {
        qCWarning(lcTlsBackend) << "No digest for nid" << nid;
        return false;
    }

    OCSP_CERTID *recreatedId = q_OCSP_cert_to_id(digest, peerCert, issuer);
    if (!recreatedId) {
        qCWarning(lcTlsBackend, "Failed to re-create CertID");
        return false;
    }
    const QSharedPointer<OCSP_CERTID> guard(recreatedId, q_OCSP_CERTID_free);

    if (q_OCSP_id_cmp(const_cast<OCSP_CERTID *>(certId), recreatedId)) {
        qCDebug(lcTlsBackend, "Certificate ID mismatch");
        return false;
    }
    // Bingo!
    return true;
}

} // unnamed namespace
#endif // ocsp

TlsCryptographOpenSSL::~TlsCryptographOpenSSL()
{
    destroySslContext();
}

void TlsCryptographOpenSSL::init(QSslSocket *qObj, QSslSocketPrivate *dObj)
{
    Q_ASSERT(qObj);
    Q_ASSERT(dObj);
    q = qObj;
    d = dObj;

    ocspResponses.clear();
    ocspResponseDer.clear();

    systemOrSslErrorDetected = false;
    handshakeInterrupted = false;

    fetchAuthorityInformation = false;
    caToFetch.reset();
}

void TlsCryptographOpenSSL::checkSettingSslContext(std::shared_ptr<QSslContext> tlsContext)
{
    if (!sslContextPointer)
        sslContextPointer = std::move(tlsContext);
}

std::shared_ptr<QSslContext> TlsCryptographOpenSSL::sslContext() const
{
    return sslContextPointer;
}

QList<QSslError> TlsCryptographOpenSSL::tlsErrors() const
{
    return sslErrors;
}

void TlsCryptographOpenSSL::startClientEncryption()
{
    if (!initSslContext()) {
        Q_ASSERT(d);
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context: %1").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

void TlsCryptographOpenSSL::startServerEncryption()
{
    if (!initSslContext()) {
        Q_ASSERT(d);
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context: %1").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

bool TlsCryptographOpenSSL::startHandshake()
{
    // Check if the connection has been established. Get all errors from the
    // verification stage.
    Q_ASSERT(q);
    Q_ASSERT(d);

    using ScopedBool = QScopedValueRollback<bool>;

    if (inSetAndEmitError)
        return false;

    const auto mode = d->tlsMode();

    pendingFatalAlert = false;
    errorsReportedFromCallback = false;
    QList<QSslErrorEntry> lastErrors;
    q_SSL_set_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData + errorOffsetInExData, &lastErrors);

    // SSL_set_ex_data can fail, but see the callback's code - we handle this there.
    q_SSL_set_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData + socketOffsetInExData, this);
    q_SSL_set_info_callback(ssl, qt_AlertInfoCallback);

    int result = (mode == QSslSocket::SslClientMode) ? q_SSL_connect(ssl) : q_SSL_accept(ssl);
    q_SSL_set_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData + errorOffsetInExData, nullptr);
    // Note, unlike errors as external data on SSL object, we do not unset
    // a callback/ex-data if alert notifications are enabled: an alert can
    // arrive after the handshake, for example, this happens when the server
    // does not find a ClientCert or does not like it.

    if (!lastErrors.isEmpty() || errorsReportedFromCallback)
        storePeerCertificates();

    // storePeerCertificate() if called above - would update the
    // configuration with peer's certificates.
    auto configuration = q->sslConfiguration();
    if (!errorsReportedFromCallback) {
        const auto &peerCertificateChain = configuration.peerCertificateChain();
        for (const auto &currentError : std::as_const(lastErrors)) {
            emit q->peerVerifyError(QTlsPrivate::X509CertificateOpenSSL::openSSLErrorToQSslError(currentError.code,
                                    peerCertificateChain.value(currentError.depth)));
            if (q->state() != QAbstractSocket::ConnectedState)
                break;
        }
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
            QString errorString = QTlsBackendOpenSSL::msgErrorsDuringHandshake();
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::startHandshake: error!" << errorString;
#endif
            {
                const ScopedBool bg(inSetAndEmitError, true);
                setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError, errorString);
                if (pendingFatalAlert) {
                    trySendFatalAlert();
                    pendingFatalAlert = false;
                }
            }
            q->abort();
        }
        return false;
    }

    // store peer certificate chain
    storePeerCertificates();

    // Start translating errors.
    QList<QSslError> errors;

    // Note, the storePeerCerificates() probably updated the configuration at this point.
    configuration = q->sslConfiguration();
    // Check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer)
    const auto &peerCertificateChain = configuration.peerCertificateChain();
    for (const QSslCertificate &cert : peerCertificateChain) {
        if (QSslCertificatePrivate::isBlacklisted(cert)) {
            QSslError error(QSslError::CertificateBlacklisted, cert);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    const bool doVerifyPeer = configuration.peerVerifyMode() == QSslSocket::VerifyPeer
                              || (configuration.peerVerifyMode() == QSslSocket::AutoVerifyPeer
                                  && mode == QSslSocket::SslClientMode);

#if QT_CONFIG(ocsp)
    // For now it's always QSslSocket::SslClientMode - initSslContext() will bail out early,
    // if it's enabled in QSslSocket::SslServerMode. This can change.
    if (!configuration.peerCertificate().isNull() && configuration.ocspStaplingEnabled() && doVerifyPeer) {
        if (!checkOcspStatus()) {
            if (ocspErrors.isEmpty()) {
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError, ocspErrorDescription);
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
    if (!configuration.peerCertificate().isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        const auto verificationPeerName = d->verificationName();
        if (mode == QSslSocket::SslClientMode) {
            QString peerName = (verificationPeerName.isEmpty () ? q->peerName() : verificationPeerName);

            if (!isMatchingHostname(configuration.peerCertificate(), peerName)) {
                // No matches in common names or alternate names.
                QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate());
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
    for (const auto &error : std::as_const(errorList))
        errors << X509CertificateOpenSSL::openSSLErrorToQSslError(error.code, peerCertificateChain.value(error.depth));

    if (!errors.isEmpty()) {
        sslErrors = errors;
#ifdef Q_OS_WIN
        const bool fetchEnabled = QSslSocketPrivate::rootCertOnDemandLoadingSupported()
                                  && d->isRootsOnDemandAllowed();
        // !fetchEnabled is a special case scenario, when we potentially have a missing
        // intermediate certificate and a recoverable chain, but on demand cert loading
        // was disabled by setCaCertificates call. For this scenario we check if "Authority
        // Information Access" is present - wincrypt can deal with such certificates.
        QSslCertificate certToFetch;
        if (doVerifyPeer && !d->verifyErrorsHaveBeenIgnored())
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
#endif // Q_OS_WIN
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

void TlsCryptographOpenSSL::enableHandshakeContinuation()
{
    handshakeInterrupted = false;
}

void TlsCryptographOpenSSL::cancelCAFetch()
{
    fetchAuthorityInformation = false;
    caToFetch.reset();
}

void TlsCryptographOpenSSL::continueHandshake()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    const auto mode = d->tlsMode();

    // if we have a max read buffer size, reset the plain socket's to match
    if (const auto maxSize = d->maxReadBufferSize())
        plainSocket->setReadBufferSize(maxSize);

    if (q_SSL_session_reused(ssl))
        QTlsBackend::setPeerSessionShared(d, true);

#ifdef QT_DECRYPT_SSL_TRAFFIC
    if (q_SSL_get_session(ssl)) {
        size_t master_key_len = q_SSL_SESSION_get_master_key(q_SSL_get_session(ssl), nullptr, 0);
        size_t client_random_len = q_SSL_get_client_random(ssl, nullptr, 0);
        QByteArray masterKey(int(master_key_len), Qt::Uninitialized); // Will not overflow
        QByteArray clientRandom(int(client_random_len), Qt::Uninitialized); // Will not overflow

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

        QString sslKeyFile = QDir::tempPath() + "/qt-ssl-keys"_L1;
        QFile file(sslKeyFile);
        if (!file.open(QIODevice::Append))
            qCWarning(lcTlsBackend) << "could not open file" << sslKeyFile << "for appending";
        if (!file.write(debugLineClientRandom))
            qCWarning(lcTlsBackend) << "could not write to file" << sslKeyFile;
        file.close();
    } else {
        qCWarning(lcTlsBackend, "could not decrypt SSL traffic");
    }
#endif // QT_DECRYPT_SSL_TRAFFIC

    const auto &configuration = q->sslConfiguration();
    // Cache this SSL session inside the QSslContext
    if (!(configuration.testSslOption(QSsl::SslOptionDisableSessionSharing))) {
        if (!sslContextPointer->cacheSession(ssl)) {
            sslContextPointer.reset(); // we could not cache the session
        } else {
            // Cache the session for permanent usage as well
            if (!(configuration.testSslOption(QSsl::SslOptionDisableSessionPersistence))) {
                if (!sslContextPointer->sessionASN1().isEmpty())
                    QTlsBackend::setSessionAsn1(d, sslContextPointer->sessionASN1());
                QTlsBackend::setSessionLifetimeHint(d, sslContextPointer->sessionTicketLifeTimeHint());
            }
        }
    }

#if !defined(OPENSSL_NO_NEXTPROTONEG)

    QTlsBackend::setAlpnStatus(d, sslContextPointer->npnContext().status);
    if (sslContextPointer->npnContext().status == QSslConfiguration::NextProtocolNegotiationUnsupported) {
        // we could not agree -> be conservative and use HTTP/1.1
        // T.P.: I have to admit, this is a really strange notion of 'conservative',
        // given the protocol-neutral nature of ALPN/NPN.
        QTlsBackend::setNegotiatedProtocol(d, QByteArrayLiteral("http/1.1"));
    } else {
        const unsigned char *proto = nullptr;
        unsigned int proto_len = 0;

        q_SSL_get0_alpn_selected(ssl, &proto, &proto_len);
        if (proto_len && mode == QSslSocket::SslClientMode) {
            // Client does not have a callback that sets it ...
            QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationNegotiated);
        }

        if (!proto_len) { // Test if NPN was more lucky ...
            q_SSL_get0_next_proto_negotiated(ssl, &proto, &proto_len);
        }

        if (proto_len)
            QTlsBackend::setNegotiatedProtocol(d, QByteArray(reinterpret_cast<const char *>(proto), proto_len));
        else
            QTlsBackend::setNegotiatedProtocol(d,{});
    }
#endif // !defined(OPENSSL_NO_NEXTPROTONEG)

    if (mode == QSslSocket::SslClientMode) {
        EVP_PKEY *key;
        if (q_SSL_get_server_tmp_key(ssl, &key))
            QTlsBackend::setEphemeralKey(d, QSslKey(key, QSsl::PublicKey));
    }

    d->setEncrypted(true);
    emit q->encrypted();
    if (d->isAutoStartingHandshake() && d->isPendingClose()) {
        d->setPendingClose(false);
        q->disconnectFromHost();
    }
}

void TlsCryptographOpenSSL::transmit()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    using ScopedBool = QScopedValueRollback<bool>;

    if (inSetAndEmitError)
        return;

    // If we don't have any SSL context, don't bother transmitting.
    if (!ssl)
        return;

    auto &writeBuffer = d->tlsWriteBuffer();
    auto &buffer = d->tlsBuffer();
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    bool &emittedBytesWritten = d->tlsEmittedBytesWritten();

    bool transmitting;
    do {
        transmitting = false;

        // If the connection is secure, we can transfer data from the write
        // buffer (in plain text) to the write BIO through SSL_write.
        if (q->isEncrypted() && !writeBuffer.isEmpty()) {
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
                        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                        QSslSocket::tr("Unable to write data: %1").arg(
                                        QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
                        return;
                    }
                }
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: encrypted" << writtenBytes << "bytes";
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
            qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: wrote" << encryptedBytesRead
                                  << "encrypted bytes to the socket" << actualWritten << "actual.";
#endif
            if (actualWritten < 0) {
                //plain socket write fails if it was in the pending close state.
                const ScopedBool bg(inSetAndEmitError, true);
                setErrorAndEmit(d, plainSocket->error(), plainSocket->errorString());
                return;
            }
            transmitting = true;
        }

        // Check if we've got any data to be read from the socket.
        if (!q->isEncrypted() || !d->maxReadBufferSize() || buffer.size() < d->maxReadBufferSize())
            while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
                // Read encrypted data from the socket into a buffer.
                data.resize(pendingBytes);
                // just peek() here because q_BIO_write could write less data than expected
                int encryptedBytesRead = plainSocket->peek(data.data(), pendingBytes);

#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: read" << encryptedBytesRead << "encrypted bytes from the socket";
#endif
                // Write encrypted data from the buffer into the read BIO.
                int writtenToBio = q_BIO_write(readBio, data.constData(), encryptedBytesRead);

                // Throw away the results.
                if (writtenToBio > 0) {
                    plainSocket->skip(writtenToBio);
                } else {
                    // ### Better error handling.
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Unable to decrypt data: %1")
                                    .arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
                    return;
                }

                transmitting = true;
            }

        // If the connection isn't secured yet, this is the time to retry the
        // connect / accept.
        if (!q->isEncrypted()) {
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: testing encryption";
#endif
            if (startHandshake()) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: encryption established";
#endif
                d->setEncrypted(true);
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: connection lost";
#endif
                break;
            } else if (d->isPaused()) {
                // just wait until the user continues
                return;
            } else {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: encryption not done yet";
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
            if (q->readChannelCount() == 0) {
                // The read buffer is deallocated, don't try resize or write to it.
                break;
            }
            // Don't use SSL_pending(). It's very unreliable.
            inSslRead = true;
            readBytes = q_SSL_read(ssl, buffer.reserve(bytesToRead), bytesToRead);
            inSslRead = false;
            if (renegotiated) {
                renegotiated = false;
                X509 *x509 = q_SSL_get_peer_certificate(ssl);
                const auto peerCertificate =
                        QTlsPrivate::X509CertificateOpenSSL::certificateFromX509(x509);
                // Fail the renegotiate if the certificate has changed, else: continue.
                if (peerCertificate != q->peerCertificate()) {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(
                            d, QAbstractSocket::RemoteHostClosedError,
                            QSslSocket::tr(
                                    "TLS certificate unexpectedly changed during renegotiation!"));
                    q->abort();
                    return;
                }
            }
            if (readBytes > 0) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: decrypted" << readBytes << "bytes";
#endif
                buffer.chop(bytesToRead - readBytes);

                if (bool *readyReadEmittedPointer = d->readyReadPointer())
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
                qCDebug(lcTlsBackend) << "TlsCryptographOpenSSL::transmit: remote disconnect";
#endif
                shutdown = true; // the other side shut down, make sure we do not send shutdown ourselves
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(d, QAbstractSocket::RemoteHostClosedError,
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
                    setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Error while reading: %1")
                                    .arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
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
                    setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Error while reading: %1")
                                    .arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
                }
                break;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && transmitting);
}

void TlsCryptographOpenSSL::disconnectFromHost()
{
    if (ssl) {
        if (!shutdown && !q_SSL_in_init(ssl) && !systemOrSslErrorDetected) {
            if (q_SSL_shutdown(ssl) != 1) {
                // Some error may be queued, clear it.
                QTlsBackendOpenSSL::clearErrorQueue();
            }
            shutdown = true;
            transmit();
        }
    }
    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    plainSocket->disconnectFromHost();
}

void TlsCryptographOpenSSL::disconnected()
{
    Q_ASSERT(d);
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    d->setEncrypted(false);

    if (plainSocket->bytesAvailable() <= 0) {
        destroySslContext();
    } else {
        // Move all bytes into the plain buffer.
        const qint64 tmpReadBufferMaxSize = d->maxReadBufferSize();
        // Reset temporarily, so the plain socket buffer is completely drained:
        d->setMaxReadBufferSize(0);
        transmit();
        d->setMaxReadBufferSize(tmpReadBufferMaxSize);
    }
    //if there is still buffered data in the plain socket, don't destroy the ssl context yet.
    //it will be destroyed when the socket is deleted.
}

QSslCipher TlsCryptographOpenSSL::sessionCipher() const
{
    if (!ssl)
        return {};

    const SSL_CIPHER *sessionCipher = q_SSL_get_current_cipher(ssl);
    return sessionCipher ? QTlsBackendOpenSSL::qt_OpenSSL_cipher_to_QSslCipher(sessionCipher) : QSslCipher{};
}

QSsl::SslProtocol TlsCryptographOpenSSL::sessionProtocol() const
{
    if (!ssl)
        return QSsl::UnknownProtocol;

    const int ver = q_SSL_version(ssl);
    switch (ver) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case 0x301:
        return QSsl::TlsV1_0;
    case 0x302:
        return QSsl::TlsV1_1;
QT_WARNING_POP
    case 0x303:
        return QSsl::TlsV1_2;
    case 0x304:
        return QSsl::TlsV1_3;
    }

    return QSsl::UnknownProtocol;
}

QList<QOcspResponse> TlsCryptographOpenSSL::ocsps() const
{
    return ocspResponses;
}

bool TlsCryptographOpenSSL::checkSslErrors()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    if (sslErrors.isEmpty())
        return true;

    emit q->sslErrors(sslErrors);

    const auto vfyMode = q->peerVerifyMode();
    const auto mode = d->tlsMode();

    bool doVerifyPeer = vfyMode == QSslSocket::VerifyPeer || (vfyMode == QSslSocket::AutoVerifyPeer
                                                               && mode == QSslSocket::SslClientMode);
    bool doEmitSslError = !d->verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            QSslSocketPrivate::pauseSocketNotifiers(q);
            d->setPaused(true);
        } else {
            setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError, sslErrors.constFirst().errorString());
            auto *plainSocket = d->plainTcpSocket();
            Q_ASSERT(plainSocket);
            plainSocket->disconnectFromHost();
        }
        return false;
    }
    return true;
}

int TlsCryptographOpenSSL::handleNewSessionTicket(SSL *connection)
{
    // If we return 1, this means we own the session, but we don't.
    // 0 would tell OpenSSL to deref (but they still have it in the
    // internal cache).
    Q_ASSERT(connection);

    Q_ASSERT(q);
    Q_ASSERT(d);

    if (q->sslConfiguration().testSslOption(QSsl::SslOptionDisableSessionPersistence)) {
        // We silently ignore, do nothing, remove from cache.
        return 0;
    }

    SSL_SESSION *currentSession = q_SSL_get_session(connection);
    if (!currentSession) {
        qCWarning(lcTlsBackend,
                  "New session ticket callback, the session is invalid (nullptr)");
        return 0;
    }

    if (q_SSL_version(connection) < 0x304) {
        // We only rely on this mechanics with TLS >= 1.3
        return 0;
    }

#ifdef TLS1_3_VERSION
    if (!q_SSL_SESSION_is_resumable(currentSession)) {
        qCDebug(lcTlsBackend, "New session ticket, but the session is non-resumable");
        return 0;
    }
#endif // TLS1_3_VERSION

    const int sessionSize = q_i2d_SSL_SESSION(currentSession, nullptr);
    if (sessionSize <= 0) {
        qCWarning(lcTlsBackend, "could not store persistent version of SSL session");
        return 0;
    }

    // We have somewhat perverse naming, it's not a ticket, it's a session.
    QByteArray sessionTicket(sessionSize, 0);
    auto data = reinterpret_cast<unsigned char *>(sessionTicket.data());
    if (!q_i2d_SSL_SESSION(currentSession, &data)) {
        qCWarning(lcTlsBackend, "could not store persistent version of SSL session");
        return 0;
    }

    QTlsBackend::setSessionAsn1(d, sessionTicket);
    QTlsBackend::setSessionLifetimeHint(d, q_SSL_SESSION_get_ticket_lifetime_hint(currentSession));

    emit q->newSessionTicketReceived();
    return 0;
}

void TlsCryptographOpenSSL::alertMessageSent(int value)
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    const auto level = tlsAlertLevel(value);
    if (level == QSsl::AlertLevel::Fatal && !q->isEncrypted()) {
        // Note, this logic is handshake-time only:
        pendingFatalAlert = true;
    }

    emit q->alertSent(level, tlsAlertType(value), tlsAlertDescription(value));

}

void TlsCryptographOpenSSL::alertMessageReceived(int value)
{
    Q_ASSERT(q);

    emit q->alertReceived(tlsAlertLevel(value), tlsAlertType(value), tlsAlertDescription(value));
}

int TlsCryptographOpenSSL::emitErrorFromCallback(X509_STORE_CTX *ctx)
{
    // Returns 0 to abort verification, 1 to continue despite error (as
    // OpenSSL expects from the verification callback).
    Q_ASSERT(q);
    Q_ASSERT(ctx);

    using ScopedBool = QScopedValueRollback<bool>;
    // While we are not setting, we are emitting and in general -
    // we want to prevent accidental recursive startHandshake()
    // calls:
    const ScopedBool bg(inSetAndEmitError, true);

    X509 *x509 = q_X509_STORE_CTX_get_current_cert(ctx);
    if (!x509) {
        qCWarning(lcTlsBackend, "Could not obtain the certificate (that failed to verify)");
        return 0;
    }

    const QSslCertificate certificate = QTlsPrivate::X509CertificateOpenSSL::certificateFromX509(x509);
    const auto errorAndDepth = QTlsPrivate::X509CertificateOpenSSL::errorEntryFromStoreContext(ctx);
    const QSslError tlsError = QTlsPrivate::X509CertificateOpenSSL::openSSLErrorToQSslError(errorAndDepth.code, certificate);

    errorsReportedFromCallback = true;
    handshakeInterrupted = true;
    emit q->handshakeInterruptedOnError(tlsError);

    // Conveniently so, we also can access 'lastErrors' external data set
    // in startHandshake, we store it for the case an application later
    // wants to check errors (ignored or not):
    const auto offset = QTlsBackendOpenSSL::s_indexForSSLExtraData
                        + TlsCryptographOpenSSL::errorOffsetInExData;
    if (auto errorList = static_cast<QList<QSslErrorEntry> *>(q_SSL_get_ex_data(ssl, offset)))
        errorList->append(errorAndDepth);

    // An application is expected to ignore this error (by calling ignoreSslErrors)
    // in its directly connected slot:
    return !handshakeInterrupted;
}

void TlsCryptographOpenSSL::trySendFatalAlert()
{
    Q_ASSERT(pendingFatalAlert);
    Q_ASSERT(d);

    auto *plainSocket = d->plainTcpSocket();

    pendingFatalAlert = false;
    QVarLengthArray<char, 4096> data;
    int pendingBytes = 0;
    while (plainSocket->isValid() && (pendingBytes = q_BIO_pending(writeBio)) > 0
           && plainSocket->openMode() != QIODevice::NotOpen) {
        // Read encrypted data from the write BIO into a buffer.
        data.resize(pendingBytes);
        const int bioReadBytes = q_BIO_read(writeBio, data.data(), pendingBytes);

        // Write encrypted data from the buffer to the socket.
        qint64 actualWritten = plainSocket->write(data.constData(), bioReadBytes);
        if (actualWritten < 0)
            return;
        plainSocket->flush();
    }
}

bool TlsCryptographOpenSSL::initSslContext()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    // If no external context was set (e.g. by QHttpNetworkConnection) we will
    // create a new one.
    const auto mode = d->tlsMode();
    const auto configuration = q->sslConfiguration();
    if (!sslContextPointer)
        sslContextPointer = QSslContext::sharedFromConfiguration(mode, configuration, d->isRootsOnDemandAllowed());

    if (sslContextPointer->error() != QSslError::NoError) {
        setErrorAndEmit(d, QAbstractSocket::SslInvalidUserDataError, sslContextPointer->errorString());
        sslContextPointer.reset();
        return false;
    }

    // Create and initialize SSL session
    if (!(ssl = sslContextPointer->createSsl())) {
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL session, %1").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
        return false;
    }

    if (configuration.protocol() != QSsl::UnknownProtocol && mode == QSslSocket::SslClientMode) {
        const auto verificationPeerName = d->verificationName();
        // Set server hostname on TLS extension. RFC4366 section 3.1 requires it in ACE format.
        QString tlsHostName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
        if (tlsHostName.isEmpty())
            tlsHostName = d->tlsHostName();
        QByteArray ace = QUrl::toAce(tlsHostName);
        // only send the SNI header if the URL is valid and not an IP
        if (!ace.isEmpty()
            && !QHostAddress().setAddress(tlsHostName)
            && !(configuration.testSslOption(QSsl::SslOptionDisableServerNameIndication))) {
            // We don't send the trailing dot from the host header if present see
            // https://tools.ietf.org/html/rfc6066#section-3
            if (ace.endsWith('.'))
                ace.chop(1);
            if (!q_SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, ace.data()))
                qCWarning(lcTlsBackend, "could not set SSL_CTRL_SET_TLSEXT_HOSTNAME, Server Name Indication disabled");
        }
    }

    // Clear the session.
    errorList.clear();

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL session: %1").arg(QTlsBackendOpenSSL::getErrorsFromOpenSsl()));
        if (readBio)
            q_BIO_free(readBio);
        if (writeBio)
            q_BIO_free(writeBio);
        return false;
    }

    // Assign the bios.
    q_SSL_set_bio(ssl, readBio, writeBio);

    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_connect_state(ssl);
    else
        q_SSL_set_accept_state(ssl);

    q_SSL_set_ex_data(ssl, QTlsBackendOpenSSL::s_indexForSSLExtraData, this);

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
    if (configuration.ocspStaplingEnabled()) {
        if (mode == QSslSocket::SslServerMode) {
            setErrorAndEmit(d, QAbstractSocket::SslInvalidUserDataError,
                            QSslSocket::tr("Server-side QSslSocket does not support OCSP stapling"));
            return false;
        }
        if (q_SSL_set_tlsext_status_type(ssl, TLSEXT_STATUSTYPE_ocsp) != 1) {
            setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                            QSslSocket::tr("Failed to enable OCSP stapling"));
            return false;
        }
    }

    ocspResponseDer.clear();
    const auto backendConfig = configuration.backendConfiguration();
    auto responsePos = backendConfig.find("Qt-OCSP-response");
    if (responsePos != backendConfig.end()) {
        // This is our private, undocumented 'API' we use for the auto-testing of
        // OCSP-stapling. It must be a der-encoded OCSP response, presumably set
        // by tst_QOcsp.
        const QVariant data(responsePos.value());
        if (data.canConvert<QByteArray>())
            ocspResponseDer = data.toByteArray();
    }

    if (ocspResponseDer.size()) {
        if (mode != QSslSocket::SslServerMode) {
            setErrorAndEmit(d, QAbstractSocket::SslInvalidUserDataError,
                            QSslSocket::tr("Client-side sockets do not send OCSP responses"));
            return false;
        }
    }
#endif // ocsp

    return true;
}

void TlsCryptographOpenSSL::destroySslContext()
{
    if (ssl) {
        if (!q_SSL_in_init(ssl) && !systemOrSslErrorDetected) {
            // We do not send a shutdown alert here. Just mark the session as
            // resumable for qhttpnetworkconnection's "optimization", otherwise
            // OpenSSL won't start a session resumption.
            if (q_SSL_shutdown(ssl) != 1) {
                // Some error may be queued, clear it.
                const auto errors = QTlsBackendOpenSSL::getErrorsFromOpenSsl();
                Q_UNUSED(errors);
            }
        }
        q_SSL_free(ssl);
        ssl = nullptr;
    }
    sslContextPointer.reset();
}

void TlsCryptographOpenSSL::storePeerCertificates()
{
    Q_ASSERT(d);

    // Store the peer certificate and chain. For clients, the peer certificate
    // chain includes the peer certificate; for servers, it doesn't. Both the
    // peer certificate and the chain may be empty if the peer didn't present
    // any certificate.
    X509 *x509 = q_SSL_get_peer_certificate(ssl);

    const auto peerCertificate = QTlsPrivate::X509CertificateOpenSSL::certificateFromX509(x509);
    QTlsBackend::storePeerCertificate(d, peerCertificate);
    q_X509_free(x509);
    auto peerCertificateChain = q->peerCertificateChain();
    if (peerCertificateChain.isEmpty()) {
        peerCertificateChain = QTlsPrivate::X509CertificateOpenSSL::stackOfX509ToQSslCertificates(q_SSL_get_peer_cert_chain(ssl));
        if (!peerCertificate.isNull() && d->tlsMode() == QSslSocket::SslServerMode)
            peerCertificateChain.prepend(peerCertificate);
        QTlsBackend::storePeerCertificateChain(d, peerCertificateChain);
    }
}

#if QT_CONFIG(ocsp)

bool TlsCryptographOpenSSL::checkOcspStatus()
{
    Q_ASSERT(ssl);
    Q_ASSERT(d);

    const auto &configuration = q->sslConfiguration();
    Q_ASSERT(d->tlsMode() == QSslSocket::SslClientMode); // See initSslContext() for SslServerMode
    Q_ASSERT(configuration.peerVerifyMode() != QSslSocket::VerifyNone);

    const auto clearErrorQueue = qScopeGuard([] {
        QTlsBackendOpenSSL::logAndClearErrorQueue();
    });

    ocspResponses.clear();
    ocspErrorDescription.clear();
    ocspErrors.clear();

    const unsigned char *responseData = nullptr;
    const long responseLength = q_SSL_get_tlsext_status_ocsp_resp(ssl, &responseData);
    if (responseLength <= 0 || !responseData) {
        ocspErrors.push_back(QSslError(QSslError::OcspNoResponseFound));
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
        ocspErrors.push_back(QSslError(qt_OCSP_response_status_to_SslError(ocspStatus)));
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
        ocspErrors.push_back(QSslError(QSslError::OcspResponseCannotBeTrusted));

    if (q_OCSP_resp_count(basicResponse) != 1) {
        ocspErrors.push_back(QSslError(QSslError::OcspMalformedResponse));
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
    dResponse->subjectCert = configuration.peerCertificate();
    bool matchFound = false;
    if (dResponse->subjectCert.isSelfSigned()) {
        dResponse->signerCert = configuration.peerCertificate();
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
                        dResponse->signerCert =  QTlsPrivate::X509CertificateOpenSSL::certificateFromX509(issuer);
                        break;
                    }
                    matchFound = false;
                }
            }
        }
    }

    if (!matchFound) {
        dResponse->signerCert.clear();
        ocspErrors.push_back({QSslError::OcspResponseCertIdUnknown, configuration.peerCertificate()});
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
        ocspErrors.push_back({QSslError::OcspResponseExpired, configuration.peerCertificate()});

    // And finally, the status:
    switch (certStatus) {
    case V_OCSP_CERTSTATUS_GOOD:
        // This certificate was not found among the revoked ones.
        dResponse->certificateStatus = QOcspCertificateStatus::Good;
        break;
    case V_OCSP_CERTSTATUS_REVOKED:
        dResponse->certificateStatus = QOcspCertificateStatus::Revoked;
        dResponse->revocationReason = qt_OCSP_revocation_reason(reason);
        ocspErrors.push_back({QSslError::CertificateRevoked, configuration.peerCertificate()});
        break;
    case V_OCSP_CERTSTATUS_UNKNOWN:
        dResponse->certificateStatus = QOcspCertificateStatus::Unknown;
        ocspErrors.push_back({QSslError::OcspStatusUnknown, configuration.peerCertificate()});
    }

    return !ocspErrors.size();
}

#endif // QT_CONFIG(ocsp)


unsigned TlsCryptographOpenSSL::pskClientTlsCallback(const char *hint, char *identity,
                                                     unsigned max_identity_len,
                                                     unsigned char *psk, unsigned max_psk_len)
{
    Q_ASSERT(q);

    QSslPreSharedKeyAuthenticator authenticator;
    // Fill in some read-only fields (for the user)
    const int hintLength = hint ? int(std::strlen(hint)) : 0;
    QTlsBackend::setupClientPskAuth(&authenticator, hint, hintLength, max_identity_len, max_psk_len);
    // Let the client provide the remaining bits...
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int identityLength = qMin(authenticator.identity().size(), authenticator.maximumIdentityLength());
    std::memcpy(identity, authenticator.identity().constData(), identityLength);
    identity[identityLength] = 0;

    const int pskLength = qMin(authenticator.preSharedKey().size(), authenticator.maximumPreSharedKeyLength());
    std::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

unsigned TlsCryptographOpenSSL::pskServerTlsCallback(const char *identity, unsigned char *psk,
                                                     unsigned max_psk_len)
{
    Q_ASSERT(q);

    QSslPreSharedKeyAuthenticator authenticator;

    // Fill in some read-only fields (for the user)
    QTlsBackend::setupServerPskAuth(&authenticator, identity, q->sslConfiguration().preSharedKeyIdentityHint(),
                                    max_psk_len);
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int pskLength = qMin(authenticator.preSharedKey().size(), authenticator.maximumPreSharedKeyLength());
    std::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

bool TlsCryptographOpenSSL::isInSslRead() const
{
    return inSslRead;
}

void TlsCryptographOpenSSL::setRenegotiated(bool renegotiated)
{
    this->renegotiated = renegotiated;
}

#ifdef Q_OS_WIN

void TlsCryptographOpenSSL::fetchCaRootForCert(const QSslCertificate &cert)
{
    Q_ASSERT(d);
    Q_ASSERT(q);

    //The root certificate is downloaded from windows update, which blocks for 15 seconds in the worst case
    //so the request is done in a worker thread.
    QList<QSslCertificate> customRoots;
    if (fetchAuthorityInformation)
        customRoots = q->sslConfiguration().caCertificates();

    //Remember we are fetching and what we are fetching:
    caToFetch = cert;

    QWindowsCaRootFetcher *fetcher = new QWindowsCaRootFetcher(cert, d->tlsMode(), customRoots,
                                                               q->peerVerifyName());
    connect(fetcher,  &QWindowsCaRootFetcher::finished, this, &TlsCryptographOpenSSL::caRootLoaded,
            Qt::QueuedConnection);
    QMetaObject::invokeMethod(fetcher, "start", Qt::QueuedConnection);
    QSslSocketPrivate::pauseSocketNotifiers(q);
    d->setPaused(true);
}

void TlsCryptographOpenSSL::caRootLoaded(QSslCertificate cert, QSslCertificate trustedRoot)
{
    if (caToFetch != cert) {
        //Ooops, something from the previous connection attempt, ignore!
        return;
    }

    Q_ASSERT(d);
    Q_ASSERT(q);

    //Done, fetched already:
    caToFetch.reset();

    if (fetchAuthorityInformation) {
        if (!q->sslConfiguration().caCertificates().contains(trustedRoot))
            trustedRoot = QSslCertificate{};
        fetchAuthorityInformation = false;
    }

    if (!trustedRoot.isNull() && !trustedRoot.isBlacklisted()) {
        if (QSslSocketPrivate::rootCertOnDemandLoadingSupported()) {
            //Add the new root cert to default cert list for use by future sockets
            auto defaultConfig = QSslConfiguration::defaultConfiguration();
            defaultConfig.addCaCertificate(trustedRoot);
            QSslConfiguration::setDefaultConfiguration(defaultConfig);
        }
        //Add the new root cert to this socket for future connections
        QTlsBackend::addTustedRoot(d, trustedRoot);
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

    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    // Continue with remaining errors
    if (plainSocket)
        plainSocket->resume();
    d->setPaused(false);
    if (checkSslErrors() && ssl) {
        bool willClose = (d->isAutoStartingHandshake() && d->isPendingClose());
        continueHandshake();
        if (!willClose)
            transmit();
    }
}

#endif // Q_OS_WIN

} // namespace QTlsPrivate

QT_END_NAMESPACE
