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
    properties->addChild(property);

    property = new QTestElement(QTest::LET_Property);
    property->addAttribute(QTest::AI_Name, "QtVersion");
    property->addAttribute(QTest::AI_PropertyValue, qVersion());
    properties->addChild(property);

    property = new QTestElement(QTest::LET_Property);
    property->addAttribute(QTest::AI_Name, "QtBuild");
    property->addAttribute(QTest::AI_PropertyValue, QLibraryInfo::build());
    properties->addChild(property);

    currentTestSuite->addChild(properties);

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

    for (auto *testCase : listOfTestcases)
        currentTestSuite->addChild(testCase);
    listOfTestcases.clear();

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
    currentTestCase = new QTestElement(QTest::LET_TestCase);
    currentTestCase->addAttribute(QTest::AI_Name, name);
    currentTestCase->addAttribute(QTest::AI_Classname, QTestResult::currentTestObjectName());
    listOfTestcases.push_back(currentTestCase);

    Q_ASSERT(!systemOutputElement && !systemErrorElement);
    systemOutputElement = new QTestElement(QTest::LET_SystemOutput);
    systemErrorElement = new QTestElement(QTest::LET_SystemError);

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
            currentTestCase->attribute(QTest::AI_Name));
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
    currentTestCase->addAttribute(QTest::AI_Time,
        toSecondsFormat(elapsedTestCaseSeconds() * 1000).constData());

    if (!systemOutputElement->childElements().empty())
        currentTestCase->addChild(systemOutputElement);
    else
        delete systemOutputElement;

    if (!systemErrorElement->childElements().empty())
        currentTestCase->addChild(systemErrorElement);
    else
        delete systemErrorElement;

    systemOutputElement = nullptr;
    systemErrorElement = nullptr;
}

void QJUnitTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    if (type == QAbstractTestLogger::Fail || type == QAbstractTestLogger::XPass) {
        auto failureType = [&]() {
            switch (type) {
            case QAbstractTestLogger::Fail: return "fail";
            case QAbstractTestLogger::XPass: return "xpass";
            default: Q_UNREACHABLE();
            }
        }();

        addFailure(QTest::LET_Failure, failureType, QString::fromUtf8(description));
    } else if (type == QAbstractTestLogger::XFail) {
        // Since XFAIL does not add a failure to the testlog in JUnit XML we add a
        // message, so we still have some information about the expected failure.
        addMessage(QAbstractTestLogger::Info, QString::fromUtf8(description), file, line);
    }
}

void QJUnitTestLogger::addFailure(QTest::LogElementType elementType,
    const char *failureType, const QString &failureDescription)
{
    if (elementType == QTest::LET_Failure) {
        // Make sure we're not adding failure when we already have error,
        // or adding additional failures when we already have a failure.
        for (auto *childElement : currentTestCase->childElements()) {
            if (childElement->elementType() == QTest::LET_Error ||
                childElement->elementType() == QTest::LET_Failure)
                return;
        }
    }

    QTestElement *failureElement = new QTestElement(elementType);
    failureElement->addAttribute(QTest::AI_Type, failureType);

    // Assume the first line is the message, and the remainder are details
    QString message = failureDescription.section(QLatin1Char('\n'), 0, 0);
    QString details = failureDescription.section(QLatin1Char('\n'), 1);

    failureElement->addAttribute(QTest::AI_Message, message.toUtf8().constData());

    if (!details.isEmpty()) {
        auto textNode = new QTestElement(QTest::LET_Text);
        textNode->addAttribute(QTest::AI_Value, details.toUtf8().constData());
        failureElement->addChild(textNode);
    }

    currentTestCase->addChild(failureElement);

    switch (elementType) {
    case QTest::LET_Failure: ++failureCounter; break;
    case QTest::LET_Error: ++errorCounter; break;
    default: Q_UNREACHABLE();
    }
}

void QJUnitTestLogger::addMessage(MessageTypes type, const QString &message, const char *file, int line)
{
    Q_UNUSED(file);
    Q_UNUSED(line);

    if (type == QAbstractTestLogger::Skip) {
        auto skippedElement = new QTestElement(QTest::LET_Skipped);
        skippedElement->addAttribute(QTest::AI_Message, message.toUtf8().constData());
        currentTestCase->addChild(skippedElement);
        return;
    } else if (type == QAbstractTestLogger::QFatal) {
        addFailure(QTest::LET_Error, "qfatal", message);
        return;
    }

    auto systemLogElement = [&]() {
        switch (type) {
        case QAbstractTestLogger::QDebug:
        case QAbstractTestLogger::Info:
        case QAbstractTestLogger::QInfo:
            return systemOutputElement;
        case QAbstractTestLogger::Warn:
        case QAbstractTestLogger::QWarning:
        case QAbstractTestLogger::QCritical:
            return systemErrorElement;
        default:
            Q_UNREACHABLE();
        }
    }();

    if (!systemLogElement)
        return; // FIXME: Handle messages outside of test functions

    auto textNode = new QTestElement(QTest::LET_Text);
    textNode->addAttribute(QTest::AI_Value, message.toUtf8().constData());
    systemLogElement->addChild(textNode);
}

QT_END_NAMESPACE

