/****************************************************************************
**
** Copyright (C) 2013 Ruslan Nigmatullin <euroelessar@yandex.ru>
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

#include "qmessageauthenticationcode.h"
#include "qvarlengtharray.h"

#include "qtcore-config_p.h"

// Header from rfc6234
#include "../../3rdparty/rfc6234/sha.h"

#if QT_CONFIG(system_libb2)
#include <blake2.h>
#else
#include "../../3rdparty/blake2/src/blake2.h"
#endif

QT_BEGIN_NAMESPACE

static int qt_hash_block_size(QCryptographicHash::Algorithm method)
{
    switch (method) {
    case QCryptographicHash::Md4:
        return 64;
    case QCryptographicHash::Md5:
        return 64;
    case QCryptographicHash::Sha1:
        return SHA1_Message_Block_Size;
    case QCryptographicHash::Sha224:
        return SHA224_Message_Block_Size;
    case QCryptographicHash::Sha256:
        return SHA256_Message_Block_Size;
    case QCryptographicHash::Sha384:
        return SHA384_Message_Block_Size;
    case QCryptographicHash::Sha512:
        return SHA512_Message_Block_Size;
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
        return 144;
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
        return 136;
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
        return 104;
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
        return 72;
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2b_384:
    case QCryptographicHash::Blake2b_512:
        return BLAKE2B_BLOCKBYTES;
    case QCryptographicHash::Blake2s_128:
    case QCryptographicHash::Blake2s_160:
    case QCryptographicHash::Blake2s_224:
    case QCryptographicHash::Blake2s_256:
        return BLAKE2S_BLOCKBYTES;
    }
    return 0;
}

class QMessageAuthenticationCodePrivate
{
public:
    QMessageAuthenticationCodePrivate(QCryptographicHash::Algorithm m)
        : messageHash(m), method(m), messageHashInited(false)
    {
    }

    QByteArray key;
    QByteArray result;
    QCryptographicHash messageHash;
    QCryptographicHash::Algorithm method;
    bool messageHashInited;

    void initMessageHash();
};

void QMessageAuthenticationCodePrivate::initMessageHash()
{
    if (messageHashInited)
        return;
    messageHashInited = true;

    const int blockSize = qt_hash_block_size(method);

    if (key.size() > blockSize) {
        key = QCryptographicHash::hash(key, method);
    }

    if (key.size() < blockSize) {
        const int size = key.size();
        key.resize(blockSize);
        memset(key.data() + size, 0, blockSize - size);
    }

    QVarLengthArray<char> iKeyPad(blockSize);
    const char * const keyData = key.constData();

    for (int i = 0; i < blockSize; ++i)
        iKeyPad[i] = keyData[i] ^ 0x36;

    messageHash.addData(iKeyPad);
}

/*!
    \class QMessageAuthenticationCode
    \inmodule QtCore

    \brief The QMessageAuthenticationCode class provides a way to generate
    hash-based message authentication codes.

    \since 5.1

    \ingroup tools
    \reentrant

    QMessageAuthenticationCode supports all cryptographic hashes which are supported by
    QCryptographicHash.

    To generate message authentication code, pass hash algorithm QCryptographicHash::Algorithm
    to constructor, then set key and message by setKey() and addData() functions. Result
    can be acquired by result() function.
    \snippet qmessageauthenticationcode/main.cpp 0
    \dots
    \snippet qmessageauthenticationcode/main.cpp 1

    Alternatively, this effect can be achieved by providing message,
    key and method to hash() method.
    \snippet qmessageauthenticationcode/main.cpp 2

    \sa QCryptographicHash
*/

/*!
    Constructs an object that can be used to create a cryptographic hash from data
    using method \a method and key \a key.
*/
QMessageAuthenticationCode::QMessageAuthenticationCode(QCryptographicHash::Algorithm method,
                                                       const QByteArray &key)
    : d(new QMessageAuthenticationCodePrivate(method))
{
    d->key = key;
}

/*!
    Destroys the object.
*/
QMessageAuthenticationCode::~QMessageAuthenticationCode()
{
    delete d;
}

/*!
    Resets message data. Calling this method doesn't affect the key.
*/
void QMessageAuthenticationCode::reset()
{
    d->result.clear();
    d->messageHash.reset();
    d->messageHashInited = false;
}

/*!
    Sets secret \a key. Calling this method automatically resets the object state.
*/
void QMessageAuthenticationCode::setKey(const QByteArray &key)
{
    reset();
    d->key = key;
}

/*!
    Adds the first \a length chars of \a data to the message.
*/
void QMessageAuthenticationCode::addData(const char *data, qsizetype length)
{
    d->initMessageHash();
    d->messageHash.addData({data, length});
}

/*!
    \overload addData()
*/
void QMessageAuthenticationCode::addData(const QByteArray &data)
{
    d->initMessageHash();
    d->messageHash.addData(data);
}

/*!
    Reads the data from the open QIODevice \a device until it ends
    and adds it to message. Returns \c true if reading was successful.

    \note \a device must be already opened.
 */
bool QMessageAuthenticationCode::addData(QIODevice *device)
{
    d->initMessageHash();
    return d->messageHash.addData(device);
}

/*!
    Returns the final authentication code.

    \sa QByteArray::toHex()
*/
QByteArray QMessageAuthenticationCode::result() const
{
    if (!d->result.isEmpty())
        return d->result;

    d->initMessageHash();

    const int blockSize = qt_hash_block_size(d->method);

    QByteArrayView hashedMessage = d->messageHash.resultView();

    QVarLengthArray<char> oKeyPad(blockSize);
    const char * const keyData = d->key.constData();

    for (int i = 0; i < blockSize; ++i)
        oKeyPad[i] = keyData[i] ^ 0x5c;

    QCryptographicHash hash(d->method);
    hash.addData(oKeyPad);
    hash.addData(hashedMessage);

    d->result = hash.result();
    return d->result;
}

/*!
    Returns the authentication code for the message \a message using
    the key \a key and the method \a method.
*/
QByteArray QMessageAuthenticationCode::hash(const QByteArray &message, const QByteArray &key,
                                            QCryptographicHash::Algorithm method)
{
    QMessageAuthenticationCode mac(method);
    mac.setKey(key);
    mac.addData(message);
    return mac.result();
}

QT_END_NAMESPACE
