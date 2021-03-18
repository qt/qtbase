/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QtTest/qtestcase.h>
#include <QtTest/qtestassert.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvector.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfloat16.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/private/qtools_p.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qlocking_p.h>
#include <QtCore/private/qwaitcondition_p.h>

#include <QtCore/qtestsupport_core.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtesttable_p.h>
#include <QtTest/qtestdata.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qsignaldumper_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/cycle_p.h>
#include <QtTest/private/qtestblacklist_p.h>
#if defined(HAVE_XCTEST)
#include <QtTest/private/qxctestlogger_p.h>
#endif
#if defined Q_OS_MACOS
#include <QtTest/private/qtestutil_macos_p.h>
#endif

#if defined(Q_OS_DARWIN)
#include <QtTest/private/qappletestlogger_p.h>
#endif

#include <cmath>
#include <numeric>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <chrono>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(Q_OS_LINUX)
#include <sys/types.h>
#include <fcntl.h>
#endif

#ifdef Q_OS_WIN
# if !defined(Q_CC_MINGW) || (defined(Q_CC_MINGW) && defined(__MINGW64_VERSION_MAJOR))
#  include <crtdbg.h>
# endif
#include <windows.h> // for Sleep
#endif
#ifdef Q_OS_UNIX
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
# if !defined(Q_OS_INTEGRITY)
#  include <sys/resource.h>
# endif
#endif

#if defined(Q_OS_MACX)
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <CoreFoundation/CFPreferences.h>
#endif

#include <vector>

QT_BEGIN_NAMESPACE

using QtMiscUtils::toHexUpper;
using QtMiscUtils::fromHex;

static bool debuggerPresent()
{
#if defined(Q_OS_LINUX)
    int fd = open("/proc/self/status", O_RDONLY);
    if (fd == -1)
        return false;
    char buffer[2048];
    ssize_t size = read(fd, buffer, sizeof(buffer) - 1);
    if (size == -1) {
        close(fd);
        return false;
    }
    buffer[size] = 0;
    const char tracerPidToken[] = "\nTracerPid:";
    char *tracerPid = strstr(buffer, tracerPidToken);
    if (!tracerPid) {
        close(fd);
        return false;
    }
    tracerPid += sizeof(tracerPidToken);
    long int pid = strtol(tracerPid, &tracerPid, 10);
    close(fd);
    return pid != 0;
#elif defined(Q_OS_WIN)
    return IsDebuggerPresent();
#elif defined(Q_OS_MACOS)
    // Check if there is an exception handler for the process:
    mach_msg_type_number_t portCount = 0;
    exception_mask_t masks[EXC_TYPES_COUNT];
    mach_port_t ports[EXC_TYPES_COUNT];
    exception_behavior_t behaviors[EXC_TYPES_COUNT];
    thread_state_flavor_t flavors[EXC_TYPES_COUNT];
    exception_mask_t mask = EXC_MASK_ALL & ~(EXC_MASK_RESOURCE | EXC_MASK_GUARD);
    kern_return_t result = task_get_exception_ports(mach_task_self(), mask, masks, &portCount,
                                                    ports, behaviors, flavors);
    if (result == KERN_SUCCESS) {
        for (mach_msg_type_number_t portIndex = 0; portIndex < portCount; ++portIndex) {
            if (MACH_PORT_VALID(ports[portIndex])) {
                return true;
            }
        }
    }
    return false;
#else
    // TODO
    return false;
#endif
}

static bool hasSystemCrashReporter()
{
#if defined(Q_OS_MACOS)
    return QTestPrivate::macCrashReporterWillShowDialog();
#else
    return false;
#endif
}

static void disableCoreDump()
{
    bool ok = false;
    const int disableCoreDump = qEnvironmentVariableIntValue("QTEST_DISABLE_CORE_DUMP", &ok);
    if (ok && disableCoreDump == 1) {
#if defined(Q_OS_UNIX) && !defined(Q_OS_INTEGRITY)
        struct rlimit limit;
        limit.rlim_cur = 0;
        limit.rlim_max = 0;
        if (setrlimit(RLIMIT_CORE, &limit) != 0)
            qWarning("Failed to disable core dumps: %d", errno);
#endif
    }
}
Q_CONSTRUCTOR_FUNCTION(disableCoreDump);

static void stackTrace()
{
    bool ok = false;
    const int disableStackDump = qEnvironmentVariableIntValue("QTEST_DISABLE_STACK_DUMP", &ok);
    if (ok && disableStackDump == 1)
        return;

    if (debuggerPresent() || hasSystemCrashReporter())
        return;

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    const int msecsFunctionTime = qRound(QTestLog::msecsFunctionTime());
    const int msecsTotalTime = qRound(QTestLog::msecsTotalTime());
    fprintf(stderr, "\n=== Received signal at function time: %dms, total time: %dms, dumping stack ===\n",
            msecsFunctionTime, msecsTotalTime);
#endif
#ifdef Q_OS_LINUX
    char cmd[512];
    qsnprintf(cmd, 512, "gdb --pid %d 2>/dev/null <<EOF\n"
                         "set prompt\n"
                         "set height 0\n"
                         "thread apply all where full\n"
                         "detach\n"
                         "quit\n"
                         "EOF\n",
                         (int)getpid());
    if (system(cmd) == -1)
        fprintf(stderr, "calling gdb failed\n");
    fprintf(stderr, "=== End of stack trace ===\n");
#elif defined(Q_OS_MACOS)
    char cmd[512];
    qsnprintf(cmd, 512, "lldb -p %d 2>/dev/null <<EOF\n"
                         "bt all\n"
                         "quit\n"
                         "EOF\n",
                         (int)getpid());
    if (system(cmd) == -1)
        fprintf(stderr, "calling lldb failed\n");
    fprintf(stderr, "=== End of stack trace ===\n");
#endif
}

static bool installCoverageTool(const char * appname, const char * testname)
{
#if defined(__COVERAGESCANNER__) && !QT_CONFIG(testlib_selfcover)
    if (!qEnvironmentVariableIsEmpty("QT_TESTCOCOON_ACTIVE"))
        return false;
    // Set environment variable QT_TESTCOCOON_ACTIVE to prevent an eventual subtest from
    // being considered as a stand-alone test regarding the coverage analysis.
    qputenv("QT_TESTCOCOON_ACTIVE", "1");

    // Install Coverage Tool
    __coveragescanner_install(appname);
    __coveragescanner_testname(testname);
    __coveragescanner_clear();
    return true;
#else
    Q_UNUSED(appname);
    Q_UNUSED(testname);
    return false;
#endif
}

static bool isValidSlot(const QMetaMethod &sl)
{
    if (sl.access() != QMetaMethod::Private || sl.parameterCount() != 0
        || sl.returnType() != QMetaType::Void || sl.methodType() != QMetaMethod::Slot)
        return false;
    const QByteArray name = sl.name();
    return !(name.isEmpty() || name.endsWith("_data")
        || name == "initTestCase" || name == "cleanupTestCase"
        || name == "init" || name == "cleanup");
}

namespace QTestPrivate
{
    Q_TESTLIB_EXPORT Qt::MouseButtons qtestMouseButtons = Qt::NoButton;
}

namespace QTest
{
extern Q_TESTLIB_EXPORT int lastMouseTimestamp;

class WatchDog;

static QObject *currentTestObject = nullptr;
static QString mainSourcePath;

#if defined(Q_OS_MACOS)
static IOPMAssertionID macPowerSavingDisabled = 0;
#endif

class TestMethods {
public:
    Q_DISABLE_COPY_MOVE(TestMethods)

    using MetaMethods = std::vector<QMetaMethod>;

    explicit TestMethods(const QObject *o, const MetaMethods &m = MetaMethods());

    void invokeTests(QObject *testObject) const;

    static QMetaMethod findMethod(const QObject *obj, const char *signature);

private:
    bool invokeTest(int index, const char *data, WatchDog *watchDog) const;
    void invokeTestOnData(int index) const;

    QMetaMethod m_initTestCaseMethod; // might not exist, check isValid().
    QMetaMethod m_initTestCaseDataMethod;
    QMetaMethod m_cleanupTestCaseMethod;
    QMetaMethod m_initMethod;
    QMetaMethod m_cleanupMethod;

    MetaMethods m_methods;
};

TestMethods::TestMethods(const QObject *o, const MetaMethods &m)
    : m_initTestCaseMethod(TestMethods::findMethod(o, "initTestCase()"))
    , m_initTestCaseDataMethod(TestMethods::findMethod(o, "initTestCase_data()"))
    , m_cleanupTestCaseMethod(TestMethods::findMethod(o, "cleanupTestCase()"))
    , m_initMethod(TestMethods::findMethod(o, "init()"))
    , m_cleanupMethod(TestMethods::findMethod(o, "cleanup()"))
    , m_methods(m)
{
    if (m.empty()) {
        const QMetaObject *metaObject = o->metaObject();
        const int count = metaObject->methodCount();
        m_methods.reserve(count);
        for (int i = 0; i < count; ++i) {
            const QMetaMethod me = metaObject->method(i);
            if (isValidSlot(me))
                m_methods.push_back(me);
        }
    }
}

QMetaMethod TestMethods::findMethod(const QObject *obj, const char *signature)
{
    const QMetaObject *metaObject = obj->metaObject();
    const int funcIndex = metaObject->indexOfMethod(signature);
    return funcIndex >= 0 ? metaObject->method(funcIndex) : QMetaMethod();
}

static int keyDelay = -1;
static int mouseDelay = -1;
static int eventDelay = -1;
#if QT_CONFIG(thread)
static int timeout = -1;
#endif
static bool noCrashHandler = false;

/*! \internal
    Invoke a method of the object without generating warning if the method does not exist
 */
static void invokeMethod(QObject *obj, const char *methodName)
{
    const QMetaObject *metaObject = obj->metaObject();
    int funcIndex = metaObject->indexOfMethod(methodName);
    if (funcIndex >= 0) {
        QMetaMethod method = metaObject->method(funcIndex);
        method.invoke(obj, Qt::DirectConnection);
    }
}

int defaultEventDelay()
{
    if (eventDelay == -1) {
        const QByteArray env = qgetenv("QTEST_EVENT_DELAY");
        if (!env.isEmpty())
            eventDelay = atoi(env.constData());
        else
            eventDelay = 0;
    }
    return eventDelay;
}

int Q_TESTLIB_EXPORT defaultMouseDelay()
{
    if (mouseDelay == -1) {
        const QByteArray env = qgetenv("QTEST_MOUSEEVENT_DELAY");
        if (!env.isEmpty())
            mouseDelay = atoi(env.constData());
        else
            mouseDelay = defaultEventDelay();
    }
    return mouseDelay;
}

int Q_TESTLIB_EXPORT defaultKeyDelay()
{
    if (keyDelay == -1) {
        const QByteArray env = qgetenv("QTEST_KEYEVENT_DELAY");
        if (!env.isEmpty())
            keyDelay = atoi(env.constData());
        else
            keyDelay = defaultEventDelay();
    }
    return keyDelay;
}
#if QT_CONFIG(thread)
static std::chrono::milliseconds defaultTimeout()
{
    if (timeout == -1) {
        bool ok = false;
        timeout = qEnvironmentVariableIntValue("QTEST_FUNCTION_TIMEOUT", &ok);

        if (!ok || timeout <= 0)
            timeout = 5*60*1000;
    }
    return std::chrono::milliseconds{timeout};
}
#endif

Q_TESTLIB_EXPORT bool printAvailableFunctions = false;
Q_TESTLIB_EXPORT QStringList testFunctions;
Q_TESTLIB_EXPORT QStringList testTags;

static void qPrintTestSlots(FILE *stream, const char *filter = nullptr)
{
    for (int i = 0; i < QTest::currentTestObject->metaObject()->methodCount(); ++i) {
        QMetaMethod sl = QTest::currentTestObject->metaObject()->method(i);
        if (isValidSlot(sl)) {
            const QByteArray signature = sl.methodSignature();
            if (!filter || QString::fromLatin1(signature).contains(QLatin1String(filter), Qt::CaseInsensitive))
                fprintf(stream, "%s\n", signature.constData());
        }
    }
}

static void qPrintDataTags(FILE *stream)
{
    // Avoid invoking the actual test functions, and also avoid printing irrelevant output:
    QTestLog::setPrintAvailableTagsMode();

    // Get global data tags:
    QTestTable::globalTestTable();
    invokeMethod(QTest::currentTestObject, "initTestCase_data()");
    const QTestTable *gTable = QTestTable::globalTestTable();

    const QMetaObject *currTestMetaObj = QTest::currentTestObject->metaObject();

    // Process test functions:
    for (int i = 0; i < currTestMetaObj->methodCount(); ++i) {
        QMetaMethod tf = currTestMetaObj->method(i);

        if (isValidSlot(tf)) {

            // Retrieve local tags:
            QStringList localTags;
            QTestTable table;
            char *slot = qstrdup(tf.methodSignature().constData());
            slot[strlen(slot) - 2] = '\0';
            QByteArray member;
            member.resize(qstrlen(slot) + qstrlen("_data()") + 1);
            qsnprintf(member.data(), member.size(), "%s_data()", slot);
            invokeMethod(QTest::currentTestObject, member.constData());
            const int dataCount = table.dataCount();
            localTags.reserve(dataCount);
            for (int j = 0; j < dataCount; ++j)
                localTags << QLatin1String(table.testData(j)->dataTag());

            // Print all tag combinations:
            if (gTable->dataCount() == 0) {
                if (localTags.count() == 0) {
                    // No tags at all, so just print the test function:
                    fprintf(stream, "%s %s\n", currTestMetaObj->className(), slot);
                } else {
                    // Only local tags, so print each of them:
                    for (int k = 0; k < localTags.size(); ++k)
                        fprintf(
                            stream, "%s %s %s\n",
                            currTestMetaObj->className(), slot, localTags.at(k).toLatin1().data());
                }
            } else {
                for (int j = 0; j < gTable->dataCount(); ++j) {
                    if (localTags.count() == 0) {
                        // Only global tags, so print the current one:
                        fprintf(
                            stream, "%s %s __global__ %s\n",
                            currTestMetaObj->className(), slot, gTable->testData(j)->dataTag());
                    } else {
                        // Local and global tags, so print each of the local ones and
                        // the current global one:
                        for (int k = 0; k < localTags.size(); ++k)
                            fprintf(
                                stream, "%s %s %s __global__ %s\n", currTestMetaObj->className(), slot,
                                localTags.at(k).toLatin1().data(), gTable->testData(j)->dataTag());
                    }
                }
            }

            delete[] slot;
        }
    }
}

static int qToInt(const char *str)
{
    char *pEnd;
    int l = (int)strtol(str, &pEnd, 10);
    if (*pEnd != 0) {
        fprintf(stderr, "Invalid numeric parameter: '%s'\n", str);
        exit(1);
    }
    return l;
}

Q_TESTLIB_EXPORT void qtest_qParseArgs(int argc, const char *const argv[], bool qml)
{
    int logFormat = -1; // Not set
    const char *logFilename = nullptr;

    QTest::testFunctions.clear();
    QTest::testTags.clear();

#if defined(Q_OS_MAC) && defined(HAVE_XCTEST)
    if (QXcodeTestLogger::canLogTestProgress())
        logFormat = QTestLog::XCTest;
#endif

    const char *testOptions =
         " New-style logging options:\n"
         " -o filename,format  : Output results to file in the specified format\n"
         "                       Use - to output to stdout\n"
         "                       Valid formats are:\n"
         "                         txt      : Plain text\n"
         "                         csv      : CSV format (suitable for benchmarks)\n"
         "                         junitxml : XML JUnit document\n"
         "                         xml      : XML document\n"
         "                         lightxml : A stream of XML tags\n"
         "                         teamcity : TeamCity format\n"
         "                         tap      : Test Anything Protocol\n"
         "\n"
         "     *** Multiple loggers can be specified, but at most one can log to stdout.\n"
         "\n"
         " Old-style logging options:\n"
         " -o filename         : Write the output into file\n"
         " -txt                : Output results in Plain Text\n"
         " -csv                : Output results in a CSV format (suitable for benchmarks)\n"
         " -junitxml           : Output results as XML JUnit document\n"
         " -xml                : Output results as XML document\n"
         " -lightxml           : Output results as stream of XML tags\n"
         " -teamcity           : Output results in TeamCity format\n"
         " -tap                : Output results in Test Anything Protocol format\n"
         "\n"
         "     *** If no output file is specified, stdout is assumed.\n"
         "     *** If no output format is specified, -txt is assumed.\n"
         "\n"
         " Test log detail options:\n"
         " -silent             : Log failures and fatal errors only\n"
         " -v1                 : Log the start of each testfunction\n"
         " -v2                 : Log each QVERIFY/QCOMPARE/QTEST (implies -v1)\n"
         " -vs                 : Log every signal emission and resulting slot invocations\n"
         "\n"
         "     *** The -silent and -v1 options only affect plain text output.\n"
         "\n"
         " Testing options:\n"
         " -functions          : Returns a list of current testfunctions\n"
         " -datatags           : Returns a list of current data tags.\n"
         "                       A global data tag is preceded by ' __global__ '.\n"
         " -eventdelay ms      : Set default delay for mouse and keyboard simulation to ms milliseconds\n"
         " -keydelay ms        : Set default delay for keyboard simulation to ms milliseconds\n"
         " -mousedelay ms      : Set default delay for mouse simulation to ms milliseconds\n"
         " -maxwarnings n      : Sets the maximum amount of messages to output.\n"
         "                       0 means unlimited, default: 2000\n"
         " -nocrashhandler     : Disables the crash handler. Useful for debugging crashes.\n"
         "\n"
         " Benchmarking options:\n"
#if QT_CONFIG(valgrind)
         " -callgrind          : Use callgrind to time benchmarks\n"
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
         " -perf               : Use Linux perf events to time benchmarks\n"
         " -perfcounter name   : Use the counter named 'name'\n"
         " -perfcounterlist    : Lists the counters available\n"
#endif
#ifdef HAVE_TICK_COUNTER
         " -tickcounter        : Use CPU tick counters to time benchmarks\n"
#endif
         " -eventcounter       : Counts events received during benchmarks\n"
         " -minimumvalue n     : Sets the minimum acceptable measurement value\n"
         " -minimumtotal n     : Sets the minimum acceptable total for repeated executions of a test function\n"
         " -iterations  n      : Sets the number of accumulation iterations.\n"
         " -median  n          : Sets the number of median iterations.\n"
         " -vb                 : Print out verbose benchmarking information.\n";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0
            || strcmp(argv[i], "/?") == 0) {
            printf(" Usage: %s [options] [testfunction[:testdata]]...\n"
                   "    By default, all testfunctions will be run.\n\n"
                   "%s", argv[0], testOptions);

            if (qml) {
                printf ("\n"
                        " QmlTest options:\n"
                        " -import dir         : Specify an import directory.\n"
                        " -plugins dir        : Specify a directory where to search for plugins.\n"
                        " -input dir/file     : Specify the root directory for test cases or a single test case file.\n"
                        " -translation file   : Specify the translation file.\n"
                        " -file-selector dir  : Specify a file selector for the QML engine.\n"
                        );
            }

            printf("\n"
                   " -help               : This help\n");
            exit(0);
        } else if (strcmp(argv[i], "-functions") == 0) {
            if (qml) {
                QTest::printAvailableFunctions = true;
            } else {
                qPrintTestSlots(stdout);
                exit(0);
            }
        } else if (strcmp(argv[i], "-datatags") == 0) {
            if (!qml) {
                qPrintDataTags(stdout);
                exit(0);
            }
        } else if (strcmp(argv[i], "-txt") == 0) {
            logFormat = QTestLog::Plain;
        } else if (strcmp(argv[i], "-csv") == 0) {
            logFormat = QTestLog::CSV;
        } else if (strcmp(argv[i], "-junitxml") == 0 || strcmp(argv[i], "-xunitxml") == 0)  {
            logFormat = QTestLog::JUnitXML;
        } else if (strcmp(argv[i], "-xml") == 0) {
            logFormat = QTestLog::XML;
        } else if (strcmp(argv[i], "-lightxml") == 0) {
            logFormat = QTestLog::LightXML;
        } else if (strcmp(argv[i], "-teamcity") == 0) {
            logFormat = QTestLog::TeamCity;
        } else if (strcmp(argv[i], "-tap") == 0) {
            logFormat = QTestLog::TAP;
        } else if (strcmp(argv[i], "-silent") == 0) {
            QTestLog::setVerboseLevel(-1);
        } else if (strcmp(argv[i], "-v1") == 0) {
            QTestLog::setVerboseLevel(1);
        } else if (strcmp(argv[i], "-v2") == 0) {
            QTestLog::setVerboseLevel(2);
        } else if (strcmp(argv[i], "-vs") == 0) {
            QSignalDumper::startDump();
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-o needs an extra parameter specifying the filename and optional format\n");
                exit(1);
            }
            ++i;
            // Do we have the old or new style -o option?
            char *filename = new char[strlen(argv[i])+1];
            char *format = new char[strlen(argv[i])+1];
            if (sscanf(argv[i], "%[^,],%s", filename, format) == 1) {
                // Old-style
                logFilename = argv[i];
            } else {
                // New-style
                if (strcmp(format, "txt") == 0)
                    logFormat = QTestLog::Plain;
                else if (strcmp(format, "csv") == 0)
                    logFormat = QTestLog::CSV;
                else if (strcmp(format, "lightxml") == 0)
                    logFormat = QTestLog::LightXML;
                else if (strcmp(format, "xml") == 0)
                    logFormat = QTestLog::XML;
                else if (strcmp(format, "junitxml") == 0 || strcmp(format, "xunitxml") == 0)
                    logFormat = QTestLog::JUnitXML;
                else if (strcmp(format, "teamcity") == 0)
                    logFormat = QTestLog::TeamCity;
                else if (strcmp(format, "tap") == 0)
                    logFormat = QTestLog::TAP;
                else {
                    fprintf(stderr, "output format must be one of txt, csv, lightxml, xml, tap, teamcity or junitxml\n");
                    exit(1);
                }
                if (strcmp(filename, "-") == 0 && QTestLog::loggerUsingStdout()) {
                    fprintf(stderr, "only one logger can log to stdout\n");
                    exit(1);
                }
                QTestLog::addLogger(QTestLog::LogMode(logFormat), filename);
            }
            delete [] filename;
            delete [] format;
        } else if (strcmp(argv[i], "-eventdelay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-eventdelay needs an extra parameter to indicate the delay(ms)\n");
                exit(1);
            } else {
                QTest::eventDelay = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-keydelay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-keydelay needs an extra parameter to indicate the delay(ms)\n");
                exit(1);
            } else {
                QTest::keyDelay = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-mousedelay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-mousedelay needs an extra parameter to indicate the delay(ms)\n");
                exit(1);
            } else {
                QTest::mouseDelay = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-maxwarnings") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-maxwarnings needs an extra parameter with the amount of warnings\n");
                exit(1);
            } else {
                QTestLog::setMaxWarnings(qToInt(argv[++i]));
            }
        } else if (strcmp(argv[i], "-nocrashhandler") == 0) {
            QTest::noCrashHandler = true;
#if QT_CONFIG(valgrind)
        } else if (strcmp(argv[i], "-callgrind") == 0) {
            if (QBenchmarkValgrindUtils::haveValgrind())
                if (QFileInfo(QDir::currentPath()).isWritable()) {
                    QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::CallgrindParentProcess);
                } else {
                    fprintf(stderr, "WARNING: Current directory not writable. Using the walltime measurer.\n");
                }
            else {
                fprintf(stderr, "WARNING: Valgrind not found or too old. Make sure it is installed and in your path. "
                       "Using the walltime measurer.\n");
            }
        } else if (strcmp(argv[i], "-callgrindchild") == 0) { // "private" option
            QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::CallgrindChildProcess);
            QBenchmarkGlobalData::current->callgrindOutFileBase =
                QBenchmarkValgrindUtils::outFileBase();
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
        } else if (strcmp(argv[i], "-perf") == 0) {
            if (QBenchmarkPerfEventsMeasurer::isAvailable()) {
                // perf available
                QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::PerfCounter);
            } else {
                fprintf(stderr, "WARNING: Linux perf events not available. Using the walltime measurer.\n");
            }
        } else if (strcmp(argv[i], "-perfcounter") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-perfcounter needs an extra parameter with the name of the counter\n");
                exit(1);
            } else {
                QBenchmarkPerfEventsMeasurer::setCounter(argv[++i]);
            }
        } else if (strcmp(argv[i], "-perfcounterlist") == 0) {
            QBenchmarkPerfEventsMeasurer::listCounters();
            exit(0);
#endif
#ifdef HAVE_TICK_COUNTER
        } else if (strcmp(argv[i], "-tickcounter") == 0) {
            QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::TickCounter);
#endif
        } else if (strcmp(argv[i], "-eventcounter") == 0) {
            QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::EventCounter);
        } else if (strcmp(argv[i], "-minimumvalue") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-minimumvalue needs an extra parameter to indicate the minimum time(ms)\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->walltimeMinimum = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-minimumtotal") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-minimumtotal needs an extra parameter to indicate the minimum total measurement\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->minimumTotal = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-iterations") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-iterations needs an extra parameter to indicate the number of iterations\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->iterationCount = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-median") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-median needs an extra parameter to indicate the number of median iterations\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->medianIterationCount = qToInt(argv[++i]);
            }

        } else if (strcmp(argv[i], "-vb") == 0) {
            QBenchmarkGlobalData::current->verboseOutput = true;
#if defined(Q_OS_WINRT)
        } else if (strncmp(argv[i], "-ServerName:", 12) == 0 ||
                   strncmp(argv[i], "-qdevel", 7) == 0) {
            continue;
#elif defined(Q_OS_MAC) && defined(HAVE_XCTEST)
        } else if (int skip = QXcodeTestLogger::parseCommandLineArgument(argv[i])) {
            i += (skip - 1); // Eating argv[i] with a continue counts towards skips
            continue;
#endif
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: '%s'\n\n%s", argv[i], testOptions);
            if (qml) {
                fprintf(stderr, "\nqmltest related options:\n"
                                " -import    : Specify an import directory.\n"
                                " -plugins   : Specify a directory where to search for plugins.\n"
                                " -input     : Specify the root directory for test cases.\n"
                       );
            }

            fprintf(stderr, "\n"
                            " -help      : This help\n");
            exit(1);
        } else {
            // We can't check the availability of test functions until
            // we load the QML files.  So just store the data for now.
            int colon = -1;
            int offset;
            for (offset = 0; argv[i][offset]; ++offset) {
                if (argv[i][offset] == ':') {
                    if (argv[i][offset + 1] == ':') {
                        // "::" is used as a test name separator.
                        // e.g. "ClickTests::test_click:row1".
                        ++offset;
                    } else {
                        colon = offset;
                        break;
                    }
                }
            }
            if (colon == -1) {
                QTest::testFunctions += QString::fromLatin1(argv[i]);
                QTest::testTags += QString();
            } else {
                QTest::testFunctions +=
                    QString::fromLatin1(argv[i], colon);
                QTest::testTags +=
                    QString::fromLatin1(argv[i] + colon + 1);
            }
        }
    }

    bool installedTestCoverage = installCoverageTool(QTestResult::currentAppName(), QTestResult::currentTestObjectName());
    QTestLog::setInstalledTestCoverage(installedTestCoverage);

    // If no loggers were created by the long version of the -o command-line
    // option, but a logger was requested via the old-style option, add it.
    const bool explicitLoggerRequested = logFormat != -1;
    if (QTestLog::loggerCount() == 0 && explicitLoggerRequested)
        QTestLog::addLogger(QTestLog::LogMode(logFormat), logFilename);

    bool addFallbackLogger = !explicitLoggerRequested;

#if defined(QT_USE_APPLE_UNIFIED_LOGGING)
    // Any explicitly requested loggers will be added by now, so we can check if they use stdout
    const bool safeToAddAppleLogger = !AppleUnifiedLogger::willMirrorToStderr() || !QTestLog::loggerUsingStdout();
    if (safeToAddAppleLogger && QAppleTestLogger::debugLoggingEnabled()) {
        QTestLog::addLogger(QTestLog::Apple, nullptr);
        if (AppleUnifiedLogger::willMirrorToStderr() && !logFilename)
            addFallbackLogger = false; // Prevent plain test logger fallback below
    }
#endif

    if (addFallbackLogger)
        QTestLog::addLogger(QTestLog::Plain, logFilename);
}

// Temporary, backwards compatibility, until qtdeclarative's use of it is converted
Q_TESTLIB_EXPORT void qtest_qParseArgs(int argc, char *argv[], bool qml) {
    qtest_qParseArgs(argc, const_cast<const char *const *>(argv), qml);
}

QBenchmarkResult qMedian(const QVector<QBenchmarkResult> &container)
{
    const int count = container.count();
    if (count == 0)
        return QBenchmarkResult();

    if (count == 1)
        return container.front();

    QVector<QBenchmarkResult> containerCopy = container;
    std::sort(containerCopy.begin(), containerCopy.end());

    const int middle = count / 2;

    // ### handle even-sized containers here by doing an aritmetic mean of the two middle items.
    return containerCopy.at(middle);
}

struct QTestDataSetter
{
    QTestDataSetter(QTestData *data)
    {
        QTestResult::setCurrentTestData(data);
    }
    ~QTestDataSetter()
    {
        QTestResult::setCurrentTestData(nullptr);
    }
};

namespace {

qreal addResult(qreal current, const QBenchmarkResult& r)
{
    return current + r.value;
}

}

void TestMethods::invokeTestOnData(int index) const
{
    /* Benchmarking: for each median iteration*/

    bool isBenchmark = false;
    int i = (QBenchmarkGlobalData::current->measurer->needsWarmupIteration()) ? -1 : 0;

    QVector<QBenchmarkResult> results;
    bool minimumTotalReached = false;
    do {
        QBenchmarkTestMethodData::current->beginDataRun();

        /* Benchmarking: for each accumulation iteration*/
        bool invokeOk;
        do {
            if (m_initMethod.isValid())
                m_initMethod.invoke(QTest::currentTestObject, Qt::DirectConnection);
            if (QTestResult::skipCurrentTest() || QTestResult::currentTestFailed())
                break;

            QBenchmarkTestMethodData::current->result = QBenchmarkResult();
            QBenchmarkTestMethodData::current->resultAccepted = false;

            QBenchmarkGlobalData::current->context.tag =
                QLatin1String(
                    QTestResult::currentDataTag()
                    ? QTestResult::currentDataTag() : "");

            invokeOk = m_methods[index].invoke(QTest::currentTestObject, Qt::DirectConnection);
            if (!invokeOk)
                QTestResult::addFailure("Unable to execute slot", __FILE__, __LINE__);

            isBenchmark = QBenchmarkTestMethodData::current->isBenchmark();

            QTestResult::finishedCurrentTestData();

            if (m_cleanupMethod.isValid())
                m_cleanupMethod.invoke(QTest::currentTestObject, Qt::DirectConnection);

            // Process any deleteLater(), like event-loop based apps would do. Fixes memleak reports.
            if (QCoreApplication::instance())
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

            // If the test isn't a benchmark, finalize the result after cleanup() has finished.
            if (!isBenchmark)
                QTestResult::finishedCurrentTestDataCleanup();

            // If this test method has a benchmark, repeat until all measurements are
            // acceptable.
            // The QBENCHMARK macro increases the number of iterations for each run until
            // this happens.
        } while (invokeOk && isBenchmark
                 && QBenchmarkTestMethodData::current->resultsAccepted() == false
                 && !QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed());

        QBenchmarkTestMethodData::current->endDataRun();
        if (!QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed()) {
            if (i > -1)  // iteration -1 is the warmup iteration.
                results.append(QBenchmarkTestMethodData::current->result);

            if (isBenchmark && QBenchmarkGlobalData::current->verboseOutput) {
                if (i == -1) {
                    QTestLog::info(qPrintable(
                        QString::fromLatin1("warmup stage result      : %1")
                            .arg(QBenchmarkTestMethodData::current->result.value)), nullptr, 0);
                } else {
                    QTestLog::info(qPrintable(
                        QString::fromLatin1("accumulation stage result: %1")
                            .arg(QBenchmarkTestMethodData::current->result.value)), nullptr, 0);
                }
            }
        }

        // Verify if the minimum total measurement is reached, if it was specified:
        if (QBenchmarkGlobalData::current->minimumTotal == -1) {
            minimumTotalReached = true;
        } else {
            const qreal total = std::accumulate(results.begin(), results.end(), 0.0, addResult);
            minimumTotalReached = (total >= QBenchmarkGlobalData::current->minimumTotal);
        }
    } while (isBenchmark
             && ((++i < QBenchmarkGlobalData::current->adjustMedianIterationCount()) || !minimumTotalReached)
             && !QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed());

    // If the test is a benchmark, finalize the result after all iterations have finished.
    if (isBenchmark) {
        bool testPassed = !QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed();
        QTestResult::finishedCurrentTestDataCleanup();
        // Only report benchmark figures if the test passed
        if (testPassed && QBenchmarkTestMethodData::current->resultsAccepted())
            QTestLog::addBenchmarkResult(qMedian(results));
    }
}

#if QT_CONFIG(thread)

class WatchDog : public QThread
{
    enum Expectation {
        ThreadStart,
        TestFunctionStart,
        TestFunctionEnd,
        ThreadEnd,
    };

    bool waitFor(std::unique_lock<QtPrivate::mutex> &m, Expectation e) {
        auto expectationChanged = [this, e] { return expecting != e; };
        switch (e) {
        case TestFunctionEnd:
            return waitCondition.wait_for(m, defaultTimeout(), expectationChanged);
        case ThreadStart:
        case ThreadEnd:
        case TestFunctionStart:
            waitCondition.wait(m, expectationChanged);
            return true;
        }
        Q_UNREACHABLE();
        return false;
    }

public:
    WatchDog()
    {
        auto locker = qt_unique_lock(mutex);
        expecting = ThreadStart;
        start();
        waitFor(locker, ThreadStart);
    }
    ~WatchDog() {
        {
            const auto locker = qt_scoped_lock(mutex);
            expecting = ThreadEnd;
            waitCondition.notify_all();
        }
        wait();
    }

    void beginTest() {
        const auto locker = qt_scoped_lock(mutex);
        expecting = TestFunctionEnd;
        waitCondition.notify_all();
    }

    void testFinished() {
        const auto locker = qt_scoped_lock(mutex);
        expecting = TestFunctionStart;
        waitCondition.notify_all();
    }

    void run() override {
        auto locker = qt_unique_lock(mutex);
        expecting = TestFunctionStart;
        waitCondition.notify_all();
        while (true) {
            switch (expecting) {
            case ThreadEnd:
                return;
            case ThreadStart:
                Q_UNREACHABLE();
            case TestFunctionStart:
            case TestFunctionEnd:
                if (Q_UNLIKELY(!waitFor(locker, expecting))) {
                    stackTrace();
                    qFatal("Test function timed out");
                }
            }
        }
    }

private:
    QtPrivate::mutex mutex;
    QtPrivate::condition_variable waitCondition;
    Expectation expecting;
};

#else // !QT_CONFIG(thread)

class WatchDog : public QObject
{
public:
    void beginTest() {};
    void testFinished() {};
};

#endif


/*!
    \internal

    Call slot_data(), init(), slot(), cleanup(), init(), slot(), cleanup(), ...
    If data is set then it is the only test that is performed

    If the function was successfully called, true is returned, otherwise
    false.
 */
bool TestMethods::invokeTest(int index, const char *data, WatchDog *watchDog) const
{
    QBenchmarkTestMethodData benchmarkData;
    QBenchmarkTestMethodData::current = &benchmarkData;

    const QByteArray &name = m_methods[index].name();
    QBenchmarkGlobalData::current->context.slotName = QLatin1String(name) + QLatin1String("()");

    char member[512];
    QTestTable table;

    QTestResult::setCurrentTestFunction(name.constData());

    const QTestTable *gTable = QTestTable::globalTestTable();
    const int globalDataCount = gTable->dataCount();
    int curGlobalDataIndex = 0;

    /* For each entry in the global data table, do: */
    do {
        if (!gTable->isEmpty())
            QTestResult::setCurrentGlobalTestData(gTable->testData(curGlobalDataIndex));

        if (curGlobalDataIndex == 0) {
            qsnprintf(member, 512, "%s_data()", name.constData());
            invokeMethod(QTest::currentTestObject, member);
            if (QTestResult::skipCurrentTest())
                break;
        }

        bool foundFunction = false;
        int curDataIndex = 0;
        const int dataCount = table.dataCount();

        // Data tag requested but none available?
        if (data && !dataCount) {
            // Let empty data tag through.
            if (!*data)
                data = nullptr;
            else {
                fprintf(stderr, "Unknown testdata for function %s(): '%s'\n", name.constData(), data);
                fprintf(stderr, "Function has no testdata.\n");
                return false;
            }
        }

        /* For each entry in this test's data table, do: */
        do {
            QTestResult::setSkipCurrentTest(false);
            QTestResult::setBlacklistCurrentTest(false);
            if (!data || !qstrcmp(data, table.testData(curDataIndex)->dataTag())) {
                foundFunction = true;

                QTestPrivate::checkBlackLists(name.constData(), dataCount ? table.testData(curDataIndex)->dataTag() : nullptr);

                QTestDataSetter s(curDataIndex >= dataCount ? nullptr : table.testData(curDataIndex));

                QTestPrivate::qtestMouseButtons = Qt::NoButton;
                if (watchDog)
                    watchDog->beginTest();
                QTest::lastMouseTimestamp += 500;   // Maintain at least 500ms mouse event timestamps between each test function call
                invokeTestOnData(index);
                if (watchDog)
                    watchDog->testFinished();

                if (data)
                    break;
            }
            ++curDataIndex;
        } while (curDataIndex < dataCount);

        if (data && !foundFunction) {
            fprintf(stderr, "Unknown testdata for function %s: '%s()'\n", name.constData(), data);
            fprintf(stderr, "Available testdata:\n");
            for (int i = 0; i < table.dataCount(); ++i)
                fprintf(stderr, "%s\n", table.testData(i)->dataTag());
            return false;
        }

        QTestResult::setCurrentGlobalTestData(nullptr);
        ++curGlobalDataIndex;
    } while (curGlobalDataIndex < globalDataCount);

    QTestResult::finishedCurrentTestFunction();
    QTestResult::setSkipCurrentTest(false);
    QTestResult::setBlacklistCurrentTest(false);
    QTestResult::setCurrentTestData(nullptr);

    return true;
}

void *fetchData(QTestData *data, const char *tagName, int typeId)
{
    QTEST_ASSERT(typeId);
    QTEST_ASSERT_X(data, "QTest::fetchData()", "Test data requested, but no testdata available.");
    QTEST_ASSERT(data->parent());

    int idx = data->parent()->indexOf(tagName);

    if (Q_UNLIKELY(idx == -1 || idx >= data->dataCount())) {
        qFatal("QFETCH: Requested testdata '%s' not available, check your _data function.",
                tagName);
    }

    if (Q_UNLIKELY(typeId != data->parent()->elementTypeId(idx))) {
        qFatal("Requested type '%s' does not match available type '%s'.",
               QMetaType::typeName(typeId),
               QMetaType::typeName(data->parent()->elementTypeId(idx)));
    }

    return data->data(idx);
}

/*!
 * \internal
 */
char *formatString(const char *prefix, const char *suffix, size_t numArguments, ...)
{
    va_list ap;
    va_start(ap, numArguments);

    QByteArray arguments;
    arguments += prefix;

    if (numArguments > 0) {
        arguments += va_arg(ap, const char *);

        for (size_t i = 1; i < numArguments; ++i) {
            arguments += ", ";
            arguments += va_arg(ap, const char *);
        }
    }

    va_end(ap);
    arguments += suffix;
    return qstrdup(arguments.constData());
}

/*!
  \fn char* QTest::toHexRepresentation(const char *ba, int length)

  Returns a pointer to a string that is the string \a ba represented
  as a space-separated sequence of hex characters. If the input is
  considered too long, it is truncated. A trucation is indicated in
  the returned string as an ellipsis at the end. The caller has
  ownership of the returned pointer and must ensure it is later passed
  to operator delete[].

  \a length is the length of the string \a ba.
 */
char *toHexRepresentation(const char *ba, int length)
{
    if (length == 0)
        return qstrdup("");

    /* We output at maximum about maxLen characters in order to avoid
     * running out of memory and flooding things when the byte array
     * is large.
     *
     * maxLen can't be for example 200 because Qt Test is sprinkled with fixed
     * size char arrays.
     * */
    const int maxLen = 50;
    const int len = qMin(maxLen, length);
    char *result = nullptr;

    if (length > maxLen) {
        const int size = len * 3 + 4;
        result = new char[size];

        char *const forElipsis = result + size - 5;
        forElipsis[0] = ' ';
        forElipsis[1] = '.';
        forElipsis[2] = '.';
        forElipsis[3] = '.';
        result[size - 1] = '\0';
    }
    else {
        const int size = len * 3;
        result = new char[size];
        result[size - 1] = '\0';
    }

    int i = 0;
    int o = 0;

    while (true) {
        const char at = ba[i];

        result[o] = toHexUpper(at >> 4);
        ++o;
        result[o] = toHexUpper(at);

        ++i;
        ++o;
        if (i == len)
            break;
        result[o] = ' ';
        ++o;
    }

    return result;
}

/*!
    \internal
    Returns the same QByteArray but with only the ASCII characters still shown;
    everything else is replaced with \c {\xHH}.
*/
char *toPrettyCString(const char *p, int length)
{
    bool trimmed = false;
    QScopedArrayPointer<char> buffer(new char[256]);
    const char *end = p + length;
    char *dst = buffer.data();

    bool lastWasHexEscape = false;
    *dst++ = '"';
    for ( ; p != end; ++p) {
        // we can add:
        //  1 byte: a single character
        //  2 bytes: a simple escape sequence (\n)
        //  3 bytes: "" and a character
        //  4 bytes: an hex escape sequence (\xHH)
        if (dst - buffer.data() > 246) {
            // plus the quote, the three dots and NUL, it's 255 in the worst case
            trimmed = true;
            break;
        }

        // check if we need to insert "" to break an hex escape sequence
        if (Q_UNLIKELY(lastWasHexEscape)) {
            if (fromHex(*p) != -1) {
                // yes, insert it
                *dst++ = '"';
                *dst++ = '"';
            }
            lastWasHexEscape = false;
        }

        if (*p < 0x7f && *p >= 0x20 && *p != '\\' && *p != '"') {
            *dst++ = *p;
            continue;
        }

        // write as an escape sequence
        // this means we may advance dst to buffer.data() + 247 or 250
        *dst++ = '\\';
        switch (*p) {
        case 0x5c:
        case 0x22:
            *dst++ = uchar(*p);
            break;
        case 0x8:
            *dst++ = 'b';
            break;
        case 0xc:
            *dst++ = 'f';
            break;
        case 0xa:
            *dst++ = 'n';
            break;
        case 0xd:
            *dst++ = 'r';
            break;
        case 0x9:
            *dst++ = 't';
            break;
        default:
            // print as hex escape
            *dst++ = 'x';
            *dst++ = toHexUpper(uchar(*p) >> 4);
            *dst++ = toHexUpper(uchar(*p));
            lastWasHexEscape = true;
            break;
        }
    }

    *dst++ = '"';
    if (trimmed) {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = '\0';
    return buffer.take();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// this used to be the signature up to and including Qt 5.9
// keep it for BC reasons:
Q_TESTLIB_EXPORT
char *toPrettyUnicode(const ushort *p, int length)
{
    return toPrettyUnicode(QStringView(p, length));
}
#endif

/*!
    \internal
    Returns the same QString but with only the ASCII characters still shown;
    everything else is replaced with \c {\uXXXX}.

    Similar to QDebug::putString().
*/
char *toPrettyUnicode(QStringView string)
{
    auto p = reinterpret_cast<const ushort *>(string.utf16());
    auto length = string.size();
    // keep it simple for the vast majority of cases
    bool trimmed = false;
    QScopedArrayPointer<char> buffer(new char[256]);
    const ushort *end = p + length;
    char *dst = buffer.data();

    *dst++ = '"';
    for ( ; p != end; ++p) {
        if (dst - buffer.data() > 245) {
            // plus the quote, the three dots and NUL, it's 250, 251 or 255
            trimmed = true;
            break;
        }

        if (*p < 0x7f && *p >= 0x20 && *p != '\\' && *p != '"') {
            *dst++ = *p;
            continue;
        }

        // write as an escape sequence
        // this means we may advance dst to buffer.data() + 246 or 250
        *dst++ = '\\';
        switch (*p) {
        case 0x22:
        case 0x5c:
            *dst++ = uchar(*p);
            break;
        case 0x8:
            *dst++ = 'b';
            break;
        case 0xc:
            *dst++ = 'f';
            break;
        case 0xa:
            *dst++ = 'n';
            break;
        case 0xd:
            *dst++ = 'r';
            break;
        case 0x9:
            *dst++ = 't';
            break;
        default:
            *dst++ = 'u';
            *dst++ = toHexUpper(*p >> 12);
            *dst++ = toHexUpper(*p >> 8);
            *dst++ = toHexUpper(*p >> 4);
            *dst++ = toHexUpper(*p);
        }
    }

    *dst++ = '"';
    if (trimmed) {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = '\0';
    return buffer.take();
}

void TestMethods::invokeTests(QObject *testObject) const
{
    const QMetaObject *metaObject = testObject->metaObject();
    QTEST_ASSERT(metaObject);
    QTestResult::setCurrentTestFunction("initTestCase");
    if (m_initTestCaseDataMethod.isValid())
        m_initTestCaseDataMethod.invoke(testObject, Qt::DirectConnection);

    QScopedPointer<WatchDog> watchDog;
    if (!debuggerPresent()
#if QT_CONFIG(valgrind)
        && QBenchmarkGlobalData::current->mode() != QBenchmarkGlobalData::CallgrindChildProcess
#endif
       ) {
        watchDog.reset(new WatchDog);
    }

    if (!QTestResult::skipCurrentTest() && !QTest::currentTestFailed()) {
        if (m_initTestCaseMethod.isValid())
            m_initTestCaseMethod.invoke(testObject, Qt::DirectConnection);

        // finishedCurrentTestDataCleanup() resets QTestResult::currentTestFailed(), so use a local copy.
        const bool previousFailed = QTestResult::currentTestFailed();
        QTestResult::finishedCurrentTestData();
        QTestResult::finishedCurrentTestDataCleanup();
        QTestResult::finishedCurrentTestFunction();

        if (!QTestResult::skipCurrentTest() && !previousFailed) {
            for (int i = 0, count = int(m_methods.size()); i < count; ++i) {
                const char *data = nullptr;
                if (i < QTest::testTags.size() && !QTest::testTags.at(i).isEmpty())
                    data = qstrdup(QTest::testTags.at(i).toLatin1().constData());
                const bool ok = invokeTest(i, data, watchDog.data());
                delete [] data;
                if (!ok)
                    break;
            }
        }

        QTestResult::setSkipCurrentTest(false);
        QTestResult::setBlacklistCurrentTest(false);
        QTestResult::setCurrentTestFunction("cleanupTestCase");
        if (m_cleanupTestCaseMethod.isValid())
            m_cleanupTestCaseMethod.invoke(testObject, Qt::DirectConnection);
        QTestResult::finishedCurrentTestData();
        QTestResult::finishedCurrentTestDataCleanup();
    }
    QTestResult::finishedCurrentTestFunction();
    QTestResult::setCurrentTestFunction(nullptr);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)

// Helper class for resolving symbol names by dynamically loading "dbghelp.dll".
class DebugSymbolResolver
{
    Q_DISABLE_COPY_MOVE(DebugSymbolResolver)
public:
    struct Symbol {
        Symbol() : name(nullptr), address(0) {}

        const char *name; // Must be freed by caller.
        DWORD64 address;
    };

    explicit DebugSymbolResolver(HANDLE process);
    ~DebugSymbolResolver() { cleanup(); }

    bool isValid() const { return m_symFromAddr; }

    Symbol resolveSymbol(DWORD64 address) const;

private:
    // typedefs from DbgHelp.h/.dll
    struct DBGHELP_SYMBOL_INFO { // SYMBOL_INFO
        ULONG       SizeOfStruct;
        ULONG       TypeIndex;        // Type Index of symbol
        ULONG64     Reserved[2];
        ULONG       Index;
        ULONG       Size;
        ULONG64     ModBase;          // Base Address of module comtaining this symbol
        ULONG       Flags;
        ULONG64     Value;            // Value of symbol, ValuePresent should be 1
        ULONG64     Address;          // Address of symbol including base address of module
        ULONG       Register;         // register holding value or pointer to value
        ULONG       Scope;            // scope of the symbol
        ULONG       Tag;              // pdb classification
        ULONG       NameLen;          // Actual length of name
        ULONG       MaxNameLen;
        CHAR        Name[1];          // Name of symbol
    };

    typedef BOOL (__stdcall *SymInitializeType)(HANDLE, PCSTR, BOOL);
    typedef BOOL (__stdcall *SymFromAddrType)(HANDLE, DWORD64, PDWORD64, DBGHELP_SYMBOL_INFO *);

    void cleanup();

    const HANDLE m_process;
    HMODULE m_dbgHelpLib;
    SymFromAddrType m_symFromAddr;
};

void DebugSymbolResolver::cleanup()
{
    if (m_dbgHelpLib)
        FreeLibrary(m_dbgHelpLib);
    m_dbgHelpLib = 0;
    m_symFromAddr = nullptr;
}

DebugSymbolResolver::DebugSymbolResolver(HANDLE process)
    : m_process(process), m_dbgHelpLib(0), m_symFromAddr(nullptr)
{
    bool success = false;
    m_dbgHelpLib = LoadLibraryW(L"dbghelp.dll");
    if (m_dbgHelpLib) {
        SymInitializeType symInitialize = reinterpret_cast<SymInitializeType>(
            reinterpret_cast<QFunctionPointer>(GetProcAddress(m_dbgHelpLib, "SymInitialize")));
        m_symFromAddr = reinterpret_cast<SymFromAddrType>(
            reinterpret_cast<QFunctionPointer>(GetProcAddress(m_dbgHelpLib, "SymFromAddr")));
        success = symInitialize && m_symFromAddr && symInitialize(process, NULL, TRUE);
    }
    if (!success)
        cleanup();
}

DebugSymbolResolver::Symbol DebugSymbolResolver::resolveSymbol(DWORD64 address) const
{
    // reserve additional buffer where SymFromAddr() will store the name
    struct NamedSymbolInfo : public DBGHELP_SYMBOL_INFO {
        enum { symbolNameLength = 255 };

        char name[symbolNameLength + 1];
    };

    Symbol result;
    if (!isValid())
        return result;
    NamedSymbolInfo symbolBuffer;
    memset(&symbolBuffer, 0, sizeof(NamedSymbolInfo));
    symbolBuffer.MaxNameLen = NamedSymbolInfo::symbolNameLength;
    symbolBuffer.SizeOfStruct = sizeof(DBGHELP_SYMBOL_INFO);
    if (!m_symFromAddr(m_process, address, 0, &symbolBuffer))
        return result;
    result.name = qstrdup(symbolBuffer.Name);
    result.address = symbolBuffer.Address;
    return result;
}

#endif // Q_OS_WIN && !Q_OS_WINRT

class FatalSignalHandler
{
public:
    FatalSignalHandler()
    {
#if defined(Q_OS_WIN)
#  if !defined(Q_CC_MINGW)
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#  endif
#  if !defined(Q_OS_WINRT)
        SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
        SetUnhandledExceptionFilter(windowsFaultHandler);
#  endif
#elif defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
        sigemptyset(&handledSignals);

        const int fatalSignals[] = {
             SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGPIPE, SIGTERM, 0 };

        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = FatalSignalHandler::signal;

        // Remove the handler after it is invoked.
#  if !defined(Q_OS_INTEGRITY)
        act.sa_flags = SA_RESETHAND;
#  endif

    // tvOS/watchOS both define SA_ONSTACK (in sys/signal.h) but mark sigaltstack() as
    // unavailable (__WATCHOS_PROHIBITED __TVOS_PROHIBITED in signal.h)
#  if defined(SA_ONSTACK) && !defined(Q_OS_TVOS) && !defined(Q_OS_WATCHOS)
        // Let the signal handlers use an alternate stack
        // This is necessary if SIGSEGV is to catch a stack overflow
#    if defined(Q_CC_GNU) && defined(Q_OF_ELF)
        // Put the alternate stack in the .lbss (large BSS) section so that it doesn't
        // interfere with normal .bss symbols
        __attribute__((section(".lbss.altstack"), aligned(4096)))
#    endif
        static char alternate_stack[16 * 1024];
        stack_t stack;
        stack.ss_flags = 0;
        stack.ss_size = sizeof alternate_stack;
        stack.ss_sp = alternate_stack;
        sigaltstack(&stack, nullptr);
        act.sa_flags |= SA_ONSTACK;
#  endif

        // Block all fatal signals in our signal handler so we don't try to close
        // the testlog twice.
        sigemptyset(&act.sa_mask);
        for (int i = 0; fatalSignals[i]; ++i)
            sigaddset(&act.sa_mask, fatalSignals[i]);

        struct sigaction oldact;

        for (int i = 0; fatalSignals[i]; ++i) {
            sigaction(fatalSignals[i], &act, &oldact);
            if (
#  ifdef SA_SIGINFO
                oldact.sa_flags & SA_SIGINFO ||
#  endif
                oldact.sa_handler != SIG_DFL) {
                sigaction(fatalSignals[i], &oldact, nullptr);
            } else
            {
                sigaddset(&handledSignals, fatalSignals[i]);
            }
        }
#endif // defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
    }

    ~FatalSignalHandler()
    {
#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
        // Unregister any of our remaining signal handlers
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = SIG_DFL;

        struct sigaction oldact;

        for (int i = 1; i < 32; ++i) {
            if (!sigismember(&handledSignals, i))
                continue;
            sigaction(i, &act, &oldact);

            // If someone overwrote it in the mean time, put it back
            if (oldact.sa_handler != FatalSignalHandler::signal)
                sigaction(i, &oldact, nullptr);
        }
#endif
    }

private:
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    static LONG WINAPI windowsFaultHandler(struct _EXCEPTION_POINTERS *exInfo)
    {
        enum { maxStackFrames = 100 };
        char appName[MAX_PATH];
        if (!GetModuleFileNameA(NULL, appName, MAX_PATH))
            appName[0] = 0;
        const int msecsFunctionTime = qRound(QTestLog::msecsFunctionTime());
        const int msecsTotalTime = qRound(QTestLog::msecsTotalTime());
        const void *exceptionAddress = exInfo->ExceptionRecord->ExceptionAddress;
        printf("A crash occurred in %s.\n"
               "Function time: %dms Total time: %dms\n\n"
               "Exception address: 0x%p\n"
               "Exception code   : 0x%lx\n",
               appName, msecsFunctionTime, msecsTotalTime,
               exceptionAddress, exInfo->ExceptionRecord->ExceptionCode);

        DebugSymbolResolver resolver(GetCurrentProcess());
        if (resolver.isValid()) {
            DebugSymbolResolver::Symbol exceptionSymbol = resolver.resolveSymbol(DWORD64(exceptionAddress));
            if (exceptionSymbol.name) {
                printf("Nearby symbol    : %s\n", exceptionSymbol.name);
                delete [] exceptionSymbol.name;
            }
            void *stack[maxStackFrames];
            fputs("\nStack:\n", stdout);
            const unsigned frameCount = CaptureStackBackTrace(0, DWORD(maxStackFrames), stack, NULL);
            for (unsigned f = 0; f < frameCount; ++f)     {
                DebugSymbolResolver::Symbol symbol = resolver.resolveSymbol(DWORD64(stack[f]));
                if (symbol.name) {
                    printf("#%3u: %s() - 0x%p\n", f + 1, symbol.name, (const void *)symbol.address);
                    delete [] symbol.name;
                } else {
                    printf("#%3u: Unable to obtain symbol\n", f + 1);
                }
            }
        }

        fputc('\n', stdout);
        fflush(stdout);

        return EXCEPTION_EXECUTE_HANDLER;
    }
#endif // defined(Q_OS_WIN) && !defined(Q_OS_WINRT)

#if defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
    static void signal(int signum)
    {
        const int msecsFunctionTime = qRound(QTestLog::msecsFunctionTime());
        const int msecsTotalTime = qRound(QTestLog::msecsTotalTime());
        if (signum != SIGINT) {
            stackTrace();
            if (qEnvironmentVariableIsSet("QTEST_PAUSE_ON_CRASH")) {
                fprintf(stderr, "Pausing process %d for debugging\n", getpid());
                raise(SIGSTOP);
            }
        }
        qFatal("Received signal %d\n"
               "         Function time: %dms Total time: %dms",
               signum, msecsFunctionTime, msecsTotalTime);
#  if defined(Q_OS_INTEGRITY)
        {
            struct sigaction act;
            memset(&act, 0, sizeof(struct sigaction));
            act.sa_handler = SIG_DFL;
            sigaction(signum, &act, NULL);
        }
#  endif
    }

    sigset_t handledSignals;
#endif // defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
};

} // namespace

static void initEnvironment()
{
    qputenv("QT_QTESTLIB_RUNNING", "1");
}

/*!
    Executes tests declared in \a testObject. In addition, the private slots
    \c{initTestCase()}, \c{cleanupTestCase()}, \c{init()} and \c{cleanup()}
    are executed if they exist. See \l{Creating a Test} for more details.

    Optionally, the command line arguments \a argc and \a argv can be provided.
    For a list of recognized arguments, read \l {Qt Test Command Line Arguments}.

    The following example will run all tests in \c MyTestObject:

    \snippet code/src_qtestlib_qtestcase.cpp 18

    This function returns 0 if no tests failed, or a value other than 0 if one
    or more tests failed or in case of unhandled exceptions.  (Skipped tests do
    not influence the return value.)

    For stand-alone test applications, the convenience macro \l QTEST_MAIN() can
    be used to declare a main() function that parses the command line arguments
    and executes the tests, avoiding the need to call this function explicitly.

    The return value from this function is also the exit code of the test
    application when the \l QTEST_MAIN() macro is used.

    For stand-alone test applications, this function should not be called more
    than once, as command-line options for logging test output to files and
    executing individual test functions will not behave correctly.

    Note: This function is not reentrant, only one test can run at a time. A
    test that was executed with qExec() can't run another test via qExec() and
    threads are not allowed to call qExec() simultaneously.

    If you have programatically created the arguments, as opposed to getting them
    from the arguments in \c main(), it is likely of interest to use
    QTest::qExec(QObject *, const QStringList &) since it is Unicode safe.

    \sa QTEST_MAIN()
*/

int QTest::qExec(QObject *testObject, int argc, char **argv)
{
    qInit(testObject, argc, argv);
    int ret = qRun();
    qCleanup();
    return ret;
}

/*! \internal
 */
void QTest::qInit(QObject *testObject, int argc, char **argv)
{
    initEnvironment();
    QBenchmarkGlobalData::current = new QBenchmarkGlobalData;

#if defined(Q_OS_MACOS)
    // Don't restore saved window state for auto tests
    QTestPrivate::disableWindowRestore();

    // Disable App Nap which may cause tests to stall
    QTestPrivate::AppNapDisabler appNapDisabler;

    if (qApp && (qstrcmp(qApp->metaObject()->className(), "QApplication") == 0)) {
        IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
            kIOPMAssertionLevelOn, CFSTR("QtTest running tests"),
            &macPowerSavingDisabled);
    }
#endif

    QTestPrivate::parseBlackList();
    QTestResult::reset();

    QTEST_ASSERT(testObject);
    QTEST_ASSERT(!currentTestObject);
    currentTestObject = testObject;

    const QMetaObject *metaObject = testObject->metaObject();
    QTEST_ASSERT(metaObject);

    QTestResult::setCurrentTestObject(metaObject->className());
    if (argc > 0)
        QTestResult::setCurrentAppName(argv[0]);

    qtest_qParseArgs(argc, argv, false);

    QTestTable::globalTestTable();
    QTestLog::startLogging();
}

/*! \internal
 */
int QTest::qRun()
{
    QTEST_ASSERT(currentTestObject);

#if QT_CONFIG(valgrind)
    int callgrindChildExitCode = 0;
#endif

#ifndef QT_NO_EXCEPTIONS
    try {
#endif

#if QT_CONFIG(valgrind)
    if (QBenchmarkGlobalData::current->mode() == QBenchmarkGlobalData::CallgrindParentProcess) {
        if (Q_UNLIKELY(!qApp))
            qFatal("QtTest: -callgrind option is not available with QTEST_APPLESS_MAIN");

        const QStringList origAppArgs(QCoreApplication::arguments());
        if (!QBenchmarkValgrindUtils::rerunThroughCallgrind(origAppArgs, callgrindChildExitCode))
            return -1;

        QBenchmarkValgrindUtils::cleanup();

    } else
#endif
    {
        QScopedPointer<FatalSignalHandler> handler;
        if (!noCrashHandler)
            handler.reset(new FatalSignalHandler);

        TestMethods::MetaMethods commandLineMethods;
        for (const QString &tf : qAsConst(QTest::testFunctions)) {
                const QByteArray tfB = tf.toLatin1();
                const QByteArray signature = tfB + QByteArrayLiteral("()");
                QMetaMethod m = TestMethods::findMethod(currentTestObject, signature.constData());
                if (!m.isValid() || !isValidSlot(m)) {
                    fprintf(stderr, "Unknown test function: '%s'. Possible matches:\n", tfB.constData());
                    qPrintTestSlots(stderr, tfB.constData());
                    fprintf(stderr, "\n%s -functions\nlists all available test functions.\n", QTestResult::currentAppName());
                    exit(1);
                }
                commandLineMethods.push_back(m);
        }
        TestMethods test(currentTestObject, commandLineMethods);
        test.invokeTests(currentTestObject);
    }

#ifndef QT_NO_EXCEPTIONS
    } catch (...) {
        QTestResult::addFailure("Caught unhandled exception", __FILE__, __LINE__);
        if (QTestResult::currentTestFunction()) {
            QTestResult::finishedCurrentTestFunction();
            QTestResult::setCurrentTestFunction(nullptr);
        }

        qCleanup();

        // Re-throw exception to make debugging easier
        throw;
        return 1;
    }
#endif

#if QT_CONFIG(valgrind)
    if (QBenchmarkGlobalData::current->mode() == QBenchmarkGlobalData::CallgrindParentProcess)
        return callgrindChildExitCode;
#endif
    // make sure our exit code is never going above 127
    // since that could wrap and indicate 0 test fails
    return qMin(QTestLog::failCount(), 127);
}

/*! \internal
 */
void QTest::qCleanup()
{
    currentTestObject = nullptr;

    QTestTable::clearGlobalTestTable();
    QTestLog::stopLogging();

    delete QBenchmarkGlobalData::current;
    QBenchmarkGlobalData::current = nullptr;

    QSignalDumper::endDump();

#if defined(Q_OS_MACOS)
    IOPMAssertionRelease(macPowerSavingDisabled);
#endif
}

/*!
  \overload
  \since 4.4

  Behaves identically to qExec(QObject *, int, char**) but takes a
  QStringList of \a arguments instead of a \c char** list.
 */
int QTest::qExec(QObject *testObject, const QStringList &arguments)
{
    const int argc = arguments.count();
    QVarLengthArray<char *> argv(argc);

    QVector<QByteArray> args;
    args.reserve(argc);

    for (int i = 0; i < argc; ++i)
    {
        args.append(arguments.at(i).toLocal8Bit().constData());
        argv[i] = args.last().data();
    }

    return qExec(testObject, argc, argv.data());
}

/*! \internal
 */
void QTest::qFail(const char *statementStr, const char *file, int line)
{
    QTestResult::addFailure(statementStr, file, line);
}

/*! \internal
 */
bool QTest::qVerify(bool statement, const char *statementStr, const char *description,
                   const char *file, int line)
{
    return QTestResult::verify(statement, statementStr, description, file, line);
}

/*! \fn void QTest::qSkip(const char *message, const char *file, int line)
\internal
 */
void QTest::qSkip(const char *message, const char *file, int line)
{
    QTestResult::addSkip(message, file, line);
    QTestResult::setSkipCurrentTest(true);
}

/*! \fn bool QTest::qExpectFail(const char *dataIndex, const char *comment, TestFailMode mode, const char *file, int line)
\internal
 */
bool QTest::qExpectFail(const char *dataIndex, const char *comment,
                       QTest::TestFailMode mode, const char *file, int line)
{
    return QTestResult::expectFail(dataIndex, qstrdup(comment), mode, file, line);
}

/*! \internal
 */
void QTest::qWarn(const char *message, const char *file, int line)
{
    QTestLog::warn(message, file, line);
}

/*!
    Ignores messages created by qDebug(), qInfo() or qWarning(). If the \a message
    with the corresponding \a type is outputted, it will be removed from the
    test log. If the test finished and the \a message was not outputted,
    a test failure is appended to the test log.

    \b {Note:} Invoking this function will only ignore one message.
    If the message you want to ignore is outputted twice, you have to
    call ignoreMessage() twice, too.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 19

    The example above tests that QDir::mkdir() outputs the right warning when invoked
    with an invalid file name.
*/
void QTest::ignoreMessage(QtMsgType type, const char *message)
{
    QTestLog::ignoreMessage(type, message);
}

#if QT_CONFIG(regularexpression)
/*!
    \overload

    Ignores messages created by qDebug(), qInfo() or qWarning(). If the message
    matching \a messagePattern
    with the corresponding \a type is outputted, it will be removed from the
    test log. If the test finished and the message was not outputted,
    a test failure is appended to the test log.

    \b {Note:} Invoking this function will only ignore one message.
    If the message you want to ignore is outputted twice, you have to
    call ignoreMessage() twice, too.

    \since 5.3
*/
void QTest::ignoreMessage(QtMsgType type, const QRegularExpression &messagePattern)
{
    QTestLog::ignoreMessage(type, messagePattern);
}
#endif // QT_CONFIG(regularexpression)

/*! \internal
 */

#ifdef Q_OS_WIN
static inline bool isWindowsBuildDirectory(const QString &dirName)
{
    return dirName.compare(QLatin1String("Debug"), Qt::CaseInsensitive) == 0
           || dirName.compare(QLatin1String("Release"), Qt::CaseInsensitive) == 0;
}
#endif

#if QT_CONFIG(temporaryfile)
/*!
    Extracts a directory from resources to disk. The content is extracted
    recursively to a temporary folder. The extracted content is removed
    automatically once the last reference to the return value goes out of scope.

    \a dirName is the name of the directory to extract from resources.

    Returns the temporary directory where the data was extracted or null in case of
    errors.
 */
QSharedPointer<QTemporaryDir> QTest::qExtractTestData(const QString &dirName)
{
      QSharedPointer<QTemporaryDir> result; // null until success, then == tempDir

      QSharedPointer<QTemporaryDir> tempDir = QSharedPointer<QTemporaryDir>::create();

      tempDir->setAutoRemove(true);

      if (!tempDir->isValid())
          return result;

      const QString dataPath = tempDir->path();
      const QString resourcePath = QLatin1Char(':') + dirName;
      const QFileInfo fileInfo(resourcePath);

      if (!fileInfo.isDir()) {
          qWarning("Resource path '%s' is not a directory.", qPrintable(resourcePath));
          return result;
      }

      QDirIterator it(resourcePath, QDirIterator::Subdirectories);
      if (!it.hasNext()) {
          qWarning("Resource directory '%s' is empty.", qPrintable(resourcePath));
          return result;
      }

      while (it.hasNext()) {
          it.next();

          QFileInfo fileInfo = it.fileInfo();

          if (!fileInfo.isDir()) {
              const QString destination = dataPath + QLatin1Char('/') + fileInfo.filePath().midRef(resourcePath.length());
              QFileInfo destinationFileInfo(destination);
              QDir().mkpath(destinationFileInfo.path());
              if (!QFile::copy(fileInfo.filePath(), destination)) {
                  qWarning("Failed to copy '%s'.", qPrintable(fileInfo.filePath()));
                  return result;
              }
              if (!QFile::setPermissions(destination, QFile::ReadUser | QFile::WriteUser | QFile::ReadGroup)) {
                  qWarning("Failed to set permissions on '%s'.", qPrintable(destination));
                  return result;
              }
          }
      }

      result = std::move(tempDir);

      return result;
}
#endif // QT_CONFIG(temporaryfile)

/*! \internal
 */

QString QTest::qFindTestData(const QString& base, const char *file, int line, const char *builddir)
{
    QString found;

    // Testdata priorities:

    //  1. relative to test binary.
    if (qApp) {
        QDir binDirectory(QCoreApplication::applicationDirPath());
        if (binDirectory.exists(base)) {
            found = binDirectory.absoluteFilePath(base);
        }
#ifdef Q_OS_WIN
        // Windows: The executable is typically located in one of the
        // 'Release' or 'Debug' directories.
        else if (isWindowsBuildDirectory(binDirectory.dirName())
                 && binDirectory.cdUp() && binDirectory.exists(base)) {
            found = binDirectory.absoluteFilePath(base);
        }
#endif // Q_OS_WIN
        else if (QTestLog::verboseLevel() >= 2) {
            const QString candidate = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QLatin1Char('/') + base);
            QTestLog::info(qPrintable(
                QString::fromLatin1("testdata %1 not found relative to test binary [%2]; "
                                    "checking next location").arg(base, candidate)),
                file, line);
        }
    }

    //  2. installed path.
    if (found.isEmpty()) {
        const char *testObjectName = QTestResult::currentTestObjectName();
        if (testObjectName) {
            const QString testsPath = QLibraryInfo::location(QLibraryInfo::TestsPath);
            const QString candidate = QString::fromLatin1("%1/%2/%3")
                .arg(testsPath, QFile::decodeName(testObjectName).toLower(), base);
            if (QFileInfo::exists(candidate)) {
                found = candidate;
            } else if (QTestLog::verboseLevel() >= 2) {
                QTestLog::info(qPrintable(
                    QString::fromLatin1("testdata %1 not found in tests install path [%2]; "
                                        "checking next location")
                        .arg(base, QDir::toNativeSeparators(candidate))),
                    file, line);
            }
        }
    }

    //  3. relative to test source.
    if (found.isEmpty() && qstrncmp(file, ":/", 2) != 0) {
        // srcdir is the directory containing the calling source file.
        QFileInfo srcdir = QFileInfo(QFile::decodeName(file)).path();

        // If the srcdir is relative, that means it is relative to the current working
        // directory of the compiler at compile time, which should be passed in as `builddir'.
        if (!srcdir.isAbsolute() && builddir) {
            srcdir.setFile(QFile::decodeName(builddir) + QLatin1String("/") + srcdir.filePath());
        }

        const QString canonicalPath = srcdir.canonicalFilePath();
        const QString candidate = QString::fromLatin1("%1/%2").arg(canonicalPath, base);
        if (!canonicalPath.isEmpty() && QFileInfo::exists(candidate)) {
            found = candidate;
        } else if (QTestLog::verboseLevel() >= 2) {
            QTestLog::info(qPrintable(
                QString::fromLatin1("testdata %1 not found relative to source path [%2]")
                    .arg(base, QDir::toNativeSeparators(candidate))),
                file, line);
        }
    }

    // 4. Try resources
    if (found.isEmpty()) {
        const QString candidate = QString::fromLatin1(":/%1").arg(base);
        if (QFileInfo::exists(candidate)) {
            found = candidate;
        } else if (QTestLog::verboseLevel() >= 2) {
            QTestLog::info(qPrintable(
                QString::fromLatin1("testdata %1 not found in resources [%2]")
                    .arg(base, QDir::toNativeSeparators(candidate))),
                file, line);
        }
    }

    // 5. Try current directory
    if (found.isEmpty()) {
        const QString candidate = QDir::currentPath() + QLatin1Char('/') + base;
        if (QFileInfo::exists(candidate)) {
            found = candidate;
        } else if (QTestLog::verboseLevel() >= 2) {
            QTestLog::info(qPrintable(
                QString::fromLatin1("testdata %1 not found in current directory [%2]")
                    .arg(base, QDir::toNativeSeparators(candidate))),
                file, line);
        }
    }

    // 6. Try main source directory
    if (found.isEmpty()) {
        const QString candidate = QTest::mainSourcePath % QLatin1Char('/') % base;
        if (QFileInfo::exists(candidate)) {
            found = candidate;
        } else if (QTestLog::verboseLevel() >= 2) {
            QTestLog::info(qPrintable(
                QString::fromLatin1("testdata %1 not found in main source directory [%2]")
                    .arg(base, QDir::toNativeSeparators(candidate))),
                file, line);
        }
    }

    if (found.isEmpty()) {
        QTest::qWarn(qPrintable(
            QString::fromLatin1("testdata %1 could not be located!").arg(base)),
            file, line);
    } else if (QTestLog::verboseLevel() >= 1) {
        QTestLog::info(qPrintable(
            QString::fromLatin1("testdata %1 was located at %2").arg(base, QDir::toNativeSeparators(found))),
            file, line);
    }

    return found;
}

/*! \internal
 */
QString QTest::qFindTestData(const char *base, const char *file, int line, const char *builddir)
{
    return qFindTestData(QFile::decodeName(base), file, line, builddir);
}

/*! \internal
 */
void *QTest::qData(const char *tagName, int typeId)
{
    return fetchData(QTestResult::currentTestData(), tagName, typeId);
}

/*! \internal
 */
void *QTest::qGlobalData(const char *tagName, int typeId)
{
    return fetchData(QTestResult::currentGlobalTestData(), tagName, typeId);
}

/*! \internal
 */
void *QTest::qElementData(const char *tagName, int metaTypeId)
{
    QTEST_ASSERT(tagName);
    QTestData *data = QTestResult::currentTestData();
    QTEST_ASSERT(data);
    QTEST_ASSERT(data->parent());

    int idx = data->parent()->indexOf(tagName);
    QTEST_ASSERT(idx != -1);
    QTEST_ASSERT(data->parent()->elementTypeId(idx) == metaTypeId);

    return data->data(data->parent()->indexOf(tagName));
}

/*! \internal
 */
void QTest::addColumnInternal(int id, const char *name)
{
    QTestTable *tbl = QTestTable::currentTestTable();
    QTEST_ASSERT_X(tbl, "QTest::addColumn()", "Cannot add testdata outside of a _data slot.");

    tbl->addColumn(id, name);
}

/*!
    Appends a new row to the current test data. \a dataTag is the name of
    the testdata that will appear in the test output. Returns a QTestData reference
    that can be used to stream in data.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 20

    \b {Note:} This macro can only be used in a test's data function
    that is invoked by the test framework.

    See \l {Chapter 2: Data Driven Testing}{Data Driven Testing} for
    a more extensive example.

    \sa addColumn(), QFETCH()
*/
QTestData &QTest::newRow(const char *dataTag)
{
    QTEST_ASSERT_X(dataTag, "QTest::newRow()", "Data tag cannot be null");
    QTestTable *tbl = QTestTable::currentTestTable();
    QTEST_ASSERT_X(tbl, "QTest::newRow()", "Cannot add testdata outside of a _data slot.");
    QTEST_ASSERT_X(tbl->elementCount(), "QTest::newRow()", "Must add columns before attempting to add rows.");

    return *tbl->newData(dataTag);
}

/*!
    \since 5.9

    Appends a new row to the current test data. The function's arguments are passed
    to qsnprintf() for formatting according to \a format. See the qvsnprintf()
    documentation for caveats and limitations.

    The formatted string will appear as the name of this test data in the test output.

    Returns a QTestData reference that can be used to stream in data.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp addRow

    \b {Note:} This function can only be used in a test's data function
    that is invoked by the test framework.

    See \l {Chapter 2: Data Driven Testing}{Data Driven Testing} for
    a more extensive example.

    \sa addColumn(), QFETCH()
*/
QTestData &QTest::addRow(const char *format, ...)
{
    QTEST_ASSERT_X(format, "QTest::addRow()", "Format string cannot be null");
    QTestTable *tbl = QTestTable::currentTestTable();
    QTEST_ASSERT_X(tbl, "QTest::addRow()", "Cannot add testdata outside of a _data slot.");
    QTEST_ASSERT_X(tbl->elementCount(), "QTest::addRow()", "Must add columns before attempting to add rows.");

    char buf[1024];

    va_list va;
    va_start(va, format);
    // we don't care about failures, we accept truncation, as well as trailing garbage.
    // Names with more than 1K characters are nonsense, anyway.
    (void)qvsnprintf(buf, sizeof buf, format, va);
    buf[sizeof buf - 1] = '\0';
    va_end(va);

    return *tbl->newData(buf);
}

/*! \fn template <typename T> void QTest::addColumn(const char *name, T *dummy = 0)

    Adds a column with type \c{T} to the current test data.
    \a name is the name of the column. \a dummy is a workaround
    for buggy compilers and can be ignored.

    To populate the column with values, newRow() can be used. Use
    \l QFETCH() to fetch the data in the actual test.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 21

    To add custom types to the testdata, the type must be registered with
    QMetaType via \l Q_DECLARE_METATYPE().

    \b {Note:} This macro can only be used in a test's data function
    that is invoked by the test framework.

    See \l {Chapter 2: Data Driven Testing}{Data Driven Testing} for
    a more extensive example.

    \sa QTest::newRow(), QFETCH(), QMetaType
*/

/*!
    Returns the name of the binary that is currently executed.
*/
const char *QTest::currentAppName()
{
    return QTestResult::currentAppName();
}

/*!
    Returns the name of the test function that is currently executed.

    Example:

    \snippet code/src_qtestlib_qtestcase.cpp 22
*/
const char *QTest::currentTestFunction()
{
    return QTestResult::currentTestFunction();
}

/*!
    Returns the name of the current test data. If the test doesn't
    have any assigned testdata, the function returns 0.
*/
const char *QTest::currentDataTag()
{
    return QTestResult::currentDataTag();
}

/*!
    Returns \c true if the current test function failed, otherwise false.
*/
bool QTest::currentTestFailed()
{
    return QTestResult::currentTestFailed();
}

/*!
    Sleeps for \a ms milliseconds, blocking execution of the
    test. qSleep() will not do any event processing and leave your test
    unresponsive. Network communication might time out while
    sleeping. Use \l {QTest::qWait()} to do non-blocking sleeping.

    \a ms must be greater than 0.

    \b {Note:} The qSleep() function calls either \c nanosleep() on
    unix or \c Sleep() on windows, so the accuracy of time spent in
    qSleep() depends on the operating system.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 23

    \sa {QTest::qWait()}
*/
void QTest::qSleep(int ms)
{
    // ### Qt 6, move to QtCore or remove altogether
    QTEST_ASSERT(ms > 0);
    QTestPrivate::qSleep(ms);
}

/*! \internal
 */
QObject *QTest::testObject()
{
    return currentTestObject;
}

/*! \internal
 */
void QTest::setMainSourcePath(const char *file, const char *builddir)
{
    QString mainSourceFile = QFile::decodeName(file);
    QFileInfo fi;
    if (builddir)
        fi.setFile(QDir(QFile::decodeName(builddir)), mainSourceFile);
    else
        fi.setFile(mainSourceFile);
    QTest::mainSourcePath = fi.absolutePath();
}

/*! \internal
    This function is called by various specializations of QTest::qCompare
    to decide whether to report a failure and to produce verbose test output.

    The failureMsg parameter can be null, in which case a default message
    will be output if the compare fails.  If the compare succeeds, failureMsg
    will not be output.

    If the caller has already passed a failure message showing the compared
    values, or if those values cannot be stringified, val1 and val2 can be null.
 */
bool QTest::compare_helper(bool success, const char *failureMsg,
                           char *val1, char *val2,
                           const char *actual, const char *expected,
                           const char *file, int line)
{
    return QTestResult::compare(success, failureMsg, val1, val2, actual, expected, file, line);
}

template <typename T>
static bool floatingCompare(const T &actual, const T &expected)
{
    switch (qFpClassify(expected))
    {
    case FP_INFINITE:
        return (expected < 0) == (actual < 0) && qFpClassify(actual) == FP_INFINITE;
    case FP_NAN:
        return qFpClassify(actual) == FP_NAN;
    default:
        if (!qFuzzyIsNull(expected))
            return qFuzzyCompare(actual, expected);
        Q_FALLTHROUGH();
    case FP_SUBNORMAL: // subnormal is always fuzzily null
    case FP_ZERO:
        return qFuzzyIsNull(actual);
    }
}

/*! \fn bool QTest::qCompare(const qfloat16 &t1, const qfloat16 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */
bool QTest::qCompare(qfloat16 const &t1, qfloat16 const &t2, const char *actual, const char *expected,
                     const char *file, int line)
{
    return compare_helper(floatingCompare(t1, t2),
                          "Compared qfloat16s are not the same (fuzzy compare)",
                          toString(t1), toString(t2), actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(const float &t1, const float &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */
bool QTest::qCompare(float const &t1, float const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return QTestResult::compare(floatingCompare(t1, t2),
                                "Compared floats are not the same (fuzzy compare)",
                                t1, t2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(const double &t1, const double &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */
bool QTest::qCompare(double const &t1, double const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return QTestResult::compare(floatingCompare(t1, t2),
                                "Compared doubles are not the same (fuzzy compare)",
                                t1, t2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(int t1, int t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */
bool QTest::qCompare(int t1, int t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return QTestResult::compare(t1 == t2,
                                "Compared values are not the same",
                                t1, t2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(unsigned t1, unsigned t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */
bool QTest::qCompare(unsigned t1, unsigned t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return QTestResult::compare(t1 == t2,
                                "Compared values are not the same",
                                t1, t2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(QStringView t1, QStringView t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */
bool QTest::qCompare(QStringView t1, QStringView t2, const char *actual, const char *expected,
                     const char *file, int line)
{
    return QTestResult::compare(t1 == t2,
                                "Compared values are not the same",
                                t1, t2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(QStringView t1, const QLatin1String &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */
bool QTest::qCompare(QStringView t1, const QLatin1String &t2, const char *actual, const char *expected,
                     const char *file, int line)
{
    return QTestResult::compare(t1 == t2,
                                "Compared values are not the same",
                                t1, t2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(const QLatin1String &t1, QStringView t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */
bool QTest::qCompare(const QLatin1String &t1, QStringView t2, const char *actual, const char *expected,
                     const char *file, int line)
{
    return QTestResult::compare(t1 == t2,
                                "Compared values are not the same",
                                t1, t2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(const QString &t1, const QString &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */

/*! \fn bool QTest::qCompare(const QString &t1, const QLatin1String &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */

/*! \fn bool QTest::qCompare(const QLatin1String &t1, const QString &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
    \since 5.14
 */

/*! \fn bool QTest::qCompare(const double &t1, const float &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */

/*! \fn bool QTest::qCompare(const float &t1, const double &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */

#define TO_STRING_IMPL(TYPE, FORMAT) \
template <> Q_TESTLIB_EXPORT char *QTest::toString<TYPE>(const TYPE &t) \
{ \
    char *msg = new char[128]; \
    qsnprintf(msg, 128, #FORMAT, t); \
    return msg; \
}

TO_STRING_IMPL(short, %hd)
TO_STRING_IMPL(ushort, %hu)
TO_STRING_IMPL(int, %d)
TO_STRING_IMPL(uint, %u)
TO_STRING_IMPL(long, %ld)
TO_STRING_IMPL(ulong, %lu)
#if defined(Q_OS_WIN)
TO_STRING_IMPL(qint64, %I64d)
TO_STRING_IMPL(quint64, %I64u)
#else
TO_STRING_IMPL(qint64, %lld)
TO_STRING_IMPL(quint64, %llu)
#endif
TO_STRING_IMPL(bool, %d)
TO_STRING_IMPL(signed char, %hhd)
TO_STRING_IMPL(unsigned char, %hhu)

/*!
  \internal

  Be consistent about leading 0 in exponent.

  POSIX specifies that %e (hence %g when using it) uses at least two digits in
  the exponent, requiring a leading 0 on single-digit exponents; (at least)
  MinGW includes a leading zero also on an already-two-digit exponent,
  e.g. 9e-040, which differs from more usual platforms.  So massage that away.
 */
static void massageExponent(char *text)
{
    char *p = strchr(text, 'e');
    if (!p)
        return;
    const char *const end = p + strlen(p); // *end is '\0'
    p += (p[1] == '-' || p[1] == '+') ? 2 : 1;
    if (p[0] != '0' || end - 2 <= p)
        return;
    // We have a leading 0 on an exponent of at least two more digits
    const char *n = p + 1;
    while (end - 2 > n && n[0] == '0')
        ++n;
    memmove(p, n, end + 1 - n);
}

// Be consistent about display of infinities and NaNs (snprintf()'s varies,
// notably on MinGW, despite POSIX documenting "[-]inf" or "[-]infinity" for %f,
// %e and %g, uppercasing for their capital versions; similar for "nan"):
#define TO_STRING_FLOAT(TYPE, FORMAT) \
template <> Q_TESTLIB_EXPORT char *QTest::toString<TYPE>(const TYPE &t) \
{ \
    char *msg = new char[128]; \
    switch (qFpClassify(t)) { \
    case FP_INFINITE: \
        qstrncpy(msg, (t < 0 ? "-inf" : "inf"), 128); \
        break; \
    case FP_NAN: \
        qstrncpy(msg, "nan", 128); \
        break; \
    default: \
        qsnprintf(msg, 128, #FORMAT, double(t));    \
        massageExponent(msg); \
        break; \
    } \
    return msg; \
}

TO_STRING_FLOAT(qfloat16, %.3g)
TO_STRING_FLOAT(float, %g)
TO_STRING_FLOAT(double, %.12g)

template <> Q_TESTLIB_EXPORT char *QTest::toString<char>(const char &t)
{
    unsigned char c = static_cast<unsigned char>(t);
    char *msg = new char[16];
    switch (c) {
    case 0x00:
        qstrcpy(msg, "'\\0'");
        break;
    case 0x07:
        qstrcpy(msg, "'\\a'");
        break;
    case 0x08:
        qstrcpy(msg, "'\\b'");
        break;
    case 0x09:
        qstrcpy(msg, "'\\t'");
        break;
    case 0x0a:
        qstrcpy(msg, "'\\n'");
        break;
    case 0x0b:
        qstrcpy(msg, "'\\v'");
        break;
    case 0x0c:
        qstrcpy(msg, "'\\f'");
        break;
    case 0x0d:
        qstrcpy(msg, "'\\r'");
        break;
    case 0x22:
        qstrcpy(msg, "'\\\"'");
        break;
    case 0x27:
        qstrcpy(msg, "'\\\''");
        break;
    case 0x5c:
        qstrcpy(msg, "'\\\\'");
        break;
    default:
        if (c < 0x20 || c >= 0x7F)
            qsnprintf(msg, 16, "'\\x%02x'", c);
        else
            qsnprintf(msg, 16, "'%c'" , c);
    }
    return msg;
}

/*! \internal
 */
char *QTest::toString(const char *str)
{
    if (!str) {
        char *msg = new char[1];
        *msg = '\0';
        return msg;
    }
    char *msg = new char[strlen(str) + 1];
    return qstrcpy(msg, str);
}

/*! \internal
 */
char *QTest::toString(const void *p)
{
    char *msg = new char[128];
    qsnprintf(msg, 128, "%p", p);
    return msg;
}

/*! \fn char *QTest::toString(const QColor &color)
    \internal
 */

/*! \fn char *QTest::toString(const QRegion &region)
    \internal
 */

/*! \fn char *QTest::toString(const QHostAddress &addr)
    \internal
 */

/*! \fn char *QTest::toString(QNetworkReply::NetworkError code)
    \internal
 */

/*! \fn char *QTest::toString(const QNetworkCookie &cookie)
    \internal
 */

/*! \fn char *QTest::toString(const QList<QNetworkCookie> &list)
    \internal
 */

/*! \internal
 */
bool QTest::compare_string_helper(const char *t1, const char *t2, const char *actual,
                                  const char *expected, const char *file, int line)
{
    return compare_helper(qstrcmp(t1, t2) == 0, "Compared strings are not the same",
                          toString(t1), toString(t2), actual, expected, file, line);
}

/*!
   \namespace QTest::Internal
   \internal
*/

/*! \fn bool QTest::compare_ptr_helper(const volatile void *t1, const volatile void *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::compare_ptr_helper(const volatile void *t1, std::nullptr_t, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::compare_ptr_helper(std::nullptr_t, const volatile void *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T1, typename T2> bool QTest::qCompare(const T1 &t1, const T2 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const QIcon &t1, const QIcon &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const QImage &t1, const QImage &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const QPixmap &t1, const QPixmap &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T> bool QTest::qCompare(const T &t1, const T &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T> bool QTest::qCompare(const T *t1, const T *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T> bool QTest::qCompare(T *t, std::nullptr_t, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T> bool QTest::qCompare(std::nullptr_t, T *t, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T> bool QTest::qCompare(T *t1, T *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T1, typename T2> bool QTest::qCompare(const T1 *t1, const T2 *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T1, typename T2> bool QTest::qCompare(T1 *t1, T2 *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const char *t1, const char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(char *t1, char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(char *t1, const char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const char *t1, char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const QString &t1, const QLatin1String &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const QLatin1String &t1, const QString &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const QStringList &t1, const QStringList &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn  template <typename T> bool QTest::qCompare(const QList<T> &t1, const QList<T> &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T> bool QTest::qCompare(const QFlags<T> &t1, const T &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn template <typename T> bool QTest::qCompare(const QFlags<T> &t1, const int &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const qint64 &t1, const qint32 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const qint64 &t1, const quint32 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const quint64 &t1, const quint32 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const qint32 &t1, const qint64 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const quint32 &t1, const qint64 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const quint32 &t1, const quint64 &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn  template <typename T> bool QTest::qTest(const T& actual, const char *elementName, const char *actualStr, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code, QString text, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWindow *window, Qt::Key code, QString text, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code, char ascii, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWindow *window, Qt::Key code, char ascii, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::simulateEvent(QWidget *widget, bool press, int code, Qt::KeyboardModifiers modifier, QString text, bool repeat, int delay=-1)
    \internal
*/

/*! \fn void QTest::simulateEvent(QWindow *window, bool press, int code, Qt::KeyboardModifiers modifier, QString text, bool repeat, int delay=-1)
    \internal
*/

QT_END_NAMESPACE
