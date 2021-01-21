/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qsslkey.h"
#include "qsslkey_p.h"

#include <CommonCrypto/CommonCrypto.h>

#include <cstddef>

QT_BEGIN_NAMESPACE

static QByteArray wrapCCCrypt(CCOperation ccOp,
                              QSslKeyPrivate::Cipher cipher,
                              const QByteArray &data,
                              const QByteArray &key, const QByteArray &iv)
{
    int blockSize;
    CCAlgorithm ccAlgorithm;
    switch (cipher) {
    case QSslKeyPrivate::DesCbc:
        blockSize = kCCBlockSizeDES;
        ccAlgorithm = kCCAlgorithmDES;
        break;
    case QSslKeyPrivate::DesEde3Cbc:
        blockSize = kCCBlockSize3DES;
        ccAlgorithm = kCCAlgorithm3DES;
        break;
    case QSslKeyPrivate::Rc2Cbc:
        blockSize = kCCBlockSizeRC2;
        ccAlgorithm = kCCAlgorithmRC2;
        break;
    case QSslKeyPrivate::Aes128Cbc:
    case QSslKeyPrivate::Aes192Cbc:
    case QSslKeyPrivate::Aes256Cbc:
        blockSize = kCCBlockSizeAES128;
        ccAlgorithm = kCCAlgorithmAES;
        break;
    }
    size_t plainLength = 0;
    QByteArray plain(data.size() + blockSize, 0);
    CCCryptorStatus status = CCCrypt(
        ccOp, ccAlgorithm, kCCOptionPKCS7Padding,
        key.constData(), std::size_t(key.size()),
        iv.constData(),
        data.constData(), std::size_t(data.size()),
        plain.data(), std::size_t(plain.size()), &plainLength);
    if (status == kCCSuccess)
        return plain.left(int(plainLength));
    return QByteArray();
}

QByteArray QSslKeyPrivate::decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    return wrapCCCrypt(kCCDecrypt, cipher, data, key, iv);
}

QByteArray QSslKeyPrivate::encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    return wrapCCCrypt(kCCEncrypt, cipher, data, key, iv);
}

QT_END_NAMESPACE
