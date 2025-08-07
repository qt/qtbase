// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestresult_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstringview.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/qtest.h> // toString() specializations for QStringView
#include <QtTest/qtestdata.h>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtesteventloop.h>

#include <climits>
#include <cwchar>
#include <QtCore/q26numeric.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *currentAppName = nullptr;

QT_BEGIN_NAMESPACE

namespace QTest
{
    namespace Internal {
        static bool failed = false;
    }

    static void setFailed(bool failed)
    {
        static const bool fatalFailure = []() {
            static const char * const environmentVar = "QTEST_FATAL_FAIL";
            if (!qEnvironmentVariableIsSet(environmentVar))
                return false;

            bool ok;
            const int fatal = qEnvironmentVariableIntValue(environmentVar, &ok);
            return ok && fatal;
        }();

        if (failed && fatalFailure)
            std::terminate();
        Internal::failed = failed;
    }

    static void resetFailed()
    {
        setFailed(false);
    }

    static bool hasFailed()
    {
        return Internal::failed;
    }

    static QTestData *currentTestData = nullptr;
    static QTestData *currentGlobalTestData = nullptr;
    static const char *currentTestFunc = nullptr;
    static const char *currentTestObjectName = nullptr;
    static bool skipCurrentTest = false;
    static bool blacklistCurrentTest = false;

    static const char *expectFailComment = nullptr;
    static int expectFailMode = 0;
}

/*!
    \class QTestResult
    \inmodule QtTest
    \internal
*/

void QTestResult::reset()
{
    QTest::currentTestData = nullptr;
    QTest::currentGlobalTestData = nullptr;
    QTest::currentTestFunc = nullptr;
    QTest::currentTestObjectName = nullptr;
    QTest::resetFailed();

    QTest::expectFailComment = nullptr;
    QTest::expectFailMode = 0;
    QTest::blacklistCurrentTest = false;

    QTestLog::resetCounters();
}

void QTestResult::setBlacklistCurrentTest(bool b)
{
    QTest::blacklistCurrentTest = b;
}

bool QTestResult::currentTestFailed()
{
    return QTest::hasFailed();
}

QTestData *QTestResult::currentGlobalTestData()
{
    return QTest::currentGlobalTestData;
}

QTestData *QTestResult::currentTestData()
{
    return QTest::currentTestData;
}

void QTestResult::setCurrentGlobalTestData(QTestData *data)
{
    QTest::currentGlobalTestData = data;
}

void QTestResult::setCurrentTestData(QTestData *data)
{
    QTest::currentTestData = data;
    QTest::resetFailed();
    if (data)
        QTestLog::enterTestData(data);
}

void QTestResult::setCurrentTestFunction(const char *func)
{
    QTest::currentTestFunc = func;
    QTest::resetFailed();
    if (func)
        QTestLog::enterTestFunction(func);
}

static void clearExpectFail()
{
    QTest::expectFailMode = 0;
    delete [] const_cast<char *>(QTest::expectFailComment);
    QTest::expectFailComment = nullptr;
}

/*!
    This function is called after completing each test function,
    including test functions that are not data-driven.

    For data-driven functions, this is called after each call to the test
    function, with distinct data. Otherwise, this function is called once,
    with currentTestData() and currentGlobalTestData() set to \nullptr.

    The function is called before the test's cleanup(), if it has one.

    For benchmarks, this will be called after each repeat of a function
    (with the same data row), when the benchmarking code decides to
    re-run one to get sufficient data.

    \sa finishedCurrentTestDataCleanup()
*/
void QTestResult::finishedCurrentTestData()
{
    if (QTest::expectFailMode)
        addFailure("QEXPECT_FAIL was called without any subsequent verification statements");

    clearExpectFail();
}

/*!
    This function is called after completing each test function,
    including test functions that are not data-driven.

    For data-driven functions, this is called after each call to the test
    function, with distinct data. Otherwise, this function is called once,
    with currentTestData() and currentGlobalTestData() set to \nullptr.

    The function is called after the test's cleanup(), if it has one.

    For benchmarks, this is called after all repeat calls to the function
    (with a given data row).

    \sa finishedCurrentTestData()
*/
void QTestResult::finishedCurrentTestDataCleanup()
{
    if (!QTest::hasFailed() && QTestLog::unhandledIgnoreMessages()) {
        QTestLog::printUnhandledIgnoreMessages();
        addFailure("Not all expected messages were received");
    }

    // If the current test hasn't failed or been skipped, then it passes.
    if (!QTest::hasFailed() && !QTest::skipCurrentTest) {
        if (QTest::blacklistCurrentTest)
            QTestLog::addBPass("");
        else
            QTestLog::addPass("");
    }

    QTestLog::clearCurrentTestState();
    QTest::resetFailed();
}

/*!
    This function is called after completing each test function,
    including test functions that are data-driven.

    For data-driven functions, this is called after after all data rows
    have been tested, and the data table has been cleared, so both
    currentTestData() and currentGlobalTestData() will be \nullptr.
*/
void QTestResult::finishedCurrentTestFunction()
{
    QTestLog::clearCurrentTestState(); // Needed if _data() skipped.
    QTestLog::leaveTestFunction();

    QTest::currentTestFunc = nullptr;
    QTest::resetFailed();
}

const char *QTestResult::currentTestFunction()
{
    return QTest::currentTestFunc;
}

const char *QTestResult::currentDataTag()
{
    return QTest::currentTestData ? QTest::currentTestData->dataTag() : nullptr;
}

const char *QTestResult::currentGlobalDataTag()
{
    return QTest::currentGlobalTestData ? QTest::currentGlobalTestData->dataTag() : nullptr;
}

static bool isExpectFailData(const char *dataIndex)
{
    if (!dataIndex || dataIndex[0] == '\0')
        return true;
    if (!QTest::currentTestData)
        return false;
    if (strcmp(dataIndex, QTest::currentTestData->dataTag()) == 0)
        return true;
    return false;
}

bool QTestResult::expectFail(const char *dataIndex, const char *comment,
                             QTest::TestFailMode mode, const char *file, int line)
{
    QTEST_ASSERT(comment);
    QTEST_ASSERT(mode > 0);

    if (!isExpectFailData(dataIndex)) {
        delete[] comment;
        return true; // we don't care
    }

    if (QTest::expectFailMode) {
        delete[] comment;
        addFailure("Already expecting a fail", file, line);
        return false;
    }

    QTest::expectFailMode = mode;
    QTest::expectFailComment = comment;
    return true;
}

static bool checkStatement(bool statement, const char *msg, const char *file, int line)
{
    if (statement) {
        if (QTest::expectFailMode) {
            if (QTest::blacklistCurrentTest)
                QTestLog::addBXPass(msg, file, line);
            else
                QTestLog::addXPass(msg, file, line);

            QTest::setFailed(true);
            // Should B?XPass always (a) continue or (b) abort, regardless of mode ?
            bool doContinue = (QTest::expectFailMode == QTest::Continue);
            clearExpectFail();
            return doContinue;
        }
        return true;
    }

    if (QTest::expectFailMode) {
        if (QTest::blacklistCurrentTest)
            QTestLog::addBXFail(QTest::expectFailComment, file, line);
        else
            QTestLog::addXFail(QTest::expectFailComment, file, line);
        bool doContinue = (QTest::expectFailMode == QTest::Continue);
        clearExpectFail();
        return doContinue;
    }

    QTestResult::addFailure(msg, file, line);
    return false;
}

void QTestResult::fail(const char *msg, const char *file, int line)
{
    checkStatement(false, msg, file, line);
}

// QPalette's << operator produces 1363 characters. A comparison failure
// involving two palettes can therefore require 2726 characters, not including
// the other output produced by QTest. Users might also have their own types
// with large amounts of output, so use a sufficiently high value here.
static constexpr size_t maxMsgLen = 4096;

bool QTestResult::verify(bool statement, const char *statementStr,
                         const char *description, const char *file, int line)
{
    QTEST_ASSERT(statementStr);

    Q_DECL_UNINITIALIZED char msg[maxMsgLen];
    msg[0] = '\0';

    if (QTestLog::verboseLevel() >= 2) {
        std::snprintf(msg, maxMsgLen, "QVERIFY(%s)", statementStr);
        QTestLog::info(msg, file, line);
    }

    if (statement == !!QTest::expectFailMode) {
        std::snprintf(msg, maxMsgLen,
                      statement ? "'%s' returned TRUE unexpectedly. (%s)" : "'%s' returned FALSE. (%s)",
                      statementStr, description ? description : "");
    }

    return checkStatement(statement, msg, file, line);
}

static const char *leftArgNameForOp(QTest::ComparisonOperation op)
{
    switch (op) {
    case QTest::ComparisonOperation::CustomCompare:
        return "Actual   ";
    case QTest::ComparisonOperation::ThreeWayCompare:
        return "Left     ";
    case QTest::ComparisonOperation::Equal:
    case QTest::ComparisonOperation::NotEqual:
    case QTest::ComparisonOperation::LessThan:
    case QTest::ComparisonOperation::LessThanOrEqual:
    case QTest::ComparisonOperation::GreaterThan:
    case QTest::ComparisonOperation::GreaterThanOrEqual:
        return "Computed ";
    }
    Q_UNREACHABLE_RETURN("");
}

static const char *rightArgNameForOp(QTest::ComparisonOperation op)
{
    switch (op) {
    case QTest::ComparisonOperation::CustomCompare:
        return "Expected ";
    case QTest::ComparisonOperation::ThreeWayCompare:
        return "Right    ";
    case QTest::ComparisonOperation::Equal:
    case QTest::ComparisonOperation::NotEqual:
    case QTest::ComparisonOperation::LessThan:
    case QTest::ComparisonOperation::LessThanOrEqual:
    case QTest::ComparisonOperation::GreaterThan:
    case QTest::ComparisonOperation::GreaterThanOrEqual:
        return "Baseline ";
    }
    Q_UNREACHABLE_RETURN("");
}

static int approx_wide_len(const char *s)
{
    std::mbstate_t state = {};
    // QNX might stop at max when dst == nullptr, so pass INT_MAX,
    // being the largest value this function will return:
    auto r = std::mbsrtowcs(nullptr, &s, INT_MAX, &state);
    if (r == size_t(-1)) // encoding error, fall back to strlen()
        r = strlen(s); // `s` was not advanced since `dst == nullptr`
    return q26::saturate_cast<int>(r);
}

// Overload to format failures for "const char *" - no need to strdup().
static Q_DECL_COLD_FUNCTION
void formatFailMessage(char *msg, size_t maxMsgLen,
                       const char *failureMsg,
                       const char *val1, const char *val2,
                       const char *actual, const char *expected,
                       QTest::ComparisonOperation op)
{
    const auto len1 = approx_wide_len(actual);
    const auto len2 = approx_wide_len(expected);
    const int written = std::snprintf(msg, maxMsgLen, "%s\n", failureMsg);
    msg += written;
    maxMsgLen -= written;

    const auto protect = [](const char *s) { return s ? s : "<null>"; };

    if (val1 || val2) {
        std::snprintf(msg, maxMsgLen, "   %s(%s)%*s %s\n   %s(%s)%*s %s",
                      leftArgNameForOp(op), actual, qMax(len1, len2) - len1 + 1, ":",
                      protect(val1),
                      rightArgNameForOp(op), expected, qMax(len1, len2) - len2 + 1, ":",
                      protect(val2));
    } else {
        // only print variable names if neither value can be represented as a string
        std::snprintf(msg, maxMsgLen, "   %s: %s\n   %s: %s",
                      leftArgNameForOp(op), actual, rightArgNameForOp(op), expected);
    }
}

const char *
QTest::Internal::formatPropertyTestHelperFailure(char *msg, size_t maxMsgLen,
                                                 const char *actual, const char *expected,
                                                 const char *actualExpr, const char *expectedExpr)
{
    formatFailMessage(msg, maxMsgLen, "Comparison failed!",
                      actual, expected, actualExpr, expectedExpr,
                      QTest::ComparisonOperation::CustomCompare);
    return msg;
}

// Format failures using the toString() template
template <class Actual, class Expected>
static Q_DECL_COLD_FUNCTION
void formatFailMessage(char *msg, size_t maxMsgLen,
                       const char *failureMsg,
                       const Actual &val1, const Expected &val2,
                       const char *actual, const char *expected,
                       QTest::ComparisonOperation op)
{
    const char *val1S = QTest::toString(val1);
    const char *val2S = QTest::toString(val2);

    formatFailMessage(msg, maxMsgLen, failureMsg, val1S, val2S, actual, expected, op);

    delete [] val1S;
    delete [] val2S;
}

template <class Actual, class Expected>
static bool compareHelper(bool success, const char *failureMsg,
                          const Actual &val1, const Expected &val2,
                          const char *actual, const char *expected,
                          const char *file, int line,
                          bool hasValues = true)
{
    Q_DECL_UNINITIALIZED char msg[maxMsgLen];
    msg[0] = '\0';

    QTEST_ASSERT(expected);
    QTEST_ASSERT(actual);

    if (QTestLog::verboseLevel() >= 2) {
        std::snprintf(msg, maxMsgLen, "QCOMPARE(%s, %s)", actual, expected);
        QTestLog::info(msg, file, line);
    }

    if (!failureMsg)
        failureMsg = "Compared values are not the same";

    if (success) {
        if (QTest::expectFailMode) {
            std::snprintf(msg, maxMsgLen,
                          "QCOMPARE(%s, %s) returned TRUE unexpectedly.", actual, expected);
        }
        return checkStatement(success, msg, file, line);
    }


    if (!hasValues) {
        std::snprintf(msg, maxMsgLen, "%s", failureMsg);
        return checkStatement(success, msg, file, line);
    }

    formatFailMessage(msg, maxMsgLen, failureMsg, val1, val2, actual, expected,
                      QTest::ComparisonOperation::CustomCompare);

    return checkStatement(success, msg, file, line);
}

// A simplified version of compareHelper that does not use string
// representations of the values, and prints only failureMsg when the
// comparison fails.
static bool compareHelper(bool success, const char *failureMsg,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    const size_t maxMsgLen = 1024;
    Q_DECL_UNINITIALIZED char msg[maxMsgLen];
    msg[0] = '\0';

    QTEST_ASSERT(expected);
    QTEST_ASSERT(actual);
    // failureMsg can be null, if we do not use it
    QTEST_ASSERT(success || failureMsg);

    if (QTestLog::verboseLevel() >= 2) {
        std::snprintf(msg, maxMsgLen, "QCOMPARE(%s, %s)", actual, expected);
        QTestLog::info(msg, file, line);
    }

    if (success) {
        if (QTest::expectFailMode) {
            std::snprintf(msg, maxMsgLen, "QCOMPARE(%s, %s) returned TRUE unexpectedly.",
                          actual, expected);
        }
        return checkStatement(success, msg, file, line);
    }

    return checkStatement(success, failureMsg, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          char *val1, char *val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    const bool result = compareHelper(success, failureMsg,
                                      val1 != nullptr ? val1 : "<null>",
                                      val2 != nullptr ? val2 : "<null>",
                                      actual, expected, file, line,
                                      val1 != nullptr && val2 != nullptr);

    // Our caller got these from QTest::toString()
    delete [] val1;
    delete [] val2;

    return result;
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          double val1, double val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          float val1, float val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          int val1, int val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

#if QT_POINTER_SIZE == 8
bool QTestResult::compare(bool success, const char *failureMsg,
                          qsizetype val1, qsizetype val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}
#endif // QT_POINTER_SIZE == 8

bool QTestResult::compare(bool success, const char *failureMsg,
                          unsigned val1, unsigned val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          QStringView val1, QStringView val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          QStringView val1, const QLatin1StringView &val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          const QLatin1StringView & val1, QStringView val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

// Simplified version of compare() that does not take the values, because they
// can't be converted to strings (or the user didn't want to do that).
bool QTestResult::compare(bool success, const char *failureMsg,
                          const char *actual, const char *expeceted,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, actual, expeceted, file, line);
}

void QTestResult::addFailure(const char *message, const char *file, int line)
{
    clearExpectFail();
    if (qApp && QThread::currentThread() == qApp->thread())
        QTestEventLoop::instance().exitLoop();

    if (QTest::blacklistCurrentTest)
        QTestLog::addBFail(message, file, line);
    else
        QTestLog::addFail(message, file, line);
    QTest::setFailed(true);
}

void QTestResult::addSkip(const char *message, const char *file, int line)
{
    clearExpectFail();

    QTestLog::addSkip(message, file, line);
}

void QTestResult::setCurrentTestObject(const char *name)
{
    QTest::currentTestObjectName = name;
}

const char *QTestResult::currentTestObjectName()
{
    return QTest::currentTestObjectName ? QTest::currentTestObjectName : "";
}

void QTestResult::setSkipCurrentTest(bool value)
{
    QTest::skipCurrentTest = value;
}

bool QTestResult::skipCurrentTest()
{
    return QTest::skipCurrentTest;
}

void QTestResult::setCurrentAppName(const char *appName)
{
    ::currentAppName = appName;
}

const char *QTestResult::currentAppName()
{
    return ::currentAppName;
}

static const char *macroNameForOp(QTest::ComparisonOperation op)
{
    using namespace QTest;
    switch (op) {
    case ComparisonOperation::CustomCompare:
        return "QCOMPARE"; /* not used */
    case ComparisonOperation::Equal:
        return "QCOMPARE_EQ";
    case ComparisonOperation::NotEqual:
        return "QCOMPARE_NE";
    case ComparisonOperation::LessThan:
        return "QCOMPARE_LT";
    case ComparisonOperation::LessThanOrEqual:
        return "QCOMPARE_LE";
    case ComparisonOperation::GreaterThan:
        return "QCOMPARE_GT";
    case ComparisonOperation::GreaterThanOrEqual:
        return "QCOMPARE_GE";
    case ComparisonOperation::ThreeWayCompare:
        return "QCOMPARE_3WAY";
    }
    Q_UNREACHABLE_RETURN("");
}

static const char *failureMessageForOp(QTest::ComparisonOperation op)
{
    using namespace QTest;
    switch (op) {
    case ComparisonOperation::CustomCompare:
        return "Compared values are not the same"; /* not used */
    case ComparisonOperation::ThreeWayCompare:
        return "The result of operator<=>() is not what was expected";
    case ComparisonOperation::Equal:
        return "The computed value is expected to be equal to the baseline, but is not";
    case ComparisonOperation::NotEqual:
        return "The computed value is expected to be different from the baseline, but is not";
    case ComparisonOperation::LessThan:
        return "The computed value is expected to be less than the baseline, but is not";
    case ComparisonOperation::LessThanOrEqual:
        return "The computed value is expected to be less than or equal to the baseline, but is not";
    case ComparisonOperation::GreaterThan:
        return "The computed value is expected to be greater than the baseline, but is not";
    case ComparisonOperation::GreaterThanOrEqual:
        return "The computed value is expected to be greater than or equal to the baseline, but is not";
    }
    Q_UNREACHABLE_RETURN("");
}

bool QTestResult::reportResult(bool success, const void *lhs, const void *rhs,
                               const char *(*lhsFormatter)(const void*),
                               const char *(*rhsFormatter)(const void*),
                               const char *lhsExpr, const char *rhsExpr,
                               QTest::ComparisonOperation op, const char *file, int line,
                               const char *failureMessage)
{
    Q_DECL_UNINITIALIZED char msg[maxMsgLen];
    msg[0] = '\0';

    QTEST_ASSERT(lhsExpr);
    QTEST_ASSERT(rhsExpr);

    if (QTestLog::verboseLevel() >= 2) {
        std::snprintf(msg, maxMsgLen, "%s(%s, %s)", macroNameForOp(op), lhsExpr, rhsExpr);
        QTestLog::info(msg, file, line);
    }

    if (success) {
        if (QTest::expectFailMode) {
            std::snprintf(msg, maxMsgLen, "%s(%s, %s) returned TRUE unexpectedly.",
                          macroNameForOp(op), lhsExpr, rhsExpr);
        }
        return checkStatement(success, msg, file, line);
    }

    const std::unique_ptr<const char[]> lhsPtr{ lhsFormatter(lhs) };
    const std::unique_ptr<const char[]> rhsPtr{ rhsFormatter(rhs) };

    if (!failureMessage)
        failureMessage = failureMessageForOp(op);

    formatFailMessage(msg, maxMsgLen, failureMessage, lhsPtr.get(), rhsPtr.get(),
                      lhsExpr, rhsExpr, op);

    return checkStatement(success, msg, file, line);
}

bool QTestResult::report3WayResult(bool success,
                                   const char *failureMessage,
                                   const void *lhs, const void *rhs,
                                   const char *(*lhsFormatter)(const void*),
                                   const char *(*rhsFormatter)(const void*),
                                   const char *lhsExpression, const char *rhsExpression,
                                   const char *(*actualOrderFormatter)(const void *),
                                   const char *(*expectedOrderFormatter)(const void *),
                                   const void *actualOrder, const void *expectedOrder,
                                   const char *expectedExpression,
                                   const char *file, int line)
{
    char msg[maxMsgLen];
    msg[0] = '\0';

    QTEST_ASSERT(lhsExpression);
    QTEST_ASSERT(rhsExpression);
    QTEST_ASSERT(expectedExpression);
    const char *macroName = macroNameForOp(QTest::ComparisonOperation::ThreeWayCompare);
    const std::string actualExpression = std::string(lhsExpression) + " <=> " + rhsExpression;

    if (QTestLog::verboseLevel() >= 2) {
        std::snprintf(msg, maxMsgLen, "%s(%s, %s, %s)",
                      macroName, lhsExpression, rhsExpression, expectedExpression);
        QTestLog::info(msg, file, line);
    }

    if (success) {
        if (QTest::expectFailMode) {
            std::snprintf(msg, maxMsgLen, "%s(%s, %s, %s) returned TRUE unexpectedly.",
                          macroName, lhsExpression, rhsExpression, expectedExpression);
        }
        return checkStatement(success, msg, file, line);
    }
    const std::unique_ptr<const char[]> lhsStr{lhsFormatter(lhs)};
    const std::unique_ptr<const char[]> rhsStr{rhsFormatter(rhs)};

    const std::unique_ptr<const char[]> actual{actualOrderFormatter(actualOrder)};
    const std::unique_ptr<const char[]> expected{expectedOrderFormatter(expectedOrder)};

    if (!failureMessage)
        failureMessage = failureMessageForOp(QTest::ComparisonOperation::ThreeWayCompare);

    // Left and Right compared parameters of QCOMPARE_3WAY
    formatFailMessage(msg, maxMsgLen, failureMessage,
                      lhsStr.get(), rhsStr.get(),
                      lhsExpression, rhsExpression,
                      QTest::ComparisonOperation::ThreeWayCompare);

    // Actual and Expected results of comparison
    formatFailMessage(msg + strlen(msg), maxMsgLen - strlen(msg), "",
                      actual.get(), expected.get(),
                      actualExpression.c_str(), expectedExpression,
                      QTest::ComparisonOperation::CustomCompare);

    return checkStatement(success, msg, file, line);
}

QT_END_NAMESPACE
