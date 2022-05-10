// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
