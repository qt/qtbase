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

#include "QtTest/qtestassert.h"

#include "QtTest/private/qtestlog_p.h"
#include "QtTest/private/qtestresult_p.h"
#include "QtTest/private/qabstracttestlogger_p.h"
#include "QtTest/private/qplaintestlogger_p.h"
#include "QtTest/private/qtestlogger_p.h"
#include "QtTest/private/qxmltestlogger_p.h"
#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>

#include <stdlib.h>
#include <string.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

namespace QTest {

    struct IgnoreResultList
    {
        inline IgnoreResultList(QtMsgType tp, const char *message)
            : type(tp), next(0)
        { msg = qstrdup(message); }
        inline ~IgnoreResultList()
        { delete [] msg; }

        static inline void clearList(IgnoreResultList *&list)
        {
            while (list) {
                IgnoreResultList *current = list;
                list = list->next;
                delete current;
            }
        }

        QtMsgType type;
        char *msg;
        IgnoreResultList *next;
    };

    static IgnoreResultList *ignoreResultList = 0;

    static QTestLog::LogMode logMode = QTestLog::Plain;
    static QTestLog::FlushMode flushMode = QTestLog::NoFlush;
    static int verbosity = 0;
    static int maxWarnings = 2002;

    static QAbstractTestLogger *testLogger = 0;
    static const char *outFile = 0;

    static QtMsgHandler oldMessageHandler;

    static bool handleIgnoredMessage(QtMsgType type, const char *msg)
    {
        IgnoreResultList *last = 0;
        IgnoreResultList *list = ignoreResultList;
        while (list) {
            if (list->type == type && strcmp(msg, list->msg) == 0) {
                // remove the item from the list
                if (last)
                    last->next = list->next;
                else if (list->next)
                    ignoreResultList = list->next;
                else
                    ignoreResultList = 0;

                delete list;
                return true;
            }

            last = list;
            list = list->next;
        }
        return false;
    }

    static void messageHandler(QtMsgType type, const char *msg)
    {
        static QBasicAtomicInt counter = Q_BASIC_ATOMIC_INITIALIZER(QTest::maxWarnings);

        if (!msg || !QTest::testLogger) {
            // if this goes wrong, something is seriously broken.
            qInstallMsgHandler(oldMessageHandler);
            QTEST_ASSERT(msg);
            QTEST_ASSERT(QTest::testLogger);
        }

        if (handleIgnoredMessage(type, msg))
            // the message is expected, so just swallow it.
            return;

        if (type != QtFatalMsg) {
            if (counter <= 0)
                return;

            if (!counter.deref()) {
                QTest::testLogger->addMessage(QAbstractTestLogger::QSystem,
                        "Maximum amount of warnings exceeded. Use -maxwarnings to override.");
                return;
            }
        }

        switch (type) {
        case QtDebugMsg:
            QTest::testLogger->addMessage(QAbstractTestLogger::QDebug, msg);
            break;
        case QtCriticalMsg:
            QTest::testLogger->addMessage(QAbstractTestLogger::QSystem, msg);
            break;
        case QtWarningMsg:
            QTest::testLogger->addMessage(QAbstractTestLogger::QWarning, msg);
            break;
        case QtFatalMsg:
            QTest::testLogger->addMessage(QAbstractTestLogger::QFatal, msg);
            /* Right now, we're inside the custom message handler and we're
             * being qt_message_output in qglobal.cpp. After we return from
             * this function, it will proceed with calling exit() and abort()
             * and hence crash. Therefore, we call these logging functions such
             * that we wrap up nicely, and in particular produce well-formed XML. */
            QTestResult::addFailure("Received a fatal error.", "Unknown file", 0);
            QTestLog::leaveTestFunction();
            QTestLog::stopLogging();
            break;
        }
    }

    void initLogger()
    {
        switch (QTest::logMode) {
        case QTestLog::Plain:
            QTest::testLogger = new QPlainTestLogger;
            break;
        case QTestLog::XML:
            if (QTest::flushMode == QTestLog::FlushOn)
                QTest::testLogger = new QXmlTestLogger(QXmlTestLogger::Complete);
            else
                QTest::testLogger = new QTestLogger(QTestLogger::TLF_XML);
            break;
        case QTestLog::LightXML:
            if (QTest::flushMode == QTestLog::FlushOn)
                QTest::testLogger = new QXmlTestLogger(QXmlTestLogger::Light);
            else
                QTest::testLogger = new QTestLogger(QTestLogger::TLF_LightXml);
            break;
        case QTestLog::XunitXML:
            QTest::testLogger = new QTestLogger(QTestLogger::TLF_XunitXml);
        }
    }

}

void QTestLog::enterTestFunction(const char* function)
{
    QTEST_ASSERT(QTest::testLogger);
    QTEST_ASSERT(function);

    QTest::testLogger->enterTestFunction(function);
}

int QTestLog::unhandledIgnoreMessages()
{
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
    QTEST_ASSERT(QTest::testLogger);

    QTest::IgnoreResultList::clearList(QTest::ignoreResultList);
    QTest::testLogger->leaveTestFunction();
}

void QTestLog::printUnhandledIgnoreMessages()
{
    QTEST_ASSERT(QTest::testLogger);

    char msg[1024];
    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    while (list) {
        QTest::qt_snprintf(msg, 1024, "Did not receive message: \"%s\"", list->msg);
        QTest::testLogger->addMessage(QAbstractTestLogger::Info, msg);

        list = list->next;
    }
}

void QTestLog::addPass(const char *msg)
{
    QTEST_ASSERT(QTest::testLogger);
    QTEST_ASSERT(msg);

    QTest::testLogger->addIncident(QAbstractTestLogger::Pass, msg);
}

void QTestLog::addFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(QTest::testLogger);
    QTEST_ASSERT(msg);

    QTest::testLogger->addIncident(QAbstractTestLogger::Fail, msg, file, line);
}

void QTestLog::addXFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(QTest::testLogger);
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    QTest::testLogger->addIncident(QAbstractTestLogger::XFail, msg, file, line);
}

void QTestLog::addXPass(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(QTest::testLogger);
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    QTest::testLogger->addIncident(QAbstractTestLogger::XPass, msg, file, line);
}

void QTestLog::addSkip(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(QTest::testLogger);
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    QTest::testLogger->addMessage(QAbstractTestLogger::Skip, msg, file, line);
}

void QTestLog::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTEST_ASSERT(QTest::testLogger);
    QTest::testLogger->addBenchmarkResult(result);
}

void QTestLog::startLogging(unsigned int randomSeed)
{
    QTEST_ASSERT(!QTest::testLogger);
    QTest::initLogger();
    QTest::testLogger->registerRandomSeed(randomSeed);
    QTest::testLogger->startLogging(QTest::outFile);
    QTest::oldMessageHandler = qInstallMsgHandler(QTest::messageHandler);
}

void QTestLog::startLogging()
{
    QTEST_ASSERT(!QTest::testLogger);
    QTest::initLogger();
    QTest::testLogger->startLogging(QTest::outFile);
    QTest::oldMessageHandler = qInstallMsgHandler(QTest::messageHandler);
}

void QTestLog::stopLogging()
{
    qInstallMsgHandler(QTest::oldMessageHandler);

    QTEST_ASSERT(QTest::testLogger);
    QTest::testLogger->stopLogging();
    delete QTest::testLogger;
    QTest::testLogger = 0;
}

void QTestLog::warn(const char *msg)
{
    QTEST_ASSERT(QTest::testLogger);
    QTEST_ASSERT(msg);

    QTest::testLogger->addMessage(QAbstractTestLogger::Warn, msg);
}

void QTestLog::info(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::testLogger)
        QTest::testLogger->addMessage(QAbstractTestLogger::Info, msg, file, line);
}

void QTestLog::setLogMode(LogMode mode)
{
    QTest::logMode = mode;
}

QTestLog::LogMode QTestLog::logMode()
{
    return QTest::logMode;
}

void QTestLog::setVerboseLevel(int level)
{
    QTest::verbosity = level;
}

int QTestLog::verboseLevel()
{
    return QTest::verbosity;
}

void QTestLog::addIgnoreMessage(QtMsgType type, const char *msg)
{
    QTEST_ASSERT(msg);

    QTest::IgnoreResultList *item = new QTest::IgnoreResultList(type, msg);

    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    if (!list) {
        QTest::ignoreResultList = item;
        return;
    }
    while (list->next)
        list = list->next;
    list->next = item;
}

void QTestLog::redirectOutput(const char *fileName)
{
    QTEST_ASSERT(fileName);

    QTest::outFile = fileName;
}

const char *QTestLog::outputFileName()
{
    return QTest::outFile;
}

void QTestLog::setMaxWarnings(int m)
{
    QTest::maxWarnings = m <= 0 ? INT_MAX : m + 2;
}

void QTestLog::setFlushMode(FlushMode mode)
{
    QTest::flushMode = mode;
}

QT_END_NAMESPACE
