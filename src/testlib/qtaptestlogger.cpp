/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qtaptestlogger_p.h"

#include "qtestlog_p.h"
#include "qtestresult_p.h"
#include "qtestassert.h"

#if QT_CONFIG(regularexpression)
#  include <QtCore/qregularexpression.h>
#endif

QT_BEGIN_NAMESPACE

QTapTestLogger::QTapTestLogger(const char *filename)
    : QAbstractTestLogger(filename)
    , m_wasExpectedFail(false)
{
}

QTapTestLogger::~QTapTestLogger() = default;

void QTapTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    QTestCharBuffer preamble;
    QTest::qt_asprintf(&preamble, "TAP version 13\n"
        // By convention, test suite names are output as diagnostics lines
        // This is a pretty poor convention, as consumers will then treat
        // actual diagnostics, e.g. qDebug, as test suite names o_O
        "# %s\n", QTestResult::currentTestObjectName());
    outputString(preamble.data());
}

void QTapTestLogger::stopLogging()
{
    const int total = QTestLog::totalCount();

    QTestCharBuffer testPlanAndStats;
    QTest::qt_asprintf(&testPlanAndStats,
        "1..%d\n"
        "# tests %d\n"
        "# pass %d\n"
        "# fail %d\n",
    total, total, QTestLog::passCount(), QTestLog::failCount());
    outputString(testPlanAndStats.data());

    QAbstractTestLogger::stopLogging();
}

void QTapTestLogger::enterTestFunction(const char *function)
{
    Q_UNUSED(function);
    m_wasExpectedFail = false;
}

void QTapTestLogger::enterTestData(QTestData *data)
{
    Q_UNUSED(data);
    m_wasExpectedFail = false;
}

using namespace QTestPrivate;

void QTapTestLogger::outputTestLine(bool ok, int testNumber, QTestCharBuffer &directive)
{
    QTestCharBuffer testIdentifier;
    QTestPrivate::generateTestIdentifier(&testIdentifier, TestFunction | TestDataTag);

    QTestCharBuffer testLine;
    QTest::qt_asprintf(&testLine, "%s %d - %s%s\n",
        ok ? "ok" : "not ok", testNumber, testIdentifier.data(), directive.data());

    outputString(testLine.data());
}

void QTapTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    if (m_wasExpectedFail && type == Pass) {
        // XFail comes with a corresponding Pass incident, but we only want
        // to emit a single test point for it, so skip the this pass.
        return;
    }

    bool ok = type == Pass || type == XPass || type == BlacklistedPass || type == BlacklistedXPass;

    QTestCharBuffer directive;
    if (type == XFail || type == XPass || type == BlacklistedFail || type == BlacklistedPass
            || type == BlacklistedXFail || type == BlacklistedXPass) {
        // We treat expected or blacklisted failures/passes as TODO-failures/passes,
        // which should be treated as soft issues by consumers. Not all do though :/
        QTest::qt_asprintf(&directive, " # TODO %s", description);
    }

    int testNumber = QTestLog::totalCount();
    if (type == XFail) {
        // The global test counter hasn't been updated yet for XFail
        testNumber += 1;
    }

    outputTestLine(ok, testNumber, directive);

    if (!ok) {
        // All failures need a diagnostics sections to not confuse consumers

        // The indent needs to be two spaces for maximum compatibility
        #define YAML_INDENT "  "

        outputString(YAML_INDENT "---\n");

        if (type != XFail) {
#if QT_CONFIG(regularexpression)
            // This is fragile, but unfortunately testlib doesn't plumb
            // the expected and actual values to the loggers (yet).
            static QRegularExpression verifyRegex(
                QLatin1String("^'(?<actualexpression>.*)' returned (?<actual>\\w+).+\\((?<message>.*)\\)$"));

            static QRegularExpression comparRegex(
                QLatin1String("^(?<message>.*)\n"
                    "\\s*Actual\\s+\\((?<actualexpression>.*)\\)\\s*: (?<actual>.*)\n"
                    "\\s*Expected\\s+\\((?<expectedexpresssion>.*)\\)\\s*: (?<expected>.*)$"));

            QString descriptionString = QString::fromUtf8(description);
            QRegularExpressionMatch match = verifyRegex.match(descriptionString);
            if (!match.hasMatch())
                match = comparRegex.match(descriptionString);

            if (match.hasMatch()) {
                bool isVerify = match.regularExpression() == verifyRegex;
                QString message = match.captured(QLatin1String("message"));
                QString expected;
                QString actual;

                if (isVerify) {
                    QString expression = QLatin1String(" (")
                        % match.captured(QLatin1String("actualexpression")) % QLatin1Char(')') ;
                    actual = match.captured(QLatin1String("actual")).toLower() % expression;
                    expected = (actual.startsWith(QLatin1String("true")) ? QLatin1String("false") : QLatin1String("true")) % expression;
                    if (message.isEmpty())
                        message = QLatin1String("Verification failed");
                } else {
                    expected = match.captured(QLatin1String("expected"))
                        % QLatin1String(" (") % match.captured(QLatin1String("expectedexpresssion")) % QLatin1Char(')');
                    actual = match.captured(QLatin1String("actual"))
                        % QLatin1String(" (") % match.captured(QLatin1String("actualexpression")) % QLatin1Char(')');
                }

                QTestCharBuffer diagnosticsYamlish;
                QTest::qt_asprintf(&diagnosticsYamlish,
                    YAML_INDENT "type: %s\n"
                    YAML_INDENT "message: %s\n"

                    // Some consumers understand 'wanted/found', while others need
                    // 'expected/actual', so we do both for maximum compatibility.
                    YAML_INDENT "wanted: %s\n"
                    YAML_INDENT "found: %s\n"
                    YAML_INDENT "expected: %s\n"
                    YAML_INDENT "actual: %s\n",

                    isVerify ? "QVERIFY" : "QCOMPARE",
                    qPrintable(message),
                    qPrintable(expected), qPrintable(actual),
                    qPrintable(expected), qPrintable(actual)
                );

                outputString(diagnosticsYamlish.data());
            } else {
                QTestCharBuffer unparsableDescription;
                QTest::qt_asprintf(&unparsableDescription,
                    YAML_INDENT "# %s\n", description);
                outputString(unparsableDescription.data());
            }
#else
            QTestCharBuffer unparsableDescription;
            QTest::qt_asprintf(&unparsableDescription,
                YAML_INDENT "# %s\n", description);
            outputString(unparsableDescription.data());
#endif
        }

        if (file) {
            QTestCharBuffer location;
            QTest::qt_asprintf(&location,
                // The generic 'at' key is understood by most consumers.
                YAML_INDENT "at: %s::%s() (%s:%d)\n"

                // The file and line keys are for consumers that are able
                // to read more granular location info.
                YAML_INDENT "file: %s\n"
                YAML_INDENT "line: %d\n",

                QTestResult::currentTestObjectName(),
                QTestResult::currentTestFunction(),
                file, line, file, line
            );
            outputString(location.data());
        }

        outputString(YAML_INDENT "...\n");
    }

    m_wasExpectedFail = type == XFail;
}

void QTapTestLogger::addMessage(MessageTypes type, const QString &message,
                    const char *file, int line)
{
    Q_UNUSED(file);
    Q_UNUSED(line);

    if (type == Skip) {
        QTestCharBuffer directive;
        QTest::qt_asprintf(&directive, " # SKIP %s", message.toUtf8().constData());
        outputTestLine(/* ok  = */ true, QTestLog::totalCount(), directive);
        return;
    }

    QTestCharBuffer diagnostics;
    QTest::qt_asprintf(&diagnostics, "# %s\n", qPrintable(message));
    outputString(diagnostics.data());
}

QT_END_NAMESPACE

