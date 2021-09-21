/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtTest/private/qabstracttestlogger_p.h>
#include <QtTest/qtestassert.h>
#include <qtestresult_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#ifdef Q_OS_ANDROID
#include <sys/stat.h>
#endif

QT_BEGIN_NAMESPACE
/*!
    \internal
    \class QAbstractTestLogger
    \inmodule QtTest
    \brief Base class for test loggers

    Implementations of logging for QtTest should implement all pure virtual
    methods of this class and may implement the other virtual methods. This
    class's documentation of each virtual method sets out how those
    implementations are invoked by the QtTest code and offers guidance on how
    the logging class should use the data. Actual implementations may have
    different requirements - such as a file format with a defined schema, or a
    target audience to serve - that affect how it interprets that guidance.
*/

/*!
    \enum QAbstractTestLogger::IncidentTypes

    \value Pass
    \value XFail
    \value Fail
    \value XPass
    \value BlacklistedPass
    \value BlacklistedFail
    \value BlacklistedXPass
    \value BlacklistedXFail
*/

/*!
    \enum QAbstractTestLogger::MessageTypes
    \value Warn
    \value QWarning
    \value QDebug
    \value QCritical
    \value QFatal
    \value Skip
    \value Info
    \value QInfo
*/

/*!
    Constructs the base-class parts of the logger.

    Derived classes should pass this base-constructor the \a filename of the
    file to which they shall log test results, or \nullptr to write to standard
    output. The protected member \c stream is set to the open file desriptor.
*/
QAbstractTestLogger::QAbstractTestLogger(const char *filename)
{
    if (!filename) {
        stream = stdout;
        return;
    }
#if defined(_MSC_VER)
    if (::fopen_s(&stream, filename, "wt")) {
#else
    stream = ::fopen(filename, "wt");
    if (!stream) {
#endif
        fprintf(stderr, "Unable to open file for logging: %s\n", filename);
        ::exit(1);
    }
#ifdef Q_OS_ANDROID
    else {
        // Make sure output is world-readable on Android
        ::chmod(filename, 0666);
    }
#endif
}

/*!
    Destroys the logger object.

    If the protected \c stream is not standard output, it is closed. In any
    case it is cleared.
*/
QAbstractTestLogger::~QAbstractTestLogger()
{
    QTEST_ASSERT(stream);
    if (stream != stdout)
        fclose(stream);
    stream = nullptr;
}

/*!
    Returns true precisely if the \c output stream is standard output.
*/
bool QAbstractTestLogger::isLoggingToStdout() const
{
    return stream == stdout;
}

/*!
    Helper utility to blot out unprintable characters in \a str.

    Takes a \c{'\0'}-terminated mutable string and changes any characters of it
    that are not suitable for printing to \c{'?'} characters.
*/
void QAbstractTestLogger::filterUnprintable(char *str) const
{
    unsigned char *idx = reinterpret_cast<unsigned char *>(str);
    while (*idx) {
        if (((*idx < 0x20 && *idx != '\n' && *idx != '\t') || *idx == 0x7f))
            *idx = '?';
        ++idx;
    }
}

/*!
    Convenience method to write \a msg to the output stream.

    The output \a msg must be a \c{'\0'}-terminated string (and not \nullptr).
    A copy of it is passed to \l filterUnprintable() and the result written to
    the output \c stream, which is then flushed.
*/
void QAbstractTestLogger::outputString(const char *msg)
{
    QTEST_ASSERT(stream);
    QTEST_ASSERT(msg);

    char *filtered = new char[strlen(msg) + 1];
    strcpy(filtered, msg);
    filterUnprintable(filtered);

    ::fputs(filtered, stream);
    ::fflush(stream);

    delete [] filtered;
}

/*!
    Called before the start of a test run.

    This virtual method is called before the first tests are run. A logging
    implementation might open a file, write some preamble or prepare in other
    ways, such as setting up initial values of variables. It can use the usual
    Qt logging infrastucture, since it is also called before QtTest installs its
    own custom message handler.

    \sa stopLogging()
*/
void QAbstractTestLogger::startLogging()
{
}

/*!
    Called after the end of a test run.

    This virtual method is called after all tests have run. A logging
    implementation might collate information gathered from the run, write a
    summary or close a file. It can use the usual Qt logging infrastucture,
    since it is also called after QtTest has restored the default message
    handler it replaced with its own custom message handler.

    \sa startLogging()
*/
void QAbstractTestLogger::stopLogging()
{
}

/*!
    \fn void QAbstractTestLogger::enterTestFunction(const char *function)

    This virtual method is called before each test function is invoked. It is
    passed the name of the test function (without its class prefix) as \a
    function. It is likewise called for \c{initTestCase()} at the start of
    testing, after \l startLogging(), and for \c{cleanupTestCase()} at the end
    of testing, in each case passing the name of the function. It is also called
    with \nullptr as \function after the last of these functions, or in the
    event of an early end to testing, before \l stopLogging().

    For data-driven test functions, this is called only once, before the data
    function is called to set up the table of datasets and the test is run with
    its first dataset.

    Every logging implementation must implement this method. It shall typically
    need to record the name of the function for later use in log messages.

    \sa leaveTestFunction(), enterTestData()
*/
/*!
    \fn void QAbstractTestLogger::leaveTestFunction()

    This virtual method is called after a test function has completed, to match
    \l enterTestFunction(). For data-driven test functions, this is called only
    once, after the test is run with its last dataset.

    Every logging implementation must implement this method. In some cases it
    may be called more than once without an intervening call to \l
    enterTestFunction(); in such cases, the implementation should ignore the
    second (and later) calls.

    \sa enterTestFunction(), enterTestData()
*/
/*!
    \fn void QAbstractTestLogger::enterTestData(QTestData *)

    This virtual method is called before and after each call to a test
    function. For a data-driven test, the call before is passed the name of the
    test data row. This may combine a global data row name with a local data row
    name. For non-data-driven tests and for the call after a test function,
    \nullptr is passed

    A logging implementation might chose to record the data row name for
    reporting of results from the test for that data row. It should, in such a
    case, clear its record of the name when called with \nullptr.

    \sa enterTestFunction(), leaveTestFunction()
*/
/*!
    \fn void QAbstractTestLogger::addIncident(IncidentTypes type, const char *description, const char *file, int line)

    This virtual method is called when an event occurs that bears on whether the
    test passes or fails. The \a type indicates whether this was a pass or a
    fail, whether a failure was expected and whether the test being run is
    blacklisted. The \a description may be empty (for a pass), a message
    describing the failure or, for an expected failure, the explanation of why a
    failure was expected. Where the location in code of the incident is known,
    it is indicated by \a file and \a line; otherwise, these are \a nullptr and
    0, respectively.

    Every logging implementation must implement this method. Note that there are
    circumstances where more than one incident may be reported, in this way, for
    a single run of a test on a single dataset.  It is (for now) the
    implementation's responsibility to recognize such cases and decide what to
    do about them.

    \sa addMessage(), addBenchmarkResult()
*/
/*!
    \fn void QAbstractTestLogger::addBenchmarkResult(const QBenchmarkResult &result)

    This virtual method is called after a benchmark has been run enough times to
    produce usable data. It is passed the median \a result from all cycles of
    the code controlled by the test's QBENCHMARK loop.

    Every logging implementation must implement this method.

    \sa addIncident(), addMessage()
*/
/*!
    \overload
    \fn void QAbstractTestLogger::addMessage(MessageTypes type, const QString &message, const char *file, int line)

    This virtual method is called, via its \c QtMsgType overload, from the
    custom message handler QtTest installs. It is also used by the
    implementations of QSKIP(), to warn about various situations detected by
    QtTest itself, such as \e failure to see a message anticipated by
    QTest::ignoreMessage() and, particularly when verbosity options have been
    enabled via the command-line, to log some extra information.

    Every logging implementation must implement this method. The \a type
    indicates the category of message and the \a message is the content to be
    reported. When the message is associated with specific code, the name of the
    \a file and \a line number within it are also supplied (otherwise, these are
    \nullptr and 0, respectively).

    \sa QTest::ignoreMessage(), addIncident()
*/

/*!
    \overload

    This virtual method is called from the custom message handler QtTest
    installs in place of Qt's default message handler for the duration of
    testing, unless QTest::ignoreMessage() was used to ignore it, or too many
    messages have previously been processed. (The limiting number of messages is
    controlled by the -maxwarnings option to a test and defaults to 2002.)

    Logging implementation should not normally need to over-ride this method.
    The base implementation converts \a type to the matching \l MessageType,
    formats the given \a message suitably for the specified \a context and
    forwards the converted type and formatted message to the overload taking
    MessageType and QString.

    \sa QTest::ignoreMessage(), addIncident()
*/
void QAbstractTestLogger::addMessage(QtMsgType type, const QMessageLogContext &context,
                                     const QString &message)
{
    QAbstractTestLogger::MessageTypes messageType = [=]() {
        switch (type) {
        case QtDebugMsg: return QAbstractTestLogger::QDebug;
        case QtInfoMsg: return QAbstractTestLogger::QInfo;
        case QtCriticalMsg: return QAbstractTestLogger::QCritical;
        case QtWarningMsg: return QAbstractTestLogger::QWarning;
        case QtFatalMsg: return QAbstractTestLogger::QFatal;
        }
        Q_UNREACHABLE();
        return QAbstractTestLogger::QFatal;
    }();

    QString formattedMessage = qFormatLogMessage(type, context, message);

    // Note that we explicitly ignore the file and line of the context here,
    // as that's what QTest::messageHandler used to do when calling the same
    // overload directly.
    addMessage(messageType, formattedMessage);
}

namespace QTest
{

extern void filter_unprintable(char *str);

/*!
    \fn int QTest::qt_asprintf(QTestCharBuffer *buf, const char *format, ...);
    \internal
 */
int qt_asprintf(QTestCharBuffer *str, const char *format, ...)
{
    static const int MAXSIZE = 1024*1024*2;

    Q_ASSERT(str);

    int size = str->size();

    va_list ap;
    int res = 0;

    for (;;) {
        va_start(ap, format);
        res = qvsnprintf(str->data(), size, format, ap);
        va_end(ap);
        str->data()[size - 1] = '\0';
        if (res >= 0 && res < size) {
            // We succeeded
            break;
        }
        // buffer wasn't big enough, try again.
        // Note, we're assuming that a result of -1 is always due to running out of space.
        size *= 2;
        if (size > MAXSIZE) {
            break;
        }
        if (!str->reset(size))
            break; // out of memory - take what we have
    }

    return res;
}

}

namespace QTestPrivate
{

void generateTestIdentifier(QTestCharBuffer *identifier, int parts)
{
    const char *testObject = parts & TestObject ? QTestResult::currentTestObjectName() : "";
    const char *testFunction = parts & TestFunction ? (QTestResult::currentTestFunction() ? QTestResult::currentTestFunction() : "UnknownTestFunc") : "";
    const char *objectFunctionFiller = parts & TestObject && parts & (TestFunction | TestDataTag) ? "::" : "";
    const char *testFuctionStart = parts & TestFunction ? "(" : "";
    const char *testFuctionEnd = parts & TestFunction ? ")" : "";

    const char *dataTag = (parts & TestDataTag) && QTestResult::currentDataTag() ? QTestResult::currentDataTag() : "";
    const char *globalDataTag = (parts & TestDataTag) && QTestResult::currentGlobalDataTag() ? QTestResult::currentGlobalDataTag() : "";
    const char *tagFiller = (dataTag[0] && globalDataTag[0]) ? ":" : "";

    QTest::qt_asprintf(identifier, "%s%s%s%s%s%s%s%s",
        testObject, objectFunctionFiller, testFunction, testFuctionStart,
        globalDataTag, tagFiller, dataTag, testFuctionEnd);
}

}

QT_END_NAMESPACE
