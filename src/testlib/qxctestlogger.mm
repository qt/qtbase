/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qxctestlogger_p.h"

#include <QtCore/qstring.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtestresult_p.h>

#import <XCTest/XCTest.h>

// ---------------------------------------------------------

@interface XCTestProbe (Private)
+ (BOOL)isTesting;
+ (void)runTests:(id)unusedArgument;
+ (NSString*)testScope;
+ (BOOL)isInverseTestScope;
@end

@interface XCTestDriver : NSObject
+ (XCTestDriver*)sharedTestDriver;
@property (readonly, assign) NSObject *IDEConnection;
@end

@interface XCTest (Private)
- (NSString *)nameForLegacyLogging;
@end

QT_WARNING_PUSH
// Ignore XCTestProbe deprecation
QT_WARNING_DISABLE_DEPRECATED

// ---------------------------------------------------------

@interface QtTestLibWrapper : XCTestCase
@end

@interface QtTestLibTests : XCTestSuite
+ (XCTestSuiteRun*)testRun;
@end

@interface QtTestLibTest : XCTestCase
@property (nonatomic, retain) NSString* testObjectName;
@property (nonatomic, retain) NSString* testFunctionName;
@end

// ---------------------------------------------------------

class ThreadBarriers
{
public:
    enum Barrier {
        XCTestCanStartTesting,
        XCTestHaveStarted,
        QtTestsCanStartTesting,
        QtTestsHaveCompleted,
        XCTestsHaveCompleted,
        BarrierCount
    };

    static ThreadBarriers *get()
    {
        static ThreadBarriers instance;
        return &instance;
    }

    static void initialize() { get(); }

    void wait(Barrier barrier) { dispatch_semaphore_wait(barriers[barrier], DISPATCH_TIME_FOREVER); }
    void signal(Barrier barrier) { dispatch_semaphore_signal(barriers[barrier]); }

private:
    #define FOREACH_BARRIER(cmd) for (int i = 0; i < BarrierCount; ++i) { cmd }

    ThreadBarriers() { FOREACH_BARRIER(barriers[i] = dispatch_semaphore_create(0);) }
    ~ThreadBarriers() { FOREACH_BARRIER(dispatch_release(barriers[i]);) }

    dispatch_semaphore_t barriers[BarrierCount];
};

#define WAIT_FOR_BARRIER(b) ThreadBarriers::get()->wait(ThreadBarriers::b);
#define SIGNAL_BARRIER(b) ThreadBarriers::get()->signal(ThreadBarriers::b);

// ---------------------------------------------------------

@implementation QtTestLibWrapper

+ (void)load
{
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    if (![XCTestProbe isTesting])
        return;

    if (Q_UNLIKELY(!([NSDate timeIntervalSinceReferenceDate] > 0)))
        qFatal("error: Device date '%s' is bad, likely set to update automatically. Please correct.",
            [[NSDate date] description].UTF8String);

    XCTestDriver *testDriver = nil;
    if ([QtTestLibWrapper usingTestManager])
        testDriver = [XCTestDriver sharedTestDriver];

    // Spawn off task to run test infrastructure on separate thread so that we can
    // let main() execute like normal on the main thread. The queue will never be
    // destroyed, so there's no point in trying to keep a proper retain count.
    dispatch_async(dispatch_queue_create("io.qt.QTestLib.xctest-wrapper", DISPATCH_QUEUE_SERIAL), ^{
        Q_ASSERT(![NSThread isMainThread]);
        [XCTestProbe runTests:nil];
        Q_UNREACHABLE();
    });

    // Initialize barriers before registering exit handler so that the
    // semaphores stay alive until after the exit handler completes.
    ThreadBarriers::initialize();

    // We register an exit handler so that we can intercept when main() completes
    // and let the XCTest thread finish up. For main() functions that never started
    // testing using QtTestLib we also need to signal that xcTestsCanStart.
    atexit_b(^{
        Q_ASSERT([NSThread isMainThread]);

        // In case not started by startLogging
        SIGNAL_BARRIER(XCTestCanStartTesting);

        // [XCTestProbe runTests:] ends up calling [XCTestProbe exitTests:] after
        // all test suites have completed, which calls exit(). We use that to signal
        // to the main thread that it's free to continue its exit handler.
        atexit_b(^{
            Q_ASSERT(![NSThread isMainThread]);
            SIGNAL_BARRIER(XCTestsHaveCompleted);

            // Block forever so that the main thread does all the cleanup
            dispatch_semaphore_wait(dispatch_semaphore_create(0), DISPATCH_TIME_FOREVER);
        });

        SIGNAL_BARRIER(QtTestsHaveCompleted);

        // Ensure XCTest complets the top level tests suite
        WAIT_FOR_BARRIER(XCTestsHaveCompleted);
    });

    // Let test driver (Xcode) connection setup complete before continuing
    if ([[[NSProcessInfo processInfo] arguments] containsObject:@"--use-testmanagerd"]) {
        while (!testDriver.IDEConnection)
            [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
    }

    // Wait for our QtTestLib test suite to run before running main
    WAIT_FOR_BARRIER(QtTestsCanStartTesting);

    // Prevent XCTestProbe from re-launching runTests on application startup
    [[NSNotificationCenter defaultCenter] removeObserver:[XCTestProbe class]
        name:[NSString stringWithFormat:@"%@DidFinishLaunchingNotification",
                #if defined(Q_OS_MACOS)
                    @"NSApplication"
                #else
                    @"UIApplication"
                #endif
             ]
        object:nil];

    [autoreleasepool release];
}

+ (QTestLibTests *)defaultTestSuite
{
    return [[QtTestLibTests alloc] initWithName:@"QtTestLib"];
}

+ (BOOL)usingTestManager
{
    return [[[NSProcessInfo processInfo] arguments] containsObject:@"--use-testmanagerd"];
}

@end

// ---------------------------------------------------------

static XCTestSuiteRun *s_qtTestSuiteRun = 0;

@implementation QtTestLibTests

- (void)performTest:(XCTestSuiteRun *)testSuiteRun
{
    Q_ASSERT(![NSThread isMainThread]);

    Q_ASSERT(!s_qtTestSuiteRun);
    s_qtTestSuiteRun = testSuiteRun;

    SIGNAL_BARRIER(QtTestsCanStartTesting);

    // Wait for main() to complete, or a QtTestLib test to start, so we
    // know if we should start the QtTestLib test suite.
    WAIT_FOR_BARRIER(XCTestCanStartTesting);

    if (QXcodeTestLogger::isActive())
        [testSuiteRun start];

    SIGNAL_BARRIER(XCTestHaveStarted);

    // All test reporting happens on main thread from now on. Wait until
    // main() completes before allowing the XCTest thread to continue.
    WAIT_FOR_BARRIER(QtTestsHaveCompleted);

    if ([testSuiteRun startDate])
        [testSuiteRun stop];
}

+ (XCTestSuiteRun*)testRun
{
    return s_qtTestSuiteRun;
}

@end

// ---------------------------------------------------------

@implementation QtTestLibTest

- (instancetype)initWithInvocation:(NSInvocation *)invocation
{
    if (self = [super initWithInvocation:invocation]) {
        // The test object name and function name are used by XCTest after QtTestLib has
        // reset them, so we need to store them up front for each XCTestCase.
        self.testObjectName = [NSString stringWithUTF8String:QTestResult::currentTestObjectName()];
        self.testFunctionName = [NSString stringWithUTF8String:QTestResult::currentTestFunction()];
    }

    return self;
}

- (NSString *)testClassName
{
    return self.testObjectName;
}

- (NSString *)testMethodName
{
    return self.testFunctionName;
}

- (NSString *)nameForLegacyLogging
{
    NSString *name = [NSString stringWithFormat:@"%@::%@", [self testClassName], [self testMethodName]];
    if (QTestResult::currentDataTag() || QTestResult::currentGlobalDataTag()) {
        const char *currentDataTag = QTestResult::currentDataTag() ? QTestResult::currentDataTag() : "";
        const char *globalDataTag = QTestResult::currentGlobalDataTag() ? QTestResult::currentGlobalDataTag() : "";
        const char *filler = (currentDataTag[0] && globalDataTag[0]) ? ":" : "";
        name = [name stringByAppendingString:[NSString stringWithFormat:@"(%s%s%s)",
            globalDataTag, filler, currentDataTag]];
    }

    return name;
}

@end

// ---------------------------------------------------------

bool QXcodeTestLogger::canLogTestProgress()
{
    return [XCTestProbe isTesting]; // FIXME: Exclude xctool
}

int QXcodeTestLogger::parseCommandLineArgument(const char *argument)
{
    if (strncmp(argument, "-NS", 3) == 0 || strncmp(argument, "-Apple", 6) == 0)
        return 2; // -NSTreatUnknownArgumentsAsOpen, -ApplePersistenceIgnoreState, etc, skip argument
    else if (strcmp(argument, "--use-testmanagerd") == 0)
        return 2; // Skip UID argument
    else if (strncmp(argument, "-XCTest", 7) == 0)
        return 2; // -XCTestInvertScope, -XCTest scope, etc, skip argument
    else if (strcmp(argument + (strlen(argument) - 7), ".xctest") == 0)
        return 1; // Skip test bundle
    else
        return 0;
}

// ---------------------------------------------------------

QXcodeTestLogger *QXcodeTestLogger::s_currentTestLogger = 0;

// ---------------------------------------------------------

QXcodeTestLogger::QXcodeTestLogger()
    : QAbstractTestLogger(0)
    , m_testRuns([[NSMutableArray<XCTestRun *> arrayWithCapacity:2] retain])

{
    Q_ASSERT(!s_currentTestLogger);
    s_currentTestLogger = this;
}

QXcodeTestLogger::~QXcodeTestLogger()
{
    s_currentTestLogger = 0;
    [m_testRuns release];
}

void QXcodeTestLogger::startLogging()
{
    SIGNAL_BARRIER(XCTestCanStartTesting);

    static dispatch_once_t onceToken;
    dispatch_once (&onceToken, ^{
        WAIT_FOR_BARRIER(XCTestHaveStarted);
    });

    // Scope test object suite under top level QtTestLib test run
    [m_testRuns addObject:[QtTestLibTests testRun]];

    NSString *suiteName = [NSString stringWithUTF8String:QTestResult::currentTestObjectName()];
    pushTestRunForTest([XCTestSuite testSuiteWithName:suiteName], true);
}

void QXcodeTestLogger::stopLogging()
{
    popTestRun();
}

static bool isTestFunctionInActiveScope(const char *function)
{
    static NSString *testScope = [XCTestProbe testScope];

    enum TestScope { Unknown, All, None, Self, Selected };
    static TestScope activeScope = Unknown;

    if (activeScope == Unknown) {
        if ([testScope isEqualToString:@"All"])
            activeScope = All;
        else if ([testScope isEqualToString:@"None"])
            activeScope = None;
        else if ([testScope isEqualToString:@"Self"])
            activeScope = Self;
        else
            activeScope = Selected;
    }

    if (activeScope == All)
        return true;
    else if (activeScope == None)
        return false;
    else if (activeScope == Self)
        return true; // Investigate

    Q_ASSERT(activeScope == Selected);

    static NSArray<NSString *> *forcedTests = [@[ @"initTestCase", @"initTestCase_data", @"cleanupTestCase" ] retain];
    if ([forcedTests containsObject:[NSString stringWithUTF8String:function]])
        return true;

    static NSArray<NSString *> *testsInScope = [[testScope componentsSeparatedByString:@","] retain];
    bool inScope = [testsInScope containsObject:[NSString stringWithFormat:@"%s/%s",
                        QTestResult::currentTestObjectName(), function]];

    if ([XCTestProbe isInverseTestScope])
        inScope = !inScope;

    return inScope;
}

void QXcodeTestLogger::enterTestFunction(const char *function)
{
    if (!isTestFunctionInActiveScope(function))
        QTestResult::setSkipCurrentTest(true);

    XCTest *test = [QtTestLibTest testCaseWithInvocation:nil];
    pushTestRunForTest(test, !QTestResult::skipCurrentTest());
}

void QXcodeTestLogger::leaveTestFunction()
{
    popTestRun();
}

void QXcodeTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    XCTestRun *testRun = [m_testRuns lastObject];

    // The 'expected' argument to recordFailureWithDescription refers to whether
    // the failure was a regular failed assertion, or an unexpected exception,
    // so in our case it's always 'YES', and we need to explicitly ignore XFail.
    if (type == QAbstractTestLogger::XFail) {
        QTestCharBuffer buf;
        NSString *testCaseName = [[testRun test] nameForLegacyLogging];
        QTest::qt_asprintf(&buf, "Test Case '%s' failed expectedly (%s).\n",
            [testCaseName UTF8String], description);
        outputString(buf.constData());
        return;
    }

    if (type == QAbstractTestLogger::Pass) {
        // We ignore non-data passes, as we're already reporting that as part of the
        // normal test case start/stop cycle.
        if (!(QTestResult::currentDataTag() || QTestResult::currentGlobalDataTag()))
            return;

        QTestCharBuffer buf;
        NSString *testCaseName = [[testRun test] nameForLegacyLogging];
        QTest::qt_asprintf(&buf, "Test Case '%s' passed.\n", [testCaseName UTF8String]);
        outputString(buf.constData());
        return;
    }

    // FIXME: Handle blacklisted tests

    if (!file || !description)
        return; // Or report?

    [testRun recordFailureWithDescription:[NSString stringWithUTF8String:description]
        inFile:[NSString stringWithUTF8String:file] atLine:line expected:YES];
}

void QXcodeTestLogger::addMessage(MessageTypes type, const QString &message,
                                  const char *file, int line)
{
    QTestCharBuffer buf;

    if (QTestLog::verboseLevel() > 0 && (file && line)) {
        QTest::qt_asprintf(&buf, "%s:%d: ", file, line);
        outputString(buf.constData());
    }

    if (type == QAbstractTestLogger::Skip) {
        XCTestRun *testRun = [m_testRuns lastObject];
        NSString *testCaseName = [[testRun test] nameForLegacyLogging];
        QTest::qt_asprintf(&buf, "Test Case '%s' skipped (%s).\n",
            [testCaseName UTF8String], message.toUtf8().constData());
    } else {
        QTest::qt_asprintf(&buf, "%s\n", message.toUtf8().constData());
    }

    outputString(buf.constData());
}

void QXcodeTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    Q_UNUSED(result);
}

void QXcodeTestLogger::pushTestRunForTest(XCTest *test, bool start)
{
    XCTestRun *testRun = [[test testRunClass] testRunWithTest:test];
    [m_testRuns addObject:testRun];

    if (start)
        [testRun start];
}

XCTestRun *QXcodeTestLogger::popTestRun()
{
    XCTestRun *testRun = [[m_testRuns lastObject] retain];
    [m_testRuns removeLastObject];

    if ([testRun startDate])
        [testRun stop];

    [[m_testRuns lastObject] addTestRun:testRun];
    [testRun release];

    return testRun;
}

bool QXcodeTestLogger::isActive()
{
    return s_currentTestLogger;
}

QT_WARNING_POP
