/****************************************************************************
**
** Copyright (C) 2019 Intel Corporation.
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

// for rand_s
#define _CRT_RAND_S

#include "qrandom.h"
#include "qrandom_p.h"
#include <qobjectdefs.h>
#include <qmutex.h>
#include <qthreadstorage.h>

#include <errno.h>

#if !QT_CONFIG(getentropy) && (!defined(Q_OS_BSD4) || defined(__GLIBC__)) && !defined(Q_OS_WIN)
#  include "qdeadlinetimer.h"
#  include "qhashfunctions.h"

#  if QT_CONFIG(getauxval)
#    include <sys/auxv.h>
#  endif
#endif // !QT_CONFIG(getentropy)

#ifdef Q_OS_UNIX
#  include <fcntl.h>
#  include <private/qcore_unix_p.h>
#else
#  include <qt_windows.h>

// RtlGenRandom is not exported by its name in advapi32.dll, but as SystemFunction036
// See https://msdn.microsoft.com/en-us/library/windows/desktop/aa387694(v=vs.85).aspx
// Implementation inspired on https://hg.mozilla.org/mozilla-central/file/722fdbff1efc/security/nss/lib/freebl/win_rand.c#l146
// Argument why this is safe to use: https://bugzilla.mozilla.org/show_bug.cgi?id=504270
extern "C" {
DECLSPEC_IMPORT BOOLEAN WINAPI SystemFunction036(PVOID RandomBuffer, ULONG RandomBufferLength);
}
#endif

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
#  include <private/qjni_p.h>
#endif

// This file is too low-level for regular Q_ASSERT (the logging framework may
// recurse back), so use regular assert()
#undef NDEBUG
#undef Q_ASSERT_X
#undef Q_ASSERT
#define Q_ASSERT(cond) assert(cond)
#define Q_ASSERT_X(cond, x, msg) assert(cond && msg)
#if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#  define NDEBUG    1
#endif
#include <assert.h>

QT_BEGIN_NAMESPACE

enum {
    // may be "overridden" by a member enum
    FillBufferNoexcept = true
};

struct QRandomGenerator::SystemGenerator
{
#if QT_CONFIG(getentropy)
    static qsizetype fillBuffer(void *buffer, qsizetype count) noexcept
    {
        // getentropy can read at most 256 bytes, so break the reading
        qsizetype read = 0;
        while (count - read > 256) {
            // getentropy can't fail under normal circumstances
            int ret = getentropy(reinterpret_cast<uchar *>(buffer) + read, 256);
            Q_ASSERT(ret == 0);
            Q_UNUSED(ret);
            read += 256;
        }

        int ret = getentropy(reinterpret_cast<uchar *>(buffer) + read, count - read);
        Q_ASSERT(ret == 0);
        Q_UNUSED(ret);
        return count;
    }

#elif defined(Q_OS_UNIX)
    enum { FillBufferNoexcept = false };

    QBasicAtomicInt fdp1;   // "file descriptor plus 1"
    int openDevice()
    {
        int fd = fdp1.loadAcquire() - 1;
        if (fd != -1)
            return fd;

        fd = qt_safe_open("/dev/urandom", O_RDONLY);
        if (fd == -1)
            fd = qt_safe_open("/dev/random", O_RDONLY | O_NONBLOCK);
        if (fd == -1) {
            // failed on both, set to -2 so we won't try again
            fd = -2;
        }

        int opened_fdp1;
        if (fdp1.testAndSetOrdered(0, fd + 1, opened_fdp1))
            return fd;

        // failed, another thread has opened the file descriptor
        if (fd >= 0)
            qt_safe_close(fd);
        return opened_fdp1 - 1;
    }

#ifdef Q_CC_GNU
     // If it's not GCC or GCC-like, then we'll leak the file descriptor
    __attribute__((destructor))
#endif
    static void closeDevice()
    {
        int fd = self().fdp1.loadRelaxed() - 1;
        if (fd >= 0)
            qt_safe_close(fd);
    }

    Q_DECL_CONSTEXPR SystemGenerator() : fdp1 Q_BASIC_ATOMIC_INITIALIZER(0) {}

    qsizetype fillBuffer(void *buffer, qsizetype count)
    {
        int fd = openDevice();
        if (Q_UNLIKELY(fd < 0))
            return 0;

        qint64 n = qt_safe_read(fd, buffer, count);
        return qMax<qsizetype>(n, 0);        // ignore any errors
    }

#elif defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    qsizetype fillBuffer(void *buffer, qsizetype count) noexcept
    {
        auto RtlGenRandom = SystemFunction036;
        return RtlGenRandom(buffer, ULONG(count)) ? count: 0;
    }
#elif defined(Q_OS_WINRT)
    qsizetype fillBuffer(void *, qsizetype) noexcept
    {
        // always use the fallback
        return 0;
    }
#endif // Q_OS_WINRT

    static SystemGenerator &self();
    typedef quint32 result_type;
    void generate(quint32 *begin, quint32 *end) noexcept(FillBufferNoexcept);

    // For std::mersenne_twister_engine implementations that use something
    // other than quint32 (unsigned int) to fill their buffers.
    template <typename T> void generate(T *begin, T *end)
    {
        Q_STATIC_ASSERT(sizeof(T) >= sizeof(quint32));
        if (sizeof(T) == sizeof(quint32)) {
            // Microsoft Visual Studio uses unsigned long, but that's still 32-bit
            generate(reinterpret_cast<quint32 *>(begin), reinterpret_cast<quint32 *>(end));
        } else {
            // Slow path. Fix your C++ library.
            std::generate(begin, end, [this]() {
                quint32 datum;
                generate(&datum, &datum + 1);
                return datum;
            });
        }
    }
};

#if defined(Q_OS_WIN)
static void fallback_update_seed(unsigned) {}
static void fallback_fill(quint32 *ptr, qsizetype left) noexcept
{
    // on Windows, rand_s is a high-quality random number generator
    // and it requires no seeding
    std::generate(ptr, ptr + left, []() {
        unsigned value;
        rand_s(&value);
        return value;
    });
}
#elif QT_CONFIG(getentropy)
static void fallback_update_seed(unsigned) {}
static void fallback_fill(quint32 *, qsizetype) noexcept
{
    // no fallback necessary, getentropy cannot fail under normal circumstances
    Q_UNREACHABLE();
}
#elif defined(Q_OS_BSD4) && !defined(__GLIBC__)
static void fallback_update_seed(unsigned) {}
static void fallback_fill(quint32 *ptr, qsizetype left) noexcept
{
    // BSDs have arc4random(4) and these work even in chroot(2)
    arc4random_buf(ptr, left * sizeof(*ptr));
}
#else
static QBasicAtomicInteger<unsigned> seed = Q_BASIC_ATOMIC_INITIALIZER(0U);
static void fallback_update_seed(unsigned value)
{
    // Update the seed to be used for the fallback mechansim, if we need to.
    // We can't use QtPrivate::QHashCombine here because that is not an atomic
    // operation. A simple XOR will have to do then.
    seed.fetchAndXorRelaxed(value);
}

Q_NEVER_INLINE
#ifdef Q_CC_GNU
__attribute__((cold))   // this function is pretty big, so optimize for size
#endif
static void fallback_fill(quint32 *ptr, qsizetype left) noexcept
{
    quint32 scratch[12];    // see element count below
    quint32 *end = scratch;

    auto foldPointer = [](quintptr v) {
        if (sizeof(quintptr) == sizeof(quint32)) {
            // For 32-bit systems, we simply return the pointer.
            return quint32(v);
        } else {
            // For 64-bit systems, we try to return the variable part of the
            // pointer. On current x86-64 and AArch64, the top 17 bits are
            // architecturally required to be the same, but in reality the top
            // 24 bits on Linux are likely to be the same for all processes.
            return quint32(v >> (32 - 24));
        }
    };

    Q_ASSERT(left);

    *end++ = foldPointer(quintptr(&seed));          // 1: variable in this library/executable's .data
    *end++ = foldPointer(quintptr(&scratch));       // 2: variable in the stack
    *end++ = foldPointer(quintptr(&errno));         // 3: veriable either in libc or thread-specific
    *end++ = foldPointer(quintptr(reinterpret_cast<void*>(strerror)));   // 4: function in libc (and unlikely to be a macro)

#ifndef QT_BOOTSTRAPPED
    quint64 nsecs = QDeadlineTimer::current(Qt::PreciseTimer).deadline();
    *end++ = quint32(nsecs);    // 5
#endif

    if (quint32 v = seed.loadRelaxed())
        *end++ = v; // 6

#if QT_CONFIG(getauxval)
    // works on Linux -- all modern libc have getauxval
#  ifdef AT_RANDOM
    // ELF's auxv AT_RANDOM has 16 random bytes
    // (other ELF-based systems don't seem to have AT_RANDOM)
    ulong auxvSeed = getauxval(AT_RANDOM);
    if (auxvSeed) {
        memcpy(end, reinterpret_cast<void *>(auxvSeed), 16);
        end += 4;   // 7 to 10
    }
#  endif

    // Both AT_BASE and AT_SYSINFO_EHDR have some randomness in them due to the
    // system's ASLR, even if many bits are the same. They also have randomness
    // between them.
#  ifdef AT_BASE
    // present at least on the BSDs too, indicates the address of the loader
    ulong base = getauxval(AT_BASE);
    if (base)
        *end++ = foldPointer(base); // 11
#  endif
#  ifdef AT_SYSINFO_EHDR
    // seems to be Linux-only, indicates the global page of the sysinfo
    ulong sysinfo_ehdr = getauxval(AT_SYSINFO_EHDR);
    if (sysinfo_ehdr)
        *end++ = foldPointer(sysinfo_ehdr); // 12
#  endif
#endif

    Q_ASSERT(end <= std::end(scratch));

    // this is highly inefficient, we should save the generator across calls...
    std::seed_seq sseq(scratch, end);
    std::mt19937 generator(sseq);
    std::generate(ptr, ptr + left, generator);

    fallback_update_seed(*ptr);
}
#endif

Q_NEVER_INLINE void QRandomGenerator::SystemGenerator::generate(quint32 *begin, quint32 *end)
    noexcept(FillBufferNoexcept)
{
    quint32 *buffer = begin;
    qsizetype count = end - begin;

    if (Q_UNLIKELY(uint(qt_randomdevice_control.loadAcquire()) & SetRandomData)) {
        uint value = uint(qt_randomdevice_control.loadAcquire()) & RandomDataMask;
        std::fill_n(buffer, count, value);
        return;
    }

    qsizetype filled = 0;
    if (qHasHwrng() && (uint(qt_randomdevice_control.loadAcquire()) & SkipHWRNG) == 0)
        filled += qRandomCpu(buffer, count);

    if (filled != count && (uint(qt_randomdevice_control.loadAcquire()) & SkipSystemRNG) == 0) {
        qsizetype bytesFilled =
                fillBuffer(buffer + filled, (count - filled) * qsizetype(sizeof(*buffer)));
        filled += bytesFilled / qsizetype(sizeof(*buffer));
    }
    if (filled)
        fallback_update_seed(*buffer);

    if (Q_UNLIKELY(filled != count)) {
        // failed to fill the entire buffer, try the faillback mechanism
        fallback_fill(buffer + filled, count - filled);
    }
}

struct QRandomGenerator::SystemAndGlobalGenerators
{
    // Construction notes:
    // 1) The global PRNG state is in a different cacheline compared to the
    //    mutex that protects it. This avoids any false cacheline sharing of
    //    the state in case another thread tries to lock the mutex. It's not
    //    a common scenario, but since sizeof(QRandomGenerator) >= 2560, the
    //    overhead is actually acceptable.
    // 2) We use both Q_DECL_ALIGN and std::aligned_storage<..., 64> because
    //    some implementations of std::aligned_storage can't align to more
    //    than a primitive type's alignment.
    // 3) We don't store the entire system QRandomGenerator, only the space
    //    used by the QRandomGenerator::type member. This is fine because we
    //    (ab)use the common initial sequence exclusion to aliasing rules.
    QBasicMutex globalPRNGMutex;
    struct ShortenedSystem { uint type; } system_;
    SystemGenerator sys;
    Q_DECL_ALIGN(64) std::aligned_storage<sizeof(QRandomGenerator64), 64>::type global_;

#ifdef Q_COMPILER_CONSTEXPR
    constexpr SystemAndGlobalGenerators()
        : globalPRNGMutex{}, system_{0}, sys{}, global_{}
    {}
#endif

    void confirmLiteral()
    {
#if defined(Q_COMPILER_CONSTEXPR) && !defined(Q_CC_MSVC) && !defined(Q_OS_INTEGRITY)
        // Currently fails to compile with MSVC 2017, saying QBasicMutex is not
        // a literal type. Disassembly with MSVC 2013 and 2015 shows it is
        // actually a literal; MSVC 2017 has a bug relating to this, so we're
        // withhold judgement for now.  Integrity's compiler is unable to
        // guarantee g's alignment for some reason.

        constexpr SystemAndGlobalGenerators g = {};
        Q_UNUSED(g);
        Q_STATIC_ASSERT(std::is_literal_type<SystemAndGlobalGenerators>::value);
#endif
    }

    static SystemAndGlobalGenerators *self()
    {
        static SystemAndGlobalGenerators g;
        Q_STATIC_ASSERT(sizeof(g) > sizeof(QRandomGenerator64));
        return &g;
    }

    static QRandomGenerator64 *system()
    {
        // Though we never call the constructor, the system QRandomGenerator is
        // properly initialized by the zero initialization performed in self().
        // Though QRandomGenerator is has non-vacuous initialization, we
        // consider it initialized because of the common initial sequence.
        return reinterpret_cast<QRandomGenerator64 *>(&self()->system_);
    }

    static QRandomGenerator64 *globalNoInit()
    {
        // This function returns the pointer to the global QRandomGenerator,
        // but does not initialize it. Only call it directly if you meant to do
        // a pointer comparison.
        return reinterpret_cast<QRandomGenerator64 *>(&self()->global_);
    }

    static void securelySeed(QRandomGenerator *rng)
    {
        // force reconstruction, just to be pedantic
        new (rng) QRandomGenerator{System{}};

        rng->type = MersenneTwister;
        new (&rng->storage.engine()) RandomEngine(self()->sys);
    }

    struct PRNGLocker {
        const bool locked;
        PRNGLocker(const QRandomGenerator *that)
            : locked(that == globalNoInit())
        {
            if (locked)
                self()->globalPRNGMutex.lock();
        }
        ~PRNGLocker()
        {
            if (locked)
                self()->globalPRNGMutex.unlock();
        }
    };
};

inline QRandomGenerator::SystemGenerator &QRandomGenerator::SystemGenerator::self()
{
    return SystemAndGlobalGenerators::self()->sys;
}

/*!
    \class QRandomGenerator
    \inmodule QtCore
    \reentrant
    \since 5.10

    \brief The QRandomGenerator class allows one to obtain random values from a
    high-quality Random Number Generator.

    QRandomGenerator may be used to generate random values from a high-quality
    random number generator. Like the C++ random engines, QRandomGenerator can
    be seeded with user-provided values through the constructor.
    When seeded, the sequence of numbers generated by this
    class is deterministic. That is to say, given the same seed data,
    QRandomGenerator will generate the same sequence of numbers. But given
    different seeds, the results should be considerably different.

    QRandomGenerator::securelySeeded() can be used to create a QRandomGenerator
    that is securely seeded with QRandomGenerator::system(), meaning that the
    sequence of numbers it generates cannot be easily predicted. Additionally,
    QRandomGenerator::global() returns a global instance of QRandomGenerator
    that Qt will ensure to be securely seeded. This object is thread-safe, may
    be shared for most uses, and is always seeded from
    QRandomGenerator::system()

    QRandomGenerator::system() may be used to access the system's
    cryptographically-safe random generator. On Unix systems, it's equivalent
    to reading from \c {/dev/urandom} or the \c {getrandom()} or \c
    {getentropy()} system calls.

    The class can generate 32-bit or 64-bit quantities, or fill an array of
    those. The most common way of generating new values is to call the generate(),
    generate64() or fillRange() functions. One would use it as:

    \snippet code/src_corelib_global_qrandom.cpp 0

    Additionally, it provides a floating-point function generateDouble() that
    returns a number in the range [0, 1) (that is, inclusive of zero and
    exclusive of 1). There's also a set of convenience functions that
    facilitate obtaining a random number in a bounded, integral range.

    \section1 Seeding and determinism

    QRandomGenerator may be seeded with specific seed data. When that is done,
    the numbers generated by the object will always be the same, as in the
    following example:

    \snippet code/src_corelib_global_qrandom.cpp 1

    The seed data takes the form of one or more 32-bit words. The ideal seed
    size is approximately equal to the size of the QRandomGenerator class
    itself. Due to mixing of the seed data, QRandomGenerator cannot guarantee
    that distinct seeds will produce different sequences.

    QRandomGenerator::global(), like all generators created by
    QRandomGenerator::securelySeeded(), is always seeded from
    QRandomGenerator::system(), so it's not possible to make it produce
    identical sequences.

    \section1 Bulk data

    When operating in deterministic mode, QRandomGenerator may be used for bulk
    data generation. In fact, applications that do not need
    cryptographically-secure or true random data are advised to use a regular
    QRandomGenerator instead of QRandomGenerator::system() for their random
    data needs.

    For ease of use, QRandomGenerator provides a global object that can
    be easily used, as in the following example:

    \snippet code/src_corelib_global_qrandom.cpp 2

    \section1 System-wide random number generator

    QRandomGenerator::system() may be used to access the system-wide random
    number generator, which is cryptographically-safe on all systems that Qt
    runs on. This function will use hardware facilities to generate random
    numbers where available. On such systems, those facilities are true Random
    Number Generators. However, if they are true RNGs, those facilities have
    finite entropy sources and thus may fail to produce any results if their
    entropy pool is exhausted.

    If that happens, first the operating system then QRandomGenerator will fall
    back to Pseudo Random Number Generators of decreasing qualities (Qt's
    fallback generator being the simplest). Whether those generators are still
    of cryptographic quality is implementation-defined. Therefore,
    QRandomGenerator::system() should not be used for high-frequency random
    number generation, lest the entropy pool become empty. As a rule of thumb,
    this class should not be called upon to generate more than a kilobyte per
    second of random data (note: this may vary from system to system).

    If an application needs true RNG data in bulk, it should use the operating
    system facilities (such as \c{/dev/random} on Linux) directly and wait for
    entropy to become available. If the application requires PRNG engines of
    cryptographic quality but not of true randomness,
    QRandomGenerator::system() may still be used (see section below).

    If neither a true RNG nor a cryptographically secure PRNG are required,
    applications should instead use PRNG engines like QRandomGenerator's
    deterministic mode and those from the C++ Standard Library.
    QRandomGenerator::system() can be used to seed those.

    \section2 Fallback quality

    QRandomGenerator::system() uses the operating system facilities to obtain
    random numbers, which attempt to collect real entropy from the surrounding
    environment to produce true random numbers. However, it's possible that the
    entropy pool becomes exhausted, in which case the operating system will
    fall back to a pseudo-random engine for a time. Under no circumstances will
    QRandomGenerator::system() block, waiting for more entropy to be collected.

    The following operating systems guarantee that the results from their
    random-generation API will be of at least cryptographically-safe quality,
    even if the entropy pool is exhausted: Apple OSes (Darwin), BSDs, Linux,
    Windows. Barring a system installation problem (such as \c{/dev/urandom}
    not being readable by the current process), QRandomGenerator::system() will
    therefore have the same guarantees.

    On other operating systems, QRandomGenerator will fall back to a PRNG of
    good numeric distribution, but it cannot guarantee proper seeding in all
    cases. Please consult the OS documentation for more information.

    Applications that require QRandomGenerator not to fall back to
    non-cryptographic quality generators are advised to check their operating
    system documentation or restrict their deployment to one of the above.

    \section1 Reentrancy and thread-safety

    QRandomGenerator is reentrant, meaning that multiple threads can operate on
    this class at the same time, so long as they operate on different objects.
    If multiple threads need to share one PRNG sequence, external locking by a
    mutex is required.

    The exceptions are the objects returned by QRandomGenerator::global() and
    QRandomGenerator::system(): those objects are thread-safe and may be used
    by any thread without external locking. Note that thread-safety does not
    extend to copying those objects: they should always be used by reference.

    \section1 Standard C++ Library compatibility

    QRandomGenerator is modeled after the requirements for random number
    engines in the C++ Standard Library and may be used in almost all contexts
    that the Standard Library engines can. Exceptions to the requirements are
    the following:

    \list
      \li QRandomGenerator does not support seeding from another seed
          sequence-like class besides std::seed_seq itself;
      \li QRandomGenerator is not comparable (but is copyable) or
          streamable to \c{std::ostream} or from \c{std::istream}.
    \endlist

    QRandomGenerator is also compatible with the uniform distribution classes
    \c{std::uniform_int_distribution} and \c{std:uniform_real_distribution}, as
    well as the free function \c{std::generate_canonical}. For example, the
    following code may be used to generate a floating-point number in the range
    [1, 2.5):

    \snippet code/src_corelib_global_qrandom.cpp 3

    \sa QRandomGenerator64, qrand()
 */

/*!
    \enum QRandomGenerator::System
    \internal
*/

/*!
    \fn QRandomGenerator::QRandomGenerator(quint32 seedValue)

    Initializes this QRandomGenerator object with the value \a seedValue as
    the seed. Two objects constructed or reseeded with the same seed value will
    produce the same number sequence.

    \sa seed(), securelySeeded()
 */

/*!
    \fn template <qsizetype N> QRandomGenerator::QRandomGenerator(const quint32 (&seedBuffer)[N])
    \overload

    Initializes this QRandomGenerator object with the values found in the
    array \a seedBuffer as the seed. Two objects constructed or reseeded with
    the same seed value will produce the same number sequence.

    \sa seed(), securelySeeded()
 */

/*!
    \fn QRandomGenerator::QRandomGenerator(const quint32 *seedBuffer, qsizetype len)
    \overload

    Initializes this QRandomGenerator object with \a len values found in
    the array \a seedBuffer as the seed. Two objects constructed or reseeded
    with the same seed value will produce the same number sequence.

    This constructor is equivalent to:
    \snippet code/src_corelib_global_qrandom.cpp 4

    \sa seed(), securelySeeded()
 */

/*!
    \fn QRandomGenerator::QRandomGenerator(const quint32 *begin, const quint32 *end)
    \overload

    Initializes this QRandomGenerator object with the values found in the range
    from \a begin to \a end as the seed. Two objects constructed or reseeded
    with the same seed value will produce the same number sequence.

    This constructor is equivalent to:
    \snippet code/src_corelib_global_qrandom.cpp 5

    \sa seed(), securelySeeded()
 */

/*!
    \fn QRandomGenerator::QRandomGenerator(std::seed_seq &sseq)
    \overload

    Initializes this QRandomGenerator object with the seed sequence \a
    sseq as the seed. Two objects constructed or reseeded with the same seed
    value will produce the same number sequence.

    \sa seed(), securelySeeded()
 */

/*!
   \fn QRandomGenerator::QRandomGenerator(const QRandomGenerator &other)

   Creates a copy of the generator state in the \a other object. If \a other is
   QRandomGenerator::system() or a copy of that, this object will also read
   from the operating system random-generating facilities. In that case, the
   sequences generated by the two objects will be different.

   In all other cases, the new QRandomGenerator object will start at the same
   position in the deterministic sequence as the \a other object was. Both
   objects will generate the same sequence from this point on.

   For that reason, it is not adviseable to create a copy of
   QRandomGenerator::global(). If one needs an exclusive deterministic
   generator, consider instead using securelySeeded() to obtain a new object
   that shares no relationship with the QRandomGenerator::global().
 */

/*!
    \fn bool operator==(const QRandomGenerator &rng1, const QRandomGenerator &rng2)
    \relates QRandomGenerator

    Returns true if the two the two engines \a rng1 and \a rng2 are at the same
    state or if they are both reading from the operating system facilities,
    false otherwise.
*/

/*!
    \fn bool operator!=(const QRandomGenerator &rng1, const QRandomGenerator &rng2)
    \relates QRandomGenerator

    Returns true if the two the two engines \a rng1 and \a rng2 are at
    different states or if one of them is reading from the operating system
    facilities and the other is not, false otherwise.
*/

/*!
    \typedef QRandomGenerator::result_type

    A typedef to the type that operator() returns. That is, quint32.

    \sa operator()
 */

/*!
    \fn result_type QRandomGenerator::operator()()

    Generates a 32-bit random quantity and returns it.

    \sa generate(), generate64()
 */

/*!
    \fn quint32 QRandomGenerator::generate()

    Generates a 32-bit random quantity and returns it.

    \sa {QRandomGenerator::operator()}{operator()()}, generate64()
 */

/*!
    \fn quint64 QRandomGenerator::generate64()

    Generates a 64-bit random quantity and returns it.

    \sa {QRandomGenerator::operator()}{operator()()}, generate()
 */

/*!
    \fn result_type QRandomGenerator::min()

    Returns the minimum value that QRandomGenerator may ever generate. That is, 0.

    \sa max(), QRandomGenerator64::min()
 */

/*!
    \fn result_type QRandomGenerator::max()

    Returns the maximum value that QRandomGenerator may ever generate. That is,
    \c {std::numeric_limits<result_type>::max()}.

    \sa min(), QRandomGenerator64::max()
 */

/*!
    \fn void QRandomGenerator::seed(quint32 seed)

    Reseeds this object using the value \a seed as the seed.
 */

/*!
    \fn void QRandomGenerator::seed(std::seed_seq &seed)
    \overload

    Reseeds this object using the seed sequence \a seed as the seed.
 */

/*!
    \fn void QRandomGenerator::discard(unsigned long long z)

    Discards the next \a z entries from the sequence. This method is equivalent
    to calling generate() \a z times and discarding the result, as in:

    \snippet code/src_corelib_global_qrandom.cpp 6
*/

/*!
    \fn template <typename ForwardIterator> void QRandomGenerator::generate(ForwardIterator begin, ForwardIterator end)

    Generates 32-bit quantities and stores them in the range between \a begin
    and \a end. This function is equivalent to (and is implemented as):

    \snippet code/src_corelib_global_qrandom.cpp 7

    This function complies with the requirements for the function
    \l{http://en.cppreference.com/w/cpp/numeric/random/seed_seq/generate}{\c std::seed_seq::generate},
    which requires unsigned 32-bit integer values.

    Note that if the [begin, end) range refers to an area that can store more
    than 32 bits per element, the elements will still be initialized with only
    32 bits of data. Any other bits will be zero. To fill the range with 64 bit
    quantities, one can write:

    \snippet code/src_corelib_global_qrandom.cpp 8

    If the range refers to contiguous memory (such as an array or the data from
    a QVector), the fillRange() function may be used too.

    \sa fillRange()
 */

/*!
    \fn void QRandomGenerator::generate(quint32 *begin, quint32 *end)
    \overload
    \internal

    Same as the other overload, but more efficiently fills \a begin to \a end.
 */

/*!
    \fn template <typename UInt> void QRandomGenerator::fillRange(UInt *buffer, qsizetype count)

    Generates \a count 32- or 64-bit quantities (depending on the type \c UInt)
    and stores them in the buffer pointed by \a buffer. This is the most
    efficient way to obtain more than one quantity at a time, as it reduces the
    number of calls into the Random Number Generator source.

    For example, to fill a vector of 16 entries with random values, one may
    write:

    \snippet code/src_corelib_global_qrandom.cpp 9

    \sa generate()
 */

/*!
    \fn template <typename UInt, size_t N> void QRandomGenerator::fillRange(UInt (&buffer)[N])

    Generates \c N 32- or 64-bit quantities (depending on the type \c UInt) and
    stores them in the \a buffer array. This is the most efficient way to
    obtain more than one quantity at a time, as it reduces the number of calls
    into the Random Number Generator source.

    For example, to fill generate two 32-bit quantities, one may write:

    \snippet code/src_corelib_global_qrandom.cpp 10

    It would have also been possible to make one call to generate64() and then split
    the two halves of the 64-bit value.

    \sa generate()
 */

/*!
    \fn qreal QRandomGenerator::generateDouble()

    Generates one random qreal in the canonical range [0, 1) (that is,
    inclusive of zero and exclusive of 1).

    This function is equivalent to:
    \snippet code/src_corelib_global_qrandom.cpp 11

    The same may also be obtained by using
    \l{http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution}{\c std::uniform_real_distribution}
    with parameters 0 and 1.

    \sa generate(), generate64(), bounded()
 */

/*!
    \fn double QRandomGenerator::bounded(double highest)

    Generates one random double in the range between 0 (inclusive) and \a
    highest (exclusive). This function is equivalent to and is implemented as:

    \snippet code/src_corelib_global_qrandom.cpp 12

    If the \a highest parameter is negative, the result will be negative too;
    if it is infinite or NaN, the result will be infinite or NaN too (that is,
    not random).

    \sa generateDouble(), bounded()
 */

/*!
    \fn quint32 QRandomGenerator::bounded(quint32 highest)
    \overload

    Generates one random 32-bit quantity in the range between 0 (inclusive) and
    \a highest (exclusive). The same result may also be obtained by using
    \l{http://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution}{\c std::uniform_int_distribution}
    with parameters 0 and \c{highest - 1}. That class can also be used to obtain
    quantities larger than 32 bits.

    For example, to obtain a value between 0 and 255 (inclusive), one would write:

    \snippet code/src_corelib_global_qrandom.cpp 13

    Naturally, the same could also be obtained by masking the result of generate()
    to only the lower 8 bits. Either solution is as efficient.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of quint32. Instead, use generate().

    \sa generate(), generate64(), generateDouble()
 */

/*!
    \fn int QRandomGenerator::bounded(int highest)
    \overload

    Generates one random 32-bit quantity in the range between 0 (inclusive) and
    \a highest (exclusive). \a highest must be positive.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of int. Instead, use generate() and cast to int.

    \sa generate(), generate64(), generateDouble()
 */

/*!
    \fn quint32 QRandomGenerator::bounded(quint32 lowest, quint32 highest)
    \overload

    Generates one random 32-bit quantity in the range between \a lowest
    (inclusive) and \a highest (exclusive). The \a highest parameter must be
    greater than \a lowest.

    The same result may also be obtained by using
    \l{http://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution}{\c std::uniform_int_distribution}
    with parameters \a lowest and \c{\a highest - 1}. That class can also be used to
    obtain quantities larger than 32 bits.

    For example, to obtain a value between 1000 (incl.) and 2000 (excl.), one
    would write:

    \snippet code/src_corelib_global_qrandom.cpp 14

    Note that this function cannot be used to obtain values in the full 32-bit
    range of quint32. Instead, use generate().

    \sa generate(), generate64(), generateDouble()
 */

/*!
    \fn int QRandomGenerator::bounded(int lowest, int highest)
    \overload

    Generates one random 32-bit quantity in the range between \a lowest
    (inclusive) and \a highest (exclusive), both of which may be negative, but
    \a highest must be greater than \a lowest.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of int. Instead, use generate() and cast to int.

    \sa generate(), generate64(), generateDouble()
 */

/*!
    \fn QRandomGenerator *QRandomGenerator::system()
    \threadsafe

    Returns a pointer to a shared QRandomGenerator that always uses the
    facilities provided by the operating system to generate random numbers. The
    system facilities are considered to be cryptographically safe on at least
    the following operating systems: Apple OSes (Darwin), BSDs, Linux, Windows.
    That may also be the case on other operating systems.

    They are also possibly backed by a true hardware random number generator.
    For that reason, the QRandomGenerator returned by this function should not
    be used for bulk data generation. Instead, use it to seed QRandomGenerator
    or a random engine from the <random> header.

    The object returned by this function is thread-safe and may be used in any
    thread without locks. It may also be copied and the resulting
    QRandomGenerator will also access the operating system facilities, but they
    will not generate the same sequence.

    \sa securelySeeded(), global()
*/

/*!
    \fn QRandomGenerator *QRandomGenerator::global()
    \threadsafe

    Returns a pointer to a shared QRandomGenerator that was seeded using
    securelySeeded(). This function should be used to create random data
    without the expensive creation of a securely-seeded QRandomGenerator
    for a specific use or storing the rather large QRandomGenerator object.

    For example, the following creates a random RGB color:

    \snippet code/src_corelib_global_qrandom.cpp 15

    Accesses to this object are thread-safe and it may therefore be used in any
    thread without locks. The object may also be copied and the sequence
    produced by the copy will be the same as the shared object will produce.
    Note, however, that if there are other threads accessing the global object,
    those threads may obtain samples at unpredictable intervals.

    \sa securelySeeded(), system()
*/

/*!
    \fn QRandomGenerator QRandomGenerator::securelySeeded()

    Returns a new QRandomGenerator object that was securely seeded with
    QRandomGenerator::system(). This function will obtain the ideal seed size
    for the algorithm that QRandomGenerator uses and is therefore the
    recommended way for creating a new QRandomGenerator object that will be
    kept for some time.

    Given the amount of data required to securely seed the deterministic
    engine, this function is somewhat expensive and should not be used for
    short-term uses of QRandomGenerator (using it to generate fewer than 2600
    bytes of random data is effectively a waste of resources). If the use
    doesn't require that much data, consider using QRandomGenerator::global()
    and not storing a QRandomGenerator object instead.

    \sa global(), system()
 */

/*!
    \class QRandomGenerator64
    \inmodule QtCore
    \since 5.10

    \brief The QRandomGenerator64 class allows one to obtain 64-bit random values
    from a high-quality, seed-less Random Number Generator.

    QRandomGenerator64 is a simple adaptor class around QRandomGenerator, making the
    QRandomGenerator::generate64() function the default for operator()(), instead of the
    function that returns 32-bit quantities. This class is intended to be used
    in conjunction with Standard Library algorithms that need 64-bit quantities
    instead of 32-bit ones.

    In all other aspects, the class is the same. Please refer to
    QRandomGenerator's documentation for more information.

    \sa QRandomGenerator
*/

/*!
    \typedef QRandomGenerator64::result_type

    A typedef to the type that operator() returns. That is, quint64.

    \sa operator()
 */

/*!
    \fn quint64 QRandomGenerator64::generate()

    Generates one 64-bit random value and returns it.

    Note about casting to a signed integer: all bits returned by this function
    are random, so there's a 50% chance that the most significant bit will be
    set. If you wish to cast the returned value to qint64 and keep it positive,
    you should mask the sign bit off:

    \snippet code/src_corelib_global_qrandom.cpp 16

    \sa QRandomGenerator, QRandomGenerator::generate64()
 */

/*!
    \fn result_type QRandomGenerator64::operator()()

    Generates a 64-bit random quantity and returns it.

    \sa QRandomGenerator::generate(), QRandomGenerator::generate64()
 */

Q_DECL_CONSTEXPR QRandomGenerator::Storage::Storage()
    : dummy(0)
{
    // nothing
}

inline QRandomGenerator64::QRandomGenerator64(System s)
    : QRandomGenerator(s)
{
}

QRandomGenerator64 *QRandomGenerator64::system()
{
    auto self = SystemAndGlobalGenerators::system();
    Q_ASSERT(self->type == SystemRNG);
    return self;
}

QRandomGenerator64 *QRandomGenerator64::global()
{
    auto self = SystemAndGlobalGenerators::globalNoInit();

    // Yes, this is a double-checked lock.
    // We can return even if the type is not completely initialized yet:
    // any thread trying to actually use the contents of the random engine
    // will necessarily wait on the lock.
    if (Q_LIKELY(self->type != SystemRNG))
        return self;

    SystemAndGlobalGenerators::PRNGLocker locker(self);
    if (self->type == SystemRNG)
        SystemAndGlobalGenerators::securelySeed(self);

    return self;
}

QRandomGenerator64 QRandomGenerator64::securelySeeded()
{
    QRandomGenerator64 result(System{});
    SystemAndGlobalGenerators::securelySeed(&result);
    return result;
}

/*!
    \internal
*/
inline QRandomGenerator::QRandomGenerator(System)
    : type(SystemRNG)
{
    // don't touch storage
}

QRandomGenerator::QRandomGenerator(const QRandomGenerator &other)
    : type(other.type)
{
    Q_ASSERT(this != system());
    Q_ASSERT(this != SystemAndGlobalGenerators::globalNoInit());

    if (type != SystemRNG) {
        SystemAndGlobalGenerators::PRNGLocker lock(&other);
        storage.engine() = other.storage.engine();
    }
}

QRandomGenerator &QRandomGenerator::operator=(const QRandomGenerator &other)
{
    if (Q_UNLIKELY(this == system()) || Q_UNLIKELY(this == SystemAndGlobalGenerators::globalNoInit()))
        qFatal("Attempted to overwrite a QRandomGenerator to system() or global().");

    if ((type = other.type) != SystemRNG) {
        SystemAndGlobalGenerators::PRNGLocker lock(&other);
        storage.engine() = other.storage.engine();
    }
    return *this;
}

QRandomGenerator::QRandomGenerator(std::seed_seq &sseq) noexcept
    : type(MersenneTwister)
{
    Q_ASSERT(this != system());
    Q_ASSERT(this != SystemAndGlobalGenerators::globalNoInit());

    new (&storage.engine()) RandomEngine(sseq);
}

QRandomGenerator::QRandomGenerator(const quint32 *begin, const quint32 *end)
    : type(MersenneTwister)
{
    Q_ASSERT(this != system());
    Q_ASSERT(this != SystemAndGlobalGenerators::globalNoInit());

    std::seed_seq s(begin, end);
    new (&storage.engine()) RandomEngine(s);
}

void QRandomGenerator::discard(unsigned long long z)
{
    if (Q_UNLIKELY(type == SystemRNG))
        return;

    SystemAndGlobalGenerators::PRNGLocker lock(this);
    storage.engine().discard(z);
}

bool operator==(const QRandomGenerator &rng1, const QRandomGenerator &rng2)
{
    if (rng1.type != rng2.type)
        return false;
    if (rng1.type == SystemRNG)
        return true;

    // Lock global() if either is it (otherwise this locking is a no-op)
    using PRNGLocker = QRandomGenerator::SystemAndGlobalGenerators::PRNGLocker;
    PRNGLocker locker(&rng1 == QRandomGenerator::global() ? &rng1 : &rng2);
    return rng1.storage.engine() == rng2.storage.engine();
}

/*!
    \internal

    Fills the range pointed by \a buffer and \a bufferEnd with 32-bit random
    values. The buffer must be correctly aligned.
 */
void QRandomGenerator::_fillRange(void *buffer, void *bufferEnd)
{
    // Verify that the pointers are properly aligned for 32-bit
    Q_ASSERT(quintptr(buffer) % sizeof(quint32) == 0);
    Q_ASSERT(quintptr(bufferEnd) % sizeof(quint32) == 0);
    quint32 *begin = static_cast<quint32 *>(buffer);
    quint32 *end = static_cast<quint32 *>(bufferEnd);

    if (type == SystemRNG || Q_UNLIKELY(uint(qt_randomdevice_control.loadAcquire()) & (UseSystemRNG|SetRandomData)))
        return SystemGenerator::self().generate(begin, end);

    SystemAndGlobalGenerators::PRNGLocker lock(this);
    std::generate(begin, end, [this]() { return storage.engine()(); });
}

namespace {
struct QRandEngine
{
    std::minstd_rand engine;
    QRandEngine() : engine(1) {}

    int generate()
    {
        std::minstd_rand::result_type v = engine();
        if (std::numeric_limits<int>::max() != RAND_MAX)
            v %= uint(RAND_MAX) + 1;

        return int(v);
    }

    void seed(std::minstd_rand::result_type q)
    {
        engine.seed(q);
    }
};
}

#if defined(Q_OS_WIN)
// On Windows srand() and rand() already use Thread-Local-Storage
// to store the seed between calls
static inline QRandEngine *randTLS()
{
    return nullptr;
}
#elif defined(Q_COMPILER_THREAD_LOCAL)
static inline QRandEngine *randTLS()
{
    thread_local QRandEngine r;
    return &r;
}
#else
Q_GLOBAL_STATIC(QThreadStorage<QRandEngine>, g_randTLS)
static inline QRandEngine *randTLS()
{
    auto tls = g_randTLS();
    if (!tls)
        return nullptr;
    return &tls->localData();

}
#endif

/*!
    \relates <QtGlobal>
    \deprecated
    \since 4.2

    Thread-safe version of the standard C++ \c srand() function.

    Sets the argument \a seed to be used to generate a new random number sequence of
    pseudo random integers to be returned by qrand().

    The sequence of random numbers generated is deterministic per thread. For example,
    if two threads call qsrand(1) and subsequently call qrand(), the threads will get
    the same random number sequence.

    \note This function is deprecated. In new applications, use
    QRandomGenerator instead.

    \sa qrand(), QRandomGenerator
*/
void qsrand(uint seed)
{
    auto prng = randTLS();
    if (prng)
        prng->seed(seed);
    else
        srand(seed);
}

/*!
    \relates <QtGlobal>
    \deprecated
    \since 4.2

    Thread-safe version of the standard C++ \c rand() function.

    Returns a value between 0 and \c RAND_MAX (defined in \c <cstdlib> and
    \c <stdlib.h>), the next number in the current sequence of pseudo-random
    integers.

    Use \c qsrand() to initialize the pseudo-random number generator with a
    seed value. Seeding must be performed at least once on each thread. If that
    step is skipped, then the sequence will be pre-seeded with a constant
    value.

    \note This function is deprecated. In new applications, use
    QRandomGenerator instead.

    \sa qsrand(), QRandomGenerator
*/
int qrand()
{
    auto prng = randTLS();
    if (prng)
        return prng->generate();
    else
        return rand();
}

QT_END_NAMESPACE
