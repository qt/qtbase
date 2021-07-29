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

#include <QtTest/private/qjunittestlogger_p.h>
#include <QtTest/private/qtestelement_p.h>
#include <QtTest/private/qtestjunitstreamer_p.h>
#include <QtTest/qtestcase.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qtestlog_p.h>

#ifdef min // windows.h without NOMINMAX is included by the benchmark headers.
#  undef min
#endif
#ifdef max
#  undef max
#endif

#include <QtCore/qlibraryinfo.h>

#include <string.h>

QT_BEGIN_NAMESPACE

QJUnitTestLogger::QJUnitTestLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QJUnitTestLogger::~QJUnitTestLogger()
{
    Q_ASSERT(!currentTestSuite);
    delete logFormatter;
}

// We track test timing per test case, so we
// need to maintain our own elapsed timer.
static QElapsedTimer elapsedTestcaseTime;
static qreal elapsedTestCaseSeconds()
{
    return elapsedTestcaseTime.nsecsElapsed() / 1e9;
}

static QByteArray toSecondsFormat(qreal ms)
{
    return QByteArray::number(ms / 1000, 'f', 3);
}

void QJUnitTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    logFormatter = new QTestJUnitStreamer(this);
    delete systemOutputElement;
    systemOutputElement = new QTestElement(QTest::LET_SystemOutput);
    delete systemErrorElement;
    systemErrorElement = new QTestElement(QTest::LET_SystemError);

    Q_ASSERT(!currentTestSuite);
    currentTestSuite = new QTestElement(QTest::LET_TestSuite);
    currentTestSuite->addAttribute(QTest::AI_Name, QTestResult::currentTestObjectName());

    auto localTime = QDateTime::currentDateTime();
    currentTestSuite->addAttribute(QTest::AI_Timestamp,
        localTime.toString(Qt::ISODate).toUtf8().constData());

    currentTestSuite->addAttribute(QTest::AI_Hostname,
        QSysInfo::machineHostName().toUtf8().constData());

    QTestElement *property;
    QTestElement *properties = new QTestElement(QTest::LET_Properties);

    property = new QTestElement(QTest::LET_Property);
    property->addAttribute(QTest::AI_Name, "QTestVersion");
    property->addAttribute(QTest::AI_PropertyValue, QTEST_VERSION_STR);
    properties->addLogElement(property);

    property = new QTestElement(QTest::LET_Property);
    property->addAttribute(QTest::AI_Name, "QtVersion");
    property->addAttribute(QTest::AI_PropertyValue, qVersion());
    properties->addLogElement(property);

    property = new QTestElement(QTest::LET_Property);
    property->addAttribute(QTest::AI_Name, "QtBuild");
    property->addAttribute(QTest::AI_PropertyValue, QLibraryInfo::build());
    properties->addLogElement(property);

    currentTestSuite->addLogElement(properties);

    elapsedTestcaseTime.start();
}

void QJUnitTestLogger::stopLogging()
{
    char buf[10];

    qsnprintf(buf, sizeof(buf), "%i", testCounter);
    currentTestSuite->addAttribute(QTest::AI_Tests, buf);

    qsnprintf(buf, sizeof(buf), "%i", failureCounter);
    currentTestSuite->addAttribute(QTest::AI_Failures, buf);

    qsnprintf(buf, sizeof(buf), "%i", errorCounter);
    currentTestSuite->addAttribute(QTest::AI_Errors, buf);

    qsnprintf(buf, sizeof(buf), "%i", QTestLog::skipCount());
    currentTestSuite->addAttribute(QTest::AI_Skipped, buf);

    currentTestSuite->addAttribute(QTest::AI_Time,
        toSecondsFormat(QTestLog::msecsTotalTime()).constData());

    currentTestSuite->addLogElement(listOfTestcases);

    // For correct indenting, make sure every testcase knows its parent
    QTestElement *testcase = listOfTestcases;
    while (testcase) {
        testcase->setParent(currentTestSuite);
        testcase = testcase->nextElement();
    }

    currentTestSuite->addLogElement(systemOutputElement);
    currentTestSuite->addLogElement(systemErrorElement);

    logFormatter->output(currentTestSuite);

    delete currentTestSuite;
    currentTestSuite = nullptr;

    QAbstractTestLogger::stopLogging();
}

void QJUnitTestLogger::enterTestFunction(const char *function)
{
    enterTestCase(function);
}

void QJUnitTestLogger::enterTestCase(const char *name)
{
    currentLogElement = new QTestElement(QTest::LET_TestCase);
    currentLogElement->addAttribute(QTest::AI_Name, name);
    currentLogElement->addAttribute(QTest::AI_Classname, QTestResult::currentTestObjectName());
    currentLogElement->addToList(&listOfTestcases);

    // The element will be deleted when the suite is deleted

    ++testCounter;

    elapsedTestcaseTime.restart();
}

void QJUnitTestLogger::enterTestData(QTestData *)
{
    QTestCharBuffer testIdentifier;
    QTestPrivate::generateTestIdentifier(&testIdentifier,
        QTestPrivate::TestFunction | QTestPrivate::TestDataTag);

    static const char *lastTestFunction = nullptr;
    if (QTestResult::currentTestFunction() != lastTestFunction) {
        // Adopt existing testcase for the initial test data
        auto *name = const_cast<QTestElementAttribute*>(
            currentLogElement->attribute(QTest::AI_Name));
        name->setPair(QTest::AI_Name, testIdentifier.data());
        lastTestFunction = QTestResult::currentTestFunction();
        elapsedTestcaseTime.restart();
    } else {
        // Create new test cases for remaining test data
        leaveTestCase();
        enterTestCase(testIdentifier.data());
    }
}

void QJUnitTestLogger::leaveTestFunction()
{
    leaveTestCase();
}

void QJUnitTestLogger::leaveTestCase()
{
    currentLogElement->addAttribute(QTest::AI_Time,
        toSecondsFormat(elapsedTestCaseSeconds()).constData());
}

void QJUnitTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    const char *typeBuf = nullptr;

    switch (type) {
    case QAbstractTestLogger::XPass:
        ++failureCounter;
        typeBuf = "xpass";
        break;
    case QAbstractTestLogger::Pass:
        typeBuf = "pass";
        break;
    case QAbstractTestLogger::XFail:
        typeBuf = "xfail";
        break;
    case QAbstractTestLogger::Fail:
        ++failureCounter;
        typeBuf = "fail";
        break;
    case QAbstractTestLogger::BlacklistedPass:
        typeBuf = "bpass";
        break;
    case QAbstractTestLogger::BlacklistedFail:
        ++failureCounter;
        typeBuf = "bfail";
        break;
    case QAbstractTestLogger::BlacklistedXPass:
        typeBuf = "bxpass";
        break;
    case QAbstractTestLogger::BlacklistedXFail:
        ++failureCounter;
        typeBuf = "bxfail";
        break;
    default:
        typeBuf = "??????";
        break;
    }

    if (type == QAbstractTestLogger::Fail || type == QAbstractTestLogger::XPass) {
        QTestElement *failureElement = new QTestElement(QTest::LET_Failure);
        failureElement->addAttribute(QTest::AI_Type, typeBuf);
        failureElement->addAttribute(QTest::AI_Message, description);
        currentLogElement->addLogElement(failureElement);
    }

    /*
        Since XFAIL does not add a failure to the testlog in junitxml, add a message, so we still
        have some information about the expected failure.
    */
    if (type == QAbstractTestLogger::XFail) {
        QJUnitTestLogger::addMessage(QAbstractTestLogger::Info, QString::fromUtf8(description), file, line);
    }
}

void QJUnitTestLogger::addMessage(MessageTypes type, const QString &message, const char *file, int line)
{
    Q_UNUSED(file);
    Q_UNUSED(line);

    if (type == QAbstractTestLogger::Skip) {
        auto skippedElement = new QTestElement(QTest::LET_Skipped);
        skippedElement->addAttribute(QTest::AI_Message, message.toUtf8().constData());
        currentLogElement->addLogElement(skippedElement);
        return;
    }

    auto messageElement = new QTestElement(QTest::LET_Message);
    auto systemLogElement = systemOutputElement;
    const char *typeBuf = nullptr;

    switch (type) {
    case QAbstractTestLogger::Warn:
        systemLogElement = systemErrorElement;
        typeBuf = "warn";
        break;
    case QAbstractTestLogger::QSystem:
        typeBuf = "system";
        break;
    case QAbstractTestLogger::QDebug:
        typeBuf = "qdebug";
        break;
    case QAbstractTestLogger::QInfo:
        typeBuf = "qinfo";
        break;
    case QAbstractTestLogger::QWarning:
        systemLogElement = systemErrorElement;
        typeBuf = "qwarn";
        break;
    case QAbstractTestLogger::QFatal:
        systemLogElement = systemErrorElement;
        typeBuf = "qfatal";
        break;
    case QAbstractTestLogger::Skip:
        Q_UNREACHABLE();
        break;
    case QAbstractTestLogger::Info:
        typeBuf = "info";
        break;
    default:
        typeBuf = "??????";
        break;
    }

    messageElement->addAttribute(QTest::AI_Type, typeBuf);
    messageElement->addAttribute(QTest::AI_Message, message.toUtf8().constData());

    currentLogElement->addLogElement(messageElement);

    // Also add the message to the system log (stdout/stderr), if one exists
    if (systemLogElement) {
        auto messageElement = new QTestElement(QTest::LET_Message);
        messageElement->addAttribute(QTest::AI_Message, message.toUtf8().constData());
        systemLogElement->addLogElement(messageElement);
    }
}

QT_END_NAMESPACE

