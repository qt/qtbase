/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/private/qtestresult_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qstringview.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/qtest.h> // toString() specializations for QStringView
#include <QtTest/qtestdata.h>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestassert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *currentAppName = nullptr;

QT_BEGIN_NAMESPACE

namespace QTest
{
    static QTestData *currentTestData = nullptr;
    static QTestData *currentGlobalTestData = nullptr;
    static const char *currentTestFunc = nullptr;
    static const char *currentTestObjectName = nullptr;
    static bool failed = false;
    static bool skipCurrentTest = false;
    static bool blacklistCurrentTest = false;

    static const char *expectFailComment = nullptr;
    static int expectFailMode = 0;
}

void QTestResult::reset()
{
    QTest::currentTestData = nullptr;
    QTest::currentGlobalTestData = nullptr;
    QTest::currentTestFunc = nullptr;
    QTest::currentTestObjectName = nullptr;
    QTest::failed = false;

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
    return QTest::failed;
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
    QTest::failed = false;
    if (data)
        QTestLog::enterTestData(data);
}

void QTestResult::setCurrentTestFunction(const char *func)
{
    QTest::currentTestFunc = func;
    QTest::failed = false;
    if (func)
        QTestLog::enterTestFunction(func);
}

static void clearExpectFail()
{
    QTest::expectFailMode = 0;
    delete [] const_cast<char *>(QTest::expectFailComment);
    QTest::expectFailComment = nullptr;
}

void QTestResult::finishedCurrentTestData()
{
    if (QTest::expectFailMode)
        addFailure("QEXPECT_FAIL was called without any subsequent verification statements", nullptr, 0);
    clearExpectFail();

    if (!QTest::failed && QTestLog::unhandledIgnoreMessages()) {
        QTestLog::printUnhandledIgnoreMessages();
        addFailure("Not all expected messages were received", nullptr, 0);
    }
    QTestLog::clearIgnoreMessages();
}

void QTestResult::finishedCurrentTestDataCleanup()
{
    // If the current test hasn't failed or been skipped, then it passes.
    if (!QTest::failed && !QTest::skipCurrentTest) {
        if (QTest::blacklistCurrentTest)
            QTestLog::addBPass("");
        else
            QTestLog::addPass("");
    }

    QTest::failed = false;
}

void QTestResult::finishedCurrentTestFunction()
{
    QTest::currentTestFunc = nullptr;
    QTest::failed = false;

    QTestLog::leaveTestFunction();
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
        clearExpectFail();
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

            QTest::failed = true;
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

bool QTestResult::verify(bool statement, const char *statementStr,
                         const char *description, const char *file, int line)
{
    QTEST_ASSERT(statementStr);

    char msg[1024] = {'\0'};

    if (QTestLog::verboseLevel() >= 2) {
        qsnprintf(msg, 1024, "QVERIFY(%s)", statementStr);
        QTestLog::info(msg, file, line);
    }

    if (!statement && !QTest::expectFailMode)
        qsnprintf(msg, 1024, "'%s' returned FALSE. (%s)", statementStr, description ? description : "");
    else if (statement && QTest::expectFailMode)
        qsnprintf(msg, 1024, "'%s' returned TRUE unexpectedly. (%s)", statementStr, description ? description : "");

    return checkStatement(statement, msg, file, line);
}

// Format failures using the toString() template
template <class Actual, class Expected>
void formatFailMessage(char *msg, size_t maxMsgLen,
                       const char *failureMsg,
                       const Actual &val1, const Expected &val2,
                       const char *actual, const char *expected)
{
    auto val1S = QTest::toString(val1);
    auto val2S = QTest::toString(val2);

    size_t len1 = mbstowcs(nullptr, actual, maxMsgLen);    // Last parameter is not ignored on QNX
    size_t len2 = mbstowcs(nullptr, expected, maxMsgLen);  // (result is never larger than this).
    qsnprintf(msg, maxMsgLen, "%s\n   Actual   (%s)%*s %s\n   Expected (%s)%*s %s",
              failureMsg,
              actual, qMax(len1, len2) - len1 + 1, ":", val1S ? val1S : "<null>",
              expected, qMax(len1, len2) - len2 + 1, ":", val2S ? val2S : "<null>");

    delete [] val1S;
    delete [] val2S;
}

// Overload to format failures for "const char *" - no need to strdup().
void formatFailMessage(char *msg, size_t maxMsgLen,
                       const char *failureMsg,
                       const char *val1, const char *val2,
                       const char *actual, const char *expected)
{
    size_t len1 = mbstowcs(nullptr, actual, maxMsgLen);    // Last parameter is not ignored on QNX
    size_t len2 = mbstowcs(nullptr, expected, maxMsgLen);  // (result is never larger than this).
    qsnprintf(msg, maxMsgLen, "%s\n   Actual   (%s)%*s %s\n   Expected (%s)%*s %s",
              failureMsg,
              actual, qMax(len1, len2) - len1 + 1, ":", val1 ? val1 : "<null>",
              expected, qMax(len1, len2) - len2 + 1, ":", val2 ? val2 : "<null>");
}

template <class Actual, class Expected>
static bool compareHelper(bool success, const char *failureMsg,
                          const Actual &val1, const Expected &val2,
                          const char *actual, const char *expected,
                          const char *file, int line,
                          bool hasValues = true)
{
    const size_t maxMsgLen = 1024;
    char msg[maxMsgLen] = {'\0'};

    QTEST_ASSERT(expected);
    QTEST_ASSERT(actual);

    if (QTestLog::verboseLevel() >= 2) {
        qsnprintf(msg, maxMsgLen, "QCOMPARE(%s, %s)", actual, expected);
        QTestLog::info(msg, file, line);
    }

    if (!failureMsg)
        failureMsg = "Compared values are not the same";

    if (success) {
        if (QTest::expectFailMode) {
            qsnprintf(msg, maxMsgLen,
                      "QCOMPARE(%s, %s) returned TRUE unexpectedly.", actual, expected);
        }
        return checkStatement(success, msg, file, line);
    }


    if (!hasValues) {
        qsnprintf(msg, maxMsgLen, "%s", failureMsg);
        return checkStatement(success, msg, file, line);
    }

    formatFailMessage(msg, maxMsgLen, failureMsg, val1, val2, actual, expected);

    return checkStatement(success, msg, file, line);
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
                          QStringView val1, const QLatin1String &val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

bool QTestResult::compare(bool success, const char *failureMsg,
                          const QLatin1String & val1, QStringView val2,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    return compareHelper(success, failureMsg, val1, val2, actual, expected, file, line);
}

void QTestResult::addFailure(const char *message, const char *file, int line)
{
    clearExpectFail();

    if (QTest::blacklistCurrentTest)
        QTestLog::addBFail(message, file, line);
    else
        QTestLog::addFail(message, file, line);
    QTest::failed = true;
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

QT_END_NAMESPACE
