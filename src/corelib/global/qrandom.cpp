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
#include <qthreadstorage.h>

#if QT_HAS_INCLUDE(<random>)
#  include <random>
#endif

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

QT_BEGIN_NAMESPACE

namespace {
#ifdef Q_OS_UNIX
class SystemRandom
{
    static QBasicAtomicInt s_fdp1;  // "file descriptor plus 1"
    static int openDevice();
    SystemRandom() {}
    ~SystemRandom();
public:
    enum { EfficientBufferFill = true };
    static qssize_t fillBuffer(void *buffer, qssize_t count);
};
QBasicAtomicInt SystemRandom::s_fdp1 = Q_BASIC_ATOMIC_INITIALIZER(0);

SystemRandom::~SystemRandom()
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
    static qssize_t fillBuffer(void *buffer, qssize_t count)
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
    static qssize_t fillBuffer(void *, qssize_t)
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
#elif QT_HAS_INCLUDE(<chrono>)
static QBasicAtomicInteger<unsigned> seed = Q_BASIC_ATOMIC_INITIALIZER(0U);
static void fallback_update_seed(unsigned value)
{
    // Update the seed to be used for the fallback mechansim, if we need to.
    // We can't use QtPrivate::QHashCombine here because that is not an atomic
    // operation. A simple XOR will have to do then.
    seed.fetchAndXorRelaxed(value);
}

static void fallback_fill(quint32 *ptr, qssize_t left) Q_DECL_NOTHROW
{
    Q_ASSERT(left);

    // this is highly inefficient, we should save the generator across calls...
    std::mt19937 generator(seed.load());
    std::generate(ptr, ptr + left, generator);

    fallback_update_seed(*ptr);
}
#else
static void fallback_update_seed(unsigned) {}
static Q_NORETURN void fallback_fill(quint32 *, qssize_t)
{
    qFatal("Random number generator failed and no high-quality backup available");
}
#endif

static void fill_internal(quint32 *buffer, qssize_t count)
{
    if (Q_UNLIKELY(uint(qt_randomdevice_control) & SetRandomData)) {
        uint value = uint(qt_randomdevice_control) & RandomDataMask;
        std::fill_n(buffer, count, value);
        return;
    }

    qssize_t filled = 0;
    if (uint(qt_randomdevice_control) & SkipSystemRNG) == 0) {
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

/**
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
    those. The most common way of generating new values is to call the get32(),
    get64() or fillRange() functions. One would use it as:

    \code
        quint32 value = QRandomGenerator::get32();
    \endcode

    Additionally, it provides a floating-point function getReal() that returns
    a number in the range [0, 1) (that is, inclusive of zero and exclusive of
    1). There's also a set of convenience functions that facilitate obtaininga
    random number in a bounded, integral range.

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

/**
    \fn QRandomGenerator::QRandomGenerator()
    \internal
    Defaulted constructor, does nothing.
 */

/**
    \typedef QRandomGenerator::result_type

    A typedef to the type that operator()() returns. That is, quint32.

    \sa operator()()
 */

/**
    \fn result_type QRandomGenerator::operator()()

    Generates a 32-bit random quantity and returns it.

    \sa QRandomGenerator::get32(), QRandomGenerator::get64()
 */

/**
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

/**
    \fn result_type QRandomGenerator::min()

    Returns the minimum value that QRandomGenerator may ever generate. That is, 0.

    \sa max(), QRandomGenerator64::max()
 */

/**
    \fn result_type QRandomGenerator::max()

    Returns the maximum value that QRandomGenerator may ever generate. That is,
    \c {std::numeric_limits<result_type>::max()}.

    \sa min(), QRandomGenerator64::max()
 */

/**
    \fn void QRandomGenerator::generate(ForwardIterator begin, ForwardIterator end)

    Generates 32-bit quantities and stores them in the range between \a begin
    and \a end. This function is equivalent to (and is implemented as):

    \code
        std::generate(begin, end, []() { return get32(); });
    \endcode

    This function complies with the requirements for the function
    \c{\l{http://en.cppreference.com/w/cpp/numeric/random/seed_seq/generate}{std::seed_seq::generate}},
    which requires unsigned 32-bit integer values.

    Note that if the [begin, end) range refers to an area that can store more
    than 32 bits per element, the elements will still be initialized with only
    32 bits of data. Any other bits will be zero. To fill the range with 64 bit
    quantities, one can write:

    \code
        std::generate(begin, end, []() { return get64(); });
    \endcode

    If the range refers to contiguous memory (such as an array or the data from
    a QVector), the fillRange() function may be used too.

    \sa fillRange()
 */

/**
    \fn void QRandomGenerator::generate(quint32 *begin, quint32 *end)
    \overload
    \internal

    Same as the other overload, but more efficiently fills \a begin to \a end.
 */

/**
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

/**
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

    It would have also been possible to make one call to get64() and then split
    the two halves of the 64-bit value.

    \sa generate()
 */

/**
    \fn qreal QRandomGenerator::getReal()

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

    \sa get32(), get64(), bounded()
 */

/**
    \fn qreal QRandomGenerator::bounded(qreal sup)

    Generates one random qreal in the range between 0 (inclusive) and \a
    sup (exclusive). This function is equivalent to and is implemented as:

    \code
        return getReal() * sup;
    \endcode

    \sa getReal(), bounded()
 */

/**
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

    Naturally, the same could also be obtained by masking the result of get32()
    to only the lower 8 bits. Either solution is as efficient.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of quint32. Instead, use get32().

    \sa get32(), get64(), getReal()
 */

/**
    \fn quint32 QRandomGenerator::bounded(int sup)
    \overload

    Generates one random 32-bit quantity in the range between 0 (inclusive) and
    \a sup (exclusive). \a sup must not be negative.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of int. Instead, use get32() and cast to int.

    \sa get32(), get64(), getReal()
 */

/**
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
    range of quint32. Instead, use get32().

    \sa get32(), get64(), getReal()
 */

/**
    \fn quint32 QRandomGenerator::bounded(int min, int sup)
    \overload

    Generates one random 32-bit quantity in the range between \a min
    (inclusive) and \a sup (exclusive), both of which may be negative.

    Note that this function cannot be used to obtain values in the full 32-bit
    range of int. Instead, use get32() and cast to int.

    \sa get32(), get64(), getReal()
 */

/**
    \class QRandomGenerator64
    \inmodule QtCore
    \since 5.10

    \brief The QRandomGenerator64 class allows one to obtain 64-bit random values
    from a high-quality, seed-less Random Number Generator.

    QRandomGenerator64 is a simple adaptor class around QRandomGenerator, making the
    QRandomGenerator::get64() function the default for operator()(), instead of the
    function that returns 32-bit quantities. This class is intended to be used
    in conjunction with Standard Library algorithms that need 64-bit quantities
    instead of 32-bit ones.

    In all other aspects, the class is the same. Please refer to
    QRandomGenerator's documentation for more information.

    \sa QRandomGenerator
*/

/**
    \fn QRandomGenerator64::QRandomGenerator64()
    \internal
    Defaulted constructor, does nothing.
 */

/**
    \typedef QRandomGenerator64::result_type

    A typedef to the type that operator()() returns. That is, quint64.

    \sa operator()()
 */

/**
    \fn result_type QRandomGenerator64::operator()()

    Generates a 64-bit random quantity and returns it.

    \sa QRandomGenerator::get32(), QRandomGenerator::get64()
 */

/**
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

/**
    \fn result_type QRandomGenerator64::min()

    Returns the minimum value that QRandomGenerator64 may ever generate. That is, 0.

    \sa max(), QRandomGenerator::max()
 */

/**
    \fn result_type QRandomGenerator64::max()

    Returns the maximum value that QRandomGenerator64 may ever generate. That is,
    \c {std::numeric_limits<result_type>::max()}.

    \sa min(), QRandomGenerator::max()
 */

/**
    Generates one 32-bit random value and returns it.

    \sa get64(), getReal()
 */
quint32 QRandomGenerator::get32()
{
    quint32 ret;
    fill(&ret, &ret + 1);
    return ret;
}

/**
    Generates one 64-bit random value and returns it.

    \sa get32(), getReal(), QRandomGenerator64
 */
quint64 QRandomGenerator::get64()
{
    quint64 ret;
    fill(&ret, &ret + 1);
    return ret;
}

/**
    \internal

    Fills the range pointed by \a buffer and \a bufferEnd with 32-bit random
    values. The buffer must be correctly aligned.
 */
void QRandomGenerator::fillRange_helper(void *buffer, void *bufferEnd)
{
    fill(buffer, bufferEnd);
}

#if defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)

#  if defined(Q_OS_INTEGRITY) && defined(__GHS_VERSION_NUMBER) && (__GHS_VERSION_NUMBER < 500)
// older versions of INTEGRITY used a long instead of a uint for the seed.
typedef long SeedStorageType;
#  else
typedef uint SeedStorageType;
#  endif

typedef QThreadStorage<SeedStorageType *> SeedStorage;
Q_GLOBAL_STATIC(SeedStorage, randTLS)  // Thread Local Storage for seed value

#elif defined(Q_OS_ANDROID)
typedef QThreadStorage<QJNIObjectPrivate> AndroidRandomStorage;
Q_GLOBAL_STATIC(AndroidRandomStorage, randomTLS)
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
#if defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
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
#elif defined(Q_OS_ANDROID)
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
#if defined(Q_OS_UNIX) && !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
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
#elif defined(Q_OS_ANDROID)
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
#else
    // On Windows srand() and rand() already use Thread-Local-Storage
    // to store the seed between calls
    // this is also valid for QT_NO_THREAD
    return rand();
#endif
}

QT_END_NAMESPACE
