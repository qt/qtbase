// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsslsocket_openssl_symbols_p.h"
#include "qtlsbackend_openssl_p.h"
#include "qtlskey_openssl_p.h"
#include "qx509_openssl_p.h"
#include "qtls_openssl_p.h"

#include <QtNetwork/private/qsslcertificate_p.h>

#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qhostaddress.h>

#include <QtCore/qendian.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qhash.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtimezone.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QTlsPrivate {

namespace {

QByteArray asn1ObjectId(ASN1_OBJECT *object)
{
    if (!object)
        return {};

    char buf[80] = {}; // The openssl docs a buffer length of 80 should be more than enough
    q_OBJ_obj2txt(buf, sizeof(buf), object, 1); // the 1 says always use the oid not the long name

    return QByteArray(buf);
}

QByteArray asn1ObjectName(ASN1_OBJECT *object)
{
    if (!object)
        return {};

    const int nid = q_OBJ_obj2nid(object);
    if (nid != NID_undef)
        return QByteArray(q_OBJ_nid2sn(nid));

    return asn1ObjectId(object);
}

QMultiMap<QByteArray, QString> mapFromX509Name(X509_NAME *name)
{
    if (!name)
        return {};

    QMultiMap<QByteArray, QString> info;
    for (int i = 0; i < q_X509_NAME_entry_count(name); ++i) {
        X509_NAME_ENTRY *e = q_X509_NAME_get_entry(name, i);

        QByteArray name = asn1ObjectName(q_X509_NAME_ENTRY_get_object(e));
        unsigned char *data = nullptr;
        int size = q_ASN1_STRING_to_UTF8(&data, q_X509_NAME_ENTRY_get_data(e));
        info.insert(name, QString::fromUtf8((char*)data, size));
        q_CRYPTO_free(data, nullptr, 0);
    }

    return info;
}

QDateTime dateTimeFromASN1(const ASN1_TIME *aTime)
{
    QDateTime result;
    tm lTime;

    if (q_ASN1_TIME_to_tm(aTime, &lTime)) {
        QDate resDate(lTime.tm_year + 1900, lTime.tm_mon + 1, lTime.tm_mday);
        QTime resTime(lTime.tm_hour, lTime.tm_min, lTime.tm_sec);
        result = QDateTime(resDate, resTime, QTimeZone::UTC);
    }

    return result;
}


#define BEGINCERTSTRING "-----BEGIN CERTIFICATE-----"
#define ENDCERTSTRING "-----END CERTIFICATE-----"

QByteArray x509ToQByteArray(X509 *x509, QSsl::EncodingFormat format)
{
    Q_ASSERT(x509);

    // Use i2d_X509 to convert the X509 to an array.
    const int length = q_i2d_X509(x509, nullptr);
    if (length <= 0) {
        QTlsBackendOpenSSL::logAndClearErrorQueue();
        return {};
    }

    QByteArray array;
    array.resize(length);

    char *data = array.data();
    char **dataP = &data;
    unsigned char **dataPu = (unsigned char **)dataP;
    if (q_i2d_X509(x509, dataPu) < 0)
        return QByteArray();

    if (format == QSsl::Der)
        return array;

    // Convert to Base64 - wrap at 64 characters.
    array = array.toBase64();
    QByteArray tmp;
    for (int i = 0; i <= array.size() - 64; i += 64) {
        tmp += QByteArray::fromRawData(array.data() + i, 64);
        tmp += '\n';
    }
    if (int remainder = array.size() % 64) {
        tmp += QByteArray::fromRawData(array.data() + array.size() - remainder, remainder);
        tmp += '\n';
    }

    return BEGINCERTSTRING "\n" + tmp + ENDCERTSTRING "\n";
}

QString x509ToText(X509 *x509)
{
    Q_ASSERT(x509);

    QByteArray result;
    BIO *bio = q_BIO_new(q_BIO_s_mem());
    if (!bio)
        return QString();
    const auto bioRaii = qScopeGuard([bio]{q_BIO_free(bio);});

    q_X509_print(bio, x509);

    QVarLengthArray<char, 16384> data;
    int count = q_BIO_read(bio, data.data(), 16384);
    if ( count > 0 )
        result = QByteArray( data.data(), count );

    return QString::fromLatin1(result);
}

QVariant x509UnknownExtensionToValue(X509_EXTENSION *ext)
{
    // Get the extension specific method object if available
    // we cast away the const-ness here because some versions of openssl
    // don't use const for the parameters in the functions pointers stored
    // in the object.
    Q_ASSERT(ext);

    X509V3_EXT_METHOD *meth = const_cast<X509V3_EXT_METHOD *>(q_X509V3_EXT_get(ext));
    if (!meth) {
        ASN1_OCTET_STRING *value = q_X509_EXTENSION_get_data(ext);
        Q_ASSERT(value);
        QByteArray result( reinterpret_cast<const char *>(q_ASN1_STRING_get0_data(value)),
                           q_ASN1_STRING_length(value));
        return result;
    }

    void *ext_internal = q_X509V3_EXT_d2i(ext);
    if (!ext_internal)
        return {};

    const auto extCleaner = qScopeGuard([meth, ext_internal]{
        Q_ASSERT(ext_internal && meth);

        if (meth->it)
            q_ASN1_item_free(static_cast<ASN1_VALUE *>(ext_internal), ASN1_ITEM_ptr(meth->it));
        else if (meth->ext_free)
            meth->ext_free(ext_internal);
        else
            qCWarning(lcTlsBackend, "No method to free an unknown extension, a potential memory leak?");
    });

    // If this extension can be converted
    if (meth->i2v) {
        STACK_OF(CONF_VALUE) *val = meth->i2v(meth, ext_internal, nullptr);
        const auto stackCleaner = qScopeGuard([val]{
            if (val)
                q_OPENSSL_sk_pop_free((OPENSSL_STACK *)val, (void(*)(void*))q_X509V3_conf_free);
        });

        QVariantMap map;
        QVariantList list;
        bool isMap = false;

        for (int j = 0; j < q_SKM_sk_num(val); j++) {
            CONF_VALUE *nval = q_SKM_sk_value(CONF_VALUE, val, j);
            if (nval->name && nval->value) {
                isMap = true;
                map[QString::fromUtf8(nval->name)] = QString::fromUtf8(nval->value);
            } else if (nval->name) {
                list << QString::fromUtf8(nval->name);
            } else if (nval->value) {
                list << QString::fromUtf8(nval->value);
            }
        }

        if (isMap)
            return map;
        else
            return list;
    } else if (meth->i2s) {
        const char *hexString = meth->i2s(meth, ext_internal);
        QVariant result(hexString ? QString::fromUtf8(hexString) : QString{});
        q_OPENSSL_free((void *)hexString);
        return result;
    } else if (meth->i2r) {
        QByteArray result;

        BIO *bio = q_BIO_new(q_BIO_s_mem());
        if (!bio)
            return result;

        meth->i2r(meth, ext_internal, bio, 0);

        char *bio_buffer;
        long bio_size = q_BIO_get_mem_data(bio, &bio_buffer);
        result = QByteArray(bio_buffer, bio_size);

        q_BIO_free(bio);
        return result;
    }

    return QVariant();
}

/*
 * Convert extensions to a variant. The naming of the keys of the map are
 * taken from RFC 5280, however we decided the capitalisation in the RFC
 * was too silly for the real world.
 */
QVariant x509ExtensionToValue(X509_EXTENSION *ext)
{
    ASN1_OBJECT *obj = q_X509_EXTENSION_get_object(ext);
    int nid = q_OBJ_obj2nid(obj);

    // We cast away the const-ness here because some versions of openssl
    // don't use const for the parameters in the functions pointers stored
    // in the object.
    X509V3_EXT_METHOD *meth = const_cast<X509V3_EXT_METHOD *>(q_X509V3_EXT_get(ext));

    void *ext_internal = nullptr; // The value, returned by X509V3_EXT_d2i.
    const auto extCleaner = qScopeGuard([meth, &ext_internal]() {
        if (!meth || !ext_internal)
            return;

        if (meth->it)
            q_ASN1_item_free(static_cast<ASN1_VALUE *>(ext_internal), ASN1_ITEM_ptr(meth->it));
        else if (meth->ext_free)
            meth->ext_free(ext_internal);
        else
            qWarning(lcTlsBackend, "Cannot free an extension, a potential memory leak?");
    });

    const char * hexString = nullptr; // The value returned by meth->i2s.
    const auto hexStringCleaner = qScopeGuard([&hexString](){
        if (hexString)
            q_OPENSSL_free((void*)hexString);
    });

    switch (nid) {
    case NID_basic_constraints:
        {
            BASIC_CONSTRAINTS *basic = reinterpret_cast<BASIC_CONSTRAINTS *>(q_X509V3_EXT_d2i(ext));
            if (!basic)
                return {};
            QVariantMap result;
            result["ca"_L1] = basic->ca ? true : false;
            if (basic->pathlen)
                result["pathLenConstraint"_L1] = (qlonglong)q_ASN1_INTEGER_get(basic->pathlen);

            q_BASIC_CONSTRAINTS_free(basic);
            return result;
        }
        break;
    case NID_info_access:
        {
            AUTHORITY_INFO_ACCESS *info = reinterpret_cast<AUTHORITY_INFO_ACCESS *>(q_X509V3_EXT_d2i(ext));
            if (!info)
                return {};
            QVariantMap result;
            for (int i=0; i < q_SKM_sk_num(info); i++) {
                ACCESS_DESCRIPTION *ad = q_SKM_sk_value(ACCESS_DESCRIPTION, info, i);

                GENERAL_NAME *name = ad->location;
                if (name->type == GEN_URI) {
                    int len = q_ASN1_STRING_length(name->d.uniformResourceIdentifier);
                    if (len < 0 || len >= 8192) {
                        // broken name
                        continue;
                    }

                    const char *uriStr = reinterpret_cast<const char *>(q_ASN1_STRING_get0_data(name->d.uniformResourceIdentifier));
                    const QString uri = QString::fromUtf8(uriStr, len);

                    result[QString::fromUtf8(asn1ObjectName(ad->method))] = uri;
                } else {
                   qCWarning(lcTlsBackend) << "Strange location type" << name->type;
                }
            }

            q_AUTHORITY_INFO_ACCESS_free(info);
            return result;
        }
        break;
    case NID_subject_key_identifier:
        {
            ext_internal = q_X509V3_EXT_d2i(ext);
            if (!ext_internal)
                return {};

            hexString = meth->i2s(meth, ext_internal);
            return QVariant(QString::fromUtf8(hexString));
        }
        break;
    case NID_authority_key_identifier:
        {
            AUTHORITY_KEYID *auth_key = reinterpret_cast<AUTHORITY_KEYID *>(q_X509V3_EXT_d2i(ext));
            if (!auth_key)
                return {};
            QVariantMap result;

            // keyid
            if (auth_key->keyid) {
                QByteArray keyid(reinterpret_cast<const char *>(auth_key->keyid->data),
                                 auth_key->keyid->length);
                result["keyid"_L1] = keyid.toHex();
            }

            // issuer
            // TODO: GENERAL_NAMES

            // serial
            if (auth_key->serial)
                result["serial"_L1] = (qlonglong)q_ASN1_INTEGER_get(auth_key->serial);

            q_AUTHORITY_KEYID_free(auth_key);
            return result;
        }
        break;
    }

    return {};
}

} // Unnamed namespace

extern "C" int qt_X509Callback(int ok, X509_STORE_CTX *ctx)
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

            // TLSTODO: verification callback has to change as soon as TlsCryptographer is in place.
            // This is a temporary solution for now to ease the transition.
            const auto offset = QTlsBackendOpenSSL::s_indexForSSLExtraData
                                + TlsCryptographOpenSSL::errorOffsetInExData;
            if (SSL *ssl = static_cast<SSL *>(q_X509_STORE_CTX_get_ex_data(ctx, q_SSL_get_ex_data_X509_STORE_CTX_idx())))
                errors = ErrorListPtr(q_SSL_get_ex_data(ssl, offset));
        }

        if (!errors) {
            qCWarning(lcTlsBackend, "Neither X509_STORE, nor SSL contains error list, verification failed");
            return 0;
        }

        errors->append(X509CertificateOpenSSL::errorEntryFromStoreContext(ctx));
    }
    // Always return OK to allow verification to continue. We handle the
    // errors gracefully after collecting all errors, after verification has
    // completed.
    return 1;
}

X509CertificateOpenSSL::X509CertificateOpenSSL() = default;

X509CertificateOpenSSL::~X509CertificateOpenSSL()
{
    if (x509)
        q_X509_free(x509);
}

bool X509CertificateOpenSSL::isEqual(const X509Certificate &rhs) const
{
    //TLSTODO: to make it safe I'll check the backend type later.
    const auto &other = static_cast<const X509CertificateOpenSSL &>(rhs);
    if (x509 && other.x509) {
        const int ret = q_X509_cmp(x509, other.x509);
        if (ret >= -1 && ret <= 1)
            return ret == 0;
        QTlsBackendOpenSSL::logAndClearErrorQueue();
    }

    return false;
}

bool X509CertificateOpenSSL::isSelfSigned() const
{
    if (!x509)
        return false;

    return q_X509_check_issued(x509, x509) == X509_V_OK;
}

QMultiMap<QSsl::AlternativeNameEntryType, QString>
X509CertificateOpenSSL::subjectAlternativeNames() const
{
    QMultiMap<QSsl::AlternativeNameEntryType, QString> result;

    if (!x509)
        return result;

    auto *altNames = static_cast<STACK_OF(GENERAL_NAME) *>(q_X509_get_ext_d2i(x509, NID_subject_alt_name,
                                                                              nullptr, nullptr));
    if (!altNames)
        return result;

    auto altName = [](ASN1_IA5STRING *ia5, int len) {
        const char *altNameStr = reinterpret_cast<const char *>(q_ASN1_STRING_get0_data(ia5));
        return QString::fromLatin1(altNameStr, len);
    };

    for (int i = 0; i < q_sk_GENERAL_NAME_num(altNames); ++i) {
        const GENERAL_NAME *genName = q_sk_GENERAL_NAME_value(altNames, i);
        if (genName->type != GEN_DNS && genName->type != GEN_EMAIL && genName->type != GEN_IPADD)
            continue;

        const int len = q_ASN1_STRING_length(genName->d.ia5);
        if (len < 0 || len >= 8192) {
            // broken name
            continue;
        }

        switch (genName->type) {
        case GEN_DNS:
            result.insert(QSsl::DnsEntry, altName(genName->d.ia5, len));
            break;
        case GEN_EMAIL:
            result.insert(QSsl::EmailEntry, altName(genName->d.ia5, len));
            break;
        case GEN_IPADD: {
            QHostAddress ipAddress;
            switch (len) {
            case 4: // IPv4
                ipAddress = QHostAddress(qFromBigEndian(*reinterpret_cast<quint32 *>(genName->d.iPAddress->data)));
                break;
            case 16: // IPv6
                ipAddress = QHostAddress(reinterpret_cast<quint8 *>(genName->d.iPAddress->data));
                break;
            default: // Unknown IP address format
                break;
            }
            if (!ipAddress.isNull())
                result.insert(QSsl::IpAddressEntry, ipAddress.toString());
            break;
        }
        default:
            break;
        }
    }

    q_OPENSSL_sk_pop_free((OPENSSL_STACK*)altNames, reinterpret_cast<void(*)(void*)>(q_GENERAL_NAME_free));

    return result;
}

TlsKey *X509CertificateOpenSSL::publicKey() const
{
    if (!x509)
        return {};

    return TlsKeyOpenSSL::publicKeyFromX509(x509);
}

QByteArray X509CertificateOpenSSL::toPem() const
{
    if (!x509)
        return {};

    return x509ToQByteArray(x509, QSsl::Pem);
}

QByteArray X509CertificateOpenSSL::toDer() const
{
    if (!x509)
        return {};

    return x509ToQByteArray(x509, QSsl::Der);

}
QString X509CertificateOpenSSL::toText() const
{
    if (!x509)
        return {};

    return x509ToText(x509);
}

Qt::HANDLE X509CertificateOpenSSL::handle() const
{
    return Qt::HANDLE(x509);
}

size_t X509CertificateOpenSSL::hash(size_t seed) const noexcept
{
    if (x509) {
        const EVP_MD *sha1 = q_EVP_sha1();
        unsigned int len = 0;
        unsigned char md[EVP_MAX_MD_SIZE];
        q_X509_digest(x509, sha1, md, &len);
        return qHashBits(md, len, seed);
    }

    return seed;
}

QSslCertificate X509CertificateOpenSSL::certificateFromX509(X509 *x509)
{
    QSslCertificate certificate;

    auto *backend = QTlsBackend::backend<X509CertificateOpenSSL>(certificate);
    if (!backend || !x509)
        return certificate;

    ASN1_TIME *nbef = q_X509_getm_notBefore(x509);
    if (nbef)
        backend->notValidBefore = dateTimeFromASN1(nbef);

    ASN1_TIME *naft = q_X509_getm_notAfter(x509);
    if (naft)
        backend->notValidAfter = dateTimeFromASN1(naft);

    backend->null = false;
    backend->x509 = q_X509_dup(x509);

    backend->issuerInfoEntries = mapFromX509Name(q_X509_get_issuer_name(x509));
    backend->subjectInfoEntries = mapFromX509Name(q_X509_get_subject_name(x509));
    backend->versionString = QByteArray::number(qlonglong(q_X509_get_version(x509)) + 1);

    if (ASN1_INTEGER *serialNumber = q_X509_get_serialNumber(x509)) {
        QByteArray hexString;
        hexString.reserve(serialNumber->length * 3);
        for (int a = 0; a < serialNumber->length; ++a) {
            hexString += QByteArray::number(serialNumber->data[a], 16).rightJustified(2, '0');
            hexString += ':';
        }
        hexString.chop(1);
        backend->serialNumberString = hexString;
    }

    backend->parseExtensions();

    return certificate;
}

QList<QSslCertificate> X509CertificateOpenSSL::stackOfX509ToQSslCertificates(STACK_OF(X509) *x509)
{
    if (!x509)
        return {};

    QList<QSslCertificate> certificates;
    for (int i = 0; i < q_sk_X509_num(x509); ++i) {
        if (X509 *entry = q_sk_X509_value(x509, i))
            certificates << certificateFromX509(entry);
    }

    return certificates;
}

QSslErrorEntry X509CertificateOpenSSL::errorEntryFromStoreContext(X509_STORE_CTX *ctx)
{
    Q_ASSERT(ctx);

    return {q_X509_STORE_CTX_get_error(ctx), q_X509_STORE_CTX_get_error_depth(ctx)};
}

QList<QSslError> X509CertificateOpenSSL::verify(const QList<QSslCertificate> &chain,
                                                const QString &hostName)
{
    // This was previously QSslSocketPrivate::verify().
    auto roots = QSslConfiguration::defaultConfiguration().caCertificates();
#ifndef Q_OS_WIN
    // On Windows, system CA certificates are already set as default ones.
    // No need to add them again (and again) and also, if the default configuration
    // has its own set of CAs, this probably should not be amended by the ones
    // from the 'ROOT' store, since it's not what an application chose to trust.
    if (QSslSocketPrivate::rootCertOnDemandLoadingSupported())
        roots.append(QSslSocketPrivate::systemCaCertificates());
#endif // Q_OS_WIN
    return verify(roots, chain, hostName);
}

QList<QSslError> X509CertificateOpenSSL::verify(const QList<QSslCertificate> &caCertificates,
                                                const QList<QSslCertificate> &certificateChain,
                                                const QString &hostName)
{
    // This was previously QSslSocketPrivate::verify().
    if (certificateChain.size() <= 0)
        return {QSslError(QSslError::UnspecifiedError)};

    QList<QSslError> errors;
    X509_STORE *certStore = q_X509_STORE_new();
    if (!certStore) {
        qCWarning(lcTlsBackend) << "Unable to create certificate store";
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
        // See also: QSslContext::sharedFromConfiguration()
        if (caCertificate.expiryDate() >= now) {
            q_X509_STORE_add_cert(certStore, reinterpret_cast<X509 *>(caCertificate.handle()));
        }
    }

    QList<QSslErrorEntry> lastErrors;
    if (!q_X509_STORE_set_ex_data(certStore, 0, &lastErrors)) {
        qCWarning(lcTlsBackend) << "Unable to attach external data (error list) to a store";
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Register a custom callback to get all verification errors.
    q_X509_STORE_set_verify_cb(certStore, qt_X509Callback);

    // Build the chain of intermediate certificates
    STACK_OF(X509) *intermediates = nullptr;
    if (certificateChain.size() > 1) {
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

    if (certificateChain[0].isBlacklisted())
        errors << QSslError(QSslError::CertificateBlacklisted, certificateChain[0]);

    // Check the certificate name against the hostname if one was specified
    if (!hostName.isEmpty() && !TlsCryptograph::isMatchingHostname(certificateChain[0], hostName)) {
        // No matches in common names or alternate names.
        QSslError error(QSslError::HostNameMismatch, certificateChain[0]);
        errors << error;
    }

    // Translate errors from the error list into QSslErrors.
    errors.reserve(errors.size() + lastErrors.size());
    for (const auto &error : std::as_const(lastErrors))
        errors << openSSLErrorToQSslError(error.code, certificateChain.value(error.depth));

    return errors;
}

QList<QSslCertificate> X509CertificateOpenSSL::certificatesFromPem(const QByteArray &pem, int count)
{
    QList<QSslCertificate> certificates;

    int offset = 0;
    while (count == -1 || certificates.size() < count) {
        int startPos = pem.indexOf(BEGINCERTSTRING, offset);
        if (startPos == -1)
            break;
        startPos += sizeof(BEGINCERTSTRING) - 1;
        if (!matchLineFeed(pem, &startPos))
            break;

        int endPos = pem.indexOf(ENDCERTSTRING, startPos);
        if (endPos == -1)
            break;

        offset = endPos + sizeof(ENDCERTSTRING) - 1;
        if (offset < pem.size() && !matchLineFeed(pem, &offset))
            break;

        QByteArray decoded = QByteArray::fromBase64(
            QByteArray::fromRawData(pem.data() + startPos, endPos - startPos));
        const unsigned char *data = (const unsigned char *)decoded.data();

        if (X509 *x509 = q_d2i_X509(nullptr, &data, decoded.size())) {
            certificates << certificateFromX509(x509);
            q_X509_free(x509);
        }
    }

    return certificates;
}

QList<QSslCertificate> X509CertificateOpenSSL::certificatesFromDer(const QByteArray &der, int count)
{
    QList<QSslCertificate> certificates;

    const unsigned char *data = (const unsigned char *)der.data();
    int size = der.size();

    while (size > 0 && (count == -1 || certificates.size() < count)) {
        if (X509 *x509 = q_d2i_X509(nullptr, &data, size)) {
            certificates << certificateFromX509(x509);
            q_X509_free(x509);
        } else {
            break;
        }
        size -= ((const char *)data - der.data());
    }

    return certificates;
}

bool X509CertificateOpenSSL::importPkcs12(QIODevice *device, QSslKey *key, QSslCertificate *cert,
                                          QList<QSslCertificate> *caCertificates,
                                          const QByteArray &passPhrase)
{
    // These are required
    Q_ASSERT(device);
    Q_ASSERT(key);
    Q_ASSERT(cert);

    // Read the file into a BIO
    QByteArray pkcs12data = device->readAll();
    if (pkcs12data.size() == 0)
        return false;

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pkcs12data.constData()), pkcs12data.size());
    if (!bio) {
        qCWarning(lcTlsBackend, "BIO_new_mem_buf returned null");
        return false;
    }
    const auto bioRaii = qScopeGuard([bio]{q_BIO_free(bio);});

    // Create the PKCS#12 object
    PKCS12 *p12 = q_d2i_PKCS12_bio(bio, nullptr);
    if (!p12) {
        qCWarning(lcTlsBackend, "Unable to read PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), nullptr));
        return false;
    }
    const auto p12Raii = qScopeGuard([p12]{q_PKCS12_free(p12);});

    // Extract the data
    EVP_PKEY *pkey = nullptr;
    X509 *x509 = nullptr;
    STACK_OF(X509) *ca = nullptr;

    if (!q_PKCS12_parse(p12, passPhrase.constData(), &pkey, &x509, &ca)) {
        qCWarning(lcTlsBackend, "Unable to parse PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), nullptr));
        return false;
    }

    const auto x509Raii = qScopeGuard([x509]{q_X509_free(x509);});
    const auto keyRaii = qScopeGuard([pkey]{q_EVP_PKEY_free(pkey);});
    const auto caRaii = qScopeGuard([ca] {
        q_OPENSSL_sk_pop_free(reinterpret_cast<OPENSSL_STACK *>(ca),
                              reinterpret_cast<void (*)(void *)>(q_X509_free));
    });

    // Convert to Qt types
    auto *tlsKey = QTlsBackend::backend<TlsKeyOpenSSL>(*key);
    if (!tlsKey || !tlsKey->fromEVP_PKEY(pkey)) {
        qCWarning(lcTlsBackend, "Unable to convert private key");
        return false;
    }

    *cert = certificateFromX509(x509);

    if (caCertificates)
        *caCertificates = stackOfX509ToQSslCertificates(ca);

    return true;
}

QSslError X509CertificateOpenSSL::openSSLErrorToQSslError(int errorCode, const QSslCertificate &cert)
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

void X509CertificateOpenSSL::parseExtensions()
{
    extensions.clear();

    if (!x509)
        return;

    int count = q_X509_get_ext_count(x509);
    if (count <= 0)
        return;

    extensions.reserve(count);

    for (int i = 0; i < count; i++) {
        X509_EXTENSION *ext = q_X509_get_ext(x509, i);
        if (!ext) {
            qCWarning(lcTlsBackend) << "Invalid (nullptr) extension at index" << i;
            continue;
        }

        extensions << convertExtension(ext);
    }

    // Converting an extension may result in an error(s), clean them up:
    QTlsBackendOpenSSL::clearErrorQueue();
}

X509CertificateBase::X509CertificateExtension X509CertificateOpenSSL::convertExtension(X509_EXTENSION *ext)
{
    Q_ASSERT(ext);

    X509CertificateExtension result;

    ASN1_OBJECT *obj = q_X509_EXTENSION_get_object(ext);
    if (!obj)
        return result;

    result.oid = QString::fromUtf8(asn1ObjectId(obj));
    result.name = QString::fromUtf8(asn1ObjectName(obj));

    result.critical = bool(q_X509_EXTENSION_get_critical(ext));

    // Lets see if we have custom support for this one
    QVariant extensionValue = x509ExtensionToValue(ext);
    if (extensionValue.isValid()) {
        result.value = extensionValue;
        result.supported = true;
        return result;
    }

    extensionValue = x509UnknownExtensionToValue(ext);
    if (extensionValue.isValid())
        result.value = extensionValue;

    result.supported = false;

    return result;
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
