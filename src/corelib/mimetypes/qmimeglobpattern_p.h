/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QMIMEGLOBPATTERN_P_H
#define QMIMEGLOBPATTERN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

QT_REQUIRE_CONFIG(mimetype);

#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

struct QMimeGlobMatchResult
{
    void addMatch(const QString &mimeType, int weight, const QString &pattern, int knownSuffixLength = 0);

    QStringList m_matchingMimeTypes; // only those with highest weight
    QStringList m_allMatchingMimeTypes;
    int m_weight = 0;
    int m_matchingPatternLength = 0;
    int m_knownSuffixLength = 0;
};

class QMimeGlobPattern
{
public:
    static const unsigned MaxWeight = 100;
    static const unsigned DefaultWeight = 50;
    static const unsigned MinWeight = 1;

    explicit QMimeGlobPattern(const QString &thePattern, const QString &theMimeType, unsigned theWeight = DefaultWeight, Qt::CaseSensitivity s = Qt::CaseInsensitive) :
        m_pattern(s == Qt::CaseInsensitive ? thePattern.toLower() : thePattern),
        m_mimeType(theMimeType), m_weight(theWeight), m_caseSensitivity(s)
    {
    }

    void swap(QMimeGlobPattern &other) noexcept
    {
        qSwap(m_pattern,         other.m_pattern);
        qSwap(m_mimeType,        other.m_mimeType);
        qSwap(m_weight,          other.m_weight);
        qSwap(m_caseSensitivity, other.m_caseSensitivity);
    }

    bool matchFileName(const QString &filename) const;

    inline const QString &pattern() const { return m_pattern; }
    inline unsigned weight() const { return m_weight; }
    inline const QString &mimeType() const { return m_mimeType; }
    inline bool isCaseSensitive() const { return m_caseSensitivity == Qt::CaseSensitive; }

private:
    QString m_pattern;
    QString m_mimeType;
    int m_weight;
    Qt::CaseSensitivity m_caseSensitivity;
};
Q_DECLARE_SHARED(QMimeGlobPattern)

class QMimeGlobPatternList : public QList<QMimeGlobPattern>
{
public:
    bool hasPattern(const QString &mimeType, const QString &pattern) const
    {
        const_iterator it = begin();
        const const_iterator myend = end();
        for (; it != myend; ++it)
            if ((*it).pattern() == pattern && (*it).mimeType() == mimeType)
                return true;
        return false;
    }

    /*!
        "noglobs" is very rare occurrence, so it's ok if it's slow
     */
    void removeMimeType(const QString &mimeType)
    {
        auto isMimeTypeEqual = [&mimeType](const QMimeGlobPattern &pattern) {
            return pattern.mimeType() == mimeType;
        };
        erase(std::remove_if(begin(), end(), isMimeTypeEqual), end());
    }

    void match(QMimeGlobMatchResult &result, const QString &fileName) const;
};

/*!
    Result of the globs parsing, as data structures ready for efficient MIME type matching.
    This contains:
    1) a map of fast regular patterns (e.g. *.txt is stored as "txt" in a qhash's key)
    2) a linear list of high-weight globs
    3) a linear list of low-weight globs
 */
class QMimeAllGlobPatterns
{
public:
    typedef QHash<QString, QStringList> PatternsMap; // MIME type -> patterns

    void addGlob(const QMimeGlobPattern &glob);
    void removeMimeType(const QString &mimeType);
    void matchingGlobs(const QString &fileName, QMimeGlobMatchResult &result) const;
    void clear();

    PatternsMap m_fastPatterns; // example: "doc" -> "application/msword", "text/plain"
    QMimeGlobPatternList m_highWeightGlobs;
    QMimeGlobPatternList m_lowWeightGlobs; // <= 50, including the non-fast 50 patterns
};

QT_END_NAMESPACE

#endif // QMIMEGLOBPATTERN_P_H
