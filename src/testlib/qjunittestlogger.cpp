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
    delete currentLogElement;
    delete logFormatter;
}

void QJUnitTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    logFormatter = new QTestJUnitStreamer(this);
    delete errorLogElement;
    errorLogElement = new QTestElement(QTest::LET_SystemError);
}

void QJUnitTestLogger::stopLogging()
{
    QTestElement *iterator = listOfTestcases;

    char buf[10];

    currentLogElement = new QTestElement(QTest::LET_TestSuite);
    currentLogElement->addAttribute(QTest::AI_Name, QTestResult::currentTestObjectName());

    qsnprintf(buf, sizeof(buf), "%i", testCounter);
    currentLogElement->addAttribute(QTest::AI_Tests, buf);

    qsnprintf(buf, sizeof(buf), "%i", failureCounter);
    currentLogElement->addAttribute(QTest::AI_Failures, buf);

    qsnprintf(buf, sizeof(buf), "%i", errorCounter);
    currentLogElement->addAttribute(QTest::AI_Errors, buf);

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

    currentLogElement->addLogElement(properties);

    currentLogElement->addLogElement(iterator);

    /* For correct indenting, make sure every testcase knows its parent */
    QTestElement* testcase = iterator;
    while (testcase) {
        testcase->setParent(currentLogElement);
        testcase = testcase->nextElement();
    }

    currentLogElement->addLogElement(errorLogElement);

    QTestElement *it = currentLogElement;
    logFormatter->output(it);

    QAbstractTestLogger::stopLogging();
}

void QJUnitTestLogger::enterTestFunction(const char *function)
{
    currentLogElement = new QTestElement(QTest::LET_TestCase);
    currentLogElement->addAttribute(QTest::AI_Name, function);
    currentLogElement->addToList(&listOfTestcases);

    ++testCounter;
}

void QJUnitTestLogger::leaveTestFunction()
{
}

void QJUnitTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    const char *typeBuf = nullptr;
    char buf[100];

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
        failureElement->addAttribute(QTest::AI_Result, typeBuf);
        if (file)
            failureElement->addAttribute(QTest::AI_File, file);
        else
            failureElement->addAttribute(QTest::AI_File, "");
        qsnprintf(buf, sizeof(buf), "%i", line);
        failureElement->addAttribute(QTest::AI_Line, buf);
        failureElement->addAttribute(QTest::AI_Description, description);
        addTag(failureElement);
        currentLogElement->addLogElement(failureElement);
    }

    /*
        Only one result can be shown for the whole testfunction.
        Check if we currently have a result, and if so, overwrite it
        iff the new result is worse.
    */
    QTestElementAttribute* resultAttr =
        const_cast<QTestElementAttribute*>(currentLogElement->attribute(QTest::AI_Result));
    if (resultAttr) {
        const char* oldResult = resultAttr->value();
        bool overwrite = false;
        if (!strcmp(oldResult, "pass")) {
            overwrite = true;
        }
        else if (!strcmp(oldResult, "bpass") || !strcmp(oldResult, "bxfail")) {
            overwrite = (type == QAbstractTestLogger::XPass || type == QAbstractTestLogger::Fail) || (type == QAbstractTestLogger::XFail)
                    || (type == QAbstractTestLogger::BlacklistedFail) || (type == QAbstractTestLogger::BlacklistedXPass);
        }
        else if (!strcmp(oldResult, "bfail") || !strcmp(oldResult, "bxpass")) {
            overwrite = (type == QAbstractTestLogger::XPass || type == QAbstractTestLogger::Fail) || (type == QAbstractTestLogger::XFail);
        }
        else if (!strcmp(oldResult, "xfail")) {
            overwrite = (type == QAbstractTestLogger::XPass || type == QAbstractTestLogger::Fail);
        }
        else if (!strcmp(oldResult, "xpass")) {
            overwrite = (type == QAbstractTestLogger::Fail);
        }
        if (overwrite) {
            resultAttr->setPair(QTest::AI_Result, typeBuf);
        }
    }
    else {
        currentLogElement->addAttribute(QTest::AI_Result, typeBuf);
    }

    if (file)
        currentLogElement->addAttribute(QTest::AI_File, file);
    else
        currentLogElement->addAttribute(QTest::AI_File, "");

    qsnprintf(buf, sizeof(buf), "%i", line);
    currentLogElement->addAttribute(QTest::AI_Line, buf);

    /*
        Since XFAIL does not add a failure to the testlog in junitxml, add a message, so we still
        have some information about the expected failure.
    */
    if (type == QAbstractTestLogger::XFail) {
        QJUnitTestLogger::addMessage(QAbstractTestLogger::Info, QString::fromUtf8(description), file, line);
    }
}

void QJUnitTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTestElement *benchmarkElement = new QTestElement(QTest::LET_Benchmark);

    benchmarkElement->addAttribute(
        QTest::AI_Metric,
        QTest::benchmarkMetricName(result.metric));
    benchmarkElement->addAttribute(QTest::AI_Tag, result.context.tag.toUtf8().data());

    const qreal valuePerIteration = qreal(result.value) / qreal(result.iterations);
    benchmarkElement->addAttribute(QTest::AI_Value, QByteArray::number(valuePerIteration).constData());

    char buf[100];
    qsnprintf(buf, sizeof(buf), "%i", result.iterations);
    benchmarkElement->addAttribute(QTest::AI_Iterations, buf);
    currentLogElement->addLogElement(benchmarkElement);
}

void QJUnitTestLogger::addTag(QTestElement* element)
{
    const char *tag = QTestResult::currentDataTag();
    const char *gtag = QTestResult::currentGlobalDataTag();
    const char *filler = (tag && gtag) ? ":" : "";
    if ((!tag || !tag[0]) && (!gtag || !gtag[0])) {
        return;
    }

    if (!tag) {
        tag = "";
    }
    if (!gtag) {
        gtag = "";
    }

    QTestCharBuffer buf;
    QTest::qt_asprintf(&buf, "%s%s%s", gtag, filler, tag);
    element->addAttribute(QTest::AI_Tag, buf.constData());
}

void QJUnitTestLogger::addMessage(MessageTypes type, const QString &message, const char *file, int line)
{
    QTestElement *errorElement = new QTestElement(QTest::LET_Error);
    const char *typeBuf = nullptr;

    switch (type) {
    case QAbstractTestLogger::Warn:
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
        typeBuf = "qwarn";
        break;
    case QAbstractTestLogger::QFatal:
        typeBuf = "qfatal";
        break;
    case QAbstractTestLogger::Skip:
        typeBuf = "skip";
        break;
    case QAbstractTestLogger::Info:
        typeBuf = "info";
        break;
    default:
        typeBuf = "??????";
        break;
    }

    errorElement->addAttribute(QTest::AI_Type, typeBuf);
    errorElement->addAttribute(QTest::AI_Description, message.toUtf8().constData());
    addTag(errorElement);

    if (file)
        errorElement->addAttribute(QTest::AI_File, file);
    else
        errorElement->addAttribute(QTest::AI_File, "");

    char buf[100];
    qsnprintf(buf, sizeof(buf), "%i", line);
    errorElement->addAttribute(QTest::AI_Line, buf);

    currentLogElement->addLogElement(errorElement);
    ++errorCounter;

    // Also add the message to the system error log (i.e. stderr), if one exists
    if (errorLogElement) {
        QTestElement *systemErrorElement = new QTestElement(QTest::LET_Error);
        systemErrorElement->addAttribute(QTest::AI_Description, message.toUtf8().constData());
        errorLogElement->addLogElement(systemErrorElement);
    }
}

QT_END_NAMESPACE

