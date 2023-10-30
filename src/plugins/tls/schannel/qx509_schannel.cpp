// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtlsbackend_schannel_p.h"
#include "qtlskey_schannel_p.h"
#include "qx509_schannel_p.h"

#include <QtCore/private/qsystemerror_p.h>
#include <QtNetwork/private/qsslcertificate_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

X509CertificateSchannel::X509CertificateSchannel() = default;

X509CertificateSchannel::~X509CertificateSchannel()
{
    if (certificateContext)
        CertFreeCertificateContext(certificateContext);
}

TlsKey *X509CertificateSchannel::publicKey() const
{
    auto key = std::make_unique<TlsKeySchannel>(QSsl::PublicKey);
    if (publicKeyAlgorithm != QSsl::Opaque)
        key->decodeDer(QSsl::PublicKey, publicKeyAlgorithm, publicKeyDerData, {}, false);

    return key.release();
}

Qt::HANDLE X509CertificateSchannel::handle() const
{
    return Qt::HANDLE(certificateContext);
}

QSslCertificate X509CertificateSchannel::QSslCertificate_from_CERT_CONTEXT(const CERT_CONTEXT *certificateContext)
{
    QByteArray derData = QByteArray((const char *)certificateContext->pbCertEncoded,
                                    certificateContext->cbCertEncoded);
    QSslCertificate certificate(derData, QSsl::Der);
    if (!certificate.isNull()) {
        auto *certBackend = QTlsBackend::backend<X509CertificateSchannel>(certificate);
        Q_ASSERT(certBackend);
        certBackend->certificateContext = CertDuplicateCertificateContext(certificateContext);
    }
    return certificate;
}

bool X509CertificateSchannel::importPkcs12(QIODevice *device, QSslKey *key, QSslCertificate *cert,
                                           QList<QSslCertificate> *caCertificates,
                                           const QByteArray &passPhrase)
{
    // These are required
    Q_ASSERT(device);
    Q_ASSERT(key);
    Q_ASSERT(cert);

    QByteArray pkcs12data = device->readAll();
    if (pkcs12data.size() == 0)
        return false;

    CRYPT_DATA_BLOB dataBlob;
    dataBlob.cbData = pkcs12data.size();
    dataBlob.pbData = reinterpret_cast<BYTE*>(pkcs12data.data());

    const auto password = QString::fromUtf8(passPhrase);

    const DWORD flags = (CRYPT_EXPORTABLE | PKCS12_NO_PERSIST_KEY | PKCS12_PREFER_CNG_KSP);

    auto certStore = QHCertStorePointer(PFXImportCertStore(&dataBlob,
                                                           reinterpret_cast<LPCWSTR>(password.utf16()),
                                                           flags));

    if (!certStore) {
        qCWarning(lcTlsBackendSchannel, "Failed to import PFX data: %s",
                  qPrintable(QSystemError::windowsString()));
        return false;
    }

    // first extract the certificate with the private key
    const auto certContext = QPCCertContextPointer(CertFindCertificateInStore(certStore.get(),
                                                                              X509_ASN_ENCODING |
                                                                              PKCS_7_ASN_ENCODING,
                                                                              0,
                                                                              CERT_FIND_HAS_PRIVATE_KEY,
                                                                              nullptr, nullptr));

    if (!certContext) {
        qCWarning(lcTlsBackendSchannel, "Failed to find certificate in PFX store: %s",
                  qPrintable(QSystemError::windowsString()));
        return false;
    }

    *cert = QSslCertificate_from_CERT_CONTEXT(certContext.get());

    // retrieve the private key for the certificate
    NCRYPT_KEY_HANDLE keyHandle = {};
    DWORD keyHandleSize = sizeof(keyHandle);
    if (!CertGetCertificateContextProperty(certContext.get(), CERT_NCRYPT_KEY_HANDLE_PROP_ID,
                                           &keyHandle, &keyHandleSize)) {
        qCWarning(lcTlsBackendSchannel, "Failed to find private key handle in certificate context: %s",
                  qPrintable(QSystemError::windowsString()));
        return false;
    }

    SECURITY_STATUS securityStatus = ERROR_SUCCESS;

    // we need the 'NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG' to make NCryptExportKey succeed
    DWORD policy = (NCRYPT_ALLOW_EXPORT_FLAG | NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG);
    DWORD policySize = sizeof(policy);

    securityStatus = NCryptSetProperty(keyHandle, NCRYPT_EXPORT_POLICY_PROPERTY,
                                       reinterpret_cast<BYTE*>(&policy), policySize, 0);
    if (securityStatus != ERROR_SUCCESS) {
        qCWarning(lcTlsBackendSchannel, "Failed to update export policy of private key: 0x%x",
                  static_cast<unsigned int>(securityStatus));
        return false;
    }

    DWORD blobSize = {};
    securityStatus = NCryptExportKey(keyHandle, {}, BCRYPT_RSAFULLPRIVATE_BLOB,
                                     nullptr, nullptr, 0, &blobSize, 0);
    if (securityStatus != ERROR_SUCCESS) {
        qCWarning(lcTlsBackendSchannel, "Failed to retrieve private key size: 0x%x",
                  static_cast<unsigned int>(securityStatus));
        return false;
    }

    std::vector<BYTE> blob(blobSize);
    securityStatus = NCryptExportKey(keyHandle, {}, BCRYPT_RSAFULLPRIVATE_BLOB,
                                     nullptr, blob.data(), blobSize, &blobSize, 0);
    if (securityStatus != ERROR_SUCCESS) {
        qCWarning(lcTlsBackendSchannel, "Failed to retrieve private key from certificate: 0x%x",
                  static_cast<unsigned int>(securityStatus));
        return false;
    }

    DWORD privateKeySize = {};

    if (!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB,
                           blob.data(), nullptr, &privateKeySize)) {
        qCWarning(lcTlsBackendSchannel, "Failed to encode private key to key info: %s",
                  qPrintable(QSystemError::windowsString()));
        return false;
    }

    std::vector<BYTE> privateKeyData(privateKeySize);

    CRYPT_PRIVATE_KEY_INFO privateKeyInfo = {};
    privateKeyInfo.Algorithm.pszObjId = const_cast<PSTR>(szOID_RSA_RSA);
    privateKeyInfo.PrivateKey.cbData = privateKeySize;
    privateKeyInfo.PrivateKey.pbData = privateKeyData.data();

    if (!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           CNG_RSA_PRIVATE_KEY_BLOB, blob.data(),
                           privateKeyInfo.PrivateKey.pbData, &privateKeyInfo.PrivateKey.cbData)) {
        qCWarning(lcTlsBackendSchannel, "Failed to encode private key to key info: %s",
                  qPrintable(QSystemError::windowsString()));
        return false;
    }


    DWORD derSize = {};

    if (!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_PRIVATE_KEY_INFO,
                           &privateKeyInfo, nullptr, &derSize)) {
        qCWarning(lcTlsBackendSchannel, "Failed to encode key info to DER format: %s",
                  qPrintable(QSystemError::windowsString()));

        return false;
    }

    QByteArray derData(derSize, Qt::Uninitialized);

    if (!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_PRIVATE_KEY_INFO,
                           &privateKeyInfo, reinterpret_cast<BYTE*>(derData.data()), &derSize)) {
        qCWarning(lcTlsBackendSchannel, "Failed to encode key info to DER format: %s",
                  qPrintable(QSystemError::windowsString()));

        return false;
    }

    *key = QSslKey(derData, QSsl::Rsa, QSsl::Der, QSsl::PrivateKey);
    if (key->isNull()) {
        qCWarning(lcTlsBackendSchannel, "Failed to parse private key from DER format");
        return false;
    }

    // fetch all the remaining certificates as CA certificates
    if (caCertificates) {
        PCCERT_CONTEXT caCertContext = nullptr;
        while ((caCertContext = CertFindCertificateInStore(certStore.get(),
                                                           X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                           0, CERT_FIND_ANY, nullptr, caCertContext))) {
            if (CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                       certContext->pCertInfo, caCertContext->pCertInfo))
                continue; // ignore the certificate with private key

            auto caCertificate = QSslCertificate_from_CERT_CONTEXT(caCertContext);

            caCertificates->append(caCertificate);
        }
    }

    return true;
}

} // namespace QTlsPrivate

QT_END_NAMESPACE

