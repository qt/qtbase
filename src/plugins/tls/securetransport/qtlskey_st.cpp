/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
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

#include "qtlskey_st_p.h"

#include <QtNetwork/private/qsslkey_p.h>

#include <QtCore/qbytearray.h>

#include <CommonCrypto/CommonCrypto.h>

#include <cstddef>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {
namespace {

// Before this code was located in qsslkey_mac.cpp.
QByteArray wrapCCCrypt(CCOperation ccOp, QSslKeyPrivate::Cipher cipher,
                       const QByteArray &data, const QByteArray &key,
                       const QByteArray &iv)
{
    int blockSize = {};
    CCAlgorithm ccAlgorithm = {};
    switch (cipher) {
    case Cipher::DesCbc:
        blockSize = kCCBlockSizeDES;
        ccAlgorithm = kCCAlgorithmDES;
        break;
    case Cipher::DesEde3Cbc:
        blockSize = kCCBlockSize3DES;
        ccAlgorithm = kCCAlgorithm3DES;
        break;
    case Cipher::Rc2Cbc:
        blockSize = kCCBlockSizeRC2;
        ccAlgorithm = kCCAlgorithmRC2;
        break;
    case Cipher::Aes128Cbc:
    case Cipher::Aes192Cbc:
    case Cipher::Aes256Cbc:
        blockSize = kCCBlockSizeAES128;
        ccAlgorithm = kCCAlgorithmAES;
        break;
    }
    std::size_t plainLength = 0;
    QByteArray plain(data.size() + blockSize, 0);
    CCCryptorStatus status = CCCrypt(ccOp, ccAlgorithm, kCCOptionPKCS7Padding,
                                     key.constData(), std::size_t(key.size()),
                                     iv.constData(), data.constData(), std::size_t(data.size()),
                                     plain.data(), std::size_t(plain.size()), &plainLength);
    if (status == kCCSuccess)
        return plain.left(int(plainLength));

    return {};
}

} // Unnamed namespace.

QByteArray TlsKeySecureTransport::decrypt(Cipher cipher, const QByteArray &data,
                                          const QByteArray &key, const QByteArray &iv) const
{
    return wrapCCCrypt(kCCDecrypt, cipher, data, key, iv);
}

QByteArray TlsKeySecureTransport::encrypt(Cipher cipher, const QByteArray &data,
                                          const QByteArray &key, const QByteArray &iv) const
{
    return wrapCCCrypt(kCCEncrypt, cipher, data, key, iv);
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
