// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>

QT_REQUIRE_CONFIG(process);

#if QT_CONFIG(temporaryfile)
#  define USE_DIFF
#  include <QtCore/QTemporaryFile>
#  include <QtCore/QStandardPaths>
#endif

#include <QtCore/QXmlStreamReader>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>

#include <QTest>

#include <QProcess>

#include <private/cycle_p.h>

#include <QtTest/private/qemulationdetector_p.h>

using namespace Qt::StringLiterals;

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

static QString teamCityLocation() { return QStringLiteral("|[Loc: _FILE_(_LINE_)|]"); }
static QString qtVersionPlaceHolder() { return QStringLiteral("@INSERT_QT_VERSION_HERE@"); }

// Forward declarations
bool compareLine(const QString &logger, const QString &subdir, bool benchmark,
    const QString &actualLine, const QString &expectedLine, QString *errorMessage);
bool checkXml(const QString &logger, QByteArray xml, QString *errorMessage);

bool compareOutput(const QString &logger, const QString &subdir,
                                  const QByteArray &rawOutput, const QByteArrayList &actual,
                                  const QByteArrayList &expected,
                                  QString *errorMessage)
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
        if (actualLineBA.endsWith(" : message location"))
            continue;

        if (actualLineBA.startsWith("Config: Using QtTest library") // Text build string
            || actualLineBA.startsWith("    <QtBuild") // XML, Light XML build string
            || (actualLineBA.startsWith("    <property name=\"QtBuild\" value="))) { // JUnit-XML build string
            continue;
        }

        QString actualLine = QString::fromLatin1(actualLineBA);
        QString expectedLine = QString::fromLatin1(expected.at(i));
        expectedLine.replace(qtVersionPlaceHolder(), qtVersion);

        if (logger.endsWith(QLatin1String("junitxml"))) {
            static QRegularExpression timestampRegex("timestamp=\".*?\"");
            actualLine.replace(timestampRegex, "timestamp=\"@TEST_START_TIME@\"");
            static QRegularExpression timeRegex("time=\".*?\"");
            actualLine.replace(timeRegex, "time=\"@TEST_DURATION@\"");
            static QRegularExpression hostnameRegex("hostname=\".*?\"");
            actualLine.replace(hostnameRegex, "hostname=\"@HOSTNAME@\"");
        }

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

bool compareLine(const QString &logger, const QString &subdir,
                                bool benchmark,
                                const QString &actualLine, const QString &expectedLine,
                                QString *errorMessage)
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

    if (actualLine.contains(QLatin1String("<Duration msecs="))) {
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

    if (QTestPrivate::isRunningArmOnX86() && subdir == QLatin1String("float")) {
        // QEMU cheats at qfloat16, so outputs it as if it were a float.
        if (actualLine.endsWith(QLatin1String("Actual   (operandLeft) : 0.001"))
            && expectedLine.endsWith(QLatin1String("Actual   (operandLeft) : 0.000999"))) {
            return true;
        }
    }

    *errorMessage = msgMismatch(actualLine, expectedLine);
    return false;
}

bool checkXml(const QString &logger, QByteArray xml, QString *errorMessage)
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
        if (split.size() != 6) {
            if (error) *error = QString("Wrong number of columns (%1)").arg(split.size());
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

    const auto begin = remaining.cbegin();
    auto it = std::find_if(begin, remaining.cend(), [](const auto ch) {
        return ch.isSpace();
    });
    QString sFirstNumber{std::distance(begin, it), Qt::Uninitialized};
    std::move(begin, it, sFirstNumber.begin());
    remaining.erase(begin, it);
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

// ----------------------------------------------------------------------

#include "catch_p.h"
#include <QtTest/private/qtestlog_p.h>

#if defined(Q_OS_MACOS)
#include <QtCore/private/qcore_mac_p.h>
#endif

enum RebaseMode { NoRebase, RebaseMissing, RebaseFailing, RebaseAll };
static RebaseMode rebaseMode = NoRebase;

static QTemporaryDir testOutputDir(QDir::tempPath() + "/tst_selftests.XXXXXX");

enum ArgumentStyle { NewStyleArgument, OldStyleArguments };
enum OutputMode { FileOutput, StdoutOutput };

struct TestLogger
{
    TestLogger(QTestLog::LogMode logger) : logger(logger) {}

    TestLogger(QTestLog::LogMode logger, ArgumentStyle argumentStyle)
        : logger(logger), argumentStyle(argumentStyle) {}
    TestLogger(QTestLog::LogMode logger, OutputMode outputMode)
        : logger(logger), outputMode(outputMode) {}

    TestLogger(QTestLog::LogMode logger, OutputMode outputMode, ArgumentStyle argumentStyle)
        : logger(logger), outputMode(outputMode), argumentStyle(argumentStyle) {}

    QString shortName() const
    {
        if (logger == QTestLog::Plain)
            return "txt";

        auto loggers = QMetaEnum::fromType<QTestLog::LogMode>();
        return QString(loggers.valueToKey(logger)).toLower();
    }

    QString outputFileName(const QString &test) const
    {
        return testOutputDir.filePath("output_" + test +
            (outputMode == StdoutOutput ? ".stdout" : "") +
            "." + shortName());
    }

    QString expectationFileName(const QString &test, int version = 0) const
    {
        auto fileName = "expected_" + test;
        if (version)
            fileName += QString("_%1").arg(version);
        fileName += "." + shortName();
        return fileName;
    }

    QStringList arguments(const QString &test) const
    {
        QStringList arguments;
        if (argumentStyle == NewStyleArgument) {
            arguments << "-o" << (outputMode == FileOutput
                ? outputFileName(test) : QStringLiteral("-")) +
                "," + shortName();
        } else {
            arguments << "-" + shortName();
            if (outputMode == FileOutput)
                arguments << "-o" << outputFileName(test);
        }

        return arguments;
    }

    QByteArray testOutput(const QString &test) const
    {
        QFile outputFile(outputFileName(test));
        REQUIRE(outputFile.exists());
        REQUIRE(outputFile.open(QIODevice::ReadOnly));
        return outputFile.readAll();
    }

    bool shouldIgnoreTest(const QString &test) const;

    operator QTestLog::LogMode() const { return logger; }

    QTestLog::LogMode logger;
    OutputMode outputMode = FileOutput;
    ArgumentStyle argumentStyle = NewStyleArgument;
};

bool TestLogger::shouldIgnoreTest(const QString &test) const
{
#if defined(QT_USE_APPLE_UNIFIED_LOGGING)
    if (logger == QTestLog::Apple)
        return true;
#endif

    if (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        qDebug() << "TestLogger::shouldIgnoreTest() ignore" << test << "on wayland/xwayland!";
        return true;
    }

    // These tests are affected by timing and whether the CPU tick counter
    // is monotonically increasing. They won't work on some machines so
    // leave them off by default. Feel free to enable them for your own
    // testing by setting the QTEST_ENABLE_EXTRA_SELFTESTS environment
    // variable to something non-empty.
    static bool enableExtraTests = !qEnvironmentVariableIsEmpty("QTEST_ENABLE_EXTRA_SELFTESTS");
    if (!enableExtraTests && (test == "benchlibtickcounter" || test == "benchlibwalltime"))
        return true;

#if defined(Q_OS_WIN)
    // On windows, assert does nothing in release mode and blocks execution
    // with a popup window in debug mode, so skip tests that assert.
    if (test == "assert" || test == "faildatatype" || test == "failfetchtype"
        || test == "fetchbogus")
        return true;
#endif

#if defined(QT_NO_EXCEPTIONS) || defined(Q_OS_WIN)
    // Disable this test on Windows or for Intel compiler, as the run-times
    // will popup dialogs with warnings that uncaught exceptions were thrown
    if (test == "exceptionthrow")
        return true;
#endif

#if defined(QT_NO_EXCEPTIONS)
    // This test will test nothing if the exceptions are disabled
    if (test == "verifyexceptionthrown")
        return true;
#endif

    if (test == "benchlibcallgrind") {
#if defined(__GNUC__) && (defined(__i386) || defined(__x86_64)) && defined(Q_OS_LINUX)
        // Check that it's actually available
        QProcess checkProcess;
        QStringList args{u"--version"_s};
        checkProcess.start("valgrind", args);
        if (!checkProcess.waitForFinished(-1)) {
            WARN("Valgrind broken or not available. Not running benchlibcallgrind test!");
            return true;
        }
#else
        // Skip on platforms where callgrind is not available
        return true;
#endif
    }

    if (logger != QTestLog::Plain || outputMode == FileOutput) {
        // The following tests only work with plain text output to stdout,
        // either because they execute multiple test objects or because
        // they internally supply arguments to themselves.
        if (test == "differentexec"
            || test == "multiexec"
            || test == "qexecstringlist"
            || test == "benchliboptions"
            || test == "printdatatags"
            || test == "printdatatagswithglobaltags"
            || test == "silent")
            return true;

        // These tests produce variable output (callgrind because of #if-ery,
        // crashes by virtue of platform differences in where the output cuts
        // off), so only test them for one format, to avoid the need for several
        // _n variants for each format. Also, crashes can produce invalid XML.
        if (test == "crashes" || test == "benchlibcallgrind")
            return true;

        // this test prints out some floats in the testlog and the formatting is
        // platform-specific and hard to predict.
        if (test == "float")
            return true;

        // This test is quite slow, and running it for all the loggers
        // significantly increases the overall test time.  It does not really
        // relate to logging, so it should be safe to run it just for the stdout
        // loggers.
        if (test == "sleep")
            return true;
    }

    if (test == "badxml" && !(logger == QTestLog::XML
            || logger == QTestLog::LightXML || logger == QTestLog::JUnitXML))
        return true;

    // Skip benchmark for TeamCity logger, skip everything else for CSV:
    if (logger == (test.startsWith("benchlib") ? QTestLog::TeamCity : QTestLog::CSV))
        return true;

    if (logger != QTestLog::JUnitXML && test == "junit")
        return true;

    return false;
}

using TestLoggers = QList<TestLogger>;

// ----------------------- Output checking -----------------------

/*
    Check that the test doesn't produce any unexpected error output.

    Some tests may output unpredictable strings to stderr, which we'll ignore.

    For instance, uncaught exceptions on Windows might say (depending on Windows
    version and JIT debugger settings):
    "This application has requested the Runtime to terminate it in an unusual way.
    Please contact the application's support team for more information."

    Also, tests which use valgrind may generate warnings if the toolchain is
    newer than the valgrind version, such that valgrind can't understand the
    debug information on the binary.
*/
void checkErrorOutput(const QString &test, const QByteArray &errorOutput)
{
    if (test == "exceptionthrow"
        || test == "cmptest" // QImage comparison requires QGuiApplication
        || test == "fetchbogus"
        || test == "watchdog"
        || test == "junit"
        || test == "benchlibcallgrind")
        return;

#ifdef Q_CC_MINGW
    if (test == "silent") // calls qFatal()
        return;
#endif

#ifdef Q_OS_WIN
    if (test == "crashes")
        return; // Complains about uncaught exception
#endif

#ifdef Q_OS_UNIX
    if (test == "assert"
        || test == "crashes"
        || test == "failfetchtype"
        || test == "faildatatype")
    return; // Outputs "Received signal 6 (SIGABRT)"
#endif

#ifdef Q_OS_LINUX
    if (test == "silent") {
        if (QTestPrivate::isRunningArmOnX86())
            return;         // QEMU outputs to stderr about uncaught signals
    }
#endif

    INFO(errorOutput.toStdString());
    REQUIRE(errorOutput.isEmpty());
}

/*
    Removes any parts of the output that may vary between test runs.
*/
QByteArray sanitizeOutput(const QString &test, const QByteArray &output)
{
    QByteArray actual = output;

    if (test == "crashes") {
#if !defined(Q_OS_WIN)
        // Remove digits of times
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
#endif

#if defined(Q_OS_WIN)
        // Remove stack trace which is output to stdout
        const int exceptionLogStart = actual.indexOf("A crash occurred in ");
        if (exceptionLogStart >= 0)
            actual.truncate(exceptionLogStart);
#endif
    }

    return actual;
}

QByteArray readExpectationFile(const QString &fileName)
{
    QFile file(QStringLiteral(":/") + fileName);

    if (!file.exists() && rebaseMode != NoRebase) {
        // Try rebased test results
        file.setFileName(testOutputDir.filePath(fileName));
    }

    if (!file.exists())
        return QByteArray();

    CAPTURE(file.fileName());
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}

void checkTestOutput(const QString &test, const TestLogger &logger, const QByteArray &testOutput)
{
    REQUIRE(!testOutput.isEmpty());

    QByteArray actual = sanitizeOutput(test, testOutput);
    auto actualLines = splitLines(actual);

    QString outputMessage;
    bool expectationMatched = false;

    QString expectationFileName;

    // Rebases test results if the given mode has been enabled on the command line
    auto rebaseTestResult = [&](RebaseMode mode) {
        if (rebaseMode < mode)
            return false;

        QFile file(testOutputDir.filePath(expectationFileName));
        REQUIRE(file.open(QIODevice::WriteOnly));
        file.write(actual);

        expectationMatched = true;
        return true;
    };

    bool foundExpectionFile = false;
    for (int version = 0; !expectationMatched; ++version) {
        // Look for a test expectation file. Most tests only have a single
        // expectation file, while some have multiple versions that should
        // all be considered before failing the test.
        expectationFileName = logger.expectationFileName(test, version);

        if (rebaseTestResult(RebaseAll))
            break;

        const QByteArray expected = readExpectationFile(expectationFileName);
        if (expected.isEmpty()) {
            if (rebaseTestResult(RebaseMissing))
                break;

            if (!version) {
                // Look for version-specific expectations
                continue;
            } else {
                // No more versions found, and still no match
                assert(!expectationMatched);
                if (!foundExpectionFile)
                    outputMessage += "Could not find any expectation files for subtest '" + test + "'";
                break;
            }
        }

        // Found expected result
        foundExpectionFile = true;
        QString errorMessage;
        auto expectedLines = splitLines(expected);
        if (compareOutput(logger.shortName(), test, actual, actualLines, expectedLines, &errorMessage)) {
            expectationMatched = true;
        } else if (rebaseTestResult(RebaseFailing)) {
            break;
        } else {
            if (!outputMessage.isEmpty())
                outputMessage += "\n\n" + QString('-').repeated(80) + "\n";
            outputMessage += "\n" + errorMessage + "\n";
            outputMessage += "\nExpected (" + expectationFileName + "):\n" + expected;
            outputMessage += "\nActual:\n" + actual;
            const QByteArray diff = runDiff(expectedLines, actualLines);
            if (!diff.isEmpty())
                 outputMessage += "\nDiff:\n" + diff.trimmed();
        }
    }

    INFO(outputMessage.toStdString());
    CHECK(expectationMatched);
}

// ----------------------- Test running -----------------------

static QProcessEnvironment testEnvironment()
{
    static QProcessEnvironment environment;
    if (environment.isEmpty()) {
        const QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();
        const bool preserveLibPath = qEnvironmentVariableIsSet("QT_PRESERVE_TESTLIB_PATH");
        foreach (const QString &key, systemEnvironment.keys()) {
            const bool useVariable = key == "PATH" || key == "QT_QPA_PLATFORM"
                || key == "ASAN_OPTIONS"
#if defined(Q_OS_QNX)
                || key == "GRAPHICS_ROOT" || key == "TZ"
#elif defined(Q_OS_UNIX)
                || key == "HOME" || key == "USER" // Required for X11 on openSUSE
                || key == "QEMU_SET_ENV" || key == "QEMU_LD_PREFIX" // Required for QEMU
#  if !defined(Q_OS_MACOS)
                || key == "DISPLAY" || key == "XAUTHLOCALHOSTNAME"
                || key.startsWith("XDG_") || key == "XAUTHORITY"
#  endif // !Q_OS_MACOS
#endif // Q_OS_UNIX
#ifdef __COVERAGESCANNER__
                || key == "QT_TESTCOCOON_ACTIVE"
#endif
                || ( preserveLibPath && (key == "QT_PLUGIN_PATH"
                                        || key == "LD_LIBRARY_PATH"))
                ;
            if (useVariable)
                environment.insert(key, systemEnvironment.value(key));
        }
        // Avoid interference from any qtlogging.ini files, e.g. in /etc/xdg/QtProject/:
        environment.insert("QT_LOGGING_RULES", "*.debug=true;qt.*=false");

#if defined(Q_OS_UNIX)
        // Avoid the warning from QCoreApplication
        environment.insert("LC_ALL", "en_US.UTF-8");
#endif
    }
    return environment;
}

struct TestProcessResult
{
    int exitCode;
    QByteArray standardOutput;
    QByteArray errorOutput;
};

TestProcessResult runTestProcess(const QString &test, const QStringList &arguments)
{
    QProcessEnvironment environment = testEnvironment();

    const bool expectedCrash = test == "assert" || test == "exceptionthrow"
        || test == "fetchbogus" || test == "crashedterminate"
        || test == "faildatatype" || test == "failfetchtype"
        || test == "crashes" || test == "silent" || test == "watchdog";

    if (expectedCrash) {
        environment.insert("QTEST_DISABLE_CORE_DUMP", "1");
        environment.insert("QTEST_DISABLE_STACK_DUMP", "1");
        if (test == "watchdog")
            environment.insert("QTEST_FUNCTION_TIMEOUT", "100");
    }

    QProcess process;
    process.setProcessEnvironment(environment);
    const QString command = test + '/' + test;
    process.start(command, arguments);

    CAPTURE(command);
    INFO(environment.toStringList().join('\n').toStdString());

    bool startedSuccessfully = process.waitForStarted();
    bool finishedSuccessfully = process.waitForFinished();

    CAPTURE(process.errorString());
    REQUIRE(startedSuccessfully);
    REQUIRE(finishedSuccessfully);

    auto standardOutput = process.readAllStandardOutput();
    auto standardError = process.readAllStandardError();

    auto processCrashed = process.exitStatus() == QProcess::CrashExit;
    if (!expectedCrash && processCrashed) {
        INFO(standardOutput.toStdString());
        INFO(standardError.toStdString());
        REQUIRE(!processCrashed);
    }

    return { process.exitCode(), standardOutput, standardError };
}

/*
    Runs a single test and verifies the output against the expected results.
*/
void runTest(const QString &test, const TestLoggers &requestedLoggers)
{
    TestLoggers loggers;
    for (auto logger : requestedLoggers) {
        if (!logger.shouldIgnoreTest(test))
            loggers += logger;
    }

    if (loggers.isEmpty())
        return;

    QStringList arguments;
    for (auto logger : loggers)
        arguments += logger.arguments(test);

    CAPTURE(test);
    CAPTURE(arguments);

    auto testProcess = runTestProcess(test, arguments);

    checkErrorOutput(test, testProcess.errorOutput);

    for (auto logger : loggers) {
        QByteArray testOutput;
        if (logger.outputMode == StdoutOutput) {
            testOutput = testProcess.standardOutput;
            QFile file(logger.outputFileName(test));
            REQUIRE(file.open(QIODevice::WriteOnly));
            file.write(testOutput);
        } else {
            testOutput = logger.testOutput(test);
        }

        checkTestOutput(test, logger, testOutput);
    }
}

/*
    Runs a single test and verifies the output against the expected result.
*/
void runTest(const QString &test, const TestLogger &logger)
{
    runTest(test, TestLoggers{logger});
}

// ----------------------- Catch helpers -----------------------

template <typename T>
class QtMetaEnumGenerator : public Catch::Generators::IGenerator<T>
{
public:
    QtMetaEnumGenerator()
    {
        metaEnum = QMetaEnum::fromType<T>();
        next();
    }

    bool next() override
    {
        current = static_cast<T>(metaEnum.value(++index));
        return index < metaEnum.keyCount();
    }

    const T& get() const override { return current; }

private:
    QMetaEnum metaEnum;
    int index = -1;
    T current;
};

template <typename T>
Catch::Generators::GeneratorWrapper<T> enums()
{
    return Catch::Generators::GeneratorWrapper<T>(
        std::unique_ptr<Catch::Generators::IGenerator<T>>(
            new QtMetaEnumGenerator<T>()));
}

QT_BEGIN_NAMESPACE
template <typename T, typename A = typename std::enable_if<std::is_same<T, std::string>::value == false, void>::type>
std::ostream& operator<<(std::ostream &os, const T &value)
{
    QString output;
    QDebug debug(&output);
    debug.nospace() << value;
    os << output.toStdString();
    return os;
}
QT_END_NAMESPACE

// ----------------------- Test cases -----------------------

static const auto kBaselineTest = "pass";

bool isCommandLineLogger(QTestLog::LogMode logger)
{
#if defined(QT_USE_APPLE_UNIFIED_LOGGING)
    // The Apple logger is internal and never logs to file or stdout
    return logger != QTestLog::Apple;
#else
    Q_UNUSED(logger);
    return true;
#endif
}

bool isGenericCommandLineLogger(QTestLog::LogMode logger)
{
    // The CSV logger is only used for benchmarks
    return isCommandLineLogger(logger) && logger != QTestLog::CSV;
}

TEST_CASE("Loggers support both old and new style arguments")
{
    auto logger = GENERATE(filter(isGenericCommandLineLogger, enums<QTestLog::LogMode>()));

    GIVEN("The " << logger << " logger") {
        auto argumentStyle = GENERATE(OldStyleArguments, NewStyleArgument);
        WHEN("Passing arguments with " <<
            (argumentStyle == NewStyleArgument ? "new" : "old") << " style") {
            runTest(kBaselineTest, TestLogger(logger, argumentStyle));
        }
    }
}

TEST_CASE("Loggers can output to both file and stdout")
{
    auto logger = GENERATE(filter(isGenericCommandLineLogger, enums<QTestLog::LogMode>()));

    GIVEN("The " << logger << " logger") {
        auto outputMode = GENERATE(StdoutOutput, FileOutput);
        WHEN("Directing output to " << (outputMode == FileOutput ? "file" : "stdout")) {
            runTest(kBaselineTest, TestLogger(logger, outputMode));
        }
    }
}

TEST_CASE("Logging to file and stdout at the same time")
{
    auto loggerEnum = QMetaEnum::fromType<QTestLog::LogMode>();
    for (int i = 0; i < loggerEnum.keyCount(); ++i) {
        auto stdoutLogger = QTestLog::LogMode(loggerEnum.value(i));
        if (!isGenericCommandLineLogger(stdoutLogger))
            continue;

        for (int j = 0; j < loggerEnum.keyCount(); ++j) {
            auto fileLogger = QTestLog::LogMode(loggerEnum.value(j));
            if (!isGenericCommandLineLogger(fileLogger))
                continue;

            runTest(kBaselineTest, TestLoggers{
                TestLogger(fileLogger, FileOutput),
                TestLogger(stdoutLogger, StdoutOutput)
            });
        }
    }
}

TEST_CASE("All loggers can be enabled at the same time")
{
    TestLoggers loggers;

    auto loggerEnum = QMetaEnum::fromType<QTestLog::LogMode>();
    for (int i = 0; i < loggerEnum.keyCount(); ++i) {
        auto logger = QTestLog::LogMode(loggerEnum.value(i));
        if (!isGenericCommandLineLogger(logger))
            continue;

        loggers += TestLogger(logger, FileOutput);
    }

    runTest(kBaselineTest, loggers);
}

SCENARIO("Test output of the loggers is as expected")
{
    static QStringList tests = QString(QT_STRINGIFY(SUBPROGRAMS)).split(' ');

    auto logger = GENERATE(filter(isGenericCommandLineLogger, enums<QTestLog::LogMode>()));

    GIVEN("The " << logger << " logger") {
        for (QString test : tests) {
            AND_GIVEN("The " << test << " subtest") {
                runTest(test, TestLogger(logger, StdoutOutput));
            }
        }
    }
}

struct TestCase {
    int expectedExitCode;
    const char *cmdline;
};

SCENARIO("Exit code is as expected")
{
    // Listing of test command lines and expected exit codes
    // NOTE: Use at least 2 spaces to separate arguments because some contain a space themselves.
    const struct TestCase testCases[] = {
    // 'pass' is a test with no data tags at all
        { 0, "pass  testNumber1" },
        { 1, "pass  unknownFunction" },
        { 1, "pass  testNumber1:blah" },
        { 1, "pass  testNumber1:blah:blue" },
    // 'counting' is a test that has only local data tags
        { 0, "counting  testPassPass" },
        { 0, "counting  testPassPass:row 1" },
        { 1, "counting  testPassPass:blah" },
        { 1, "counting  testPassPass:blah:row 1" },
        { 1, "counting  testPassPass:blah:blue" },
    // 'globaldata' is a test with global and local data tags
        { 0, "globaldata  testGlobal" },
        { 0, "globaldata  testGlobal:global=true" },
        { 0, "globaldata  testGlobal:local=true" },
        { 0, "globaldata  testGlobal:global=true:local=true" },
        { 1, "globaldata  testGlobal:local=true:global=true" },
        { 1, "globaldata  testGlobal:global=true:blah" },
        { 1, "globaldata  testGlobal:blah:local=true" },
        { 1, "globaldata  testGlobal:blah:global=true" },
        { 1, "globaldata  testGlobal:blah" },
        { 1, "globaldata  testGlobal:blah:blue" },
    // Passing multiple testcase:data on the command line
        { 0, "globaldata  testGlobal:global=true  skipSingle:global=true:local=true" },
        { 1, "globaldata  testGlobal:blah         skipSingle:global=true:local=true" },
        { 1, "globaldata  testGlobal:global=true  skipSingle:blah" },
        { 2, "globaldata  testGlobal:blah         skipSingle:blue" },
    };

    size_t n_testCases = sizeof(testCases) / sizeof(*testCases);
    for (size_t i = 0; i < n_testCases; i++) {
        GIVEN("The command line: " << testCases[i].cmdline) {
            const QStringList cmdSplit = QString(testCases[i].cmdline)
                    .split(QRegularExpression("  +"));    // at least 2 spaces
            const QString test     = cmdSplit[0];
            const QStringList args = cmdSplit.sliced(1);
            auto runResult = runTestProcess(test, args);
            REQUIRE(runResult.exitCode == testCases[i].expectedExitCode);
        }
    }
}

// ----------------------- Entrypoint -----------------------

int main(int argc, char **argv)
{
    std::vector<const char*> args(argv, argv + argc);

    static auto kRebaseArgument = "--rebase";
    auto rebaseArgument = std::find_if(args.begin(), args.end(),
        [=](const char *arg) { return strncmp(arg, kRebaseArgument, 8) == 0; });
    if (rebaseArgument != args.end()) {
        QString mode((*rebaseArgument) + 8);
        if (mode == "=missing")
            rebaseMode = RebaseMissing;
        else if (mode.isEmpty() || mode == "=failing")
            rebaseMode = RebaseFailing;
        else if (mode == "=all")
            rebaseMode = RebaseAll;

        args.erase(rebaseArgument);
        argc = int(args.size());
        argv = const_cast<char**>(&args[0]);
    }

    QCoreApplication app(argc, argv);

    if (!testOutputDir.isValid())
        qFatal("Could not create temp directory: %s", qUtf8Printable(testOutputDir.errorString()));

    // Detect the location of the sub programs
    QString subProgram = "pass/pass";
#if defined(Q_OS_WIN)
    subProgram += ".exe";
#endif
    QString testdataDir = QFINDTESTDATA(subProgram);
    int testDataDirCutoff = testdataDir.lastIndexOf(subProgram);
    testdataDir = testDataDirCutoff > 0 ? testdataDir.left(testDataDirCutoff)
        : QCoreApplication::applicationDirPath();

    // Move into testdata path and execute tests relative to that
    if (!QDir::setCurrent(testdataDir))
        qFatal("Could not chdir to %s", qUtf8Printable(testdataDir));

    auto result = QTestPrivate::catchMain(argc, argv);

    if (result != 0 || rebaseMode != NoRebase) {
        // Note: Ctrl+C won't pass though here, so the test output won't be kept
        qDebug() << "Test outputs left in" << qUtf8Printable(testOutputDir.path());
        testOutputDir.setAutoRemove(false);
    }

    return result;
}

