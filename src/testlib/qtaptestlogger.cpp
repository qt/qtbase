// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtaptestlogger_p.h"

#include "qtestlog_p.h"
#include "qtestresult_p.h"
#include "qtestassert.h"

#if QT_CONFIG(regularexpression)
#  include <QtCore/qregularexpression.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*! \internal
    \class QTapTestLogger
    \inmodule QtTest

    QTapTestLogger implements the Test Anything Protocol v13.

    The \l{Test Anything Protocol} (TAP) is a simple plain-text interface
    between testing code and systems for reporting and analyzing test results.
    Since QtTest doesn't build the table for a data-driven test until the test
    is about to be run, we don't typically know how many tests we'll run until
    we've run them, so we put The Plan at the end, rather than the beginning.
    We summarise the results in comments following The Plan.

    \section1 YAMLish

    The TAP protocol supports inclusion, after a Test Line, of a "diagnostic
    block" in YAML, containing supporting information. We use this to package
    other information from the test, in combination with comments to record
    information we're unable to deliver at the same time as a test line.  By
    convention, TAP producers limit themselves to a restricted subset of YAML,
    known as YAMLish, for maximal compatibility with TAP consumers.

    YAML (see \c yaml.org for details) supports three data-types: mapping (hash
    or dictionary), sequence (list, array or set) and scalar (string or number),
    much as JSON does. It uses indentation to indicate levels of nesting of
    mappings and sequences within one another. A line starting with a dash (at
    the same level of indent as its context) introduces a list entry. A line
    starting with a key (more indented than its context, aside from any earlier
    keys at its level) followed by a colon introduces a mapping entry; the key
    may be either a simple word or a quoted string (within which numeric escapes
    may be used to indicate unicode characters). The value associated with a
    given key, or the entry in a list, can appar after the key-colon or dasy
    either on the same line or on a succession of subsequent lines at higher
    indent. Thus

    \code
    - first list item
    -
      second: list item is a mapping
      with:
      - two keys
      - the second of which is a list
    - back in the outer list, a third item
    \endcode

    In YAMLish, the top-level structure should be a hash. The keys supported for
    this hash, with the meanings for their values, are:

    \list
    \li \c message: free text supplying supporting details
    \li \c severity: how bad is it ?
    \li \c source: source describing the test, as an URL (compare file, line)
    \li \c at: identifies the function (with file and line) performing the test
    \li \c datetime: when the test was run (ISO 8601 or HTTP format)
    \li \c file: source of the test as a local file-name, when appropriate
    \li \c line: line number within the source
    \li \c name: test name
    \li \c extensions: sub-hash in which to store random other stuff
    \li \c actual: what happened (a.k.a. found; contrast expected)
    \li \c expected: what was expected (a.k.a. wanted; contrast actual)
    \li \c display: description of the result, suitable for display
    \li \c dump: a sub-hash of variable values when the result arose
    \li \c error: describes the error
    \li \c backtrace: describes the call-stack of the error
    \endlist

    In practice, only \c at, \c expected and \c actual appear to be generally
    supported.

    We can also have several messages produced within a single test, so the
    simple \c message / \c severity pair of top-level keys does not serve us
    well. We therefore use \c extensions with a sub-tag \c messages in which to
    package a list of messages.

    \sa QAbstractTestLogger
*/

QTapTestLogger::QTapTestLogger(const char *filename)
    : QAbstractTestLogger(filename)
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
        "1..%d\n" // The plan (last non-diagnostic line)
        "# tests %d\n"
        "# pass %d\n"
        "# fail %d\n",
    total, total, QTestLog::passCount(), QTestLog::failCount());
    outputString(testPlanAndStats.data());

    QAbstractTestLogger::stopLogging();
}

void QTapTestLogger::enterTestFunction(const char *function)
{
    m_firstExpectedFail.clear();
    Q_ASSERT(!m_gatherMessages);
    Q_ASSERT(m_comments.isEmpty());
    Q_ASSERT(m_messages.isEmpty());
    m_gatherMessages = function != nullptr;
}

void QTapTestLogger::enterTestData(QTestData *data)
{
    m_firstExpectedFail.clear();
    if (!m_messages.isEmpty() || !m_comments.isEmpty())
        flushMessages();
    m_gatherMessages = data != nullptr;
}

using namespace QTestPrivate;

void QTapTestLogger::outputTestLine(bool ok, int testNumber, const QTestCharBuffer &directive)
{
    QTestCharBuffer testIdentifier;
    QTestPrivate::generateTestIdentifier(&testIdentifier, TestFunction | TestDataTag);

    QTestCharBuffer testLine;
    QTest::qt_asprintf(&testLine, "%s %d - %s%s\n", ok ? "ok" : "not ok",
                       testNumber, testIdentifier.data(), directive.constData());

    outputString(testLine.data());
}

// The indent needs to be two spaces for maximum compatibility.
// This matches the width of the "- " prefix on a list item's first line.
#define YAML_INDENT "  "

void QTapTestLogger::outputBuffer(const QTestCharBuffer &buffer)
{
    auto isComment = [&buffer]() {
        return buffer.constData()[strlen(YAML_INDENT)] == '#';
    };
    if (!m_gatherMessages)
        outputString(buffer.constData());
    else
        QTestPrivate::appendCharBuffer(isComment() ? &m_comments : &m_messages, buffer);
}

void QTapTestLogger::beginYamlish()
{
    outputString(YAML_INDENT "---\n");
}

void QTapTestLogger::endYamlish()
{
    // Flush any accumulated messages:
    if (!m_messages.isEmpty()) {
        outputString(YAML_INDENT "extensions:\n");
        outputString(YAML_INDENT YAML_INDENT "messages:\n");
        outputString(m_messages.constData());
        m_messages.clear();
    }
    outputString(YAML_INDENT "...\n");
}

void QTapTestLogger::flushComments()
{
    if (!m_comments.isEmpty()) {
        outputString(m_comments.constData());
        m_comments.clear();
    }
}

void QTapTestLogger::flushMessages()
{
    /* A _data() function's messages show up here. */
    QTestCharBuffer dataLine;
    QTest::qt_asprintf(&dataLine, "ok %d - %s() # Data prepared\n",
                       QTestLog::totalCount(), QTestResult::currentTestFunction());
    outputString(dataLine.constData());
    flushComments();
    if (!m_messages.isEmpty()) {
        beginYamlish();
        endYamlish();
    }
}

void QTapTestLogger::addIncident(IncidentTypes type, const char *description,
                                 const char *file, int line)
{
    const bool isExpectedFail = type == XFail || type == BlacklistedXFail;
    const bool ok = (m_firstExpectedFail.isEmpty()
                     && (type == Pass || type == BlacklistedPass || type == Skip
                         || type == XPass || type == BlacklistedXPass));

    const char *const incident = [type](const char *priorXFail) {
        switch (type) {
        // We treat expected or blacklisted failures/passes as TODO-failures/passes,
        // which should be treated as soft issues by consumers. Not all do though :/
        case BlacklistedPass:
            if (priorXFail[0] != '\0')
                return priorXFail;
            Q_FALLTHROUGH();
        case XFail: case BlacklistedXFail:
        case XPass: case BlacklistedXPass:
        case BlacklistedFail:
            return "TODO";
        case Skip:
            return "SKIP";
        case Pass:
            if (priorXFail[0] != '\0')
                return priorXFail;
            Q_FALLTHROUGH();
        case Fail:
            break;
        }
        return static_cast<const char *>(nullptr);
    }(m_firstExpectedFail.constData());

    QTestCharBuffer directive;
    if (incident) {
        QTest::qt_asprintf(&directive, "%s%s%s%s",
                           isExpectedFail ? "" : " # ", incident,
                           description && description[0] ? " " : "", description);
    }

    if (!isExpectedFail) {
        m_gatherMessages = false;
        outputTestLine(ok, QTestLog::totalCount(), directive);
    } else if (m_gatherMessages && m_firstExpectedFail.isEmpty()) {
        QTestPrivate::appendCharBuffer(&m_firstExpectedFail, directive);
    }
    flushComments();

    if (!ok || !m_messages.isEmpty()) {
        // All failures need a diagnostics section to not confuse consumers.
        // We also need a diagnostics section when we have messages to report.
        if (isExpectedFail) {
            QTestCharBuffer message;
            if (m_gatherMessages) {
                QTest::qt_asprintf(&message, YAML_INDENT YAML_INDENT "- severity: xfail\n"
                                   YAML_INDENT YAML_INDENT YAML_INDENT "message:%s\n",
                                   directive.constData() + 4);
            } else {
                QTest::qt_asprintf(&message, YAML_INDENT "# xfail:%s\n", directive.constData() + 4);
            }
            outputBuffer(message);
        } else {
            beginYamlish();
        }

        if (!isExpectedFail || m_gatherMessages) {
            const char *indent = isExpectedFail ? YAML_INDENT YAML_INDENT YAML_INDENT : YAML_INDENT;
            if (!ok) {
#if QT_CONFIG(regularexpression)
                enum class OperationType {
                    Unknown,
                    Compare,    /* Plain old QCOMPARE */
                    Verify,     /* QVERIFY */
                    CompareOp,  /* QCOMPARE_OP */
                };

                // This is fragile, but unfortunately testlib doesn't plumb
                // the expected and actual values to the loggers (yet).
                static const QRegularExpression verifyRegex(
                            u"^'(?<actualexpression>.*)' returned "
                            "(?<actual>\\w+)\\. \\((?<message>.*)\\)$"_s);

                static const QRegularExpression compareRegex(
                            u"^(?<message>.*)\n"
                            "\\s*Actual\\s+\\((?<actualexpression>.*)\\)\\s*: (?<actual>.*)\n"
                            "\\s*Expected\\s+\\((?<expectedexpresssion>.*)\\)\\s*: "
                            "(?<expected>.*)$"_s);

                static const QRegularExpression compareOpRegex(
                            u"^(?<message>.*)\n"
                            "\\s*Computed\\s+\\((?<actualexpression>.*)\\)\\s*: (?<actual>.*)\n"
                            "\\s*Baseline\\s+\\((?<expectedexpresssion>.*)\\)\\s*: "
                            "(?<expected>.*)$"_s);

                const QString descriptionString = QString::fromUtf8(description);
                QRegularExpressionMatch match = verifyRegex.match(descriptionString);

                OperationType opType = OperationType::Unknown;
                if (match.hasMatch())
                    opType = OperationType::Verify;

                if (opType == OperationType::Unknown) {
                    match = compareRegex.match(descriptionString);
                    if (match.hasMatch())
                        opType = OperationType::Compare;
                }

                if (opType == OperationType::Unknown) {
                    match = compareOpRegex.match(descriptionString);
                    if (match.hasMatch())
                        opType = OperationType::CompareOp;
                }

                if (opType != OperationType::Unknown) {
                    QString message = match.captured(u"message");
                    QLatin1StringView comparisonType;
                    QString expected;
                    QString actual;
                    const auto parenthesize = [&match](QLatin1StringView key) -> QString {
                        return " ("_L1 % match.captured(key) % u')';
                    };
                    const QString actualExpression = parenthesize("actualexpression"_L1);

                    if (opType == OperationType::Verify) {
                        comparisonType = "QVERIFY"_L1;
                        actual = match.captured(u"actual").toLower() % actualExpression;
                        expected = (actual.startsWith("true "_L1) ? "false"_L1 : "true"_L1)
                                % actualExpression;
                        if (message.isEmpty())
                            message = u"Verification failed"_s;
                    } else if (opType == OperationType::Compare) {
                        comparisonType = "QCOMPARE"_L1;
                        expected = match.captured(u"expected")
                            % parenthesize("expectedexpresssion"_L1);
                        actual = match.captured(u"actual") % actualExpression;
                    } else {
                        struct ComparisonInfo {
                            const char *comparisonType;
                            const char *comparisonStringOp;
                        };
                        // get a proper comparison type based on the error message
                        const auto info = [](const QString &err) -> ComparisonInfo {
                            if (err.contains("different"_L1))
                                return { "QCOMPARE_NE", "!= " };
                            else if (err.contains("less than or equal to"_L1))
                                return { "QCOMPARE_LE", "<= " };
                            else if (err.contains("greater than or equal to"_L1))
                                return { "QCOMPARE_GE", ">= " };
                            else if (err.contains("less than"_L1))
                                return { "QCOMPARE_LT", "< " };
                            else if (err.contains("greater than"_L1))
                                return { "QCOMPARE_GT", "> " };
                            else if (err.contains("to be equal to"_L1))
                                return { "QCOMPARE_EQ", "== " };
                            else
                                return { "Unknown", "" };
                        }(message);
                        comparisonType = QLatin1StringView(info.comparisonType);
                        expected = QLatin1StringView(info.comparisonStringOp)
                                    % match.captured(u"expected")
                                    % parenthesize("expectedexpresssion"_L1);
                        actual = match.captured(u"actual") % actualExpression;
                    }

                    QTestCharBuffer diagnosticsYamlish;
                    QTest::qt_asprintf(&diagnosticsYamlish,
                                       "%stype: %s\n"
                                       "%smessage: %s\n"
                                       // Some consumers understand 'wanted/found', others need
                                       // 'expected/actual', so be compatible with both.
                                       "%swanted: %s\n"
                                       "%sfound: %s\n"
                                       "%sexpected: %s\n"
                                       "%sactual: %s\n",
                                       indent, comparisonType.latin1(),
                                       indent, qPrintable(message),
                                       indent, qPrintable(expected), indent, qPrintable(actual),
                                       indent, qPrintable(expected), indent, qPrintable(actual)
                    );

                    outputBuffer(diagnosticsYamlish);
                } else
#endif
                if (description && !incident) {
                    QTestCharBuffer unparsableDescription;
                    QTest::qt_asprintf(&unparsableDescription, YAML_INDENT "# %s\n", description);
                    outputBuffer(unparsableDescription);
                }
            }

            if (file) {
                QTestCharBuffer location;
                QTest::qt_asprintf(&location,
                                   // The generic 'at' key is understood by most consumers.
                                   "%sat: %s::%s() (%s:%d)\n"

                                   // The file and line keys are for consumers that are able
                                   // to read more granular location info.
                                   "%sfile: %s\n"
                                   "%sline: %d\n",

                                   indent, QTestResult::currentTestObjectName(),
                                   QTestResult::currentTestFunction(),
                                   file, line, indent, file, indent, line
                    );
                outputBuffer(location);
            }
        }

        if (!isExpectedFail)
            endYamlish();
    }
}

void QTapTestLogger::addMessage(MessageTypes type, const QString &message,
                                const char *file, int line)
{
    Q_UNUSED(file);
    Q_UNUSED(line);
    const char *const flavor = [type]() {
        switch (type) {
        case QDebug: return "debug";
        case QInfo: return "info";
        case QWarning: return "warning";
        case QCritical: return "critical";
        case QFatal: return "fatal";
        // Handle internal messages as comments
        case Info: return "# inform";
        case Warn: return "# warn";
        }
        return "unrecognised message";
    }();

    QTestCharBuffer diagnostic;
    if (!m_gatherMessages) {
        QTest::qt_asprintf(&diagnostic, "%s%s: %s\n",
                           flavor[0] == '#' ? "" : "# ",
                           flavor, qPrintable(message));
        outputString(diagnostic.constData());
    } else if (flavor[0] == '#') {
        QTest::qt_asprintf(&diagnostic, YAML_INDENT "%s: %s\n",
                           flavor, qPrintable(message));
        QTestPrivate::appendCharBuffer(&m_comments, diagnostic);
    } else {
        // These shall appear in a messages: sub-block of the extensions: block,
        // so triple-indent.
        QTest::qt_asprintf(&diagnostic, YAML_INDENT YAML_INDENT "- severity: %s\n"
                           YAML_INDENT YAML_INDENT YAML_INDENT "message: %s\n",
                           flavor, qPrintable(message));
        QTestPrivate::appendCharBuffer(&m_messages, diagnostic);
    }
}

QT_END_NAMESPACE
