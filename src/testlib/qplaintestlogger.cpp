/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include "QtTest/private/qtestresult_p.h"
#include "QtTest/qtestassert.h"
#include "QtTest/private/qtestlog_p.h"
#include "QtTest/private/qplaintestlogger_p.h"
#include "QtTest/private/qbenchmark_p.h"
#include "QtTest/private/qbenchmarkmetric_p.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef Q_OS_WIN
#include "windows.h"
#endif

#if defined(Q_OS_SYMBIAN)
#include <e32debug.h>
#endif

#ifdef Q_OS_WINCE
#include <QtCore/QString>
#endif

#include <QtCore/QByteArray>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

namespace QTest {

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)

    static CRITICAL_SECTION outputCriticalSection;
    static HANDLE hConsole = INVALID_HANDLE_VALUE;
    static WORD consoleAttributes = 0;

    static const char *qWinColoredMsg(int prefix, int color, const char *msg)
    {
        if (!hConsole)
            return msg;

        WORD attr = consoleAttributes & ~(FOREGROUND_GREEN | FOREGROUND_BLUE
                  | FOREGROUND_RED | FOREGROUND_INTENSITY);
        if (prefix)
            attr |= FOREGROUND_INTENSITY;
        if (color == 32)
            attr |= FOREGROUND_GREEN;
        if (color == 36)
            attr |= FOREGROUND_BLUE | FOREGROUND_GREEN;
        if (color == 31)
            attr |= FOREGROUND_RED | FOREGROUND_INTENSITY;
        if (color == 37)
            attr |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        if (color == 33)
            attr |= FOREGROUND_RED | FOREGROUND_GREEN;
        SetConsoleTextAttribute(hConsole, attr);
        printf(msg);
        SetConsoleTextAttribute(hConsole, consoleAttributes);
        return "";
    }

# define COLORED_MSG(prefix, color, msg) colored ? qWinColoredMsg(prefix, color, msg) : msg
#else
# define COLORED_MSG(prefix, color, msg) colored && QAbstractTestLogger::isTtyOutput() ? "\033["#prefix";"#color"m" msg "\033[0m" : msg
#endif

    static const char *incidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        static bool colored = (!qgetenv("QTEST_COLORED").isEmpty());
        switch (type) {
        case QAbstractTestLogger::Pass:
            return COLORED_MSG(0, 32, "PASS   "); //green
        case QAbstractTestLogger::XFail:
            return COLORED_MSG(1, 32, "XFAIL  "); //light green
        case QAbstractTestLogger::Fail:
            return COLORED_MSG(0, 31, "FAIL!  "); //red
        case QAbstractTestLogger::XPass:
            return COLORED_MSG(0, 31, "XPASS  "); //red, too
        }
        return "??????";
    }

    static const char *benchmarkResult2String()
    {
        static bool colored = (!qgetenv("QTEST_COLORED").isEmpty());
        return COLORED_MSG(0, 36, "RESULT "); // cyan
    }

    static const char *messageType2String(QAbstractTestLogger::MessageTypes type)
    {
#ifdef Q_OS_WIN
        static bool colored = (!qgetenv("QTEST_COLORED").isEmpty());
#else
        static bool colored = ::getenv("QTEST_COLORED");
#endif
        switch (type) {
        case QAbstractTestLogger::Skip:
            return COLORED_MSG(0, 37, "SKIP   "); //white
        case QAbstractTestLogger::Warn:
            return COLORED_MSG(0, 33, "WARNING"); // yellow
        case QAbstractTestLogger::QWarning:
            return COLORED_MSG(1, 33, "QWARN  ");
        case QAbstractTestLogger::QDebug:
            return COLORED_MSG(1, 33, "QDEBUG ");
        case QAbstractTestLogger::QSystem:
            return COLORED_MSG(1, 33, "QSYSTEM");
        case QAbstractTestLogger::QFatal:
            return COLORED_MSG(0, 31, "QFATAL "); // red
        case QAbstractTestLogger::Info:
            return "INFO   "; // no coloring
        }
        return "??????";
    }

    static void outputMessage(const char *str)
    {
#if defined(Q_OS_WINCE)
        QString strUtf16 = QString::fromLatin1(str);
        const int maxOutputLength = 255;
        do {
            QString tmp = strUtf16.left(maxOutputLength);
            OutputDebugString((wchar_t*)tmp.utf16());
            strUtf16.remove(0, maxOutputLength);
        } while (!strUtf16.isEmpty());
        if (QTestLog::outputFileName())
#elif defined(Q_OS_WIN)
        EnterCriticalSection(&outputCriticalSection);
        // OutputDebugString is not threadsafe
        OutputDebugStringA(str);
        LeaveCriticalSection(&outputCriticalSection);
#elif defined(Q_OS_SYMBIAN)
        // RDebug::Print has a cap of 256 characters so break it up
        TPtrC8 ptr(reinterpret_cast<const TUint8*>(str));
        _LIT(format, "[QTestLib] %S");
        const int maxBlockSize = 256 - ((const TDesC &)format).Length();
        HBufC* hbuffer = HBufC::New(maxBlockSize);
        if(hbuffer) {
            for (int i = 0; i < ptr.Length(); i += maxBlockSize) {
                int size = Min(maxBlockSize, ptr.Length() - i);
                hbuffer->Des().Copy(ptr.Mid(i, size));
                RDebug::Print(format, hbuffer);
            }
            delete hbuffer;
        }
        else {
            // fast, no allocations, but truncates silently
            RDebug::RawPrint(format);
            TPtrC8 ptr(reinterpret_cast<const TUint8*>(str));
            RDebug::RawPrint(ptr);
            RDebug::RawPrint(_L8("\n"));
        }
#endif
        QAbstractTestLogger::outputString(str);
    }

    static void printMessage(const char *type, const char *msg, const char *file = 0, int line = 0)
    {
        QTEST_ASSERT(type);
        QTEST_ASSERT(msg);

        QTestCharBuffer buf;

        const char *fn = QTestResult::currentTestFunction() ? QTestResult::currentTestFunction()
            : "UnknownTestFunc";
        const char *tag = QTestResult::currentDataTag() ? QTestResult::currentDataTag() : "";
        const char *gtag = QTestResult::currentGlobalDataTag()
                         ? QTestResult::currentGlobalDataTag()
                         : "";
        const char *filler = (tag[0] && gtag[0]) ? ":" : "";
        if (file) {
            QTest::qt_asprintf(&buf, "%s: %s::%s(%s%s%s)%s%s\n"
#ifdef Q_OS_WIN
                          "%s(%d) : failure location\n"
#else
                          "   Loc: [%s(%d)]\n"
#endif
                          , type, QTestResult::currentTestObjectName(), fn, gtag, filler, tag,
                          msg[0] ? " " : "", msg, file, line);
        } else {
            QTest::qt_asprintf(&buf, "%s: %s::%s(%s%s%s)%s%s\n",
                    type, QTestResult::currentTestObjectName(), fn, gtag, filler, tag,
                    msg[0] ? " " : "", msg);
        }
        // In colored mode, printf above stripped our nonprintable control characters.
        // Put them back.
        memcpy(buf.data(), type, strlen(type));
        outputMessage(buf.data());
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
    template <typename T> QString formatResult(T number, int significantDigits)
    {
        if (number < T(0))
            return QLatin1String("NAN");
        if (number == T(0))
            return QLatin1String("0");

        QString beforeDecimalPoint = QString::number(qint64(number), 'f', 0);
        QString afterDecimalPoint = QString::number(number, 'f', 20);
        afterDecimalPoint.remove(0, beforeDecimalPoint.count() + 1);

        int beforeUse = qMin(beforeDecimalPoint.count(), significantDigits);
        int beforeRemove = beforeDecimalPoint.count() - beforeUse;

        // Replace insignificant digits before the decimal point with zeros.
        beforeDecimalPoint.chop(beforeRemove);
        for (int i = 0; i < beforeRemove; ++i) {
            beforeDecimalPoint.append(QLatin1Char('0'));
        }

        int afterUse = significantDigits - beforeUse;

        // leading zeroes after the decimal point does not count towards the digit use.
        if (beforeDecimalPoint == QLatin1String("0") && afterDecimalPoint.isEmpty() == false) {
            ++afterUse;

            int i = 0;
            while (i < afterDecimalPoint.count() && afterDecimalPoint.at(i) == QLatin1Char('0')) {
                ++i;
            }

            afterUse += i;
        }

        int afterRemove = afterDecimalPoint.count() - afterUse;
        afterDecimalPoint.chop(afterRemove);

        QChar separator = QLatin1Char(',');
        QChar decimalPoint = QLatin1Char('.');

        // insert thousands separators
        int length = beforeDecimalPoint.length();
        for (int i = beforeDecimalPoint.length() -1; i >= 1; --i) {
            if ((length - i) % 3 == 0)
                beforeDecimalPoint.insert(i, separator);
        }

        QString print;
        print = beforeDecimalPoint;
        if (afterUse > 0)
            print.append(decimalPoint);

        print += afterDecimalPoint;


        return print;
    }

    template <typename T>
    int formatResult(char * buffer, int bufferSize, T number, int significantDigits)
    {
        QString result = formatResult(number, significantDigits);
        qstrncpy(buffer, result.toAscii().constData(), bufferSize);
        int size = result.count();
        return size;
    }

//    static void printBenchmarkResult(const char *bmtag, int value, int iterations)
    static void printBenchmarkResult(const QBenchmarkResult &result)
    {
        const char *bmtag = QTest::benchmarkResult2String();

        char buf1[1024];
        QTest::qt_snprintf(
            buf1, sizeof(buf1), "%s: %s::%s",
            bmtag,
            QTestResult::currentTestObjectName(),
            result.context.slotName.toAscii().data());

        char bufTag[1024];
        bufTag[0] = 0;
        QByteArray tag = result.context.tag.toAscii();
        if (tag.isEmpty() == false) {
            QTest::qt_snprintf(bufTag, sizeof(bufTag), ":\"%s\"", tag.data());
        }


        char fillFormat[8];
        int fillLength = 5;
        QTest::qt_snprintf(
            fillFormat, sizeof(fillFormat), ":\n%%%ds", fillLength);
        char fill[1024];
        QTest::qt_snprintf(fill, sizeof(fill), fillFormat, "");

        const char * unitText = QTest::benchmarkMetricUnit(result.metric);

        qreal valuePerIteration = qreal(result.value) / qreal(result.iterations);
        char resultBuffer[100] = "";
        formatResult(resultBuffer, 100, valuePerIteration, countSignificantDigits(result.value));

        char buf2[1024];
        QTest::qt_snprintf(
            buf2, sizeof(buf2), "%s %s",
            resultBuffer,
            unitText);

        char buf2_[1024];
        QByteArray iterationText = " per iteration";
        Q_ASSERT(result.iterations > 0);
        QTest::qt_snprintf(
            buf2_,
            sizeof(buf2_), "%s",
            iterationText.data());

        char buf3[1024];
        Q_ASSERT(result.iterations > 0);
        formatResult(resultBuffer, 100, result.value, countSignificantDigits(result.value));
        QTest::qt_snprintf(
            buf3, sizeof(buf3), " (total: %s, iterations: %d)",
            resultBuffer,
            result.iterations);

        char buf[1024];

        if (result.setByMacro) {
            QTest::qt_snprintf(
                buf, sizeof(buf), "%s%s%s%s%s%s\n", buf1, bufTag, fill, buf2, buf2_, buf3);
        } else {
            QTest::qt_snprintf(buf, sizeof(buf), "%s%s%s%s\n", buf1, bufTag, fill, buf2);
        }

        memcpy(buf, bmtag, strlen(bmtag));
        outputMessage(buf);
    }
}

QPlainTestLogger::QPlainTestLogger()
: randomSeed(9), hasRandomSeed(false)
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    InitializeCriticalSection(&QTest::outputCriticalSection);
    QTest::hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (QTest::hConsole != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(QTest::hConsole, &info)) {
            QTest::consoleAttributes = info.wAttributes;
        } else {
            QTest::hConsole = INVALID_HANDLE_VALUE;
        }
    }
#endif
}

QPlainTestLogger::~QPlainTestLogger()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
	DeleteCriticalSection(&QTest::outputCriticalSection);
#endif
}

void QPlainTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    char buf[1024];
    if (QTestLog::verboseLevel() < 0) {
        QTest::qt_snprintf(buf, sizeof(buf), "Testing %s\n",
                           QTestResult::currentTestObjectName());
    } else {
        if (hasRandomSeed) {
            QTest::qt_snprintf(buf, sizeof(buf),
                             "********* Start testing of %s *********\n"
                             "Config: Using QTest library " QTEST_VERSION_STR
                             ", Qt %s, Random seed %d\n", QTestResult::currentTestObjectName(), qVersion(), randomSeed);
        } else {
            QTest::qt_snprintf(buf, sizeof(buf),
                             "********* Start testing of %s *********\n"
                             "Config: Using QTest library " QTEST_VERSION_STR
                             ", Qt %s\n", QTestResult::currentTestObjectName(), qVersion());
        }
    }
    QTest::outputMessage(buf);
}

void QPlainTestLogger::stopLogging()
{
    char buf[1024];
    if (QTestLog::verboseLevel() < 0) {
        QTest::qt_snprintf(buf, sizeof(buf), "Totals: %d passed, %d failed, %d skipped\n",
                           QTestResult::passCount(), QTestResult::failCount(),
                           QTestResult::skipCount());
    } else {
        QTest::qt_snprintf(buf, sizeof(buf),
                         "Totals: %d passed, %d failed, %d skipped\n"
                         "********* Finished testing of %s *********\n",
                         QTestResult::passCount(), QTestResult::failCount(),
                         QTestResult::skipCount(), QTestResult::currentTestObjectName());
    }
    QTest::outputMessage(buf);

    QAbstractTestLogger::stopLogging();
}


void QPlainTestLogger::enterTestFunction(const char * /*function*/)
{
    if (QTestLog::verboseLevel() >= 1)
        QTest::printMessage(QTest::messageType2String(Info), "entering");
}

void QPlainTestLogger::leaveTestFunction()
{
}

void QPlainTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    // suppress PASS in silent mode
    if (type == QAbstractTestLogger::Pass && QTestLog::verboseLevel() < 0)
        return;

    QTest::printMessage(QTest::incidentType2String(type), description, file, line);
}

void QPlainTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
//    QTest::printBenchmarkResult(QTest::benchmarkResult2String(), value, iterations);
    QTest::printBenchmarkResult(result);
}

void QPlainTestLogger::addMessage(MessageTypes type, const char *message,
                                  const char *file, int line)
{
    // suppress PASS in silent mode
    if ((type == QAbstractTestLogger::Skip || type == QAbstractTestLogger::Info)
       && QTestLog::verboseLevel() < 0)
        return;

    QTest::printMessage(QTest::messageType2String(type), message, file, line);
}

void QPlainTestLogger::registerRandomSeed(unsigned int seed)
{
    randomSeed = seed;
    hasRandomSeed = true;
}

QT_END_NAMESPACE
