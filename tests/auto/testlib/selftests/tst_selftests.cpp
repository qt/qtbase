/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtTest/QtTest>
#include <QtCore/QXmlStreamReader>
#include <private/cycle_p.h>

class tst_Selftests: public QObject
{
    Q_OBJECT
private slots:
    void runSubTest_data();
    void runSubTest();
    void cleanup();

private:
    void doRunSubTest(QString const& subdir, QString const& logger, QStringList const& arguments );
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
    if (r1.unit == "msec") {
        variance = 0.1;
    }
    else if (r1.unit == "instruction reads") {
        variance = 0.001;
    }
    else if (r1.unit == "ticks") {
        variance = 0.001;
    }
    if (variance == 0.) {
        // No variance allowed - compare whole string
        return qCompare(r1.toString(), r2.toString(), actual, expected, file, line);
    }

    if (qAbs(qreal(r1.total) - qreal(r2.total)) <= qreal(r1.total)*variance) {
        return compare_helper(true, "COMPARE()", file, line);
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
            int end = line.indexOf('"', index + strlen(markers[j][0]));
            if (end == -1) {
                continue;
            }
            line.replace(index, end-index + 1, markers[j][1]);
        }
    }

    return out;
}

// Load the expected test output for the nominated test (subdir) and logger
// as an array of lines.  If there is no expected output file, return an
// empty array.
static QList<QByteArray> expectedResult(const QString &subdir, const QString &logger)
{
    QFile file(":/expected_" + subdir + "." + logger);
    if (!file.open(QIODevice::ReadOnly))
        return QList<QByteArray>();
    return splitLines(file.readAll());
}

struct Logger
{
    Logger(QString const&, QStringList const&);

    QString name;
    QStringList arguments;
};

Logger::Logger(QString const& _name, QStringList const& _arguments)
    : name(_name)
    , arguments(_arguments)
{
}

static QList<Logger> allLoggers()
{
    return QList<Logger>()
        << Logger("txt",      QStringList())
        << Logger("xml",      QStringList() << "-xml")
        << Logger("xunitxml", QStringList() << "-xunitxml")
        << Logger("lightxml", QStringList() << "-lightxml")
    ;
}

void tst_Selftests::runSubTest_data()
{
    QTest::addColumn<QString>("subdir");
    QTest::addColumn<QString>("logger");
    QTest::addColumn<QStringList>("arguments");

    QStringList tests = QStringList()
        << "subtest"
        << "warnings"
        << "maxwarnings"
        << "cmptest"
//        << "alive"    // timer dependent
        << "globaldata"
        << "skipglobal"
        << "skip"
        << "strcmp"
        << "expectfail"
        << "sleep"
        << "fetchbogus"
        << "crashes"
        << "multiexec"
        << "failinit"
        << "failinitdata"
        << "skipinit"
        << "skipinitdata"
        << "datetime"
        << "singleskip"

        //on windows assert does nothing in release mode and blocks execution with a popup window in debug mode
#if !defined(Q_OS_WIN)
        << "assert"
#endif

        << "differentexec"
#ifndef QT_NO_EXCEPTIONS
        // The machine that run the intel autotests will popup a dialog
        // with a warning that an uncaught exception was thrown.
        // This will time out and falsely fail, therefore we disable the test for that platform.
# if !defined(Q_CC_INTEL) || !defined(Q_OS_WIN)
        << "exceptionthrow"
# endif
#endif
        << "qexecstringlist"
        << "datatable"
        << "commandlinedata"

#if defined(__GNUC__) && defined(__i386) && defined(Q_OS_LINUX)
        << "benchlibcallgrind"
#endif
        << "benchlibeventcounter"
        << "benchliboptions"

        //### These tests are affected by timing and whether the CPU tick counter is
        //### monotonically increasing. They won't work on some machines so leave them off by default.
        //### Feel free to uncomment for your own testing.
#if 0
        << "benchlibwalltime"
        << "benchlibtickcounter"
#endif

        << "xunit"
        << "longstring"
        << "badxml"
    ;

    foreach (Logger const& logger, allLoggers()) {
        QString rowSuffix;
        if (logger.name != "txt") {
            rowSuffix = QString(" %1").arg(logger.name);
        }

        foreach (QString const& subtest, tests) {
            QStringList arguments = logger.arguments;
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

            // These tests don't work right with loggers other than plain text, usually because
            // they internally supply arguments to themselves.
            if (logger.name != "txt") {
                if (subtest == "differentexec") {
                    continue;
                }
                if (subtest == "qexecstringlist") {
                    continue;
                }
                if (subtest == "benchliboptions") {
                    continue;
                }
                // `crashes' will not output valid XML on platforms without a crash handler
                if (subtest == "crashes") {
                    continue;
                }
                // this test prints out some floats in the testlog and the formatting is
                // platform-specific and hard to predict.
                if (subtest == "subtest") {
                    continue;
                }
            }

            QTest::newRow(qPrintable(QString("%1%2").arg(subtest).arg(rowSuffix)))
                << subtest
                << logger.name
                << arguments
            ;
        }
    }
}

void tst_Selftests::doRunSubTest(QString const& subdir, QString const& logger, QStringList const& arguments )
{
    // For the plain text logger, we'll read straight from standard output.
    // For all other loggers (XML), we'll tell testlib to redirect to a file.
    // The reason is that tests are allowed to print to standard output, and
    // that means the test log is no longer guaranteed to be valid XML.
    QStringList extraArguments;
    QString logfile;
    if (logger != "txt") {
        logfile = "test_output";
        extraArguments << "-o" << logfile;
    }

    QProcess proc;
    proc.setEnvironment(QStringList(""));
    proc.start(subdir + "/" + subdir, QStringList() << arguments << extraArguments);
    QVERIFY2(proc.waitForFinished(), qPrintable(proc.errorString()));

    QByteArray out;
    if (logfile.isEmpty()) {
        out = proc.readAllStandardOutput();
    } else {
        QFile file(logfile);
        if (file.open(QIODevice::ReadOnly))
            out = file.readAll();
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
        && subdir != QLatin1String("fetchbogus")
        && subdir != QLatin1String("xunit")
        && subdir != QLatin1String("benchlibcallgrind"))
        QVERIFY2(err.isEmpty(), err.constData());

    QList<QByteArray> res = splitLines(out);
    QList<QByteArray> exp = expectedResult(subdir, logger);

    if (exp.count() == 0) {
        QList<QList<QByteArray> > expArr;
        int i = 1;
        do {
            exp = expectedResult(subdir + QString("_%1").arg(i++), logger);
            if (exp.count())
                expArr += exp;
        } while (exp.count());

        for (int j = 0; j < expArr.count(); ++j) {
            if (res.count() == expArr.at(j).count()) {
                exp = expArr.at(j);
                break;
            }
        }
    } else {
        QCOMPARE(res.count(), exp.count());
    }

    if (logger == "xunitxml" || logger == "xml" || logger == "lightxml") {
        QByteArray xml(out);
        // lightxml intentionally skips the root element, which technically makes it
        // not valid XML.
        // We'll add that ourselves for the purpose of validation.
        if (logger == "lightxml") {
            xml.prepend("<root>");
            xml.append("</root>");
        }

        QXmlStreamReader reader(xml);

        while(!reader.atEnd())
            reader.readNext();

        QVERIFY2(!reader.error(), qPrintable(QString("line %1, col %2: %3")
            .arg(reader.lineNumber())
            .arg(reader.columnNumber())
            .arg(reader.errorString())
        ));
    }

    bool benchmark = false;
    for (int i = 0; i < res.count(); ++i) {
        QByteArray line = res.at(i);
        if (line.startsWith("Config: Using QTest"))
            continue;
        // the __FILE__ __LINE__ output is compiler dependent, skip it
        if (line.startsWith("   Loc: [") && line.endsWith(")]"))
            continue;
        if (line.endsWith(" : failure location"))
            continue;

        const QString output(QString::fromLatin1(line));
        const QString expected(QString::fromLatin1(exp.at(i)).replace("@INSERT_QT_VERSION_HERE@", QT_VERSION_STR));

        // Q_ASSERT uses __FILE__.  Some compilers include the absolute path in
        // __FILE__, while others do not.
        if (line.contains("ASSERT") && output != expected) {
            const char msg[] = "Q_ASSERT prints out the absolute path on this platform.";
            QEXPECT_FAIL("assert",                msg, Continue);
            QEXPECT_FAIL("assert xml",            msg, Continue);
            QEXPECT_FAIL("assert lightxml",       msg, Continue);
            QEXPECT_FAIL("assert xunitxml",       msg, Continue);
        }

        if (expected.startsWith(QLatin1String("FAIL!  : tst_Exception::throwException() Caught unhandled exce")) && expected != output)
            // On some platforms we compile without RTTI, and as a result we never throw an exception.
            QCOMPARE(output.simplified(), QString::fromLatin1("tst_Exception::throwException()").simplified());
        else if (output != expected && qstrcmp(QTest::currentDataTag(), "subtest") == 0)
            // The floating point formatting differs between platforms, so let's just skip it.
            continue;
        else if (benchmark || line.startsWith("<BenchmarkResult")) {
            // Don't do a literal comparison for benchmark results, since
            // results have some natural variance.
            QString error;

            BenchmarkResult actualResult = BenchmarkResult::parse(output, &error);
            QVERIFY2(error.isEmpty(), qPrintable(QString("Actual line didn't parse as benchmark result: %1\nLine: %2").arg(error).arg(output)));

            BenchmarkResult expectedResult = BenchmarkResult::parse(expected, &error);
            QVERIFY2(error.isEmpty(), qPrintable(QString("Expected line didn't parse as benchmark result: %1\nLine: %2").arg(error).arg(expected)));

            QCOMPARE(actualResult, expectedResult);
        } else {
            QCOMPARE(output, expected);
        }

        benchmark = line.startsWith("RESULT : ");
    }
}

void tst_Selftests::runSubTest()
{
    QFETCH(QString, subdir);
    QFETCH(QString, logger);
    QFETCH(QStringList, arguments);

    doRunSubTest(subdir, logger, arguments);
}

// attribute must contain ="
QString extractXmlAttribute(const QString &line, const char *attribute)
{
    int index = line.indexOf(attribute);
    if (index == -1)
        return QString();
    int end = line.indexOf('"', index + strlen(attribute));
    if (end == -1)
        return QString();

    QString result = line.mid(index + strlen(attribute), end - index - strlen(attribute));
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
    // Remove the test output file
    QFile::remove("test_output");
}

QTEST_MAIN(tst_Selftests)

#include "tst_selftests.moc"
