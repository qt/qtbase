/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#if QT_CONFIG(temporaryfile) && QT_CONFIG(process)
#  define USE_DIFF
#endif
#include <QtCore/QXmlStreamReader>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#ifdef USE_DIFF
#  include <QtCore/QTemporaryFile>
#  include <QtCore/QStandardPaths>
#endif
#include <QtTest/QtTest>

#include <private/cycle_p.h>

#include "emulationdetector.h"

struct LoggerSet;

class tst_Selftests: public QObject
{
    Q_OBJECT
public:
    tst_Selftests();

private slots:
    void initTestCase();
    void runSubTest_data();
    void runSubTest();
    void cleanup();

private:
    void doRunSubTest(QString const& subdir, QStringList const& loggers, QStringList const& arguments, bool crashes);
    bool compareOutput(const QString &logger, const QString &subdir,
                       const QByteArray &rawOutput, const QByteArrayList &actual,
                       const QByteArrayList &expected,
                       QString *errorMessage) const;
    bool compareLine(const QString &logger, const QString &subdir, bool benchmark,
                     const QString &actualLine, const QString &expLine,
                     QString *errorMessage) const;
    bool checkXml(const QString &logger, QByteArray rawOutput,
                  QString *errorMessage) const;

    QString logName(const QString &logger) const;
    QList<LoggerSet> allLoggerSets() const;

    QTemporaryDir tempDir;
};

struct BenchmarkResult
{
    qint64  total;
    qint64  iterations;
    QString unit;

    inline QString toString() const
    { return QString("total:%1, unit:%2, iterations:%3").arg(total).arg(unit).arg(iterations); }

    static BenchmarkResult parse(QString const&, QString*);
};

static QString msgMismatch(const QString &actual, const QString &expected)
{
    return QLatin1String("Mismatch:\n'") + actual + QLatin1String("'\n !=\n'")
        + expected + QLatin1Char('\'');
}

static bool compareBenchmarkResult(BenchmarkResult const &r1, BenchmarkResult const &r2,
                                   QString *errorMessage)
{
    // First make sure the iterations and unit match.
    if (r1.iterations != r2.iterations || r1.unit != r2.unit) {
        // Nope - compare whole string for best failure message
        *errorMessage = msgMismatch(r1.toString(), r2.toString());
        return false;
    }

    // Now check the value.  Some variance is allowed, and how much depends on
    // the measured unit.
    qreal variance = 0.;
    if (r1.unit == QLatin1String("msecs") || r1.unit == QLatin1String("WalltimeMilliseconds"))
        variance = 0.1;
    else if (r1.unit == QLatin1String("instruction reads"))
        variance = 0.001;
    else if (r1.unit == QLatin1String("CPU ticks") || r1.unit == QLatin1String("CPUTicks"))
        variance = 0.001;

    if (variance == 0.) {
        // No variance allowed - compare whole string
        const QString r1S = r1.toString();
        const QString r2S = r2.toString();
        if (r1S != r2S) {
            *errorMessage = msgMismatch(r1S, r2S);
            return false;
        }
        return true;
    }

    if (qAbs(qreal(r1.total) - qreal(r2.total)) > qreal(r1.total) * variance) {
        // Whoops, didn't match.  Compare the whole string for the most useful failure message.
        *errorMessage = msgMismatch(r1.toString(), r2.toString());
        return false;
    }
    return true;
}

// Split the passed block of text into an array of lines, replacing any
// filenames and line numbers with generic markers to avoid failing the test
// due to compiler-specific behaviour.
static QList<QByteArray> splitLines(QByteArray ba)
{
    ba.replace('\r', "");
    QList<QByteArray> out = ba.split('\n');

    // Replace any ` file="..."' or ` line="..."'  in XML with a generic location.
    static const char *markers[][2] = {
        { " file=\"", " file=\"__FILE__\"" },
        { " line=\"", " line=\"__LINE__\"" }
    };
    static const int markerCount = sizeof markers / sizeof markers[0];

    for (int i = 0; i < out.size(); ++i) {
        QByteArray& line = out[i];
        for (int j = 0; j < markerCount; ++j) {
            int index = line.indexOf(markers[j][0]);
            if (index == -1) {
                continue;
            }
            const int end = line.indexOf('"', index + int(strlen(markers[j][0])));
            if (end == -1) {
                continue;
            }
            line.replace(index, end-index + 1, markers[j][1]);
        }
    }

    return out;
}

// Return the log format, e.g. for both "stdout txt" and "txt", return "txt'.
static inline QString logFormat(const QString &logger)
{
    return (logger.startsWith("stdout") ? logger.mid(7) : logger);
}

// Return the log file name, or an empty string if the log goes to stdout.
QString tst_Selftests::logName(const QString &logger) const
{
    return (logger.startsWith("stdout") ? "" : QString(tempDir.path() + "/test_output." + logger));
}

static QString expectedFileNameFromTest(const QString &subdir, const QString &logger)
{
    return QStringLiteral("expected_") + subdir + QLatin1Char('.') + logFormat(logger);
}

// Load the expected test output for the nominated test (subdir) and logger
// as an array of lines.  If there is no expected output file, return an
// empty array.
static QList<QByteArray> expectedResult(const QString &fileName)
{
    QFile file(QStringLiteral(":/") + fileName);
    if (!file.open(QIODevice::ReadOnly))
        return QList<QByteArray>();
    return splitLines(file.readAll());
}

// Helpers for running the 'diff' tool in case comparison fails
#ifdef USE_DIFF
static inline void writeLines(QIODevice &d, const QByteArrayList &lines)
{
    for (const QByteArray &l : lines) {
        d.write(l);
        d.write("\n");
    }
}
#endif // USE_DIFF

static QByteArray runDiff(const QByteArrayList &expected, const QByteArrayList &actual)
{
    QByteArray result;
#ifdef USE_DIFF
#  ifndef Q_OS_WIN
    const QString diff = QStandardPaths::findExecutable("diff");
#  else
    const QString diff = QStandardPaths::findExecutable("diff.exe");
#  endif
    if (diff.isEmpty())
        return result;
    QTemporaryFile expectedFile;
    if (!expectedFile.open())
        return result;
    writeLines(expectedFile, expected);
    expectedFile.close();
    QTemporaryFile actualFile;
    if (!actualFile.open())
        return result;
    writeLines(actualFile, actual);
    actualFile.close();
    QProcess diffProcess;
    diffProcess.start(diff, {QLatin1String("-u"), expectedFile.fileName(), actualFile.fileName()});
    if (!diffProcess.waitForStarted())
        return result;
    if (diffProcess.waitForFinished())
        result = diffProcess.readAllStandardOutput();
    else
        diffProcess.kill();
#endif // USE_DIFF
    return result;
}

// Each test is run with a set of one or more test output loggers.
// This struct holds information about one such test.
struct LoggerSet
{
    LoggerSet(QString const& _name, QStringList const& _loggers, QStringList const& _arguments)
        : name(_name), loggers(_loggers), arguments(_arguments)
    { }

    QString name;
    QStringList loggers;
    QStringList arguments;
};

// This function returns a list of all sets of loggers to be used for
// running each subtest.
QList<LoggerSet> tst_Selftests::allLoggerSets() const
{
    // Note that in order to test XML output to standard output, the subtests
    // must not send output directly to stdout, bypassing Qt's output mechanisms
    // (e.g. via printf), otherwise the output may not be well-formed XML.
    return QList<LoggerSet>()
        // Test with old-style options for a single logger
        << LoggerSet("old stdout txt",
                     QStringList() << "stdout txt",
                     QStringList()
                    )
        << LoggerSet("old txt",
                     QStringList() << "txt",
                     QStringList() << "-o" << logName("txt")
                    )
        << LoggerSet("old stdout xml",
                     QStringList() << "stdout xml",
                     QStringList() << "-xml"
                    )
        << LoggerSet("old xml",
                     QStringList() << "xml",
                     QStringList() << "-xml" << "-o" << logName("xml")
                    )
        << LoggerSet("old stdout junitxml",
                     QStringList() << "stdout junitxml",
                     QStringList() << "-junitxml"
                    )
        << LoggerSet("old junitxml",
                     QStringList() << "junitxml",
                     QStringList() << "-junitxml" << "-o" << logName("junitxml")
                    )
        << LoggerSet("old xunitxml compatibility",
                     QStringList() << "junitxml",
                     QStringList() << "-xunitxml" << "-o" << logName("junitxml")
                    )
        << LoggerSet("old stdout lightxml",
                     QStringList() << "stdout lightxml",
                     QStringList() << "-lightxml"
                    )
        << LoggerSet("old lightxml",
                     QStringList() << "lightxml",
                     QStringList() << "-lightxml" << "-o" << logName("lightxml")
                    )
        << LoggerSet("old stdout csv", // benchmarks only
                     QStringList() << "stdout csv",
                     QStringList() << "-csv")
        << LoggerSet("old csv", // benchmarks only
                     QStringList() << "csv",
                     QStringList() << "-csv" << "-o" << logName("csv"))
        << LoggerSet("old stdout teamcity",
                     QStringList() << "stdout teamcity",
                     QStringList() << "-teamcity"
                    )
        << LoggerSet("old teamcity",
                     QStringList() << "teamcity",
                     QStringList() << "-teamcity" << "-o" << logName("teamcity")
                    )
        << LoggerSet("old stdout tap",
                     QStringList() << "stdout tap",
                     QStringList() << "-tap"
                    )
        << LoggerSet("old tap",
                     QStringList() << "tap",
                     QStringList() << "-tap" << "-o" << logName("tap")
                    )
        // Test with new-style options for a single logger
        << LoggerSet("new stdout txt",
                     QStringList() << "stdout txt",
                     QStringList() << "-o" << "-,txt"
                    )
        << LoggerSet("new txt",
                     QStringList() << "txt",
                     QStringList() << "-o" << logName("txt")+",txt"
                    )
        << LoggerSet("new stdout xml",
                     QStringList() << "stdout xml",
                     QStringList() << "-o" << "-,xml"
                    )
        << LoggerSet("new xml",
                     QStringList() << "xml",
                     QStringList() << "-o" << logName("xml")+",xml"
                    )
        << LoggerSet("new stdout junitxml",
                     QStringList() << "stdout junitxml",
                     QStringList() << "-o" << "-,junitxml"
                    )
        << LoggerSet("new stdout xunitxml compatibility",
                     QStringList() << "stdout junitxml",
                     QStringList() << "-o" << "-,xunitxml"
                    )
        << LoggerSet("new junitxml",
                     QStringList() << "junitxml",
                     QStringList() << "-o" << logName("junitxml")+",junitxml"
                    )
        << LoggerSet("new stdout lightxml",
                     QStringList() << "stdout lightxml",
                     QStringList() << "-o" << "-,lightxml"
                    )
        << LoggerSet("new lightxml",
                     QStringList() << "lightxml",
                     QStringList() << "-o" << logName("lightxml")+",lightxml"
                    )
        << LoggerSet("new stdout csv", // benchmarks only
                     QStringList() << "stdout csv",
                     QStringList() << "-o" << "-,csv")
        << LoggerSet("new csv", // benchmarks only
                     QStringList() << "csv",
                     QStringList() << "-o" << logName("csv")+",csv")
        << LoggerSet("new stdout teamcity",
                     QStringList() << "stdout teamcity",
                     QStringList() << "-o" << "-,teamcity"
                    )
        << LoggerSet("new teamcity",
                     QStringList() << "teamcity",
                     QStringList() << "-o" << logName("teamcity")+",teamcity"
                    )
        << LoggerSet("new stdout tap",
                     QStringList() << "stdout tap",
                     QStringList() << "-o" << "-,tap"
                    )
        << LoggerSet("new tap",
                     QStringList() << "tap",
                     QStringList() << "-o" << logName("tap")+",tap"
                    )
        // Test with two loggers (don't test all 32 combinations, just a sample)
        << LoggerSet("stdout txt + txt",
                     QStringList() << "stdout txt" << "txt",
                     QStringList() << "-o" << "-,txt"
                                   << "-o" << logName("txt")+",txt"
                    )
        << LoggerSet("xml + stdout txt",
                     QStringList() << "xml" << "stdout txt",
                     QStringList() << "-o" << logName("xml")+",xml"
                                   << "-o" << "-,txt"
                    )
        << LoggerSet("txt + junitxml",
                     QStringList() << "txt" << "junitxml",
                     QStringList() << "-o" << logName("txt")+",txt"
                                   << "-o" << logName("junitxml")+",junitxml"
                    )
        << LoggerSet("lightxml + stdout junitxml",
                     QStringList() << "lightxml" << "stdout junitxml",
                     QStringList() << "-o" << logName("lightxml")+",lightxml"
                                   << "-o" << "-,junitxml"
                    )
        // All loggers at the same time (except csv)
        << LoggerSet("all loggers",
                     QStringList() << "txt" << "xml" << "lightxml" << "stdout txt" << "junitxml" << "tap",
                     QStringList() << "-o" << logName("txt")+",txt"
                                   << "-o" << logName("xml")+",xml"
                                   << "-o" << logName("lightxml")+",lightxml"
                                   << "-o" << "-,txt"
                                   << "-o" << logName("junitxml")+",junitxml"
                                   << "-o" << logName("teamcity")+",teamcity"
                                   << "-o" << logName("tap")+",tap"
                    )
    ;
}

tst_Selftests::tst_Selftests()
    : tempDir(QDir::tempPath() + "/tst_selftests.XXXXXX")
{}

void tst_Selftests::initTestCase()
{
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    //Detect the location of the sub programs
    QString subProgram = QLatin1String("pass/pass");
#if defined(Q_OS_WIN)
    subProgram = QLatin1String("pass/pass.exe");
#endif
    QString testdataDir = QFINDTESTDATA(subProgram);
    if (testdataDir.lastIndexOf(subProgram) > 0)
        testdataDir = testdataDir.left(testdataDir.lastIndexOf(subProgram));
    else
        testdataDir = QCoreApplication::applicationDirPath();
    // chdir to our testdata path and execute helper apps relative to that.
    QVERIFY2(QDir::setCurrent(testdataDir), qPrintable("Could not chdir to " + testdataDir));
}

void tst_Selftests::runSubTest_data()
{
    QTest::addColumn<QString>("subdir");
    QTest::addColumn<QStringList>("loggers");
    QTest::addColumn<QStringList>("arguments");
    QTest::addColumn<bool>("crashes");

    QStringList tests = QStringList()
#if !defined(Q_OS_WIN)
        // On windows, assert does nothing in release mode and blocks execution
        // with a popup window in debug mode.
        << "assert"
#endif
        << "badxml"
#if defined(__GNUC__) && defined(__i386) && defined(Q_OS_LINUX)
        // Only run on platforms where callgrind is available.
        << "benchlibcallgrind"
#endif
        << "benchlibcounting"
        << "benchlibeventcounter"
        << "benchliboptions"
        << "blacklisted"
        << "cmptest"
        << "commandlinedata"
        << "counting"
        << "crashes"
        << "datatable"
        << "datetime"
        << "differentexec"
#if !defined(QT_NO_EXCEPTIONS) && !defined(Q_CC_INTEL) && !defined(Q_OS_WIN)
        // Disable this test on Windows and for intel compiler, as the run-times
        // will popup dialogs with warnings that uncaught exceptions were thrown
        << "exceptionthrow"
#endif
        << "expectfail"
        << "failcleanup"
#ifndef Q_OS_WIN // these assert, by design; so same problem as "assert"
        << "faildatatype"
        << "failfetchtype"
#endif
        << "failinit"
        << "failinitdata"
#ifndef Q_OS_WIN // asserts, by design; so same problem as "assert"
        << "fetchbogus"
#endif
        << "findtestdata"
        << "float"
        << "globaldata"
        << "keyboard"
        << "longstring"
        << "maxwarnings"
        << "multiexec"
        << "pass"
        << "pairdiagnostics"
        << "printdatatags"
        << "printdatatagswithglobaltags"
        << "qexecstringlist"
        << "signaldumper"
        << "silent"
        << "singleskip"
        << "skip"
        << "skipcleanup"
        << "skipinit"
        << "skipinitdata"
        << "sleep"
        << "strcmp"
        << "subtest"
        << "testlib"
        << "tuplediagnostics"
        << "verbose1"
        << "verbose2"
#ifndef QT_NO_EXCEPTIONS
        // this test will test nothing if the exceptions are disabled
        << "verifyexceptionthrown"
#endif //!QT_NO_EXCEPTIONS
        << "warnings"
        << "watchdog"
        << "xunit"
    ;

    // These tests are affected by timing and whether the CPU tick counter
    // is monotonically increasing.  They won't work on some machines so
    // leave them off by default.  Feel free to enable them for your own
    // testing by setting the QTEST_ENABLE_EXTRA_SELFTESTS environment
    // variable to something non-empty.
    if (!qgetenv("QTEST_ENABLE_EXTRA_SELFTESTS").isEmpty())
        tests << "benchlibtickcounter"
              << "benchlibwalltime"
        ;

    foreach (LoggerSet const& loggerSet, allLoggerSets()) {
        QStringList loggers = loggerSet.loggers;

        foreach (QString const& subtest, tests) {
            // These tests don't work right unless logging plain text to
            // standard output, either because they execute multiple test
            // objects or because they internally supply arguments to
            // themselves.
            if (loggerSet.name != "old stdout txt" && loggerSet.name != "new stdout txt") {
                if (subtest == "differentexec") {
                    continue;
                }
                if (subtest == "multiexec") {
                    continue;
                }
                if (subtest == "qexecstringlist") {
                    continue;
                }
                if (subtest == "benchliboptions") {
                    continue;
                }
                if (subtest == "blacklisted") {
                    continue;
                }
                if (subtest == "printdatatags") {
                    continue;
                }
                if (subtest == "printdatatagswithglobaltags") {
                    continue;
                }
                if (subtest == "silent") {
                    continue;
                }
                // `crashes' will not output valid XML on platforms without a crash handler
                if (subtest == "crashes") {
                    continue;
                }
                // this test prints out some floats in the testlog and the formatting is
                // platform-specific and hard to predict.
                if (subtest == "float") {
                    continue;
                }
                // these tests are quite slow, and running them for all the loggers significantly
                // increases the overall test time.  They do not really relate to logging, so it
                // should be safe to run them just for the stdout loggers.
                if (subtest == "benchlibcallgrind" || subtest == "sleep") {
                    continue;
                }
            }
            if (subtest == "badxml" && (loggerSet.name == "all loggers" || loggerSet.name.contains("txt")))
                continue; // XML only, do not mix txt and XML for encoding test.

            if (loggerSet.name.contains("csv") && !subtest.startsWith("benchlib"))
                continue;

            if (loggerSet.name.contains("teamcity") && subtest.startsWith("benchlib"))
                continue;   // Skip benchmark for TeamCity logger

            // Keep in sync with generateTestData()'s crashers in generate_expected_output.py:
            const bool crashes = subtest == QLatin1String("assert") || subtest == QLatin1String("exceptionthrow")
                || subtest == QLatin1String("fetchbogus") || subtest == QLatin1String("crashedterminate")
                || subtest == QLatin1String("faildatatype") || subtest == QLatin1String("failfetchtype")
                || subtest == QLatin1String("crashes") || subtest == QLatin1String("silent")
                || subtest == QLatin1String("blacklisted") || subtest == QLatin1String("watchdog");
            QTest::newRow(qPrintable(QString("%1 %2").arg(subtest).arg(loggerSet.name)))
                << subtest
                << loggers
                << loggerSet.arguments
                << crashes
            ;
        }
    }
}

#if QT_CONFIG(process)

static QProcessEnvironment processEnvironment()
{
    static QProcessEnvironment result;
    if (result.isEmpty()) {
        const QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();
        const bool preserveLibPath = qEnvironmentVariableIsSet("QT_PRESERVE_TESTLIB_PATH");
        foreach (const QString &key, systemEnvironment.keys()) {
            const bool useVariable = key == QLatin1String("PATH") || key == QLatin1String("QT_QPA_PLATFORM")
#if defined(Q_OS_QNX)
                || key == QLatin1String("GRAPHICS_ROOT") || key == QLatin1String("TZ")
#elif defined(Q_OS_UNIX)
                || key == QLatin1String("HOME") || key == QLatin1String("USER") // Required for X11 on openSUSE
                || key == QLatin1String("QEMU_SET_ENV") || key == QLatin1String("QEMU_LD_PREFIX") // Required for QEMU
#  if !defined(Q_OS_MAC)
                || key == QLatin1String("DISPLAY") || key == QLatin1String("XAUTHLOCALHOSTNAME")
                || key.startsWith(QLatin1String("XDG_"))
#  endif // !Q_OS_MAC
#endif // Q_OS_UNIX
#ifdef __COVERAGESCANNER__
                || key == QLatin1String("QT_TESTCOCOON_ACTIVE")
#endif
                || ( preserveLibPath && (key == QLatin1String("QT_PLUGIN_PATH")
                                        || key == QLatin1String("LD_LIBRARY_PATH")))
                ;
            if (useVariable)
                result.insert(key, systemEnvironment.value(key));
        }
        // Avoid interference from any qtlogging.ini files, e.g. in /etc/xdg/QtProject/:
        result.insert(QStringLiteral("QT_LOGGING_RULES"),
                      // Must match generate_expected_output.py's main()'s value:
                      QStringLiteral("*.debug=true;qt.*=false"));
    }
    return result;
}

static inline QByteArray msgProcessError(const QString &binary, const QStringList &args,
                                         const QProcessEnvironment &e, const QString &what)
{
    QString result;
    QTextStream(&result) <<"Error running " << binary << ' ' << args.join(' ')
        << " with environment " << e.toStringList().join(' ') << ": " << what;
    return result.toLocal8Bit();
}

void tst_Selftests::doRunSubTest(QString const& subdir, QStringList const& loggers, QStringList const& arguments, bool crashes)
{
#if defined(__GNUC__) && defined(__i386) && defined(Q_OS_LINUX)
    if (subdir == "benchlibcallgrind") {
        QProcess checkProcess;
        QStringList args;
        args << QLatin1String("--version");
        checkProcess.start(QLatin1String("valgrind"), args);
        if (!checkProcess.waitForFinished(-1))
            QSKIP(QString("Valgrind broken or not available. Not running %1 test!").arg(subdir).toLocal8Bit());
    }
#endif

    QProcess proc;
    QProcessEnvironment environment = processEnvironment();
    // Keep in sync with generateTestData()'s extraEnv in generate_expected_output.py:
    if (crashes) {
        environment.insert("QTEST_DISABLE_CORE_DUMP", "1");
        environment.insert("QTEST_DISABLE_STACK_DUMP", "1");
        if (subdir == QLatin1String("watchdog"))
            environment.insert("QTEST_FUNCTION_TIMEOUT", "100");
    }
    proc.setProcessEnvironment(environment);
    const QString path = subdir + QLatin1Char('/') + subdir;
    proc.start(path, arguments);
    QVERIFY2(proc.waitForStarted(), msgProcessError(path, arguments, environment, QStringLiteral("Cannot start: ") + proc.errorString()));
    QVERIFY2(proc.waitForFinished(), msgProcessError(path, arguments, environment, QStringLiteral("Timed out: ") + proc.errorString()));
    if (!crashes) {
        QVERIFY2(proc.exitStatus() == QProcess::NormalExit,
                 msgProcessError(path, arguments, environment,
                                 QStringLiteral("Crashed: ") + proc.errorString()
                                 + QStringLiteral(": ") + QString::fromLocal8Bit(proc.readAllStandardError())));
    }

    QList<QByteArray> actualOutputs;
    for (int i = 0; i < loggers.count(); ++i) {
        QString logFile = logName(loggers[i]);
        QByteArray out;
        if (logFile.isEmpty()) {
            out = proc.readAllStandardOutput();
        } else {
            QFile file(logFile);
            if (file.open(QIODevice::ReadOnly))
                out = file.readAll();
        }
        actualOutputs << out;
    }

    const QByteArray err(proc.readAllStandardError());

    // Some tests may output unpredictable strings to stderr, which we'll ignore.
    //
    // For instance, uncaught exceptions on Windows might say (depending on Windows
    // version and JIT debugger settings):
    // "This application has requested the Runtime to terminate it in an unusual way.
    // Please contact the application's support team for more information."
    //
    // Also, tests which use valgrind may generate warnings if the toolchain is
    // newer than the valgrind version, such that valgrind can't understand the
    // debug information on the binary.
    if (subdir != QLatin1String("exceptionthrow")
        && subdir != QLatin1String("cmptest") // QImage comparison requires QGuiApplication
        && subdir != QLatin1String("fetchbogus")
        && subdir != QLatin1String("watchdog")
        && subdir != QLatin1String("xunit")
#ifdef Q_CC_MINGW
        && subdir != QLatin1String("blacklisted") // calls qFatal()
        && subdir != QLatin1String("silent") // calls qFatal()
#endif
#ifdef Q_OS_LINUX
        // QEMU outputs to stderr about uncaught signals
        && !(EmulationDetector::isRunningArmOnX86() &&
                (subdir == QLatin1String("assert")
                 || subdir == QLatin1String("blacklisted")
                 || subdir == QLatin1String("crashes")
                 || subdir == QLatin1String("faildatatype")
                 || subdir == QLatin1String("failfetchtype")
                 || subdir == QLatin1String("silent")
                )
            )
#endif
        && subdir != QLatin1String("benchlibcallgrind"))
        QVERIFY2(err.isEmpty(), err.constData());

    for (int n = 0; n < loggers.count(); ++n) {
        QString logger = loggers[n];
        if (n == 0 && subdir == QLatin1String("crashes")) {
            QByteArray &actual = actualOutputs[0];
#ifndef Q_OS_WIN
             // Remove digits of times to match the expected file.
            const QByteArray timePattern("Function time:");
            int timePos = actual.indexOf(timePattern);
            if (timePos >= 0) {
                timePos += timePattern.size();
                const int nextLinePos = actual.indexOf('\n', timePos);
                for (int c = (nextLinePos != -1 ? nextLinePos : actual.size()) - 1; c >= timePos; --c) {
                    if (actual.at(c) >= '0' && actual.at(c) <= '9')
                        actual.remove(c, 1);
                }
            }
#else // !Q_OS_WIN
            // Remove stack trace which is output to stdout.
            const int exceptionLogStart = actual.indexOf("A crash occurred in ");
            if (exceptionLogStart >= 0)
                actual.truncate(exceptionLogStart);
#endif // Q_OS_WIN
        }

        QList<QByteArray> res = splitLines(actualOutputs[n]);
        QString errorMessage;
        QString expectedFileName = expectedFileNameFromTest(subdir, logger);
        QByteArrayList exp = expectedResult(expectedFileName);
        if (!exp.isEmpty()) {
            if (!compareOutput(logger, subdir, actualOutputs[n], res, exp, &errorMessage)) {
                errorMessage.prepend(QLatin1Char('"') + logger + QLatin1String("\", ")
                                     + expectedFileName + QLatin1Char(' '));
                errorMessage += QLatin1String("\nActual:\n") + QLatin1String(actualOutputs[n]);
                const QByteArray diff = runDiff(exp, res);
                if (!diff.isEmpty())
                    errorMessage += QLatin1String("\nDiff:\n") + QLatin1String(diff);
                QFAIL(qPrintable(errorMessage));
            }
        } else {
            // For the "crashes" and other tests, there are multiple versions of the
            // expected output. Loop until a matching one is found.
            bool ok = false;
            for (int i = 1; !ok; ++i) {
                expectedFileName = expectedFileNameFromTest(subdir + QLatin1Char('_') + QString::number(i), logger);
                const QByteArrayList exp = expectedResult(expectedFileName);
                if (exp.isEmpty())
                    break;
                QString errorMessage2;
                ok = compareOutput(logger, subdir, actualOutputs[n], res, exp, &errorMessage2);
                if (!ok)
                    errorMessage += QLatin1Char('\n') + expectedFileName + QLatin1String(": ") + errorMessage2;
            }
            if (!ok) { // Use QDebug's quote mechanism to report potentially garbled output.
                errorMessage.prepend(QLatin1String("Cannot find a matching file for ") + subdir);
                errorMessage += QLatin1String("\nActual:\n");
                QDebug(&errorMessage) << actualOutputs[n];
                QFAIL(qPrintable(errorMessage));
            }
        }
    }
}

static QString teamCityLocation() { return QStringLiteral("|[Loc: _FILE_(_LINE_)|]"); }
static QString qtVersionPlaceHolder() { return QStringLiteral("@INSERT_QT_VERSION_HERE@"); }

bool tst_Selftests::compareOutput(const QString &logger, const QString &subdir,
                                  const QByteArray &rawOutput, const QByteArrayList &actual,
                                  const QByteArrayList &expected,
                                  QString *errorMessage) const
{

    if (actual.size() != expected.size()) {
        *errorMessage = QString::fromLatin1("Mismatch in line count. Expected %1 but got %2.")
                        .arg(expected.size()).arg(actual.size());
        return false;
    }

    // For xml output formats, verify that the log is valid XML.
    if (logger.endsWith(QLatin1String("xml")) && !checkXml(logger, rawOutput, errorMessage))
        return false;

    // Verify that the actual output is an acceptable match for the
    // expected output.

    const QString qtVersion = QLatin1String(QT_VERSION_STR);
    bool benchmark = false;
    for (int i = 0, size = actual.size(); i < size; ++i) {
        const QByteArray &actualLineBA = actual.at(i);
        // the __FILE__ __LINE__ output is compiler dependent, skip it
        if (actualLineBA.startsWith("   Loc: [") && actualLineBA.endsWith(")]"))
            continue;
        if (actualLineBA.endsWith(" : failure location"))
            continue;

        if (actualLineBA.startsWith("Config: Using QtTest library") // Text build string
            || actualLineBA.startsWith("    <QtBuild") // XML, Light XML build string
            || (actualLineBA.startsWith("    <property value=") &&  actualLineBA.endsWith("name=\"QtBuild\"/>"))) { // XUNIT-XML build string
            continue;
        }

        QString actualLine = QString::fromLatin1(actualLineBA);
        QString expectedLine = QString::fromLatin1(expected.at(i));
        expectedLine.replace(qtVersionPlaceHolder(), qtVersion);

        // Special handling for ignoring _FILE_ and _LINE_ if logger is teamcity
        if (logger.endsWith(QLatin1String("teamcity"))) {
            static QRegularExpression teamcityLocRegExp("\\|\\[Loc: .*\\(\\d*\\)\\|\\]");
            actualLine.replace(teamcityLocRegExp, teamCityLocation());
            expectedLine.replace(teamcityLocRegExp, teamCityLocation());
        }

        if (logger.endsWith(QLatin1String("tap"))) {
            if (expectedLine.contains(QLatin1String("at:"))
                || expectedLine.contains(QLatin1String("file:"))
                || expectedLine.contains(QLatin1String("line:")))
                actualLine = expectedLine;
        }

        if (!compareLine(logger, subdir, benchmark, actualLine,
                         expectedLine, errorMessage)) {
            errorMessage->prepend(QLatin1String("Line ") + QString::number(i + 1)
                                  + QLatin1String(": "));
            return false;
        }

        benchmark = actualLineBA.startsWith("RESULT : ");
    }
    return true;
}

bool tst_Selftests::compareLine(const QString &logger, const QString &subdir,
                                bool benchmark,
                                const QString &actualLine, const QString &expectedLine,
                                QString *errorMessage) const
{
    if (actualLine == expectedLine)
        return true;

    if ((subdir == QLatin1String("assert")
         || subdir == QLatin1String("faildatatype") || subdir == QLatin1String("failfetchtype"))
        && actualLine.contains(QLatin1String("ASSERT: "))
        && expectedLine.contains(QLatin1String("ASSERT: "))) {
        // Q_ASSERT uses __FILE__, the exact contents of which are
        // undefined. If have we something that looks like a Q_ASSERT and we
        // were expecting to see a Q_ASSERT, we'll skip the line.
        return true;
    }

    if (expectedLine.startsWith(QLatin1String("FAIL!  : tst_Exception::throwException() Caught unhandled exce"))) {
        // On some platforms we compile without RTTI, and as a result we never throw an exception
        if (actualLine.simplified() != QLatin1String("tst_Exception::throwException()")) {
            *errorMessage = QString::fromLatin1("'%1' != 'tst_Exception::throwException()'").arg(actualLine);
            return false;
        }
        return true;
    }

    if (benchmark || actualLine.startsWith(QLatin1String("<BenchmarkResult"))
        || (logger == QLatin1String("csv") && actualLine.startsWith(QLatin1Char('"')))) {
        // Don't do a literal comparison for benchmark results, since
        // results have some natural variance.
        QString error;
        BenchmarkResult actualResult = BenchmarkResult::parse(actualLine, &error);
        if (!error.isEmpty()) {
            *errorMessage = QString::fromLatin1("Actual line didn't parse as benchmark result: %1\nLine: %2").arg(error, actualLine);
            return false;
        }
        BenchmarkResult expectedResult = BenchmarkResult::parse(expectedLine, &error);
        if (!error.isEmpty()) {
            *errorMessage = QString::fromLatin1("Expected line didn't parse as benchmark result: %1\nLine: %2").arg(error, expectedLine);
            return false;
        }
        return compareBenchmarkResult(actualResult, expectedResult, errorMessage);
    }

    if (actualLine.startsWith(QLatin1String("    <Duration msecs="))
        || actualLine.startsWith(QLatin1String("<Duration msecs="))) {
        static QRegularExpression durationRegExp("<Duration msecs=\"[\\d\\.]+\"/>");
        QRegularExpressionMatch match = durationRegExp.match(actualLine);
        if (match.hasMatch())
            return true;
        *errorMessage = QString::fromLatin1("Invalid Duration tag: '%1'").arg(actualLine);
        return false;
    }

    if (actualLine.startsWith(QLatin1String("Totals:")) && expectedLine.startsWith(QLatin1String("Totals:")))
        return true;

    const QLatin1String pointerPlaceholder("_POINTER_");
    if (expectedLine.contains(pointerPlaceholder)
        && (expectedLine.contains(QLatin1String("Signal: "))
            || expectedLine.contains(QLatin1String("Slot: ")))) {
        QString actual = actualLine;
        // We don't care about the pointer of the object to whom the signal belongs, so we
        // replace it with _POINTER_, e.g.:
        // Signal: SignalSlotClass(7ffd72245410) signalWithoutParameters ()
        // Signal: QThread(7ffd72245410) started ()
        // After this instance pointer we may have further pointers and
        // references (with an @ prefix) as parameters of the signal or
        // slot being invoked.
        // Signal: SignalSlotClass(_POINTER_) qStringRefSignal ((QString&)@55f5fbb8dd40)
        actual.replace(QRegularExpression("\\b[a-f0-9]{8,}\\b"), pointerPlaceholder);
        // Also change QEventDispatcher{Glib,Win32,etc.} to QEventDispatcherPlatform
        actual.replace(QRegularExpression("\\b(QEventDispatcher)\\w+\\b"), QLatin1String("\\1Platform"));
        if (actual != expectedLine) {
          *errorMessage = msgMismatch(actual, expectedLine);
          return false;
        }
        return true;
    }

    if (EmulationDetector::isRunningArmOnX86() && subdir == QLatin1String("float")) {
        // QEMU cheats at qfloat16, so outputs it as if it were a float.
        if (actualLine.endsWith(QLatin1String("Actual   (operandLeft) : 0.001"))
            && expectedLine.endsWith(QLatin1String("Actual   (operandLeft) : 0.000999"))) {
            return true;
        }
    }

    *errorMessage = msgMismatch(actualLine, expectedLine);
    return false;
}

bool tst_Selftests::checkXml(const QString &logger, QByteArray xml,
                             QString *errorMessage) const
{
    // lightxml intentionally skips the root element, which technically makes it
    // not valid XML.
    // We'll add that ourselves for the purpose of validation.
    if (logger.endsWith(QLatin1String("lightxml"))) {
        xml.prepend("<root>");
        xml.append("</root>");
    }

    QXmlStreamReader reader(xml);
    while (!reader.atEnd())
        reader.readNext();

    if (reader.hasError()) {
        const int lineNumber = int(reader.lineNumber());
        const QByteArray line = xml.split('\n').value(lineNumber - 1);
        *errorMessage = QString::fromLatin1("line %1, col %2 '%3': %4")
                        .arg(lineNumber).arg(reader.columnNumber())
                        .arg(QString::fromLatin1(line), reader.errorString());
        return false;
    }
    return true;
}

#endif // QT_CONFIG(process)

void tst_Selftests::runSubTest()
{
#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    QFETCH(QString, subdir);
    QFETCH(QStringList, loggers);
    QFETCH(QStringList, arguments);
    QFETCH(bool, crashes);

    doRunSubTest(subdir, loggers, arguments, crashes);
#endif // QT_CONFIG(process)
}

// attribute must contain ="
QString extractXmlAttribute(const QString &line, const char *attribute)
{
    int index = line.indexOf(attribute);
    if (index == -1)
        return QString();
    const int attributeLength = int(strlen(attribute));
    const int end = line.indexOf('"', index + attributeLength);
    if (end == -1)
        return QString();

    const QString result = line.mid(index + attributeLength, end - index - attributeLength);
    if (result.isEmpty())
        return ""; // ensure empty but not null
    return result;
}

// Parse line into the BenchmarkResult it represents.
BenchmarkResult BenchmarkResult::parse(QString const& line, QString* error)
{
    if (error) *error = QString();
    BenchmarkResult out;

    QString remaining = line.trimmed();

    if (remaining.isEmpty()) {
        if (error) *error = "Line is empty";
        return out;
    }

    if (line.startsWith("<BenchmarkResult ")) {
        // XML result
        // format:
        //   <BenchmarkResult metric="$unit" tag="$tag" value="$total" iterations="$iterations" />
        if (!line.endsWith("/>")) {
            if (error) *error = "unterminated XML";
            return out;
        }

        QString unit = extractXmlAttribute(line, " metric=\"");
        QString sTotal = extractXmlAttribute(line, " value=\"");
        QString sIterations = extractXmlAttribute(line, " iterations=\"");
        if (unit.isNull() || sTotal.isNull() || sIterations.isNull()) {
            if (error) *error = "XML snippet did not contain all required values";
            return out;
        }

        bool ok;
        double total = sTotal.toDouble(&ok);
        if (!ok) {
            if (error) *error = sTotal + " is not a valid number";
            return out;
        }
        double iterations = sIterations.toDouble(&ok);
        if (!ok) {
            if (error) *error = sIterations + " is not a valid number";
            return out;
        }

        out.unit = unit;
        out.total = total;
        out.iterations = iterations;
        return out;
    }

    if (line.startsWith('"')) {
        // CSV result
        // format:
        //  "function","[globaltag:]tag","metric",value_per_iteration,total,iterations
        QStringList split = line.split(',');
        if (split.count() != 6) {
            if (error) *error = QString("Wrong number of columns (%1)").arg(split.count());
            return out;
        }

        bool ok;
        double total = split.at(4).toDouble(&ok);
        if (!ok) {
            if (error) *error = split.at(4) + " is not a valid number";
            return out;
        }
        double iterations = split.at(5).toDouble(&ok);
        if (!ok) {
            if (error) *error = split.at(5) + " is not a valid number";
            return out;
        }

        out.unit = split.at(2);
        out.total = total;
        out.iterations = iterations;
        return out;
    }

    // Text result
    // This code avoids using a QRegExp because QRegExp might be broken.
    // Sample format: 4,000 msec per iteration (total: 4,000, iterations: 1)

    QString sFirstNumber;
    while (!remaining.isEmpty() && !remaining.at(0).isSpace()) {
        sFirstNumber += remaining.at(0);
        remaining.remove(0,1);
    }
    remaining = remaining.trimmed();

    // 4,000 -> 4000
    sFirstNumber.remove(',');

    // Should now be parseable as floating point
    bool ok;
    double firstNumber = sFirstNumber.toDouble(&ok);
    if (!ok) {
        if (error) *error = sFirstNumber + " (at beginning of line) is not a valid number";
        return out;
    }

    // Remaining: msec per iteration (total: 4000, iterations: 1)
    static const char periterbit[] = " per iteration (total: ";
    QString unit;
    while (!remaining.startsWith(periterbit) && !remaining.isEmpty()) {
        unit += remaining.at(0);
        remaining.remove(0,1);
    }
    if (remaining.isEmpty()) {
        if (error) *error = "Could not find pattern: '<unit> per iteration (total: '";
        return out;
    }

    remaining = remaining.mid(sizeof(periterbit)-1);

    // Remaining: 4,000, iterations: 1)
    static const char itersbit[] = ", iterations: ";
    QString sTotal;
    while (!remaining.startsWith(itersbit) && !remaining.isEmpty()) {
        sTotal += remaining.at(0);
        remaining.remove(0,1);
    }
    if (remaining.isEmpty()) {
        if (error) *error = "Could not find pattern: '<number>, iterations: '";
        return out;
    }

    remaining = remaining.mid(sizeof(itersbit)-1);

    // 4,000 -> 4000
    sTotal.remove(',');

    double total = sTotal.toDouble(&ok);
    if (!ok) {
        if (error) *error = sTotal + " (total) is not a valid number";
        return out;
    }

    // Remaining: 1)
    QString sIters;
    while (remaining != QLatin1String(")") && !remaining.isEmpty()) {
        sIters += remaining.at(0);
        remaining.remove(0,1);
    }
    if (remaining.isEmpty()) {
        if (error) *error = "Could not find pattern: '<num>)'";
        return out;
    }
    qint64 iters = sIters.toLongLong(&ok);
    if (!ok) {
        if (error) *error = sIters + " (iterations) is not a valid integer";
        return out;
    }

    double calcFirstNumber = double(total)/double(iters);
    if (!qFuzzyCompare(firstNumber, calcFirstNumber)) {
        if (error) *error = QString("total/iters is %1, but benchlib output result as %2").arg(calcFirstNumber).arg(firstNumber);
        return out;
    }

    out.total = total;
    out.unit = unit;
    out.iterations = iters;
    return out;
}

void tst_Selftests::cleanup()
{
    QFETCH(QStringList, loggers);

    // Remove the test output files
    for (int i = 0; i < loggers.count(); ++i) {
        QString logFileName = logName(loggers[i]);
        if (!logFileName.isEmpty()) {
            QFile logFile(logFileName);
            if (logFile.exists())
                QVERIFY2(logFile.remove(), qPrintable(QString::fromLatin1("Cannot remove file '%1': %2: ").arg(logFileName, logFile.errorString())));
        }
    }
}

QTEST_MAIN(tst_Selftests)

#include "tst_selftests.moc"
