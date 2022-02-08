/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
#include <private/qglobal_p.h>

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

// SHA-3
#include "../../3rdparty/SHA-3/HashFunction.cpp"
#include "../../3rdparty/SHA-3/Keccak.cpp"

// Header from rfc6234
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

static inline int SHA224_256AddLength(SHA256Context *context, unsigned int length)
{
  uint32_t addTemp;
  return SHA224_256AddLengthM(context, length);
}
static inline int SHA384_512AddLength(SHA512Context *context, unsigned int length)
{
  uint64_t addTemp;
  return SHA384_512AddLengthM(context, length);
}

#if QT_CONFIG(system_libb2)
#include <blake2.h>
#else
#include "../../3rdparty/blake2/src/blake2b-ref.c"
#include "../../3rdparty/blake2/src/blake2s-ref.c"
#endif
#endif // QT_CRYPTOGRAPHICHASH_ONLY_SHA1

QT_BEGIN_NAMESPACE

static constexpr qsizetype MaxHashLength = 64;

static constexpr int hashLengthInternal(QCryptographicHash::Algorithm method) noexcept
{
    switch (method) {
#define CASE(Enum, Size) \
    case QCryptographicHash:: Enum : \
        /* if this triggers, then increase MaxHashLength accordingly */ \
        static_assert(MaxHashLength >= qsizetype(Size) ); \
        return Size \
    /*end*/
    CASE(Sha1, 20);
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    CASE(Md4, 16);
    CASE(Md5, 16);
    CASE(Sha224, SHA224HashSize);
    CASE(Sha256, SHA256HashSize);
    CASE(Sha384, SHA384HashSize);
    CASE(Sha512, SHA512HashSize);
    CASE(Blake2s_128, 128 / 8);
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2s_160:
        static_assert(160 / 8 <= MaxHashLength);
        return 160 / 8;
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::Blake2s_224:
        static_assert(224 / 8 <= MaxHashLength);
        return 224 / 8;
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2s_256:
        static_assert(256 / 8 <= MaxHashLength);
        return 256 / 8;
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::Blake2b_384:
        static_assert(384 / 8 <= MaxHashLength);
        return 384 / 8;
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
    case QCryptographicHash::Blake2b_512:
        static_assert(512 / 8 <= MaxHashLength);
        return 512 / 8;
#endif
#undef CASE
    }
    return 0;
}

class QCryptographicHashPrivate
{
public:
    explicit QCryptographicHashPrivate(QCryptographicHash::Algorithm method) noexcept
        : method(method)
    {
        allocate();
    }
    ~QCryptographicHashPrivate() { free(); }
    void reset() noexcept
    {
        free();
        allocate();
    }
    void addData(QByteArrayView bytes) noexcept;
    void finalize() noexcept;
    QByteArrayView resultView() const noexcept { return result.toByteArrayView(); }

    const QCryptographicHash::Algorithm method;
    union Contexts {
        Sha1State sha1Context;
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        MD5Context md5Context;
        md4_context md4Context;
        SHA224Context sha224Context;
        SHA256Context sha256Context;
        SHA384Context sha384Context;
        SHA512Context sha512Context;
        blake2b_state blake2bContext;
        blake2s_state blake2sContext;
        Keccak keccakContext;
        Sha3 sha3Context;
#endif
        Contexts() { } // Defined because they are
        ~Contexts() { } // implicitly-deleted
    } contexts;

private:
    void allocate() noexcept;
    void free() noexcept;
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    void digestToResult(KeccakBase &keccak) noexcept;
#endif
    class SmallByteArray {
        std::array<char, MaxHashLength> m_data;
        static_assert(MaxHashLength <= std::numeric_limits<std::uint8_t>::max());
        std::uint8_t m_size;
    public:
        char *data() noexcept { return m_data.data(); }
        const char *data() const noexcept { return m_data.data(); }
        qsizetype size() const noexcept { return qsizetype{m_size}; }
        bool isEmpty() const noexcept { return size() == 0; }
        void clear() noexcept { m_size = 0; }
        void resize(qsizetype s) {
            Q_ASSERT(s >= 0);
            Q_ASSERT(s <= MaxHashLength);
            m_size = std::uint8_t(s);
        }
        QByteArrayView toByteArrayView() const noexcept
        { return QByteArrayView{data(), size()}; }
    };
    SmallByteArray result;
};

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
  \value Blake2b_160 Generate a BLAKE2b-160 hash sum. Introduced in Qt 6.0
  \value Blake2b_256 Generate a BLAKE2b-256 hash sum. Introduced in Qt 6.0
  \value Blake2b_384 Generate a BLAKE2b-384 hash sum. Introduced in Qt 6.0
  \value Blake2b_512 Generate a BLAKE2b-512 hash sum. Introduced in Qt 6.0
  \value Blake2s_128 Generate a BLAKE2s-128 hash sum. Introduced in Qt 6.0
  \value Blake2s_160 Generate a BLAKE2s-160 hash sum. Introduced in Qt 6.0
  \value Blake2s_224 Generate a BLAKE2s-224 hash sum. Introduced in Qt 6.0
  \value Blake2s_256 Generate a BLAKE2s-256 hash sum. Introduced in Qt 6.0
  \omitvalue RealSha3_224
  \omitvalue RealSha3_256
  \omitvalue RealSha3_384
  \omitvalue RealSha3_512
*/

/*!
  Constructs an object that can be used to create a cryptographic hash from data using \a method.
*/
QCryptographicHash::QCryptographicHash(Algorithm method)
    : d(new QCryptographicHashPrivate{method})
{
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
void QCryptographicHash::reset() noexcept
{
    d->reset();
}

void QCryptographicHashPrivate::free() noexcept
{
    switch (method) {
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::RealSha3_512:
        contexts.sha3Context.~Sha3();
        break;
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::Keccak_512:
        contexts.keccakContext.~Keccak();
#endif
    default:
        break;
    }
}

void QCryptographicHashPrivate::allocate() noexcept
{
    switch (method) {
    case QCryptographicHash::Sha1:
        new (&contexts.sha1Context) Sha1State;
        sha1InitState(&contexts.sha1Context);
        break;
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case QCryptographicHash::Md4:
        new (&contexts.md4Context) md4_context;
        md4_init(&contexts.md4Context);
        break;
    case QCryptographicHash::Md5:
        new (&contexts.md5Context) MD5Context;
        MD5Init(&contexts.md5Context);
        break;
    case QCryptographicHash::Sha224:
        new (&contexts.sha224Context) SHA224Context;
        SHA224Reset(&contexts.sha224Context);
        break;
    case QCryptographicHash::Sha256:
        new (&contexts.sha256Context) SHA256Context;
        SHA256Reset(&contexts.sha256Context);
        break;
    case QCryptographicHash::Sha384:
        new (&contexts.sha384Context) SHA384Context;
        SHA384Reset(&contexts.sha384Context);
        break;
    case QCryptographicHash::Sha512:
        new (&contexts.sha512Context) SHA512Context;
        SHA512Reset(&contexts.sha512Context);
        break;
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::RealSha3_512:
        new (&contexts.sha3Context) Sha3(hashLengthInternal(method) * 8);
        break;
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::Keccak_512:
        new (&contexts.keccakContext) Keccak(hashLengthInternal(method) * 8);
        break;
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2b_384:
    case QCryptographicHash::Blake2b_512:
        new (&contexts.blake2bContext) blake2b_state;
        blake2b_init(&contexts.blake2bContext, hashLengthInternal(method));
        break;
    case QCryptographicHash::Blake2s_128:
    case QCryptographicHash::Blake2s_160:
    case QCryptographicHash::Blake2s_224:
    case QCryptographicHash::Blake2s_256:
        new (&contexts.blake2sContext) blake2s_state;
        blake2s_init(&contexts.blake2sContext, hashLengthInternal(method));
        break;
#endif
    }
    result.clear();
}

#if QT_DEPRECATED_SINCE(6, 4)
/*!
    Adds the first \a length chars of \a data to the cryptographic
    hash.

    \obsolete
    Use the QByteArrayView overload instead.
*/
void QCryptographicHash::addData(const char *data, qsizetype length)
{
    Q_ASSERT(length >= 0);
    addData(QByteArrayView{data, length});
}
#endif

/*!
    Adds the characters in \a bytes to the cryptographic hash.

    \note In Qt versions prior to 6.3, this function took QByteArray,
    not QByteArrayView.
*/
void QCryptographicHash::addData(QByteArrayView bytes) noexcept
{
    d->addData(bytes);
}

void QCryptographicHashPrivate::addData(QByteArrayView bytes) noexcept
{
    const char *data = bytes.data();
    auto length = bytes.size();

#if QT_POINTER_SIZE == 8
    // feed the data UINT_MAX bytes at a time, as some of the methods below
    // take a uint (of course, feeding more than 4G of data into the hashing
    // functions will be pretty slow anyway)
    for (auto remaining = length; remaining; remaining -= length, data += length) {
        length = qMin(qsizetype(std::numeric_limits<uint>::max()), remaining);
#else
    {
#endif
        switch (method) {
        case QCryptographicHash::Sha1:
            sha1Update(&contexts.sha1Context, (const unsigned char *)data, length);
            break;
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        default:
            Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
            Q_UNREACHABLE();
            break;
#else
        case QCryptographicHash::Md4:
            md4_update(&contexts.md4Context, (const unsigned char *)data, length);
            break;
        case QCryptographicHash::Md5:
            MD5Update(&contexts.md5Context, (const unsigned char *)data, length);
            break;
        case QCryptographicHash::Sha224:
            SHA224Input(&contexts.sha224Context, reinterpret_cast<const unsigned char *>(data),
                        length);
            break;
        case QCryptographicHash::Sha256:
            SHA256Input(&contexts.sha256Context, reinterpret_cast<const unsigned char *>(data),
                        length);
            break;
        case QCryptographicHash::Sha384:
            SHA384Input(&contexts.sha384Context, reinterpret_cast<const unsigned char *>(data),
                        length);
            break;
        case QCryptographicHash::Sha512:
            SHA512Input(&contexts.sha512Context, reinterpret_cast<const unsigned char *>(data),
                        length);
            break;
        case QCryptographicHash::Keccak_224:
        case QCryptographicHash::Keccak_256:
        case QCryptographicHash::Keccak_384:
        case QCryptographicHash::Keccak_512:
            contexts.keccakContext.addData(reinterpret_cast<const uint8_t *>(data), 0, length);
            break;
        case QCryptographicHash::RealSha3_224:
        case QCryptographicHash::RealSha3_256:
        case QCryptographicHash::RealSha3_384:
        case QCryptographicHash::RealSha3_512:
            contexts.sha3Context.addData(reinterpret_cast<const uint8_t *>(data), 0, length);
            break;
        case QCryptographicHash::Blake2b_160:
        case QCryptographicHash::Blake2b_256:
        case QCryptographicHash::Blake2b_384:
        case QCryptographicHash::Blake2b_512:
            blake2b_update(&contexts.blake2bContext, reinterpret_cast<const uint8_t *>(data),
                           length);
            break;
        case QCryptographicHash::Blake2s_128:
        case QCryptographicHash::Blake2s_160:
        case QCryptographicHash::Blake2s_224:
        case QCryptographicHash::Blake2s_256:
            blake2s_update(&contexts.blake2sContext, reinterpret_cast<const uint8_t *>(data),
                           length);
            break;
#endif
        }
    }
    result.clear();
}

/*!
  Reads the data from the open QIODevice \a device until it ends
  and hashes it. Returns \c true if reading was successful.
  \since 5.0
 */
bool QCryptographicHash::addData(QIODevice *device)
{
    if (!device->isReadable())
        return false;

    if (!device->isOpen())
        return false;

    char buffer[1024];
    int length;

    while ((length = device->read(buffer, sizeof(buffer))) > 0)
        d->addData({buffer, length});

    return device->atEnd();
}


/*!
  Returns the final hash value.

  \sa resultView(), QByteArray::toHex()
*/
QByteArray QCryptographicHash::result() const
{
    return resultView().toByteArray();
}

/*!
  \since 6.3

  Returns the final hash value.

  Note that the returned view remains valid only as long as the QCryptographicHash object is
  not modified by other means.

  \sa result()
*/
QByteArrayView QCryptographicHash::resultView() const noexcept
{
    d->finalize();
    return d->resultView();
}

void QCryptographicHashPrivate::finalize() noexcept
{
    if (!result.isEmpty())
        return;

    switch (method) {
    case QCryptographicHash::Sha1: {
        Sha1State copy = contexts.sha1Context;
        result.resize(20);
        sha1FinalizeState(&copy);
        sha1ToHash(&copy, reinterpret_cast<unsigned char *>(result.data()));
        break;
    }
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case QCryptographicHash::Md4: {
        md4_context copy = contexts.md4Context;
        result.resize(MD4_RESULTLEN);
        md4_final(&copy, reinterpret_cast<unsigned char *>(result.data()));
        break;
    }
    case QCryptographicHash::Md5: {
        MD5Context copy = contexts.md5Context;
        result.resize(16);
        MD5Final(&copy, reinterpret_cast<unsigned char *>(result.data()));
        break;
    }
    case QCryptographicHash::Sha224: {
        SHA224Context copy = contexts.sha224Context;
        result.resize(SHA224HashSize);
        SHA224Result(&copy, reinterpret_cast<unsigned char *>(result.data()));
        break;
    }
    case QCryptographicHash::Sha256: {
        SHA256Context copy = contexts.sha256Context;
        result.resize(SHA256HashSize);
        SHA256Result(&copy, reinterpret_cast<unsigned char *>(result.data()));
        break;
    }
    case QCryptographicHash::Sha384: {
        SHA384Context copy = contexts.sha384Context;
        result.resize(SHA384HashSize);
        SHA384Result(&copy, reinterpret_cast<unsigned char *>(result.data()));
        break;
    }
    case QCryptographicHash::Sha512: {
        SHA512Context copy = contexts.sha512Context;
        result.resize(SHA512HashSize);
        SHA512Result(&copy, reinterpret_cast<unsigned char *>(result.data()));
        break;
    }
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::RealSha3_512: {
        Sha3 copy = contexts.sha3Context;
        digestToResult(copy);
        break;
    }
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::Keccak_512: {
        Keccak copy = contexts.keccakContext;
        digestToResult(copy);
        break;
    }
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2b_384:
    case QCryptographicHash::Blake2b_512: {
        const auto length = hashLengthInternal(method);
        blake2b_state copy = contexts.blake2bContext;
        result.resize(length);
        blake2b_final(&copy, reinterpret_cast<uint8_t *>(result.data()), length);
        break;
    }
    case QCryptographicHash::Blake2s_128:
    case QCryptographicHash::Blake2s_160:
    case QCryptographicHash::Blake2s_224:
    case QCryptographicHash::Blake2s_256: {
        const auto length = hashLengthInternal(method);
        blake2s_state copy = contexts.blake2sContext;
        result.resize(length);
        blake2s_final(&copy, reinterpret_cast<uint8_t *>(result.data()), length);
        break;
    }
#endif
    }
}

#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
void QCryptographicHashPrivate::digestToResult(KeccakBase &keccak) noexcept
{
    std::vector<unsigned char> digest = keccak.digest();
    result.resize(digest.size());
    for (size_t i = 0; i < digest.size(); ++i)
        result.data()[i] = static_cast<char>(digest[i]);
}
#endif

/*!
  Returns the hash of \a data using \a method.

  \note In Qt versions prior to 6.3, this function took QByteArray,
  not QByteArrayView.
*/
QByteArray QCryptographicHash::hash(QByteArrayView data, Algorithm method)
{
    QCryptographicHashPrivate hash(method);
    hash.addData(data);
    hash.finalize();
    return hash.resultView().toByteArray();
}

/*!
  Returns the size of the output of the selected hash \a method in bytes.

  \since 5.12
*/
int QCryptographicHash::hashLength(QCryptographicHash::Algorithm method)
{
    return hashLengthInternal(method);
}

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qcryptographichash.cpp"
#endif
