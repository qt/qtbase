// Copyright (C) 2015 Mikkel Krautz <mikkel@krautz.dk>
// Copyright (C) 2016 Richard J. Moore <rich@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsslsocket_openssl_symbols_p.h"
#include "qtlsbackend_openssl_p.h"

#include <QtNetwork/private/qsslsocket_p.h>

#include <QtCore/qscopeguard.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qdebug.h>

#include <openssl/bn.h>
#include <openssl/dh.h>

QT_BEGIN_NAMESPACE

#ifndef OPENSSL_NO_DEPRECATED_3_0

namespace {

bool isSafeDH(DH *dh)
{
    int status = 0;
    int bad = 0;

    // TLSTODO: check it's needed or if supportsSsl()
    // is enough.
    QSslSocketPrivate::ensureInitialized();

    // From https://wiki.openssl.org/index.php/Diffie-Hellman_parameters:
    //
    //     The additional call to BN_mod_word(dh->p, 24)
    //     (and unmasking of DH_NOT_SUITABLE_GENERATOR)
    //     is performed to ensure your program accepts
    //     IETF group parameters. OpenSSL checks the prime
    //     is congruent to 11 when g = 2; while the IETF's
    //     primes are congruent to 23 when g = 2.
    //     Without the test, the IETF parameters would
    //     fail validation. For details, see Diffie-Hellman
    //     Parameter Check (when g = 2, must p mod 24 == 11?).
    // Mark p < 1024 bits as unsafe.
    if (q_DH_bits(dh) < 1024)
        return false;

    if (q_DH_check(dh, &status) != 1)
        return false;

    const BIGNUM *p = nullptr;
    const BIGNUM *q = nullptr;
    const BIGNUM *g = nullptr;
    q_DH_get0_pqg(dh, &p, &q, &g);

    if (q_BN_is_word(const_cast<BIGNUM *>(g), DH_GENERATOR_2)) {
        const unsigned long residue = q_BN_mod_word(p, 24);
        if (residue == 11 || residue == 23)
            status &= ~DH_NOT_SUITABLE_GENERATOR;
    }

    bad |= DH_CHECK_P_NOT_PRIME;
    bad |= DH_CHECK_P_NOT_SAFE_PRIME;
    bad |= DH_NOT_SUITABLE_GENERATOR;

    return !(status & bad);
}

} // unnamed namespace

#endif

int QTlsBackendOpenSSL::dhParametersFromDer(const QByteArray &der, QByteArray *derData) const
{
#ifndef OPENSSL_NO_DEPRECATED_3_0
    Q_ASSERT(derData);

    if (der.isEmpty())
        return DHParams::InvalidInputDataError;

    const unsigned char *data = reinterpret_cast<const unsigned char *>(der.data());
    const int len = der.size();

    // TLSTODO: check it's needed (loading ciphers and certs in
    // addition to the library!)
    QSslSocketPrivate::ensureInitialized();

    DH *dh = q_d2i_DHparams(nullptr, &data, len);
    if (dh) {
        const auto dhRaii = qScopeGuard([dh] {q_DH_free(dh);});

        if (isSafeDH(dh))
            *derData = der;
        else
            return DHParams::UnsafeParametersError;
    } else {
        return DHParams::InvalidInputDataError;
    }
#else
    Q_UNUSED(der);
    Q_UNUSED(derData);
    qCWarning(lcTlsBackend, "Diffie-Hellman parameters are not supported, because OpenSSL v3 was built with deprecated API removed");
#endif
    return DHParams::NoError;
}

int QTlsBackendOpenSSL::dhParametersFromPem(const QByteArray &pem, QByteArray *data) const
{
#ifndef OPENSSL_NO_DEPRECATED_3_0
    Q_ASSERT(data);

    if (pem.isEmpty())
        return DHParams::InvalidInputDataError;

    // TLSTODO: check it was not a cargo-cult programming in case of
    // DH ...
    QSslSocketPrivate::ensureInitialized();

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pem.data()), pem.size());
    if (!bio)
        return DHParams::InvalidInputDataError;

    const auto bioRaii = qScopeGuard([bio]
    {
        q_BIO_free(bio);
    });

    DH *dh = nullptr;
    q_PEM_read_bio_DHparams(bio, &dh, nullptr, nullptr);

    if (dh) {
        const auto dhGuard = qScopeGuard([dh]
        {
            q_DH_free(dh);
        });

        if (isSafeDH(dh)) {
            char *buf = nullptr;
            const int len = q_i2d_DHparams(dh, reinterpret_cast<unsigned char **>(&buf));
            const auto freeBuf = qScopeGuard([&] { q_OPENSSL_free(buf); });
            if (len > 0)
                data->assign({buf, len});
            else
                return DHParams::InvalidInputDataError;
        } else {
            return DHParams::UnsafeParametersError;
        }
    } else {
        return DHParams::InvalidInputDataError;
    }
#else
    Q_UNUSED(pem);
    Q_UNUSED(data);
    qCWarning(lcTlsBackend, "Diffie-Hellman parameters are not supported, because OpenSSL v3 was built with deprecated API removed");
#endif
    return DHParams::NoError;
}

QT_END_NAMESPACE
