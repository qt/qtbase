/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qpassworddigestor.h"

#include <QtCore/QDebug>
#include <QtCore/QMessageAuthenticationCode>
#include <QtCore/QtEndian>

#include <limits>

QT_BEGIN_NAMESPACE
namespace QPasswordDigestor {

/*!
    \namespace QPasswordDigestor
    \inmodule QtNetwork

    \brief The QPasswordDigestor namespace contains functions which you can use
    to generate hashes or keys.
*/

/*!
    \since 5.12

    Returns a hash computed using the PBKDF1-algorithm as defined in
    \l {https://tools.ietf.org/html/rfc8018#section-5.1} {RFC 8018}.

    The function takes the \a data and \a salt, and then hashes it repeatedly
    for \a iterations iterations using the specified hash \a algorithm. If the
    resulting hash is longer than \a dkLen then it is truncated before it is
    returned.

    This function only supports SHA-1 and MD5! The max output size is 160 bits
    (20 bytes) when using SHA-1, or 128 bits (16 bytes) when using MD5.
    Specifying a value for \a dkLen which is greater than this results in a
    warning and an empty QByteArray is returned. To programmatically check this
    limit you can use \l {QCryptographicHash::hashLength}. Furthermore: the
    \a salt must always be 8 bytes long!

    \note This function is provided for use with legacy applications and all
    new applications are recommended to use \l {deriveKeyPbkdf2} {PBKDF2}.

    \sa deriveKeyPbkdf2, QCryptographicHash, QCryptographicHash::hashLength
*/
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf1(QCryptographicHash::Algorithm algorithm,
                                            const QByteArray &data, const QByteArray &salt,
                                            int iterations, quint64 dkLen)
{
    // https://tools.ietf.org/html/rfc8018#section-5.1

    if (algorithm != QCryptographicHash::Sha1
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        && algorithm != QCryptographicHash::Md5
#endif
    ) {
        qWarning("The only supported algorithms for pbkdf1 are SHA-1 and MD5!");
        return QByteArray();
    }

    if (salt.size() != 8) {
        qWarning("The salt must be 8 bytes long!");
        return QByteArray();
    }
    if (iterations < 1 || dkLen < 1)
        return QByteArray();

    if (dkLen > quint64(QCryptographicHash::hashLength(algorithm))) {
        qWarning() << "Derived key too long:\n"
                   << algorithm << "was chosen which produces output of length"
                   << QCryptographicHash::hashLength(algorithm) << "but" << dkLen
                   << "was requested.";
        return QByteArray();
    }

    QCryptographicHash hash(algorithm);
    hash.addData(data);
    hash.addData(salt);
    QByteArray key = hash.result();

    for (int i = 1; i < iterations; i++) {
        hash.reset();
        hash.addData(key);
        key = hash.result();
    }
    return key.left(dkLen);
}

/*!
    \since 5.12

    Derive a key using the PBKDF2-algorithm as defined in
    \l {https://tools.ietf.org/html/rfc8018#section-5.2} {RFC 8018}.

    This function takes the \a data and \a salt, and then applies HMAC-X, where
    the X is \a algorithm, repeatedly. It internally concatenates intermediate
    results to the final output until at least \a dkLen amount of bytes have
    been computed and it will execute HMAC-X \a iterations times each time a
    concatenation is required. The total number of times it will execute HMAC-X
    depends on \a iterations, \a dkLen and \a algorithm and can be calculated
    as
    \c{iterations * ceil(dkLen / QCryptographicHash::hashLength(algorithm))}.

    \sa deriveKeyPbkdf1, QMessageAuthenticationCode, QCryptographicHash
*/
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm,
                                            const QByteArray &data, const QByteArray &salt,
                                            int iterations, quint64 dkLen)
{
    // https://tools.ietf.org/html/rfc8018#section-5.2

    // The RFC recommends checking that 'dkLen' is not greater than '(2^32 - 1) * hLen'
    int hashLen = QCryptographicHash::hashLength(algorithm);
    const quint64 maxLen = quint64(std::numeric_limits<quint32>::max() - 1) * hashLen;
    if (dkLen > maxLen) {
        qWarning().nospace() << "Derived key too long:\n"
                             << algorithm << " was chosen which produces output of length "
                             << maxLen << " but " << dkLen << " was requested.";
        return QByteArray();
    }

    if (iterations < 1 || dkLen < 1)
        return QByteArray();

    QByteArray key;
    quint32 currentIteration = 1;
    QMessageAuthenticationCode hmac(algorithm, data);
    QByteArray index(4, Qt::Uninitialized);
    while (quint64(key.length()) < dkLen) {
        hmac.addData(salt);

        qToBigEndian(currentIteration, index.data());
        hmac.addData(index);

        QByteArray u = hmac.result();
        hmac.reset();
        QByteArray tkey = u;
        for (int iter = 1; iter < iterations; iter++) {
            hmac.addData(u);
            u = hmac.result();
            hmac.reset();
            std::transform(tkey.cbegin(), tkey.cend(), u.cbegin(), tkey.begin(),
                           std::bit_xor<char>());
        }
        key += tkey;
        currentIteration++;
    }
    return key.left(dkLen);
}
} // namespace QPasswordDigestor
QT_END_NAMESPACE
