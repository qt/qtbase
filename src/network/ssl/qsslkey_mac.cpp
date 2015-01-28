/****************************************************************************
**
** Copyright (C) 2014 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsslkey.h"
#include "qsslkey_p.h"

#include <CommonCrypto/CommonCrypto.h>

QT_USE_NAMESPACE

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
    };
    size_t plainLength = 0;
    QByteArray plain(data.size() + blockSize, 0);
    CCCryptorStatus status = CCCrypt(
        ccOp, ccAlgorithm, kCCOptionPKCS7Padding,
        key.constData(), key.size(),
        iv.constData(),
        data.constData(), data.size(),
        plain.data(), plain.size(), &plainLength);
    if (status == kCCSuccess)
        return plain.left(plainLength);
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
