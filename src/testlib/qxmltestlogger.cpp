// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <stdio.h>
#include <string.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qxmltestlogger_p.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>
#include <QtTest/qtestcase.h>

QT_BEGIN_NAMESPACE

namespace QTest {

    static const char *xmlMessageType2String(QAbstractTestLogger::MessageTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::QDebug:
            return "qdebug";
        case QAbstractTestLogger::QInfo:
            return "qinfo";
        case QAbstractTestLogger::QWarning:
            return "qwarn";
        case QAbstractTestLogger::QCritical:
            return "qcritical";
        case QAbstractTestLogger::QFatal:
            return "qfatal";
        case QAbstractTestLogger::Info:
            return "info";
        case QAbstractTestLogger::Warn:
            return "warn";
        }
        return "??????";
    }

    static const char *xmlIncidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Skip:
            return "skip";
        case QAbstractTestLogger::Pass:
            return "pass";
        case QAbstractTestLogger::XFail:
            return "xfail";
        case QAbstractTestLogger::Fail:
            return "fail";
        case QAbstractTestLogger::XPass:
            return "xpass";
        case QAbstractTestLogger::BlacklistedPass:
            return "bpass";
        case QAbstractTestLogger::BlacklistedFail:
            return "bfail";
        case QAbstractTestLogger::BlacklistedXPass:
            return "bxpass";
        case QAbstractTestLogger::BlacklistedXFail:
            return "bxfail";
        }
        return "??????";
    }

}

/*! \internal
    \class QXmlTestLogger
    \inmodule QtTest

    QXmlTestLogger implements two XML formats specific to Qt.

    The two formats are distinguished by the XmlMode enum.
*/
/*! \internal
    \enum QXmlTestLogger::XmlMode

    This enumerated type selects the type of XML output to produce.

    \value Complete A full self-contained XML document
    \value Light XML content suitable for embedding in an XML document

    The Complete form wraps the Light form in a <TestCase> element whose name
    attribute identifies the test class whose private slots are to be run. It
    also includes the usual <?xml ...> preamble.
*/

QXmlTestLogger::QXmlTestLogger(XmlMode mode, const char *filename)
    : QAbstractTestLogger(filename), xmlmode(mode)
{
}

QXmlTestLogger::~QXmlTestLogger() = default;

void QXmlTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();
    QTestCharBuffer buf;

    if (xmlmode == QXmlTestLogger::Complete) {
        QTestCharBuffer quotedTc;
        QTest::qt_asprintf(&buf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        outputString(buf.constData());
        if (xmlQuote(&quotedTc, QTestResult::currentTestObjectName())) {
            QTest::qt_asprintf(&buf, "<TestCase name=\"%s\">\n", quotedTc.constData());
            outputString(buf.constData());
        } else {
            // Unconditional end-tag => omitting the start tag is bad.
            Q_ASSERT_X(false, "QXmlTestLogger::startLogging",
                       "Insanely long test-case name or OOM issue");
        }
    }

    QTestCharBuffer quotedBuild;
    if (!QLibraryInfo::build() || xmlQuote(&quotedBuild, QLibraryInfo::build())) {
        QTest::qt_asprintf(&buf,
                           "  <Environment>\n"
                           "    <QtVersion>%s</QtVersion>\n"
                           "    <QtBuild>%s</QtBuild>\n"
                           "    <QTestVersion>" QTEST_VERSION_STR "</QTestVersion>\n"
                           "  </Environment>\n", qVersion(), quotedBuild.constData());
        outputString(buf.constData());
    }
}

void QXmlTestLogger::stopLogging()
{
    QTestCharBuffer buf;

    QTest::qt_asprintf(&buf, "  <Duration msecs=\"%s\"/>\n",
        QString::number(QTestLog::msecsTotalTime()).toUtf8().constData());
    outputString(buf.constData());
    if (xmlmode == QXmlTestLogger::Complete)
        outputString("</TestCase>\n");

    QAbstractTestLogger::stopLogging();
}

void QXmlTestLogger::enterTestFunction(const char *function)
{
    QTestCharBuffer quotedFunction;
    if (xmlQuote(&quotedFunction, function)) {
        QTestCharBuffer buf;
        QTest::qt_asprintf(&buf, "  <TestFunction name=\"%s\">\n", quotedFunction.constData());
        outputString(buf.constData());
    } else {
        // Unconditional end-tag => omitting the start tag is bad.
        Q_ASSERT_X(false, "QXmlTestLogger::enterTestFunction",
                   "Insanely long test-function name or OOM issue");
    }
}

void QXmlTestLogger::leaveTestFunction()
{
    QTestCharBuffer buf;
    QTest::qt_asprintf(&buf,
                "    <Duration msecs=\"%s\"/>\n"
                "  </TestFunction>\n",
        QString::number(QTestLog::msecsFunctionTime()).toUtf8().constData());

    outputString(buf.constData());
}

namespace QTest
{

inline static bool isEmpty(const char *str)
{
    return !str || !str[0];
}

static const char *incidentFormatString(bool noDescription, bool noTag)
{
    if (noDescription) {
        return noTag
            ?   "    <Incident type=\"%s\" file=\"%s\" line=\"%d\" />\n"
            :   "    <Incident type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "      <DataTag><![CDATA[%s%s%s%s]]></DataTag>\n"
                "    </Incident>\n";
    }
    return noTag
        ? "    <Incident type=\"%s\" file=\"%s\" line=\"%d\">\n"
          "      <Description><![CDATA[%s%s%s%s]]></Description>\n"
          "    </Incident>\n"
        : "    <Incident type=\"%s\" file=\"%s\" line=\"%d\">\n"
          "      <DataTag><![CDATA[%s%s%s]]></DataTag>\n"
          "      <Description><![CDATA[%s]]></Description>\n"
          "    </Incident>\n";
}

static const char *benchmarkResultFormatString()
{
    return "  <BenchmarkResult metric=\"%s\" tag=\"%s\" value=\"%.6g\" iterations=\"%d\" />\n";
}

static const char *messageFormatString(bool noDescription, bool noTag)
{
    if (noDescription) {
        if (noTag)
            return "  <Message type=\"%s\" file=\"%s\" line=\"%d\" />\n";
        else
            return "  <Message type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <DataTag><![CDATA[%s%s%s%s]]></DataTag>\n"
                "  </Message>\n";
    } else {
        if (noTag)
            return "  <Message type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <Description><![CDATA[%s%s%s%s]]></Description>\n"
                "  </Message>\n";
        else
            return "  <Message type=\"%s\" file=\"%s\" line=\"%d\">\n"
                "    <DataTag><![CDATA[%s%s%s]]></DataTag>\n"
                "    <Description><![CDATA[%s]]></Description>\n"
                "  </Message>\n";
    }
}

} // namespace

void QXmlTestLogger::addIncident(IncidentTypes type, const char *description,
                                const char *file, int line)
{
    QTestCharBuffer buf;
    const char *tag = QTestResult::currentDataTag();
    const char *gtag = QTestResult::currentGlobalDataTag();
    const char *filler = (tag && gtag) ? ":" : "";
    const bool notag = QTest::isEmpty(tag) && QTest::isEmpty(gtag);

    QTestCharBuffer quotedFile;
    QTestCharBuffer cdataGtag;
    QTestCharBuffer cdataTag;
    QTestCharBuffer cdataDescription;

    if (xmlQuote(&quotedFile, file)
        && xmlCdata(&cdataGtag, gtag)
        && xmlCdata(&cdataTag, tag)
        && xmlCdata(&cdataDescription, description)) {

        QTest::qt_asprintf(&buf,
                           QTest::incidentFormatString(QTest::isEmpty(description), notag),
                           QTest::xmlIncidentType2String(type),
                           quotedFile.constData(), line,
                           cdataGtag.constData(),
                           filler,
                           cdataTag.constData(),
                           cdataDescription.constData());

        outputString(buf.constData());
    }
}

void QXmlTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTestCharBuffer quotedMetric;
    QTestCharBuffer quotedTag;

    if (xmlQuote(&quotedMetric, benchmarkMetricName(result.measurement.metric))
        && xmlQuote(&quotedTag, result.context.tag.toUtf8().constData())) {
        QTestCharBuffer buf;
        QTest::qt_asprintf(&buf,
                           QTest::benchmarkResultFormatString(),
                           quotedMetric.constData(),
                           quotedTag.constData(),
                           result.measurement.value / double(result.iterations),
                           result.iterations);
        outputString(buf.constData());
    }
}

void QXmlTestLogger::addMessage(MessageTypes type, const QString &message,
                                const char *file, int line)
{
    QTestCharBuffer buf;
    const char *tag = QTestResult::currentDataTag();
    const char *gtag = QTestResult::currentGlobalDataTag();
    const char *filler = (tag && gtag) ? ":" : "";
    const bool notag = QTest::isEmpty(tag) && QTest::isEmpty(gtag);

    QTestCharBuffer quotedFile;
    QTestCharBuffer cdataGtag;
    QTestCharBuffer cdataTag;
    QTestCharBuffer cdataDescription;

    if (xmlQuote(&quotedFile, file)
        && xmlCdata(&cdataGtag, gtag)
        && xmlCdata(&cdataTag, tag)
        && xmlCdata(&cdataDescription, message.toUtf8().constData())) {
        QTest::qt_asprintf(&buf,
                           QTest::messageFormatString(message.isEmpty(), notag),
                           QTest::xmlMessageType2String(type),
                           quotedFile.constData(), line,
                           cdataGtag.constData(),
                           filler,
                           cdataTag.constData(),
                           cdataDescription.constData());

        outputString(buf.constData());
    }
}

int QXmlTestLogger::xmlQuote(QTestCharBuffer *destBuf, char const *src, qsizetype n)
{
    // QTestCharBuffer initially has size 512, with '\0' at the start of its
    // data; and we only grow it.
    Q_ASSERT(n >= 512 && destBuf->size() == n);
    char *dest = destBuf->data();

    if (!src || !*src) {
        Q_ASSERT(!dest[0]);
        return 0;
    }

    char *begin = dest;
    char *end = dest + n;

    while (dest < end) {
        switch (*src) {

#define MAP_ENTITY(chr, ent)                        \
        case chr:                                   \
            if (dest + sizeof(ent) < end) {         \
                strcpy(dest, ent);                  \
                dest += sizeof(ent) - 1;            \
            } else {                                \
                *dest = '\0';                       \
                return dest + sizeof(ent) - begin;  \
            }                                       \
            ++src;                                  \
            break;

            MAP_ENTITY('>', "&gt;");
            MAP_ENTITY('<', "&lt;");
            MAP_ENTITY('\'', "&apos;");
            MAP_ENTITY('"', "&quot;");
            MAP_ENTITY('&', "&amp;");

            // Not strictly necessary, but allows handling of comments without
            // having to explicitly look for `--'
            MAP_ENTITY('-', "&#x002D;");

#undef MAP_ENTITY

        case '\0':
            *dest = '\0';
            return dest - begin;

        default:
            *dest = *src;
            ++dest;
            ++src;
            break;
        }
    }

    // If we get here, dest was completely filled:
    Q_ASSERT(dest == end && end > begin);
    dest[-1] = '\0'; // hygiene, but it'll be ignored
    return n;
}

int QXmlTestLogger::xmlCdata(QTestCharBuffer *destBuf, char const *src, qsizetype n)
{
    Q_ASSERT(n >= 512 && destBuf->size() == n);
    char *dest = destBuf->data();

    if (!src || !*src) {
        Q_ASSERT(!dest[0]);
        return 0;
    }

    static char const CDATA_END[] = "]]>";
    static char const CDATA_END_ESCAPED[] = "]]]><![CDATA[]>";
    const size_t CDATA_END_LEN = sizeof(CDATA_END) - 1;

    char *begin = dest;
    char *end = dest + n;
    while (dest < end) {
        if (!*src) {
            *dest = '\0';
            return dest - begin;
        }

        if (!strncmp(src, CDATA_END, CDATA_END_LEN)) {
            if (dest + sizeof(CDATA_END_ESCAPED) < end) {
                strcpy(dest, CDATA_END_ESCAPED);
                src += CDATA_END_LEN;
                dest += sizeof(CDATA_END_ESCAPED) - 1;
            } else {
                *dest = '\0';
                return dest + sizeof(CDATA_END_ESCAPED) - begin;
            }
            continue;
        }

        *dest = *src;
        ++src;
        ++dest;
    }

    // If we get here, dest was completely filled; caller shall grow and retry:
    Q_ASSERT(dest == end && end > begin);
    dest[-1] = '\0'; // hygiene, but it'll be ignored
    return n;
}

typedef int (*StringFormatFunction)(QTestCharBuffer *, char const *, qsizetype);

/*
    A wrapper for string functions written to work with a fixed size buffer so they can be called
    with a dynamically allocated buffer.
*/
static bool allocateStringFn(QTestCharBuffer *str, char const *src, StringFormatFunction func)
{
    constexpr int MAXSIZE = 1024 * 1024 * 2;
    int size = str->size();
    Q_ASSERT(size >= 512 && !str->data()[0]);

    do {
        const int res = func(str, src, size);
        if (res < size) { // Success
            Q_ASSERT(res > 0 || (!res && (!src || !src[0])));
            return true;
        }

        // Buffer wasn't big enough, try again, if not too big:
        size *= 2;
    } while (size <= MAXSIZE && str->reset(size));

    return false;
}

/*
    Copy from \a src into \a destBuf, escaping any special XML characters as
    necessary so that destBuf is suitable for use in an XML quoted attribute
    string. Expands \a destBuf as needed to make room, up to a size of 2
    MiB. Input requiring more than that much space for output is considered
    invalid.

    Returns 0 on invalid or empty input, the actual length written on success.
*/
bool QXmlTestLogger::xmlQuote(QTestCharBuffer *str, char const *src)
{
    return allocateStringFn(str, src, QXmlTestLogger::xmlQuote);
}

/*
    Copy from \a src into \a destBuf, escaping any special strings such that
    destBuf is suitable for use in an XML CDATA section. Expands \a destBuf as
    needed to make room, up to a size of 2 MiB. Input requiring more than that
    much space for output is considered invalid.

    Returns 0 on invalid or empty input, the actual length written on success.
*/
bool QXmlTestLogger::xmlCdata(QTestCharBuffer *str, char const *src)
{
    return allocateStringFn(str, src, QXmlTestLogger::xmlCdata);
}

QT_END_NAMESPACE
