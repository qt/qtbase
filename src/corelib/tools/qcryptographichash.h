/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2013 Richard J. Moore <rich@kde.org>.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QCRYPTOGRAPHICHASH_H
#define QCRYPTOGRAPHICHASH_H

#include <QtCore/qbytearray.h>
#include <QtCore/qobjectdefs.h>

QT_BEGIN_NAMESPACE


class QCryptographicHashPrivate;
class QIODevice;

class Q_CORE_EXPORT QCryptographicHash
{
    Q_GADGET
public:
    enum Algorithm {
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        Md4,
        Md5,
#endif
        Sha1 = 2,
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        Sha224,
        Sha256,
        Sha384,
        Sha512,

        Keccak_224 = 7,
        Keccak_256,
        Keccak_384,
        Keccak_512,
        RealSha3_224 = 11,
        RealSha3_256,
        RealSha3_384,
        RealSha3_512,
#  ifndef QT_SHA3_KECCAK_COMPAT
        Sha3_224 = RealSha3_224,
        Sha3_256 = RealSha3_256,
        Sha3_384 = RealSha3_384,
        Sha3_512 = RealSha3_512
#  else
        Sha3_224 = Keccak_224,
        Sha3_256 = Keccak_256,
        Sha3_384 = Keccak_384,
        Sha3_512 = Keccak_512
#  endif
#endif
    };
    Q_ENUM(Algorithm)

    explicit QCryptographicHash(Algorithm method);
    ~QCryptographicHash();

    void reset();

    void addData(const char *data, int length);
    void addData(const QByteArray &data);
    bool addData(QIODevice* device);

    QByteArray result() const;

    static QByteArray hash(const QByteArray &data, Algorithm method);
    static int hashLength(Algorithm method);
private:
    Q_DISABLE_COPY(QCryptographicHash)
    QCryptographicHashPrivate *d;
};

QT_END_NAMESPACE

#endif
