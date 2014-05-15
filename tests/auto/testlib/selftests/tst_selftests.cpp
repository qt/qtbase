/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore/QCoreApplication>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

#include <private/cycle_p.h>

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
    QString logName(const QString &logger) const;
    QList<LoggerSet> allLoggerSets() const;

    QTemporaryDir tempDir;
    QRegularExpression durationRegExp;
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

QT_BEGIN_NAMESPACE
namespace QTest
{
template <>
inline bool qCompare
    (BenchmarkResult const &r1, BenchmarkResult const &r2,
     const char* actual, const char* expected, const char* file, int line)
{
    // First make sure the iterations and unit match.
    if (r1.iterations != r2.iterations || r1.unit != r2.unit) {
        // Nope - compare whole string for best failure message
        return qCompare(r1.toString(), r2.toString(), actual, expected, file, line);
    }

    // Now check the value.  Some variance is allowed, and how much depends on
    // the measured unit.
    qreal variance = 0.;
    if (r1.unit == "msecs" || r1.unit == "WalltimeMilliseconds") {
        variance = 0.1;
    }
    else if (r1.unit == "instruction reads") {
        variance = 0.001;
    }
    else if (r1.unit == "CPU ticks" || r1.unit == "CPUTicks") {
        variance = 0.001;
    }
    if (variance == 0.) {
        // No variance allowed - compare whole string
        return qCompare(r1.toString(), r2.toString(), actual, expected, file, line);
    }

    if (qAbs(qreal(r1.total) - qreal(r2.total)) <= qreal(r1.total)*variance) {
        return compare_helper(true, 0, 0, 0, actual, expected, file, line);
    }

    // Whoops, didn't match.  Compare the whole string for the most useful failure message.
    return qCompare(r1.toString(), r2.toString(), actual, expected, file, line);
}
}
QT_END_NAMESPACE

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

// Load the expected test output for the nominated test (subdir) and logger
// as an array of lines.  If there is no expected output file, return an
// empty array.
static QList<QByteArray> expectedResult(const QString &subdir, const QString &logger)
{
    QFile file(":/expected_" + subdir + "." + logFormat(logger));
    if (!file.open(QIODevice::ReadOnly))
        return QList<QByteArray>();
    return splitLines(file.readAll());
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
        << LoggerSet("old stdout xunitxml",
                     QStringList() << "stdout xunitxml",
                     QStringList() << "-xunitxml"
                    )
        << LoggerSet("old xunitxml",
                     QStringList() << "xunitxml",
                     QStringList() << "-xunitxml" << "-o" << logName("xunitxml")
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
        << LoggerSet("new stdout xunitxml",
                     QStringList() << "stdout xunitxml",
                     QStringList() << "-o" << "-,xunitxml"
                    )
        << LoggerSet("new xunitxml",
                     QStringList() << "xunitxml",
                     QStringList() << "-o" << logName("xunitxml")+",xunitxml"
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
        << LoggerSet("txt + xunitxml",
                     QStringList() << "txt" << "xunitxml",
                     QStringList() << "-o" << logName("txt")+",txt"
                                   << "-o" << logName("xunitxml")+",xunitxml"
                    )
        << LoggerSet("lightxml + stdout xunitxml",
                     QStringList() << "lightxml" << "stdout xunitxml",
                     QStringList() << "-o" << logName("lightxml")+",lightxml"
                                   << "-o" << "-,xunitxml"
                    )
        // All loggers at the same time (except csv)
        << LoggerSet("all loggers",
                     QStringList() << "txt" << "xml" << "lightxml" << "stdout txt" << "xunitxml",
                     QStringList() << "-o" << logName("txt")+",txt"
                                   << "-o" << logName("xml")+",xml"
                                   << "-o" << logName("lightxml")+",lightxml"
                                   << "-o" << "-,txt"
                                   << "-o" << logName("xunitxml")+",xunitxml"
                    )
    ;
}

tst_Selftests::tst_Selftests()
    : tempDir(QDir::tempPath() + "/tst_selftests.XXXXXX")
    , durationRegExp("<Duration msecs=\"[\\d\\.]+\"/>")
{}

void tst_Selftests::initTestCase()
{
    //Detect the location of the sub programs
    QString subProgram = QLatin1String("float/float");
#if defined(Q_OS_WIN)
    subProgram = QLatin1String("float/float.exe");
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
//        << "alive"    // timer dependent
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
        << "failinit"
        << "failinitdata"
#if !defined(Q_OS_WIN)
        // Disable this test on Windows, as the run-time will popup dialogs with warnings
        << "fetchbogus"
#endif
        << "findtestdata"
        << "float"
        << "globaldata"
        << "longstring"
        << "maxwarnings"
        << "multiexec"
        << "printdatatags"
        << "printdatatagswithglobaltags"
        << "qexecstringlist"
        << "silent"
        << "singleskip"
        << "skip"
        << "skipcleanup"
        << "skipinit"
        << "skipinitdata"
        << "sleep"
        << "strcmp"
        << "subtest"
        << "verbose1"
        << "verbose2"
#ifndef QT_NO_EXCEPTIONS
        // this test will test nothing if the exceptions are disabled
        << "verifyexceptionthrown"
#endif //!QT_NO_EXCEPTIONS
        << "warnings"
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
            QStringList arguments = loggerSet.arguments;
            if (subtest == "commandlinedata") {
                arguments << QString("fiveTablePasses fiveTablePasses:fiveTablePasses_data1 -v2").split(' ');
            }
            else if (subtest == "benchlibcallgrind") {
                arguments << "-callgrind";
            }
            else if (subtest == "benchlibeventcounter") {
                arguments << "-eventcounter";
            }
            else if (subtest == "benchliboptions") {
                arguments << "-eventcounter";
            }
            else if (subtest == "benchlibtickcounter") {
                arguments << "-tickcounter";
            }
            else if (subtest == "badxml") {
                arguments << "-eventcounter";
            }
            else if (subtest == "benchlibcounting") {
                arguments << "-eventcounter";
            }
            else if (subtest == "printdatatags") {
                arguments << "-datatags";
            }
            else if (subtest == "printdatatagswithglobaltags") {
                arguments << "-datatags";
            }
            else if (subtest == "silent") {
                arguments << "-silent";
            }
            else if (subtest == "verbose1") {
                arguments << "-v1";
            }
            else if (subtest == "verbose2") {
                arguments << "-v2";
            }


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

            const bool crashes = subtest == QLatin1String("assert") || subtest == QLatin1String("exceptionthrow")
                || subtest == QLatin1String("fetchbogus") || subtest == QLatin1String("crashedterminate")
                || subtest == QLatin1String("crashes") || subtest == QLatin1String("silent");
            QTest::newRow(qPrintable(QString("%1 %2").arg(subtest).arg(loggerSet.name)))
                << subtest
                << loggers
                << arguments
                << crashes
            ;
        }
    }
}

#ifndef QT_NO_PROCESS

static QProcessEnvironment processEnvironment()
{
    static QProcessEnvironment result;
    if (result.isEmpty()) {
        const QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();
        foreach (const QString &key, systemEnvironment.keys()) {
            const bool useVariable = key == QLatin1String("PATH") || key == QLatin1String("QT_QPA_PLATFORM")
#if defined(Q_OS_QNX)
                || key == QLatin1String("GRAPHICS_ROOT") || key == QLatin1String("TZ")
#elif defined(Q_OS_UNIX)
                || key == QLatin1String("HOME") || key == QLatin1String("USER") // Required for X11 on openSUSE
#  if !defined(Q_OS_MAC)
                || key == QLatin1String("DISPLAY") || key == QLatin1String("XAUTHLOCALHOSTNAME")
                || key.startsWith(QLatin1String("XDG_"))
#  endif // !Q_OS_MAC
#endif // Q_OS_UNIX
#ifdef __COVERAGESCANNER__
                || key == QLatin1String("QT_TESTCOCOON_ACTIVE")
#endif
                ;
            if (useVariable)
                result.insert(key, systemEnvironment.value(key));
        }
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
    if (arguments.contains("-callgrind")) {
        QProcess checkProcess;
        QStringList args;
        args << QLatin1String("--version");
        checkProcess.start(QLatin1String("valgrind"), args);
        if (!checkProcess.waitForFinished(-1))
            QSKIP(QString("Valgrind broken or not available. Not running %1 test!").arg(subdir).toLocal8Bit());
    }
#endif

    QProcess proc;
    static const QProcessEnvironment environment = processEnvironment();
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
        && subdir != QLatin1String("xunit")
#ifdef Q_CC_MINGW
        && subdir != QLatin1String("silent") // calls qFatal()
#endif
        && subdir != QLatin1String("benchlibcallgrind"))
        QVERIFY2(err.isEmpty(), err.constData());

    for (int n = 0; n < loggers.count(); ++n) {
        QString logger = loggers[n];
        QList<QByteArray> res = splitLines(actualOutputs[n]);
        QList<QByteArray> exp = expectedResult(subdir, logger);
#if defined (Q_CC_MSVC) || defined(Q_CC_MINGW)
        // MSVC, MinGW format double numbers differently
        if (n == 0 && subdir == QStringLiteral("float")) {
            for (int i = 0; i < exp.size(); ++i) {
                exp[i].replace("e-07", "e-007");
                exp[i].replace("e+07", "e+007");
            }
        }
#endif

        // For the "crashes" test, there are multiple versions of the
        // expected output.  Load the one with the same line count as
        // the actual output.
        if (exp.count() == 0) {
            QList<QList<QByteArray> > expArr;
            QList<QByteArray> tmp;
            int i = 1;
            do {
                tmp = expectedResult(subdir + QString("_%1").arg(i++), logger);
                if (tmp.count())
                    expArr += tmp;
            } while (tmp.count());

            for (int j = 0; j < expArr.count(); ++j) {
                if (res.count() == expArr.at(j).count()) {
                    exp = expArr.at(j);
                    break;
                }
            }

            if (expArr.count()) {
                QVERIFY2(exp.count(),
                         qPrintable(QString::fromLatin1("None of the expected output files for "
                                                        "%1 format has matching line count.")
                                    .arg(loggers.at(n))));
            }
        } else {
            if (res.count() != exp.count()) {
                qDebug() << "<<<<<<";
                foreach (const QByteArray &line, res)
                    qDebug() << line;
                qDebug() << "======";
                foreach (const QByteArray &line, exp)
                    qDebug() << line;
                qDebug() << ">>>>>>";

                QVERIFY2(res.count() == exp.count(),
                     qPrintable(QString::fromLatin1("Mismatch in line count: %1 != %2 (%3).")
                                .arg(res.count()).arg(exp.count()).arg(loggers.at(n))));
            }
        }

        // By this point, we should have loaded a non-empty expected data file.
        QVERIFY2(exp.count(),
                 qPrintable(QString::fromLatin1("Expected test data for %1 format is empty or not found.")
                            .arg(loggers.at(n))));

        // For xml output formats, verify that the log is valid XML.
        if (logFormat(logger) == "xunitxml" || logFormat(logger) == "xml" || logFormat(logger) == "lightxml") {
            QByteArray xml(actualOutputs[n]);
            // lightxml intentionally skips the root element, which technically makes it
            // not valid XML.
            // We'll add that ourselves for the purpose of validation.
            if (logFormat(logger) == "lightxml") {
                xml.prepend("<root>");
                xml.append("</root>");
            }

            QXmlStreamReader reader(xml);

            while (!reader.atEnd())
                reader.readNext();

            QVERIFY2(!reader.error(), qPrintable(QString("line %1, col %2: %3")
                .arg(reader.lineNumber())
                .arg(reader.columnNumber())
                .arg(reader.errorString())
            ));
        }

        // Verify that the actual output is an acceptable match for the
        // expected output.
        bool benchmark = false;
        for (int i = 0; i < res.count(); ++i) {
            QByteArray line = res.at(i);
            // the __FILE__ __LINE__ output is compiler dependent, skip it
            if (line.startsWith("   Loc: [") && line.endsWith(")]"))
                continue;
            if (line.endsWith(" : failure location"))
                continue;

            const QString output(QString::fromLatin1(line));
            const QString expected(QString::fromLatin1(exp.at(i)).replace("@INSERT_QT_VERSION_HERE@", QT_VERSION_STR));

            if (subdir == "assert" && output.contains("ASSERT: ") && expected.contains("ASSERT: ") && output != expected)
                // Q_ASSERT uses __FILE__, the exact contents of which are
                // undefined.  If we something that looks like a Q_ASSERT and we
                // were expecting to see a Q_ASSERT, we'll skip the line.
                continue;
            else if (expected.startsWith(QLatin1String("FAIL!  : tst_Exception::throwException() Caught unhandled exce")) && expected != output)
                // On some platforms we compile without RTTI, and as a result we never throw an exception.
                QCOMPARE(output.simplified(), QString::fromLatin1("tst_Exception::throwException()").simplified());
            else if (benchmark || line.startsWith("<BenchmarkResult") || (logFormat(logger) == "csv" && line.startsWith('"'))) {
                // Don't do a literal comparison for benchmark results, since
                // results have some natural variance.
                QString error;

                BenchmarkResult actualResult = BenchmarkResult::parse(output, &error);
                QVERIFY2(error.isEmpty(), qPrintable(QString("Actual line didn't parse as benchmark result: %1\nLine: %2").arg(error).arg(output)));

                BenchmarkResult expectedResult = BenchmarkResult::parse(expected, &error);
                QVERIFY2(error.isEmpty(), qPrintable(QString("Expected line didn't parse as benchmark result: %1\nLine: %2").arg(error).arg(expected)));

                QCOMPARE(actualResult, expectedResult);
            } else if (line.startsWith("    <Duration msecs=") || line.startsWith("<Duration msecs=")) {
                QRegularExpressionMatch match = durationRegExp.match(line);
                QVERIFY2(match.hasMatch(), qPrintable(QString::fromLatin1("Invalid Duration tag at line %1 (%2): '%3'")
                                                      .arg(i).arg(loggers.at(n), output)));
            } else {
                QVERIFY2(output == expected,
                         qPrintable(QString::fromLatin1("Mismatch at line %1 (%2): '%3' != '%4'")
                                    .arg(i).arg(loggers.at(n), output, expected)));
            }

            benchmark = line.startsWith("RESULT : ");
        }
    }
}

#endif // !QT_NO_PROCESS

void tst_Selftests::runSubTest()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
    QFETCH(QString, subdir);
    QFETCH(QStringList, loggers);
    QFETCH(QStringList, arguments);
    QFETCH(bool, crashes);

    doRunSubTest(subdir, loggers, arguments, crashes);
#endif // !QT_NO_PROCESS
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
