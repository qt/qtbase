/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <qtextboundaryfinder.h>
#include <qtextcodec.h>
#include <qfile.h>
#include <qdebug.h>
#include <qlist.h>

class tst_QTextBoundaryFinder : public QObject
{
    Q_OBJECT
public slots:
    void init();
private slots:
#ifdef QT_BUILD_INTERNAL
    void graphemeBoundariesDefault_data();
    void graphemeBoundariesDefault();
    void wordBoundariesDefault_data();
    void wordBoundariesDefault();
    void sentenceBoundariesDefault_data();
    void sentenceBoundariesDefault();
    void lineBoundariesDefault_data();
    void lineBoundariesDefault();
#endif

    void wordBoundaries_manual_data();
    void wordBoundaries_manual();
    void sentenceBoundaries_manual_data();
    void sentenceBoundaries_manual();
    void lineBoundaries_manual_data();
    void lineBoundaries_manual();

    void fastConstructor();
    void assignmentOperator();
    void wordBoundaries_qtbug6498();
    void isAtSoftHyphen_data();
    void isAtSoftHyphen();
    void isAtMandatoryBreak_data();
    void isAtMandatoryBreak();
    void thaiLineBreak();
};

void tst_QTextBoundaryFinder::init()
{
#ifndef Q_OS_IRIX
    // chdir into the top-level data dir, then refer to our testdata using relative paths
    QString testdata_dir = QFileInfo(QFINDTESTDATA("data")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
#endif
}

Q_DECLARE_METATYPE(QList<int>)

QT_BEGIN_NAMESPACE
namespace QTest {

template<>
inline char *toString(const QTextBoundaryFinder::BoundaryReasons &flags)
{
    return qstrdup(QByteArray::number(int(flags)).constData());
}

template<>
inline char *toString(const QList<int> &list)
{
    QByteArray s;
    for (QList<int>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it) {
        if (!s.isEmpty())
            s += ", ";
        s += QByteArray::number(*it);
    }
    s = "{ " + s + " }";
    return qstrdup(s.constData());
}

} // namespace QTest
QT_END_NAMESPACE

#ifdef QT_BUILD_INTERNAL
static void generateDataFromFile(const QString &fname)
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    QString testFile = QFINDTESTDATA(fname);
    QVERIFY2(!testFile.isEmpty(), (fname.toLatin1() + QByteArray(" not found!")));
    QFile f(testFile);
    QVERIFY(f.exists());

    f.open(QIODevice::ReadOnly);

    int linenum = 0;
    while (!f.atEnd()) {
        linenum++;

        QByteArray line = f.readLine();
        if (line.startsWith('#'))
            continue;

        QString test = QString::fromUtf8(line);
        QString comments;
        int hash = test.indexOf('#');
        if (hash > 0) {
            comments = test.mid(hash + 1).simplified();
            test = test.left(hash);
        }

        QString testString;
        QList<int> expectedBreakPositions;
        foreach (const QString &part, test.simplified().split(QLatin1Char(' '), QString::SkipEmptyParts)) {
            if (part.size() == 1) {
                if (part.at(0).unicode() == 0xf7)
                    expectedBreakPositions.append(testString.size());
                else
                    QVERIFY(part.at(0).unicode() == 0xd7);
                continue;
            }
            bool ok = true;
            uint ucs4 = part.toInt(&ok, 16);
            QVERIFY(ok && ucs4 > 0);
            if (QChar::requiresSurrogates(ucs4)) {
                testString.append(QChar::highSurrogate(ucs4));
                testString.append(QChar::lowSurrogate(ucs4));
            } else {
                testString.append(QChar(ucs4));
            }
        }
        QVERIFY(!testString.isEmpty());
        QVERIFY(!expectedBreakPositions.isEmpty());

        if (!comments.isEmpty()) {
            const QStringList lst = comments.simplified().split(QLatin1Char(' '), QString::SkipEmptyParts);
            comments.clear();
            foreach (const QString &part, lst) {
                if (part.size() == 1) {
                    if (part.at(0).unicode() == 0xf7)
                        comments += QLatin1Char('+');
                    else if (part.at(0).unicode() == 0xd7)
                        comments += QLatin1Char('x');
                    continue;
                }
                if (part.startsWith(QLatin1Char('(')) && part.endsWith(QLatin1Char(')')))
                    comments += part;
            }
        }

        QString nm = QString("line #%1: %2").arg(linenum).arg(comments);
        QTest::newRow(nm.toLatin1()) << testString << expectedBreakPositions;
    }
}

QT_BEGIN_NAMESPACE
extern Q_AUTOTEST_EXPORT int qt_initcharattributes_default_algorithm_only;
QT_END_NAMESPACE
#endif

static void doTestData(const QString &testString, const QList<int> &expectedBreakPositions,
                       QTextBoundaryFinder::BoundaryType type, bool default_algorithm_only = false)
{
#ifdef QT_BUILD_INTERNAL
    QScopedValueRollback<int> default_algorithm(qt_initcharattributes_default_algorithm_only);
    if (default_algorithm_only)
        qt_initcharattributes_default_algorithm_only++;
#else
    Q_UNUSED(default_algorithm_only)
#endif

    QTextBoundaryFinder boundaryFinder(type, testString);

    // test toNextBoundary()
    {
        QList<int> actualBreakPositions;
        if (boundaryFinder.isAtBoundary())
            actualBreakPositions.append(boundaryFinder.position());
        while (boundaryFinder.toNextBoundary() != -1) {
            QVERIFY(boundaryFinder.isAtBoundary());
            actualBreakPositions.append(boundaryFinder.position());
        }
        QCOMPARE(actualBreakPositions, expectedBreakPositions);
    }

    // test toPreviousBoundary()
    {
        QList<int> expectedBreakPositionsRev = expectedBreakPositions;
        qSort(expectedBreakPositionsRev.begin(), expectedBreakPositionsRev.end(), qGreater<int>());

        QList<int> actualBreakPositions;
        boundaryFinder.toEnd();
        if (boundaryFinder.isAtBoundary())
            actualBreakPositions.append(boundaryFinder.position());
        while (boundaryFinder.toPreviousBoundary() != -1) {
            QVERIFY(boundaryFinder.isAtBoundary());
            actualBreakPositions.append(boundaryFinder.position());
        }
        QCOMPARE(actualBreakPositions, expectedBreakPositionsRev);
    }

    // test isAtBoundary()
    for (int i = 0; i < testString.length(); ++i) {
        boundaryFinder.setPosition(i);
        QCOMPARE(boundaryFinder.isAtBoundary(), expectedBreakPositions.contains(i));
    }
}

#ifdef QT_BUILD_INTERNAL
void tst_QTextBoundaryFinder::graphemeBoundariesDefault_data()
{
    generateDataFromFile("data/GraphemeBreakTest.txt");
}

void tst_QTextBoundaryFinder::graphemeBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Grapheme, true);
}

void tst_QTextBoundaryFinder::wordBoundariesDefault_data()
{
    generateDataFromFile("data/WordBreakTest.txt");
}

void tst_QTextBoundaryFinder::wordBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Word, true);
}

void tst_QTextBoundaryFinder::sentenceBoundariesDefault_data()
{
    generateDataFromFile("data/SentenceBreakTest.txt");
}

void tst_QTextBoundaryFinder::sentenceBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Sentence, true);
}

void tst_QTextBoundaryFinder::lineBoundariesDefault_data()
{
    generateDataFromFile("data/LineBreakTest.txt");
}

void tst_QTextBoundaryFinder::lineBoundariesDefault()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    expectedBreakPositions.prepend(0); // ### QTBF generates a boundary at start of text
    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Line, true);
}
#endif // QT_BUILD_INTERNAL

void tst_QTextBoundaryFinder::wordBoundaries_manual_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    {
        QChar s[] = { 0x000D, 0x000A, 0x000A };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 2 << 3;

        QTest::newRow("+CRxLF+LF+") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x000D, 0x0308, 0x000A, 0x000A };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 2 << 3 << 4;

        QTest::newRow("+CR+FE+LF+LF+") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Aaa bbb ccc.\r\nDdd eee fff."));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 3 << 4 << 7 << 8 << 11 << 12 << 14 << 17 << 18 << 21 << 22 << 25 << 26;

        QTest::newRow("data1") << testString << expectedBreakPositions;
    }

    // Sample Strings from WordBreakTest.html
    {
        QChar s[] = { 0x0063, 0x0061, 0x006E, 0x0027, 0x0074 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 5;

        QTest::newRow("ts 1") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x0063, 0x0061, 0x006E, 0x2019, 0x0074 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 5;

        QTest::newRow("ts 2") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x0061, 0x0062, 0x00AD, 0x0062, 0x0061 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 5;

        QTest::newRow("ts 3") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x0061, 0x0024, 0x002D, 0x0033, 0x0034, 0x002C, 0x0035, 0x0036,
                      0x0037, 0x002E, 0x0031, 0x0034, 0x0025, 0x0062 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 2 << 3 << 12 << 13 << 14;

        QTest::newRow("ts 4") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x0033, 0x0061 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 2;

        QTest::newRow("ts 5") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x2060, 0x0063, 0x2060, 0x0061, 0x2060, 0x006E, 0x2060, 0x0027,
                      0x2060, 0x0074, 0x2060, 0x2060 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 12;

        QTest::newRow("ts 1e") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x2060, 0x0063, 0x2060, 0x0061, 0x2060, 0x006E, 0x2060, 0x2019,
                      0x2060, 0x0074, 0x2060, 0x2060 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 12;

        QTest::newRow("ts 2e") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x2060, 0x0061, 0x2060, 0x0062, 0x2060, 0x00AD, 0x2060, 0x0062,
                      0x2060, 0x0061, 0x2060, 0x2060 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 12;

        QTest::newRow("ts 3e") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x2060, 0x0061, 0x2060, 0x0024, 0x2060, 0x002D, 0x2060, 0x0033,
                      0x2060, 0x0034, 0x2060, 0x002C, 0x2060, 0x0035, 0x2060, 0x0036,
                      0x2060, 0x0037, 0x2060, 0x002E, 0x2060, 0x0031, 0x2060, 0x0034,
                      0x2060, 0x0025, 0x2060, 0x0062, 0x2060, 0x2060 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 3 << 5 << 7 << 25 << 27 << 30;

        QTest::newRow("ts 4e") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x2060, 0x0033, 0x2060, 0x0061, 0x2060, 0x2060 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 6;

        QTest::newRow("ts 5e") << testString << expectedBreakPositions;
    }
}

void tst_QTextBoundaryFinder::wordBoundaries_manual()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Word);
}

void tst_QTextBoundaryFinder::sentenceBoundaries_manual_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    {
        QChar s[] = { 0x000D, 0x000A, 0x000A };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 2 << 3;

        QTest::newRow("+CRxLF+LF+") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x000D, 0x0308, 0x000A, 0x000A };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 3 << 4;

        QTest::newRow("+CR+FExLF+LF+") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Aaa bbb ccc.\r\nDdd eee fff."));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 14 << 26;

        QTest::newRow("data1") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Diga-nos qualé a sua opinião"));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 28;

        QTest::newRow("data2") << testString << expectedBreakPositions;
    }
}

void tst_QTextBoundaryFinder::sentenceBoundaries_manual()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Sentence);
}

void tst_QTextBoundaryFinder::lineBoundaries_manual_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    {
        QString testString(QString::fromUtf8("Aaa bbb ccc.\r\nDdd eee fff."));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 4 << 8 << 14 << 18 << 22 << 26;

        QTest::newRow("data1") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Diga-nos qualé a sua opinião"));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 5 << 9 << 15 << 17 << 21 << 28;

        QTest::newRow("data2") << testString << expectedBreakPositions;
    }

    {
        QChar s[] = { 0x000A, 0x2E80, 0x0308, 0x0023, 0x0023 };
        QString testString(s, sizeof(s)/sizeof(QChar));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 3 << 5;

        QTest::newRow("x(LF)+(ID)x(CM)+(AL)x(AL)+") << testString << expectedBreakPositions;
    }
    {
        QChar s[] = { 0x000A, 0x0308, 0x0023, 0x0023 };
        QString testString(s, sizeof(s)/sizeof(QChar));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 4;

        QTest::newRow("x(LF)+(CM)x(AL)x(AL)+") << testString << expectedBreakPositions;
    }

    {
        QChar s[] = { 0x0061, 0x00AD, 0x0062, 0x0009, 0x0063, 0x0064 };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 2 << 4 << 6;

        QTest::newRow("x(AL)x(BA)+(AL)x(BA)+(AL)x(AL)+") << testString << expectedBreakPositions;
    }
}

void tst_QTextBoundaryFinder::lineBoundaries_manual()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Line);
}

void tst_QTextBoundaryFinder::fastConstructor()
{
    QString text("Hello World");
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text.constData(), text.length(), /*buffer*/0, /*buffer size*/0);
    QCOMPARE(finder.boundaryReasons(), QTextBoundaryFinder::StartWord);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), 5);
    QCOMPARE(finder.boundaryReasons(), QTextBoundaryFinder::EndWord);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), 6);
    QCOMPARE(finder.boundaryReasons(), QTextBoundaryFinder::StartWord);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), text.length());
    QCOMPARE(finder.boundaryReasons(), QTextBoundaryFinder::EndWord);

    finder.toNextBoundary();
    QCOMPARE(finder.position(), -1);
    QCOMPARE(finder.boundaryReasons(), QTextBoundaryFinder::NotAtBoundary);
}

void tst_QTextBoundaryFinder::assignmentOperator()
{
    QString text(QLatin1String("Hello World"));

    QTextBoundaryFinder invalidFinder;
    QVERIFY(!invalidFinder.isValid());
    QCOMPARE(invalidFinder.string(), QString());

    QTextBoundaryFinder validFinder(QTextBoundaryFinder::Word, text);
    QVERIFY(validFinder.isValid());
    QCOMPARE(validFinder.string(), text);

    QTextBoundaryFinder finder(QTextBoundaryFinder::Line, QLatin1String("dummy"));
    QVERIFY(finder.isValid());

    finder = invalidFinder;
    QVERIFY(!finder.isValid());
    QCOMPARE(finder.string(), QString());

    finder = validFinder;
    QVERIFY(finder.isValid());
    QCOMPARE(finder.string(), text);
}

void tst_QTextBoundaryFinder::wordBoundaries_qtbug6498()
{
    // text with trailing space
    QString text("Please test me. Finish ");
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);

    QCOMPARE(finder.position(), 0);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::StartWord);

    QCOMPARE(finder.toNextBoundary(), 6);
    QCOMPARE(finder.position(), 6);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::EndWord);

    QCOMPARE(finder.toNextBoundary(), 7);
    QCOMPARE(finder.position(), 7);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::StartWord);

    QCOMPARE(finder.toNextBoundary(), 11);
    QCOMPARE(finder.position(), 11);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::EndWord);

    QCOMPARE(finder.toNextBoundary(), 12);
    QCOMPARE(finder.position(), 12);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::StartWord);

    QCOMPARE(finder.toNextBoundary(), 14);
    QCOMPARE(finder.position(), 14);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::EndWord);

    QCOMPARE(finder.toNextBoundary(), 15);
    QCOMPARE(finder.position(), 15);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() == QTextBoundaryFinder::NotAtBoundary);

    QCOMPARE(finder.toNextBoundary(), 16);
    QCOMPARE(finder.position(), 16);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::StartWord);

    QCOMPARE(finder.toNextBoundary(), 22);
    QCOMPARE(finder.position(), 22);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() & QTextBoundaryFinder::EndWord);

    QCOMPARE(finder.toNextBoundary(), 23);
    QCOMPARE(finder.position(), 23);
    QVERIFY(finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() == QTextBoundaryFinder::NotAtBoundary);

    QCOMPARE(finder.toNextBoundary(), -1);
    QCOMPARE(finder.position(), -1);
    QVERIFY(!finder.isAtBoundary());
    QVERIFY(finder.boundaryReasons() == QTextBoundaryFinder::NotAtBoundary);
}

void tst_QTextBoundaryFinder::isAtSoftHyphen_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    QString testString = QString::fromUtf8("I a-m break-able");
    testString.replace(QLatin1Char('-'), QChar(QChar::SoftHyphen));
    QList<int> expectedBreakPositions;
    expectedBreakPositions << 0 << 2 << 4 << 6 << 12 << 16;
    QTest::newRow("Soft Hyphen") << testString << expectedBreakPositions;
}

void tst_QTextBoundaryFinder::isAtSoftHyphen()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    doTestData(testString, expectedBreakPositions, QTextBoundaryFinder::Line);

    QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Line, testString);
    for (int i = 0; (i = testString.indexOf(QChar(QChar::SoftHyphen), i)) != -1; ++i) {
        QVERIFY(expectedBreakPositions.contains(i + 1));
        boundaryFinder.setPosition(i + 1);
        QVERIFY(boundaryFinder.isAtBoundary());
        QVERIFY(boundaryFinder.boundaryReasons() & QTextBoundaryFinder::SoftHyphen);
    }
}

void tst_QTextBoundaryFinder::isAtMandatoryBreak_data()
{
    QTest::addColumn<QString>("testString");
    QTest::addColumn<QList<int> >("expectedBreakPositions");

    {
        QChar s[] = { 0x000D, 0x0308, 0x000A, 0x000A };
        QString testString(s, sizeof(s)/sizeof(s[0]));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 1 << 3 << 4;

        QTest::newRow("+CR+FExLF+LF+") << testString << expectedBreakPositions;
    }
    {
        QString testString(QString::fromUtf8("Aaa bbb ccc.\r\nDdd eee fff."));
        QList<int> expectedBreakPositions;
        expectedBreakPositions << 0 << 14 << 26;

        QTest::newRow("data1") << testString << expectedBreakPositions;
    }
}

void tst_QTextBoundaryFinder::isAtMandatoryBreak()
{
    QFETCH(QString, testString);
    QFETCH(QList<int>, expectedBreakPositions);

    QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Line, testString);
    for (int i = 0; i <= testString.size(); ++i) {
        boundaryFinder.setPosition(i);
        if (boundaryFinder.boundaryReasons() & QTextBoundaryFinder::MandatoryBreak)
            QVERIFY(expectedBreakPositions.contains(i));
    }
}

#include <qlibrary.h>

#define LIBTHAI_MAJOR   0
typedef int (*th_brk_def) (const unsigned char*, int*, size_t);
static th_brk_def th_brk = 0;

static bool init_libthai()
{
#if !defined(QT_NO_LIBRARY)
    static bool triedResolve = false;
    if (!triedResolve) {
        th_brk = (th_brk_def) QLibrary::resolve("thai", (int)LIBTHAI_MAJOR, "th_brk");
        triedResolve = true;
    }
#endif
    return th_brk != 0;
}

void tst_QTextBoundaryFinder::thaiLineBreak()
{
    if (!init_libthai())
        QSKIP("This test requires libThai-0.1.1x to be installed.");
#if 0
    // สวัสดีครับ นี่เป็นการงทดสอบตัวเอ
    QTextCodec *codec = QTextCodec::codecForMib(2259);
    QString text = codec->toUnicode(QByteArray("\xca\xc7\xd1\xca\xb4\xd5\xa4\xc3\xd1\xba\x20\xb9\xd5\xe8\xe0\xbb\xe7\xb9\xa1\xd2\xc3\xb7\xb4\xca\xcd\xba\xb5\xd1\xc7\xe0\xcd\xa7"));
    QCOMPARE(text.length(), 32);

    QTextBoundaryFinder finder(QTextBoundaryFinder::Line, text);
    finder.setPosition(0);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(1);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(2);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(3);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(4);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(5);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(6);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(7);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(8);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(9);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(10);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(11);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(12);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(13);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(14);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(15);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(16);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(17);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(18);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(19);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(20);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(21);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(22);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(23);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(24);
    QVERIFY(!finder.isAtBoundary());
    finder.setPosition(25);
    QVERIFY(finder.isAtBoundary());
    finder.setPosition(26);
    QVERIFY(finder.isAtBoundary());
    for (int i = 27; i < 32; ++i) {
        finder.setPosition(i);
        QVERIFY(!finder.isAtBoundary());
    }
#endif
}


QTEST_MAIN(tst_QTextBoundaryFinder)
#include "tst_qtextboundaryfinder.moc"
