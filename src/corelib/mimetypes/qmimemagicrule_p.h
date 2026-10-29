// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMIMEMAGICRULE_P_H
#define QMIMEMAGICRULE_P_H

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

#include <QtCore/qbytearray.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qlist.h>
#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#include <QtCore/qstring.h>

#include <optional>
#endif

QT_BEGIN_NAMESPACE

// The data that magic rules are matched against
class QMimeMagicData
{
public:
    explicit QMimeMagicData(const QByteArray &data) : m_data(data) {}
    QMimeMagicData(QByteArray &&) = delete; // would dangle

    const QByteArray &bytes() const { return m_data; }
#if QT_CONFIG(regularexpression)
    // The first \a size bytes, at most bytes().size(), decoded as Latin-1
    QStringView decodedLatin1(qsizetype size) const
    {
        size = qMin(size, m_data.size());
        // Shared by all the regex rules, and only as long as they need: the data can be huge
        if (m_latin1.size() < size)
            m_latin1 = QString::fromLatin1(QByteArrayView(m_data).first(size));
        return QStringView(m_latin1).first(size);
    }
#endif

private:
    const QByteArray &m_data;
#if QT_CONFIG(regularexpression)
    mutable QString m_latin1;
#endif
};

class QMimeMagicRule
{
public:
    // Regex is an extension used by the tika database, not in the shared-mime-info spec
    enum Type { Invalid = 0, String, Host16, Host32, Big16, Big32, Little16, Little32, Byte, Regex };
    // Only Qt's own database may use Regex: elsewhere it wouldn't work with a mime.cache
    enum class RegexRules { Rejected, Allowed };
    // Maximum nesting depth of magic rules that is followed, both for <match>
    // elements in XML and for matchlets in mime.cache. The deepest nesting in
    // the shared-mime-info database is 5, so this leaves ample headroom while
    // keeping recursion bounded for corrupt or hostile input.
    static constexpr int MaxRecursionDepth = 50;

    QMimeMagicRule(const QString &typeStr, const QByteArray &value, const QString &offsets,
                   const QByteArray &mask, RegexRules regexRules, QString *errorString);

    void swap(QMimeMagicRule &other) noexcept
    {
        qSwap(m_type,          other.m_type);
        qSwap(m_value,         other.m_value);
        qSwap(m_startPos,      other.m_startPos);
        qSwap(m_endPos,        other.m_endPos);
        qSwap(m_mask,          other.m_mask);
        qSwap(m_pattern,       other.m_pattern);
        qSwap(m_number,        other.m_number);
        qSwap(m_numberMask,    other.m_numberMask);
#if QT_CONFIG(regularexpression)
        qSwap(m_regExp,        other.m_regExp);
#endif
        qSwap(m_matchFunction, other.m_matchFunction);
    }

    bool operator==(const QMimeMagicRule &other) const;

    Type type() const { return m_type; }
    QByteArray value() const { return m_value; }
    int startPos() const { return m_startPos; }
    int endPos() const { return m_endPos; }
    QByteArray mask() const;

    bool isValid() const { return m_matchFunction != nullptr; }

    bool matches(const QMimeMagicData &data, int depth = MaxRecursionDepth) const;

    QList<QMimeMagicRule> m_subMatches;

    static Type type(const QByteArray &type);
    static QByteArray typeName(Type type);

    static bool matchSubstring(const char *dataPtr, qsizetype dataSize, quint64 rangeStart,
                               quint64 rangeLength, quint64 valueLength, const char *valueData,
                               const char *mask);

private:
    Type m_type;
    QByteArray m_value;
    int m_startPos;
    int m_endPos;
    QByteArray m_mask;

    QByteArray m_pattern;
    quint32 m_number;
    quint32 m_numberMask;
#if QT_CONFIG(regularexpression)
    std::optional<QRegularExpression> m_regExp; // only for Regex rules: it allocates
#endif

    typedef bool (QMimeMagicRule::*MatchFunction)(const QMimeMagicData &data) const;
    MatchFunction m_matchFunction;

private:
    // match functions
    bool matchString(const QMimeMagicData &magicData) const;
    template <typename T>
    bool matchNumber(const QMimeMagicData &magicData) const;
    bool matchRegExp(const QMimeMagicData &magicData) const;
};
Q_DECLARE_SHARED(QMimeMagicRule)

QT_END_NAMESPACE

#endif // QMIMEMAGICRULE_H
