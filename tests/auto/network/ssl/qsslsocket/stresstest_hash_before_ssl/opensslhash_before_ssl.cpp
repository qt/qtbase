// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Helper binary for tst_QSslSocket::opensslHashBeforeSslInit().
//
// Must use a custom main() so that QCryptographicHash is exercised before any
// QSslSocket API call initialises OpenSSL. The normal QTest startup sequence
// calls availableBackends() → isValid() → ensureLibraryLoaded(), so a regular
// test function would already have SSL initialised by the time it runs.

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QSslSocket>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Use QCryptographicHash before any SSL API is touched.  When
    // openssl_hash is enabled, the destructor calls OSSL_PROVIDER_unload()
    // on the default provider, draining its activation count to zero.
    // ensureLibraryLoaded() (called below via supportsSsl()) must recover.
    {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData("regression test for QTBUG-openssl-rand-after-hash");
        (void)hash.result();
    }

    if (!QSslSocket::availableBackends().contains(u"openssl"_s))
        return 2; // OpenSSL backend not built — tell caller to skip

    QSslSocket::setActiveBackend(u"openssl"_s);
    return QSslSocket::supportsSsl() ? EXIT_SUCCESS : EXIT_FAILURE;
}
