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

#include "qtestlogger_p.h"
#include "qtestelement.h"
#include "qtestxunitstreamer.h"
#include "qtestxmlstreamer.h"
#include "qtestlightxmlstreamer.h"
#include "qtestfilelogger.h"

#include "QtTest/qtestcase.h"
#include "QtTest/private/qtestresult_p.h"
#include "QtTest/private/qbenchmark_p.h"

#include <string.h>

QT_BEGIN_NAMESPACE

QTestLogger::QTestLogger(int fm)
    :listOfTestcases(0), currentLogElement(0), errorLogElement(0),
    logFormatter(0), format( (TestLoggerFormat)fm ), filelogger(new QTestFileLogger),
    testCounter(0), passCounter(0),
    failureCounter(0), errorCounter(0),
    warningCounter(0), skipCounter(0),
    systemCounter(0), qdebugCounter(0),
    qwarnCounter(0), qfatalCounter(0),
    infoCounter(0), randomSeed_(0),
    hasRandomSeed_(false)
{
}

QTestLogger::~QTestLogger()
{
    if(format == TLF_XunitXml)
        delete currentLogElement;
    else
        delete listOfTestcases;

    delete logFormatter;
    delete filelogger;
}

void QTestLogger::startLogging()
{
    switch(format){
    case TLF_LightXml:{
        logFormatter = new QTestLightXmlStreamer;
        filelogger->init();
        break;
    }case TLF_XML:{
        logFormatter = new QTestXmlStreamer;
        filelogger->init();
        break;
    }case TLF_XunitXml:{
        logFormatter = new QTestXunitStreamer;
        delete errorLogElement;
        errorLogElement = new QTestElement(QTest::LET_SystemError);
        filelogger->init();
        break;
    }
    }

    logFormatter->setLogger(this);
    logFormatter->startStreaming();
}

void QTestLogger::stopLogging()
{
    QTestElement *iterator = listOfTestcases;

    if(format == TLF_XunitXml ){
        char buf[10];

        currentLogElement = new QTestElement(QTest::LET_TestSuite);
        currentLogElement->addAttribute(QTest::AI_Name, QTestResult::currentTestObjectName());

        QTest::qt_snprintf(buf, sizeof(buf), "%i", testCounter);
        currentLogElement->addAttribute(QTest::AI_Tests, buf);

        QTest::qt_snprintf(buf, sizeof(buf), "%i", failureCounter);
        currentLogElement->addAttribute(QTest::AI_Failures, buf);

        QTest::qt_snprintf(buf, sizeof(buf), "%i", errorCounter);
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

        if (hasRandomSeed()) {
            property = new QTestElement(QTest::LET_Property);
            property->addAttribute(QTest::AI_Name, "RandomSeed");
            QTest::qt_snprintf(buf, sizeof(buf), "%i", randomSeed());
            property->addAttribute(QTest::AI_PropertyValue, buf);
            properties->addLogElement(property);
        }

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
    }else{
        logFormatter->output(iterator);
    }

    logFormatter->stopStreaming();
}

void QTestLogger::enterTestFunction(const char *function)
{
    char buf[1024];
    QTest::qt_snprintf(buf, sizeof(buf), "Entered test-function: %s\n", function);
    filelogger->flush(buf);

    currentLogElement = new QTestElement(QTest::LET_TestCase);
    currentLogElement->addAttribute(QTest::AI_Name, function);
    currentLogElement->addToList(&listOfTestcases);

    ++testCounter;
}

void QTestLogger::leaveTestFunction()
{
}

void QTestLogger::addIncident(IncidentTypes type, const char *description,
                     const char *file, int line)
{
    const char *typeBuf = 0;
    char buf[100];

    switch (type) {
    case QAbstractTestLogger::XPass:
        ++failureCounter;
        typeBuf = "xpass";
        break;
    case QAbstractTestLogger::Pass:
        ++passCounter;
        typeBuf = "pass";
        break;
    case QAbstractTestLogger::XFail:
        ++passCounter;
        typeBuf = "xfail";
        break;
    case QAbstractTestLogger::Fail:
        ++failureCounter;
        typeBuf = "fail";
        break;
    default:
        typeBuf = "??????";
        break;
    }

    if (type == QAbstractTestLogger::Fail || type == QAbstractTestLogger::XPass
            || ((format != TLF_XunitXml) && (type == QAbstractTestLogger::XFail))) {
        QTestElement *failureElement = new QTestElement(QTest::LET_Failure);
        failureElement->addAttribute(QTest::AI_Result, typeBuf);
        if(file)
            failureElement->addAttribute(QTest::AI_File, file);
        else
            failureElement->addAttribute(QTest::AI_File, "");
        QTest::qt_snprintf(buf, sizeof(buf), "%i", line);
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

    if(file)
        currentLogElement->addAttribute(QTest::AI_File, file);
    else
        currentLogElement->addAttribute(QTest::AI_File, "");

    QTest::qt_snprintf(buf, sizeof(buf), "%i", line);
    currentLogElement->addAttribute(QTest::AI_Line, buf);

    /*
        Since XFAIL does not add a failure to the testlog in xunitxml, add a message, so we still
        have some information about the expected failure.
    */
    if (format == TLF_XunitXml && type == QAbstractTestLogger::XFail) {
        QTestLogger::addMessage(QAbstractTestLogger::Info, description, file, line);
    }
}

void QTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTestElement *benchmarkElement = new QTestElement(QTest::LET_Benchmark);
//    printf("element %i", benchmarkElement->elementType());

    benchmarkElement->addAttribute(
        QTest::AI_Metric,
        QTest::benchmarkMetricName(QBenchmarkTestMethodData::current->result.metric));
    benchmarkElement->addAttribute(QTest::AI_Tag, result.context.tag.toAscii().data());
    benchmarkElement->addAttribute(QTest::AI_Value, QByteArray::number(result.value).constData());

    char buf[100];
    QTest::qt_snprintf(buf, sizeof(buf), "%i", result.iterations);
    benchmarkElement->addAttribute(QTest::AI_Iterations, buf);
    currentLogElement->addLogElement(benchmarkElement);
}

void QTestLogger::addTag(QTestElement* element)
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

void QTestLogger::addMessage(MessageTypes type, const char *message, const char *file, int line)
{
    QTestElement *errorElement = new QTestElement(QTest::LET_Error);
    const char *typeBuf = 0;

    switch (type) {
    case QAbstractTestLogger::Warn:
        ++warningCounter;
        typeBuf = "warn";
        break;
    case QAbstractTestLogger::QSystem:
        ++systemCounter;
        typeBuf = "system";
        break;
    case QAbstractTestLogger::QDebug:
        ++qdebugCounter;
        typeBuf = "qdebug";
        break;
    case QAbstractTestLogger::QWarning:
        ++qwarnCounter;
        typeBuf = "qwarn";
        break;
    case QAbstractTestLogger::QFatal:
        ++qfatalCounter;
        typeBuf = "qfatal";
        break;
    case QAbstractTestLogger::Skip:
        ++skipCounter;
        typeBuf = "skip";
        break;
    case QAbstractTestLogger::Info:
        ++infoCounter;
        typeBuf = "info";
        break;
    default:
        typeBuf = "??????";
        break;
    }

    errorElement->addAttribute(QTest::AI_Type, typeBuf);
    errorElement->addAttribute(QTest::AI_Description, message);
    addTag(errorElement);

    if(file)
        errorElement->addAttribute(QTest::AI_File, file);
    else
        errorElement->addAttribute(QTest::AI_File, "");

    char buf[100];
    QTest::qt_snprintf(buf, sizeof(buf), "%i", line);
    errorElement->addAttribute(QTest::AI_Line, buf);

    currentLogElement->addLogElement(errorElement);
    ++errorCounter;

    // Also add the message to the system error log (i.e. stderr), if one exists
    if (errorLogElement) {
        QTestElement *systemErrorElement = new QTestElement(QTest::LET_Error);
        systemErrorElement->addAttribute(QTest::AI_Description, message);
        errorLogElement->addLogElement(systemErrorElement);
    }
}

void QTestLogger::setLogFormat(TestLoggerFormat fm)
{
    format = fm;
}

QTestLogger::TestLoggerFormat QTestLogger::logFormat()
{
    return format;
}

int QTestLogger::passCount() const
{
    return passCounter;
}

int QTestLogger::failureCount() const
{
    return failureCounter;
}

int QTestLogger::errorCount() const
{
    return errorCounter;
}

int QTestLogger::warningCount() const
{
    return warningCounter;
}

int QTestLogger::skipCount() const
{
    return skipCounter;
}

int QTestLogger::systemCount() const
{
    return systemCounter;
}

int QTestLogger::qdebugCount() const
{
    return qdebugCounter;
}

int QTestLogger::qwarnCount() const
{
    return qwarnCounter;
}

int QTestLogger::qfatalCount() const
{
    return qfatalCounter;
}

int QTestLogger::infoCount() const
{
    return infoCounter;
}

void QTestLogger::registerRandomSeed(unsigned int seed)
{
    randomSeed_ = seed;
    hasRandomSeed_ = true;
}

unsigned int QTestLogger::randomSeed() const
{
    return randomSeed_;
}

bool QTestLogger::hasRandomSeed() const
{
    return hasRandomSeed_;
}

QT_END_NAMESPACE

