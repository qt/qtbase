// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2017 Borgar Ovsthus
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestresult_p.h>
#include <QtTest/qtestassert.h>
#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qteamcitylogger_p.h>
#include <QtCore/qbytearray.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QTest {

    static const char *incidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Skip:
            return "SKIP";
        case QAbstractTestLogger::Pass:
            return "PASS";
        case QAbstractTestLogger::XFail:
            return "XFAIL";
        case QAbstractTestLogger::Fail:
            return "FAIL!";
        case QAbstractTestLogger::XPass:
            return "XPASS";
        case QAbstractTestLogger::BlacklistedPass:
            return "BPASS";
        case QAbstractTestLogger::BlacklistedFail:
            return "BFAIL";
        case QAbstractTestLogger::BlacklistedXPass:
            return "BXPASS";
        case QAbstractTestLogger::BlacklistedXFail:
            return "BXFAIL";
        }
        return "??????";
    }

    static const char *messageType2String(QAbstractTestLogger::MessageTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::QDebug:
            return "QDEBUG";
        case QAbstractTestLogger::QInfo:
            return "QINFO";
        case QAbstractTestLogger::QWarning:
            return "QWARN";
        case QAbstractTestLogger::QCritical:
            return "QCRITICAL";
        case QAbstractTestLogger::QFatal:
            return "QFATAL";
        case QAbstractTestLogger::Info:
            return "INFO";
        case QAbstractTestLogger::Warn:
            return "WARNING";
        }
        return "??????";
    }
}

/*! \internal
    \class QTeamCityLogger
    \inmodule QtTest

    QTeamCityLogger implements logging in the \l{TeamCity} format.
*/

QTeamCityLogger::QTeamCityLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QTeamCityLogger::~QTeamCityLogger() = default;

void QTeamCityLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    flowID = tcEscapedString(QString::fromUtf8(QTestResult::currentTestObjectName()));

    QString str = "##teamcity[testSuiteStarted name='%1' flowId='%1']\n"_L1.arg(flowID);
    outputString(qPrintable(str));
}

void QTeamCityLogger::stopLogging()
{
    QString str = "##teamcity[testSuiteFinished name='%1' flowId='%1']\n"_L1.arg(flowID);
    outputString(qPrintable(str));

    QAbstractTestLogger::stopLogging();
}

void QTeamCityLogger::enterTestFunction(const char * /*function*/)
{
    // don't print anything
}

void QTeamCityLogger::leaveTestFunction()
{
    // don't print anything
}

void QTeamCityLogger::addIncident(IncidentTypes type, const char *description,
                                  const char *file, int line)
{
    // suppress B?PASS and B?XFAIL in silent mode
    if ((type == Pass || type == XFail || type == BlacklistedPass || type == BlacklistedXFail) && QTestLog::verboseLevel() < 0)
        return;

    QString buf;

    QString tmpFuncName = escapedTestFuncName();

    if (tmpFuncName != currTestFuncName) {
        buf = "##teamcity[testStarted name='%1' flowId='%2']\n"_L1.arg(tmpFuncName, flowID);
        outputString(qPrintable(buf));
    }

    currTestFuncName = tmpFuncName;

    if (type == QAbstractTestLogger::XFail) {
        addPendingMessage(QTest::incidentType2String(type), QString::fromUtf8(description), file, line);
        return;
    }

    QString detailedText = tcEscapedString(QString::fromUtf8(description));

    // Test failed
    if (type == Fail || type == XPass) {
        QString messageText(u"Failure!"_s);

        if (file)
            messageText += " |[Loc: %1(%2)|]"_L1.arg(QString::fromUtf8(file)).arg(line);

        buf = "##teamcity[testFailed name='%1' message='%2' details='%3' flowId='%4']\n"_L1
                        .arg(tmpFuncName, messageText, detailedText, flowID);

        outputString(qPrintable(buf));
    } else if (type == Skip) {
        if (file)
            detailedText.append(" |[Loc: %1(%2)|]"_L1.arg(QString::fromUtf8(file)).arg(line));

        buf = "##teamcity[testIgnored name='%1' message='%2' flowId='%3']\n"_L1
                .arg(escapedTestFuncName(), detailedText, flowID);

        outputString(qPrintable(buf));
    }

    if (!pendingMessages.isEmpty()) {
        buf = "##teamcity[testStdOut name='%1' out='%2' flowId='%3']\n"_L1
                .arg(tmpFuncName, pendingMessages, flowID);

        outputString(qPrintable(buf));

        pendingMessages.clear();
    }

    buf = "##teamcity[testFinished name='%1' flowId='%2']\n"_L1.arg(tmpFuncName, flowID);
    outputString(qPrintable(buf));
}

void QTeamCityLogger::addBenchmarkResult(const QBenchmarkResult &)
{
    // don't print anything
}

void QTeamCityLogger::addMessage(MessageTypes type, const QString &message,
                                 const char *file, int line)
{
    // suppress non-fatal messages in silent mode
    if (type != QFatal && QTestLog::verboseLevel() < 0)
        return;

    QString escapedMessage = tcEscapedString(message);
    addPendingMessage(QTest::messageType2String(type), escapedMessage, file, line);
}

QString QTeamCityLogger::tcEscapedString(const QString &str) const
{
    QString formattedString;

    for (QChar ch : str) {
        switch (ch.toLatin1()) {
        case '\n':
            formattedString.append("|n"_L1);
            break;
        case '\r':
            formattedString.append("|r"_L1);
            break;
        case '|':
            formattedString.append("||"_L1);
            break;
        case '[':
            formattedString.append("|["_L1);
            break;
        case ']':
            formattedString.append("|]"_L1);
            break;
        case '\'':
            formattedString.append("|'"_L1);
            break;
        default:
            formattedString.append(ch);
        }
    }

    return std::move(formattedString).simplified();
}

QString QTeamCityLogger::escapedTestFuncName() const
{
    const char *fn = QTestResult::currentTestFunction() ? QTestResult::currentTestFunction()
                                                        : "UnknownTestFunc";
    const char *tag = QTestResult::currentDataTag() ? QTestResult::currentDataTag() : "";

    return tcEscapedString(QString::asprintf("%s(%s)", fn, tag));
}

void QTeamCityLogger::addPendingMessage(const char *type, const QString &msg, const char *file, int line)
{
    QString pendMessage;

    if (!pendingMessages.isEmpty())
        pendMessage += "|n"_L1;

    if (file) {
        pendMessage += "%1 |[Loc: %2(%3)|]: %4"_L1
                .arg(QString::fromUtf8(type), QString::fromUtf8(file),
                     QString::number(line), msg);

    } else {
        pendMessage += "%1: %2"_L1.arg(QString::fromUtf8(type), msg);
    }

    pendingMessages.append(pendMessage);
}

QT_END_NAMESPACE
