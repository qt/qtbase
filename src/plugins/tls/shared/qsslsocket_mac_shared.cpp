// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 ownCloud Inc
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtNetwork/qsslcertificate.h>

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>


#ifdef Q_OS_MACOS

#include <QtCore/private/qcore_mac_p.h>

#include <CoreFoundation/CFArray.h>
#include <Security/Security.h>

#endif // Q_OS_MACOS

QT_BEGIN_NAMESPACE

#ifdef Q_OS_MACOS
namespace {

bool hasTrustedSslServerPolicy(SecPolicyRef policy, CFDictionaryRef props) {
    QCFType<CFDictionaryRef> policyProps = SecPolicyCopyProperties(policy);
    // only accept certificates with policies for SSL server validation for now
    if (CFEqual(CFDictionaryGetValue(policyProps, kSecPolicyOid), kSecPolicyAppleSSL)) {
        CFBooleanRef policyClient;
        if (CFDictionaryGetValueIfPresent(policyProps, kSecPolicyClient, reinterpret_cast<const void**>(&policyClient)) &&
            CFEqual(policyClient, kCFBooleanTrue)) {
            return false; // no client certs
        }
        if (!CFDictionaryContainsKey(props, kSecTrustSettingsResult)) {
            // as per the docs, no trust settings result implies full trust
            return true;
        }
        CFNumberRef number = static_cast<CFNumberRef>(CFDictionaryGetValue(props, kSecTrustSettingsResult));
        SecTrustSettingsResult settingsResult;
        CFNumberGetValue(number, kCFNumberSInt32Type, &settingsResult);
        switch (settingsResult) {
        case kSecTrustSettingsResultTrustRoot:
        case kSecTrustSettingsResultTrustAsRoot:
            return true;
        default:
            return false;
        }
    }
    return false;
}

bool isCaCertificateTrusted(SecCertificateRef cfCert, int domain)
{
    QCFType<CFArrayRef> cfTrustSettings;
    OSStatus status = SecTrustSettingsCopyTrustSettings(cfCert, SecTrustSettingsDomain(domain), &cfTrustSettings);
    if (status == noErr) {
        CFIndex size = CFArrayGetCount(cfTrustSettings);
        // if empty, trust for everything (as per the Security Framework documentation)
        if (size == 0) {
            return true;
        } else {
            for (CFIndex i = 0; i < size; ++i) {
                CFDictionaryRef props = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(cfTrustSettings, i));
                if (CFDictionaryContainsKey(props, kSecTrustSettingsPolicy)) {
                    if (hasTrustedSslServerPolicy((SecPolicyRef)CFDictionaryGetValue(props, kSecTrustSettingsPolicy), props))
                        return true;
                }
            }
        }
    }

    return false;
}

} // unnamed namespace
#endif // Q_OS_MACOS

namespace QTlsPrivate {
QList<QSslCertificate> systemCaCertificates()
{
    QList<QSslCertificate> systemCerts;
    // SecTrustSettingsCopyCertificates is not defined on iOS.
#ifdef Q_OS_MACOS
    // iterate through all enum members, order:
    // kSecTrustSettingsDomainUser, kSecTrustSettingsDomainAdmin, kSecTrustSettingsDomainSystem
    for (int dom = kSecTrustSettingsDomainUser; dom <= int(kSecTrustSettingsDomainSystem); dom++) {
        QCFType<CFArrayRef> cfCerts;
        OSStatus status = SecTrustSettingsCopyCertificates(SecTrustSettingsDomain(dom), &cfCerts);
        if (status == noErr) {
            const CFIndex size = CFArrayGetCount(cfCerts);
            for (CFIndex i = 0; i < size; ++i) {
                SecCertificateRef cfCert = (SecCertificateRef)CFArrayGetValueAtIndex(cfCerts, i);
                QCFType<CFDataRef> derData = SecCertificateCopyData(cfCert);
                if (isCaCertificateTrusted(cfCert, dom)) {
                    if (derData)
                        systemCerts << QSslCertificate(QByteArray::fromCFData(derData), QSsl::Der);
                }
            }
        }
    }
#endif
    return systemCerts;
}
} // namespace QTlsPrivate

QT_END_NAMESPACE
