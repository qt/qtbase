/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2013 Richard J. Moore <rich@kde.org>.
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

#include <qcryptographichash.h>
#include <qiodevice.h>

#include "../../3rdparty/sha1/sha1.cpp"

#if defined(QT_BOOTSTRAPPED) && !defined(QT_CRYPTOGRAPHICHASH_ONLY_SHA1)
#  error "Are you sure you need the other hashing algorithms besides SHA-1?"
#endif

#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
// qdoc and qmake only need SHA-1
#include "../../3rdparty/md5/md5.h"
#include "../../3rdparty/md5/md5.cpp"
#include "../../3rdparty/md4/md4.h"
#include "../../3rdparty/md4/md4.cpp"

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;
typedef enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 } HashReturn;

#ifdef Q_OS_RTEMS
#  undef ALIGN
#endif

#include "../../3rdparty/sha3/KeccakSponge.c"
typedef spongeState hashState;

#include "../../3rdparty/sha3/KeccakNISTInterface.c"

/*
  This lets us choose between SHA3 implementations at build time.
 */
typedef spongeState SHA3Context;
typedef HashReturn (SHA3Init)(hashState *state, int hashbitlen);
typedef HashReturn (SHA3Update)(hashState *state, const BitSequence *data, DataLength databitlen);
typedef HashReturn (SHA3Final)(hashState *state, BitSequence *hashval);

#if Q_PROCESSOR_WORDSIZE == 8 // 64 bit version

#include "../../3rdparty/sha3/KeccakF-1600-opt64.c"

static SHA3Init * const sha3Init = Init;
static SHA3Update * const sha3Update = Update;
static SHA3Final * const sha3Final = Final;

#else // 32 bit optimised fallback

#include "../../3rdparty/sha3/KeccakF-1600-opt32.c"

static SHA3Init * const sha3Init = Init;
static SHA3Update * const sha3Update = Update;
static SHA3Final * const sha3Final = Final;

#endif

/*
    These #defines replace the typedefs needed by the RFC6234 code. Normally
    the typedefs would come from from stdint.h, but since this header is not
    available on all platforms (MSVC 2008, for example), we #define them to the
    Qt equivalents.
*/

#ifdef uint64_t
#undef uint64_t
#endif

#define uint64_t QT_PREPEND_NAMESPACE(quint64)

#ifdef uint32_t
#undef uint32_t
#endif

#define uint32_t QT_PREPEND_NAMESPACE(quint32)

#ifdef uint8_t
#undef uint8_t
#endif

#define uint8_t QT_PREPEND_NAMESPACE(quint8)

#ifdef int_least16_t
#undef int_least16_t
#endif

#define int_least16_t QT_PREPEND_NAMESPACE(qint16)

// Header from rfc6234 with 1 modification:
// sha1.h - commented out '#include <stdint.h>' on line 74
#include "../../3rdparty/rfc6234/sha.h"

/*
    These 2 functions replace macros of the same name in sha224-256.c and
    sha384-512.c. Originally, these macros relied on a global static 'addTemp'
    variable. We do not want this for 2 reasons:

    1. since we are including the sources directly, the declaration of the 2 conflict

    2. static variables are not thread-safe, we do not want multiple threads
    computing a hash to corrupt one another
*/
static int SHA224_256AddLength(SHA256Context *context, unsigned int length);
static int SHA384_512AddLength(SHA512Context *context, unsigned int length);

// Sources from rfc6234, with 4 modifications:
// sha224-256.c - commented out 'static uint32_t addTemp;' on line 68
// sha224-256.c - appended 'M' to the SHA224_256AddLength macro on line 70
#include "../../3rdparty/rfc6234/sha224-256.c"
// sha384-512.c - commented out 'static uint64_t addTemp;' on line 302
// sha384-512.c - appended 'M' to the SHA224_256AddLength macro on line 304
#include "../../3rdparty/rfc6234/sha384-512.c"

#undef uint64_t
#undef uint32_t
#undef uint68_t
#undef int_least16_t

static inline int SHA224_256AddLength(SHA256Context *context, unsigned int length)
{
  QT_PREPEND_NAMESPACE(quint32) addTemp;
  return SHA224_256AddLengthM(context, length);
}
static inline int SHA384_512AddLength(SHA512Context *context, unsigned int length)
{
  QT_PREPEND_NAMESPACE(quint64) addTemp;
  return SHA384_512AddLengthM(context, length);
}
#endif // QT_CRYPTOGRAPHICHASH_ONLY_SHA1

QT_BEGIN_NAMESPACE

class QCryptographicHashPrivate
{
public:
    QCryptographicHash::Algorithm method;
    union {
        Sha1State sha1Context;
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        MD5Context md5Context;
        md4_context md4Context;
        SHA224Context sha224Context;
        SHA256Context sha256Context;
        SHA384Context sha384Context;
        SHA512Context sha512Context;
        SHA3Context sha3Context;
#endif
    };
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    enum class Sha3Variant
    {
        Sha3,
        Keccak
    };
    void sha3Finish(int bitCount, Sha3Variant sha3Variant);
#endif
    QByteArray result;
};

#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
void QCryptographicHashPrivate::sha3Finish(int bitCount, Sha3Variant sha3Variant)
{
    /*
        FIPS 202 §6.1 defines SHA-3 in terms of calculating the Keccak function
        over the original message with the two-bit suffix "01" appended to it.
        This variable stores that suffix (and it's fed into the calculations
        when the hash is returned to users).

        Only 2 bits of this variable are actually used (see the call to sha3Update
        below). The Keccak implementation we're using will actually use the
        *leftmost* 2 bits, and interpret them right-to-left. In other words, the
        bits must appear in order of *increasing* significance; and as the two most
        significant bits of the byte -- the rightmost 6 are ignored. (Yes, this
        seems self-contradictory, but it's the way it is...)

        Overall, this means:
        * the leftmost two bits must be "10" (not "01"!);
        * we don't care what the other six bits are set to (they can be set to
        any value), but we arbitrarily set them to 0;

        and for an unsigned char this gives us 0b10'00'00'00, or 0x80.
    */
    static const unsigned char sha3FinalSuffix = 0x80;

    result.resize(bitCount / 8);

    SHA3Context copy = sha3Context;

    switch (sha3Variant) {
    case Sha3Variant::Sha3:
        sha3Update(&copy, reinterpret_cast<const BitSequence *>(&sha3FinalSuffix), 2);
        break;
    case Sha3Variant::Keccak:
        break;
    }

    sha3Final(&copy, reinterpret_cast<BitSequence *>(result.data()));
}
#endif

/*!
  \class QCryptographicHash
  \inmodule QtCore

  \brief The QCryptographicHash class provides a way to generate cryptographic hashes.

  \since 4.3

  \ingroup tools
  \reentrant

  QCryptographicHash can be used to generate cryptographic hashes of binary or text data.

  Refer to the documentation of the \l QCryptographicHash::Algorithm enum for a
  list of the supported algorithms.
*/

/*!
  \enum QCryptographicHash::Algorithm

  \note In Qt versions before 5.9, when asked to generate a SHA3 hash sum,
  QCryptographicHash actually calculated Keccak. If you need compatibility with
  SHA-3 hashes produced by those versions of Qt, use the \c{Keccak_}
  enumerators. Alternatively, if source compatibility is required, define the
  macro \c QT_SHA3_KECCAK_COMPAT.

  \value Md4 Generate an MD4 hash sum
  \value Md5 Generate an MD5 hash sum
  \value Sha1 Generate an SHA-1 hash sum
  \value Sha224 Generate an SHA-224 hash sum (SHA-2). Introduced in Qt 5.0
  \value Sha256 Generate an SHA-256 hash sum (SHA-2). Introduced in Qt 5.0
  \value Sha384 Generate an SHA-384 hash sum (SHA-2). Introduced in Qt 5.0
  \value Sha512 Generate an SHA-512 hash sum (SHA-2). Introduced in Qt 5.0
  \value Sha3_224 Generate an SHA3-224 hash sum. Introduced in Qt 5.1
  \value Sha3_256 Generate an SHA3-256 hash sum. Introduced in Qt 5.1
  \value Sha3_384 Generate an SHA3-384 hash sum. Introduced in Qt 5.1
  \value Sha3_512 Generate an SHA3-512 hash sum. Introduced in Qt 5.1
  \value Keccak_224 Generate a Keccak-224 hash sum. Introduced in Qt 5.9.2
  \value Keccak_256 Generate a Keccak-256 hash sum. Introduced in Qt 5.9.2
  \value Keccak_384 Generate a Keccak-384 hash sum. Introduced in Qt 5.9.2
  \value Keccak_512 Generate a Keccak-512 hash sum. Introduced in Qt 5.9.2
  \omitvalue RealSha3_224
  \omitvalue RealSha3_256
  \omitvalue RealSha3_384
  \omitvalue RealSha3_512
*/

/*!
  Constructs an object that can be used to create a cryptographic hash from data using \a method.
*/
QCryptographicHash::QCryptographicHash(Algorithm method)
    : d(new QCryptographicHashPrivate)
{
    d->method = method;
    reset();
}

/*!
  Destroys the object.
*/
QCryptographicHash::~QCryptographicHash()
{
    delete d;
}

/*!
  Resets the object.
*/
void QCryptographicHash::reset()
{
    switch (d->method) {
    case Sha1:
        sha1InitState(&d->sha1Context);
        break;
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case Md4:
        md4_init(&d->md4Context);
        break;
    case Md5:
        MD5Init(&d->md5Context);
        break;
    case Sha224:
        SHA224Reset(&d->sha224Context);
        break;
    case Sha256:
        SHA256Reset(&d->sha256Context);
        break;
    case Sha384:
        SHA384Reset(&d->sha384Context);
        break;
    case Sha512:
        SHA512Reset(&d->sha512Context);
        break;
    case RealSha3_224:
    case Keccak_224:
        sha3Init(&d->sha3Context, 224);
        break;
    case RealSha3_256:
    case Keccak_256:
        sha3Init(&d->sha3Context, 256);
        break;
    case RealSha3_384:
    case Keccak_384:
        sha3Init(&d->sha3Context, 384);
        break;
    case RealSha3_512:
    case Keccak_512:
        sha3Init(&d->sha3Context, 512);
        break;
#endif
    }
    d->result.clear();
}

/*!
    Adds the first \a length chars of \a data to the cryptographic
    hash.
*/
void QCryptographicHash::addData(const char *data, int length)
{
    switch (d->method) {
    case Sha1:
        sha1Update(&d->sha1Context, (const unsigned char *)data, length);
        break;
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case Md4:
        md4_update(&d->md4Context, (const unsigned char *)data, length);
        break;
    case Md5:
        MD5Update(&d->md5Context, (const unsigned char *)data, length);
        break;
    case Sha224:
        SHA224Input(&d->sha224Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    case Sha256:
        SHA256Input(&d->sha256Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    case Sha384:
        SHA384Input(&d->sha384Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    case Sha512:
        SHA512Input(&d->sha512Context, reinterpret_cast<const unsigned char *>(data), length);
        break;
    case RealSha3_224:
    case Keccak_224:
        sha3Update(&d->sha3Context, reinterpret_cast<const BitSequence *>(data), quint64(length) * 8);
        break;
    case RealSha3_256:
    case Keccak_256:
        sha3Update(&d->sha3Context, reinterpret_cast<const BitSequence *>(data), quint64(length) * 8);
        break;
    case RealSha3_384:
    case Keccak_384:
        sha3Update(&d->sha3Context, reinterpret_cast<const BitSequence *>(data), quint64(length) * 8);
        break;
    case RealSha3_512:
    case Keccak_512:
        sha3Update(&d->sha3Context, reinterpret_cast<const BitSequence *>(data), quint64(length) * 8);
        break;
#endif
    }
    d->result.clear();
}

/*!
  \overload addData()
*/
void QCryptographicHash::addData(const QByteArray &data)
{
    addData(data.constData(), data.length());
}

/*!
  Reads the data from the open QIODevice \a device until it ends
  and hashes it. Returns \c true if reading was successful.
  \since 5.0
 */
bool QCryptographicHash::addData(QIODevice* device)
{
    if (!device->isReadable())
        return false;

    if (!device->isOpen())
        return false;

    char buffer[1024];
    int length;

    while ((length = device->read(buffer,sizeof(buffer))) > 0)
        addData(buffer,length);

    return device->atEnd();
}


/*!
  Returns the final hash value.

  \sa QByteArray::toHex()
*/
QByteArray QCryptographicHash::result() const
{
    if (!d->result.isEmpty())
        return d->result;

    switch (d->method) {
    case Sha1: {
        Sha1State copy = d->sha1Context;
        d->result.resize(20);
        sha1FinalizeState(&copy);
        sha1ToHash(&copy, (unsigned char *)d->result.data());
        break;
    }
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case Md4: {
        md4_context copy = d->md4Context;
        d->result.resize(MD4_RESULTLEN);
        md4_final(&copy, (unsigned char *)d->result.data());
        break;
    }
    case Md5: {
        MD5Context copy = d->md5Context;
        d->result.resize(16);
        MD5Final(&copy, (unsigned char *)d->result.data());
        break;
    }
    case Sha224: {
        SHA224Context copy = d->sha224Context;
        d->result.resize(SHA224HashSize);
        SHA224Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    case Sha256:{
        SHA256Context copy = d->sha256Context;
        d->result.resize(SHA256HashSize);
        SHA256Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    case Sha384:{
        SHA384Context copy = d->sha384Context;
        d->result.resize(SHA384HashSize);
        SHA384Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    case Sha512:{
        SHA512Context copy = d->sha512Context;
        d->result.resize(SHA512HashSize);
        SHA512Result(&copy, reinterpret_cast<unsigned char *>(d->result.data()));
        break;
    }
    case RealSha3_224: {
        d->sha3Finish(224, QCryptographicHashPrivate::Sha3Variant::Sha3);
        break;
    }
    case RealSha3_256: {
        d->sha3Finish(256, QCryptographicHashPrivate::Sha3Variant::Sha3);
        break;
    }
    case RealSha3_384: {
        d->sha3Finish(384, QCryptographicHashPrivate::Sha3Variant::Sha3);
        break;
    }
    case RealSha3_512: {
        d->sha3Finish(512, QCryptographicHashPrivate::Sha3Variant::Sha3);
        break;
    }
    case Keccak_224: {
        d->sha3Finish(224, QCryptographicHashPrivate::Sha3Variant::Keccak);
        break;
    }
    case Keccak_256: {
        d->sha3Finish(256, QCryptographicHashPrivate::Sha3Variant::Keccak);
        break;
    }
    case Keccak_384: {
        d->sha3Finish(384, QCryptographicHashPrivate::Sha3Variant::Keccak);
        break;
    }
    case Keccak_512: {
        d->sha3Finish(512, QCryptographicHashPrivate::Sha3Variant::Keccak);
        break;
    }
#endif
    }
    return d->result;
}

/*!
  Returns the hash of \a data using \a method.
*/
QByteArray QCryptographicHash::hash(const QByteArray &data, Algorithm method)
{
    QCryptographicHash hash(method);
    hash.addData(data);
    return hash.result();
}

/*!
  Returns the size of the output of the selected hash \a method in bytes.

  \since 5.12
*/
int QCryptographicHash::hashLength(QCryptographicHash::Algorithm method)
{
    switch (method) {
    case QCryptographicHash::Sha1:
        return 20;
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    case QCryptographicHash::Md4:
        return 16;
    case QCryptographicHash::Md5:
        return 16;
    case QCryptographicHash::Sha224:
        return SHA224HashSize;
    case QCryptographicHash::Sha256:
        return SHA256HashSize;
    case QCryptographicHash::Sha384:
        return SHA384HashSize;
    case QCryptographicHash::Sha512:
        return SHA512HashSize;
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
        return 224 / 8;
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
        return 256 / 8;
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
        return 384 / 8;
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
        return 512 / 8;
#endif
    }
    return 0;
}

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qcryptographichash.cpp"
#endif
