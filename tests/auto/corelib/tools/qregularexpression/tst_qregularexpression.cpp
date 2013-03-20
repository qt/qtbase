/****************************************************************************
**
** Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
** Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qstring.h>
#include <qlist.h>
#include <qstringlist.h>
#include <qhash.h>

#include "tst_qregularexpression.h"

struct Match
{
    Match()
    {
        clear();
    }

    void clear()
    {
        isValid = false;
        hasMatch = false;
        hasPartialMatch = false;
        captured.clear();
        namedCaptured.clear();
    }

    bool isValid;
    bool hasMatch;
    bool hasPartialMatch;
    QStringList captured;
    QHash<QString, QString> namedCaptured;
};

Q_DECLARE_METATYPE(Match)

bool operator==(const QRegularExpressionMatch &rem, const Match &m)
{
    if (rem.isValid() != m.isValid)
        return false;
    if (!rem.isValid())
        return true;
    if ((rem.hasMatch() != m.hasMatch) || (rem.hasPartialMatch() != m.hasPartialMatch))
        return false;
    if (rem.hasMatch() || rem.hasPartialMatch()) {
        if (rem.lastCapturedIndex() != (m.captured.size() - 1))
            return false;
        for (int i = 0; i <= rem.lastCapturedIndex(); ++i) {
            QString remCaptured = rem.captured(i);
            QString mCaptured = m.captured.at(i);
            if (remCaptured != mCaptured
                || remCaptured.isNull() != mCaptured.isNull()
                || remCaptured.isEmpty() != mCaptured.isEmpty()) {
                return false;
            }
        }

        Q_FOREACH (const QString &name, m.namedCaptured.keys()) {
            QString remCaptured = rem.captured(name);
            QString mCaptured = m.namedCaptured.value(name);
            if (remCaptured != mCaptured
                || remCaptured.isNull() != mCaptured.isNull()
                || remCaptured.isEmpty() != mCaptured.isEmpty()) {
                return false;
            }
        }
    }

    return true;
}

bool operator==(const Match &m, const QRegularExpressionMatch &rem)
{
    return operator==(rem, m);
}

bool operator!=(const QRegularExpressionMatch &rem, const Match &m)
{
    return !operator==(rem, m);
}

bool operator!=(const Match &m, const QRegularExpressionMatch &rem)
{
    return !operator==(m, rem);
}


bool operator==(const QRegularExpressionMatchIterator &iterator, const QList<Match> &expectedMatchList)
{
    QRegularExpressionMatchIterator i = iterator;

    foreach (const Match &expectedMatch, expectedMatchList)
    {
        if (!i.hasNext())
            return false;

        QRegularExpressionMatch match = i.next();
        if (match != expectedMatch)
            return false;
    }

    if (i.hasNext())
        return false;

    return true;
}

bool operator==(const QList<Match> &expectedMatchList, const QRegularExpressionMatchIterator &iterator)
{
    return operator==(iterator, expectedMatchList);
}

bool operator!=(const QRegularExpressionMatchIterator &iterator, const QList<Match> &expectedMatchList)
{
    return !operator==(iterator, expectedMatchList);
}

bool operator!=(const QList<Match> &expectedMatchList, const QRegularExpressionMatchIterator &iterator)
{
    return !operator==(expectedMatchList, iterator);
}

void consistencyCheck(const QRegularExpressionMatch &match)
{
    if (match.isValid()) {
        QVERIFY(match.regularExpression().isValid());
        QVERIFY(!(match.hasMatch() && match.hasPartialMatch()));

        if (match.hasMatch() || match.hasPartialMatch()) {
            QVERIFY(match.lastCapturedIndex() >= 0);
            if (match.hasPartialMatch())
                QVERIFY(match.lastCapturedIndex() == 0);

            for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
                int startPos = match.capturedStart(i);
                int endPos = match.capturedEnd(i);
                int length = match.capturedLength(i);
                QString captured = match.captured(i);
                QStringRef capturedRef = match.capturedRef(i);

                if (!captured.isNull()) {
                    QVERIFY(startPos >= 0);
                    QVERIFY(endPos >= 0);
                    QVERIFY(length >= 0);
                    QVERIFY(endPos >= startPos);
                    QVERIFY((endPos - startPos) == length);
                    QVERIFY(captured == capturedRef);
                } else {
                    QVERIFY(startPos == -1);
                    QVERIFY(endPos == -1);
                    QVERIFY((endPos - startPos) == length);
                    QVERIFY(capturedRef.isNull());
                }
            }
        }
    } else {
        QVERIFY(!match.hasMatch());
        QVERIFY(!match.hasPartialMatch());
        QVERIFY(match.captured(0).isNull());
        QVERIFY(match.capturedStart(0) == -1);
        QVERIFY(match.capturedEnd(0) == -1);
        QVERIFY(match.capturedLength(0) == 0);
    }
}

void consistencyCheck(const QRegularExpressionMatchIterator &iterator)
{
    QRegularExpressionMatchIterator i(iterator); // make a copy, we modify it
    if (i.isValid()) {
        while (i.hasNext()) {
            QRegularExpressionMatch peeked = i.peekNext();
            QRegularExpressionMatch match = i.next();
            consistencyCheck(peeked);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch() || match.hasPartialMatch());
            QCOMPARE(i.regularExpression(), match.regularExpression());
            QCOMPARE(i.matchOptions(), match.matchOptions());
            QCOMPARE(i.matchType(), match.matchType());

            QVERIFY(peeked.isValid() == match.isValid());
            QVERIFY(peeked.hasMatch() == match.hasMatch());
            QVERIFY(peeked.hasPartialMatch() == match.hasPartialMatch());
            QVERIFY(peeked.lastCapturedIndex() == match.lastCapturedIndex());
            for (int i = 0; i <= peeked.lastCapturedIndex(); ++i) {
                QVERIFY(peeked.captured(i) == match.captured(i));
                QVERIFY(peeked.capturedStart(i) == match.capturedStart(i));
                QVERIFY(peeked.capturedEnd(i) == match.capturedEnd(i));
            }
        }
    } else {
        QVERIFY(!i.hasNext());
        QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatchIterator::peekNext() called on an iterator already at end");
        QRegularExpressionMatch peeked = i.peekNext();
        QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatchIterator::next() called on an iterator already at end");
        QRegularExpressionMatch match = i.next();
        consistencyCheck(peeked);
        consistencyCheck(match);
        QVERIFY(!match.isValid());
        QVERIFY(!peeked.isValid());
    }

}

void tst_QRegularExpression::provideRegularExpressions()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QRegularExpression::PatternOptions>("patternOptions");

    QTest::newRow("emptynull01") << QString()
                                 << QRegularExpression::PatternOptions(0);
    QTest::newRow("emptynull02") << QString()
                                 << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                       | QRegularExpression::DotMatchesEverythingOption
                                                                       | QRegularExpression::MultilineOption);
    QTest::newRow("emptynull03") << ""
                                 << QRegularExpression::PatternOptions(0);
    QTest::newRow("emptynull04") << ""
                                 << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                       | QRegularExpression::DotMatchesEverythingOption
                                                                       | QRegularExpression::MultilineOption);

    QTest::newRow("regexp01") << "a pattern"
                              << QRegularExpression::PatternOptions(0);
    QTest::newRow("regexp02") << "^a (.*) more complicated(?<P>pattern)$"
                              << QRegularExpression::PatternOptions(0);
    QTest::newRow("regexp03") << "(?:a) pAttErN"
                              << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("regexp04") << "a\nmultiline\npattern"
                              << QRegularExpression::PatternOptions(QRegularExpression::MultilineOption);
    QTest::newRow("regexp05") << "an extended # IGNOREME\npattern"
                              << QRegularExpression::PatternOptions(QRegularExpression::ExtendedPatternSyntaxOption);
    QTest::newRow("regexp06") << "a [sS]ingleline .* match"
                              << QRegularExpression::PatternOptions(QRegularExpression::DotMatchesEverythingOption);
    QTest::newRow("regexp07") << "multiple.*options"
                              << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                    | QRegularExpression::DotMatchesEverythingOption
                                                                    | QRegularExpression::MultilineOption
                                                                    | QRegularExpression::DontCaptureOption
                                                                    | QRegularExpression::InvertedGreedinessOption);

    QTest::newRow("unicode01") << QString::fromUtf8("^s[ome] latin-1 \xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99 chars$")
                               << QRegularExpression::PatternOptions(0);
    QTest::newRow("unicode02") << QString::fromUtf8("^s[ome] latin-1 \xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99 chars$")
                               << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                     | QRegularExpression::DotMatchesEverythingOption
                                                                     | QRegularExpression::InvertedGreedinessOption);
    QTest::newRow("unicode03") << QString::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f")
                               << QRegularExpression::PatternOptions(0);
    QTest::newRow("unicode04") << QString::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f")
                               << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                     | QRegularExpression::DotMatchesEverythingOption
                                                                     | QRegularExpression::InvertedGreedinessOption);
}

void tst_QRegularExpression::defaultConstructors()
{
    QRegularExpression re;
    QCOMPARE(re.pattern(), QString());
    QCOMPARE(re.patternOptions(), QRegularExpression::NoPatternOption);

    QRegularExpressionMatch match;
    QCOMPARE(match.regularExpression(), QRegularExpression());
    QCOMPARE(match.regularExpression(), re);
    QCOMPARE(match.matchType(), QRegularExpression::NoMatch);
    QCOMPARE(match.matchOptions(), QRegularExpression::NoMatchOption);
    QCOMPARE(match.hasMatch(), false);
    QCOMPARE(match.hasPartialMatch(), false);
    QCOMPARE(match.isValid(), true);
    QCOMPARE(match.lastCapturedIndex(), -1);

    QRegularExpressionMatchIterator iterator;
    QCOMPARE(iterator.regularExpression(), QRegularExpression());
    QCOMPARE(iterator.regularExpression(), re);
    QCOMPARE(iterator.matchType(), QRegularExpression::NoMatch);
    QCOMPARE(iterator.matchOptions(), QRegularExpression::NoMatchOption);
    QCOMPARE(iterator.isValid(), true);
    QCOMPARE(iterator.hasNext(), false);
}

void tst_QRegularExpression::gettersSetters_data()
{
    provideRegularExpressions();
}

void tst_QRegularExpression::gettersSetters()
{
    QFETCH(QString, pattern);
    QFETCH(QRegularExpression::PatternOptions, patternOptions);
    {
        QRegularExpression re;
        re.setPattern(pattern);
        QCOMPARE(re.pattern(), pattern);
        QCOMPARE(re.patternOptions(), QRegularExpression::NoPatternOption);
    }
    {
        QRegularExpression re;
        re.setPatternOptions(patternOptions);
        QCOMPARE(re.pattern(), QString());
        QCOMPARE(re.patternOptions(), patternOptions);
    }
    {
        QRegularExpression re(pattern);
        QCOMPARE(re.pattern(), pattern);
        QCOMPARE(re.patternOptions(), QRegularExpression::NoPatternOption);
    }
    {
        QRegularExpression re(pattern, patternOptions);
        QCOMPARE(re.pattern(), pattern);
        QCOMPARE(re.patternOptions(), patternOptions);
    }
}

void tst_QRegularExpression::escape_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("escaped");
    QTest::newRow("escape01") << "a normal pattern"
                              << "a\\ normal\\ pattern";

    QTest::newRow("escape02") << "abcdefghijklmnopqrstuvzABCDEFGHIJKLMNOPQRSTUVZ1234567890_"
                              << "abcdefghijklmnopqrstuvzABCDEFGHIJKLMNOPQRSTUVZ1234567890_";

    QTest::newRow("escape03") << "^\\ba\\b.*(?<NAME>reg|exp)$"
                              << "\\^\\\\ba\\\\b\\.\\*\\(\\?\\<NAME\\>reg\\|exp\\)\\$";

    QString nulString("abcXabcXXabc");
    nulString[3] = nulString[7] = nulString[8] = QChar(0, 0);
    QTest::newRow("NUL") << nulString
                         << "abc\\0abc\\0\\0abc";

    QTest::newRow("unicode01") << QString::fromUtf8("^s[ome] latin-1 \xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99 chars$")
                               << QString::fromUtf8("\\^s\\[ome\\]\\ latin\\-1\\ \\\xc3\x80\\\xc3\x88\\\xc3\x8c\\\xc3\x92\\\xc3\x99\\ chars\\$");
    QTest::newRow("unicode02") << QString::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f")
                               << QString::fromUtf8("Unicode\\ \\\xf0\x9d\x85\x9d\\ \\\xf0\x9d\x85\x9e\\\xf0\x9d\x85\x9f");

    QString unicodeAndNulString = QString::fromUtf8("^\xc3\x80\xc3\x88\xc3\x8cN\xc3\x92NN\xc3\x99 chars$");
    unicodeAndNulString[4] = unicodeAndNulString[6] = unicodeAndNulString[7] = QChar(0, 0);
    QTest::newRow("unicode03") << unicodeAndNulString
                               << QString::fromUtf8("\\^\\\xc3\x80\\\xc3\x88\\\xc3\x8c\\0\\\xc3\x92\\0\\0\\\xc3\x99\\ chars\\$");
}

void tst_QRegularExpression::escape()
{
    QFETCH(QString, string);
    QFETCH(QString, escaped);
    QCOMPARE(QRegularExpression::escape(string), escaped);
    QRegularExpression re(escaped);
    QCOMPARE(re.isValid(), true);
}

void tst_QRegularExpression::validity_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("validity");

    QTest::newRow("valid01") << "a pattern" << true;
    QTest::newRow("valid02") << "(a|pattern)" << true;
    QTest::newRow("valid03") << "a [pP]attern" << true;
    QTest::newRow("valid04") << "^(?<article>a).*(?<noun>pattern)$" << true;
    QTest::newRow("valid05") << "a \\P{Ll}attern" << true;

    QTest::newRow("invalid01") << "a pattern\\" << false;
    QTest::newRow("invalid02") << "(a|pattern" << false;
    QTest::newRow("invalid03") << "a \\P{BLAH}attern" << false;

    QString pattern;
    // 0xD800 (high surrogate) not followed by a low surrogate
    pattern = "abcdef";
    pattern[3] = QChar(0x00, 0xD8);
    QTest::newRow("invalidUnicode01") << pattern << false;
}

void tst_QRegularExpression::validity()
{
    QFETCH(QString, pattern);
    QFETCH(bool, validity);
    QRegularExpression re(pattern);
    QCOMPARE(re.isValid(), validity);
    if (!validity)
        QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionPrivate::doMatch(): called on an invalid QRegularExpression object");
    QRegularExpressionMatch match = re.match("a pattern");
    QCOMPARE(match.isValid(), validity);
    consistencyCheck(match);

    if (!validity)
        QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionPrivate::doMatch(): called on an invalid QRegularExpression object");
    QRegularExpressionMatchIterator iterator = re.globalMatch("a pattern");
    QCOMPARE(iterator.isValid(), validity);
}

void tst_QRegularExpression::patternOptions_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<Match>("match");

    // none of these would successfully match if the respective
    // pattern option is not set

    Match m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString::fromUtf8("AbC\xc3\xa0");
    QTest::newRow("/i") << QRegularExpression(QString::fromUtf8("abc\xc3\x80"), QRegularExpression::CaseInsensitiveOption)
                        << QString::fromUtf8("AbC\xc3\xa0")
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "abc123\n678def";
    QTest::newRow("/s") << QRegularExpression("\\Aabc.*def\\z", QRegularExpression::DotMatchesEverythingOption)
                        << "abc123\n678def"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "jumped over";
    QTest::newRow("/m") << QRegularExpression("^\\w+ \\w+$", QRegularExpression::MultilineOption)
                        << "the quick fox\njumped over\nthe lazy\ndog"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "abc 123456";
    QTest::newRow("/x") << QRegularExpression("\\w+  # a word\n"
                                              "\\ # a space\n"
                                              "\\w+ # another word",
                                              QRegularExpression::ExtendedPatternSyntaxOption)
                        << "abc 123456 def"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "the quick fox" << "the" << "quick fox";
    QTest::newRow("/U") << QRegularExpression("(.+) (.+?)", QRegularExpression::InvertedGreedinessOption)
                        << "the quick fox"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "the quick fox" << "quick";
    m.namedCaptured["named"] = "quick";
    QTest::newRow("no cap") << QRegularExpression("(\\w+) (?<named>\\w+) (\\w+)", QRegularExpression::DontCaptureOption)
                            << "the quick fox"
                            << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString::fromUtf8("abc\xc3\x80\xc3\xa0 12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98")
               << QString::fromUtf8("abc\xc3\x80\xc3\xa0")
               << QString::fromUtf8("12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98");
    QTest::newRow("unicode properties") << QRegularExpression("(\\w+) (\\d+)", QRegularExpression::UseUnicodePropertiesOption)
                            << QString::fromUtf8("abc\xc3\x80\xc3\xa0 12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98")
                            << m;
}

void tst_QRegularExpression::patternOptions()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(Match, match);

    QRegularExpressionMatch m = regexp.match(subject);
    consistencyCheck(m);
    QVERIFY(m == match);
}

void tst_QRegularExpression::normalMatch_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<int>("offset");
    QTest::addColumn<QRegularExpression::MatchOptions>("matchOptions");
    QTest::addColumn<Match>("match");

    Match m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "string" << "string";
    QTest::newRow("match01") << QRegularExpression("(\\bstring\\b)")
                             << "a string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "a string" << "a" << "string";
    QTest::newRow("match02") << QRegularExpression("(\\w+) (\\w+)")
                             << "a string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "a string" << "a" << "string";
    m.namedCaptured["article"] = "a";
    m.namedCaptured["noun"] = "string";
    QTest::newRow("match03") << QRegularExpression("(?<article>\\w+) (?<noun>\\w+)")
                             << "a string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << " string" << QString() << "string";
    QTest::newRow("match04") << QRegularExpression("(\\w+)? (\\w+)")
                             << " string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << " string" << QString("") << "string";
    QTest::newRow("match05") << QRegularExpression("(\\w*) (\\w+)")
                             << " string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "c123def" << "c12" << "3" << "def";
    QTest::newRow("match06") << QRegularExpression("(\\w*)(\\d+)(\\w*)")
                             << "abc123def"
                             << 2
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString("");
    QTest::newRow("match07") << QRegularExpression("\\w*")
                             << "abc123def"
                             << 9
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString("a string") << QString("a string") << QString("");
    QTest::newRow("match08") << QRegularExpression("(.*)(.*)")
                             << "a string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString("a string") << QString("") << QString("a string");
    QTest::newRow("match09") << QRegularExpression("(.*?)(.*)")
                             << "a string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    // non existing names for capturing groups
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "a string" << "a" << "string";
    m.namedCaptured["article"] = "a";
    m.namedCaptured["noun"] = "string";
    m.namedCaptured["nonexisting1"] = QString();
    m.namedCaptured["nonexisting2"] = QString();
    m.namedCaptured["nonexisting3"] = QString();
    QTest::newRow("match10") << QRegularExpression("(?<article>\\w+) (?<noun>\\w+)")
                             << "a string"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "" << "";
    m.namedCaptured["digits"] = ""; // empty VS null
    m.namedCaptured["nonexisting"] = QString();
    QTest::newRow("match11") << QRegularExpression("(?<digits>\\d*)")
                             << "abcde"
                             << 0
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    // ***

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch01") << QRegularExpression("\\d+")
                               << "a string"
                               << 0
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch02") << QRegularExpression("(\\w+) (\\w+)")
                               << "a string"
                               << 1
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch03") << QRegularExpression("\\w+")
                               << "abc123def"
                               << 9
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    // ***

    m.clear();
    m.isValid = true;
    QTest::newRow("anchoredmatch01") << QRegularExpression("\\d+")
                                     << "abc123def"
                                     << 0
                                     << QRegularExpression::MatchOptions(QRegularExpression::AnchoredMatchOption)
                                     << m;

    // ***

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "678";
    QTest::newRow("negativeoffset01") << QRegularExpression("\\d+")
                                      << "abc123def678ghi"
                                      << -6
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "678";
    QTest::newRow("negativeoffset02") << QRegularExpression("\\d+")
                                      << "abc123def678ghi"
                                      << -8
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "678ghi" << "678" << "ghi";
    QTest::newRow("negativeoffset03") << QRegularExpression("(\\d+)(\\w+)")
                                      << "abc123def678ghi"
                                      << -8
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("negativeoffset04") << QRegularExpression("\\d+")
                                      << "abc123def678ghi"
                                      << -3
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true; m.hasMatch =  true;
    m.captured << "678";
    QTest::newRow("negativeoffset05") << QRegularExpression("^\\d+", QRegularExpression::MultilineOption)
                                      << "a\nbc123\ndef\n678gh\ni"
                                      << -10
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;
}


void tst_QRegularExpression::normalMatch()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(int, offset);
    QFETCH(QRegularExpression::MatchOptions, matchOptions);
    QFETCH(Match, match);

    {
        QRegularExpressionMatch m = regexp.match(subject, offset, QRegularExpression::NormalMatch, matchOptions);
        consistencyCheck(m);
        QVERIFY(m == match);
        QCOMPARE(m.regularExpression(), regexp);
        QCOMPARE(m.matchType(), QRegularExpression::NormalMatch);
        QCOMPARE(m.matchOptions(), matchOptions);
    }
    {
        // ignore the expected results provided by the match object --
        // we'll never get any result when testing the NoMatch type.
        // Just check the validity of the match here.
        Match realMatch;
        realMatch.clear();
        realMatch.isValid = match.isValid;

        QRegularExpressionMatch m = regexp.match(subject, offset, QRegularExpression::NoMatch, matchOptions);
        consistencyCheck(m);
        QVERIFY(m == realMatch);
        QCOMPARE(m.regularExpression(), regexp);
        QCOMPARE(m.matchType(), QRegularExpression::NoMatch);
        QCOMPARE(m.matchOptions(), matchOptions);
    }
}

void tst_QRegularExpression::partialMatch_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<int>("offset");
    QTest::addColumn<QRegularExpression::MatchType>("matchType");
    QTest::addColumn<QRegularExpression::MatchOptions>("matchOptions");
    QTest::addColumn<Match>("match");

    Match m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "str";
    QTest::newRow("softmatch01") << QRegularExpression("string")
                                    << "a str"
                                    << 0
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("softmatch02") << QRegularExpression("\\bstring\\b")
                                    << "a str"
                                    << 0
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("softmatch03") << QRegularExpression("(\\bstring\\b)")
                                    << "a str"
                                    << 0
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "8 Dec 19";
    QTest::newRow("softmatch04") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 19"
                                    << 0
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "8 Dec 1985" << "8" << "Dec" << "1985";
    QTest::newRow("softmatch05") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 1985"
                                    << 0
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "def";
    QTest::newRow("softmatch06") << QRegularExpression("abc\\w+X|def")
                                    << "abcdef"
                                    << 0
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abcdef";
    QTest::newRow("softmatch07") << QRegularExpression("abc\\w+X|defY")
                                    << "abcdef"
                                    << 0
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "def";
    QTest::newRow("softmatch08") << QRegularExpression("abc\\w+X|defY")
                                    << "abcdef"
                                    << 1
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    // ***

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "str";
    QTest::newRow("hardmatch01") << QRegularExpression("string")
                                    << "a str"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("hardmatch02") << QRegularExpression("\\bstring\\b")
                                    << "a str"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("hardmatch03") << QRegularExpression("(\\bstring\\b)")
                                    << "a str"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "8 Dec 19";
    QTest::newRow("hardmatch04") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 19"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "8 Dec 1985";
    QTest::newRow("hardmatch05") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 1985"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abcdef";
    QTest::newRow("hardmatch06") << QRegularExpression("abc\\w+X|def")
                                    << "abcdef"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abcdef";
    QTest::newRow("hardmatch07") << QRegularExpression("abc\\w+X|defY")
                                    << "abcdef"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "def";
    QTest::newRow("hardmatch08") << QRegularExpression("abc\\w+X|defY")
                                    << "abcdef"
                                    << 1
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "ab";
    QTest::newRow("hardmatch09") << QRegularExpression("abc|ab")
                                    << "ab"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abc";
    QTest::newRow("hardmatch10") << QRegularExpression("abc(def)?")
                                    << "abc"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abc";
    QTest::newRow("hardmatch11") << QRegularExpression("(abc)*")
                                    << "abc"
                                    << 0
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;


    // ***

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch01") << QRegularExpression("abc\\w+X|defY")
                               << "123456"
                               << 0
                               << QRegularExpression::PartialPreferCompleteMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch02") << QRegularExpression("abc\\w+X|defY")
                               << "123456"
                               << 0
                               << QRegularExpression::PartialPreferFirstMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch03") << QRegularExpression("abc\\w+X|defY")
                               << "ab123"
                               << 0
                               << QRegularExpression::PartialPreferCompleteMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch04") << QRegularExpression("abc\\w+X|defY")
                               << "ab123"
                               << 0
                               << QRegularExpression::PartialPreferFirstMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

}

void tst_QRegularExpression::partialMatch()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(int, offset);
    QFETCH(QRegularExpression::MatchType, matchType);
    QFETCH(QRegularExpression::MatchOptions, matchOptions);
    QFETCH(Match, match);

    {
        QRegularExpressionMatch m = regexp.match(subject, offset, matchType, matchOptions);
        consistencyCheck(m);
        QVERIFY(m == match);
        QCOMPARE(m.regularExpression(), regexp);
        QCOMPARE(m.matchType(), matchType);
        QCOMPARE(m.matchOptions(), matchOptions);
    }
    {
        // ignore the expected results provided by the match object --
        // we'll never get any result when testing the NoMatch type.
        // Just check the validity of the match here.
        Match realMatch;
        realMatch.clear();
        realMatch.isValid = match.isValid;

        QRegularExpressionMatch m = regexp.match(subject, offset, QRegularExpression::NoMatch, matchOptions);
        consistencyCheck(m);
        QVERIFY(m == realMatch);
        QCOMPARE(m.regularExpression(), regexp);
        QCOMPARE(m.matchType(), QRegularExpression::NoMatch);
        QCOMPARE(m.matchOptions(), matchOptions);
    }
}

void tst_QRegularExpression::globalMatch_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<int>("offset");
    QTest::addColumn<QRegularExpression::MatchType>("matchType");
    QTest::addColumn<QRegularExpression::MatchOptions>("matchOptions");
    QTest::addColumn<QList<Match> >("matchList");

    QList<Match> matchList;
    Match m;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "the";
    matchList << m;
    m.captured = QStringList() << "quick";
    matchList << m;
    m.captured = QStringList() << "fox";
    matchList << m;
    QTest::newRow("globalmatch01") << QRegularExpression("\\w+")
                                   << "the quick fox"
                                   << 0
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                   << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "the" << "t" << "he";
    matchList << m;
    m.captured = QStringList() << "quick" << "q" << "uick";
    matchList << m;
    m.captured = QStringList() << "fox" << "f" << "ox";
    matchList << m;
    QTest::newRow("globalmatch02") << QRegularExpression("(\\w+?)(\\w+)")
                                   << "the quick fox"
                                   << 0
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                   << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "ACA""GTG""CGA""AAA";
    matchList << m;
    m.captured = QStringList() << "AAA";
    matchList << m;
    m.captured = QStringList() << "AAG""GAA""AAG""AAA";
    matchList << m;
    m.captured = QStringList() << "AAA";
    matchList << m;
    QTest::newRow("globalmatch03") << QRegularExpression("\\G(?:\\w\\w\\w)*?AAA")
                                   << "ACA""GTG""CGA""AAA""AAA""AAG""GAA""AAG""AAA""AAA"
                                   << 0
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                   << matchList;

    QTest::newRow("globalmatch04") << QRegularExpression("(?:\\w\\w\\w)*?AAA")
                                   << "ACA""GTG""CGA""AAA""AAA""AAG""GAA""AAG""AAA""AAA"
                                   << 0
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::AnchoredMatchOption)
                                   << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "c";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "c";
    matchList << m;
    m.captured = QStringList() << "aabb";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures01") << QRegularExpression("a*b*|c")
                                                 << "ccaabbd"
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "the";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "quick";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "fox";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures02") << QRegularExpression(".*")
                                                 << "the\nquick\nfox"
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "the";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "quick";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "fox";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures03") << QRegularExpression(".*")
                                                 << "the\nquick\nfox\n"
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "the";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "quick";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "fox";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures04") << QRegularExpression("(*CRLF).*")
                                                 << "the\r\nquick\r\nfox"
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "the";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "quick";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "fox";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures05") << QRegularExpression("(*CRLF).*")
                                                 << "the\r\nquick\r\nfox\r\n"
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "the";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "quick";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "fox";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "jumped";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures06") << QRegularExpression("(*ANYCRLF).*")
                                                 << "the\r\nquick\nfox\rjumped"
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << "ABC";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "DEF";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << "GHI";
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    QTest::newRow("globalmatch_emptycaptures07") << QRegularExpression("[\\x{0000}-\\x{FFFF}]*")
                                                 << QString::fromUtf8("ABC""\xf0\x9d\x85\x9d""DEF""\xf0\x9d\x85\x9e""GHI")
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = QStringList() << QString::fromUtf8("ABC""\xc3\x80");
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    m.captured = QStringList() << QString::fromUtf8("\xc3\x80""DEF""\xc3\x80");
    matchList << m;
    m.captured = QStringList() << "";
    matchList << m;
    QTest::newRow("globalmatch_emptycaptures08") << QRegularExpression("[\\x{0000}-\\x{FFFF}]*")
                                                 << QString::fromUtf8("ABC""\xc3\x80""\xf0\x9d\x85\x9d""\xc3\x80""DEF""\xc3\x80")
                                                 << 0
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;
}

void tst_QRegularExpression::globalMatch()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(int, offset);
    QFETCH(QRegularExpression::MatchType, matchType);
    QFETCH(QRegularExpression::MatchOptions, matchOptions);
    QFETCH(QList<Match>, matchList);
    {
        QRegularExpressionMatchIterator iterator = regexp.globalMatch(subject, offset, matchType, matchOptions);
        consistencyCheck(iterator);
        QVERIFY(iterator == matchList);
        QCOMPARE(iterator.regularExpression(), regexp);
        QCOMPARE(iterator.matchType(), matchType);
        QCOMPARE(iterator.matchOptions(), matchOptions);
    }
    {
        // ignore the expected results provided by the match object --
        // we'll never get any result when testing the NoMatch type.
        // Just check the validity of the match here.
        QList<Match> realMatchList;

        QRegularExpressionMatchIterator iterator = regexp.globalMatch(subject, offset, QRegularExpression::NoMatch, matchOptions);
        consistencyCheck(iterator);
        QVERIFY(iterator == realMatchList);
        QCOMPARE(iterator.regularExpression(), regexp);
        QCOMPARE(iterator.matchType(), QRegularExpression::NoMatch);
        QCOMPARE(iterator.matchOptions(), matchOptions);
    }

}

void tst_QRegularExpression::serialize_data()
{
    provideRegularExpressions();
}

void tst_QRegularExpression::serialize()
{
    QFETCH(QString, pattern);
    QFETCH(QRegularExpression::PatternOptions, patternOptions);
    QRegularExpression outRe(pattern, patternOptions);
    QByteArray buffer;
    {
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << outRe;
    }
    QRegularExpression inRe;
    {
        QDataStream in(&buffer, QIODevice::ReadOnly);
        in >> inRe;
    }
    QCOMPARE(inRe, outRe);
}

static void verifyEquality(const QRegularExpression &re1, const QRegularExpression &re2)
{
    QVERIFY(re1 == re2);
    QVERIFY(re2 == re1);
    QVERIFY(!(re1 != re2));
    QVERIFY(!(re2 != re1));

    QRegularExpression re3(re1);

    QVERIFY(re1 == re3);
    QVERIFY(re3 == re1);
    QVERIFY(!(re1 != re3));
    QVERIFY(!(re3 != re1));

    QVERIFY(re2 == re3);
    QVERIFY(re3 == re2);
    QVERIFY(!(re2 != re3));
    QVERIFY(!(re3 != re2));

    re3 = re2;
    QVERIFY(re1 == re3);
    QVERIFY(re3 == re1);
    QVERIFY(!(re1 != re3));
    QVERIFY(!(re3 != re1));

    QVERIFY(re2 == re3);
    QVERIFY(re3 == re2);
    QVERIFY(!(re2 != re3));
    QVERIFY(!(re3 != re2));
}

void tst_QRegularExpression::operatoreq_data()
{
    provideRegularExpressions();
}

void tst_QRegularExpression::operatoreq()
{
    QFETCH(QString, pattern);
    QFETCH(QRegularExpression::PatternOptions, patternOptions);
    {
        QRegularExpression re1(pattern);
        QRegularExpression re2(pattern);
        verifyEquality(re1, re2);
    }
    {
        QRegularExpression re1(QString(), patternOptions);
        QRegularExpression re2(QString(), patternOptions);
        verifyEquality(re1, re2);
    }
    {
        QRegularExpression re1(pattern, patternOptions);
        QRegularExpression re2(pattern, patternOptions);
        verifyEquality(re1, re2);
    }
}

void tst_QRegularExpression::captureCount_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<int>("captureCount");
    QTest::newRow("captureCount01") << "a pattern" << 0;
    QTest::newRow("captureCount02") << "a.*pattern" << 0;
    QTest::newRow("captureCount03") << "(a) pattern" << 1;
    QTest::newRow("captureCount04") << "(a).*(pattern)" << 2;
    QTest::newRow("captureCount05") << "^(?<article>\\w+) (?<noun>\\w+)$" << 2;
    QTest::newRow("captureCount06") << "^(\\w+) (?<word>\\w+) (.)$" << 3;
    QTest::newRow("captureCount07") << "(?:non capturing) (capturing) (?<n>named) (?:non (capturing))" << 3;
    QTest::newRow("captureCount08") << "(?|(a)(b)|(c)(d))" << 2;
    QTest::newRow("captureCount09") << "(?|(a)(b)|(c)(d)(?:e))" << 2;
    QTest::newRow("captureCount10") << "(?|(a)(b)|(c)(d)(e)) (f)(g)" << 5;
    QTest::newRow("captureCount11") << "(?|(a)(b)|(c)(d)(e)) (f)(?:g)" << 4;
    QTest::newRow("captureCount_invalid01") << "(.*" << -1;
    QTest::newRow("captureCount_invalid02") << "\\" << -1;
    QTest::newRow("captureCount_invalid03") << "(?<noun)" << -1;
}

void tst_QRegularExpression::captureCount()
{
    QFETCH(QString, pattern);
    QRegularExpression re(pattern);
    QTEST(re.captureCount(), "captureCount");
    if (!re.isValid())
        QCOMPARE(re.captureCount(), -1);
}

// the comma in the template breaks QFETCH...
typedef QMultiHash<QString, int> StringToIntMap;
Q_DECLARE_METATYPE(StringToIntMap)

void tst_QRegularExpression::captureNames_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<StringToIntMap>("namedCapturesIndexMap");
    StringToIntMap map;

    QTest::newRow("captureNames01") << "a pattern" << map;
    QTest::newRow("captureNames02") << "a.*pattern" << map;
    QTest::newRow("captureNames03") << "(a) pattern" << map;
    QTest::newRow("captureNames04") << "(a).*(pattern)" << map;

    map.clear();
    map.replace("named", 1);
    QTest::newRow("captureNames05") << "a.*(?<named>pattern)" << map;

    map.clear();
    map.replace("named", 2);
    QTest::newRow("captureNames06") << "(a).*(?<named>pattern)" << map;

    map.clear();
    map.replace("name1", 1);
    map.replace("name2", 2);
    QTest::newRow("captureNames07") << "(?<name1>a).*(?<name2>pattern)" << map;

    map.clear();
    map.replace("name1", 2);
    map.replace("name2", 1);
    QTest::newRow("captureNames08") << "(?<name2>a).*(?<name1>pattern)" << map;

    map.clear();
    map.replace("date", 1);
    map.replace("month", 2);
    map.replace("year", 3);
    QTest::newRow("captureNames09") << "^(?<date>\\d\\d)/(?<month>\\d\\d)/(?<year>\\d\\d\\d\\d)$" << map;

    map.clear();
    map.replace("date", 2);
    map.replace("month", 1);
    map.replace("year", 3);
    QTest::newRow("captureNames10") << "^(?<month>\\d\\d)/(?<date>\\d\\d)/(?<year>\\d\\d\\d\\d)$" << map;

    map.clear();
    map.replace("noun", 2);
    QTest::newRow("captureNames11") << "(a)(?|(?<noun>b)|(?<noun>c))(d)" << map;

    map.clear();
    QTest::newRow("captureNames_invalid01") << "(.*" << map;
    QTest::newRow("captureNames_invalid02") << "\\" << map;
    QTest::newRow("captureNames_invalid03") << "(?<noun)" << map;
    QTest::newRow("captureNames_invalid04") << "(?|(?<noun1>a)|(?<noun2>b))" << map;
}

void tst_QRegularExpression::captureNames()
{
    QFETCH(QString, pattern);
    QFETCH(StringToIntMap, namedCapturesIndexMap);

    const QRegularExpression re(pattern);
    QStringList namedCaptureGroups = re.namedCaptureGroups();
    int namedCaptureGroupsCount = namedCaptureGroups.size();

    QCOMPARE(namedCaptureGroupsCount, re.captureCount() + 1);

    for (int i = 0; i < namedCaptureGroupsCount; ++i) {
        const QString &name = namedCaptureGroups.at(i);

        if (name.isEmpty()) {
            QVERIFY(!namedCapturesIndexMap.contains(name));
        } else {
            QVERIFY(namedCapturesIndexMap.contains(name));
            QCOMPARE(i, namedCapturesIndexMap.value(name));
        }
    }

}

void tst_QRegularExpression::pcreJitStackUsage_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("subject");
    // these patterns cause enough backtrack (or even infinite recursion)
    // in the regexp engine, so that JIT requests more memory.
    QTest::newRow("jitstack01") << "(?(R)a*(?1)|((?R))b)" << "aaaabcde";
    QTest::newRow("jitstack02") << "(?(R)a*(?1)|((?R))b)" << "aaaaaaabcde";
}

void tst_QRegularExpression::pcreJitStackUsage()
{
    QFETCH(QString, pattern);
    QFETCH(QString, subject);

    QRegularExpression re(pattern);
    QVERIFY(re.isValid());
    QRegularExpressionMatch match = re.match(subject);
    consistencyCheck(match);
    QRegularExpressionMatchIterator iterator = re.globalMatch(subject);
    consistencyCheck(iterator);
    while (iterator.hasNext()) {
        match = iterator.next();
        consistencyCheck(match);
    }
}

void tst_QRegularExpression::regularExpressionMatch_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("subject");

    QTest::newRow("validity01") << "(?<digits>\\d+)" << "1234 abcd";
    QTest::newRow("validity02") << "(?<digits>\\d+) (?<alpha>\\w+)" << "1234 abcd";
}

void tst_QRegularExpression::regularExpressionMatch()
{
    QFETCH(QString, pattern);
    QFETCH(QString, subject);

    QRegularExpression re(pattern);
    QVERIFY(re.isValid());
    QRegularExpressionMatch match = re.match(subject);
    consistencyCheck(match);
    QCOMPARE(match.captured("non-existing").isNull(), true);
    QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatch::captured: empty capturing group name passed");
    QCOMPARE(match.captured("").isNull(), true);
    QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatch::captured: empty capturing group name passed");
    QCOMPARE(match.captured(QString()).isNull(), true);
}

void tst_QRegularExpression::JOptionUsage_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("JOptionUsed");

    QTest::newRow("joption-notused-01") << "a.*b" << true << false;
    QTest::newRow("joption-notused-02") << "^a(b)(c)$" << true << false;
    QTest::newRow("joption-notused-03") << "a(b)(?<c>d)|e" << true << false;
    QTest::newRow("joption-notused-04") << "(?<a>.)(?<a>.)" << false << false;

    QTest::newRow("joption-used-01") << "(?J)a.*b" << true << true;
    QTest::newRow("joption-used-02") << "(?-J)a.*b" << true << true;
    QTest::newRow("joption-used-03") << "(?J)(?<a>.)(?<a>.)" << true << true;
    QTest::newRow("joption-used-04") << "(?-J)(?<a>.)(?<a>.)" << false << true;

}

void tst_QRegularExpression::JOptionUsage()
{
    QFETCH(QString, pattern);
    QFETCH(bool, isValid);
    QFETCH(bool, JOptionUsed);

    const QString warningMessage = QStringLiteral("QRegularExpressionPrivate::getPatternInfo(): the pattern '%1'\n    is using the (?J) option; duplicate capturing group names are not supported by Qt");

    QRegularExpression re(pattern);
    if (isValid && JOptionUsed)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warningMessage.arg(pattern)));
    QCOMPARE(re.isValid(), isValid);
}
