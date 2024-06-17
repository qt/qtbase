// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QTEST_THROW_ON_FAIL // fails ### investigate

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QTest>

class tst_Warnings: public QObject
{
    Q_OBJECT
private slots:
    void testWarnings();
    void testMissingWarnings();
#if QT_CONFIG(regularexpression)
    void testMissingWarningsRegularExpression();
#endif
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
    void testFailOnTemporaryObjectDestruction();
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

#if QT_CONFIG(regularexpression)
void tst_Warnings::testMissingWarningsRegularExpression()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Warning\\d\\d"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Warning\\s\\d"));

    qWarning("Warning11");
}
#endif

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
    // failOnWarning() wasn't called yet; shouldn't fail;
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
    // each test function, so this shouldn't fail because of the failOnWarning()
    // call in the previous function. Note that this test always needs to come
    // after testFailOnWarnings for it to test anything meaningfully.
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
    const QTest::ThrowOnFailDisabler nothrow; // tests repeated QFAILs
    [](){ QFAIL("This failure message should be printed but not cause the test to abort"); }();
    // So we've already failed, but we get more messages - that don't increment counters.
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
    QSKIP("My cabbage! :("); // Reports, but doesn't count.
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

void tst_Warnings::testFailOnTemporaryObjectDestruction()
{
    QTest::failOnWarning("Running low on toothpaste!");
    QTest::ignoreMessage(QtWarningMsg, "Ran out of cabbage!");

    class TestObject : public QObject
    {
    public:
        ~TestObject()
        {
            // Shouldn't fail - ignored
            qWarning("Ran out of cabbage!");
            // Should fail
            qWarning("Running low on toothpaste!");
        }
    };

    QScopedPointer<TestObject, QScopedPointerDeleteLater> testObject(new TestObject);
    QVERIFY(testObject);
}

QTEST_MAIN(tst_Warnings)

#include "tst_warnings.moc"
