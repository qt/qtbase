// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qbytearraymatcher.h"

#include <qtconfiginclude.h>
#ifndef QT_BOOTSTRAPPED
#  include <private/qtcore-config_p.h>
#endif

#include <limits.h>

QT_BEGIN_NAMESPACE

static inline void bm_init_skiptable(const uchar *cc, qsizetype len, uchar *skiptable)
{
    int l = int(qMin(len, qsizetype(255)));
    memset(skiptable, l, 256*sizeof(uchar));
    cc += len - l;
    while (l--)
        skiptable[*cc++] = l;
}

static inline qsizetype bm_find(const uchar *cc, qsizetype l, qsizetype index, const uchar *puc,
                                qsizetype pl, const uchar *skiptable)
{
    if (pl == 0)
        return index > l ? -1 : index;
    const qsizetype pl_minus_one = pl - 1;

    const uchar *current = cc + index + pl_minus_one;
    const uchar *end = cc + l;
    while (current < end) {
        qsizetype skip = skiptable[*current];
        if (!skip) {
            // possible match
            while (skip < pl) {
                if (*(current - skip) != puc[pl_minus_one - skip])
                    break;
                skip++;
            }
            if (skip > pl_minus_one) // we have a match
                return (current - cc) - skip + 1;

            // in case we don't have a match we are a bit inefficient as we only skip by one
            // when we have the non matching char in the string.
            if (skiptable[*(current - skip)] == pl)
                skip = pl - skip;
            else
                skip = 1;
        }
        if (current > end - skip)
            break;
        current += skip;
    }
    return -1; // not found
}

/*! \class QByteArrayMatcher
    \inmodule QtCore
    \brief The QByteArrayMatcher class holds a sequence of bytes that
    can be quickly matched in a byte array.

    \ingroup tools
    \ingroup string-processing

    This class is useful when you have a sequence of bytes that you
    want to repeatedly match against some byte arrays (perhaps in a
    loop), or when you want to search for the same sequence of bytes
    multiple times in the same byte array. Using a matcher object and
    indexIn() is faster than matching a plain QByteArray with
    QByteArray::indexOf() if repeated matching takes place. This
    class offers no benefit if you are doing one-off byte array
    matches.

    Create the QByteArrayMatcher with the QByteArray you want to
    search for. Then call indexIn() on the QByteArray that you want to
    search.

    \sa QByteArray, QStringMatcher
*/

/*!
    Constructs an empty byte array matcher that won't match anything.
    Call setPattern() to give it a pattern to match.
*/
QByteArrayMatcher::QByteArrayMatcher()
    : d(nullptr)
{
    p.p = nullptr;
    p.l = 0;
    memset(p.q_skiptable, 0, sizeof(p.q_skiptable));
}

/*!
    Constructs a byte array matcher from \a pattern. \a pattern
    has the given \a length. Call indexIn() to perform a search.

    \note the data that \a pattern is referencing must remain valid while this
    object is used.
*/
QByteArrayMatcher::QByteArrayMatcher(const char *pattern, qsizetype length) : d(nullptr)
{
    p.p = reinterpret_cast<const uchar *>(pattern);
    if (length < 0)
        p.l = qstrlen(pattern);
    else
        p.l = length;
    bm_init_skiptable(p.p, p.l, p.q_skiptable);
}

/*!
    Constructs a byte array matcher that will search for \a pattern.
    Call indexIn() to perform a search.
*/
QByteArrayMatcher::QByteArrayMatcher(const QByteArray &pattern)
    : d(nullptr), q_pattern(pattern)
{
    p.p = reinterpret_cast<const uchar *>(pattern.constData());
    p.l = pattern.size();
    bm_init_skiptable(p.p, p.l, p.q_skiptable);
}

/*!
    \fn QByteArrayMatcher::QByteArrayMatcher(QByteArrayView pattern)
    \since 6.3
    \overload

    Constructs a byte array matcher that will search for \a pattern.
    Call indexIn() to perform a search.

    \note the data that \a pattern is referencing must remain valid while this
    object is used.
*/

/*!
    Copies the \a other byte array matcher to this byte array matcher.
*/
QByteArrayMatcher::QByteArrayMatcher(const QByteArrayMatcher &other)
    : d(nullptr)
{
    operator=(other);
}

/*!
    Destroys the byte array matcher.
*/
QByteArrayMatcher::~QByteArrayMatcher()
{
    Q_UNUSED(d);
}

/*!
    Assigns the \a other byte array matcher to this byte array matcher.
*/
QByteArrayMatcher &QByteArrayMatcher::operator=(const QByteArrayMatcher &other)
{
    q_pattern = other.q_pattern;
    memcpy(&p, &other.p, sizeof(p));
    return *this;
}

/*!
    Sets the byte array that this byte array matcher will search for
    to \a pattern.

    \sa pattern(), indexIn()
*/
void QByteArrayMatcher::setPattern(const QByteArray &pattern)
{
    q_pattern = pattern;
    p.p = reinterpret_cast<const uchar *>(pattern.constData());
    p.l = pattern.size();
    bm_init_skiptable(p.p, p.l, p.q_skiptable);
}

/*!
    Searches the char string \a str, which has length \a len, from
    byte position \a from (default 0, i.e. from the first byte), for
    the byte array pattern() that was set in the constructor or in the
    most recent call to setPattern(). Returns the position where the
    pattern() matched in \a str, or -1 if no match was found.
*/
qsizetype QByteArrayMatcher::indexIn(const char *str, qsizetype len, qsizetype from) const
{
    if (from < 0)
        from = 0;
    return bm_find(reinterpret_cast<const uchar *>(str), len, from,
                   p.p, p.l, p.q_skiptable);
}

/*!
    \fn qsizetype QByteArrayMatcher::indexIn(QByteArrayView data, qsizetype from) const
    \since 6.3
    \overload

    Searches the byte array \a data, from byte position \a from (default
    0, i.e. from the first byte), for the byte array pattern() that
    was set in the constructor or in the most recent call to
    setPattern(). Returns the position where the pattern() matched in
    \a data, or -1 if no match was found.
*/
qsizetype QByteArrayMatcher::indexIn(QByteArrayView data, qsizetype from) const
{
    if (from < 0)
        from = 0;
    return bm_find(reinterpret_cast<const uchar *>(data.data()), data.size(), from,
                   p.p, p.l, p.q_skiptable);
}

/*!
    \fn QByteArray QByteArrayMatcher::pattern() const

    Returns the byte array pattern that this byte array matcher will
    search for.

    \sa setPattern()
*/

/*!
    \internal
 */
Q_NEVER_INLINE
static qsizetype qFindByteArrayBoyerMoore(
    const char *haystack, qsizetype haystackLen, qsizetype haystackOffset,
    const char *needle, qsizetype needleLen)
{
    uchar skiptable[256];
    bm_init_skiptable((const uchar *)needle, needleLen, skiptable);
    if (haystackOffset < 0)
        haystackOffset = 0;
    return bm_find((const uchar *)haystack, haystackLen, haystackOffset,
                   (const uchar *)needle, needleLen, skiptable);
}

/*!
    \internal
 */
static qsizetype qFindByteArray(const char *haystack0, qsizetype l, qsizetype from,
                                const char *needle, qsizetype sl);
qsizetype QtPrivate::findByteArray(QByteArrayView haystack, qsizetype from, QByteArrayView needle) noexcept
{
    const auto haystack0 = haystack.data();
    const auto l = haystack.size();
    const auto sl = needle.size();
#if !QT_CONFIG(memmem)
    if (sl == 1)
        return findByteArray(haystack, from, needle.front());
#endif

    if (from < 0)
        from += l;
    if (std::size_t(sl + from) > std::size_t(l))
        return -1;
    if (!sl)
        return from;
    if (!l)
        return -1;

#if QT_CONFIG(memmem)
    auto where = memmem(haystack0 + from, l - from, needle.data(), sl);
    return where ? static_cast<const char *>(where) - haystack0 : -1;
#endif

    /*
      We use the Boyer-Moore algorithm in cases where the overhead
      for the skip table should pay off, otherwise we use a simple
      hash function.
    */
    if (l > 500 && sl > 5)
        return qFindByteArrayBoyerMoore(haystack0, l, from, needle.data(), sl);
    return qFindByteArray(haystack0, l, from, needle.data(), sl);
}

qsizetype qFindByteArray(const char *haystack0, qsizetype l, qsizetype from,
                         const char *needle, qsizetype sl)
{
    /*
      We use some hashing for efficiency's sake. Instead of
      comparing strings, we compare the hash value of str with that
      of a part of this QByteArray. Only if that matches, we call memcmp().
    */
    const char *haystack = haystack0 + from;
    const char *end = haystack0 + (l - sl);
    const qregisteruint sl_minus_1 = sl - 1;
    qregisteruint hashNeedle = 0, hashHaystack = 0;
    qsizetype idx;
    for (idx = 0; idx < sl; ++idx) {
        hashNeedle = ((hashNeedle<<1) + needle[idx]);
        hashHaystack = ((hashHaystack<<1) + haystack[idx]);
    }
    hashHaystack -= *(haystack + sl_minus_1);

    while (haystack <= end) {
        hashHaystack += *(haystack + sl_minus_1);
        if (hashHaystack == hashNeedle && *needle == *haystack
             && memcmp(needle, haystack, sl) == 0)
            return haystack - haystack0;

        if (sl_minus_1 < sizeof(sl_minus_1) * CHAR_BIT)
            hashHaystack -= qregisteruint(*haystack) << sl_minus_1;
        hashHaystack <<= 1;
        ++haystack;
    }
    return -1;
}

/*!
    \class QStaticByteArrayMatcherBase
    \since 5.9
    \internal
    \brief Non-template base class of QStaticByteArrayMatcher.
*/

/*!
    \class QStaticByteArrayMatcher
    \since 5.9
    \inmodule QtCore
    \brief The QStaticByteArrayMatcher class is a compile-time version of QByteArrayMatcher.

    \ingroup tools
    \ingroup string-processing

    This class is useful when you have a sequence of bytes that you
    want to repeatedly match against some byte arrays (perhaps in a
    loop), or when you want to search for the same sequence of bytes
    multiple times in the same byte array. Using a matcher object and
    indexIn() is faster than matching a plain QByteArray with
    QByteArray::indexOf(), in particular if repeated matching takes place.

    Unlike QByteArrayMatcher, this class calculates the internal
    representation at \e{compile-time}, so it can
    even benefit if you are doing one-off byte array matches.

    Create the QStaticByteArrayMatcher by calling qMakeStaticByteArrayMatcher(),
    passing it the C string literal you want to search for. Store the return
    value of that function in a \c{static const auto} variable, so you don't need
    to pass the \c{N} template parameter explicitly:

    \snippet code/src_corelib_text_qbytearraymatcher.cpp 0

    Then call indexIn() on the QByteArray in which you want to search, just like
    with QByteArrayMatcher.

    Since this class is designed to do all the up-front calculations at compile-time,
    it does not offer a setPattern() method.

    \sa QByteArrayMatcher, QStringMatcher
*/

/*!
    \fn template <size_t N> qsizetype QStaticByteArrayMatcher<N>::indexIn(const char *haystack, qsizetype hlen, qsizetype from = 0) const

    Searches the char string \a haystack, which has length \a hlen, from
    byte position \a from (default 0, i.e. from the first byte), for
    the byte array pattern() that was set in the constructor.

    Returns the position where the pattern() matched in \a haystack, or -1 if no match was found.
*/

/*!
    \fn template <size_t N> qsizetype QStaticByteArrayMatcher<N>::indexIn(const QByteArray &haystack, qsizetype from = 0) const

    Searches the char string \a haystack, from byte position \a from
    (default 0, i.e. from the first byte), for the byte array pattern()
    that was set in the constructor.

    Returns the position where the pattern() matched in \a haystack, or -1 if no match was found.
*/

/*!
    \fn template <size_t N> QByteArray QStaticByteArrayMatcher<N>::pattern() const

    Returns the byte array pattern that this byte array matcher will
    search for.

    \sa QByteArrayMatcher::setPattern()
*/

/*!
    \internal
*/
qsizetype QStaticByteArrayMatcherBase::indexOfIn(const char *needle, size_t nlen, const char *haystack, qsizetype hlen, qsizetype from) const noexcept
{
    if (from < 0)
        from = 0;
    return bm_find(reinterpret_cast<const uchar *>(haystack), hlen, from,
                   reinterpret_cast<const uchar *>(needle),   nlen, m_skiptable.data);
}

/*!
    \fn template <size_t N> QStaticByteArrayMatcher<N>::QStaticByteArrayMatcher(const char (&pattern)[N])
    \internal
*/

/*!
    \fn template <size_t N> QStaticByteArrayMatcher qMakeStaticByteArrayMatcher(const char (&pattern)[N])
    \since 5.9
    \relates QStaticByteArrayMatcher

    Return a QStaticByteArrayMatcher with the correct \c{N} determined
    automatically from the \a pattern passed.

    To take full advantage of this function, assign the result to an
    \c{auto} variable:

    \snippet code/src_corelib_text_qbytearraymatcher.cpp 1
*/

QT_END_NAMESPACE
