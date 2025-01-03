// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// for rand_s, _CRT_RAND_S must be #defined before #including stdlib.h.
// put it at the beginning so some indirect inclusion doesn't break it
#ifndef _CRT_RAND_S
#define _CRT_RAND_S
#endif
#include <stdlib.h>
#include <stdint.h>

#include "qhash.h"

#ifdef truncate
#undef truncate
#endif

#include <qbitarray.h>
#include <qstring.h>
#include <qglobal.h>
#include <qbytearray.h>
#include <qdatetime.h>
#include <qbasicatomic.h>
#include <qendian.h>
#include <private/qrandom_p.h>
#include <private/qsimd_p.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#include <qrandom.h>
#include <private/qlocale_tools_p.h>
#endif // QT_BOOTSTRAPPED

#include <array>
#include <limits.h>

#if defined(QT_NO_DEBUG) && !defined(NDEBUG)
#  define NDEBUG
#endif
#include <assert.h>

#ifdef Q_CC_GNU
#  define Q_DECL_HOT_FUNCTION       __attribute__((hot))
#else
#  define Q_DECL_HOT_FUNCTION
#endif

QT_BEGIN_NAMESPACE

// We assume that pointers and size_t have the same size. If that assumption should fail
// on a platform the code selecting the different methods below needs to be fixed.
static_assert(sizeof(size_t) == QT_POINTER_SIZE, "size_t and pointers have different size.");

namespace {
struct HashSeedStorage
{
    static constexpr int SeedCount = 2;
    QBasicAtomicInteger<quintptr> seeds[SeedCount] = { Q_BASIC_ATOMIC_INITIALIZER(0), Q_BASIC_ATOMIC_INITIALIZER(0) };

#if !QT_SUPPORTS_INIT_PRIORITY || defined(QT_BOOTSTRAPPED)
    constexpr HashSeedStorage() = default;
#else
    HashSeedStorage() { initialize(0); }
#endif

    enum State {
        OverriddenByEnvironment = -1,
        JustInitialized,
        AlreadyInitialized
    };
    struct StateResult {
        quintptr requestedSeed;
        State state;
    };

    StateResult state(int which = -1);
    Q_DECL_HOT_FUNCTION QHashSeed currentSeed(int which)
    {
        return { state(which).requestedSeed };
    }

    void resetSeed()
    {
        if (state().state < AlreadyInitialized)
            return;

        // update the public seed
        QRandomGenerator *generator = QRandomGenerator::system();
        seeds[0].storeRelaxed(sizeof(size_t) > sizeof(quint32)
                              ? generator->generate64() : generator->generate());
    }

    void clearSeed()
    {
        state();
        seeds[0].storeRelaxed(0);   // always write (smaller code)
    }

private:
    Q_DECL_COLD_FUNCTION Q_NEVER_INLINE StateResult initialize(int which) noexcept;
};

[[maybe_unused]] HashSeedStorage::StateResult HashSeedStorage::initialize(int which) noexcept
{
    StateResult result = { 0, OverriddenByEnvironment };
#ifdef QT_BOOTSTRAPPED
    Q_UNUSED(which);
    Q_UNREACHABLE_RETURN(result);
#else
    // can't use qEnvironmentVariableIntValue (reentrancy)
    const char *seedstr = getenv("QT_HASH_SEED");
    if (seedstr) {
        auto r = qstrntoll(seedstr, strlen(seedstr), 10);
        if (r.used > 0 && size_t(r.used) == strlen(seedstr)) {
            if (r.result) {
                // can't use qWarning here (reentrancy)
                fprintf(stderr, "QT_HASH_SEED: forced seed value is not 0; ignored.\n");
            }

            // we don't have to store to the seed, since it's pre-initialized by
            // the compiler to zero
            return result;
        }
    }

    // update the full seed
    auto x = qt_initial_random_value();
    for (int i = 0; i < SeedCount; ++i) {
        seeds[i].storeRelaxed(x.data[i]);
        if (which == i)
            result.requestedSeed = x.data[i];
    }
    result.state = JustInitialized;
    return result;
#endif
}

inline HashSeedStorage::StateResult HashSeedStorage::state(int which)
{
    constexpr quintptr BadSeed = quintptr(Q_UINT64_C(0x5555'5555'5555'5555));
    StateResult result = { BadSeed, AlreadyInitialized };

#if defined(QT_BOOTSTRAPPED)
    result = { 0, OverriddenByEnvironment };
#elif !QT_SUPPORTS_INIT_PRIORITY
    // dynamic initialization
    static auto once = [&]() {
        result = initialize(which);
        return true;
    }();
    Q_UNUSED(once);
#endif

    if (result.state == AlreadyInitialized && which >= 0)
        return { seeds[which].loadRelaxed(), AlreadyInitialized };
    return result;
}
} // unnamed namespace

/*
    The QHash seed itself.
*/
#ifdef Q_DECL_INIT_PRIORITY
Q_DECL_INIT_PRIORITY(05)
#else
Q_CONSTINIT
#endif
static HashSeedStorage qt_qhash_seed;

/*
 * Hashing for memory segments is based on the public domain MurmurHash2 by
 * Austin Appleby. See http://murmurhash.googlepages.com/
 */
#if QT_POINTER_SIZE == 4
Q_NEVER_INLINE Q_DECL_HOT_FUNCTION
static inline uint murmurhash(const void *key, uint len, uint seed) noexcept
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value

    unsigned int h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const unsigned char *data = reinterpret_cast<const unsigned char *>(key);
    const unsigned char *end = data + (len & ~3);

    while (data != end) {
        size_t k;
        memcpy(&k, data, sizeof(uint));

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
    }

    // Handle the last few bytes of the input array
    len &= 3;
    if (len) {
        unsigned int k = 0;
        end += len;

        while (data != end) {
            k <<= 8;
            k |= *data;
            ++data;
        }
        h ^= k;
        h *= m;
    }

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

#else
Q_NEVER_INLINE Q_DECL_HOT_FUNCTION
static inline uint64_t murmurhash(const void *key, uint64_t len, uint64_t seed) noexcept
{
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const unsigned char *data = reinterpret_cast<const unsigned char *>(key);
    const unsigned char *end = data + (len & ~7ul);

    while (data != end) {
        uint64_t k;
        memcpy(&k, data, sizeof(uint64_t));

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;

        data += 8;
    }

    len &= 7;
    if (len) {
        // handle the last few bytes of input
        size_t k = 0;
        end += len;

        while (data != end) {
            k <<= 8;
            k |= *data;
            ++data;
        }
        h ^= k;
        h *= m;
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

#endif

#if QT_POINTER_SIZE == 8
// This is an inlined version of the SipHash implementation that is
// trying to avoid some memcpy's from uint64 to uint8[] and back.
//

// Use SipHash-1-2, which has similar performance characteristics as
// stablehash() above, instead of the SipHash-2-4 default
#define cROUNDS 1
#define dROUNDS 2

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define SIPROUND                                                               \
  do {                                                                         \
    v0 += v1;                                                                  \
    v1 = ROTL(v1, 13);                                                         \
    v1 ^= v0;                                                                  \
    v0 = ROTL(v0, 32);                                                         \
    v2 += v3;                                                                  \
    v3 = ROTL(v3, 16);                                                         \
    v3 ^= v2;                                                                  \
    v0 += v3;                                                                  \
    v3 = ROTL(v3, 21);                                                         \
    v3 ^= v0;                                                                  \
    v2 += v1;                                                                  \
    v1 = ROTL(v1, 17);                                                         \
    v1 ^= v2;                                                                  \
    v2 = ROTL(v2, 32);                                                         \
  } while (0)

Q_NEVER_INLINE Q_DECL_HOT_FUNCTION
static uint64_t siphash(const uint8_t *in, uint64_t inlen, uint64_t seed, uint64_t seed2)
{
    /* "somepseudorandomlygeneratedbytes" */
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t b;
    uint64_t k0 = seed;
    uint64_t k1 = seed2;
    int i;
    const uint8_t *end = in + (inlen & ~7ULL);
    const int left = inlen & 7;
    b = inlen << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; in != end; in += 8) {
        uint64_t m = qFromUnaligned<uint64_t>(in);
        v3 ^= m;

        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }


#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 700
    QT_WARNING_DISABLE_GCC("-Wimplicit-fallthrough")
#endif
    switch (left) {
    case 7:
        b |= ((uint64_t)in[6]) << 48;
    case 6:
        b |= ((uint64_t)in[5]) << 40;
    case 5:
        b |= ((uint64_t)in[4]) << 32;
    case 4:
        b |= ((uint64_t)in[3]) << 24;
    case 3:
        b |= ((uint64_t)in[2]) << 16;
    case 2:
        b |= ((uint64_t)in[1]) << 8;
    case 1:
        b |= ((uint64_t)in[0]);
        break;
    case 0:
        break;
    }

    v3 ^= b;

    for (i = 0; i < cROUNDS; ++i)
        SIPROUND;

    v0 ^= b;

    v2 ^= 0xff;

    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    return b;
}
#else
// This is a "SipHash" implementation adopted for 32bit platforms. It performs
// basically the same operations as the 64bit version using 4 byte at a time
// instead of 8.
//
// To make this work, we also need to change the constants for the mixing
// rotations in ROTL. We're simply using half of the 64bit constants, rounded up
// for odd numbers.
//
// For the v0-v4 constants, simply use the first four bytes of the 64 bit versions.
//
// Use SipHash-1-2, which has similar performance characteristics as
// stablehash() above, instead of the SipHash-2-4 default
#define cROUNDS 1
#define dROUNDS 2

#define ROTL(x, b) (uint32_t)(((x) << (b)) | ((x) >> (32 - (b))))

#define SIPROUND                                                               \
  do {                                                                         \
    v0 += v1;                                                                  \
    v1 = ROTL(v1, 7);                                                          \
    v1 ^= v0;                                                                  \
    v0 = ROTL(v0, 16);                                                         \
    v2 += v3;                                                                  \
    v3 = ROTL(v3, 8);                                                          \
    v3 ^= v2;                                                                  \
    v0 += v3;                                                                  \
    v3 = ROTL(v3, 11);                                                         \
    v3 ^= v0;                                                                  \
    v2 += v1;                                                                  \
    v1 = ROTL(v1, 9);                                                          \
    v1 ^= v2;                                                                  \
    v2 = ROTL(v2, 16);                                                         \
  } while (0)

Q_NEVER_INLINE Q_DECL_HOT_FUNCTION
static uint siphash(const uint8_t *in, uint inlen, uint seed, uint seed2)
{
    /* "somepseudorandomlygeneratedbytes" */
    uint v0 = 0x736f6d65U;
    uint v1 = 0x646f7261U;
    uint v2 = 0x6c796765U;
    uint v3 = 0x74656462U;
    uint b;
    uint k0 = seed;
    uint k1 = seed2;
    int i;
    const uint8_t *end = in + (inlen & ~3ULL);
    const int left = inlen & 3;
    b = inlen << 24;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; in != end; in += 4) {
        uint m = qFromUnaligned<uint>(in);
        v3 ^= m;

        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }

#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 700
    QT_WARNING_DISABLE_GCC("-Wimplicit-fallthrough")
#endif
    switch (left) {
    case 3:
        b |= ((uint)in[2]) << 16;
    case 2:
        b |= ((uint)in[1]) << 8;
    case 1:
        b |= ((uint)in[0]);
        break;
    case 0:
        break;
    }

    v3 ^= b;

    for (i = 0; i < cROUNDS; ++i)
        SIPROUND;

    v0 ^= b;

    v2 ^= 0xff;

    for (i = 0; i < dROUNDS; ++i)
        SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    return b;
}
#endif

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)  // GCC
#  define QHASH_AES_SANITIZER_BUILD
#elif __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)  // Clang
#  define QHASH_AES_SANITIZER_BUILD
#endif

// When built with a sanitizer, aeshash() is rightfully reported to have a
// heap-buffer-overflow issue. However, we consider it to be safe in this
// specific case and overcome the problem by correctly discarding the
// out-of-range bits. To allow building the code with sanitizer,
// QHASH_AES_SANITIZER_BUILD is used to disable aeshash() usage.
#if QT_COMPILER_SUPPORTS_HERE(AES) && QT_COMPILER_SUPPORTS_HERE(SSE4_2) && \
    !defined(QHASH_AES_SANITIZER_BUILD)
#  define AESHASH
#  define QT_FUNCTION_TARGET_STRING_AES_AVX2    "avx2,aes"
#  define QT_FUNCTION_TARGET_STRING_AES_AVX512          \
    QT_FUNCTION_TARGET_STRING_ARCH_SKYLAKE_AVX512 ","   \
    QT_FUNCTION_TARGET_STRING_AES
#  define QT_FUNCTION_TARGET_STRING_VAES_AVX512         \
    QT_FUNCTION_TARGET_STRING_ARCH_SKYLAKE_AVX512 ","   \
    QT_FUNCTION_TARGET_STRING_VAES
#  undef QHASH_AES_SANITIZER_BUILD
#  if QT_POINTER_SIZE == 8
#    define mm_set1_epz     _mm_set1_epi64x
#    define mm_cvtsz_si128  _mm_cvtsi64_si128
#    define mm_cvtsi128_sz  _mm_cvtsi128_si64
#    define mm256_set1_epz  _mm256_set1_epi64x
#  else
#    define mm_set1_epz     _mm_set1_epi32
#    define mm_cvtsz_si128  _mm_cvtsi32_si128
#    define mm_cvtsi128_sz  _mm_cvtsi128_si32
#    define mm256_set1_epz  _mm256_set1_epi32
#  endif

namespace {
    // This is inspired by the algorithm in the Go language. See:
    // https://github.com/golang/go/blob/01b6cf09fc9f272d9db3d30b4c93982f4911d120/src/runtime/asm_amd64.s#L1105
    // https://github.com/golang/go/blob/01b6cf09fc9f272d9db3d30b4c93982f4911d120/src/runtime/asm_386.s#L908
    //
    // Even though we're using the AESENC instruction from the CPU, this code
    // is not encryption and this routine makes no claim to be
    // cryptographically secure. We're simply using the instruction that performs
    // the scrambling round (step 3 in [1]) because it's just very good at
    // spreading the bits around.
    //
    // [1] https://en.wikipedia.org/wiki/Advanced_Encryption_Standard#High-level_description_of_the_algorithm

    // hash 16 bytes, running 3 scramble rounds of AES on itself (like label "final1")
    static void QT_FUNCTION_TARGET(AES) QT_VECTORCALL
    hash16bytes(__m128i &state0, __m128i data)
    {
        state0 = _mm_xor_si128(state0, data);
        state0 = _mm_aesenc_si128(state0, state0);
        state0 = _mm_aesenc_si128(state0, state0);
        state0 = _mm_aesenc_si128(state0, state0);
    }

    // hash twice 16 bytes, running 2 scramble rounds of AES on itself
    static void QT_FUNCTION_TARGET(AES) QT_VECTORCALL
    hash2x16bytes(__m128i &state0, __m128i &state1, const __m128i *src0, const __m128i *src1)
    {
        __m128i data0 = _mm_loadu_si128(src0);
        __m128i data1 = _mm_loadu_si128(src1);
        state0 = _mm_xor_si128(data0, state0);
        state1 = _mm_xor_si128(data1, state1);
        state0 = _mm_aesenc_si128(state0, state0);
        state1 = _mm_aesenc_si128(state1, state1);
        state0 = _mm_aesenc_si128(state0, state0);
        state1 = _mm_aesenc_si128(state1, state1);
    }

    struct AESHashSeed
    {
        __m128i state0;
        __m128i mseed2;
        AESHashSeed(size_t seed, size_t seed2) QT_FUNCTION_TARGET(AES);
        __m128i state1() const QT_FUNCTION_TARGET(AES);
        __m256i state0_256() const QT_FUNCTION_TARGET(AES_AVX2)
        { return _mm256_set_m128i(state1(), state0); }
    };
} // unnamed namespace

Q_ALWAYS_INLINE AESHashSeed::AESHashSeed(size_t seed, size_t seed2)
{
    __m128i mseed = mm_cvtsz_si128(seed);
    mseed2 = mm_set1_epz(seed2);

    // mseed (epi16) = [ seed, seed >> 16, seed >> 32, seed >> 48, len, 0, 0, 0 ]
    mseed = _mm_insert_epi16(mseed, short(seed), 4);
    // mseed (epi16) = [ seed, seed >> 16, seed >> 32, seed >> 48, len, len, len, len ]
    mseed = _mm_shufflehi_epi16(mseed, 0);

    // merge with the process-global seed
    __m128i key = _mm_xor_si128(mseed, mseed2);

    // scramble the key
    __m128i state0 = _mm_aesenc_si128(key, key);
    this->state0 = state0;
}

Q_ALWAYS_INLINE __m128i AESHashSeed::state1() const
{
    {
        // unlike the Go code, we don't have more per-process seed
        __m128i state1 = _mm_aesenc_si128(state0, mseed2);
        return state1;
    }
}

static size_t QT_FUNCTION_TARGET(AES) QT_VECTORCALL
aeshash128_16to32(__m128i state0, __m128i state1, const __m128i *src, const __m128i *srcend)
{
    {
        if (src + 1 < srcend) {
            // epilogue: between 16 and 31 bytes
            hash2x16bytes(state0, state1, src, srcend - 1);
        } else if (src != srcend) {
            // epilogue: between 1 and 16 bytes, overlap with the end
            __m128i data = _mm_loadu_si128(srcend - 1);
            hash16bytes(state0, data);
        }

        // combine results:
        state0 = _mm_xor_si128(state0, state1);
    }

    return mm_cvtsi128_sz(state0);
}

static size_t QT_FUNCTION_TARGET(AES) QT_VECTORCALL
aeshash128_lt16(__m128i state0, const uchar *p, size_t len)
{
    if (len) {
        // We're going to load 16 bytes and mask zero the part we don't care
        // (the hash of a short string is different from the hash of a longer
        // including NULLs at the end because the length is in the key)
        // WARNING: this may produce valgrind warnings, but it's safe

        constexpr quintptr PageSize = 4096;
        __m128i data;

        if ((quintptr(p) & (PageSize / 2)) == 0) {
            // lower half of the page:
            // load all 16 bytes and mask off the bytes past the end of the source
            static const qint8 maskarray[] = {
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            };
            __m128i mask = _mm_loadu_si128(reinterpret_cast<const __m128i *>(maskarray + 15 - len));
            data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
            data = _mm_and_si128(data, mask);
        } else {
            // upper half of the page:
            // load 16 bytes ending at the data end, then shuffle them to the beginning
            static const qint8 shufflecontrol[] = {
                1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
            };
            __m128i control = _mm_loadu_si128(reinterpret_cast<const __m128i *>(shufflecontrol + 15 - len));
            data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p + len) - 1);
            data = _mm_shuffle_epi8(data, control);
        }

        hash16bytes(state0, data);
    }
    return mm_cvtsi128_sz(state0);
}

static size_t QT_FUNCTION_TARGET(AES) QT_VECTORCALL
aeshash128_ge32(__m128i state0, __m128i state1, const __m128i *src, const __m128i *srcend)
{
    // main loop: scramble two 16-byte blocks
    for ( ; src + 2 < srcend; src += 2)
        hash2x16bytes(state0, state1, src, src + 1);

    return aeshash128_16to32(state0, state1, src, srcend);
}

#  if QT_COMPILER_SUPPORTS_HERE(VAES)
static size_t QT_FUNCTION_TARGET(ARCH_ICL) QT_VECTORCALL
aeshash256_lt32_avx256(__m256i state0, const uchar *p, size_t len)
{
    __m128i state0_128 = _mm256_castsi256_si128(state0);
    if (len) {
        __mmask32 mask = _bzhi_u32(-1, unsigned(len));
        __m256i data = _mm256_maskz_loadu_epi8(mask, p);
        __m128i data0 = _mm256_castsi256_si128(data);
        if (len >= sizeof(__m128i)) {
            state0 = _mm256_xor_si256(state0, data);
            state0 = _mm256_aesenc_epi128(state0, state0);
            state0 = _mm256_aesenc_epi128(state0, state0);
            // we're XOR'ing the two halves so we skip the third AESENC
            // state0 = _mm256_aesenc_epi128(state0, state0);

            // XOR the two halves and extract
            __m128i low = _mm256_extracti128_si256(state0, 0);
            __m128i high = _mm256_extracti128_si256(state0, 1);
            state0_128 = _mm_xor_si128(low, high);
        } else {
            hash16bytes(state0_128, data0);
        }
    }
    return mm_cvtsi128_sz(state0_128);
}

static size_t QT_FUNCTION_TARGET(VAES) QT_VECTORCALL
aeshash256_ge32(__m256i state0, const uchar *p, size_t len)
{
    static const auto hash32bytes = [](__m256i &state0, __m256i data) QT_FUNCTION_TARGET(VAES) {
        state0 = _mm256_xor_si256(state0, data);
        state0 = _mm256_aesenc_epi128(state0, state0);
        state0 = _mm256_aesenc_epi128(state0, state0);
        state0 = _mm256_aesenc_epi128(state0, state0);
    };

    // hash twice 32 bytes, running 2 scramble rounds of AES on itself
    const auto hash2x32bytes = [](__m256i &state0, __m256i &state1, const __m256i *src0,
            const __m256i *src1) QT_FUNCTION_TARGET(VAES) {
        __m256i data0 = _mm256_loadu_si256(src0);
        __m256i data1 = _mm256_loadu_si256(src1);
        state0 = _mm256_xor_si256(data0, state0);
        state1 = _mm256_xor_si256(data1, state1);
        state0 = _mm256_aesenc_epi128(state0, state0);
        state1 = _mm256_aesenc_epi128(state1, state1);
        state0 = _mm256_aesenc_epi128(state0, state0);
        state1 = _mm256_aesenc_epi128(state1, state1);
    };

    const __m256i *src = reinterpret_cast<const __m256i *>(p);
    const __m256i *srcend = reinterpret_cast<const __m256i *>(p + len);

    __m256i state1 = _mm256_aesenc_epi128(state0, mm256_set1_epz(len));

    // main loop: scramble two 32-byte blocks
    for ( ; src + 2 < srcend; src += 2)
        hash2x32bytes(state0, state1, src, src + 1);

    if (src + 1 < srcend) {
        // epilogue: between 32 and 31 bytes
        hash2x32bytes(state0, state1, src, srcend - 1);
    } else if (src != srcend) {
        // epilogue: between 1 and 32 bytes, overlap with the end
        __m256i data = _mm256_loadu_si256(srcend - 1);
        hash32bytes(state0, data);
    }

    // combine results:
    state0 = _mm256_xor_si256(state0, state1);

    // XOR the two halves and extract
    __m128i low = _mm256_extracti128_si256(state0, 0);
    __m128i high = _mm256_extracti128_si256(state0, 1);
    return mm_cvtsi128_sz(_mm_xor_si128(low, high));
}

static size_t QT_FUNCTION_TARGET(VAES)
aeshash256(const uchar *p, size_t len, size_t seed, size_t seed2) noexcept
{
    AESHashSeed state(seed, seed2);
    auto src = reinterpret_cast<const __m128i *>(p);
    const auto srcend = reinterpret_cast<const __m128i *>(p + len);

    if (len < sizeof(__m128i))
        return aeshash128_lt16(state.state0, p, len);

    if (len <= sizeof(__m256i))
        return aeshash128_16to32(state.state0, state.state1(), src, srcend);

    return aeshash256_ge32(state.state0_256(), p, len);
}

static size_t QT_FUNCTION_TARGET(VAES_AVX512)
aeshash256_avx256(const uchar *p, size_t len, size_t seed, size_t seed2) noexcept
{
    AESHashSeed state(seed, seed2);
    if (len <= sizeof(__m256i))
        return aeshash256_lt32_avx256(state.state0_256(), p, len);

    return aeshash256_ge32(state.state0_256(), p, len);
}
#  endif // VAES

static size_t QT_FUNCTION_TARGET(AES)
aeshash128(const uchar *p, size_t len, size_t seed, size_t seed2) noexcept
{
    AESHashSeed state(seed, seed2);
    auto src = reinterpret_cast<const __m128i *>(p);
    const auto srcend = reinterpret_cast<const __m128i *>(p + len);

    if (len < sizeof(__m128i))
        return aeshash128_lt16(state.state0, p, len);

    if (len <= sizeof(__m256i))
        return aeshash128_16to32(state.state0, state.state1(), src, srcend);

    return aeshash128_ge32(state.state0, state.state1(), src, srcend);
}

static size_t aeshash(const uchar *p, size_t len, size_t seed, size_t seed2) noexcept
{
#  if QT_COMPILER_SUPPORTS_HERE(VAES)
    if (qCpuHasFeature(VAES)) {
        if (qCpuHasFeature(AVX512VL))
            return aeshash256_avx256(p, len, seed, seed2);
        return aeshash256(p, len, seed, seed2);
    }
#  endif
    return aeshash128(p, len, seed, seed2);
}
#endif // x86 AESNI

#if defined(Q_PROCESSOR_ARM) && QT_COMPILER_SUPPORTS_HERE(AES) && !defined(QHASH_AES_SANITIZER_BUILD) && !defined(QT_BOOTSTRAPPED)
QT_FUNCTION_TARGET(AES)
static size_t aeshash(const uchar *p, size_t len, size_t seed, size_t seed2) noexcept
{
    uint8x16_t key;
#  if QT_POINTER_SIZE == 8
    uint64x2_t vseed = vcombine_u64(vcreate_u64(seed), vcreate_u64(seed2));
    key = vreinterpretq_u8_u64(vseed);
#  else

    uint32x2_t vseed = vmov_n_u32(seed);
    vseed = vset_lane_u32(seed2, vseed, 1);
    key = vreinterpretq_u8_u32(vcombine_u32(vseed, vseed));
#  endif

    // Compared to x86 AES, ARM splits each round into two instructions
    // and includes the pre-xor instead of the post-xor.
    const auto hash16bytes = [](uint8x16_t &state0, uint8x16_t data) {
        auto state1 = state0;
        state0 = vaeseq_u8(state0, data);
        state0 = vaesmcq_u8(state0);
        auto state2 = state0;
        state0 = vaeseq_u8(state0, state1);
        state0 = vaesmcq_u8(state0);
        auto state3 = state0;
        state0 = vaeseq_u8(state0, state2);
        state0 = vaesmcq_u8(state0);
        state0 = veorq_u8(state0, state3);
    };

    uint8x16_t state0 = key;

    if (len < 8)
        goto lt8;
    if (len < 16)
        goto lt16;
    if (len < 32)
        goto lt32;

    // rounds of 32 bytes
    {
        // Make state1 = ~state0:
        uint8x16_t state1 = veorq_u8(state0, vdupq_n_u8(255));

        // do simplified rounds of 32 bytes: unlike the Go code, we only
        // scramble twice and we keep 256 bits of state
        const auto *e = p + len - 31;
        while (p < e) {
            uint8x16_t data0 = vld1q_u8(p);
            uint8x16_t data1 = vld1q_u8(p + 16);
            auto oldstate0 = state0;
            auto oldstate1 = state1;
            state0 = vaeseq_u8(state0, data0);
            state1 = vaeseq_u8(state1, data1);
            state0 = vaesmcq_u8(state0);
            state1 = vaesmcq_u8(state1);
            auto laststate0 = state0;
            auto laststate1 = state1;
            state0 = vaeseq_u8(state0, oldstate0);
            state1 = vaeseq_u8(state1, oldstate1);
            state0 = vaesmcq_u8(state0);
            state1 = vaesmcq_u8(state1);
            state0 = veorq_u8(state0, laststate0);
            state1 = veorq_u8(state1, laststate1);
            p += 32;
        }
        state0 = veorq_u8(state0, state1);
    }
    len &= 0x1f;

    // do we still have 16 or more bytes?
    if (len & 0x10) {
lt32:
        uint8x16_t data = vld1q_u8(p);
        hash16bytes(state0, data);
        p += 16;
    }
    len &= 0xf;

    if (len & 0x08) {
lt16:
        uint8x8_t data8 = vld1_u8(p);
        uint8x16_t data = vcombine_u8(data8, vdup_n_u8(0));
        hash16bytes(state0, data);
        p += 8;
    }
    len &= 0x7;

lt8:
    if (len) {
        // load the last chunk of data
        // We're going to load 8 bytes and mask zero the part we don't care
        // (the hash of a short string is different from the hash of a longer
        // including NULLs at the end because the length is in the key)
        // WARNING: this may produce valgrind warnings, but it's safe

        uint8x8_t data8;

        if (Q_LIKELY(quintptr(p + 8) & 0xff8)) {
            // same page, we definitely can't fault:
            // load all 8 bytes and mask off the bytes past the end of the source
            static const qint8 maskarray[] = {
                -1, -1, -1, -1, -1, -1, -1,
                 0,  0,  0,  0,  0,  0,  0,
            };
            uint8x8_t mask = vld1_u8(reinterpret_cast<const quint8 *>(maskarray) + 7 - len);
            data8 = vld1_u8(p);
            data8 = vand_u8(data8, mask);
        } else {
            // too close to the end of the page, it could fault:
            // load 8 bytes ending at the data end, then shuffle them to the beginning
            static const qint8 shufflecontrol[] = {
                 1,  2,  3,  4,  5,  6,  7,
                -1, -1, -1, -1, -1, -1, -1,
            };
            uint8x8_t control = vld1_u8(reinterpret_cast<const quint8 *>(shufflecontrol) + 7 - len);
            data8 = vld1_u8(p - 8 + len);
            data8 = vtbl1_u8(data8, control);
        }
        uint8x16_t data = vcombine_u8(data8, vdup_n_u8(0));
        hash16bytes(state0, data);
    }

    // extract state0
#  if QT_POINTER_SIZE == 8
    return vgetq_lane_u64(vreinterpretq_u64_u8(state0), 0);
#  else
    return vgetq_lane_u32(vreinterpretq_u32_u8(state0), 0);
#  endif
}
#endif

size_t qHashBits(const void *p, size_t size, size_t seed) noexcept
{
#ifdef QT_BOOTSTRAPPED
    // the seed is always 0 in bootstrapped mode (no seed generation code),
    // so help the compiler do dead code elimination
    seed = 0;
#endif
    // mix in the length as a secondary seed. For seed == 0, seed2 must be
    // size, to match what we used to do prior to Qt 6.2.
    size_t seed2 = size;
    if (seed)
        seed2 = qt_qhash_seed.currentSeed(1);
#ifdef AESHASH
    if (seed && qCpuHasFeature(AES) && qCpuHasFeature(SSE4_2))
        return aeshash(reinterpret_cast<const uchar *>(p), size, seed, seed2);
#elif defined(Q_PROCESSOR_ARM) && QT_COMPILER_SUPPORTS_HERE(AES) && !defined(QHASH_AES_SANITIZER_BUILD) && !defined(QT_BOOTSTRAPPED)
# if defined(Q_OS_LINUX)
    // Do specific runtime-only check as Yocto hard enables Crypto extension for
    // all armv8 configs
    if (seed && (qCpuFeatures() & CpuFeatureAES))
# else
    if (seed && qCpuHasFeature(AES))
# endif
        return aeshash(reinterpret_cast<const uchar *>(p), size, seed, seed2);
#endif

    if (size <= QT_POINTER_SIZE)
        return murmurhash(p, size, seed);

    return siphash(reinterpret_cast<const uchar *>(p), size, seed, seed2);
}

size_t qHash(QByteArrayView key, size_t seed) noexcept
{
    return qHashBits(key.constData(), size_t(key.size()), seed);
}

size_t qHash(QStringView key, size_t seed) noexcept
{
    return qHashBits(key.data(), key.size()*sizeof(QChar), seed);
}

size_t qHash(const QBitArray &bitArray, size_t seed) noexcept
{
    qsizetype m = bitArray.d.size() - 1;
    size_t result = qHashBits(reinterpret_cast<const uchar *>(bitArray.d.constData()), size_t(qMax(0, m)), seed);

    // deal with the last 0 to 7 bits manually, because we can't trust that
    // the padding is initialized to 0 in bitArray.d
    qsizetype n = bitArray.size();
    if (n & 0x7)
        result = ((result << 4) + bitArray.d.at(m)) & ((1 << n) - 1);
    return result;
}

size_t qHash(QLatin1StringView key, size_t seed) noexcept
{
    return qHashBits(reinterpret_cast<const uchar *>(key.data()), size_t(key.size()), seed);
}

/*!
    \class QHashSeed
    \inmodule QtCore
    \since 6.2

    The QHashSeed class is used to convey the QHash seed. This is used
    internally by QHash and provides three static member functions to allow
    users to obtain the hash and to reset it.

    QHash and the qHash() functions implement what is called as "salted hash".
    The intent is that different applications and different instances of the
    same application will produce different hashing values for the same input,
    thus causing the ordering of elements in QHash to be unpredictable by
    external observers. This improves the applications' resilience against
    attacks that attempt to force hashing tables into degenerate mode.

    Most applications will not need to deal directly with the hash seed, as
    QHash will do so when needed. However, applications may wish to use this
    for their own purposes in the same way as QHash does: as an
    application-global random value (but see \l QRandomGenerator too). Note
    that the global hash seed may change during the application's lifetime, if
    the resetRandomGlobalSeed() function is called. Users of the global hash
    need to store the value they are using and not rely on getting it again.

    This class also implements functionality to set the hash seed to a
    deterministic value, which the qHash() functions will take to mean that
    they should use a fixed hashing function on their data too. This
    functionality is only meant to be used in debugging applications. This
    behavior can also be controlled by setting the \c QT_HASH_SEED environment
    variable to the value zero (any other value is ignored).

    \sa QHash, QRandomGenerator
*/

/*!
    \fn QHashSeed::QHashSeed(size_t data)

    Constructs a new QHashSeed object using \a data as the seed.
 */

/*!
    \fn QHashSeed::operator size_t() const

    Converts the returned hash seed into a \c size_t.
 */

/*!
    \threadsafe

    Returns the current global QHash seed. The value returned by this function
    will be zero if setDeterministicGlobalSeed() has been called or if the
    \c{QT_HASH_SEED} environment variable is set to zero.
 */
QHashSeed QHashSeed::globalSeed() noexcept
{
    return qt_qhash_seed.currentSeed(0);
}

/*!
    \threadsafe

    Forces the Qt hash seed to a deterministic value (zero) and asks the
    qHash() functions to use a pre-determined hashing function. This mode is
    only useful for debugging and should not be used in production code.

    Regular operation can be restored by calling resetRandomGlobalSeed().
 */
void QHashSeed::setDeterministicGlobalSeed()
{
    qt_qhash_seed.clearSeed();
}

/*!
    \threadsafe

    Reseeds the Qt hashing seed to a new, random value. Calling this function
    is not necessary, but long-running applications may want to do so after a
    long period of time in which information about its hash may have been
    exposed to potential attackers.

    If the environment variable \c QT_HASH_SEED is set to zero, calling this
    function will result in a no-op.

    Qt never calls this function during the execution of the application, but
    unless the \c QT_HASH_SEED variable is set to 0, the hash seed returned by
    globalSeed() will be a random value as if this function had been called.
 */
void QHashSeed::resetRandomGlobalSeed()
{
    qt_qhash_seed.resetSeed();
}

#if QT_DEPRECATED_SINCE(6,6)
/*! \relates QHash
    \since 5.6
    \deprecated [6.6] Use QHashSeed::globalSeed() instead.

    Returns the current global QHash seed.

    The seed is set in any newly created QHash. See \l{qHash} about how this seed
    is being used by QHash.

    \sa QHashSeed, QHashSeed::globalSeed()
 */
int qGlobalQHashSeed()
{
    return int(QHashSeed::globalSeed() & INT_MAX);
}

/*! \relates QHash
    \since 5.6
    \deprecated [6.6] Use QHashSeed instead.

    Sets the global QHash seed to \a newSeed.

    Manually setting the global QHash seed value should be done only for testing
    and debugging purposes, when deterministic and reproducible behavior on a QHash
    is needed. We discourage to do it in production code as it can make your
    application susceptible to \l{algorithmic complexity attacks}.

    From Qt 5.10 and onwards, the only allowed values are 0 and -1. Passing the
    value -1 will reinitialize the global QHash seed to a random value, while
    the value of 0 is used to request a stable algorithm for C++ primitive
    types types (like \c int) and string types (QString, QByteArray).

    The seed is set in any newly created QHash. See \l{qHash} about how this seed
    is being used by QHash.

    If the environment variable \c QT_HASH_SEED is set, calling this function will
    result in a no-op.

    \sa QHashSeed::globalSeed(), QHashSeed
 */
void qSetGlobalQHashSeed(int newSeed)
{
    if (Q_LIKELY(newSeed == 0 || newSeed == -1)) {
        if (newSeed == 0)
            QHashSeed::setDeterministicGlobalSeed();
        else
            QHashSeed::resetRandomGlobalSeed();
    } else {
        // can't use qWarning here (reentrancy)
        fprintf(stderr, "qSetGlobalQHashSeed: forced seed value is not 0; ignoring call\n");
    }
}
#endif  // QT_DEPRECATED_SINCE(6,6)

/*!
    \internal

    Private copy of the implementation of the Qt 4 qHash algorithm for strings,
    (that is, QChar-based arrays, so all QString-like classes),
    to be used wherever the result is somehow stored or reused across multiple
    Qt versions. The public qHash implementation can change at any time,
    therefore one must not rely on the fact that it will always give the same
    results.

    The qt_hash functions must *never* change their results.

    This function can hash discontiguous memory by invoking it on each chunk,
    passing the previous's result in the next call's \a chained argument.
*/
uint qt_hash(QStringView key, uint chained) noexcept
{
    auto n = key.size();
    auto p = key.utf16();

    uint h = chained;

    while (n--) {
        h = (h << 4) + *p++;
        h ^= (h & 0xf0000000) >> 23;
        h &= 0x0fffffff;
    }
    return h;
}

/*!
    \fn template <typename T1, typename T2> size_t qHash(const std::pair<T1, T2> &key, size_t seed = 0)
    \since 5.7
    \relates QHash

    Returns the hash value for the \a key, using \a seed to seed the calculation.

    Types \c T1 and \c T2 must be supported by qHash().
*/

/*!
    \fn template <typename... T> size_t qHashMulti(size_t seed, const T &...args)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a{args}, using \a seed to seed
    the calculation, by successively applying qHash() to each
    element and combining the hash values into a single one.

    Note that the order of the arguments is significant. If order does
    not matter, use qHashMultiCommutative() instead. If you are hashing raw
    memory, use qHashBits(); if you are hashing a range, use qHashRange().

    This function is provided as a convenience to implement qHash() for
    your own custom types. For example, here's how you could implement
    a qHash() overload for a class \c{Employee}:

    \snippet code/src_corelib_tools_qhash.cpp 13

    \sa qHashMultiCommutative, qHashRange
*/

/*!
    \fn template <typename... T> size_t qHashMultiCommutative(size_t seed, const T &...args)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a{args}, using \a seed to seed
    the calculation, by successively applying qHash() to each
    element and combining the hash values into a single one.

    The order of the arguments is insignificant. If order does
    matter, use qHashMulti() instead, as it may produce better quality
    hashing. If you are hashing raw memory, use qHashBits(); if you are
    hashing a range, use qHashRange().

    This function is provided as a convenience to implement qHash() for
    your own custom types.

    \sa qHashMulti, qHashRange
*/

/*! \fn template <typename InputIterator> size_t qHashRange(InputIterator first, InputIterator last, size_t seed = 0)
    \relates QHash
    \since 5.5

    Returns the hash value for the range [\a{first},\a{last}), using \a seed
    to seed the calculation, by successively applying qHash() to each
    element and combining the hash values into a single one.

    The return value of this function depends on the order of elements
    in the range. That means that

    \snippet code/src_corelib_tools_qhash.cpp 30

    and
    \snippet code/src_corelib_tools_qhash.cpp 31

    hash to \b{different} values. If order does not matter, for example for hash
    tables, use qHashRangeCommutative() instead. If you are hashing raw
    memory, use qHashBits().

    Use this function only to implement qHash() for your own custom
    types. For example, here's how you could implement a qHash() overload for
    std::vector<int>:

    \snippet code/src_corelib_tools_qhash.cpp qhashrange

    It bears repeating that the implementation of qHashRange() - like
    the qHash() overloads offered by Qt - may change at any time. You
    \b{must not} rely on the fact that qHashRange() will give the same
    results (for the same inputs) across different Qt versions, even
    if qHash() for the element type would.

    \sa qHashBits(), qHashRangeCommutative()
*/

/*! \fn template <typename InputIterator> size_t qHashRangeCommutative(InputIterator first, InputIterator last, size_t seed = 0)
    \relates QHash
    \since 5.5

    Returns the hash value for the range [\a{first},\a{last}), using \a seed
    to seed the calculation, by successively applying qHash() to each
    element and combining the hash values into a single one.

    The return value of this function does not depend on the order of
    elements in the range. That means that

    \snippet code/src_corelib_tools_qhash.cpp 30

    and
    \snippet code/src_corelib_tools_qhash.cpp 31

    hash to the \b{same} values. If order matters, for example, for vectors
    and arrays, use qHashRange() instead. If you are hashing raw
    memory, use qHashBits().

    Use this function only to implement qHash() for your own custom
    types. For example, here's how you could implement a qHash() overload for
    std::unordered_set<int>:

    \snippet code/src_corelib_tools_qhash.cpp qhashrangecommutative

    It bears repeating that the implementation of
    qHashRangeCommutative() - like the qHash() overloads offered by Qt
    - may change at any time. You \b{must not} rely on the fact that
    qHashRangeCommutative() will give the same results (for the same
    inputs) across different Qt versions, even if qHash() for the
    element type would.

    \sa qHashBits(), qHashRange()
*/

/*! \fn size_t qHashBits(const void *p, size_t len, size_t seed = 0)
    \relates QHash
    \since 5.4

    Returns the hash value for the memory block of size \a len pointed
    to by \a p, using \a seed to seed the calculation.

    Use this function only to implement qHash() for your own custom
    types. For example, here's how you could implement a qHash() overload for
    std::vector<int>:

    \snippet code/src_corelib_tools_qhash.cpp qhashbits

    This takes advantage of the fact that std::vector lays out its data
    contiguously. If that is not the case, or the contained type has
    padding, you should use qHashRange() instead.

    It bears repeating that the implementation of qHashBits() - like
    the qHash() overloads offered by Qt - may change at any time. You
    \b{must not} rely on the fact that qHashBits() will give the same
    results (for the same inputs) across different Qt versions.

    \sa qHashRange(), qHashRangeCommutative()
*/

/*! \fn size_t qHash(char key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(uchar key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(signed char key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(ushort key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(short key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(uint key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(int key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(ulong key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(long key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(quint64 key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(qint64 key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(char8_t key, size_t seed = 0)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(char16_t key, size_t seed = 0)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(char32_t key, size_t seed = 0)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(wchar_t key, size_t seed = 0)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(float key, size_t seed = 0) noexcept
    \relates QHash
    \since 5.3

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \relates QHash
    \since 5.3

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
size_t qHash(double key, size_t seed) noexcept
{
    // ensure -0 gets mapped to 0
    key += 0.0;
    if constexpr (sizeof(double) == sizeof(size_t)) {
        size_t k;
        memcpy(&k, &key, sizeof(double));
        return QHashPrivate::hash(k, seed);
    } else {
        return murmurhash(&key, sizeof(key), seed);
    }
}

#if !defined(Q_OS_DARWIN) || defined(Q_QDOC)
/*! \relates QHash
    \since 5.3

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
size_t qHash(long double key, size_t seed) noexcept
{
    // ensure -0 gets mapped to 0
    key += static_cast<long double>(0.0);
    if constexpr (sizeof(long double) == sizeof(size_t)) {
        size_t k;
        memcpy(&k, &key, sizeof(long double));
        return QHashPrivate::hash(k, seed);
    } else {
        return murmurhash(&key, sizeof(key), seed);
    }
}
#endif

/*! \fn size_t qHash(const QChar key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(const QByteArray &key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(const QByteArrayView &key, size_t seed = 0)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(const QBitArray &key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(const QString &key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(QStringView key, size_t seed = 0)
    \relates QStringView
    \since 5.10

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn size_t qHash(QLatin1StringView key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn template <class T> size_t qHash(const T *key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn template <class T> size_t qHash(std::nullptr_t key, size_t seed = 0)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn template<typename T> bool qHashEquals(const T &a, const T &b)
    \relates QHash
    \since 6.0
    \internal

    This method is being used by QHash to compare two keys. Returns true if the
    keys \a a and \a b are considered equal for hashing purposes.

    The default implementation returns the result of (a == b). It can be reimplemented
    for a certain type if the equality operator is not suitable for hashing purposes.
    This is for example the case if the equality operator uses qFuzzyCompare to compare
    floating point values.
*/


/*!
    \class QHash
    \inmodule QtCore
    \brief The QHash class is a template class that provides a hash-table-based dictionary.

    \ingroup tools
    \ingroup shared

    \reentrant

    QHash\<Key, T\> is one of Qt's generic \l{container classes}. It
    stores (key, value) pairs and provides very fast lookup of the
    value associated with a key.

    QHash provides very similar functionality to QMap. The
    differences are:

    \list
    \li QHash provides faster lookups than QMap. (See \l{Algorithmic
       Complexity} for details.)
    \li When iterating over a QMap, the items are always sorted by
       key. With QHash, the items are arbitrarily ordered.
    \li The key type of a QMap must provide operator<(). The key
       type of a QHash must provide operator==() and a global
       hash function called qHash() (see \l{qHash}).
    \endlist

    Here's an example QHash with QString keys and \c int values:
    \snippet code/src_corelib_tools_qhash.cpp 0

    To insert a (key, value) pair into the hash, you can use operator[]():

    \snippet code/src_corelib_tools_qhash.cpp 1

    This inserts the following three (key, value) pairs into the
    QHash: ("one", 1), ("three", 3), and ("seven", 7). Another way to
    insert items into the hash is to use insert():

    \snippet code/src_corelib_tools_qhash.cpp 2

    To look up a value, use operator[]() or value():

    \snippet code/src_corelib_tools_qhash.cpp 3

    If there is no item with the specified key in the hash, these
    functions return a \l{default-constructed value}.

    If you want to check whether the hash contains a particular key,
    use contains():

    \snippet code/src_corelib_tools_qhash.cpp 4

    There is also a value() overload that uses its second argument as
    a default value if there is no item with the specified key:

    \snippet code/src_corelib_tools_qhash.cpp 5

    In general, we recommend that you use contains() and value()
    rather than operator[]() for looking up a key in a hash. The
    reason is that operator[]() silently inserts an item into the
    hash if no item exists with the same key (unless the hash is
    const). For example, the following code snippet will create 1000
    items in memory:

    \snippet code/src_corelib_tools_qhash.cpp 6

    To avoid this problem, replace \c hash[i] with \c hash.value(i)
    in the code above.

    Internally, QHash uses a hash table to perform lookups. This
    hash table automatically grows to
    provide fast lookups without wasting too much memory. You can
    still control the size of the hash table by calling reserve() if
    you already know approximately how many items the QHash will
    contain, but this isn't necessary to obtain good performance. You
    can also call capacity() to retrieve the hash table's size.

    QHash will not shrink automatically if items are removed from the
    table. To minimize the memory used by the hash, call squeeze().

    If you want to navigate through all the (key, value) pairs stored
    in a QHash, you can use an iterator. QHash provides both
    \l{Java-style iterators} (QHashIterator and QMutableHashIterator)
    and \l{STL-style iterators} (QHash::const_iterator and
    QHash::iterator). Here's how to iterate over a QHash<QString,
    int> using a Java-style iterator:

    \snippet code/src_corelib_tools_qhash.cpp 7

    Here's the same code, but using an STL-style iterator:

    \snippet code/src_corelib_tools_qhash.cpp 8

    QHash is unordered, so an iterator's sequence cannot be assumed
    to be predictable. If ordering by key is required, use a QMap.

    A QHash allows only one value per key. If you call
    insert() with a key that already exists in the QHash, the
    previous value is erased. For example:

    \snippet code/src_corelib_tools_qhash.cpp 9

    If you need to store multiple entries for the same key in the
    hash table, use \l{QMultiHash}.

    If you only need to extract the values from a hash (not the keys),
    you can also use range-based for:

    \snippet code/src_corelib_tools_qhash.cpp 12

    Items can be removed from the hash in several ways. One way is to
    call remove(); this will remove any item with the given key.
    Another way is to use QMutableHashIterator::remove(). In addition,
    you can clear the entire hash using clear().

    QHash's key and value data types must be \l{assignable data
    types}. You cannot, for example, store a QWidget as a value;
    instead, store a QWidget *.

    \target qHash
    \section2 The hashing function

    A QHash's key type has additional requirements other than being an
    assignable data type: it must provide operator==(), and there must also be
    a hashing function that returns a hash value for an argument of the
    key's type.

    The hashing function computes a numeric value based on a key. It
    can use any algorithm imaginable, as long as it always returns
    the same value if given the same argument. In other words, if
    \c{e1 == e2}, then \c{hash(e1) == hash(e2)} must hold as well.
    However, to obtain good performance, the hashing function should
    attempt to return different hash values for different keys to the
    largest extent possible.

    A hashing function for a key type \c{K} may be provided in two
    different ways.

    The first way is by having an overload of \c{qHash()} in \c{K}'s
    namespace. The \c{qHash()} function must have one of these signatures:

    \snippet code/src_corelib_tools_qhash.cpp 32

    The two-arguments overloads take an unsigned integer that should be used to
    seed the calculation of the hash function. This seed is provided by QHash
    in order to prevent a family of \l{algorithmic complexity attacks}.

    \note In Qt 6 it is possible to define a \c{qHash()} overload
    taking only one argument; support for this is deprecated. Starting
    with Qt 7, it will be mandatory to use a two-arguments overload. If
    both a one-argument and a two-arguments overload are defined for a
    key type, the latter is used by QHash (note that you can simply
    define a two-arguments version, and use a default value for the
    seed parameter).

    The second way to provide a hashing function is by specializing
    the \c{std::hash} class for the key type \c{K}, and providing a
    suitable function call operator for it:

    \snippet code/src_corelib_tools_qhash.cpp 33

    The seed argument has the same meaning as for \c{qHash()},
    and may be left out.

    This second way allows to reuse the same hash function between
    QHash and the C++ Standard Library unordered associative containers.
    If both a \c{qHash()} overload and a \c{std::hash} specializations
    are provided for a type, then the \c{qHash()} overload is preferred.

    Here's a partial list of the C++ and Qt types that can serve as keys in a
    QHash: any integer type (char, unsigned long, etc.), any pointer type,
    QChar, QString, and QByteArray. For all of these, the \c <QHash> header
    defines a qHash() function that computes an adequate hash value. Many other
    Qt classes also declare a qHash overload for their type; please refer to
    the documentation of each class.

    If you want to use other types as the key, make sure that you provide
    operator==() and a hash implementation.

    The convenience qHashMulti() function can be used to implement
    qHash() for a custom type, where one usually wants to produce a
    hash value from multiple fields:

    Example:
    \snippet code/src_corelib_tools_qhash.cpp 13

    In the example above, we've relied on Qt's own implementation of
    qHash() for QString and QDate to give us a hash value for the
    employee's name and date of birth respectively.

    Note that the implementation of the qHash() overloads offered by Qt
    may change at any time. You \b{must not} rely on the fact that qHash()
    will give the same results (for the same inputs) across different Qt
    versions.

    \section2 Algorithmic complexity attacks

    All hash tables are vulnerable to a particular class of denial of service
    attacks, in which the attacker carefully pre-computes a set of different
    keys that are going to be hashed in the same bucket of a hash table (or
    even have the very same hash value). The attack aims at getting the
    worst-case algorithmic behavior (O(n) instead of amortized O(1), see
    \l{Algorithmic Complexity} for the details) when the data is fed into the
    table.

    In order to avoid this worst-case behavior, the calculation of the hash
    value done by qHash() can be salted by a random seed, that nullifies the
    attack's extent. This seed is automatically generated by QHash once per
    process, and then passed by QHash as the second argument of the
    two-arguments overload of the qHash() function.

    This randomization of QHash is enabled by default. Even though programs
    should never depend on a particular QHash ordering, there may be situations
    where you temporarily need deterministic behavior, for example for debugging or
    regression testing. To disable the randomization, define the environment
    variable \c QT_HASH_SEED to have the value 0. Alternatively, you can call
    the QHashSeed::setDeterministicGlobalSeed() function.

    \sa QHashIterator, QMutableHashIterator, QMap, QSet
*/

/*! \fn template <class Key, class T> QHash<Key, T>::QHash()

    Constructs an empty hash.

    \sa clear()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::QHash(QHash &&other)

    Move-constructs a QHash instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*! \fn template <class Key, class T> QHash<Key, T>::QHash(std::initializer_list<std::pair<Key,T> > list)
    \since 5.1

    Constructs a hash with a copy of each of the elements in the
    initializer list \a list.
*/

/*! \fn template <class Key, class T> template <class InputIterator> QHash<Key, T>::QHash(InputIterator begin, InputIterator end)
    \since 5.14

    Constructs a hash with a copy of each of the elements in the iterator range
    [\a begin, \a end). Either the elements iterated by the range must be
    objects with \c{first} and \c{second} data members (like \c{QPair},
    \c{std::pair}, etc.) convertible to \c Key and to \c T respectively; or the
    iterators must have \c{key()} and \c{value()} member functions, returning a
    key convertible to \c Key and a value convertible to \c T respectively.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::QHash(const QHash &other)

    Constructs a copy of \a other.

    This operation occurs in \l{constant time}, because QHash is
    \l{implicitly shared}. This makes returning a QHash from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and this takes \l{linear time}.

    \sa operator=()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::~QHash()

    Destroys the hash. References to the values in the hash and all
    iterators of this hash become invalid.
*/

/*! \fn template <class Key, class T> QHash &QHash<Key, T>::operator=(const QHash &other)

    Assigns \a other to this hash and returns a reference to this hash.
*/

/*!
    \fn template <class Key, class T> QHash &QHash<Key, T>::operator=(QHash &&other)

    Move-assigns \a other to this QHash instance.

    \since 5.2
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::swap(QHash &other)
    \since 4.8

    Swaps hash \a other with this hash. This operation is very
    fast and never fails.
*/

/*! \fn template <class Key, class T> void QMultiHash<Key, T>::swap(QMultiHash &other)
    \since 4.8

    Swaps hash \a other with this hash. This operation is very
    fast and never fails.
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::operator==(const QHash &other) const

    Returns \c true if \a other is equal to this hash; otherwise returns
    false.

    Two hashes are considered equal if they contain the same (key,
    value) pairs.

    This function requires the value type to implement \c operator==().

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::operator!=(const QHash &other) const

    Returns \c true if \a other is not equal to this hash; otherwise
    returns \c false.

    Two hashes are considered equal if they contain the same (key,
    value) pairs.

    This function requires the value type to implement \c operator==().

    \sa operator==()
*/

/*! \fn template <class Key, class T> qsizetype QHash<Key, T>::size() const

    Returns the number of items in the hash.

    \sa isEmpty(), count()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::isEmpty() const

    Returns \c true if the hash contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn template <class Key, class T> qsizetype QHash<Key, T>::capacity() const

    Returns the number of buckets in the QHash's internal hash table.

    The sole purpose of this function is to provide a means of fine
    tuning QHash's memory usage. In general, you will rarely ever
    need to call this function. If you want to know how many items are
    in the hash, call size().

    \sa reserve(), squeeze()
*/

/*! \fn template <class Key, class T> float QHash<Key, T>::load_factor() const noexcept

    Returns the current load factor of the QHash's internal hash table.
    This is the same as capacity()/size(). The implementation used
    will aim to keep the load factor between 0.25 and 0.5. This avoids
    having too many hash table collisions that would degrade performance.

    Even with a low load factor, the implementation of the hash table has a
    very low memory overhead.

    This method purely exists for diagnostic purposes and you should rarely
    need to call it yourself.

    \sa reserve(), squeeze()
*/


/*! \fn template <class Key, class T> void QHash<Key, T>::reserve(qsizetype size)

    Ensures that the QHash's internal hash table has space to store at
    least \a size items without having to grow the hash table.

    This implies that the hash table will contain at least 2 * \a size buckets
    to ensure good performance

    This function is useful for code that needs to build a huge hash
    and wants to avoid repeated reallocation. For example:

    \snippet code/src_corelib_tools_qhash.cpp 14

    Ideally, \a size should be the maximum number of items expected
    in the hash. QHash will then choose the smallest possible
    number of buckets that will allow storing \a size items in the table
    without having to grow the internal hash table. If \a size
    is an underestimate, the worst that will happen is that the QHash
    will be a bit slower.

    In general, you will rarely ever need to call this function.
    QHash's internal hash table automatically grows to
    provide good performance without wasting too much memory.

    \sa squeeze(), capacity()
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::squeeze()

    Reduces the size of the QHash's internal hash table to save
    memory.

    The sole purpose of this function is to provide a means of fine
    tuning QHash's memory usage. In general, you will rarely ever
    need to call this function.

    \sa reserve(), capacity()
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::detach()

    \internal

    Detaches this hash from any other hashes with which it may share
    data.

    \sa isDetached()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::isDetached() const

    \internal

    Returns \c true if the hash's internal data isn't shared with any
    other hash object; otherwise returns \c false.

    \sa detach()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::isSharedWith(const QHash &other) const

    \internal

    Returns true if the internal hash table of this QHash is shared with \a other, otherwise false.
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::clear()

    Removes all items from the hash and frees up all memory used by it.

    \sa remove()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::remove(const Key &key)

    Removes the item that has the \a key from the hash.
    Returns true if the key exists in the hash and the item has been removed,
    and false otherwise.

    \sa clear(), take()
*/

/*! \fn template <class Key, class T> template <typename Predicate> qsizetype QHash<Key, T>::removeIf(Predicate pred)
    \since 6.1

    Removes all elements for which the predicate \a pred returns true
    from the hash.

    The function supports predicates which take either an argument of
    type \c{QHash<Key, T>::iterator}, or an argument of type
    \c{std::pair<const Key &, T &>}.

    Returns the number of elements removed, if any.

    \sa clear(), take()
*/

/*! \fn template <class Key, class T> T QHash<Key, T>::take(const Key &key)

    Removes the item with the \a key from the hash and returns
    the value associated with it.

    If the item does not exist in the hash, the function simply
    returns a \l{default-constructed value}.

    If you don't use the return value, remove() is more efficient.

    \sa remove()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::contains(const Key &key) const

    Returns \c true if the hash contains an item with the \a key;
    otherwise returns \c false.

    \sa count(), QMultiHash::contains()
*/

/*! \fn template <class Key, class T> T QHash<Key, T>::value(const Key &key) const
    \fn template <class Key, class T> T QHash<Key, T>::value(const Key &key, const T &defaultValue) const
    \overload

    Returns the value associated with the \a key.

    If the hash contains no item with the \a key, the function
    returns \a defaultValue, or a \l{default-constructed value} if this
    parameter has not been supplied.
*/

/*! \fn template <class Key, class T> T &QHash<Key, T>::operator[](const Key &key)

    Returns the value associated with the \a key as a modifiable
    reference.

    If the hash contains no item with the \a key, the function inserts
    a \l{default-constructed value} into the hash with the \a key, and
    returns a reference to it.

    \sa insert(), value()
*/

/*! \fn template <class Key, class T> const T QHash<Key, T>::operator[](const Key &key) const

    \overload

    Same as value().
*/

/*! \fn template <class Key, class T> QList<Key> QHash<Key, T>::keys() const

    Returns a list containing all the keys in the hash, in an
    arbitrary order.

    The order is guaranteed to be the same as that used by values().

    This function creates a new list, in \l {linear time}. The time and memory
    use that entails can be avoided by iterating from \l keyBegin() to
    \l keyEnd().

    \sa values(), key()
*/

/*! \fn template <class Key, class T> QList<Key> QHash<Key, T>::keys(const T &value) const

    \overload

    Returns a list containing all the keys associated with value \a
    value, in an arbitrary order.

    This function can be slow (\l{linear time}), because QHash's
    internal data structure is optimized for fast lookup by key, not
    by value.
*/

/*! \fn template <class Key, class T> QList<T> QHash<Key, T>::values() const

    Returns a list containing all the values in the hash, in an
    arbitrary order.

    The order is guaranteed to be the same as that used by keys().

    This function creates a new list, in \l {linear time}. The time and memory
    use that entails can be avoided by iterating from \l keyValueBegin() to
    \l keyValueEnd().

    \sa keys(), value()
*/

/*!
    \fn template <class Key, class T> Key QHash<Key, T>::key(const T &value) const
    \fn template <class Key, class T> Key QHash<Key, T>::key(const T &value, const Key &defaultKey) const
    \since 4.3

    Returns the first key mapped to \a value. If the hash contains no item
    mapped to \a value, returns \a defaultKey, or a \l{default-constructed
    value}{default-constructed key} if this parameter has not been supplied.

    This function can be slow (\l{linear time}), because QHash's
    internal data structure is optimized for fast lookup by key, not
    by value.
*/

/*! \fn template <class Key, class T> qsizetype QHash<Key, T>::count(const Key &key) const

    Returns the number of items associated with the \a key.

    \sa contains()
*/

/*! \fn template <class Key, class T> qsizetype QHash<Key, T>::count() const

    \overload

    Same as size().
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the hash.

    \sa constBegin(), end()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::begin() const

    \overload
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the hash.

    \sa begin(), cend()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the hash.

    \sa begin(), constEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator QHash<Key, T>::keyBegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first key
    in the hash.

    \sa keyEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the hash.

    \sa begin(), constEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::end() const

    \overload
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the hash.

    \sa constBegin(), end()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the hash.

    \sa cbegin(), end()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator QHash<Key, T>::keyEnd() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last key in the hash.

    \sa keyBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_value_iterator QHash<Key, T>::keyValueBegin()
    \since 5.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_value_iterator QHash<Key, T>::keyValueEnd()
    \since 5.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::keyValueBegin() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::constKeyValueBegin() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::keyValueEnd() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::constKeyValueEnd() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa constKeyValueBegin()
*/

/*! \fn template <class Key, class T> auto QHash<Key, T>::asKeyValueRange() &
    \fn template <class Key, class T> auto QHash<Key, T>::asKeyValueRange() const &
    \fn template <class Key, class T> auto QHash<Key, T>::asKeyValueRange() &&
    \fn template <class Key, class T> auto QHash<Key, T>::asKeyValueRange() const &&
    \since 6.4

    Returns a range object that allows iteration over this hash as
    key/value pairs. For instance, this range object can be used in a
    range-based for loop, in combination with a structured binding declaration:

    \snippet code/src_corelib_tools_qhash.cpp 34

    Note that both the key and the value obtained this way are
    references to the ones in the hash. Specifically, mutating the value
    will modify the hash itself.

    \sa QKeyValueIterator
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::erase(const_iterator pos)
    \since 5.7

    Removes the (key, value) pair associated with the iterator \a pos
    from the hash, and returns an iterator to the next item in the
    hash.

    This function never causes QHash to
    rehash its internal data structure. This means that it can safely
    be called while iterating, and won't affect the order of items in
    the hash. For example:

    \snippet code/src_corelib_tools_qhash.cpp 15

    \sa remove(), take(), find()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::find(const Key &key)

    Returns an iterator pointing to the item with the \a key in the
    hash.

    If the hash contains no item with the \a key, the function
    returns end().

    If the hash contains multiple items with the \a key, this
    function returns an iterator that points to the most recently
    inserted value. The other values are accessible by incrementing
    the iterator. For example, here's some code that iterates over all
    the items with the same key:

    \snippet code/src_corelib_tools_qhash.cpp 16

    \sa value(), values()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::find(const Key &key) const

    \overload
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::constFind(const Key &key) const
    \since 4.1

    Returns an iterator pointing to the item with the \a key in the
    hash.

    If the hash contains no item with the \a key, the function
    returns constEnd().

    \sa find()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::insert(const Key &key, const T &value)

    Inserts a new item with the \a key and a value of \a value.

    If there is already an item with the \a key, that item's value
    is replaced with \a value.
*/

/*!
    \fn template <class Key, class T> template <typename ...Args> QHash<Key, T>::iterator QHash<Key, T>::emplace(const Key &key, Args&&... args)
    \fn template <class Key, class T> template <typename ...Args> QHash<Key, T>::iterator QHash<Key, T>::emplace(Key &&key, Args&&... args)

    Inserts a new element into the container. This new element
    is constructed in-place using \a args as the arguments for its
    construction.

    Returns an iterator pointing to the new element.
*/


/*! \fn template <class Key, class T> void QHash<Key, T>::insert(const QHash &other)
    \since 5.15

    Inserts all the items in the \a other hash into this hash.

    If a key is common to both hashes, its value will be replaced with the
    value stored in \a other.
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty(), returning true if the hash is empty; otherwise
    returns \c false.
*/

/*! \fn template <class Key, class T> QPair<iterator, iterator> QMultiHash<Key, T>::equal_range(const Key &key)
    \since 5.7

    Returns a pair of iterators delimiting the range of values \c{[first, second)}, that
    are stored under \a key. If the range is empty then both iterators will be equal to end().
*/

/*!
    \fn template <class Key, class T> QPair<const_iterator, const_iterator> QMultiHash<Key, T>::equal_range(const Key &key) const
    \overload
    \since 5.7
*/

/*! \typedef QHash::ConstIterator

    Qt-style synonym for QHash::const_iterator.
*/

/*! \typedef QHash::Iterator

    Qt-style synonym for QHash::iterator.
*/

/*! \typedef QHash::difference_type

    Typedef for ptrdiff_t. Provided for STL compatibility.
*/

/*! \typedef QHash::key_type

    Typedef for Key. Provided for STL compatibility.
*/

/*! \typedef QHash::mapped_type

    Typedef for T. Provided for STL compatibility.
*/

/*! \typedef QHash::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*! \typedef QHash::iterator::difference_type
    \internal
*/

/*! \typedef QHash::iterator::iterator_category
    \internal
*/

/*! \typedef QHash::iterator::pointer
    \internal
*/

/*! \typedef QHash::iterator::reference
    \internal
*/

/*! \typedef QHash::iterator::value_type
    \internal
*/

/*! \typedef QHash::const_iterator::difference_type
    \internal
*/

/*! \typedef QHash::const_iterator::iterator_category
    \internal
*/

/*! \typedef QHash::const_iterator::pointer
    \internal
*/

/*! \typedef QHash::const_iterator::reference
    \internal
*/

/*! \typedef QHash::const_iterator::value_type
    \internal
*/

/*! \typedef QHash::key_iterator::difference_type
    \internal
*/

/*! \typedef QHash::key_iterator::iterator_category
    \internal
*/

/*! \typedef QHash::key_iterator::pointer
    \internal
*/

/*! \typedef QHash::key_iterator::reference
    \internal
*/

/*! \typedef QHash::key_iterator::value_type
    \internal
*/

/*! \class QHash::iterator
    \inmodule QtCore
    \brief The QHash::iterator class provides an STL-style non-const iterator for QHash.

    QHash\<Key, T\>::iterator allows you to iterate over a QHash
    and to modify the value (but not the key) associated
    with a particular key. If you want to iterate over a const QHash,
    you should use QHash::const_iterator. It is generally good
    practice to use QHash::const_iterator on a non-const QHash as
    well, unless you need to change the QHash through the iterator.
    Const iterators are slightly faster, and can improve code
    readability.

    The default QHash::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QHash function like
    QHash::begin(), QHash::end(), or QHash::find() before you can
    start iterating. Here's a typical loop that prints all the (key,
    value) pairs stored in a hash:

    \snippet code/src_corelib_tools_qhash.cpp 17

    Unlike QMap, which orders its items by key, QHash stores its
    items in an arbitrary order.

    Here's an example that increments every value stored in the QHash
    by 2:

    \snippet code/src_corelib_tools_qhash.cpp 18

    To remove elements from a QHash you can use erase_if(QHash\<Key, T\> &map, Predicate pred):

    \snippet code/src_corelib_tools_qhash.cpp 21

    Multiple iterators can be used on the same hash. However, be aware
    that any modification performed directly on the QHash (inserting and
    removing items) can cause the iterators to become invalid.

    Inserting items into the hash or calling methods such as QHash::reserve()
    or QHash::squeeze() can invalidate all iterators pointing into the hash.
    Iterators are guaranteed to stay valid only as long as the QHash doesn't have
    to grow/shrink its internal hash table.
    Using any iterator after a rehashing operation has occurred will lead to undefined behavior.

    If you need to keep iterators over a long period of time, we recommend
    that you use QMap rather than QHash.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QHash::const_iterator, QHash::key_iterator, QHash::key_value_iterator
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QHash::begin(), QHash::end()
*/

/*! \fn template <class Key, class T> const Key &QHash<Key, T>::iterator::key() const

    Returns the current item's key as a const reference.

    There is no direct way of changing an item's key through an
    iterator, although it can be done by calling QHash::erase()
    followed by QHash::insert().

    \sa value()
*/

/*! \fn template <class Key, class T> T &QHash<Key, T>::iterator::value() const

    Returns a modifiable reference to the current item's value.

    You can change the value of an item by using value() on
    the left side of an assignment, for example:

    \snippet code/src_corelib_tools_qhash.cpp 22

    \sa key(), operator*()
*/

/*! \fn template <class Key, class T> T &QHash<Key, T>::iterator::operator*() const

    Returns a modifiable reference to the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn template <class Key, class T> T *QHash<Key, T>::iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*!
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator==(const iterator &other) const
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator!=(const iterator &other) const
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::iterator &QHash<Key, T>::iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QHash::end() leads to undefined results.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previously
    current item.
*/

/*! \class QHash::const_iterator
    \inmodule QtCore
    \brief The QHash::const_iterator class provides an STL-style const iterator for QHash.

    QHash\<Key, T\>::const_iterator allows you to iterate over a
    QHash. If you want to modify the QHash as you
    iterate over it, you must use QHash::iterator instead. It is
    generally good practice to use QHash::const_iterator on a
    non-const QHash as well, unless you need to change the QHash
    through the iterator. Const iterators are slightly faster, and
    can improve code readability.

    The default QHash::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QHash
    function like QHash::cbegin(), QHash::cend(), or
    QHash::constFind() before you can start iterating. Here's a typical
    loop that prints all the (key, value) pairs stored in a hash:

    \snippet code/src_corelib_tools_qhash.cpp 23

    Unlike QMap, which orders its items by key, QHash stores its
    items in an arbitrary order. The only guarantee is that items that
    share the same key (because they were inserted using
    a QMultiHash) will appear consecutively, from the most
    recently to the least recently inserted value.

    Multiple iterators can be used on the same hash. However, be aware
    that any modification performed directly on the QHash (inserting and
    removing items) can cause the iterators to become invalid.

    Inserting items into the hash or calling methods such as QHash::reserve()
    or QHash::squeeze() can invalidate all iterators pointing into the hash.
    Iterators are guaranteed to stay valid only as long as the QHash doesn't have
    to grow/shrink its internal hash table.
    Using any iterator after a rehashing operation has occurred will lead to undefined behavior.

    You can however safely use iterators to remove entries from the hash
    using the QHash::erase() method. This function can safely be called while
    iterating, and won't affect the order of items in the hash.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QHash::iterator, QHash::key_iterator, QHash::const_key_value_iterator
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QHash::constBegin(), QHash::constEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class Key, class T> const Key &QHash<Key, T>::const_iterator::key() const

    Returns the current item's key.

    \sa value()
*/

/*! \fn template <class Key, class T> const T &QHash<Key, T>::const_iterator::value() const

    Returns the current item's value.

    \sa key(), operator*()
*/

/*! \fn template <class Key, class T> const T &QHash<Key, T>::const_iterator::operator*() const

    Returns the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn template <class Key, class T> const T *QHash<Key, T>::const_iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::const_iterator &QHash<Key, T>::const_iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QHash::end() leads to undefined results.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previously
    current item.
*/

/*! \class QHash::key_iterator
    \inmodule QtCore
    \since 5.6
    \brief The QHash::key_iterator class provides an STL-style const iterator for QHash keys.

    QHash::key_iterator is essentially the same as QHash::const_iterator
    with the difference that operator*() and operator->() return a key
    instead of a value.

    For most uses QHash::iterator and QHash::const_iterator should be used,
    you can easily access the key by calling QHash::iterator::key():

    \snippet code/src_corelib_tools_qhash.cpp 27

    However, to have interoperability between QHash's keys and STL-style
    algorithms we need an iterator that dereferences to a key instead
    of a value. With QHash::key_iterator we can apply an algorithm to a
    range of keys without having to call QHash::keys(), which is inefficient
    as it costs one QHash iteration and memory allocation to create a temporary
    QList.

    \snippet code/src_corelib_tools_qhash.cpp 28

    QHash::key_iterator is const, it's not possible to modify the key.

    The default QHash::key_iterator constructor creates an uninitialized
    iterator. You must initialize it using a QHash function like
    QHash::keyBegin() or QHash::keyEnd().

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QHash::const_iterator, QHash::iterator
*/

/*! \fn template <class Key, class T> const T &QHash<Key, T>::key_iterator::operator*() const

    Returns the current item's key.
*/

/*! \fn template <class Key, class T> const T *QHash<Key, T>::key_iterator::operator->() const

    Returns a pointer to the current item's key.
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::key_iterator::operator==(key_iterator other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::key_iterator::operator!=(key_iterator other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::key_iterator &QHash<Key, T>::key_iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QHash::keyEnd() leads to undefined results.

*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator QHash<Key, T>::key_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previous
    item.
*/

/*! \fn template <class Key, class T> const_iterator QHash<Key, T>::key_iterator::base() const
    Returns the underlying const_iterator this key_iterator is based on.
*/

/*! \typedef QHash::const_key_value_iterator
    \inmodule QtCore
    \since 5.10
    \brief The QHash::const_key_value_iterator typedef provides an STL-style const iterator for QHash.

    QHash::const_key_value_iterator is essentially the same as QHash::const_iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \typedef QHash::key_value_iterator
    \inmodule QtCore
    \since 5.10
    \brief The QHash::key_value_iterator typedef provides an STL-style iterator for QHash.

    QHash::key_value_iterator is essentially the same as QHash::iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \fn template <class Key, class T> QDataStream &operator<<(QDataStream &out, const QHash<Key, T>& hash)
    \relates QHash

    Writes the hash \a hash to stream \a out.

    This function requires the key and value types to implement \c
    operator<<().

    \sa {Serializing Qt Data Types}
*/

/*! \fn template <class Key, class T> QDataStream &operator>>(QDataStream &in, QHash<Key, T> &hash)
    \relates QHash

    Reads a hash from stream \a in into \a hash.

    This function requires the key and value types to implement \c
    operator>>().

    \sa {Serializing Qt Data Types}
*/

/*! \class QMultiHash
    \inmodule QtCore
    \brief The QMultiHash class is a convenience QHash subclass that provides multi-valued hashes.

    \ingroup tools
    \ingroup shared

    \reentrant

    QMultiHash\<Key, T\> is one of Qt's generic \l{container classes}.
    It inherits QHash and extends it with a few convenience functions
    that make it more suitable than QHash for storing multi-valued
    hashes. A multi-valued hash is a hash that allows multiple values
    with the same key.

    QMultiHash mostly mirrors QHash's API. For example, you can use isEmpty() to test
    whether the hash is empty, and you can traverse a QMultiHash using
    QHash's iterator classes (for example, QHashIterator). But opposed to
    QHash, it provides an insert() function that allows the insertion of
    multiple items with the same key. The replace() function corresponds to
    QHash::insert(). It also provides convenient operator+() and
    operator+=().

    Unlike QMultiMap, QMultiHash does not provide and ordering of the
    inserted items. The only guarantee is that items that
    share the same key will appear consecutively, from the most
    recently to the least recently inserted value.

    Example:
    \snippet code/src_corelib_tools_qhash.cpp 24

    Unlike QHash, QMultiHash provides no operator[]. Use value() or
    replace() if you want to access the most recently inserted item
    with a certain key.

    If you want to retrieve all the values for a single key, you can
    use values(const Key &key), which returns a QList<T>:

    \snippet code/src_corelib_tools_qhash.cpp 25

    The items that share the same key are available from most
    recently to least recently inserted.

    A more efficient approach is to call find() to get
    the STL-style iterator for the first item with a key and iterate from
    there:

    \snippet code/src_corelib_tools_qhash.cpp 26

    QMultiHash's key and value data types must be \l{assignable data
    types}. You cannot, for example, store a QWidget as a value;
    instead, store a QWidget *. In addition, QMultiHash's key type
    must provide operator==(), and there must also be a qHash() function
   in the type's namespace that returns a hash value for an argument of the
    key's type. See the QHash documentation for details.

    \sa QHash, QHashIterator, QMutableHashIterator, QMultiMap
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::QMultiHash()

    Constructs an empty hash.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::QMultiHash(std::initializer_list<std::pair<Key,T> > list)
    \since 5.1

    Constructs a multi-hash with a copy of each of the elements in the
    initializer list \a list.

    This function is only available if the program is being
    compiled in C++11 mode.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::QMultiHash(const QHash<Key, T> &other)

    Constructs a copy of \a other (which can be a QHash or a
    QMultiHash).
*/

/*! \fn template <class Key, class T> template <class InputIterator> QMultiHash<Key, T>::QMultiHash(InputIterator begin, InputIterator end)
    \since 5.14

    Constructs a multi-hash with a copy of each of the elements in the iterator range
    [\a begin, \a end). Either the elements iterated by the range must be
    objects with \c{first} and \c{second} data members (like \c{QPair},
    \c{std::pair}, etc.) convertible to \c Key and to \c T respectively; or the
    iterators must have \c{key()} and \c{value()} member functions, returning a
    key convertible to \c Key and a value convertible to \c T respectively.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::replace(const Key &key, const T &value)

    Inserts a new item with the \a key and a value of \a value.

    If there is already an item with the \a key, that item's value
    is replaced with \a value.

    If there are multiple items with the \a key, the most
    recently inserted item's value is replaced with \a value.

    \sa insert()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::insert(const Key &key, const T &value)

    Inserts a new item with the \a key and a value of \a value.

    If there is already an item with the same key in the hash, this
    function will simply create a new one. (This behavior is
    different from replace(), which overwrites the value of an
    existing item.)

    \sa replace()
*/

/*!
    \fn template <class Key, class T> template <typename ...Args> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::emplace(const Key &key, Args&&... args)
    \fn template <class Key, class T> template <typename ...Args> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::emplace(Key &&key, Args&&... args)

    Inserts a new element into the container. This new element
    is constructed in-place using \a args as the arguments for its
    construction.

    If there is already an item with the same key in the hash, this
    function will simply create a new one. (This behavior is
    different from replace(), which overwrites the value of an
    existing item.)

    Returns an iterator pointing to the new element.

    \sa insert
*/

/*!
    \fn template <class Key, class T> template <typename ...Args> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::emplaceReplace(const Key &key, Args&&... args)
    \fn template <class Key, class T> template <typename ...Args> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::emplaceReplace(Key &&key, Args&&... args)

    Inserts a new element into the container. This new element
    is constructed in-place using \a args as the arguments for its
    construction.

    If there is already an item with the same key in the hash, that item's
    value is replaced with a value constructed from \a args.

    Returns an iterator pointing to the new element.

    \sa replace, emplace
*/


/*! \fn template <class Key, class T> QMultiHash &QMultiHash<Key, T>::unite(const QMultiHash &other)
    \since 5.13

    Inserts all the items in the \a other hash into this hash
    and returns a reference to this hash.

    \sa insert()
*/


/*! \fn template <class Key, class T> QMultiHash &QMultiHash<Key, T>::unite(const QHash<Key, T> &other)
    \since 6.0

    Inserts all the items in the \a other hash into this hash
    and returns a reference to this hash.

    \sa insert()
*/

/*! \fn template <class Key, class T> QList<Key> QMultiHash<Key, T>::uniqueKeys() const
    \since 5.13

    Returns a list containing all the keys in the map. Keys that occur multiple
    times in the map occur only once in the returned list.

    \sa keys(), values()
*/

/*! \fn template <class Key, class T> T QMultiHash<Key, T>::value(const Key &key) const
    \fn template <class Key, class T> T QMultiHash<Key, T>::value(const Key &key, const T &defaultValue) const

    Returns the value associated with the \a key.

    If the hash contains no item with the \a key, the function
    returns \a defaultValue, or a \l{default-constructed value} if this
    parameter has not been supplied.

    If there are multiple
    items for the \a key in the hash, the value of the most recently
    inserted one is returned.
*/

/*! \fn template <class Key, class T> QList<T> QMultiHash<Key, T>::values(const Key &key) const
    \overload

    Returns a list of all the values associated with the \a key,
    from the most recently inserted to the least recently inserted.

    \sa count(), insert()
*/

/*! \fn template <class Key, class T> T &QMultiHash<Key, T>::operator[](const Key &key)

    Returns the value associated with the \a key as a modifiable reference.

    If the hash contains no item with the \a key, the function inserts
    a \l{default-constructed value} into the hash with the \a key, and
    returns a reference to it.

    If the hash contains multiple items with the \a key, this function returns
    a reference to the most recently inserted value.

    \sa insert(), value()
*/

/*! \fn template <class Key, class T> QMultiHash &QMultiHash<Key, T>::operator+=(const QMultiHash &other)

    Inserts all the items in the \a other hash into this hash
    and returns a reference to this hash.

    \sa unite(), insert()
*/

/*! \fn template <class Key, class T> QMultiHash QMultiHash<Key, T>::operator+(const QMultiHash &other) const

    Returns a hash that contains all the items in this hash in
    addition to all the items in \a other. If a key is common to both
    hashes, the resulting hash will contain the key multiple times.

    \sa operator+=()
*/

/*!
    \fn template <class Key, class T> bool QMultiHash<Key, T>::contains(const Key &key, const T &value) const
    \since 4.3

    Returns \c true if the hash contains an item with the \a key and
    \a value; otherwise returns \c false.

    \sa contains()
*/

/*!
    \fn template <class Key, class T> qsizetype QMultiHash<Key, T>::remove(const Key &key)
    \since 4.3

    Removes all the items that have the \a key from the hash.
    Returns the number of items removed.

    \sa remove()
*/

/*!
    \fn template <class Key, class T> qsizetype QMultiHash<Key, T>::remove(const Key &key, const T &value)
    \since 4.3

    Removes all the items that have the \a key and the value \a
    value from the hash. Returns the number of items removed.

    \sa remove()
*/

/*!
    \fn template <class Key, class T> void QMultiHash<Key, T>::clear()
    \since 4.3

    Removes all items from the hash and frees up all memory used by it.

    \sa remove()
*/

/*! \fn template <class Key, class T> template <typename Predicate> qsizetype QMultiHash<Key, T>::removeIf(Predicate pred)
    \since 6.1

    Removes all elements for which the predicate \a pred returns true
    from the multi hash.

    The function supports predicates which take either an argument of
    type \c{QMultiHash<Key, T>::iterator}, or an argument of type
    \c{std::pair<const Key &, T &>}.

    Returns the number of elements removed, if any.

    \sa clear(), take()
*/

/*! \fn template <class Key, class T> T QMultiHash<Key, T>::take(const Key &key)

    Removes the item with the \a key from the hash and returns
    the value associated with it.

    If the item does not exist in the hash, the function simply
    returns a \l{default-constructed value}. If there are multiple
    items for \a key in the hash, only the most recently inserted one
    is removed.

    If you don't use the return value, remove() is more efficient.

    \sa remove()
*/

/*! \fn template <class Key, class T> QList<Key> QMultiHash<Key, T>::keys() const

    Returns a list containing all the keys in the hash, in an
    arbitrary order. Keys that occur multiple times in the hash
    also occur multiple times in the list.

    The order is guaranteed to be the same as that used by values().

    This function creates a new list, in \l {linear time}. The time and memory
    use that entails can be avoided by iterating from \l keyBegin() to
    \l keyEnd().

    \sa values(), key()
*/

/*! \fn template <class Key, class T> QList<T> QMultiHash<Key, T>::values() const

    Returns a list containing all the values in the hash, in an
    arbitrary order. If a key is associated with multiple values, all of
    its values will be in the list, and not just the most recently
    inserted one.

    The order is guaranteed to be the same as that used by keys().

    This function creates a new list, in \l {linear time}. The time and memory
    use that entails can be avoided by iterating from \l keyValueBegin() to
    \l keyValueEnd().

    \sa keys(), value()
*/

/*!
    \fn template <class Key, class T> Key QMultiHash<Key, T>::key(const T &value) const
    \fn template <class Key, class T> Key QMultiHash<Key, T>::key(const T &value, const Key &defaultKey) const
    \since 4.3

    Returns the first key mapped to \a value. If the hash contains no item
    mapped to \a value, returns \a defaultKey, or a \l{default-constructed
    value}{default-constructed key} if this parameter has not been supplied.

    This function can be slow (\l{linear time}), because QMultiHash's
    internal data structure is optimized for fast lookup by key, not
    by value.
*/

/*!
    \fn template <class Key, class T> qsizetype QMultiHash<Key, T>::count(const Key &key, const T &value) const
    \since 4.3

    Returns the number of items with the \a key and \a value.

    \sa count()
*/

/*!
    \fn template <class Key, class T> typename QMultiHash<Key, T>::iterator QMultiHash<Key, T>::find(const Key &key, const T &value)
    \since 4.3

    Returns an iterator pointing to the item with the \a key and \a value.
    If the hash contains no such item, the function returns end().

    If the hash contains multiple items with the \a key and \a value, the
    iterator returned points to the most recently inserted item.
*/

/*!
    \fn template <class Key, class T> typename QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::find(const Key &key, const T &value) const
    \since 4.3
    \overload
*/

/*!
    \fn template <class Key, class T> typename QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::constFind(const Key &key, const T &value) const
    \since 4.3

    Returns an iterator pointing to the item with the \a key and the
    \a value in the hash.

    If the hash contains no such item, the function returns
    constEnd().
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the hash.

    \sa constBegin(), end()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::begin() const

    \overload
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the hash.

    \sa begin(), cend()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the hash.

    \sa begin(), constEnd()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::key_iterator QMultiHash<Key, T>::keyBegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first key
    in the hash.

    \sa keyEnd()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the hash.

    \sa begin(), constEnd()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::end() const

    \overload
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the hash.

    \sa constBegin(), end()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the hash.

    \sa cbegin(), end()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::key_iterator QMultiHash<Key, T>::keyEnd() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last key in the hash.

    \sa keyBegin()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::key_value_iterator QMultiHash<Key, T>::keyValueBegin()
    \since 5.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueEnd()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::key_value_iterator QMultiHash<Key, T>::keyValueEnd()
    \since 5.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_key_value_iterator QMultiHash<Key, T>::keyValueBegin() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueEnd()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_key_value_iterator QMultiHash<Key, T>::constKeyValueBegin() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_key_value_iterator QMultiHash<Key, T>::keyValueEnd() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_key_value_iterator QMultiHash<Key, T>::constKeyValueEnd() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa constKeyValueBegin()
*/

/*! \fn template <class Key, class T> auto QMultiHash<Key, T>::asKeyValueRange() &
    \fn template <class Key, class T> auto QMultiHash<Key, T>::asKeyValueRange() const &
    \fn template <class Key, class T> auto QMultiHash<Key, T>::asKeyValueRange() &&
    \fn template <class Key, class T> auto QMultiHash<Key, T>::asKeyValueRange() const &&
    \since 6.4

    Returns a range object that allows iteration over this hash as
    key/value pairs. For instance, this range object can be used in a
    range-based for loop, in combination with a structured binding declaration:

    \snippet code/src_corelib_tools_qhash.cpp 35

    Note that both the key and the value obtained this way are
    references to the ones in the hash. Specifically, mutating the value
    will modify the hash itself.

    \sa QKeyValueIterator
*/

/*! \class QMultiHash::iterator
    \inmodule QtCore
    \brief The QMultiHash::iterator class provides an STL-style non-const iterator for QMultiHash.

    QMultiHash\<Key, T\>::iterator allows you to iterate over a QMultiHash
    and to modify the value (but not the key) associated
    with a particular key. If you want to iterate over a const QMultiHash,
    you should use QMultiHash::const_iterator. It is generally good
    practice to use QMultiHash::const_iterator on a non-const QMultiHash as
    well, unless you need to change the QMultiHash through the iterator.
    Const iterators are slightly faster, and can improve code
    readability.

    The default QMultiHash::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QMultiHash function like
    QMultiHash::begin(), QMultiHash::end(), or QMultiHash::find() before you can
    start iterating. Here's a typical loop that prints all the (key,
    value) pairs stored in a hash:

    \snippet code/src_corelib_tools_qhash.cpp 17

    Unlike QMap, which orders its items by key, QMultiHash stores its
    items in an arbitrary order.

    Here's an example that increments every value stored in the QMultiHash
    by 2:

    \snippet code/src_corelib_tools_qhash.cpp 18

    To remove elements from a QMultiHash you can use erase_if(QMultiHash\<Key, T\> &map, Predicate pred):

    \snippet code/src_corelib_tools_qhash.cpp 21

    Multiple iterators can be used on the same hash. However, be aware
    that any modification performed directly on the QHash (inserting and
    removing items) can cause the iterators to become invalid.

    Inserting items into the hash or calling methods such as QHash::reserve()
    or QHash::squeeze() can invalidate all iterators pointing into the hash.
    Iterators are guaranteed to stay valid only as long as the QHash doesn't have
    to grow/shrink its internal hash table.
    Using any iterator after a rehashing operation has occurred will lead to undefined behavior.

    If you need to keep iterators over a long period of time, we recommend
    that you use QMultiMap rather than QHash.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QMultiHash::const_iterator, QMultiHash::key_iterator, QMultiHash::key_value_iterator
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QMultiHash::begin(), QMultiHash::end()
*/

/*! \fn template <class Key, class T> const Key &QMultiHash<Key, T>::iterator::key() const

    Returns the current item's key as a const reference.

    There is no direct way of changing an item's key through an
    iterator, although it can be done by calling QMultiHash::erase()
    followed by QMultiHash::insert().

    \sa value()
*/

/*! \fn template <class Key, class T> T &QMultiHash<Key, T>::iterator::value() const

    Returns a modifiable reference to the current item's value.

    You can change the value of an item by using value() on
    the left side of an assignment, for example:

    \snippet code/src_corelib_tools_qhash.cpp 22

    \sa key(), operator*()
*/

/*! \fn template <class Key, class T> T &QMultiHash<Key, T>::iterator::operator*() const

    Returns a modifiable reference to the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn template <class Key, class T> T *QMultiHash<Key, T>::iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*!
    \fn template <class Key, class T> bool QMultiHash<Key, T>::iterator::operator==(const iterator &other) const
    \fn template <class Key, class T> bool QMultiHash<Key, T>::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template <class Key, class T> bool QMultiHash<Key, T>::iterator::operator!=(const iterator &other) const
    \fn template <class Key, class T> bool QMultiHash<Key, T>::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QMultiHash<Key, T>::iterator &QMultiHash<Key, T>::iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QMultiHash::end() leads to undefined results.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previously
    current item.
*/

/*! \class QMultiHash::const_iterator
    \inmodule QtCore
    \brief The QMultiHash::const_iterator class provides an STL-style const iterator for QMultiHash.

    QMultiHash\<Key, T\>::const_iterator allows you to iterate over a
    QMultiHash. If you want to modify the QMultiHash as you
    iterate over it, you must use QMultiHash::iterator instead. It is
    generally good practice to use QMultiHash::const_iterator on a
    non-const QMultiHash as well, unless you need to change the QMultiHash
    through the iterator. Const iterators are slightly faster, and
    can improve code readability.

    The default QMultiHash::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QMultiHash
    function like QMultiHash::cbegin(), QMultiHash::cend(), or
    QMultiHash::constFind() before you can start iterating. Here's a typical
    loop that prints all the (key, value) pairs stored in a hash:

    \snippet code/src_corelib_tools_qhash.cpp 23

    Unlike QMap, which orders its items by key, QMultiHash stores its
    items in an arbitrary order. The only guarantee is that items that
    share the same key (because they were inserted using
    a QMultiHash) will appear consecutively, from the most
    recently to the least recently inserted value.

    Multiple iterators can be used on the same hash. However, be aware
    that any modification performed directly on the QMultiHash (inserting and
    removing items) can cause the iterators to become invalid.

    Inserting items into the hash or calling methods such as QMultiHash::reserve()
    or QMultiHash::squeeze() can invalidate all iterators pointing into the hash.
    Iterators are guaranteed to stay valid only as long as the QMultiHash doesn't have
    to grow/shrink it's internal hash table.
    Using any iterator after a rehashing operation ahs occurred will lead to undefined behavior.

    If you need to keep iterators over a long period of time, we recommend
    that you use QMultiMap rather than QMultiHash.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QMultiHash::iterator, QMultiHash::key_iterator, QMultiHash::const_key_value_iterator
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QMultiHash::constBegin(), QMultiHash::constEnd()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class Key, class T> const Key &QMultiHash<Key, T>::const_iterator::key() const

    Returns the current item's key.

    \sa value()
*/

/*! \fn template <class Key, class T> const T &QMultiHash<Key, T>::const_iterator::value() const

    Returns the current item's value.

    \sa key(), operator*()
*/

/*! \fn template <class Key, class T> const T &QMultiHash<Key, T>::const_iterator::operator*() const

    Returns the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn template <class Key, class T> const T *QMultiHash<Key, T>::const_iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*! \fn template <class Key, class T> bool QMultiHash<Key, T>::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QMultiHash<Key, T>::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator &QMultiHash<Key, T>::const_iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QMultiHash::end() leads to undefined results.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::const_iterator QMultiHash<Key, T>::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previously
    current item.
*/

/*! \class QMultiHash::key_iterator
    \inmodule QtCore
    \since 5.6
    \brief The QMultiHash::key_iterator class provides an STL-style const iterator for QMultiHash keys.

    QMultiHash::key_iterator is essentially the same as QMultiHash::const_iterator
    with the difference that operator*() and operator->() return a key
    instead of a value.

    For most uses QMultiHash::iterator and QMultiHash::const_iterator should be used,
    you can easily access the key by calling QMultiHash::iterator::key():

    \snippet code/src_corelib_tools_qhash.cpp 27

    However, to have interoperability between QMultiHash's keys and STL-style
    algorithms we need an iterator that dereferences to a key instead
    of a value. With QMultiHash::key_iterator we can apply an algorithm to a
    range of keys without having to call QMultiHash::keys(), which is inefficient
    as it costs one QMultiHash iteration and memory allocation to create a temporary
    QList.

    \snippet code/src_corelib_tools_qhash.cpp 28

    QMultiHash::key_iterator is const, it's not possible to modify the key.

    The default QMultiHash::key_iterator constructor creates an uninitialized
    iterator. You must initialize it using a QMultiHash function like
    QMultiHash::keyBegin() or QMultiHash::keyEnd().

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QMultiHash::const_iterator, QMultiHash::iterator
*/

/*! \fn template <class Key, class T> const T &QMultiHash<Key, T>::key_iterator::operator*() const

    Returns the current item's key.
*/

/*! \fn template <class Key, class T> const T *QMultiHash<Key, T>::key_iterator::operator->() const

    Returns a pointer to the current item's key.
*/

/*! \fn template <class Key, class T> bool QMultiHash<Key, T>::key_iterator::operator==(key_iterator other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QMultiHash<Key, T>::key_iterator::operator!=(key_iterator other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QMultiHash<Key, T>::key_iterator &QMultiHash<Key, T>::key_iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QMultiHash::keyEnd() leads to undefined results.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::key_iterator QMultiHash<Key, T>::key_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previous
    item.
*/

/*! \fn template <class Key, class T> const_iterator QMultiHash<Key, T>::key_iterator::base() const
    Returns the underlying const_iterator this key_iterator is based on.
*/

/*! \typedef QMultiHash::const_key_value_iterator
    \inmodule QtCore
    \since 5.10
    \brief The QMap::const_key_value_iterator typedef provides an STL-style const iterator for QMultiHash and QMultiHash.

    QMultiHash::const_key_value_iterator is essentially the same as QMultiHash::const_iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \typedef QMultiHash::key_value_iterator
    \inmodule QtCore
    \since 5.10
    \brief The QMap::key_value_iterator typedef provides an STL-style iterator for QMultiHash and QMultiHash.

    QMultiHash::key_value_iterator is essentially the same as QMultiHash::iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \fn template <class Key, class T> QDataStream &operator<<(QDataStream &out, const QMultiHash<Key, T>& hash)
    \relates QMultiHash

    Writes the hash \a hash to stream \a out.

    This function requires the key and value types to implement \c
    operator<<().

    \sa {Serializing Qt Data Types}
*/

/*! \fn template <class Key, class T> QDataStream &operator>>(QDataStream &in, QMultiHash<Key, T> &hash)
    \relates QMultiHash

    Reads a hash from stream \a in into \a hash.

    This function requires the key and value types to implement \c
    operator>>().

    \sa {Serializing Qt Data Types}
*/

/*!
    \fn template <class Key, class T> size_t qHash(const QHash<Key, T> &key, size_t seed = 0)
    \since 5.8
    \relates QHash

    Returns the hash value for the \a key, using \a seed to seed the calculation.

    Type \c T must be supported by qHash().
*/

/*!
    \fn template <class Key, class T> size_t qHash(const QMultiHash<Key, T> &key, size_t seed = 0)
    \since 5.8
    \relates QMultiHash

    Returns the hash value for the \a key, using \a seed to seed the calculation.

    Type \c T must be supported by qHash().
*/

/*! \fn template <typename Key, typename T, typename Predicate> qsizetype erase_if(QHash<Key, T> &hash, Predicate pred)
    \relates QHash
    \since 6.1

    Removes all elements for which the predicate \a pred returns true
    from the hash \a hash.

    The function supports predicates which take either an argument of
    type \c{QHash<Key, T>::iterator}, or an argument of type
    \c{std::pair<const Key &, T &>}.

    Returns the number of elements removed, if any.
*/

/*! \fn template <typename Key, typename T, typename Predicate> qsizetype erase_if(QMultiHash<Key, T> &hash, Predicate pred)
    \relates QMultiHash
    \since 6.1

    Removes all elements for which the predicate \a pred returns true
    from the multi hash \a hash.

    The function supports predicates which take either an argument of
    type \c{QMultiHash<Key, T>::iterator}, or an argument of type
    \c{std::pair<const Key &, T &>}.

    Returns the number of elements removed, if any.
*/

#ifdef QT_HAS_CONSTEXPR_BITOPS
namespace QHashPrivate {
static_assert(qPopulationCount(SpanConstants::NEntries) == 1,
        "NEntries must be a power of 2 for bucketForHash() to work.");

// ensure the size of a Span does not depend on the template parameters
using Node1 = Node<int, int>;
static_assert(sizeof(Span<Node1>) == sizeof(Span<Node<char, void *>>));
static_assert(sizeof(Span<Node1>) == sizeof(Span<Node<qsizetype, QHashDummyValue>>));
static_assert(sizeof(Span<Node1>) == sizeof(Span<Node<QString, QVariant>>));
static_assert(sizeof(Span<Node1>) > SpanConstants::NEntries);
static_assert(qNextPowerOfTwo(sizeof(Span<Node1>)) == SpanConstants::NEntries * 2);

// ensure allocations are always a power of two, at a minimum NEntries,
// obeying the fomula
//   qNextPowerOfTwo(2 * N);
// without overflowing
static constexpr size_t NEntries = SpanConstants::NEntries;
static_assert(GrowthPolicy::bucketsForCapacity(1) == NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(NEntries / 2 + 0) == NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(NEntries / 2 + 1) == 2 * NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(NEntries * 1 - 1) == 2 * NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(NEntries * 1 + 0) == 4 * NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(NEntries * 1 + 1) == 4 * NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(NEntries * 2 - 1) == 4 * NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(NEntries * 2 + 0) == 8 * NEntries);
static_assert(GrowthPolicy::bucketsForCapacity(SIZE_MAX / 4) == SIZE_MAX / 2 + 1);
static_assert(GrowthPolicy::bucketsForCapacity(SIZE_MAX / 2) == SIZE_MAX);
static_assert(GrowthPolicy::bucketsForCapacity(SIZE_MAX) == SIZE_MAX);
}
#endif

QT_END_NAMESPACE
