/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
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

#include "qsslellipticcurve.h"
#include "qsslsocket_p.h"
#include "qsslsocket_openssl_symbols_p.h"

#include <openssl/ssl.h>
#include <openssl/obj_mac.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

QString QSslEllipticCurve::shortName() const
{
    QString result;
#ifndef OPENSSL_NO_EC
    if (id != 0)
        result = QString::fromLatin1(q_OBJ_nid2sn(id));
#endif
    return result;
}

QString QSslEllipticCurve::longName() const
{
    QString result;
#ifndef OPENSSL_NO_EC
    if (id != 0)
        result = QString::fromLatin1(q_OBJ_nid2ln(id));
#endif
    return result;
}

QSslEllipticCurve QSslEllipticCurve::fromShortName(const QString &name)
{
    if (name.isEmpty())
        return QSslEllipticCurve();

    QSslSocketPrivate::ensureInitialized();

    QSslEllipticCurve result;

#ifndef OPENSSL_NO_EC

    const QByteArray curveNameLatin1 = name.toLatin1();
    int nid = q_OBJ_sn2nid(curveNameLatin1.data());

    if (nid == 0)
        nid = q_EC_curve_nist2nid(curveNameLatin1.data());

    result.id = nid;

#endif // !OPENSSL_NO_EC

    return result;
}

QSslEllipticCurve QSslEllipticCurve::fromLongName(const QString &name)
{
    if (name.isEmpty())
        return QSslEllipticCurve();

    QSslSocketPrivate::ensureInitialized();

    QSslEllipticCurve result;

#ifndef OPENSSL_NO_EC
    const QByteArray curveNameLatin1 = name.toLatin1();

    int nid = q_OBJ_ln2nid(curveNameLatin1.data());
    result.id = nid;
#endif

    return result;
}


// The brainpool curve NIDs (RFC 7027) have been introduced in OpenSSL 1.0.2,
// redefine them here to make Qt compile with previous versions of OpenSSL
// (yet correctly recognize them as TLS named curves).
// See crypto/objects/obj_mac.h
#ifndef NID_brainpoolP256r1
#define NID_brainpoolP256r1 927
#endif

#ifndef NID_brainpoolP384r1
#define NID_brainpoolP384r1 931
#endif

#ifndef NID_brainpoolP512r1
#define NID_brainpoolP512r1 933
#endif

// NIDs of named curves allowed in TLS as per RFCs 4492 and 7027,
// see also https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-8
static const int tlsNamedCurveNIDs[] = {
    // RFC 4492
    NID_sect163k1,
    NID_sect163r1,
    NID_sect163r2,
    NID_sect193r1,
    NID_sect193r2,
    NID_sect233k1,
    NID_sect233r1,
    NID_sect239k1,
    NID_sect283k1,
    NID_sect283r1,
    NID_sect409k1,
    NID_sect409r1,
    NID_sect571k1,
    NID_sect571r1,

    NID_secp160k1,
    NID_secp160r1,
    NID_secp160r2,
    NID_secp192k1,
    NID_X9_62_prime192v1, // secp192r1
    NID_secp224k1,
    NID_secp224r1,
    NID_secp256k1,
    NID_X9_62_prime256v1, // secp256r1
    NID_secp384r1,
    NID_secp521r1,

    // RFC 7027
    NID_brainpoolP256r1,
    NID_brainpoolP384r1,
    NID_brainpoolP512r1
};

static const size_t tlsNamedCurveNIDCount = sizeof(tlsNamedCurveNIDs) / sizeof(tlsNamedCurveNIDs[0]);

bool QSslEllipticCurve::isTlsNamedCurve() const noexcept
{
    const int * const tlsNamedCurveNIDsEnd = tlsNamedCurveNIDs + tlsNamedCurveNIDCount;
    return std::find(tlsNamedCurveNIDs, tlsNamedCurveNIDsEnd, id) != tlsNamedCurveNIDsEnd;
}

QT_END_NAMESPACE
