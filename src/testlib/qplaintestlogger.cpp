// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestresult_p.h>
#include <QtTest/qtestassert.h>
#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qplaintestlogger_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>

#include <QtCore/private/qlogging_p.h>

#include <array>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QtCore/QByteArray>
#include <QtCore/qmath.h>
#include <QtCore/QLibraryInfo>

#ifdef Q_OS_ANDROID
#  include <android/log.h>
#endif

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
static const char multiplePrefixes[] = "\0kMGTPE"; // kilo, mega, giga, tera, peta, exa
static const char submultiplePrefixes[] = "afpnum"; // atto, femto, pico, nano, micro, milli

template <int N> struct FixedBufString
{
    static constexpr size_t MaxSize = N;
    size_t used = 0;
    std::array<char, N + 2> buf;    // for the newline and terminating null
    FixedBufString()
    {
        clear();
    }
    void clear()
    {
        used = 0;
        buf[0] = '\0';
    }

    operator const char *() const
    {
        return buf.data();
    }

    void append(const char *text)
    {
        size_t len = qMin(strlen(text), MaxSize - used);
        memcpy(buf.data() + used, text, len);
        used += len;
        buf[used] = '\0';
    }

    template <typename... Args> void appendf(const char *format, Args &&... args)
    {
        // vsnprintf includes the terminating null
        used += qsnprintf(buf.data() + used, MaxSize - used + 1, format,
                          std::forward<Args>(args)...);
    }

    template <int Power = 1000> void appendScaled(qreal value, const char *unit)
    {
        char prefix[2] = {};
        qreal v = qAbs(value);
        qint64 ratio;
        if (v < 1 && Power == 1000) {
            const char *prefixes = submultiplePrefixes;
            ratio = qreal(std::atto::num) / std::atto::den;
            while (value * ratio > 1000 && *prefixes) {
                ++prefixes;
                ratio *= 1000;
            }
            prefix[0] = *prefixes;
        } else {
            const char *prefixes = multiplePrefixes;
            ratio = 1;
            while (value > 1000 * ratio) {  // yes, even for binary
                ++prefixes;
                ratio *= Power;
            }
            prefix[0] = *prefixes;
        }

        // adjust the value by the ratio
        value /= ratio;
        appendf(", %.3g %s%s", value, prefix, unit);
    }
};
} // unnamed namespace

namespace QTest {

    static const char *ptIncidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Skip:
            return "SKIP   ";
        case QAbstractTestLogger::Pass:
            return "PASS   ";
        case QAbstractTestLogger::XFail:
            return "XFAIL  ";
        case QAbstractTestLogger::Fail:
            return "FAIL!  ";
        case QAbstractTestLogger::XPass:
            return "XPASS  ";
        case QAbstractTestLogger::BlacklistedPass:
            return "BPASS  ";
        case QAbstractTestLogger::BlacklistedFail:
            return "BFAIL  ";
        case QAbstractTestLogger::BlacklistedXPass:
            return "BXPASS ";
        case QAbstractTestLogger::BlacklistedXFail:
            return "BXFAIL ";
        }
        Q_UNREACHABLE_RETURN(nullptr);
    }

    static const char *benchmarkResult2String()
    {
        return "RESULT ";
    }

    static const char *ptMessageType2String(QAbstractTestLogger::MessageTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::QDebug:
            return "QDEBUG ";
        case QAbstractTestLogger::QInfo:
            return "QINFO  ";
        case QAbstractTestLogger::QWarning:
            return "QWARN  ";
        case QAbstractTestLogger::QCritical:
            return "QCRITICAL";
        case QAbstractTestLogger::QFatal:
            return "QFATAL ";
        case QAbstractTestLogger::Info:
            return "INFO   ";
        case QAbstractTestLogger::Warn:
            return "WARNING";
        }
        Q_UNREACHABLE_RETURN(nullptr);
    }

    template <typename T>
    static int countSignificantDigits(T num)
    {
        if (num <= 0)
            return 0;

        int digits = 0;
        qreal divisor = 1;

        while (num / divisor >= 1) {
            divisor *= 10;
            ++digits;
        }

        return digits;
    }

    // Pretty-prints a benchmark result using the given number of digits.
    template <typename T> QByteArray formatResult(T number, int significantDigits)
    {
        if (number < T(0))
            return "NAN";
        if (number == T(0))
            return "0";

        QByteArray beforeDecimalPoint = QByteArray::number(qint64(number), 'f', 0);
        QByteArray afterDecimalPoint = QByteArray::number(number, 'f', 20);
        afterDecimalPoint.remove(0, beforeDecimalPoint.size() + 1);

        int beforeUse = qMin(beforeDecimalPoint.size(), significantDigits);
        int beforeRemove = beforeDecimalPoint.size() - beforeUse;

        // Replace insignificant digits before the decimal point with zeros.
        beforeDecimalPoint.chop(beforeRemove);
        for (int i = 0; i < beforeRemove; ++i) {
            beforeDecimalPoint.append(u'0');
        }

        int afterUse = significantDigits - beforeUse;

        // leading zeroes after the decimal point does not count towards the digit use.
        if (beforeDecimalPoint == "0" && !afterDecimalPoint.isEmpty()) {
            ++afterUse;

            int i = 0;
            while (i < afterDecimalPoint.size() && afterDecimalPoint.at(i) == '0')
                ++i;

            afterUse += i;
        }

        int afterRemove = afterDecimalPoint.size() - afterUse;
        afterDecimalPoint.chop(afterRemove);

        char separator = ',';
        char decimalPoint = '.';

        // insert thousands separators
        int length = beforeDecimalPoint.size();
        for (int i = beforeDecimalPoint.size() -1; i >= 1; --i) {
            if ((length - i) % 3 == 0)
                beforeDecimalPoint.insert(i, separator);
        }

        QByteArray print;
        print = beforeDecimalPoint;
        if (afterUse > 0)
            print.append(decimalPoint);

        print += afterDecimalPoint;


        return print;
    }
}

/*! \internal
    \class QPlainTestLogger
    \inmodule QtTest

    QPlainTestLogger implements basic logging of test results.

    The format is Qt-specific and aims to be easy to read.
*/

void QPlainTestLogger::outputMessage(const char *str)
{
#if defined(Q_OS_WIN)
    // Log to system log only if output is not redirected and stderr not preferred
    if (stream == stdout && !QtPrivate::shouldLogToStderr()) {
        OutputDebugStringA(str);
        return;
    }
#elif defined(Q_OS_ANDROID)
    __android_log_write(ANDROID_LOG_INFO, "QTestLib", str);
#endif
    outputString(str);
}

void QPlainTestLogger::printMessage(MessageSource source, const char *type, const char *msg,
                                    const char *file, int line)
{
    QTEST_ASSERT(type);
    QTEST_ASSERT(msg);

    QTestCharBuffer messagePrefix;

    QTestCharBuffer messageLocation;
#ifdef Q_OS_WIN
    constexpr const char *INCIDENT_LOCATION_STR = "\n%s(%d) : failure location";
    constexpr const char *OTHER_LOCATION_STR = "\n%s(%d) : message location";
#else
    constexpr const char *INCIDENT_LOCATION_STR = "\n   Loc: [%s(%d)]";
    constexpr const char *OTHER_LOCATION_STR = INCIDENT_LOCATION_STR;
#endif

    if (file) {
        switch (source) {
        case MessageSource::Incident:
            QTest::qt_asprintf(&messageLocation, INCIDENT_LOCATION_STR, file, line);
            break;
        case MessageSource::Other:
            QTest::qt_asprintf(&messageLocation, OTHER_LOCATION_STR, file, line);
            break;
        }
    }

    const char *msgFiller = msg[0] ? " " : "";
    QTestCharBuffer testIdentifier;
    QTestPrivate::generateTestIdentifier(&testIdentifier);
    QTest::qt_asprintf(&messagePrefix, "%s: %s%s%s%s\n",
                       type, testIdentifier.data(), msgFiller, msg, messageLocation.data());

    // In colored mode, printf above stripped our nonprintable control characters.
    // Put them back.
    memcpy(messagePrefix.data(), type, strlen(type));

    outputMessage(messagePrefix.data());
}

void QPlainTestLogger::printBenchmarkResultsHeader(const QBenchmarkResult &result)
{
    FixedBufString<1022> buf;
    buf.appendf("%s: %s::%s", QTest::benchmarkResult2String(),
                QTestResult::currentTestObjectName(), result.context.slotName.toLatin1().data());

    if (QByteArray tag = result.context.tag.toLocal8Bit(); !tag.isEmpty())
        buf.appendf(":\"%s\":\n", tag.data());
    else
        buf.append(":\n");
    outputMessage(buf);
}

void QPlainTestLogger::printBenchmarkResults(const QList<QBenchmarkResult> &results)
{
    using namespace std::chrono;
    FixedBufString<1022> buf;
    auto findResultFor = [&results](QTest::QBenchmarkMetric metric) -> std::optional<qreal> {
        for (const QBenchmarkResult &result : results) {
            if (result.measurement.metric == metric)
                return result.measurement.value;
        }
        return std::nullopt;
    };

    // we need the execution time quite often, so find it first
    qreal executionTime = 0;
    if (auto ns = findResultFor(QTest::WalltimeNanoseconds))
        executionTime = *ns / (1000 * 1000 * 1000);
    else if (auto ms = findResultFor(QTest::WalltimeMilliseconds))
        executionTime = *ms / 1000;

    for (const QBenchmarkResult &result : results) {
        buf.clear();

        const char * unitText = QTest::benchmarkMetricUnit(result.measurement.metric);
        int significantDigits = QTest::countSignificantDigits(result.measurement.value);
        qreal valuePerIteration = qreal(result.measurement.value) / qreal(result.iterations);
        buf.appendf("     %s %s%s", QTest::formatResult(valuePerIteration, significantDigits).constData(),
                    unitText, result.setByMacro ? " per iteration" : "");

        switch (result.measurement.metric) {
        case QTest::BitsPerSecond:
            // for bits/s, we'll use powers of 10 (1 Mbit/s = 1000 kbit/s = 1000000 bit/s)
            buf.appendScaled<1000>(result.measurement.value, "bit/s");
            break;
        case QTest::BytesPerSecond:
            // for B/s, we'll use powers of 2 (1 MB/s = 1024 kB/s = 1048576 B/s)
            buf.appendScaled<1024>(result.measurement.value, "B/s");
            break;

        case QTest::CPUCycles:
        case QTest::RefCPUCycles:
            if (!qIsNull(executionTime))
                buf.appendScaled(result.measurement.value / executionTime, "Hz");
            break;

        case QTest::Instructions:
            if (auto cycles = findResultFor(QTest::CPUCycles)) {
                buf.appendf(", %.3f instr/cycle", result.measurement.value / *cycles);
                break;
            }
            Q_FALLTHROUGH();

        case QTest::InstructionReads:
        case QTest::Events:
        case QTest::BytesAllocated:
        case QTest::CPUMigrations:
        case QTest::BusCycles:
        case QTest::StalledCycles:
        case QTest::BranchInstructions:
        case QTest::BranchMisses:
        case QTest::CacheReferences:
        case QTest::CacheReads:
        case QTest::CacheWrites:
        case QTest::CachePrefetches:
        case QTest::CacheMisses:
        case QTest::CacheReadMisses:
        case QTest::CacheWriteMisses:
        case QTest::CachePrefetchMisses:
        case QTest::ContextSwitches:
        case QTest::PageFaults:
        case QTest::MinorPageFaults:
        case QTest::MajorPageFaults:
        case QTest::AlignmentFaults:
        case QTest::EmulationFaults:
            if (!qIsNull(executionTime))
                buf.appendScaled(result.measurement.value / executionTime, "/sec");
            break;

        case QTest::FramesPerSecond:
        case QTest::CPUTicks:
        case QTest::WalltimeMilliseconds:
        case QTest::WalltimeNanoseconds:
            break;  // no additional information
        }

        Q_ASSERT(result.iterations > 0);
        buf.appendf(" (total: %s, iterations: %d)\n",
                    QTest::formatResult(result.measurement.value, significantDigits).constData(),
                    result.iterations);

        outputMessage(buf);
    }
}

QPlainTestLogger::QPlainTestLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QPlainTestLogger::~QPlainTestLogger() = default;

void QPlainTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    char buf[1024];
    if (QTestLog::verboseLevel() < 0) {
        qsnprintf(buf, sizeof(buf), "Testing %s\n", QTestResult::currentTestObjectName());
    } else {
        qsnprintf(buf, sizeof(buf),
                  "********* Start testing of %s *********\n"
                  "Config: Using QtTest library " QTEST_VERSION_STR
                  ", %s, %s %s\n", QTestResult::currentTestObjectName(), QLibraryInfo::build(),
                  qPrintable(QSysInfo::productType()), qPrintable(QSysInfo::productVersion()));
    }
    outputMessage(buf);
}

void QPlainTestLogger::stopLogging()
{
    char buf[1024];
    const int timeMs = qRound(QTestLog::msecsTotalTime());
    if (QTestLog::verboseLevel() < 0) {
        qsnprintf(buf, sizeof(buf), "Totals: %d passed, %d failed, %d skipped, %d blacklisted, %dms\n",
                  QTestLog::passCount(), QTestLog::failCount(),
                  QTestLog::skipCount(), QTestLog::blacklistCount(), timeMs);
    } else {
        qsnprintf(buf, sizeof(buf),
                  "Totals: %d passed, %d failed, %d skipped, %d blacklisted, %dms\n"
                  "********* Finished testing of %s *********\n",
                  QTestLog::passCount(), QTestLog::failCount(),
                  QTestLog::skipCount(), QTestLog::blacklistCount(), timeMs,
                  QTestResult::currentTestObjectName());
    }
    outputMessage(buf);

    QAbstractTestLogger::stopLogging();
}


void QPlainTestLogger::enterTestFunction(const char * /*function*/)
{
    if (QTestLog::verboseLevel() >= 1)
        printMessage(MessageSource::Other, QTest::ptMessageType2String(Info), "entering");
}

void QPlainTestLogger::leaveTestFunction()
{
}

void QPlainTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    // suppress B?PASS and B?XFAIL in silent mode
    if ((type == Pass || type == BlacklistedPass || type == XFail || type == BlacklistedXFail)
        && QTestLog::verboseLevel() < 0)
        return;

    printMessage(MessageSource::Incident, QTest::ptIncidentType2String(type), description, file, line);
}

void QPlainTestLogger::addBenchmarkResults(const QList<QBenchmarkResult> &results)
{
    // suppress benchmark results in silent mode
    if (QTestLog::verboseLevel() < 0 || results.isEmpty())
        return;

    printBenchmarkResultsHeader(results.first());
    printBenchmarkResults(results);
}

void QPlainTestLogger::addMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QAbstractTestLogger::addMessage(type, context, message);
}

void QPlainTestLogger::addMessage(MessageTypes type, const QString &message,
                                  const char *file, int line)
{
    // suppress non-fatal messages in silent mode
    if (type != QFatal && QTestLog::verboseLevel() < 0)
        return;

    printMessage(MessageSource::Other, QTest::ptMessageType2String(type), qPrintable(message), file, line);
}

QT_END_NAMESPACE
