// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Mail.ru Group.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qstringmatcher.h"

QT_BEGIN_NAMESPACE

static constexpr qsizetype FoldBufferCapacity = 256;

static void bm_init_skiptable(QStringView needle, uchar *skiptable, Qt::CaseSensitivity cs)
{
    const char16_t *uc = needle.utf16();
    const qsizetype len =
            cs == Qt::CaseSensitive ? needle.size() : qMin(needle.size(), FoldBufferCapacity);
    int l = qMin(int(len), 255);
    memset(skiptable, l, 256 * sizeof(uchar));
    uc += len - l;
    if (cs == Qt::CaseSensitive) {
        while (l--) {
            skiptable[*uc & 0xff] = l;
            ++uc;
        }
    } else {
        const char16_t *start = uc;
        while (l--) {
            skiptable[foldCase(uc, start) & 0xff] = l;
            ++uc;
        }
    }
}

static inline qsizetype bm_find(QStringView haystack, qsizetype index, QStringView needle,
                          const uchar *skiptable, Qt::CaseSensitivity cs)
{
    const char16_t *uc = haystack.utf16();
    const qsizetype l = haystack.size();
    const char16_t *puc = needle.utf16();
    const qsizetype pl = needle.size();

    if (pl == 0)
        return index > l ? -1 : index;

    if (cs == Qt::CaseSensitive) {
        const qsizetype pl_minus_one = pl - 1;
        const char16_t *current = uc + index + pl_minus_one;
        const char16_t *end = uc + l;

        while (current < end) {
            qsizetype skip = skiptable[*current & 0xff];
            if (!skip) {
                // possible match
                while (skip < pl) {
                    if (*(current - skip) != puc[pl_minus_one-skip])
                        break;
                    ++skip;
                }
                if (skip > pl_minus_one) // we have a match
                    return (current - uc) - pl_minus_one;

                // in case we don't have a match we are a bit inefficient as we only skip by one
                // when we have the non matching char in the string.
                if (skiptable[*(current - skip) & 0xff] == pl)
                    skip = pl - skip;
                else
                    skip = 1;
            }
            if (current > end - skip)
                break;
            current += skip;
        }
    } else {
        char16_t foldBuffer[FoldBufferCapacity];
        const qsizetype foldBufferLength = qMin(FoldBufferCapacity, pl);
        const char16_t *start = puc;
        for (qsizetype i = 0; i < foldBufferLength; ++i)
            foldBuffer[i] = foldCase(&puc[i], start);
        QStringView restNeedle = needle.sliced(foldBufferLength);
        const qsizetype foldBufferEnd = foldBufferLength - 1;
        const char16_t *current = uc + index + foldBufferEnd;
        const char16_t *end = uc + l;

        while (current < end) {
            qsizetype skip = skiptable[foldCase(current, uc) & 0xff];
            if (!skip) {
                // possible match
                while (skip < foldBufferLength) {
                    if (foldCase(current - skip, uc) != foldBuffer[foldBufferEnd - skip])
                        break;
                    ++skip;
                }
                if (skip > foldBufferEnd) { // Matching foldBuffer
                    qsizetype candidatePos = (current - uc) - foldBufferEnd;
                    QStringView restHaystack =
                            haystack.sliced(qMin(haystack.size(), candidatePos + foldBufferLength));
                    if (restNeedle.size() == 0
                        || restHaystack.startsWith(
                                restNeedle, Qt::CaseInsensitive)) // Check the rest of the string
                        return candidatePos;
                }
                // in case we don't have a match we are a bit inefficient as we only skip by one
                // when we have the non matching char in the string.
                if (skiptable[foldCase(current - skip, uc) & 0xff] == foldBufferLength)
                    skip = foldBufferLength - skip;
                else
                    skip = 1;
            }
            if (current > end - skip)
                break;
            current += skip;
        }
    }
    return -1; // not found
}

void QStringMatcher::updateSkipTable()
{
    bm_init_skiptable(q_sv, q_skiptable, q_cs);
}

/*!
    \class QStringMatcher
    \inmodule QtCore
    \brief The QStringMatcher class holds a sequence of characters that
    can be quickly matched in a Unicode string.

    \ingroup tools
    \ingroup string-processing

    This class is useful when you have a sequence of \l{QChar}s that
    you want to repeatedly match against some strings (perhaps in a
    loop), or when you want to search for the same sequence of
    characters multiple times in the same string. Using a matcher
    object and indexIn() is faster than matching a plain QString with
    QString::indexOf() if repeated matching takes place. This class
    offers no benefit if you are doing one-off string matches.

    Create the QStringMatcher with the QString you want to search
    for. Then call indexIn() on the QString that you want to search.

    \sa QString, QByteArrayMatcher, QRegularExpression
*/

/*! \fn QStringMatcher::QStringMatcher()

    Constructs an empty string matcher that won't match anything.
    Call setPattern() to give it a pattern to match.
*/

/*!
    Constructs a string matcher that will search for \a pattern, with
    case sensitivity \a cs.

    Call indexIn() to perform a search.
*/
QStringMatcher::QStringMatcher(const QString &pattern, Qt::CaseSensitivity cs)
    : d_ptr(nullptr), q_cs(cs), q_pattern(pattern)
{
    q_sv = q_pattern;
    updateSkipTable();
}

/*!
    \fn QStringMatcher::QStringMatcher(const QChar *uc, qsizetype length, Qt::CaseSensitivity cs)
    \since 4.5

    Constructs a string matcher that will search for the pattern referred to
    by \a uc with the given \a length and case sensitivity specified by \a cs.
*/

/*!
    \fn QStringMatcher::QStringMatcher(QStringView pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    \since 5.14

    Constructs a string matcher that will search for \a pattern, with
    case sensitivity \a cs.

    Call indexIn() to perform a search.
*/
QStringMatcher::QStringMatcher(QStringView str, Qt::CaseSensitivity cs)
    : d_ptr(nullptr), q_cs(cs), q_sv(str)
{
    updateSkipTable();
}
/*!
    Copies the \a other string matcher to this string matcher.
*/
QStringMatcher::QStringMatcher(const QStringMatcher &other)
    : d_ptr(nullptr)
{
    operator=(other);
}

/*!
    Destroys the string matcher.
*/
QStringMatcher::~QStringMatcher()
{
    Q_UNUSED(d_ptr);
}

/*!
    Assigns the \a other string matcher to this string matcher.
*/
QStringMatcher &QStringMatcher::operator=(const QStringMatcher &other)
{
    if (this != &other) {
        q_pattern = other.q_pattern;
        q_cs = other.q_cs;
        q_sv = other.q_sv;
        memcpy(q_skiptable, other.q_skiptable, sizeof(q_skiptable));
    }
    return *this;
}

/*!
    Sets the string that this string matcher will search for to \a
    pattern.

    \sa pattern(), setCaseSensitivity(), indexIn()
*/
void QStringMatcher::setPattern(const QString &pattern)
{
    q_pattern = pattern;
    q_sv = q_pattern;
    updateSkipTable();
}

/*!
    \fn QString QStringMatcher::pattern() const

    Returns the string pattern that this string matcher will search
    for.

    \sa setPattern()
*/

QString QStringMatcher::pattern() const
{
    if (!q_pattern.isEmpty())
        return q_pattern;
    return q_sv.toString();
}

/*!
    \fn QStringView QStringMatcher::patternView() const noexcept
    \since 6.7

    Returns a string view of the pattern that this string matcher will search for.

    \sa setPattern()
*/

/*!
    Sets the case sensitivity setting of this string matcher to \a
    cs.

    \sa caseSensitivity(), setPattern(), indexIn()
*/
void QStringMatcher::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    if (cs == q_cs)
        return;
    q_cs = cs;
    updateSkipTable();
}

/*! \fn qsizetype QStringMatcher::indexIn(const QString &str, qsizetype from) const

    Searches the string \a str from character position \a from
    (default 0, i.e. from the first character), for the string
    pattern() that was set in the constructor or in the most recent
    call to setPattern(). Returns the position where the pattern()
    matched in \a str, or -1 if no match was found.

    \sa setPattern(), setCaseSensitivity()
*/

/*! \fn qsizetype QStringMatcher::indexIn(const QChar *str, qsizetype length, qsizetype from) const
    \since 4.5

    Searches the string starting at \a str (of length \a length) from
    character position \a from (default 0, i.e. from the first
    character), for the string pattern() that was set in the
    constructor or in the most recent call to setPattern(). Returns
    the position where the pattern() matched in \a str, or -1 if no
    match was found.

    \sa setPattern(), setCaseSensitivity()
*/

/*!
    \since 5.14

    Searches the string \a str from character position \a from
    (default 0, i.e. from the first character), for the string
    pattern() that was set in the constructor or in the most recent
    call to setPattern(). Returns the position where the pattern()
    matched in \a str, or -1 if no match was found.

    \sa setPattern(), setCaseSensitivity()
*/
qsizetype QStringMatcher::indexIn(QStringView str, qsizetype from) const
{
    if (from < 0)
        from = 0;
    return bm_find(str, from, q_sv, q_skiptable, q_cs);
}

/*!
    \fn Qt::CaseSensitivity QStringMatcher::caseSensitivity() const

    Returns the case sensitivity setting for this string matcher.

    \sa setCaseSensitivity()
*/

/*!
    \internal
*/

qsizetype qFindStringBoyerMoore(
    QStringView haystack, qsizetype haystackOffset,
    QStringView needle, Qt::CaseSensitivity cs)
{
    uchar skiptable[256];
    bm_init_skiptable(needle, skiptable, cs);
    if (haystackOffset < 0)
        haystackOffset = 0;
    return bm_find(haystack, haystackOffset, needle, skiptable, cs);
}

QT_END_NAMESPACE
