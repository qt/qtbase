// Copyright (C) 2012-2014 Jean-Philippe Aumasson
// Copyright (C) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: CC0-1.0

#include <QtCore/qassert.h>
#include <QtCore/qcompilerdetection.h>
#include <QtCore/qendian.h>

#ifdef Q_CC_GNU
#  define DECL_HOT_FUNCTION       __attribute__((hot))
#else
#  define DECL_HOT_FUNCTION
#endif

#include <cstdint>

QT_USE_NAMESPACE

namespace {

// This is an inlined version of the SipHash implementation that is
// trying to avoid some memcpy's from uint64 to uint8[] and back.

#define ROTL(x, b) (((x) << (b)) | ((x) >> (sizeof(x) * 8 - (b))))

#define SIPROUND                                                                       \
do {                                                                                   \
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

template <int cROUNDS = 2, int dROUNDS = 4> struct SipHash64
{
    /* "somepseudorandomlygeneratedbytes" */
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t b;
    uint64_t k0;
    uint64_t k1;

    inline SipHash64(uint64_t fulllen, uint64_t seed, uint64_t seed2);
    inline void addBlock(const uint8_t *in, size_t inlen);
    inline uint64_t finalize(const uint8_t *in, size_t left);
};

template <int cROUNDS, int dROUNDS>
SipHash64<cROUNDS, dROUNDS>::SipHash64(uint64_t inlen, uint64_t seed, uint64_t seed2)
{
    b = inlen << 56;
    k0 = seed;
    k1 = seed2;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;
}

template <int cROUNDS, int dROUNDS> DECL_HOT_FUNCTION void
SipHash64<cROUNDS, dROUNDS>::addBlock(const uint8_t *in, size_t inlen)
{
    Q_ASSERT((inlen & 7ULL) == 0);
    int i;
    const uint8_t *end = in + inlen;
    for (; in != end; in += 8) {
        uint64_t m = qFromUnaligned<uint64_t>(in);
        v3 ^= m;

        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }
}

template <int cROUNDS, int dROUNDS> DECL_HOT_FUNCTION uint64_t
SipHash64<cROUNDS, dROUNDS>::finalize(const uint8_t *in, size_t left)
{
    int i;
    switch (left) {
    case 7:
        b |= ((uint64_t)in[6]) << 48;
        Q_FALLTHROUGH();
    case 6:
        b |= ((uint64_t)in[5]) << 40;
        Q_FALLTHROUGH();
    case 5:
        b |= ((uint64_t)in[4]) << 32;
        Q_FALLTHROUGH();
    case 4:
        b |= ((uint64_t)in[3]) << 24;
        Q_FALLTHROUGH();
    case 3:
        b |= ((uint64_t)in[2]) << 16;
        Q_FALLTHROUGH();
    case 2:
        b |= ((uint64_t)in[1]) << 8;
        Q_FALLTHROUGH();
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
#undef SIPROUND

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

#define SIPROUND                                                                       \
do {                                                                                   \
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

template <int cROUNDS = 2, int dROUNDS = 4> struct SipHash32
{
    /* "somepseudorandomlygeneratedbytes" */
    uint v0 = 0x736f6d65U;
    uint v1 = 0x646f7261U;
    uint v2 = 0x6c796765U;
    uint v3 = 0x74656462U;
    uint b;
    uint k0;
    uint k1;

    inline SipHash32(size_t fulllen, uint seed, uint seed2);
    inline void addBlock(const uint8_t *in, size_t inlen);
    inline uint finalize(const uint8_t *in, size_t left);
};

template <int cROUNDS, int dROUNDS> inline
        SipHash32<cROUNDS, dROUNDS>::SipHash32(size_t inlen, uint seed, uint seed2)
{
    uint k0 = seed;
    uint k1 = seed2;
    b = inlen << 24;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;
}

template <int cROUNDS, int dROUNDS> inline DECL_HOT_FUNCTION void
SipHash32<cROUNDS, dROUNDS>::addBlock(const uint8_t *in, size_t inlen)
{
    Q_ASSERT((inlen & 3ULL) == 0);
    int i;
    const uint8_t *end = in + inlen;
    for (; in != end; in += 4) {
        uint m = qFromUnaligned<uint>(in);
        v3 ^= m;

        for (i = 0; i < cROUNDS; ++i)
            SIPROUND;

        v0 ^= m;
    }
}

template <int cROUNDS, int dROUNDS> inline DECL_HOT_FUNCTION uint
SipHash32<cROUNDS, dROUNDS>::finalize(const uint8_t *in, size_t left)
{
    int i;
    switch (left) {
    case 3:
        b |= ((uint)in[2]) << 16;
        Q_FALLTHROUGH();
    case 2:
        b |= ((uint)in[1]) << 8;
        Q_FALLTHROUGH();
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
#undef SIPROUND
#undef ROTL

// Use SipHash-1-2, which has similar performance characteristics as
// Qt 4's hash implementation, instead of the SipHash-2-4 default
template <int cROUNDS = 1, int dROUNDS = 2>
using SipHash = std::conditional_t<sizeof(void *) == 8,
                                   SipHash64<cROUNDS, dROUNDS>, SipHash32<cROUNDS, dROUNDS>>;

} // unnamed namespace

Q_NEVER_INLINE DECL_HOT_FUNCTION
static size_t siphash(const uint8_t *in, size_t inlen, size_t seed, size_t seed2)
{
    constexpr size_t TailSizeMask = sizeof(void *) - 1;
    SipHash<> hasher(inlen, seed, seed2);
    hasher.addBlock(in, inlen & ~TailSizeMask);
    return hasher.finalize(in + (inlen & ~TailSizeMask), inlen & TailSizeMask);
}
