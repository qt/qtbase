/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
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
#include <qthreadstorage.h>
#include <private/qsimd_p.h>

#include <random>

#include <errno.h>

#if QT_CONFIG(getentropy)
#  include <sys/random.h>
#elif !defined(Q_OS_BSD4) && !defined(Q_OS_WIN)
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

#if defined(Q_OS_ANDROID)
#  include <private/qjni_p.h>
#endif

// This file is too low-level for regular Q_ASSERT (the logging framework may
// recurse back), so use regular assert()
#undef NDEBUG
#undef Q_ASSERT_X
#undef Q_ASSERT
#define Q_ASSERT(cond) assert(cond)
#if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#  define NDEBUG    1
#endif
#include <assert.h>

QT_BEGIN_NAMESPACE

#if defined(Q_PROCESSOR_X86) && QT_COMPILER_SUPPORTS_HERE(RDRND)
static qssize_t qt_random_cpu(void *buffer, qssize_t count) Q_DECL_NOTHROW;

#  ifdef Q_PROCESSOR_X86_64
#    define _rdrandXX_step _rdrand64_step
#  else
#    define _rdrandXX_step _rdrand32_step
#  endif

static QT_FUNCTION_TARGET(RDRND) qssize_t qt_random_cpu(void *buffer, qssize_t count) Q_DECL_NOTHROW
{
    unsigned *ptr = reinterpret_cast<unsigned *>(buffer);
    unsigned *end = ptr + count;

    while (ptr + sizeof(qregisteruint)/sizeof(*ptr) <= end) {
        if (_rdrandXX_step(reinterpret_cast<qregisteruint *>(ptr)) == 0)
            goto out;
        ptr += sizeof(qregisteruint)/sizeof(*ptr);
    }

    if (sizeof(*ptr) != sizeof(qregisteruint) && ptr != end) {
        if (_rdrand32_step(ptr))
            goto out;
        ++ptr;
    }

out:
    return ptr - reinterpret_cast<unsigned *>(buffer);
}
#endif

namespace {
#if QT_CONFIG(getentropy)
class SystemRandom
{
public:
    enum { EfficientBufferFill = true };
    static qssize_t fillBuffer(void *buffer, qssize_t count) Q_DECL_NOTHROW
    {
        // getentropy can read at most 256 bytes, so break the reading
        qssize_t read = 0;
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
};

#elif defined(Q_OS_UNIX)
class SystemRandom
{
    static QBasicAtomicInt s_fdp1;  // "file descriptor plus 1"
    static int openDevice();
#ifdef Q_CC_GNU
     // If it's not GCC or GCC-like, then we'll leak the file descriptor
    __attribute__((destructor))
#endif
    static void closeDevice();
    SystemRandom() {}
public:
    enum { EfficientBufferFill = true };
    static qssize_t fillBuffer(void *buffer, qssize_t count);
};
QBasicAtomicInt SystemRandom::s_fdp1 = Q_BASIC_ATOMIC_INITIALIZER(0);

void SystemRandom::closeDevice()
{
    int fd = s_fdp1.loadAcquire() - 1;
    if (fd >= 0)
        qt_safe_close(fd);
}

int SystemRandom::openDevice()
{
    int fd = s_fdp1.loadAcquire() - 1;
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
    if (s_fdp1.testAndSetOrdered(0, fd + 1, opened_fdp1)) {
        if (fd >= 0) {
            static const SystemRandom closer;
            Q_UNUSED(closer);
        }
        return fd;
    }

    // failed, another thread has opened the file descriptor
    if (fd >= 0)
        qt_safe_close(fd);
    return opened_fdp1 - 1;
}

qssize_t SystemRandom::fillBuffer(void *buffer, qssize_t count)
{
    int fd = openDevice();
    if (Q_UNLIKELY(fd < 0))
        return 0;

    qint64 n = qt_safe_read(fd, buffer, count);
    return qMax<qssize_t>(n, 0);        // ignore any errors
}
#endif // Q_OS_UNIX

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
class SystemRandom
{
public:
    enum { EfficientBufferFill = true };
    static qssize_t fillBuffer(void *buffer, qssize_t count) Q_DECL_NOTHROW
    {
        auto RtlGenRandom = SystemFunction036;
        return RtlGenRandom(buffer, ULONG(count)) ? count: 0;
    }
};
#elif defined(Q_OS_WINRT)
class SystemRandom
{
public:
    enum { EfficientBufferFill = false };
    static qssize_t fillBuffer(void *, qssize_t) Q_DECL_NOTHROW
    {
        // always use the fallback
        return 0;
    }
};
#endif // Q_OS_WINRT
} // unnamed namespace

#if defined(Q_OS_WIN)
static void fallback_update_seed(unsigned) {}
static void fallback_fill(quint32 *ptr, qssize_t left) Q_DECL_NOTHROW
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
static void fallback_fill(quint32 *, qssize_t) Q_DECL_NOTHROW
{
    // no fallback necessary, getentropy cannot fail under normal circumstances
}
#elif defined(Q_OS_BSD4)
static void fallback_update_seed(unsigned) {}
static void fallback_fill(quint32 *ptr, qssize_t left) Q_DECL_NOTHROW
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
static void fallback_fill(quint32 *ptr, qssize_t left) Q_DECL_NOTHROW
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

    if (quint32 v = seed.load())
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

static qssize_t fill_cpu(quint32 *buffer, qssize_t count) Q_DECL_NOTHROW
{
#if defined(Q_PROCESSOR_X86) && QT_COMPILER_SUPPORTS_HERE(RDRND)
    if (qCpuHasFeature(RDRND) && (uint(qt_randomdevice_control) & SkipHWRNG) == 0)
        return qt_random_cpu(buffer, count);
#else
    Q_UNUSED(buffer);
    Q_UNUSED(count);
#endif
    return 0;
}

static void fill_internal(quint32 *buffer, qssize_t count)
    Q_DECL_NOEXCEPT_EXPR(noexcept(SystemRandom::fillBuffer(buffer, count)))
{
    if (Q_UNLIKELY(uint(qt_randomdevice_control) & SetRandomData)) {
        uint value = uint(qt_randomdevice_control) & RandomDataMask;
        std::fill_n(buffer, count, value);
        return;
    }

    qssize_t filled = fill_cpu(buffer, count);
    if (filled != count && (uint(qt_randomdevice_control) & SkipSystemRNG) == 0) {
        qssize_t bytesFilled =
                SystemRandom::fillBuffer(buffer + filled, (count - filled) * qssize_t(sizeof(*buffer)));
        filled += bytesFilled / qssize_t(sizeof(*buffer));
    }
    if (filled)
        fallback_update_seed(*buffer);

    if (Q_UNLIKELY(filled != count)) {
        // failed to fill the entire buffer, try the faillback mechanism
        fallback_fill(buffer + filled, count - filled);
    }
}

static Q_NEVER_INLINE void fill(void *buffer, void *bufferEnd)
    Q_DECL_NOEXCEPT_EXPR(noexcept(fill_internal(static_cast<quint32 *>(buffer), 1)))
{
    struct ThreadState {
        enum {
            DesiredBufferByteSize = 32,
            BufferCount = DesiredBufferByteSize / sizeof(quint32)
        };
        quint32 buffer[BufferCount];
        int idx = BufferCount;
    };

    // Verify that the pointers are properly aligned for 32-bit
    Q_ASSERT(quintptr(buffer) % sizeof(quint32) == 0);
    Q_ASSERT(quintptr(bufferEnd) % sizeof(quint32) == 0);

    quint32 *ptr = reinterpret_cast<quint32 *>(buffer);
    quint32 * const end = reinterpret_cast<quint32 *>(bufferEnd);

#if defined(Q_COMPILER_THREAD_LOCAL) && !defined(QT_BOOTSTRAPPED)
    if (SystemRandom::EfficientBufferFill && (end - ptr) < ThreadState::BufferCount
            && uint(qt_randomdevice_control) == 0) {
        thread_local ThreadState state;
        qssize_t itemsAvailable = ThreadState::BufferCount - state.idx;

        // copy as much as we already have
        qssize_t itemsToCopy = qMin(qssize_t(end - ptr), itemsAvailable);
        memcpy(ptr, state.buffer + state.idx, size_t(itemsToCopy) * sizeof(*ptr));
        ptr += itemsToCopy;

        if (ptr != end) {
            // refill the buffer and try again
            fill_internal(state.buffer, ThreadState::BufferCount);
            state.idx = 0;

            itemsToCopy = end - ptr;
            memcpy(ptr, state.buffer + state.idx, size_t(itemsToCopy) * sizeof(*ptr));
            ptr = end;
        }

        // erase what we copied and advance
#  ifdef Q_OS_WIN
        // Microsoft recommends this
        SecureZeroMemory(state.buffer + state.idx, size_t(itemsToCopy) * sizeof(*ptr));
#  else
        // We're quite confident the compiler will not optimize this out because
        // we're writing to a thread-local buffer
        memset(state.buffer + state.idx, 0, size_t(itemsToCopy) * sizeof(*ptr));
#  endif
        state.idx += itemsToCopy;
    }
#endif // Q_COMPILER_THREAD_LOCAL && !QT_BOOTSTRAPPED

    if (ptr != end) {
        // fill directly in the user buffer
        fill_internal(ptr, end - ptr);
    }
}

/*!
    \class QRandomGenerator
    \inmodule QtCore
    \since 5.10

    \brief The QRandomGenerator class allows one to obtain random values from a
    high-quality, seed-less Random Number Generator.

    QRandomGenerator may be used to generate random values from a high-quality
    random number generator. Unlike qrand(), QRandomGenerator does not need to be
    seeded. That also means it is not possible to force it to produce a
    reliable sequence, which may be needed for debugging.

    The class can generate 32-bit or 64-bit quantities, or fill an array of
    those. The most common way of generating new values is to call the generate(),
    generate64() or fillRange() functions. One would use it as:

    \code
        quint32 value = QRandomGenerator::generate();
    \endcode

    Additionally, it provides a floating-point function generateDouble() that returns
    a number in the range [0, 1) (that is, inclusive of zero and exclusive of
    1). There's also a set of convenience functions that facilitate obtaining a
    random number in a bounded, integral range.

    \warning This class is not suitable for bulk data creation. See below for the
    technical reasons.

    \section1 Frequency and entropy exhaustion

    QRandomGenerator does not need to be seeded and instead uses operating system
    or hardware facilities to generate random numbers. On some systems and with
    certain hardware, those facilities are true Random Number Generators.
    However, if they are true RNGs, those facilities have finite entropy source
    and thus may fail to produce any results if the entropy pool is exhausted.

    If that happens, first the operating system then QRandomGenerator will fall
    back to Pseudo Random Number Generators of decreasing qualities (Qt's
    fallback generator being the simplest). Therefore, QRandomGenerator should
    not be used for high-frequency random number generation, lest the entropy
    pool become empty. As a rule of thumb, this class should not be called upon
    to generate more than a kilobyte per second of random data (note: this may
    vary from system to system).

    If an application needs true RNG data in bulk, it should use the operating
    system facilities (such as \c{/dev/random} on Unix systems) directly and
    wait for entropy to become available. If true RNG is not required,
    applications should instead use a PRNG engines and can use QRandomGenerator to
    seed those.

    \section1 Standard C++ Library compatibility

    QRandomGenerator is modeled after
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/random_device}{std::random_device}}
    and may be used in almost all contexts that the Standard Library can.
    QRandomGenerator attempts to use either the same engine that backs
    \c{std::random_device} or a better one. Note that \c{std::random_device} is
    also allowed to fail if the source entropy pool becomes exhausted, in which
    case it will throw an exception. QRandomGenerator never throws, but may abort
    program execution instead.

    Like the Standard Library class, QRandomGenerator can be used to seed Standard
    Library deterministic random engines from \c{<random>}, such as the
    Mersenne Twister. Unlike \c{std::random_device}, QRandomGenerator also
    implements the API of
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/seed_seq}{std::seed_seq}},
    allowing it to seed the deterministic engines directly.

    The following code can be used to create and seed the
    implementation-defined default deterministic PRNG, then use it to fill a
    block range:

    \code
        QRandomGenerator rd;
        std::default_random_engine rng(rd);
        std::generate(block.begin(), block.end(), rng);

        // equivalent to:
        for (auto &v : block)
            v = rng();
    \endcode

    QRandomGenerator is also compatible with the uniform distribution classes
    \c{std::uniform_int_distribution} and \c{std:uniform_real_distribution}, as
    well as the free function \c{std::generate_canonical}. For example, the
    following code may be used to generate a floating-point number in the range
    [1, 2.5):

    \code
        QRandomGenerator64 rd;
        std::uniform_real_distribution dist(1, 2.5);
        return dist(rd);
    \endcode

    Note the use of the QRandomGenerator64 class instead of QRandomGenerator to
    obtain 64 bits of random data in a single call, though it is not required
    to make the algorithm work (the Standard Library functions will make as
    many calls as required to obtain enough bits of random data for the desired
    range).

    \sa QRandomGenerator64, qrand()
 */

/*!
    \fn QRandomGenerator::QRandomGenerator()
    \internal
    Defaulted constructor, does nothing.
 */

/*!
    \typedef QRandomGenerator::result_type

    A typedef to the type that operator()() returns. That is, quint32.

    \sa operator()()
 */

/*!
    \fn result_type QRandomGenerator::operator()()

    Generates a 32-bit random quantity and returns it.

    \sa QRandomGenerator::generate(), QRandomGenerator::generate64()
 */

/*!
    \fn double QRandomGenerator::entropy() const

    Returns the estimate of the entropy in the random generator source.

    This function exists to comply with the Standard Library requirements for
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/random_device}{std::random_device}}
    but it does not and cannot ever work. It is not possible to obtain a
    reliable entropy value in a shared entropy pool in a multi-tasking system,
    as other processes or threads may use that entropy. Any value non-zero
    value that this function could return would be obsolete by the time the
    user code reached it.

    Since QRandomGenerator attempts to use a hardware Random Number Generator,
    this function always returns 0.0.
 */

/*!
    \fn result_type QRandomGenerator::min()

    Returns the minimum value that QRandomGenerator may ever generate. That is, 0.

    \sa max(), QRandomGenerator64::max()
 */

/*!
    \fn result_type QRandomGenerator::max()

    Returns the maximum value that QRandomGenerator may ever generate. That is,
    \c {std::numeric_limits<result_type>::max()}.

    \sa min(), QRandomGenerator64::max()
 */

/*!
    \fn void QRandomGenerator::generate(ForwardIterator begin, ForwardIterator end)

    Generates 32-bit quantities and stores them in the range between \a begin
    and \a end. This function is equivalent to (and is implemented as):

    \code
        std::generate(begin, end, []() { return generate(); });
    \endcode

    This function complies with the requirements for the function
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/seed_seq/generate}{std::seed_seq::generate}},
    which requires unsigned 32-bit integer values.

    Note that if the [begin, end) range refers to an area that can store more
    than 32 bits per element, the elements will still be initialized with only
    32 bits of data. Any other bits will be zero. To fill the range with 64 bit
    quantities, one can write:

    \code
        std::generate(begin, end, []() { return QRandomGenerator::generate64(); });
    \endcode

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
    \fn void QRandomGenerator::fillRange(UInt *buffer, qssize_t count)

    Generates \a count 32- or 64-bit quantities (depending on the type \c UInt)
    and stores them in the buffer pointed by \a buffer. This is the most
    efficient way to obtain more than one quantity at a time, as it reduces the
    number of calls into the Random Number Generator source.

    For example, to fill a vector of 16 entries with random values, one may
    write:

    \code
        QVector<quint32> vector;
        vector.resize(16);
        QRandomGenerator::fillRange(vector.data(), vector.size());
    \endcode

    \sa generate()
 */

/*!
    \fn void QRandomGenerator::fillRange(UInt (&buffer)[N})

    Generates \c N 32- or 64-bit quantities (depending on the type \c UInt) and
    stores them in the \a buffer array. This is the most efficient way to
    obtain more than one quantity at a time, as it reduces the number of calls
    into the Random Number Generator source.

    For example, to fill generate two 32-bit quantities, one may write:

    \code
        quint32 array[2];
        QRandomGenerator::fillRange(array);
    \endcode

    It would have also been possible to make one call to generate64() and then split
    the two halves of the 64-bit value.

    \sa generate()
 */

/*!
    \fn qreal QRandomGenerator::generateDouble()

    Generates one random qreal in the canonical range [0, 1) (that is,
    inclusive of zero and exclusive of 1).

    This function is equivalent to:
    \code
        QRandomGenerator64 rd;
        return std::generate_canonical<qreal, std::numeric_limits<qreal>::digits>(rd);
    \endcode

    The same may also be obtained by using
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution}{std::uniform_real_distribution}}
    with parameters 0 and 1.

    \sa generate(), generate64(), bounded()
 */

/*!
    \fn qreal QRandomGenerator::bounded(qreal sup)

    Generates one random qreal in the range between 0 (inclusive) and \a
    sup (exclusive). This function is equivalent to and is implemented as:

    \code
        return generateDouble() * sup;
    \endcode

    \sa generateDouble(), bounded()
 */

/*!
    \fn quint32 QRandomGenerator::bounded(quint32 sup)
    \overload

    Generates one random 32-bit quantity in the range between 0 (inclusive) and
    \a sup (exclusive). The same result may also be obtained by using
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution}{std::uniform_int_distribution}}
    with parameters 0 and \c{sup - 1}. That class can also be used to obtain
    quantities larger than 32 bits.

    For example, to obtain a value between 0 and 255 (inclusive), one would write:

    \code
        quint32 v = QRandomGenerator::bounded(256);
    \endcode

    Naturally, the same could also be obtained by masking the result of generate()
    to only the lower 8 bits. Either solution is as efficient.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of quint32. Instead, use generate().

    \sa generate(), generate64(), generateDouble()
 */

/*!
    \fn quint32 QRandomGenerator::bounded(int sup)
    \overload

    Generates one random 32-bit quantity in the range between 0 (inclusive) and
    \a sup (exclusive). \a sup must not be negative.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of int. Instead, use generate() and cast to int.

    \sa generate(), generate64(), generateDouble()
 */

/*!
    \fn quint32 QRandomGenerator::bounded(quint32 min, quint32 sup)
    \overload

    Generates one random 32-bit quantity in the range between \a min (inclusive)
    and \a sup (exclusive). The same result may also be obtained by using
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution}{std::uniform_int_distribution}}
    with parameters \a min and \c{\a sup - 1}. That class can also be used to
    obtain quantities larger than 32 bits.

    For example, to obtain a value between 1000 (incl.) and 2000 (excl.), one
    would write:

    \code
        quint32 v = QRandomGenerator::bounded(1000, 2000);
    \endcode


    Note that this function cannot be used to obtain values in the full 32-bit
    range of quint32. Instead, use generate().

    \sa generate(), generate64(), generateDouble()
 */

/*!
    \fn quint32 QRandomGenerator::bounded(int min, int sup)
    \overload

    Generates one random 32-bit quantity in the range between \a min
    (inclusive) and \a sup (exclusive), both of which may be negative.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of int. Instead, use generate() and cast to int.

    \sa generate(), generate64(), generateDouble()
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
    \fn QRandomGenerator64::QRandomGenerator64()
    \internal
    Defaulted constructor, does nothing.
 */

/*!
    \typedef QRandomGenerator64::result_type

    A typedef to the type that operator()() returns. That is, quint64.

    \sa operator()()
 */

/*!
    \fn quint64 QRandomGenerator64::generate()

    Generates one 64-bit random value and returns it.

    Note about casting to a signed integer: all bits returned by this function
    are random, so there's a 50% chance that the most significant bit will be
    set. If you wish to cast the returned value to qint64 and keep it positive,
    you should mask the sign bit off:

    \code
        qint64 value = QRandomGenerator64::generate() & std::numeric_limits<qint64>::max();
    \endcode

    \sa QRandomGenerator, QRandomGenerator::generate64()
 */

/*!
    \fn result_type QRandomGenerator64::operator()()

    Generates a 64-bit random quantity and returns it.

    \sa QRandomGenerator::generate(), QRandomGenerator::generate64()
 */

/*!
    \fn double QRandomGenerator64::entropy() const

    Returns the estimate of the entropy in the random generator source.

    This function exists to comply with the Standard Library requirements for
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/random_device}{std::random_device}}
    but it does not and cannot ever work. It is not possible to obtain a
    reliable entropy value in a shared entropy pool in a multi-tasking system,
    as other processes or threads may use that entropy. Any value non-zero
    value that this function could return would be obsolete by the time the
    user code reached it.

    Since QRandomGenerator64 attempts to use a hardware Random Number Generator,
    this function always returns 0.0.
 */

/*!
    \fn result_type QRandomGenerator64::min()

    Returns the minimum value that QRandomGenerator64 may ever generate. That is, 0.

    \sa max(), QRandomGenerator::max()
 */

/*!
    \fn result_type QRandomGenerator64::max()

    Returns the maximum value that QRandomGenerator64 may ever generate. That is,
    \c {std::numeric_limits<result_type>::max()}.

    \sa min(), QRandomGenerator::max()
 */

/*!
    Generates one 32-bit random value and returns it.

    Note about casting to a signed integer: all bits returned by this function
    are random, so there's a 50% chance that the most significant bit will be
    set. If you wish to cast the returned value to int and keep it positive,
    you should mask the sign bit off:

    \code
        int value = QRandomGenerator::generate() & std::numeric_limits<int>::max();
    \endcode

    \sa generate64(), generateDouble()
 */
quint32 QRandomGenerator::generate()
{
    quint32 ret;
    fill(&ret, &ret + 1);
    return ret;
}

/*!
    Generates one 64-bit random value and returns it.

    Note about casting to a signed integer: all bits returned by this function
    are random, so there's a 50% chance that the most significant bit will be
    set. If you wish to cast the returned value to qint64 and keep it positive,
    you should mask the sign bit off:

    \code
        qint64 value = QRandomGenerator::generate64() & std::numeric_limits<qint64>::max();
    \endcode

    \sa generate(), generateDouble(), QRandomGenerator64
 */
quint64 QRandomGenerator::generate64()
{
    quint64 ret;
    fill(&ret, &ret + 1);
    return ret;
}

/*!
    \internal

    Fills the range pointed by \a buffer and \a bufferEnd with 32-bit random
    values. The buffer must be correctly aligned.
 */
void QRandomGenerator::fillRange_helper(void *buffer, void *bufferEnd)
{
    fill(buffer, bufferEnd);
}

#if defined(Q_OS_ANDROID) && (__ANDROID_API__ < 21)
typedef QThreadStorage<QJNIObjectPrivate> AndroidRandomStorage;
Q_GLOBAL_STATIC(AndroidRandomStorage, randomTLS)

#elif defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
using SeedStorageType = QtPrivate::FunctionPointer<decltype(&srand)>::Arguments::Car;

typedef QThreadStorage<SeedStorageType *> SeedStorage;
Q_GLOBAL_STATIC(SeedStorage, randTLS)  // Thread Local Storage for seed value

#endif

/*!
    \relates <QtGlobal>
    \since 4.2

    Thread-safe version of the standard C++ \c srand() function.

    Sets the argument \a seed to be used to generate a new random number sequence of
    pseudo random integers to be returned by qrand().

    The sequence of random numbers generated is deterministic per thread. For example,
    if two threads call qsrand(1) and subsequently call qrand(), the threads will get
    the same random number sequence.

    \sa qrand(), QRandomGenerator
*/
void qsrand(uint seed)
{
#if defined(Q_OS_ANDROID) && (__ANDROID_API__ < 21)
    if (randomTLS->hasLocalData()) {
        randomTLS->localData().callMethod<void>("setSeed", "(J)V", jlong(seed));
        return;
    }

    QJNIObjectPrivate random("java/util/Random",
                             "(J)V",
                             jlong(seed));
    if (!random.isValid()) {
        srand(seed);
        return;
    }

    randomTLS->setLocalData(random);
#elif defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
    SeedStorage *seedStorage = randTLS();
    if (seedStorage) {
        SeedStorageType *pseed = seedStorage->localData();
        if (!pseed)
            seedStorage->setLocalData(pseed = new SeedStorageType);
        *pseed = seed;
    } else {
        //global static seed storage should always exist,
        //except after being deleted by QGlobalStaticDeleter.
        //But since it still can be called from destructor of another
        //global static object, fallback to srand(seed)
        srand(seed);
    }
#else
    // On Windows srand() and rand() already use Thread-Local-Storage
    // to store the seed between calls
    // this is also valid for QT_NO_THREAD
    srand(seed);
#endif
}

/*!
    \relates <QtGlobal>
    \since 4.2

    Thread-safe version of the standard C++ \c rand() function.

    Returns a value between 0 and \c RAND_MAX (defined in \c <cstdlib> and
    \c <stdlib.h>), the next number in the current sequence of pseudo-random
    integers.

    Use \c qsrand() to initialize the pseudo-random number generator with a
    seed value. Seeding must be performed at least once on each thread. If that
    step is skipped, then the sequence will be pre-seeded with a constant
    value.

    \sa qsrand(), QRandomGenerator
*/
int qrand()
{
#if defined(Q_OS_ANDROID) && (__ANDROID_API__ < 21)
    AndroidRandomStorage *randomStorage = randomTLS();
    if (!randomStorage)
        return rand();

    if (randomStorage->hasLocalData()) {
        return randomStorage->localData().callMethod<jint>("nextInt",
                                                           "(I)I",
                                                           RAND_MAX);
    }

    QJNIObjectPrivate random("java/util/Random",
                             "(J)V",
                             jlong(1));

    if (!random.isValid())
        return rand();

    randomStorage->setLocalData(random);
    return random.callMethod<jint>("nextInt", "(I)I", RAND_MAX);
#elif defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
    SeedStorage *seedStorage = randTLS();
    if (seedStorage) {
        SeedStorageType *pseed = seedStorage->localData();
        if (!pseed) {
            seedStorage->setLocalData(pseed = new SeedStorageType);
            *pseed = 1;
        }
        return rand_r(pseed);
    } else {
        //global static seed storage should always exist,
        //except after being deleted by QGlobalStaticDeleter.
        //But since it still can be called from destructor of another
        //global static object, fallback to rand()
        return rand();
    }
#else
    // On Windows srand() and rand() already use Thread-Local-Storage
    // to store the seed between calls
    // this is also valid for QT_NO_THREAD
    return rand();
#endif
}

QT_END_NAMESPACE
