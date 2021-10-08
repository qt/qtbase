/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QTest>

class tst_Warnings: public QObject
{
    Q_OBJECT
private slots:
    void testWarnings();
    void testMissingWarnings();
    void testMissingWarningsRegularExpression();
    void testMissingWarningsWithData_data();
    void testMissingWarningsWithData();

    void testFailOnWarnings();
    void testFailOnWarningsCleared();
#if QT_CONFIG(regularexpression)
    void testFailOnWarningsWithData_data();
    void testFailOnWarningsWithData();
    void testFailOnWarningsFailInHelper();
    void testFailOnWarningsThenSkip();
#endif
    void testFailOnWarningsAndIgnoreWarnings();
};

void tst_Warnings::testWarnings()
{
    qWarning("Warning");

    QTest::ignoreMessage(QtWarningMsg, "Warning");
    qWarning("Warning");

    qWarning("Warning");

    qDebug("Debug");

    QTest::ignoreMessage(QtDebugMsg, "Debug");
    qDebug("Debug");

    qDebug("Debug");

    qInfo("Info");

    QTest::ignoreMessage(QtInfoMsg, "Info");
    qInfo("Info");

    qInfo("Info");

    QTest::ignoreMessage(QtDebugMsg, "Bubu");
    qDebug("Baba");
    qDebug("Bubu");
    qDebug("Baba");

    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("^Bubu.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Baba.*"));
    qDebug("Bubublabla");
    qWarning("Babablabla");
    qDebug("Bubublabla");
    qWarning("Babablabla");

    // accept redundant space at end to keep compatibility with Qt < 5.2
    QTest::ignoreMessage(QtDebugMsg, "Bubu ");
    qDebug() << "Bubu";

    // Cope with non-ASCII messages; should be understood as UTF-8 (it comes
    // from source code on both sides), even if the system encoding is
    // different:
    QTest::ignoreMessage(QtDebugMsg, "Hej v\xc3\xa4rlden");
    qDebug() << "Hej v\xc3\xa4rlden";
    QTest::ignoreMessage(QtInfoMsg, "Hej v\xc3\xa4rlden");
    qInfo() << "Hej v\xc3\xa4rlden";
}

void tst_Warnings::testMissingWarnings()
{
    QTest::ignoreMessage(QtWarningMsg, "Warning0");
    QTest::ignoreMessage(QtWarningMsg, "Warning1");
    QTest::ignoreMessage(QtWarningMsg, "Warning2");

    qWarning("Warning2");
}

void tst_Warnings::testMissingWarningsRegularExpression()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Warning\\d\\d"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Warning\\s\\d"));

    qWarning("Warning11");
}

void tst_Warnings::testMissingWarningsWithData_data()
{
    QTest::addColumn<int>("dummy");

    QTest::newRow("first row") << 0;
    QTest::newRow("second row") << 1;
}

void tst_Warnings::testMissingWarningsWithData()
{
    QTest::ignoreMessage(QtWarningMsg, "Warning0");
    QTest::ignoreMessage(QtWarningMsg, "Warning1");
    QTest::ignoreMessage(QtWarningMsg, "Warning2");

    qWarning("Warning2");
}

void tst_Warnings::testFailOnWarnings()
{
    // failOnWarnings() wasn't called yet; shouldn't fail;
    qWarning("Ran out of space!");

#if QT_CONFIG(regularexpression)
    const auto warnRegex = QRegularExpression("Ran out of .*!");
    QTest::failOnWarning(warnRegex);
    // Should now fail.
    qWarning("Ran out of cabbage!");

    // Should not fail; none of these are warnings.
    qDebug("Ran out of tortillas!");
    qInfo("Ran out of oil!");

    // Should not fail; regex doesn't match.
    qWarning("nope");

    // Should fail; matches regex.
    qWarning("Ran out of biscuits!");
#endif // QT_CONFIG(regularexpression)

    QTest::failOnWarning("Running low on toothpaste!");

    // Should fail; strings match.
    qWarning("Running low on toothpaste!");

    // Shouldn't fail; strings don't match.
    qWarning("Running low on flour!");

    // Should not fail; none of these are warnings.
    qDebug("Running low on toothpaste!");
    qInfo("Running low on toothpaste!");
}

void tst_Warnings::testFailOnWarningsCleared()
{
    // The patterns passed to failOnWarnings() should be cleared at the end of
    // each test function, so this shouldn't fail because of the failOnWarning() call in the previous function.
    // Note that this test always needs to come after testFailOnWarnings for it to work.
    qWarning("Ran out of muffins!");
}

#if QT_CONFIG(regularexpression)
void tst_Warnings::testFailOnWarningsWithData_data()
{
    // The warning message that should cause a failure.
    QTest::addColumn<QString>("warningMessage");

    QTest::newRow("warning1") << "warning1";
    QTest::newRow("warning2") << "warning2";
    QTest::newRow("warning3") << "warning3";
}

void tst_Warnings::testFailOnWarningsWithData()
{
    QFETCH(QString, warningMessage);

    QTest::failOnWarning(QRegularExpression(warningMessage));

    // Only one of these should fail, depending on warningMessage.
    qWarning("warning1");
    qWarning("warning2");
    qWarning("warning3");
}

void tst_Warnings::testFailOnWarningsFailInHelper()
{
    [](){ QFAIL("This failure message should be printed but not cause the test to abort"); }();
    const auto warnRegex = QRegularExpression("Ran out of .*!");
    QTest::failOnWarning(warnRegex);
    qWarning("Ran out of cabbage!");
    QFAIL("My cabbage! :(");
}

void tst_Warnings::testFailOnWarningsThenSkip()
{
    const auto warnRegex = QRegularExpression("Ran out of .*!");
    QTest::failOnWarning(warnRegex);
    qWarning("Ran out of cabbage!");
    QSKIP("My cabbage! :(");
}
#endif // QT_CONFIG(regularexpression)

void tst_Warnings::testFailOnWarningsAndIgnoreWarnings()
{
    const auto warningStr = "Running low on toothpaste!";
    QTest::failOnWarning(warningStr);
    QTest::ignoreMessage(QtWarningMsg, warningStr);
    // Shouldn't fail; we ignored it.
    qWarning(warningStr);
}

QTEST_MAIN(tst_Warnings)

#include "tst_warnings.moc"
