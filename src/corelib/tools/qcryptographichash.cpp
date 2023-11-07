// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2013 Ruslan Nigmatullin <euroelessar@yandex.ru>
// Copyright (C) 2013 Richard J. Moore <rich@kde.org>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qcryptographichash.h>
#include <qmessageauthenticationcode.h>

#include <qiodevice.h>
#include <qmutex.h>
#include <qvarlengtharray.h>
#include <private/qlocking_p.h>

#include <array>
#include <climits>
#include <numeric>

#include "../../3rdparty/sha1/sha1.cpp"

#if defined(QT_BOOTSTRAPPED) && !defined(QT_CRYPTOGRAPHICHASH_ONLY_SHA1)
#  error "Are you sure you need the other hashing algorithms besides SHA-1?"
#endif

// Header from rfc6234
#include "../../3rdparty/rfc6234/sha.h"

#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
#if !QT_CONFIG(openssl_hash)
// qdoc and qmake only need SHA-1
#include "../../3rdparty/md5/md5.h"
#include "../../3rdparty/md5/md5.cpp"
#include "../../3rdparty/md4/md4.h"
#include "../../3rdparty/md4/md4.cpp"
#endif // !QT_CONFIG(openssl_hash)

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

Q_CONSTINIT static SHA3Init * const sha3Init = Init;
Q_CONSTINIT static SHA3Update * const sha3Update = Update;
Q_CONSTINIT static SHA3Final * const sha3Final = Final;

#else // 32 bit optimised fallback

#include "../../3rdparty/sha3/KeccakF-1600-opt32.c"

Q_CONSTINIT static SHA3Init * const sha3Init = Init;
Q_CONSTINIT static SHA3Update * const sha3Update = Update;
Q_CONSTINIT static SHA3Final * const sha3Final = Final;

#endif

#if !QT_CONFIG(openssl_hash)
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
#endif // !QT_CONFIG(opensslv30)

#include "qtcore-config_p.h"

#if QT_CONFIG(system_libb2)
#include <blake2.h>
#else
#include "../../3rdparty/blake2/src/blake2b-ref.c"
#include "../../3rdparty/blake2/src/blake2s-ref.c"
#endif
#endif // QT_CRYPTOGRAPHICHASH_ONLY_SHA1

#if !defined(QT_BOOTSTRAPPED) && QT_CONFIG(openssl_hash)
#define USING_OPENSSL30
#include <openssl/evp.h>
#include <openssl/provider.h>
#endif

QT_BEGIN_NAMESPACE

template <size_t N>
class QSmallByteArray
{
    std::array<quint8, N> m_data;
    static_assert(N <= std::numeric_limits<std::uint8_t>::max());
    quint8 m_size = 0;
public:
    QSmallByteArray() = default;
    // all compiler-generated SMFs are ok!
    template <std::size_t M, std::enable_if_t<M < N, bool> = true> // M == N is for copy ctor!
    constexpr QSmallByteArray(const QSmallByteArray<M> &other) noexcept
    {
        assign(other);
    }
    template <std::size_t M, std::enable_if_t<M < N, bool> = true> // M == N is for copy-assignment op!
    constexpr QSmallByteArray &operator=(const QSmallByteArray<M> &other) noexcept
    {
        assign(other);
        return *this;
    }

    template <typename Container> // ### underconstrained
    constexpr void assign(const Container &c)
    {
        const size_t otherSize = size_t(std::size(c));
        Q_ASSERT(otherSize < N);
        memcpy(data(), std::data(c), otherSize);
        m_size = quint8(otherSize);
    }

    constexpr quint8 *data() noexcept { return m_data.data(); }
    constexpr const quint8 *data() const noexcept { return m_data.data(); }
    constexpr qsizetype size() const noexcept { return qsizetype{m_size}; }
    constexpr quint8 &operator[](qsizetype n)
    {
        Q_ASSERT(n < size());
        return data()[n];
    }
    constexpr const quint8 &operator[](qsizetype n) const
    {
        Q_ASSERT(n < size());
        return data()[n];
    }
    constexpr bool isEmpty() const noexcept { return size() == 0; }
    constexpr void clear() noexcept { m_size = 0; }
    constexpr void resizeForOverwrite(qsizetype s)
    {
        Q_ASSERT(s >= 0);
        Q_ASSERT(size_t(s) <= N);
        m_size = std::uint8_t(s);
    }
    constexpr void resize(qsizetype s, quint8 v)
    {
        const auto oldSize = size();
        resizeForOverwrite(s);
        if (s > oldSize)
            memset(data() + oldSize, v, size() - oldSize);
    }
    constexpr QByteArrayView toByteArrayView() const noexcept
    { return *this; }

    constexpr auto begin() noexcept { return data(); }
    constexpr auto begin() const noexcept { return data(); }
    constexpr auto cbegin() const noexcept { return begin(); }
    constexpr auto end() noexcept { return data() + size(); }
    constexpr auto end() const noexcept { return data() + size(); }
    constexpr auto cend() const noexcept { return end(); }
};

static constexpr int hashLengthInternal(QCryptographicHash::Algorithm method) noexcept
{
    switch (method) {
#define CASE(Enum, Size) \
    case QCryptographicHash:: Enum : \
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
        return 160 / 8;
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::Blake2s_224:
        return 224 / 8;
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2s_256:
        return 256 / 8;
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::Blake2b_384:
        return 384 / 8;
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
    case QCryptographicHash::Blake2b_512:
        return 512 / 8;
#endif
#undef CASE
    case QCryptographicHash::NumAlgorithms: ;
        // fall through
        // Q_UNREACHABLE() would be BiC here, as hashLength(~~invalid~~) worked in 6.4
    }
    return 0;
}

static constexpr int maxHashLength()
{
    int result = 0;
    using A = QCryptographicHash::Algorithm;
    for (int i = 0; i < A::NumAlgorithms; ++i)
        result = std::max(result, hashLengthInternal(A(i)));
    return result;
}

using HashResult = QSmallByteArray<maxHashLength()>;

#ifdef USING_OPENSSL30
static constexpr const char * methodToName(QCryptographicHash::Algorithm method) noexcept
{
    switch (method) {
#define CASE(Enum, Name) \
    case QCryptographicHash:: Enum : \
        return Name \
    /*end*/
    CASE(Sha1, "SHA1");
    CASE(Md4, "MD4");
    CASE(Md5, "MD5");
    CASE(Sha224, "SHA224");
    CASE(Sha256, "SHA256");
    CASE(Sha384, "SHA384");
    CASE(Sha512, "SHA512");
    CASE(RealSha3_224, "SHA3-224");
    CASE(RealSha3_256, "SHA3-256");
    CASE(RealSha3_384, "SHA3-384");
    CASE(RealSha3_512, "SHA3-512");
    CASE(Blake2b_512, "BLAKE2B512");
    CASE(Blake2s_256, "BLAKE2S256");
#undef CASE
    default: return nullptr;
    }
}

/*
    Checks whether given method is not provided by OpenSSL and whether we will
    have a fallback to non-OpenSSL implementation.
*/
static constexpr bool useNonOpenSSLFallback(QCryptographicHash::Algorithm method) noexcept
{
    if (method == QCryptographicHash::Keccak_224 || method == QCryptographicHash::Keccak_256 ||
        method == QCryptographicHash::Keccak_384 || method == QCryptographicHash::Keccak_512 ||
        method == QCryptographicHash::Blake2b_160 || method == QCryptographicHash::Blake2b_256 ||
        method == QCryptographicHash::Blake2b_384 || method == QCryptographicHash::Blake2s_128 ||
        method == QCryptographicHash::Blake2s_160 || method == QCryptographicHash::Blake2s_224)
        return true;

    return false;
}
#endif // USING_OPENSSL30

class QCryptographicHashPrivate
{
public:
    explicit QCryptographicHashPrivate(QCryptographicHash::Algorithm method) noexcept
        : state(method), method(method)
    {
    }
    ~QCryptographicHashPrivate()
    {
        state.destroy(method);
    }

    void reset() noexcept;
    void addData(QByteArrayView bytes) noexcept;
    bool addData(QIODevice *dev);
    void finalize() noexcept;
    // when not called from the static hash() function, this function needs to be
    // called with finalizeMutex held (finalize() will do that):
    void finalizeUnchecked() noexcept;
    // END functions that need to be called with finalizeMutex held
    QByteArrayView resultView() const noexcept { return result.toByteArrayView(); }
    static bool supportsAlgorithm(QCryptographicHash::Algorithm method);

#ifdef USING_OPENSSL30
    struct EVP_MD_CTX_deleter {
        void operator()(EVP_MD_CTX *ctx) const noexcept {
            EVP_MD_CTX_free(ctx);
        }
    };
    struct EVP_MD_deleter {
        void operator()(EVP_MD *md) const noexcept {
            EVP_MD_free(md);
        }
    };
    struct OSSL_PROVIDER_deleter {
        void operator()(OSSL_PROVIDER *provider) const noexcept {
            OSSL_PROVIDER_unload(provider);
        }
    };

    using EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, EVP_MD_CTX_deleter>;
    using EVP_MD_ptr = std::unique_ptr<EVP_MD, EVP_MD_deleter>;
    using OSSL_PROVIDER_ptr = std::unique_ptr<OSSL_PROVIDER, OSSL_PROVIDER_deleter>;
    struct EVP {
        EVP_MD_ptr algorithm;
        EVP_MD_CTX_ptr context;
        OSSL_PROVIDER_ptr defaultProvider;
        OSSL_PROVIDER_ptr legacyProvider;
        bool initializationFailed;

        explicit EVP(QCryptographicHash::Algorithm method);
        void reset() noexcept;
        void finalizeUnchecked(HashResult &result) noexcept;
    };
#endif

    union State {
        explicit State(QCryptographicHash::Algorithm method);
        void destroy(QCryptographicHash::Algorithm method);
#ifdef USING_OPENSSL30
        ~State() {}
#endif

        void reset(QCryptographicHash::Algorithm method) noexcept;
        void addData(QCryptographicHash::Algorithm method, QByteArrayView data) noexcept;
        void finalizeUnchecked(QCryptographicHash::Algorithm method, HashResult &result) noexcept;

        Sha1State sha1Context;
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
#ifdef USING_OPENSSL30
        EVP evp;
#else
        MD5Context md5Context;
        md4_context md4Context;
        SHA224Context sha224Context;
        SHA256Context sha256Context;
        SHA384Context sha384Context;
        SHA512Context sha512Context;
#endif
        SHA3Context sha3Context;

        enum class Sha3Variant { Sha3, Keccak };
        void sha3Finish(HashResult &result, int bitCount, Sha3Variant sha3Variant);
        blake2b_state blake2bContext;
        blake2s_state blake2sContext;
#endif
    } state;
    // protects result in finalize()
    QBasicMutex finalizeMutex;
    HashResult result;

    const QCryptographicHash::Algorithm method;
};

#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
void QCryptographicHashPrivate::State::sha3Finish(HashResult &result, int bitCount,
                                                  Sha3Variant sha3Variant)
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

    result.resizeForOverwrite(bitCount / 8);

    SHA3Context copy = sha3Context;

    switch (sha3Variant) {
    case Sha3Variant::Sha3:
        sha3Update(&copy, reinterpret_cast<const BitSequence *>(&sha3FinalSuffix), 2);
        break;
    case Sha3Variant::Keccak:
        break;
    }

    sha3Final(&copy, result.data());
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
  \omitvalue NumAlgorithms
*/

/*!
  Constructs an object that can be used to create a cryptographic hash from data using \a method.
*/
QCryptographicHash::QCryptographicHash(Algorithm method)
    : d(new QCryptographicHashPrivate{method})
{
}

/*!
    \fn QCryptographicHash::QCryptographicHash(QCryptographicHash &&other)

    Move-constructs a new QCryptographicHash from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 6.5
*/

/*!
  Destroys the object.
*/
QCryptographicHash::~QCryptographicHash()
{
    delete d;
}

/*!
    \fn QCryptographicHash &QCryptographicHash::operator=(QCryptographicHash &&other)

    Move-assigns \a other to this QCryptographicHash instance.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 6.5
*/

/*!
    \fn void QCryptographicHash::swap(QCryptographicHash &other)

    Swaps cryptographic hash \a other with this cryptographic hash. This
    operation is very fast and never fails.

    \since 6.5
*/

/*!
  Resets the object.
*/
void QCryptographicHash::reset() noexcept
{
    d->reset();
}

/*!
    Returns the algorithm used to generate the cryptographic hash.

    \since 6.5
*/
QCryptographicHash::Algorithm QCryptographicHash::algorithm() const noexcept
{
    return d->method;
}

#ifdef USING_OPENSSL30

QCryptographicHashPrivate::State::State(QCryptographicHash::Algorithm method)
{
    if (method == QCryptographicHash::Keccak_224 ||
        method == QCryptographicHash::Keccak_256 ||
        method == QCryptographicHash::Keccak_384 ||
        method == QCryptographicHash::Keccak_512) {
        new (&sha3Context) SHA3Context;
        reset(method);
    } else if (method == QCryptographicHash::Blake2b_160 ||
               method == QCryptographicHash::Blake2b_256 ||
               method == QCryptographicHash::Blake2b_384) {
        new (&blake2bContext) blake2b_state;
        reset(method);
    } else if (method == QCryptographicHash::Blake2s_128 ||
               method == QCryptographicHash::Blake2s_160 ||
               method == QCryptographicHash::Blake2s_224) {
        new (&blake2sContext) blake2s_state;
        reset(method);
    } else {
        new (&evp) EVP(method);
    }
}

void QCryptographicHashPrivate::State::destroy(QCryptographicHash::Algorithm method)
{
    if (method != QCryptographicHash::Keccak_224 &&
        method != QCryptographicHash::Keccak_256 &&
        method != QCryptographicHash::Keccak_384 &&
        method != QCryptographicHash::Keccak_512 &&
        method != QCryptographicHash::Blake2b_160 &&
        method != QCryptographicHash::Blake2b_256 &&
        method != QCryptographicHash::Blake2b_384 &&
        method != QCryptographicHash::Blake2s_128 &&
        method != QCryptographicHash::Blake2s_160 &&
        method != QCryptographicHash::Blake2s_224) {
        evp.~EVP();
    }
}

QCryptographicHashPrivate::EVP::EVP(QCryptographicHash::Algorithm method)
    : initializationFailed{true}
{
    if (method == QCryptographicHash::Md4) {
        /*
         * We need to load the legacy provider in order to have the MD4
         * algorithm available.
         */
        legacyProvider = OSSL_PROVIDER_ptr(OSSL_PROVIDER_load(nullptr, "legacy"));

        if (!legacyProvider)
            return;
    }

    defaultProvider = OSSL_PROVIDER_ptr(OSSL_PROVIDER_load(nullptr, "default"));
    if (!defaultProvider)
        return;

    context = EVP_MD_CTX_ptr(EVP_MD_CTX_new());

    if (!context) {
        return;
    }

    /*
     * Using the "-fips" option will disable the global "fips=yes" for
     * this one lookup and the algorithm can be fetched from any provider
     * that implements the algorithm (including the FIPS provider).
     */
    algorithm = EVP_MD_ptr(EVP_MD_fetch(nullptr, methodToName(method), "-fips"));
    if (!algorithm) {
        return;
    }

    initializationFailed = !EVP_DigestInit_ex(context.get(), algorithm.get(), nullptr);
}

#else // USING_OPENSSL30

QCryptographicHashPrivate::State::State(QCryptographicHash::Algorithm method)
{
    switch (method) {
    case QCryptographicHash::Sha1:
        new (&sha1Context) Sha1State;
        break;
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case QCryptographicHash::Md4:
        new (&md4Context) md4_context;
        break;
    case QCryptographicHash::Md5:
        new (&md5Context) MD5Context;
        break;
    case QCryptographicHash::Sha224:
        new (&sha224Context) SHA224Context;
        break;
    case QCryptographicHash::Sha256:
        new (&sha256Context) SHA256Context;
        break;
    case QCryptographicHash::Sha384:
        new (&sha384Context) SHA384Context;
        break;
    case QCryptographicHash::Sha512:
        new (&sha512Context) SHA512Context;
        break;
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
        new (&sha3Context) SHA3Context;
        break;
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2b_384:
    case QCryptographicHash::Blake2b_512:
        new (&blake2bContext) blake2b_state;
        break;
    case QCryptographicHash::Blake2s_128:
    case QCryptographicHash::Blake2s_160:
    case QCryptographicHash::Blake2s_224:
    case QCryptographicHash::Blake2s_256:
        new (&blake2sContext) blake2s_state;
        break;
#endif
    case QCryptographicHash::NumAlgorithms:
        Q_UNREACHABLE();
    }
    reset(method);
}

void QCryptographicHashPrivate::State::destroy(QCryptographicHash::Algorithm)
{
    static_assert(std::is_trivially_destructible_v<State>); // so nothing to do here
}
#endif // !USING_OPENSSL30

void QCryptographicHashPrivate::reset() noexcept
{
    result.clear();
    state.reset(method);
}

#ifdef USING_OPENSSL30

void QCryptographicHashPrivate::State::reset(QCryptographicHash::Algorithm method) noexcept
{
    if (method == QCryptographicHash::Keccak_224 ||
        method == QCryptographicHash::Keccak_256 ||
        method == QCryptographicHash::Keccak_384 ||
        method == QCryptographicHash::Keccak_512) {
        sha3Init(&sha3Context, hashLengthInternal(method) * 8);
   } else if (method == QCryptographicHash::Blake2b_160 ||
              method == QCryptographicHash::Blake2b_256 ||
              method == QCryptographicHash::Blake2b_384) {
        blake2b_init(&blake2bContext, hashLengthInternal(method));
    } else if (method == QCryptographicHash::Blake2s_128 ||
               method == QCryptographicHash::Blake2s_160 ||
               method == QCryptographicHash::Blake2s_224) {
        blake2s_init(&blake2sContext, hashLengthInternal(method));
    } else {
        evp.reset();
    }
}

void QCryptographicHashPrivate::EVP::reset() noexcept
{
    if (!initializationFailed) {
        Q_ASSERT(context);
        Q_ASSERT(algorithm);
        // everything already set up - just reset the context
        EVP_MD_CTX_reset(context.get());
        initializationFailed = !EVP_DigestInit_ex(context.get(), algorithm.get(), nullptr);
    }
    // if initializationFailed first time around, it will not succeed this time, either
}

#else // USING_OPENSSL30

void QCryptographicHashPrivate::State::reset(QCryptographicHash::Algorithm method) noexcept
{
    switch (method) {
    case QCryptographicHash::Sha1:
        sha1InitState(&sha1Context);
        break;
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case QCryptographicHash::Md4:
        md4_init(&md4Context);
        break;
    case QCryptographicHash::Md5:
        MD5Init(&md5Context);
        break;
    case QCryptographicHash::Sha224:
        SHA224Reset(&sha224Context);
        break;
    case QCryptographicHash::Sha256:
        SHA256Reset(&sha256Context);
        break;
    case QCryptographicHash::Sha384:
        SHA384Reset(&sha384Context);
        break;
    case QCryptographicHash::Sha512:
        SHA512Reset(&sha512Context);
        break;
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
        sha3Init(&sha3Context, hashLengthInternal(method) * 8);
        break;
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2b_384:
    case QCryptographicHash::Blake2b_512:
        blake2b_init(&blake2bContext, hashLengthInternal(method));
        break;
    case QCryptographicHash::Blake2s_128:
    case QCryptographicHash::Blake2s_160:
    case QCryptographicHash::Blake2s_224:
    case QCryptographicHash::Blake2s_256:
        blake2s_init(&blake2sContext, hashLengthInternal(method));
        break;
#endif
    case QCryptographicHash::NumAlgorithms:
        Q_UNREACHABLE();
    }
}

#endif // USING_OPENSSL30

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
    state.addData(method, bytes);
    result.clear();
}

#ifdef USING_OPENSSL30

void QCryptographicHashPrivate::State::addData(QCryptographicHash::Algorithm method,
                                               QByteArrayView bytes) noexcept
{
    const char *data = bytes.data();
    auto length = bytes.size();
    // all functions take size_t length, so we don't need to loop around them:
    {
        if (method == QCryptographicHash::Keccak_224 ||
            method == QCryptographicHash::Keccak_256 ||
            method == QCryptographicHash::Keccak_384 ||
            method == QCryptographicHash::Keccak_512) {
            sha3Update(&sha3Context, reinterpret_cast<const BitSequence *>(data), uint64_t(length) * 8);
        } else if (method == QCryptographicHash::Blake2b_160 ||
                   method == QCryptographicHash::Blake2b_256 ||
                   method == QCryptographicHash::Blake2b_384) {
            blake2b_update(&blake2bContext, reinterpret_cast<const uint8_t *>(data), length);
        } else if (method == QCryptographicHash::Blake2s_128 ||
                method == QCryptographicHash::Blake2s_160 ||
                method == QCryptographicHash::Blake2s_224) {
            blake2s_update(&blake2sContext, reinterpret_cast<const uint8_t *>(data), length);
        } else if (!evp.initializationFailed) {
            EVP_DigestUpdate(evp.context.get(), (const unsigned char *)data, length);
        }
    }
}

#else // USING_OPENSSL30

void QCryptographicHashPrivate::State::addData(QCryptographicHash::Algorithm method,
                                               QByteArrayView bytes) noexcept
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
            sha1Update(&sha1Context, (const unsigned char *)data, length);
            break;
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        default:
            Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
            Q_UNREACHABLE();
            break;
#else
        case QCryptographicHash::Md4:
            md4_update(&md4Context, (const unsigned char *)data, length);
            break;
        case QCryptographicHash::Md5:
            MD5Update(&md5Context, (const unsigned char *)data, length);
            break;
        case QCryptographicHash::Sha224:
            SHA224Input(&sha224Context, reinterpret_cast<const unsigned char *>(data), length);
            break;
        case QCryptographicHash::Sha256:
            SHA256Input(&sha256Context, reinterpret_cast<const unsigned char *>(data), length);
            break;
        case QCryptographicHash::Sha384:
            SHA384Input(&sha384Context, reinterpret_cast<const unsigned char *>(data), length);
            break;
        case QCryptographicHash::Sha512:
            SHA512Input(&sha512Context, reinterpret_cast<const unsigned char *>(data), length);
            break;
        case QCryptographicHash::RealSha3_224:
        case QCryptographicHash::Keccak_224:
        case QCryptographicHash::RealSha3_256:
        case QCryptographicHash::Keccak_256:
        case QCryptographicHash::RealSha3_384:
        case QCryptographicHash::Keccak_384:
        case QCryptographicHash::RealSha3_512:
        case QCryptographicHash::Keccak_512:
            sha3Update(&sha3Context, reinterpret_cast<const BitSequence *>(data), uint64_t(length) * 8);
            break;
        case QCryptographicHash::Blake2b_160:
        case QCryptographicHash::Blake2b_256:
        case QCryptographicHash::Blake2b_384:
        case QCryptographicHash::Blake2b_512:
            blake2b_update(&blake2bContext, reinterpret_cast<const uint8_t *>(data), length);
            break;
        case QCryptographicHash::Blake2s_128:
        case QCryptographicHash::Blake2s_160:
        case QCryptographicHash::Blake2s_224:
        case QCryptographicHash::Blake2s_256:
            blake2s_update(&blake2sContext, reinterpret_cast<const uint8_t *>(data), length);
            break;
#endif
        case QCryptographicHash::NumAlgorithms:
            Q_UNREACHABLE();
        }
    }
}
#endif // !USING_OPENSSL30

/*!
  Reads the data from the open QIODevice \a device until it ends
  and hashes it. Returns \c true if reading was successful.
  \since 5.0
 */
bool QCryptographicHash::addData(QIODevice *device)
{
    return d->addData(device);
}

bool QCryptographicHashPrivate::addData(QIODevice *device)
{
    if (!device->isReadable())
        return false;

    if (!device->isOpen())
        return false;

    char buffer[1024];
    qint64 length;

    while ((length = device->read(buffer, sizeof(buffer))) > 0)
        addData({buffer, qsizetype(length)}); // length always <= 1024

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
    // resultView() is a const function, so concurrent calls are allowed; protect:
    d->finalize();
    // resultView() remains(!) valid even after we dropped the mutex in finalize()
    return d->resultView();
}

/*!
    \internal

    Calls finalizeUnchecked(), if needed, under finalizeMutex protection.
*/
void QCryptographicHashPrivate::finalize() noexcept
{
    const auto lock = qt_scoped_lock(finalizeMutex);
    // check that no other thread already finalizeUnchecked()'ed before us:
    if (!result.isEmpty())
        return;
    finalizeUnchecked();
}

/*!
    \internal

    Must be called with finalizeMutex held (except from static hash() function,
    where no sharing can take place).
*/
void QCryptographicHashPrivate::finalizeUnchecked() noexcept
{
    state.finalizeUnchecked(method, result);
}

#ifdef USING_OPENSSL30
void QCryptographicHashPrivate::State::finalizeUnchecked(QCryptographicHash::Algorithm method,
                                                         HashResult &result) noexcept
{
    if (method == QCryptographicHash::Keccak_224 ||
        method == QCryptographicHash::Keccak_256 ||
        method == QCryptographicHash::Keccak_384 ||
        method == QCryptographicHash::Keccak_512) {
        sha3Finish(result, 8 * hashLengthInternal(method), Sha3Variant::Keccak);
    } else if (method == QCryptographicHash::Blake2b_160 ||
               method == QCryptographicHash::Blake2b_256 ||
               method == QCryptographicHash::Blake2b_384) {
        const auto length = hashLengthInternal(method);
        blake2b_state copy = blake2bContext;
        result.resizeForOverwrite(length);
        blake2b_final(&copy, result.data(), length);
    } else if (method == QCryptographicHash::Blake2s_128 ||
               method == QCryptographicHash::Blake2s_160 ||
               method == QCryptographicHash::Blake2s_224) {
        const auto length = hashLengthInternal(method);
        blake2s_state copy = blake2sContext;
        result.resizeForOverwrite(length);
        blake2s_final(&copy, result.data(), length);
    } else {
        evp.finalizeUnchecked(result);
    }
}

void QCryptographicHashPrivate::EVP::finalizeUnchecked(HashResult &result) noexcept
{
    if (!initializationFailed) {
        EVP_MD_CTX_ptr copy = EVP_MD_CTX_ptr(EVP_MD_CTX_new());
        EVP_MD_CTX_copy_ex(copy.get(), context.get());
        result.resizeForOverwrite(EVP_MD_get_size(algorithm.get()));
        EVP_DigestFinal_ex(copy.get(), result.data(), nullptr);
    }
}

#else // USING_OPENSSL30

void QCryptographicHashPrivate::State::finalizeUnchecked(QCryptographicHash::Algorithm method,
                                                         HashResult &result) noexcept
{
    switch (method) {
    case QCryptographicHash::Sha1: {
        Sha1State copy = sha1Context;
        result.resizeForOverwrite(20);
        sha1FinalizeState(&copy);
        sha1ToHash(&copy, result.data());
        break;
    }
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    default:
        Q_ASSERT_X(false, "QCryptographicHash", "Method not compiled in");
        Q_UNREACHABLE();
        break;
#else
    case QCryptographicHash::Md4: {
        md4_context copy = md4Context;
        result.resizeForOverwrite(MD4_RESULTLEN);
        md4_final(&copy, result.data());
        break;
    }
    case QCryptographicHash::Md5: {
        MD5Context copy = md5Context;
        result.resizeForOverwrite(16);
        MD5Final(&copy, result.data());
        break;
    }
    case QCryptographicHash::Sha224: {
        SHA224Context copy = sha224Context;
        result.resizeForOverwrite(SHA224HashSize);
        SHA224Result(&copy, result.data());
        break;
    }
    case QCryptographicHash::Sha256: {
        SHA256Context copy = sha256Context;
        result.resizeForOverwrite(SHA256HashSize);
        SHA256Result(&copy, result.data());
        break;
    }
    case QCryptographicHash::Sha384: {
        SHA384Context copy = sha384Context;
        result.resizeForOverwrite(SHA384HashSize);
        SHA384Result(&copy, result.data());
        break;
    }
    case QCryptographicHash::Sha512: {
        SHA512Context copy = sha512Context;
        result.resizeForOverwrite(SHA512HashSize);
        SHA512Result(&copy, result.data());
        break;
    }
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::RealSha3_512: {
        sha3Finish(result, 8 * hashLengthInternal(method), Sha3Variant::Sha3);
        break;
    }
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::Keccak_512: {
        sha3Finish(result, 8 * hashLengthInternal(method), Sha3Variant::Keccak);
        break;
    }
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2b_384:
    case QCryptographicHash::Blake2b_512: {
        const auto length = hashLengthInternal(method);
        blake2b_state copy = blake2bContext;
        result.resizeForOverwrite(length);
        blake2b_final(&copy, result.data(), length);
        break;
    }
    case QCryptographicHash::Blake2s_128:
    case QCryptographicHash::Blake2s_160:
    case QCryptographicHash::Blake2s_224:
    case QCryptographicHash::Blake2s_256: {
        const auto length = hashLengthInternal(method);
        blake2s_state copy = blake2sContext;
        result.resizeForOverwrite(length);
        blake2s_final(&copy, result.data(), length);
        break;
    }
#endif
    case QCryptographicHash::NumAlgorithms:
        Q_UNREACHABLE();
    }
}
#endif // !USING_OPENSSL30

/*!
  Returns the hash of \a data using \a method.

  \note In Qt versions prior to 6.3, this function took QByteArray,
  not QByteArrayView.
*/
QByteArray QCryptographicHash::hash(QByteArrayView data, Algorithm method)
{
    QCryptographicHashPrivate hash(method);
    hash.addData(data);
    hash.finalizeUnchecked(); // no mutex needed: no-one but us has access to 'hash'
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

/*!
  Returns whether the selected algorithm \a method is supported and if
  result() will return a value when the \a method is used.

  \note OpenSSL will be responsible for providing this information when
  used as a provider, otherwise \c true will be returned as the non-OpenSSL
  implementation doesn't have any restrictions.
  We return \c false if we fail to query OpenSSL.

  \since 6.5
*/


bool QCryptographicHash::supportsAlgorithm(QCryptographicHash::Algorithm method)
{
    return QCryptographicHashPrivate::supportsAlgorithm(method);
}

bool QCryptographicHashPrivate::supportsAlgorithm(QCryptographicHash::Algorithm method)
{
#ifdef USING_OPENSSL30
    // OpenSSL doesn't support Blake2b{60,236,384} and Blake2s{128,160,224}
    // and these would automatically return FALSE in that case, while they are
    // actually supported by our non-OpenSSL implementation.
    if (useNonOpenSSLFallback(method))
        return true;

    auto legacyProvider = OSSL_PROVIDER_ptr(OSSL_PROVIDER_load(nullptr, "legacy"));
    auto defaultProvider = OSSL_PROVIDER_ptr(OSSL_PROVIDER_load(nullptr, "default"));

    const char *restriction = "-fips";
    EVP_MD_ptr algorithm = EVP_MD_ptr(EVP_MD_fetch(nullptr, methodToName(method), restriction));

    return algorithm != nullptr;
#else
    switch (method) {
    case QCryptographicHash::Sha1:
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    case QCryptographicHash::Md4:
    case QCryptographicHash::Md5:
    case QCryptographicHash::Sha224:
    case QCryptographicHash::Sha256:
    case QCryptographicHash::Sha384:
    case QCryptographicHash::Sha512:
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
    case QCryptographicHash::Blake2b_160:
    case QCryptographicHash::Blake2b_256:
    case QCryptographicHash::Blake2b_384:
    case QCryptographicHash::Blake2b_512:
    case QCryptographicHash::Blake2s_128:
    case QCryptographicHash::Blake2s_160:
    case QCryptographicHash::Blake2s_224:
    case QCryptographicHash::Blake2s_256:
#endif
        return true;
    case QCryptographicHash::NumAlgorithms: ;
    };
    return false;
#endif // !USING_OPENSSL3
}

static constexpr int qt_hash_block_size(QCryptographicHash::Algorithm method)
{
    switch (method) {
    case QCryptographicHash::Sha1:
        return SHA1_Message_Block_Size;
#ifndef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    case QCryptographicHash::Md4:
        return 64;
    case QCryptographicHash::Md5:
        return 64;
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
#endif // QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    case QCryptographicHash::NumAlgorithms:
#if !defined(Q_CC_GNU_ONLY) || Q_CC_GNU >= 900
        // GCC 8 has trouble with Q_UNREACHABLE() in constexpr functions
        Q_UNREACHABLE();
#endif
        break;
    }
    return 0;
}

constexpr int maxHashBlockSize()
{
    int result = 0;
    using A = QCryptographicHash::Algorithm;
    for (int i = 0; i < A::NumAlgorithms ; ++i)
        result = std::max(result, qt_hash_block_size(A(i)));
    return result;
}

[[maybe_unused]]
constexpr int minHashBlockSize()
{
    int result = INT_MAX;
    using A = QCryptographicHash::Algorithm;
    for (int i = 0; i < A::NumAlgorithms ; ++i)
        result = std::min(result, qt_hash_block_size(A(i)));
    return result;
}

[[maybe_unused]]
constexpr int gcdHashBlockSize()
{
    int result = 0;
    using A = QCryptographicHash::Algorithm;
    for (int i = 0; i < A::NumAlgorithms ; ++i)
        result = std::gcd(result, qt_hash_block_size(A(i)));
    return result;
}

using HashBlock = QSmallByteArray<maxHashBlockSize()>;

static HashBlock xored(const HashBlock &block, quint8 val) noexcept
{
    // some hints for the optimizer:
    Q_ASSUME(block.size() >= minHashBlockSize());
    Q_ASSUME(block.size() <= maxHashBlockSize());
    Q_ASSUME(block.size() % gcdHashBlockSize() == 0);
    HashBlock result;
    result.resizeForOverwrite(block.size());
    for (qsizetype i = 0; i < block.size(); ++i)
        result[i] = block[i] ^ val;
    return result;
}

class QMessageAuthenticationCodePrivate
{
public:
    QMessageAuthenticationCodePrivate(QCryptographicHash::Algorithm m)
        : messageHash(m)
    {
    }

    HashBlock key;
    QCryptographicHashPrivate messageHash;

    void setKey(QByteArrayView k) noexcept;
    void initMessageHash() noexcept;
    void finalize();

    // when not called from the static hash() function, this function needs to be
    // called with messageHash.finalizeMutex held:
    void finalizeUnchecked() noexcept;
    // END functions that need to be called with finalizeMutex held
};

/*!
    \internal

    Transforms key \a newKey into a block-sized format and stores it in member
    \c key.

    This function assumes it can use messageHash (i.e. it's in its initial
    state (reset() has been called)).
*/
void QMessageAuthenticationCodePrivate::setKey(QByteArrayView newKey) noexcept
{
    const int blockSize = qt_hash_block_size(messageHash.method);

    if (newKey.size() > blockSize) {
        messageHash.addData(newKey);
        messageHash.finalizeUnchecked();
        static_assert([] {
                using A = QCryptographicHash::Algorithm;
                for (int i = 0; i < A::NumAlgorithms; ++i) {
                    if (hashLengthInternal(A(i)) > qt_hash_block_size(A(i)))
                        return false;
                }
                return true;
            }(), "this code assumes that a hash's result always fits into that hash's block size");
        key = messageHash.result;
        messageHash.reset();
    } else {
        key.assign(newKey);
    }

    if (key.size() < blockSize)
        key.resize(blockSize, '\0');

    initMessageHash();
}

/*!
    \internal

    Seeds messageHash from \c key.

    This function assumes that messageHash is in its initial state (reset() has
    been called).
*/
void QMessageAuthenticationCodePrivate::initMessageHash() noexcept
{
    messageHash.addData(xored(key, 0x36));
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

//! [qba-to-qbav-6.6]
    \note In Qt versions prior to 6.6, this function took its arguments as
    QByteArray, not QByteArrayView. If you experience compile errors, it's
    because your code is passing objects that are implicitly convertible to
    QByteArray, but not QByteArrayView. Wrap the corresponding argument in
    \c{QByteArray{~~~}} to make the cast explicit. This is backwards-compatible
    with old Qt versions.
//! [qba-to-qbav-6.6]
*/
QMessageAuthenticationCode::QMessageAuthenticationCode(QCryptographicHash::Algorithm method,
                                                       QByteArrayView key)
    : d(new QMessageAuthenticationCodePrivate(method))
{
    d->setKey(key);
}

/*!
    Destroys the object.
*/
QMessageAuthenticationCode::~QMessageAuthenticationCode()
{
    delete d;
}

/*!
    \fn QMessageAuthenticationCode::QMessageAuthenticationCode(QMessageAuthenticationCode &&other)

    Move-constructs a new QMessageAuthenticationCode from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new object.

    \since 6.6
*/

/*!
    \fn QMessageAuthenticationCode &QMessageAuthenticationCode::operator=(QMessageAuthenticationCode &&other)

    Move-assigns \a other to this QMessageAuthenticationCode instance.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new object.

    \since 6.6
*/

/*!
    \fn void QMessageAuthenticationCode::swap(QMessageAuthenticationCode &other)

    Swaps message authentication code \a other with this message authentication
    code. This operation is very fast and never fails.

    \since 6.6
*/

/*!
    Resets message data. Calling this method doesn't affect the key.
*/
void QMessageAuthenticationCode::reset() noexcept
{
    d->messageHash.reset();
    d->initMessageHash();
}

/*!
    Sets secret \a key. Calling this method automatically resets the object state.

    For optimal performance, call this method only to \e change the active key,
    not to set an \e initial key, as in

    \code
    QMessageAuthenticationCode mac(method);
    mac.setKey(key); // does extra work
    use(mac);
    \endcode

    Prefer to pass initial keys as the constructor argument:

    \code
    QMessageAuthenticationCode mac(method, key); // OK, optimal
    use(mac);
    \endcode

    You can use std::optional to delay construction of a
    QMessageAuthenticationCode until you know the key:

    \code
    std::optional<QMessageAuthenticationCode> mac;
    ~~~
    key = ~~~;
    mac.emplace(method, key);
    use(*mac);
    \endcode

    \include qcryptographichash.cpp {qba-to-qbav-6.6}
*/
void QMessageAuthenticationCode::setKey(QByteArrayView key) noexcept
{
    d->messageHash.reset();
    d->setKey(key);
}

/*!
    \overload
    Adds the first \a length chars of \a data to the message.
*/
void QMessageAuthenticationCode::addData(const char *data, qsizetype length)
{
    d->messageHash.addData({data, length});
}

/*!
    Adds \a data to the message.

    \include qcryptographichash.cpp {qba-to-qbav-6.6}

    \sa resultView(), result()
*/
void QMessageAuthenticationCode::addData(QByteArrayView data) noexcept
{
    d->messageHash.addData(data);
}

/*!
    Reads the data from the open QIODevice \a device until it ends
    and adds it to message. Returns \c true if reading was successful.

    \note \a device must be already opened.
 */
bool QMessageAuthenticationCode::addData(QIODevice *device)
{
    return d->messageHash.addData(device);
}

/*!
    \since 6.6

    Returns the final hash value.

    Note that the returned view remains valid only as long as the
    QMessageAuthenticationCode object is not modified by other means.

    \sa result()
*/
QByteArrayView QMessageAuthenticationCode::resultView() const noexcept
{
    d->finalize();
    return d->messageHash.resultView();
}

/*!
    Returns the final authentication code.

    \sa resultView(), QByteArray::toHex()
*/
QByteArray QMessageAuthenticationCode::result() const
{
    return resultView().toByteArray();
}

void QMessageAuthenticationCodePrivate::finalize()
{
    const auto lock = qt_scoped_lock(messageHash.finalizeMutex);
    if (!messageHash.result.isEmpty())
        return;
    finalizeUnchecked();
}

void QMessageAuthenticationCodePrivate::finalizeUnchecked() noexcept
{
    messageHash.finalizeUnchecked();
    const HashResult hashedMessage = messageHash.result;

    messageHash.reset();
    messageHash.addData(xored(key, 0x5c));
    messageHash.addData(hashedMessage);
    messageHash.finalizeUnchecked();
}

/*!
    Returns the authentication code for the message \a message using
    the key \a key and the method \a method.

    \include qcryptographichash.cpp {qba-to-qbav-6.6}
*/
QByteArray QMessageAuthenticationCode::hash(QByteArrayView message, QByteArrayView key,
                                            QCryptographicHash::Algorithm method)
{
    QMessageAuthenticationCodePrivate mac(method);
    mac.setKey(key);
    mac.messageHash.addData(message);
    mac.finalizeUnchecked();
    return mac.messageHash.resultView().toByteArray();
}

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qcryptographichash.cpp"
#endif
