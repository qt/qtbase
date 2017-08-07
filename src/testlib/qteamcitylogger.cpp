/****************************************************************************
**
** Copyright (C) 2016 Borgar Ovsthus
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
#include <QtTest/qtestassert.h>
#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qteamcitylogger_p.h>
#include <QtCore/qbytearray.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

QT_BEGIN_NAMESPACE

namespace QTest {

    static const char *incidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        switch (type) {
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
        }
        return "??????";
    }

    static const char *messageType2String(QAbstractTestLogger::MessageTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Skip:
            return "SKIP";
        case QAbstractTestLogger::Warn:
            return "WARNING";
        case QAbstractTestLogger::QWarning:
            return "QWARN";
        case QAbstractTestLogger::QDebug:
            return "QDEBUG";
        case QAbstractTestLogger::QInfo:
            return "QINFO";
        case QAbstractTestLogger::QSystem:
            return "QSYSTEM";
        case QAbstractTestLogger::QFatal:
            return "QFATAL";
        case QAbstractTestLogger::Info:
            return "INFO";
        }
        return "??????";
    }
}

QTeamCityLogger::QTeamCityLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QTeamCityLogger::~QTeamCityLogger()
{
}

void QTeamCityLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    flowID = tcEscapedString(QString::fromUtf8(QTestResult::currentTestObjectName()));

    QString str = QString(QLatin1String("##teamcity[testSuiteStarted name='%1' flowId='%1']\n")).arg(flowID);
    outputString(qPrintable(str));
}

void QTeamCityLogger::stopLogging()
{
    QString str = QString(QLatin1String("##teamcity[testSuiteFinished name='%1' flowId='%1']\n")).arg(flowID);
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
    // suppress PASS and XFAIL in silent mode
    if ((type == QAbstractTestLogger::Pass || type == QAbstractTestLogger::XFail) && QTestLog::verboseLevel() < 0)
        return;

    QString buf;

    QString tmpFuncName = escapedTestFuncName();

    if (tmpFuncName != currTestFuncName) {
        buf = QString(QLatin1String("##teamcity[testStarted name='%1' flowId='%2']\n")).arg(tmpFuncName, flowID);
        outputString(qPrintable(buf));
    }

    currTestFuncName = tmpFuncName;

    if (type == QAbstractTestLogger::XFail) {
        addPendingMessage(QTest::incidentType2String(type), QString::fromUtf8(description), file, line);
        return;
    }

    QString detailedText = QString::fromUtf8(description);
    detailedText = tcEscapedString(detailedText);

    // Test failed
    if ((type == QAbstractTestLogger::Fail) || (type == QAbstractTestLogger::XPass)) {
        QString messageText(QLatin1String("Failure!"));

        if (file)
            messageText += QString(QLatin1String(" |[Loc: %1(%2)|]")).arg(QString::fromUtf8(file)).arg(line);

        buf = QString(QLatin1String("##teamcity[testFailed name='%1' message='%2' details='%3' flowId='%4']\n"))
                        .arg(tmpFuncName,
                             messageText,
                             detailedText,
                             flowID);

        outputString(qPrintable(buf));
    }

    if (!pendingMessages.isEmpty()) {
        buf = QString(QLatin1String("##teamcity[testStdOut name='%1' out='%2' flowId='%3']\n"))
                .arg(tmpFuncName, pendingMessages, flowID);

        outputString(qPrintable(buf));

        pendingMessages.clear();
    }

    buf = QString(QLatin1String("##teamcity[testFinished name='%1' flowId='%2']\n")).arg(tmpFuncName, flowID);
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
    if (type != QAbstractTestLogger::QFatal && QTestLog::verboseLevel() < 0)
        return;

    QString escapedMessage = tcEscapedString(message);

    QString buf;

    if (type == QAbstractTestLogger::Skip) {
        if (file)
            escapedMessage.append(QString(QLatin1String(" |[Loc: %1(%2)|]")).arg(QString::fromUtf8(file)).arg(line));

        buf = QString(QLatin1String("##teamcity[testIgnored name='%1' message='%2' flowId='%3']\n"))
                .arg(escapedTestFuncName(), escapedMessage, flowID);

        outputString(qPrintable(buf));
    }
    else {
        addPendingMessage(QTest::messageType2String(type), escapedMessage, file, line);
    }
}

QString QTeamCityLogger::tcEscapedString(const QString &str) const
{
    QString formattedString;

    for (int i = 0; i < str.length(); i++) {
        QChar ch = str.at(i);

        switch (ch.toLatin1()) {
        case '\n':
            formattedString.append(QLatin1String("|n"));
            break;
        case '\r':
            formattedString.append(QLatin1String("|r"));
            break;
        case '|':
            formattedString.append(QLatin1String("||"));
            break;
        case '[':
            formattedString.append(QLatin1String("|["));
            break;
        case ']':
            formattedString.append(QLatin1String("|]"));
            break;
        case '\'':
            formattedString.append(QLatin1String("|'"));
            break;
        default:
            formattedString.append(ch);
        }
    }

    return qMove(formattedString).simplified();
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
        pendMessage += QLatin1String("|n");

    if (file) {
        pendMessage += QString(QLatin1String("%1 |[Loc: %2(%3)|]: %4"))
                                .arg(QString::fromUtf8(type), QString::fromUtf8(file))
                                .arg(line)
                                .arg(msg);

    }
    else {
        pendMessage += QString(QLatin1String("%1: %2"))
                                .arg(QString::fromUtf8(type), msg);
    }

    pendingMessages.append(pendMessage);
}

QT_END_NAMESPACE
