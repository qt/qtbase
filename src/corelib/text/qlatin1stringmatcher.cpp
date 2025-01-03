// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlatin1stringmatcher.h"
#include <limits.h>

QT_BEGIN_NAMESPACE

/*! \class QLatin1StringMatcher
    \inmodule QtCore
    \brief Optimized search for substring in Latin-1 text.

    A QLatin1StringMatcher can search for one QLatin1StringView
    as a substring of another, either ignoring case or taking it into
    account.

    \since 6.5
    \ingroup tools
    \ingroup string-processing

    This class is useful when you have a Latin-1 encoded string that
    you want to repeatedly search for in some QLatin1StringViews
    (perhaps in a loop), or when you want to search for all
    instances of it in a given QLatin1StringView. Using a matcher
    object and indexIn() is faster than matching a plain
    QLatin1StringView with QLatin1StringView::indexOf() if repeated
    matching takes place. This class offers no benefit if you are
    doing one-off matches. The string to be searched for must not
    be destroyed or changed before the matcher object is destroyed,
    as the matcher accesses the string when searching for it.

    Create a QLatin1StringMatcher for the QLatin1StringView
    you want to search for and the case sensitivity. Then call
    indexIn() with the QLatin1StringView that you want to search
    within.

    \sa QLatin1StringView, QStringMatcher, QByteArrayMatcher
*/

/*!
    Construct an empty Latin-1 string matcher.
    This will match at each position in any string.
    \sa setPattern(), setCaseSensitivity(), indexIn()
*/
QLatin1StringMatcher::QLatin1StringMatcher() noexcept
    : m_pattern(),
      m_cs(Qt::CaseSensitive),
      m_caseSensitiveSearcher(m_pattern.data(), m_pattern.data())
{
}

/*!
    Constructs a Latin-1 string matcher that searches for the given \a pattern
    with given case sensitivity \a cs. The \a pattern argument must
    not be destroyed before this matcher object. Call indexIn()
    to find the \a pattern in the given QLatin1StringView.
*/
QLatin1StringMatcher::QLatin1StringMatcher(QLatin1StringView pattern,
                                           Qt::CaseSensitivity cs) noexcept
    : m_pattern(pattern), m_cs(cs)
{
    setSearcher();
}

/*!
    Destroys the Latin-1 string matcher.
*/
QLatin1StringMatcher::~QLatin1StringMatcher() noexcept
{
    freeSearcher();
}

/*!
    \internal
*/
void QLatin1StringMatcher::setSearcher() noexcept
{
    if (m_cs == Qt::CaseSensitive) {
        new (&m_caseSensitiveSearcher) CaseSensitiveSearcher(m_pattern.data(), m_pattern.end());
    } else {
        QtPrivate::QCaseInsensitiveLatin1Hash foldCase;
        qsizetype bufferSize = std::min(m_pattern.size(), qsizetype(sizeof m_foldBuffer));
        for (qsizetype i = 0; i < bufferSize; ++i)
            m_foldBuffer[i] = static_cast<char>(foldCase(m_pattern[i].toLatin1()));

        new (&m_caseInsensitiveSearcher)
                CaseInsensitiveSearcher(m_foldBuffer, &m_foldBuffer[bufferSize]);
    }
}

/*!
    \internal
*/
void QLatin1StringMatcher::freeSearcher() noexcept
{
    if (m_cs == Qt::CaseSensitive)
        m_caseSensitiveSearcher.~CaseSensitiveSearcher();
    else
        m_caseInsensitiveSearcher.~CaseInsensitiveSearcher();
}

/*!
    Sets the \a pattern to search for. The string pointed to by the
    QLatin1StringView must not be destroyed before the matcher is
    destroyed, unless it is set to point to a different \a pattern
    with longer lifetime first.

    \sa pattern(), indexIn()
*/
void QLatin1StringMatcher::setPattern(QLatin1StringView pattern) noexcept
{
    if (m_pattern.latin1() == pattern.latin1() && m_pattern.size() == pattern.size())
        return; // Same address and size

    freeSearcher();
    m_pattern = pattern;
    setSearcher();
}

/*!
    Returns the Latin-1 pattern that the matcher searches for.

    \sa setPattern(), indexIn()
*/
QLatin1StringView QLatin1StringMatcher::pattern() const noexcept
{
    return m_pattern;
}

/*!
    Sets the case sensitivity to \a cs.

    \sa caseSensitivity(), indexIn()
*/
void QLatin1StringMatcher::setCaseSensitivity(Qt::CaseSensitivity cs) noexcept
{
    if (m_cs == cs)
        return;

    freeSearcher();
    m_cs = cs;
    setSearcher();
}

/*!
    Returns the case sensitivity the matcher uses.

    \sa setCaseSensitivity(), indexIn()
*/
Qt::CaseSensitivity QLatin1StringMatcher::caseSensitivity() const noexcept
{
    return m_cs;
}

/*!
    Searches for the pattern in the given \a haystack starting from
    \a from.

    \sa caseSensitivity(), pattern()
*/
qsizetype QLatin1StringMatcher::indexIn(QLatin1StringView haystack, qsizetype from) const noexcept
{
    if (m_pattern.isEmpty() && from == haystack.size())
        return from;
    if (from >= haystack.size())
        return -1;
    auto begin = haystack.begin() + from;
    auto end = haystack.end();
    auto found = begin;
    if (m_cs == Qt::CaseSensitive) {
        found = m_caseSensitiveSearcher(begin, end, m_pattern.begin(), m_pattern.end()).begin;
        if (found == end)
            return -1;
    } else {
        const qsizetype bufferSize = std::min(m_pattern.size(), qsizetype(sizeof m_foldBuffer));
        const QLatin1StringView restNeedle = m_pattern.sliced(bufferSize);
        const bool needleLongerThanBuffer = restNeedle.size() > 0;
        QLatin1StringView restHaystack = haystack;
        do {
            found = m_caseInsensitiveSearcher(found, end, m_foldBuffer, &m_foldBuffer[bufferSize])
                            .begin;
            if (found == end) {
                return -1;
            } else if (!needleLongerThanBuffer) {
                break;
            }
            restHaystack = haystack.sliced(
                    qMin(haystack.size(),
                         bufferSize + qsizetype(std::distance(haystack.begin(), found))));
            if (restHaystack.startsWith(restNeedle, Qt::CaseInsensitive))
                break;
            ++found;
        } while (true);
    }
    return std::distance(haystack.begin(), found);
}

QT_END_NAMESPACE
