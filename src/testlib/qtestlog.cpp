// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/qtestassert.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qabstracttestlogger_p.h>
#include <QtTest/private/qplaintestlogger_p.h>
#include <QtTest/private/qcsvbenchmarklogger_p.h>
#include <QtTest/private/qjunittestlogger_p.h>
#include <QtTest/private/qxmltestlogger_p.h>
#include <QtTest/private/qteamcitylogger_p.h>
#include <QtTest/private/qtaptestlogger_p.h>
#if defined(HAVE_XCTEST)
#include <QtTest/private/qxctestlogger_p.h>
#endif

#if defined(Q_OS_DARWIN)
#include <QtTest/private/qappletestlogger_p.h>
#endif

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvariant.h>
#if QT_CONFIG(regularexpression)
#include <QtCore/QRegularExpression>
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <QtCore/q20algorithm.h>
#include <atomic>
#include <cstdio>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static void saveCoverageTool(const char * appname, bool testfailed, bool installedTestCoverage)
{
#ifdef __COVERAGESCANNER__
#  if QT_CONFIG(testlib_selfcover)
    __coveragescanner_teststate(QTestLog::failCount() > 0 ? "FAILED" :
                                QTestLog::passCount() > 0 ? "PASSED" : "SKIPPED");
#  else
    if (!installedTestCoverage)
        return;
    // install again to make sure the filename is correct.
    // without this, a plugin or similar may have changed the filename.
    __coveragescanner_install(appname);
    __coveragescanner_teststate(testfailed ? "FAILED" : "PASSED");
    __coveragescanner_save();
    __coveragescanner_testname("");
    __coveragescanner_clear();
    unsetenv("QT_TESTCOCOON_ACTIVE");
#  endif // testlib_selfcover
#else
    Q_UNUSED(appname);
    Q_UNUSED(testfailed);
    Q_UNUSED(installedTestCoverage);
#endif
}

Q_CONSTINIT static QBasicMutex elapsedTimersMutex; // due to the WatchDog thread
Q_CONSTINIT static QElapsedTimer elapsedFunctionTime;
Q_CONSTINIT static QElapsedTimer elapsedTotalTime;

namespace {
class LoggerRegistry
{
    using LoggersContainer = std::vector<std::shared_ptr<QAbstractTestLogger>>;
    using SharedLoggersContainer = std::shared_ptr<const LoggersContainer>;

public:
    void addLogger(std::unique_ptr<QAbstractTestLogger> logger)
    {
        // read/update/clone
        const SharedLoggersContainer currentLoggers = load();
        auto newLoggers = currentLoggers
                ? std::make_shared<LoggersContainer>(*currentLoggers)
                : std::make_shared<LoggersContainer>();
        newLoggers->emplace_back(std::move(logger));
        store(std::move(newLoggers));
    }

    void clear() { store(SharedLoggersContainer{}); }

    auto allLoggers() const
    {
        struct LoggersRange
        {
            const SharedLoggersContainer loggers;

            auto begin() const
            {
                return loggers ? loggers->cbegin() : LoggersContainer::const_iterator{};
            }
            auto end() const
            {
                return loggers ? loggers->cend() : LoggersContainer::const_iterator{};
            }
            bool isEmpty() const { return loggers ? loggers->empty() : true; }
        };

        return LoggersRange{ load() };
    }

private:
#ifdef __cpp_lib_atomic_shared_ptr
    SharedLoggersContainer load() const { return loggers.load(std::memory_order_acquire); }
    void store(SharedLoggersContainer newLoggers)
    {
        loggers.store(std::move(newLoggers), std::memory_order_release);
    }
    std::atomic<SharedLoggersContainer> loggers = nullptr;
#else
    SharedLoggersContainer load() const
    {
        return std::atomic_load_explicit(&loggers, std::memory_order_acquire);
    }
    void store(SharedLoggersContainer newLoggers)
    {
        std::atomic_store_explicit(&loggers, std::move(newLoggers), std::memory_order_release);
    }
    SharedLoggersContainer loggers;
#endif
};

} // namespace

namespace QTest {

    int fails = 0;
    int passes = 0;
    int skips = 0;
    int blacklists = 0;
    enum { Unresolved, Passed, Skipped, Suppressed, Failed } currentTestState;

    struct IgnoreResultList
    {
        inline IgnoreResultList(QtMsgType tp, const QVariant &patternIn)
            : type(tp), pattern(patternIn) {}

        static inline void clearList(IgnoreResultList *&list)
        {
            while (list) {
                IgnoreResultList *current = list;
                list = list->next;
                delete current;
            }
        }

        static void append(IgnoreResultList *&list, QtMsgType type, const QVariant &patternIn)
        {
            QTest::IgnoreResultList *item = new QTest::IgnoreResultList(type, patternIn);

            if (!list) {
                list = item;
                return;
            }
            IgnoreResultList *last = list;
            for ( ; last->next; last = last->next) ;
            last->next = item;
        }

        static bool stringsMatch(const QString &expected, const QString &actual)
        {
            if (expected == actual)
                return true;

            // ignore an optional whitespace at the end of str
            // (the space was added automatically by ~QDebug() until Qt 5.3,
            //  so autotests still might expect it)
            if (expected.endsWith(u' '))
                return actual == QStringView{expected}.left(expected.size() - 1);

            return false;
        }

        inline bool matches(QtMsgType tp, const QString &message) const
        {
            if (tp != type)
                return false;
#if QT_CONFIG(regularexpression)
            if (const auto *regex = get_if<QRegularExpression>(&pattern))
                return regex->match(message).hasMatch();
#endif
            Q_ASSERT(pattern.metaType() == QMetaType::fromType<QString>());
            return stringsMatch(pattern.toString(), message);
        }

        QtMsgType type;
        QVariant pattern;
        IgnoreResultList *next = nullptr;
    };

    static IgnoreResultList *ignoreResultList = nullptr;
    Q_CONSTINIT static QBasicMutex mutex;

    static std::vector<QVariant> failOnWarningList;

    Q_GLOBAL_STATIC(LoggerRegistry, loggers)

    static int verbosity = 0;
    static int maxWarnings = 2002;
    static bool installedTestCoverage = true;

    static QtMessageHandler oldMessageHandler;

    static bool handleIgnoredMessage(QtMsgType type, const QString &message)
    {
        const QMutexLocker mutexLocker(&QTest::mutex);

        if (!ignoreResultList)
            return false;
        IgnoreResultList *last = nullptr;
        IgnoreResultList *list = ignoreResultList;
        while (list) {
            if (list->matches(type, message)) {
                // remove the item from the list
                if (last)
                    last->next = list->next;
                else
                    ignoreResultList = list->next;

                delete list;
                return true;
            }

            last = list;
            list = list->next;
        }
        return false;
    }

    static void handleFatal()
    {
            /* Right now, we're inside the custom message handler and we're
               being qt_message_output in qglobal.cpp. After we return from this
               function, it will proceed with calling exit() and abort() and
               hence crash. Therefore, we call these logging functions such that
               we wrap up nicely, and in particular produce well-formed XML.
            */
            QTestLog::leaveTestFunction();
            QTestLog::stopLogging();
    }

    static bool handleFailOnWarning(const QMessageLogContext &context, const QString &message)
    {
        // failOnWarning can be called multiple times per test function, so let
        // each call cause a failure if required.
        for (const auto &pattern : failOnWarningList) {
            if (const auto *text = get_if<QString>(&pattern)) {
                if (message != *text)
                    continue;
#if QT_CONFIG(regularexpression)
            } else if (const auto *regex = get_if<QRegularExpression>(&pattern)) {
                if (!message.contains(*regex))
                    continue;
#endif
            } else {
                // The no-arg clearFailOnWarnings()'s null pattern matches all messages.
                Q_ASSERT(pattern.isNull());
            }

            const size_t maxMsgLen = 1024;
            char msg[maxMsgLen] = {'\0'};
            std::snprintf(msg, maxMsgLen, "Received a warning that resulted in a failure:\n%s",
                          qPrintable(message));
            QTestResult::addFailure(msg, context.file, context.line);
            return true;
        }
        return false;
    }

    static constexpr bool isWarnOrWorse(QtMsgType type)
    {
        // ## TODO Inline this once we get to Qt 7 !
#if QT_VERSION_MAJOR == 7 || defined(QT_BOOTSTRAPPED) // To match QtMsgType decl
        return type >= QtWarningMsg;
#else
        // Until Qt 6, Info was > Fatal :-(
        switch (type) {
        case QtWarningMsg:
        case QtCriticalMsg:
        case QtFatalMsg:
            return true;
        case QtDebugMsg:
        case QtInfoMsg:
            return false;
        }
        Q_UNREACHABLE_RETURN(false);
#endif
    }

    static void messageHandler(QtMsgType type, const QMessageLogContext & context, const QString &message)
    {
        static QBasicAtomicInt counter = Q_BASIC_ATOMIC_INITIALIZER(QTest::maxWarnings);

        auto loggerCapture = loggers->allLoggers();

        if (loggerCapture.isEmpty()) {
            // the message handler may be called from a worker thread, after the main thread stopped
            // logging. Forwarding to original message handler to avoid swallowing the message
            Q_ASSERT(oldMessageHandler);
            oldMessageHandler(type, context, message);
            return;
        }

        if (handleIgnoredMessage(type, message)) {
            // the message is expected, so just swallow it.
            return;
        }

        if (isWarnOrWorse(type) && handleFailOnWarning(context, message)) {
            if (type == QtFatalMsg)
                handleFatal();
            return;
        }

        if (type != QtFatalMsg) {
            if (counter.loadRelaxed() <= 0)
                return;

            if (!counter.deref()) {
                for (auto &logger : loggerCapture)
                    logger->addMessage(QAbstractTestLogger::Warn,
                                       QStringLiteral("Maximum amount of warnings exceeded. Use "
                                                      "-maxwarnings to override."));

                return;
            }
        }

        for (auto &logger : loggerCapture)
            logger->addMessage(type, context, message);

        if (type == QtFatalMsg) {
            QTestResult::addFailure("Received a fatal error.", context.file, context.line);
            handleFatal();
        }
    }
}

void QTestLog::enterTestFunction(const char* function)
{
    {
        QMutexLocker locker(&elapsedTimersMutex);
        elapsedFunctionTime.start();
    }
    if (printAvailableTags)
        return;

    QTEST_ASSERT(function);

    for (auto &logger : QTest::loggers->allLoggers())
        logger->enterTestFunction(function);
}

void QTestLog::enterTestData(QTestData *data)
{
    QTEST_ASSERT(data);

    for (auto &logger : QTest::loggers->allLoggers())
        logger->enterTestData(data);
}

int QTestLog::unhandledIgnoreMessages()
{
    const QMutexLocker mutexLocker(&QTest::mutex);
    int i = 0;
    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    while (list) {
        ++i;
        list = list->next;
    }
    return i;
}

void QTestLog::leaveTestFunction()
{
    if (printAvailableTags)
        return;

    for (auto &logger : QTest::loggers->allLoggers())
        logger->leaveTestFunction();
}

void QTestLog::printUnhandledIgnoreMessages()
{
    const QMutexLocker mutexLocker(&QTest::mutex);
    QString message;
    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    while (list) {
        if (const auto *text = get_if<QString>(&list->pattern)) {
            message = "Did not receive message: \"%1\""_L1.arg(*text);
#if QT_CONFIG(regularexpression)
        } else if (const auto *regex = get_if<QRegularExpression>(&list->pattern)) {
            message = "Did not receive any message matching: \"%1\""_L1.arg(regex->pattern());
#endif
        } else {
            Q_UNREACHABLE();
            message = "Missing message of unrecognized pattern type: \"%1\""_L1.arg(
                list->pattern.metaType().name());
        }
        for (auto &logger : QTest::loggers->allLoggers())
            logger->addMessage(QAbstractTestLogger::Info, message);

        list = list->next;
    }
}

void QTestLog::clearIgnoreMessages()
{
    const QMutexLocker mutexLocker(&QTest::mutex);
    QTest::IgnoreResultList::clearList(QTest::ignoreResultList);
}

void QTestLog::clearFailOnWarnings()
{
    QTest::failOnWarningList.clear();
}

void QTestLog::clearCurrentTestState()
{
    clearIgnoreMessages();
    clearFailOnWarnings();
    QTest::currentTestState = QTest::Unresolved;
}

void QTestLog::addPass(const char *msg)
{
    if (printAvailableTags)
        return;

    QTEST_ASSERT(msg);
    Q_ASSERT(QTest::currentTestState == QTest::Unresolved);

    ++QTest::passes;
    QTest::currentTestState = QTest::Passed;

    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::Pass, msg);
}

void QTestLog::addFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::currentTestState == QTest::Unresolved) {
        ++QTest::fails;
    } else {
        // After an XPASS/Continue, or fail or skip in a function the test
        // calls, we can subsequently fail.
        Q_ASSERT(QTest::currentTestState == QTest::Failed
                 || QTest::currentTestState == QTest::Skipped);
    }
    // It is up to particular loggers to decide whether to report such
    // subsequent failures; they may carry useful information.

    QTest::currentTestState = QTest::Failed;
    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::Fail, msg, file, line);
}

void QTestLog::addXFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    // Will be counted in addPass() if we get there.

    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::XFail, msg, file, line);
}

void QTestLog::addXPass(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::currentTestState == QTest::Unresolved) {
        ++QTest::fails;
    } else {
        // After an XPASS/Continue, we can subsequently XPASS again.
        // Likewise after a fail or skip in a function called by the test.
        Q_ASSERT(QTest::currentTestState == QTest::Failed
                 || QTest::currentTestState == QTest::Skipped);
    }

    QTest::currentTestState = QTest::Failed;
    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::XPass, msg, file, line);
}

void QTestLog::addBPass(const char *msg)
{
    QTEST_ASSERT(msg);
    Q_ASSERT(QTest::currentTestState == QTest::Unresolved);

    ++QTest::blacklists; // Not passes ?
    QTest::currentTestState = QTest::Suppressed;

    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::BlacklistedPass, msg);
}

void QTestLog::addBFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::currentTestState == QTest::Unresolved) {
        ++QTest::blacklists;
    } else {
        // After a BXPASS/Continue, we can subsequently fail.
        // Likewise after a fail or skip in a function called by a test.
        Q_ASSERT(QTest::currentTestState == QTest::Suppressed
                 || QTest::currentTestState == QTest::Skipped);
    }

    QTest::currentTestState = QTest::Suppressed;
    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::BlacklistedFail, msg, file, line);
}

void QTestLog::addBXPass(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::currentTestState == QTest::Unresolved) {
        ++QTest::blacklists;
    } else {
        // After a BXPASS/Continue, we may BXPASS again.
        // Likewise after a fail or skip in a function called by a test.
        Q_ASSERT(QTest::currentTestState == QTest::Suppressed
                 || QTest::currentTestState == QTest::Skipped);
    }

    QTest::currentTestState = QTest::Suppressed;
    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::BlacklistedXPass, msg, file, line);
}

void QTestLog::addBXFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    // Will be counted in addBPass() if we get there.

    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::BlacklistedXFail, msg, file, line);
}

void QTestLog::addSkip(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::currentTestState == QTest::Unresolved) {
        ++QTest::skips;
        QTest::currentTestState = QTest::Skipped;
    } else {
        // After an B?XPASS/Continue, we might subsequently skip.
        // Likewise after a skip in a function called by a test.
        Q_ASSERT(QTest::currentTestState == QTest::Suppressed
                 || QTest::currentTestState == QTest::Failed
                 || QTest::currentTestState == QTest::Skipped);
    }
    // It is up to particular loggers to decide whether to report such
    // subsequent skips; they may carry useful information.

    for (auto &logger : QTest::loggers->allLoggers())
        logger->addIncident(QAbstractTestLogger::Skip, msg, file, line);
}

void QTestLog::addBenchmarkResults(const QList<QBenchmarkResult> &results)
{
    for (auto &logger : QTest::loggers->allLoggers())
        logger->addBenchmarkResults(results);
}

void QTestLog::startLogging()
{
    {
        QMutexLocker locker(&elapsedTimersMutex);
        elapsedTotalTime.start();
        elapsedFunctionTime.start();
    }
    for (auto &logger : QTest::loggers->allLoggers())
        logger->startLogging();
    QTest::oldMessageHandler = qInstallMessageHandler(QTest::messageHandler);
}

void QTestLog::stopLogging()
{
    qInstallMessageHandler(QTest::oldMessageHandler);
    for (auto &logger : QTest::loggers->allLoggers())
        logger->stopLogging();

    QTest::loggers->clear();
    saveCoverageTool(QTestResult::currentAppName(), failCount() != 0, QTestLog::installedTestCoverage());
}

void QTestLog::addLogger(LogMode mode, const char *filename)
{
    if (filename && strcmp(filename, "-") == 0)
        filename = nullptr;

    QAbstractTestLogger *logger = nullptr;
    switch (mode) {
    case QTestLog::Plain:
        logger = new QPlainTestLogger(filename);
        break;
    case QTestLog::CSV:
        logger = new QCsvBenchmarkLogger(filename);
        break;
    case QTestLog::XML:
        logger = new QXmlTestLogger(QXmlTestLogger::Complete, filename);
        break;
    case QTestLog::LightXML:
        logger = new QXmlTestLogger(QXmlTestLogger::Light, filename);
        break;
    case QTestLog::JUnitXML:
        logger = new QJUnitTestLogger(filename);
        break;
    case QTestLog::TeamCity:
        logger = new QTeamCityLogger(filename);
        break;
    case QTestLog::TAP:
        logger = new QTapTestLogger(filename);
        break;
#if defined(QT_USE_APPLE_UNIFIED_LOGGING)
    case QTestLog::Apple:
        logger = new QAppleTestLogger;
        break;
#endif
#if defined(HAVE_XCTEST)
    case QTestLog::XCTest:
        logger = new QXcodeTestLogger;
        break;
#endif
    }

    QTEST_ASSERT(logger);
    addLogger(std::unique_ptr<QAbstractTestLogger>{ logger });
}

/*!
    \internal

    Adds a new logger to the set of loggers that will be used
    to report incidents and messages during testing.
*/
void QTestLog::addLogger(std::unique_ptr<QAbstractTestLogger> logger)
{
    QTEST_ASSERT(logger);
    QTest::loggers()->addLogger(std::move(logger));
}

bool QTestLog::hasLoggers()
{
    return !QTest::loggers()->allLoggers().isEmpty();
}

/*!
    \internal

    Returns true if all loggers support repeated test runs
*/
bool QTestLog::isRepeatSupported()
{
    for (auto &logger : QTest::loggers->allLoggers())
        if (!logger->isRepeatSupported())
            return false;

    return true;
}

bool QTestLog::loggerUsingStdout()
{
    auto loggersCapture = QTest::loggers->allLoggers();
    return q20::ranges::any_of(loggersCapture.begin(), loggersCapture.end(), [](auto &logger) {
        return logger->isLoggingToStdout();
    });
}

void QTestLog::warn(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    for (auto &logger : QTest::loggers->allLoggers())
        logger->addMessage(QAbstractTestLogger::Warn, QString::fromUtf8(msg), file, line);
}

void QTestLog::info(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    for (auto &logger : QTest::loggers->allLoggers())
        logger->addMessage(QAbstractTestLogger::Info, QString::fromUtf8(msg), file, line);
}

void QTestLog::setVerboseLevel(int level)
{
    QTest::verbosity = level;
}

int QTestLog::verboseLevel()
{
    return QTest::verbosity;
}

void QTestLog::ignoreMessage(QtMsgType type, const char *msg)
{
    QTEST_ASSERT(msg);

    const QMutexLocker mutexLocker(&QTest::mutex);
    QTest::IgnoreResultList::append(QTest::ignoreResultList, type, QString::fromUtf8(msg));
}

#if QT_CONFIG(regularexpression)
void QTestLog::ignoreMessage(QtMsgType type, const QRegularExpression &expression)
{
    QTEST_ASSERT(expression.isValid());

    const QMutexLocker mutexLocker(&QTest::mutex);
    QTest::IgnoreResultList::append(QTest::ignoreResultList, type, QVariant(expression));
}
#endif // QT_CONFIG(regularexpression)

void QTestLog::failOnWarning()
{
    QTest::failOnWarningList.push_back({});
}

void QTestLog::failOnWarning(const char *msg)
{
    QTest::failOnWarningList.push_back(QString::fromUtf8(msg));
}

#if QT_CONFIG(regularexpression)
void QTestLog::failOnWarning(const QRegularExpression &expression)
{
    QTEST_ASSERT(expression.isValid());

    QTest::failOnWarningList.push_back(QVariant::fromValue(expression));
}
#endif // QT_CONFIG(regularexpression)

void QTestLog::setMaxWarnings(int m)
{
    QTest::maxWarnings = m <= 0 ? INT_MAX : m + 2;
}

bool QTestLog::printAvailableTags = false;

void QTestLog::setPrintAvailableTagsMode()
{
    printAvailableTags = true;
}

int QTestLog::passCount()
{
    return QTest::passes;
}

int QTestLog::failCount()
{
    return QTest::fails;
}

int QTestLog::skipCount()
{
    return QTest::skips;
}

int QTestLog::blacklistCount()
{
    return QTest::blacklists;
}

int QTestLog::totalCount()
{
    return passCount() + failCount() + skipCount() + blacklistCount();
}

void QTestLog::resetCounters()
{
    QTest::passes = 0;
    QTest::fails = 0;
    QTest::skips = 0;
}

void QTestLog::setInstalledTestCoverage(bool installed)
{
    QTest::installedTestCoverage = installed;
}

bool QTestLog::installedTestCoverage()
{
    return QTest::installedTestCoverage;
}

qint64 QTestLog::nsecsTotalTime()
{
    QMutexLocker locker(&elapsedTimersMutex);
    return elapsedTotalTime.nsecsElapsed();
}

qint64 QTestLog::nsecsFunctionTime()
{
    QMutexLocker locker(&elapsedTimersMutex);
    return elapsedFunctionTime.nsecsElapsed();
}

QT_END_NAMESPACE

#include "moc_qtestlog_p.cpp"
