/****************************************************************************
**
** Copyright (C) 2015 Mikkel Krautz <mikkel@krautz.dk>
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

#include "qssldiffiehellmanparameters.h"
#include "qssldiffiehellmanparameters_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslsocket_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qiodevice.h>
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

#include <openssl/bn.h>
#include <openssl/dh.h>

QT_BEGIN_NAMESPACE

static bool isSafeDH(DH *dh)
{
    int status = 0;
    int bad = 0;

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
#if QT_CONFIG(opensslv11)
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
        long residue = q_BN_mod_word(p, 24);
        if (residue == 11 || residue == 23)
            status &= ~DH_NOT_SUITABLE_GENERATOR;
    }

#else
    // Mark p < 1024 bits as unsafe.
    if (q_BN_num_bits(dh->p) < 1024)
        return false;

    if (q_DH_check(dh, &status) != 1)
        return false;

    if (q_BN_is_word(dh->g, DH_GENERATOR_2)) {
        long residue = q_BN_mod_word(dh->p, 24);
        if (residue == 11 || residue == 23)
            status &= ~DH_NOT_SUITABLE_GENERATOR;
    }
#endif

    bad |= DH_CHECK_P_NOT_PRIME;
    bad |= DH_CHECK_P_NOT_SAFE_PRIME;
    bad |= DH_NOT_SUITABLE_GENERATOR;

    return !(status & bad);
}

void QSslDiffieHellmanParametersPrivate::decodeDer(const QByteArray &der)
{
    if (der.isEmpty()) {
        error = QSslDiffieHellmanParameters::InvalidInputDataError;
        return;
    }

    const unsigned char *data = reinterpret_cast<const unsigned char *>(der.data());
    int len = der.size();

    QSslSocketPrivate::ensureInitialized();

    DH *dh = q_d2i_DHparams(NULL, &data, len);
    if (dh) {
        if (isSafeDH(dh))
            derData = der;
        else
            error =  QSslDiffieHellmanParameters::UnsafeParametersError;
    } else {
        error = QSslDiffieHellmanParameters::InvalidInputDataError;
    }

    q_DH_free(dh);
}

void QSslDiffieHellmanParametersPrivate::decodePem(const QByteArray &pem)
{
    if (pem.isEmpty()) {
        error = QSslDiffieHellmanParameters::InvalidInputDataError;
        return;
    }

    if (!QSslSocket::supportsSsl()) {
        error = QSslDiffieHellmanParameters::InvalidInputDataError;
        return;
    }

    QSslSocketPrivate::ensureInitialized();

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pem.data()), pem.size());
    if (!bio) {
        error = QSslDiffieHellmanParameters::InvalidInputDataError;
        return;
    }

    DH *dh = nullptr;
    q_PEM_read_bio_DHparams(bio, &dh, 0, 0);

    if (dh) {
        if (isSafeDH(dh)) {
            char *buf = nullptr;
            int len = q_i2d_DHparams(dh, reinterpret_cast<unsigned char **>(&buf));
            if (len > 0)
                derData = QByteArray(buf, len);
            else
                error = QSslDiffieHellmanParameters::InvalidInputDataError;
        } else {
            error = QSslDiffieHellmanParameters::UnsafeParametersError;
        }
    } else {
        error = QSslDiffieHellmanParameters::InvalidInputDataError;
    }

    q_DH_free(dh);
    q_BIO_free(bio);
}

QT_END_NAMESPACE
