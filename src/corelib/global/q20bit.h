// Copyright (C) 2025 Intel Corporation.
// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef Q20BIT_H
#define Q20BIT_H

#include <QtCore/q20type_traits.h>

#if defined(__cpp_lib_bitops) || defined(__cpp_lib_int_pow2)
#  include <bit>
#else
#  include <QtCore/qtypes.h>
#  include <limits>

#  ifdef Q_CC_MSVC
// avoiding qsimd.h -> immintrin.h unless necessary, because it increases
// compilation time
#    include <QtCore/qsimd.h>
#    include <intrin.h>
#  endif
#endif

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

namespace q20 {
#if defined(__cpp_lib_bitops)
using std::countl_zero;
using std::countr_zero;
using std::popcount;
using std::rotl;
using std::rotr;
#else
namespace detail {
template <typename T> /*non-constexpr*/ inline auto hw_popcount(T v) noexcept
{
#if defined(Q_CC_MSVC) && defined(Q_PROCESSOR_ARM_64)
    if constexpr (sizeof(T) == sizeof(quint64))
        return int(_CountOneBits64(v));
    return int(_CountOneBits(v));
#endif
#if defined(Q_CC_MSVC) && defined(Q_PROCESSOR_X86) && defined(__POPCNT__)
    // Note: __POPCNT__ comes from qsimd.h, not the compiler.
#  ifdef Q_PROCESSOR_X86_64
    if constexpr (sizeof(T) == sizeof(quint64))
        return int(__popcnt64(v));
#  endif
    if constexpr (sizeof(T) == sizeof(quint64))
        return int(__popcnt(quint32(v)) + __popcnt(quint32(v >> 32)));
    if constexpr (sizeof(T) == sizeof(quint32))
        return int(__popcnt(v));
    return int(__popcnt16(v));
#else
    Q_UNUSED(v);
#endif
}

template <typename T> /*non-constexpr*/ inline auto hw_countl_zero(T v) noexcept
{
#if defined(Q_CC_MSVC_ONLY) && defined(Q_PROCESSOR_ARM_64)
    if constexpr (sizeof(T) == sizeof(quint64))
        return int(_CountLeadingZeros64(v));
    return int(_CountLeadingZeros(v)) - (32 - std::numeric_limits<T>::digits);
#endif
#if defined(Q_CC_MSVC) && defined(Q_PROCESSOR_X86) && defined(__LZCNT__)
    // Note: __LZCNT__ comes from qsimd.h, not the compiler
#  if defined(Q_PROCESSOR_X86_64)
    if constexpr (sizeof(T) == sizeof(quint64))
        return int(__lzcnt64(v));
#  endif
    if constexpr (sizeof(T) == sizeof(quint32))
        return int(__lzcnt(v));
    if constexpr (sizeof(T) == sizeof(quint16))
        return int(__lzcnt16(v));
    if constexpr (sizeof(T) == sizeof(quint8))
        return int(__lzcnt(v)) - 24;
#endif
#if defined(Q_CC_MSVC) && defined(Q_PROCESSOR_X86)
    constexpr int Digits = std::numeric_limits<T>::digits;
    unsigned long result;

    if constexpr (sizeof(T) == sizeof(quint64)) {
#  ifdef Q_PROCESSOR_X86_64
        if (_BitScanReverse64(&result, v) == 0)
            return Digits;
#  else
        if (quint32 h = quint32(v >> 32))
            return hw_countl_zero(h);
        return hw_countl_zero(quint32(v)) + 32;
#  endif
    } else {
        if (_BitScanReverse(&result, v) == 0)
            return Digits;
    }

    // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
    // and the lsb index is 0. The result for the index when counting up: msb index is 0 (because it
    // starts there), and the lsb index is 31.
    result ^= sizeof(T) * 8 - 1;
    return int(result);
#else
    Q_UNUSED(v);
#endif
}

template <typename T> /*non-constexpr*/ inline auto hw_countr_zero(T v) noexcept
{
#if defined(Q_CC_MSVC_ONLY) && defined(Q_PROCESSOR_ARM_64)
    if constexpr (sizeof(T) == sizeof(quint64))
        return int(_CountTrailingZeros64(v));
    constexpr int Digits = std::numeric_limits<T>::digits;
    const int result = int(_CountTrailingZeros(v));
    return result > Digits ? Digits : result;
#endif
#if defined(Q_CC_MSVC) && defined(Q_PROCESSOR_X86) && defined(__BMI__)
    // Note: __BMI__ comes from qsimd.h, not the compiler
#  if defined(Q_PROCESSOR_X86_64)
    if constexpr (sizeof(T) == sizeof(quint64))
        return int(_tzcnt_u64(v));
#  endif
    if constexpr (sizeof(T) == sizeof(quint32))
        return int(_tzcnt_u32(v));
    // No _tzcnt_u16 or u8 intrinsics
#endif
#if defined(Q_CC_MSVC) && defined(Q_PROCESSOR_X86)
    constexpr int Digits = std::numeric_limits<T>::digits;
    unsigned long result;
    if constexpr (sizeof(T) <= sizeof(quint32))
        return _BitScanForward(&result, v) ? int(result) : Digits;
#  ifdef Q_PROCESSOR_X86_64
    return _BitScanForward64(&result, v) ? int(result) : Digits;
#  endif
#else
    Q_UNUSED(v);
#endif
}
} // namespace q20::detail

template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, int>
popcount(T v) noexcept
{
#if __has_builtin(__builtin_popcountg)
    return __builtin_popcountg(v);
#endif

    if constexpr (sizeof(T) > sizeof(quint64)) {
        static_assert(sizeof(T) == 16, "Unsupported integer size");
        return popcount(quint64(v)) + popcount(quint64(v >> 64));
    }

#  if __has_builtin(__builtin_popcount)
    // These GCC/Clang intrinsics are constexpr and use the HW instructions
    // where available. Note: no runtime detection.
    if constexpr (sizeof(T) > sizeof(quint32))
        return __builtin_popcountll(v);
    return __builtin_popcount(v);
#  endif

#  ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
    // Try hardware functions if not constexpr. Note: no runtime detection.
    if (!is_constant_evaluated()) {
        if constexpr (std::is_integral_v<decltype(detail::hw_popcount(v))>)
            return detail::hw_popcount(v);
    }
#  endif

    constexpr int Digits = std::numeric_limits<T>::digits;
    int r =  (((v      ) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
    if constexpr (Digits > 12)
        r += (((v >> 12) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
    if constexpr (Digits > 24)
        r += (((v >> 24) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
    if constexpr (Digits > 36) {
        r += (((v >> 36) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
             (((v >> 48) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f +
             (((v >> 60) & 0xfff)    * Q_UINT64_C(0x1001001001001) & Q_UINT64_C(0x84210842108421)) % 0x1f;
    }
    return r;
}

template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, int>
countl_zero(T v) noexcept
{
#if __has_builtin(__builtin_clzg)
    // "If two arguments are specified, and first argument is 0, the result is
    // the second argument."
    return __builtin_clzg(v, std::numeric_limits<T>::digits);
#endif

    if constexpr (sizeof(T) > sizeof(quint64)) {
        static_assert(sizeof(T) == 16, "Unsupported integer size");
        if (quint64 h = quint64(v >> 64))
            return countl_zero(h);
        return countl_zero(quint64(v)) + 64;
    }

#if __has_builtin(__builtin_clz)
    // These GCC/Clang intrinsics are constexpr and use the HW instructions
    // where available.
    if (!v)
        return std::numeric_limits<T>::digits;
    if constexpr (sizeof(T) == sizeof(quint64))
        return __builtin_clzll(v);
#  if __has_builtin(__builtin_clzs)
    if constexpr (sizeof(T) == sizeof(quint16))
        return __builtin_clzs(v);
#  endif
    return __builtin_clz(v) - (32 - std::numeric_limits<T>::digits);
#endif

#ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
    // Try hardware functions if not constexpr. Note: no runtime detection.
    if (!is_constant_evaluated()) {
        if constexpr (std::is_integral_v<decltype(detail::hw_countl_zero(v))>)
            return detail::hw_countl_zero(v);
    }
#endif

    // Hacker's Delight, 2nd ed. Fig 5-16, p. 102
    v = v | (v >> 1);
    v = v | (v >> 2);
    v = v | (v >> 4);
    if constexpr (sizeof(T) > sizeof(quint8))
        v = v | (v >> 8);
    if constexpr (sizeof(T) > sizeof(quint16))
        v = v | (v >> 16);
    if constexpr (sizeof(T) > sizeof(quint32))
        v = v | (v >> 32);
    return popcount(T(~v));
}

template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, int>
countr_zero(T v) noexcept
{
#if __has_builtin(__builtin_ctzg)
    // "If two arguments are specified, and first argument is 0, the result is
    // the second argument."
    return __builtin_ctzg(v, std::numeric_limits<T>::digits);
#endif

    if constexpr (sizeof(T) > sizeof(quint64)) {
        static_assert(sizeof(T) == 16, "Unsupported integer size");
        quint64 l = quint64(v);
        return l ? countr_zero(l) : 64 + countr_zero(quint64(v >> 64));
    }

#if __has_builtin(__builtin_ctz)
    // These GCC/Clang intrinsics are constexpr and use the HW instructions
    // where available.
    if (!v)
        return std::numeric_limits<T>::digits;
    if constexpr (sizeof(T) == sizeof(quint64))
        return __builtin_ctzll(v);
#  if __has_builtin(__builtin_ctzs)
    if constexpr (sizeof(T) == sizeof(quint16))
        return __builtin_ctzs(v);
#  endif
    return __builtin_ctz(v);
#endif

#ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
    // Try hardware functions if not constexpr. Note: no runtime detection.
    if (!is_constant_evaluated()) {
        if constexpr (std::is_integral_v<decltype(detail::hw_countr_zero(v))>)
            return detail::hw_countr_zero(v);
    }
#endif

    if constexpr (sizeof(T) > sizeof(quint32)) {
        quint32 l = quint32(v);
        return l ? countr_zero(l) : 32 + countr_zero(quint32(v >> 32));
    }

    // see http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightParallel
    int c = std::numeric_limits<T>::digits; // c will be the number of zero bits on the right
QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4146)   // unary minus operator applied to unsigned type, result still unsigned
    v &= T(-v);
QT_WARNING_POP
    if (v) c--;
    if constexpr (sizeof(T) == sizeof(quint32)) {
        if (v & 0x0000FFFF) c -= 16;
        if (v & 0x00FF00FF) c -= 8;
        if (v & 0x0F0F0F0F) c -= 4;
        if (v & 0x33333333) c -= 2;
        if (v & 0x55555555) c -= 1;
    } else if constexpr (sizeof(T) == sizeof(quint16)) {
        if (v & 0x000000FF) c -= 8;
        if (v & 0x00000F0F) c -= 4;
        if (v & 0x00003333) c -= 2;
        if (v & 0x00005555) c -= 1;
    } else /*if constexpr (sizeof(T) == sizeof(quint8))*/ {
        if (v & 0x0000000F) c -= 4;
        if (v & 0x00000033) c -= 2;
        if (v & 0x00000055) c -= 1;
    }
    return c;
}

template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, T>
rotl(T v, int s) noexcept
{
    constexpr int Digits = std::numeric_limits<T>::digits;
    unsigned n = unsigned(s) % Digits;
    return (v << n) | (v >> (Digits - n));
}

template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, T>
rotr(T v, int s) noexcept
{
    constexpr int Digits = std::numeric_limits<T>::digits;
    unsigned n = unsigned(s) % Digits;
    return (v >> n) | (v << (Digits - n));
}
#endif // __cpp_lib_bitops

#if defined(__cpp_lib_int_pow2)
using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
#else
template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, T>
bit_ceil(T v) noexcept
{
    // Difference from standard: we do not enforce UB
    constexpr int Digits = std::numeric_limits<T>::digits;
    if (v <= 1)
        return 1;
    return T(1) << (Digits - countl_zero(T(v - 1)));
}

template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, T>
bit_width(T v) noexcept
{
    return std::numeric_limits<T>::digits - countl_zero(v);
}

template <typename T> constexpr std::enable_if_t<std::is_unsigned_v<T>, T>
bit_floor(T v) noexcept
{
    return v ? T(1) << (bit_width(v) - 1) : 0;
}
#endif // __cpp_lib_int_pow2
} // namespace q20

QT_END_NAMESPACE

#endif // Q20BIT_H
