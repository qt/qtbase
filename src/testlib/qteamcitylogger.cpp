// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2017 Borgar Ovsthus
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestresult_p.h>
#include <QtTest/qtestassert.h>
#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qteamcitylogger_p.h>
#include <QtCore/qbytearray.h>
#include <private/qlocale_p.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QTest {

    static const char *tcIncidentType2String(QAbstractTestLogger::IncidentTypes type)
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

    static const char *tcMessageType2String(QAbstractTestLogger::MessageTypes type)
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

    tcEscapedString(&flowID, QTestResult::currentTestObjectName());

    QTestCharBuffer buf;
    QTest::qt_asprintf(&buf, "##teamcity[testSuiteStarted name='%s' flowId='%s']\n",
                       flowID.constData(), flowID.constData());
    outputString(buf.constData());
}

void QTeamCityLogger::stopLogging()
{
    QTestCharBuffer buf;
    QTest::qt_asprintf(&buf, "##teamcity[testSuiteFinished name='%s' flowId='%s']\n",
                       flowID.constData(), flowID.constData());
    outputString(buf.constData());

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

    QTestCharBuffer buf;
    QTestCharBuffer tmpFuncName;
    escapedTestFuncName(&tmpFuncName);

    if (qstrcmp(tmpFuncName.constData(), currTestFuncName.constData()) != 0) {
        QTest::qt_asprintf(&buf, "##teamcity[testStarted name='%s' flowId='%s']\n",
                           tmpFuncName.constData(), flowID.constData());
        outputString(buf.constData());

        currTestFuncName.clear();
        QTestPrivate::appendCharBuffer(&currTestFuncName, tmpFuncName);
    }

    if (type == QAbstractTestLogger::XFail) {
        addPendingMessage(QTest::tcIncidentType2String(type), description, file, line);
        return;
    }

    QTestCharBuffer detailedText;
    tcEscapedString(&detailedText, description);

    // Test failed
    if (type == Fail || type == XPass) {
        QTestCharBuffer messageText;
        if (file)
            QTest::qt_asprintf(&messageText, "Failure! |[Loc: %s(%d)|]", file, line);
        else
            QTest::qt_asprintf(&messageText, "Failure!");

        QTest::qt_asprintf(&buf, "##teamcity[testFailed name='%s' message='%s' details='%s'"
                           " flowId='%s']\n", tmpFuncName.constData(), messageText.constData(),
                           detailedText.constData(), flowID.constData());

        outputString(buf.constData());
    } else if (type == Skip) {
        if (file) {
            QTestCharBuffer detail;
            QTest::qt_asprintf(&detail, " |[Loc: %s(%d)|]", file, line);
            QTestPrivate::appendCharBuffer(&detailedText, detail);
        }

        QTest::qt_asprintf(&buf, "##teamcity[testIgnored name='%s' message='%s' flowId='%s']\n",
                           currTestFuncName.constData(), detailedText.constData(),
                           flowID.constData());

        outputString(buf.constData());
    }

    if (!pendingMessages.isEmpty()) {
        QTest::qt_asprintf(&buf, "##teamcity[testStdOut name='%s' out='%s' flowId='%s']\n",
                           tmpFuncName.constData(), pendingMessages.constData(),
                           flowID.constData());

        outputString(buf.constData());
        pendingMessages.clear();
    }

    QTest::qt_asprintf(&buf, "##teamcity[testFinished name='%s' flowId='%s']\n",
                       tmpFuncName.constData(), flowID.constData());
    outputString(buf.constData());
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

    QTestCharBuffer escapedMessage;
    tcEscapedString(&escapedMessage, qUtf8Printable(message));
    addPendingMessage(QTest::tcMessageType2String(type), escapedMessage.constData(), file, line);
}

void QTeamCityLogger::tcEscapedString(QTestCharBuffer *buf, const char *str) const
{
    {
        size_t size = qstrlen(str) + 1;
        for (const char *p = str; *p; ++p) {
            if (strchr("\n\r|[]'", *p))
                ++size;
        }
        Q_ASSERT(size <= size_t(INT_MAX));
        buf->resize(int(size));
    }

    bool swallowSpace = true;
    char *p = buf->data();
    for (; *str; ++str) {
        char ch = *str;
        switch (ch) {
        case '\n':
            p++[0] = '|';
            ch = 'n';
            swallowSpace = false;
            break;
        case '\r':
            p++[0] = '|';
            ch = 'r';
            swallowSpace = false;
            break;
        case '|':
        case '[':
        case ']':
        case '\'':
            p++[0] = '|';
            swallowSpace = false;
            break;
        default:
            if (ascii_isspace(ch)) {
                if (swallowSpace)
                    continue;
                swallowSpace = true;
                ch = ' ';
            } else {
                swallowSpace = false;
            }
            break;
        }
        p++[0] = ch;
    }
    Q_ASSERT(p < buf->data() + buf->size());
    if (swallowSpace && p > buf->data()) {
        Q_ASSERT(p[-1] == ' ');
        --p;
    }
    Q_ASSERT(p == buf->data() || !ascii_isspace(p[-1]));
    *p = '\0';
}

void QTeamCityLogger::escapedTestFuncName(QTestCharBuffer *buf) const
{
    constexpr int TestTag = QTestPrivate::TestFunction | QTestPrivate::TestDataTag;
    QTestPrivate::generateTestIdentifier(buf, TestTag);
}

void QTeamCityLogger::addPendingMessage(const char *type, const char *msg,
                                        const char *file, int line)
{
    const char *pad = pendingMessages.isEmpty() ? "" : "|n";

    QTestCharBuffer newMessage;
    if (file)
        QTest::qt_asprintf(&newMessage, "%s%s |[Loc: %s(%d)|]: %s", pad, type, file, line, msg);
    else
        QTest::qt_asprintf(&newMessage, "%s%s: %s", pad, type, msg);

    QTestPrivate::appendCharBuffer(&pendingMessages, newMessage);
}

QT_END_NAMESPACE
