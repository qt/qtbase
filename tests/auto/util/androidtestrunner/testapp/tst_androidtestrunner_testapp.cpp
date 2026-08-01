// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qbytearray.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qglobal.h>
#include <QtCore/qnativeinterface.h>
#include <QtTest/qtest.h>

#include <chrono>
#include <cstdlib>
#include <thread>

using namespace Qt::StringLiterals;

namespace {
QByteArray scenario()
{
    return qgetenv("QT_TESTRUNNER_SCENARIO");
}

// Runs while the library loads, before main(), to simulate a crash during app launch.
struct CrashBeforeMain
{
    CrashBeforeMain()
    {
        if (scenario() == "crash_before_main")
            std::abort();
    }
};
const CrashBeforeMain crashBeforeMain;
}

class tst_AndroidTestRunnerTestApp : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void alwaysPasses();
    void alwaysFails();
    void alwaysSkips();

    void test_function_with_underscores_and_digits_123();

    void dataDriven_data();
    void dataDriven();

    void verifyEnvVarQt();
    void verifyEnvVarQtest();
    void verifyEnvVarLeak();
    void verifyEnvVarValueShapes();
    void verifyEnvVarFullValueWithSpaces();
    void verifyEnvVarTabInValue();

    void runScenario();

    void anrBlockUi();
    void blockForDisconnect();
};

void tst_AndroidTestRunnerTestApp::initTestCase()
{
    if (scenario() == "crash_in_init") {
        volatile int *p = nullptr;
        *p = 1;
    }
}

void tst_AndroidTestRunnerTestApp::alwaysPasses()
{
    QVERIFY(true);
}

void tst_AndroidTestRunnerTestApp::alwaysFails()
{
    QFAIL("intentional failure from alwaysFails");
}

void tst_AndroidTestRunnerTestApp::alwaysSkips()
{
    QSKIP("intentional skip from alwaysSkips");
}

void tst_AndroidTestRunnerTestApp::test_function_with_underscores_and_digits_123()
{
    QVERIFY(true);
}

void tst_AndroidTestRunnerTestApp::dataDriven_data()
{
    QTest::addColumn<bool>("shouldPass");
    QTest::newRow("rowOne") << true;
    QTest::newRow("rowTwo") << false;
    QTest::newRow("rowThree") << true;
    QTest::newRow("row with spaces") << true;
    QTest::newRow("row-with-dashes") << true;
    QTest::newRow("row_with_underscores") << true;
    QTest::newRow("row.with.dots") << true;
    QTest::newRow("row/with/slashes") << true;
    QTest::newRow("special!@#$%^&*()") << true;
    QTest::newRow("row\"with\"quotes") << true;
}

void tst_AndroidTestRunnerTestApp::dataDriven()
{
    QFETCH(bool, shouldPass);
    if (!shouldPass)
        QFAIL("intentional failure in dataDriven row");
}

void tst_AndroidTestRunnerTestApp::verifyEnvVarQt()
{
    QCOMPARE(qgetenv("QT_TESTRUNNER_PROBE"), QByteArrayLiteral("qt_value"));
}

void tst_AndroidTestRunnerTestApp::verifyEnvVarQtest()
{
    QCOMPARE(qgetenv("QTEST_TESTRUNNER_PROBE"), QByteArrayLiteral("qtest_value"));
}

void tst_AndroidTestRunnerTestApp::verifyEnvVarLeak()
{
    const QByteArray leaked = qgetenv("TESTRUNNER_LEAK_PROBE");
    QVERIFY2(leaked.isEmpty(), qPrintable(
        QLatin1String("Non QtTest environment variables leaked into the app: ") + leaked)
    );
}

void tst_AndroidTestRunnerTestApp::verifyEnvVarValueShapes()
{
    QCOMPARE(qgetenv("QT_TESTRUNNER_PROBE_PLAIN"),
             QByteArrayLiteral("hello-world_123"));
    QCOMPARE(qgetenv("QT_TESTRUNNER_PROBE_SPECIAL"),
             QByteArrayLiteral("special!@#$%^&*()_+-=,.<>?[]{}|/\\'\""));
    QCOMPARE(qgetenv("QT_TESTRUNNER_PROBE_SPACES"),
             QByteArrayLiteral("hello world with spaces"));
}

void tst_AndroidTestRunnerTestApp::verifyEnvVarFullValueWithSpaces()
{
    QCOMPARE(qgetenv("QT_TESTRUNNER_PROBE"), QByteArrayLiteral("hello world with spaces"));
}

void tst_AndroidTestRunnerTestApp::verifyEnvVarTabInValue()
{
    QCOMPARE(qgetenv("QT_TESTRUNNER_PROBE_TAB"), QByteArrayLiteral("before\tafter"));
}

void tst_AndroidTestRunnerTestApp::runScenario()
{
    const QByteArray mode = scenario();
    if (mode.isEmpty() || mode == "pass") {
        QVERIFY(true);
        return;
    }
    if (mode == "fail")
        QFAIL("intentional failure from runScenario");
    if (mode == "skip")
        QSKIP("intentional skip from runScenario");
    if (mode == "crash") {
        volatile int *p = nullptr;
        *p = 1;
    }
    if (mode == "qfatal")
        qFatal("intentional qFatal from runScenario");
    if (mode == "drop_results") {
        // Wipe result files at atexit so pullResults() reads empty content.
        std::atexit([]() {
            for (const QFileInfo &fi : QDir().entryInfoList(QDir::Files))
                if (fi.fileName() != "qtest_last_exit_code"_L1)
                    QFile::remove(fi.filePath());
        });
        return;
    }
    if (mode == "drop_exit_code") {
        // Unlink qtest_last_exit_code at atexit so testExitCode() fails.
        std::atexit([]() { QFile::remove("qtest_last_exit_code"_L1); });
        return;
    }
    QFAIL(qPrintable("Unknown scenario: "_L1 + QString::fromUtf8(mode)));
}

void tst_AndroidTestRunnerTestApp::anrBlockUi()
{
    using namespace std::chrono_literals;
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() -> QVariant {
        std::this_thread::sleep_for(40s);
        return {};
    });
    std::this_thread::sleep_for(40s);
}

void tst_AndroidTestRunnerTestApp::blockForDisconnect()
{
    // A plain liveness window (no UI-thread block, so no stray ANR) wide enough
    // that the driver's evict() lands while the runner is still polling.
    std::this_thread::sleep_for(std::chrono::seconds(30));
}

QTEST_MAIN(tst_AndroidTestRunnerTestApp)
#include "tst_androidtestrunner_testapp.moc"
