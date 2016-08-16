/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QTemporaryDir>
#include <QtCore/QRegExp>
#include <QtCore/QDebug>
#include <QtCore/QMetaType>
#include <QtNetwork/QHostInfo>
#include <stdlib.h>

typedef void (QProcess::*QProcessFinishedSignal1)(int);
typedef void (QProcess::*QProcessFinishedSignal2)(int, QProcess::ExitStatus);
typedef void (QProcess::*QProcessErrorSignal)(QProcess::ProcessError);

class tst_QProcess : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

#ifndef QT_NO_PROCESS
private slots:
    void getSetCheck();
    void constructing();
    void simpleStart();
    void startWithOpen();
    void startWithOldOpen();
    void execute();
    void startDetached();
    void crashTest();
    void crashTest2();
    void echoTest_data();
    void echoTest();
    void echoTest2();
#ifdef Q_OS_WIN
    void echoTestGui();
    void testSetNamedPipeHandleState();
    void batFiles_data();
    void batFiles();
#endif
    void loopBackTest();
    void readTimeoutAndThenCrash();
    void deadWhileReading();
    void restartProcessDeadlock();
    void closeWriteChannel();
    void closeReadChannel();
    void openModes();
    void emitReadyReadOnlyWhenNewDataArrives();
    void softExitInSlots_data();
    void softExitInSlots();
    void mergedChannels();
    void forwardedChannels_data();
    void forwardedChannels();
    void atEnd();
    void atEnd2();
    void waitForFinishedWithTimeout();
    void waitForReadyReadInAReadyReadSlot();
    void waitForBytesWrittenInABytesWrittenSlot();
    void setEnvironment_data();
    void setEnvironment();
    void setProcessEnvironment_data();
    void setProcessEnvironment();
    void spaceInName();
    void setStandardInputFile();
    void setStandardOutputFile_data();
    void setStandardOutputFile();
    void setStandardOutputFileNullDevice();
    void setStandardOutputFileAndWaitForBytesWritten();
    void setStandardOutputProcess_data();
    void setStandardOutputProcess();
    void removeFileWhileProcessIsRunning();
    void fileWriterProcess();
    void switchReadChannels();
    void discardUnwantedOutput();
    void setWorkingDirectory();
    void setNonExistentWorkingDirectory();

    void exitStatus_data();
    void exitStatus();
    void waitForFinished();
    void hardExit();
    void softExit();
    void processInAThread();
    void processesInMultipleThreads();
    void spaceArgsTest_data();
    void spaceArgsTest();
#if defined(Q_OS_WIN)
    void nativeArguments();
    void createProcessArgumentsModifier();
#endif // Q_OS_WIN
    void exitCodeTest();
    void systemEnvironment();
    void lockupsInStartDetached();
    void waitForReadyReadForNonexistantProcess();
    void detachedWorkingDirectoryAndPid();
    void startFinishStartFinish();
    void invalidProgramString_data();
    void invalidProgramString();
    void onlyOneStartedSignal();
    void finishProcessBeforeReadingDone();
    void waitForStartedWithoutStart();
    void startStopStartStop();
    void startStopStartStopBuffers_data();
    void startStopStartStopBuffers();
    void processEventsInAReadyReadSlot_data();
    void processEventsInAReadyReadSlot();

    // keep these at the end, since they use lots of processes and sometimes
    // caused obscure failures to occur in tests that followed them (esp. on the Mac)
    void failToStart();
    void failToStartWithWait();
    void failToStartWithEventLoop();
    void failToStartEmptyArgs_data();
    void failToStartEmptyArgs();

protected slots:
    void readFromProcess();
    void exitLoopSlot();
    void processApplicationEvents();
    void restartProcess();
    void waitForReadyReadInAReadyReadSlotSlot();
    void waitForBytesWrittenInABytesWrittenSlotSlot();

private:
    qint64 bytesAvailable;
    QTemporaryDir m_temporaryDir;
#endif //QT_NO_PROCESS
};

void tst_QProcess::initTestCase()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
    QVERIFY2(m_temporaryDir.isValid(), qPrintable(m_temporaryDir.errorString()));
    // chdir to our testdata path and execute helper apps relative to that.
    QString testdata_dir = QFileInfo(QFINDTESTDATA("testProcessNormal")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
#endif
}

void tst_QProcess::cleanupTestCase()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#endif
}

void tst_QProcess::init()
{
    bytesAvailable = 0;
}

#ifndef QT_NO_PROCESS

// Testing get/set functions
void tst_QProcess::getSetCheck()
{
    QProcess obj1;
    // ProcessChannelMode QProcess::readChannelMode()
    // void QProcess::setReadChannelMode(ProcessChannelMode)
    obj1.setReadChannelMode(QProcess::ProcessChannelMode(QProcess::SeparateChannels));
    QCOMPARE(QProcess::ProcessChannelMode(QProcess::SeparateChannels), obj1.readChannelMode());
    obj1.setReadChannelMode(QProcess::ProcessChannelMode(QProcess::MergedChannels));
    QCOMPARE(QProcess::ProcessChannelMode(QProcess::MergedChannels), obj1.readChannelMode());
    obj1.setReadChannelMode(QProcess::ProcessChannelMode(QProcess::ForwardedChannels));
    QCOMPARE(QProcess::ProcessChannelMode(QProcess::ForwardedChannels), obj1.readChannelMode());

    // ProcessChannel QProcess::readChannel()
    // void QProcess::setReadChannel(ProcessChannel)
    obj1.setReadChannel(QProcess::ProcessChannel(QProcess::StandardOutput));
    QCOMPARE(QProcess::ProcessChannel(QProcess::StandardOutput), obj1.readChannel());
    obj1.setReadChannel(QProcess::ProcessChannel(QProcess::StandardError));
    QCOMPARE(QProcess::ProcessChannel(QProcess::StandardError), obj1.readChannel());
}

void tst_QProcess::constructing()
{
    QProcess process;
    QCOMPARE(process.readChannel(), QProcess::StandardOutput);
    QCOMPARE(process.workingDirectory(), QString());
    QCOMPARE(process.environment(), QStringList());
    QCOMPARE(process.error(), QProcess::UnknownError);
    QCOMPARE(process.state(), QProcess::NotRunning);
    QCOMPARE(process.pid(), Q_PID(0));
    QCOMPARE(process.readAllStandardOutput(), QByteArray());
    QCOMPARE(process.readAllStandardError(), QByteArray());
    QCOMPARE(process.canReadLine(), false);

    // QIODevice
    QCOMPARE(process.openMode(), QIODevice::NotOpen);
    QVERIFY(!process.isOpen());
    QVERIFY(!process.isReadable());
    QVERIFY(!process.isWritable());
    QVERIFY(process.isSequential());
    QCOMPARE(process.pos(), qlonglong(0));
    QCOMPARE(process.size(), qlonglong(0));
    QVERIFY(process.atEnd());
    QCOMPARE(process.bytesAvailable(), qlonglong(0));
    QCOMPARE(process.bytesToWrite(), qlonglong(0));
    QVERIFY(!process.errorString().isEmpty());

    char c;
    QCOMPARE(process.read(&c, 1), qlonglong(-1));
    QCOMPARE(process.write(&c, 1), qlonglong(-1));

    QProcess proc2;
}

void tst_QProcess::simpleStart()
{
    qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");

    QScopedPointer<QProcess> process(new QProcess);
    QSignalSpy spy(process.data(), &QProcess::stateChanged);
    QVERIFY(spy.isValid());
    connect(process.data(), &QIODevice::readyRead, this, &tst_QProcess::readFromProcess);

    /* valgrind dislike SUID binaries(those that have the `s'-flag set), which
     * makes it fail to start the process. For this reason utilities like `ping' won't
     * start, when the auto test is run through `valgrind'. */
    process->start("testProcessNormal/testProcessNormal");
    if (process->state() != QProcess::Starting)
        QCOMPARE(process->state(), QProcess::Running);
    QVERIFY2(process->waitForStarted(5000), qPrintable(process->errorString()));
    QCOMPARE(process->state(), QProcess::Running);
    QTRY_COMPARE(process->state(), QProcess::NotRunning);

    process.reset();

    QCOMPARE(spy.count(), 3);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(spy.at(0).at(0)), QProcess::Starting);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(spy.at(1).at(0)), QProcess::Running);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(spy.at(2).at(0)), QProcess::NotRunning);
}

void tst_QProcess::startWithOpen()
{
    QProcess p;
    QTest::ignoreMessage(QtWarningMsg, "QProcess::start: program not set");
    QCOMPARE(p.open(QIODevice::ReadOnly), false);

    p.setProgram("testProcessNormal/testProcessNormal");
    QCOMPARE(p.program(), QString("testProcessNormal/testProcessNormal"));

    p.setArguments(QStringList() << "arg1" << "arg2");
    QCOMPARE(p.arguments().size(), 2);

    QVERIFY(p.open(QIODevice::ReadOnly));
    QCOMPARE(p.openMode(), QIODevice::ReadOnly);
    QVERIFY(p.waitForFinished(5000));
}

void tst_QProcess::startWithOldOpen()
{
    // similar to the above, but we start with start() actually
    // while open() is overridden to call QIODevice::open().
    // This tests the BC requirement that "it works with the old implementation"
    class OverriddenOpen : public QProcess
    {
    public:
        virtual bool open(OpenMode mode) Q_DECL_OVERRIDE
        { return QIODevice::open(mode); }
    };

    OverriddenOpen p;
    p.start("testProcessNormal/testProcessNormal");
    QVERIFY(p.waitForStarted(5000));
    QVERIFY(p.waitForFinished(5000));
}

void tst_QProcess::execute()
{
    QCOMPARE(QProcess::execute("testProcessNormal/testProcessNormal",
                               QStringList() << "arg1" << "arg2"), 0);
    QCOMPARE(QProcess::execute("nonexistingexe"), -2);
}

void tst_QProcess::startDetached()
{
    QVERIFY(QProcess::startDetached("testProcessNormal/testProcessNormal",
                                    QStringList() << "arg1" << "arg2"));
    QCOMPARE(QProcess::startDetached("nonexistingexe"), false);
}

void tst_QProcess::readFromProcess()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    QVERIFY(process);
    int lines = 0;
    while (process->canReadLine()) {
        ++lines;
        process->readLine();
    }
}

void tst_QProcess::crashTest()
{
    qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");
    QScopedPointer<QProcess> process(new QProcess);
    QSignalSpy stateSpy(process.data(), &QProcess::stateChanged);
    QVERIFY(stateSpy.isValid());
    process->start("testProcessCrash/testProcessCrash");
    QVERIFY(process->waitForStarted(5000));

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QSignalSpy spy(process.data(), &QProcess::errorOccurred);
    QSignalSpy spy2(process.data(), static_cast<QProcessErrorSignal>(&QProcess::error));
    QSignalSpy spy3(process.data(), static_cast<QProcessFinishedSignal2>(&QProcess::finished));

    QVERIFY(spy.isValid());
    QVERIFY(spy2.isValid());
    QVERIFY(spy3.isValid());

    QVERIFY(process->waitForFinished(30000));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy.at(0).at(0).constData()), QProcess::Crashed);

    QCOMPARE(spy2.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy2.at(0).at(0).constData()), QProcess::Crashed);

    QCOMPARE(spy3.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ExitStatus *>(spy3.at(0).at(1).constData()), QProcess::CrashExit);

    QCOMPARE(process->exitStatus(), QProcess::CrashExit);

    // delete process;
    process.reset();

    QCOMPARE(stateSpy.count(), 3);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(0).at(0)), QProcess::Starting);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(1).at(0)), QProcess::Running);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(2).at(0)), QProcess::NotRunning);
}

void tst_QProcess::crashTest2()
{
    QProcess process;
    process.start("testProcessCrash/testProcessCrash");
    QVERIFY(process.waitForStarted(5000));

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QSignalSpy spy(&process, static_cast<QProcessErrorSignal>(&QProcess::errorOccurred));
    QSignalSpy spy2(&process, static_cast<QProcessFinishedSignal2>(&QProcess::finished));

    QVERIFY(spy.isValid());
    QVERIFY(spy2.isValid());

    QObject::connect(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished),
                     this, &tst_QProcess::exitLoopSlot);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Failed to detect crash : operation timed out");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy.at(0).at(0).constData()), QProcess::Crashed);

    QCOMPARE(spy2.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ExitStatus *>(spy2.at(0).at(1).constData()), QProcess::CrashExit);

    QCOMPARE(process.exitStatus(), QProcess::CrashExit);
}

void tst_QProcess::echoTest_data()
{
    QTest::addColumn<QByteArray>("input");

    QTest::newRow("1") << QByteArray("H");
    QTest::newRow("2") << QByteArray("He");
    QTest::newRow("3") << QByteArray("Hel");
    QTest::newRow("4") << QByteArray("Hell");
    QTest::newRow("5") << QByteArray("Hello");
    QTest::newRow("100 bytes") << QByteArray(100, '@');
    QTest::newRow("1000 bytes") << QByteArray(1000, '@');
    QTest::newRow("10000 bytes") << QByteArray(10000, '@');
}

void tst_QProcess::echoTest()
{
    QFETCH(QByteArray, input);

    QProcess process;
    connect(&process, &QIODevice::readyRead, this, &tst_QProcess::exitLoopSlot);

    process.start("testProcessEcho/testProcessEcho");
    QVERIFY(process.waitForStarted(5000));

    process.write(input);

    QTime stopWatch;
    stopWatch.start();
    do {
        QVERIFY(process.isOpen());
        QTestEventLoop::instance().enterLoop(2);
    } while (stopWatch.elapsed() < 60000 && process.bytesAvailable() < input.size());
    if (stopWatch.elapsed() >= 60000)
        QFAIL("Timed out");

    QByteArray message = process.readAll();
    QCOMPARE(message.size(), input.size());

    char *c1 = message.data();
    char *c2 = input.data();
    while (*c1 && *c2) {
        if (*c1 != *c2)
            QCOMPARE(*c1, *c2);
        ++c1;
        ++c2;
    }
    QCOMPARE(*c1, *c2);

    process.write("", 1);

    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::exitLoopSlot()
{
    QTestEventLoop::instance().exitLoop();
}

void tst_QProcess::processApplicationEvents()
{
    QCoreApplication::processEvents();
}

void tst_QProcess::echoTest2()
{

    QProcess process;
    connect(&process, &QIODevice::readyRead, this, &tst_QProcess::exitLoopSlot);

    process.start("testProcessEcho2/testProcessEcho2");
    QVERIFY(process.waitForStarted(5000));
    QVERIFY(!process.waitForReadyRead(250));
    QCOMPARE(process.error(), QProcess::Timedout);

    process.write("Hello");
    QSignalSpy spy0(&process, &QProcess::channelReadyRead);
    QSignalSpy spy1(&process, &QProcess::readyReadStandardOutput);
    QSignalSpy spy2(&process, &QProcess::readyReadStandardError);

    QVERIFY(spy0.isValid());
    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());

    QTime stopWatch;
    stopWatch.start();
    forever {
        QTestEventLoop::instance().enterLoop(1);
        if (stopWatch.elapsed() >= 30000)
            QFAIL("Timed out");
        process.setReadChannel(QProcess::StandardOutput);
        qint64 baso = process.bytesAvailable();

        process.setReadChannel(QProcess::StandardError);
        qint64 base = process.bytesAvailable();
        if (baso == 5 && base == 5)
            break;
    }

    QVERIFY(spy0.count() > 0);
    QVERIFY(spy1.count() > 0);
    QVERIFY(spy2.count() > 0);

    QCOMPARE(process.readAllStandardOutput(), QByteArray("Hello"));
    QCOMPARE(process.readAllStandardError(), QByteArray("Hello"));

    process.write("", 1);
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

#if defined(Q_OS_WIN)
void tst_QProcess::echoTestGui()
{
    QProcess process;

    process.start("testProcessEchoGui/testProcessEchoGui");


    process.write("Hello");
    process.write("q");

    QVERIFY(process.waitForFinished(50000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    QCOMPARE(process.readAllStandardOutput(), QByteArray("Hello"));
    QCOMPARE(process.readAllStandardError(), QByteArray("Hello"));
}

void tst_QProcess::testSetNamedPipeHandleState()
{
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start("testSetNamedPipeHandleState/testSetNamedPipeHandleState");
    QVERIFY2(process.waitForStarted(5000), qPrintable(process.errorString()));
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
}
#endif // Q_OS_WIN

#if defined(Q_OS_WIN)
void tst_QProcess::batFiles_data()
{
    QTest::addColumn<QString>("batFile");
    QTest::addColumn<QByteArray>("output");

    QTest::newRow("simple") << QFINDTESTDATA("testBatFiles/simple.bat") << QByteArray("Hello");
    QTest::newRow("with space") << QFINDTESTDATA("testBatFiles/with space.bat") << QByteArray("Hello");
}

void tst_QProcess::batFiles()
{
    QFETCH(QString, batFile);
    QFETCH(QByteArray, output);

    QProcess proc;

    proc.start(batFile, QStringList());

    QVERIFY(proc.waitForFinished(5000));
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QCOMPARE(proc.exitCode(), 0);

    QVERIFY(proc.bytesAvailable() > 0);

    QVERIFY(proc.readAll().startsWith(output));
}
#endif // Q_OS_WIN

void tst_QProcess::exitStatus_data()
{
    QTest::addColumn<QStringList>("processList");
    QTest::addColumn<QList<QProcess::ExitStatus> >("exitStatus");

    QTest::newRow("normal") << (QStringList() << "testProcessNormal/testProcessNormal")
                            << (QList<QProcess::ExitStatus>() << QProcess::NormalExit);
    QTest::newRow("crash") << (QStringList() << "testProcessCrash/testProcessCrash")
                            << (QList<QProcess::ExitStatus>() << QProcess::CrashExit);

    QTest::newRow("normal-crash") << (QStringList()
                                      << "testProcessNormal/testProcessNormal"
                                      << "testProcessCrash/testProcessCrash")
                                  << (QList<QProcess::ExitStatus>()
                                      << QProcess::NormalExit
                                      << QProcess::CrashExit);
    QTest::newRow("crash-normal") << (QStringList()
                                      << "testProcessCrash/testProcessCrash"
                                      << "testProcessNormal/testProcessNormal")
                                  << (QList<QProcess::ExitStatus>()
                                      << QProcess::CrashExit
                                      << QProcess::NormalExit);
}

void tst_QProcess::exitStatus()
{
    QProcess process;
    QFETCH(QStringList, processList);
    QFETCH(QList<QProcess::ExitStatus>, exitStatus);

    QCOMPARE(exitStatus.count(), processList.count());
    for (int i = 0; i < processList.count(); ++i) {
        process.start(processList.at(i));
        QVERIFY(process.waitForStarted(5000));
        QVERIFY(process.waitForFinished(30000));

        QCOMPARE(process.exitStatus(), exitStatus.at(i));
    }
}

void tst_QProcess::loopBackTest()
{

    QProcess process;
    process.start("testProcessEcho/testProcessEcho");
    QVERIFY(process.waitForStarted(5000));

    for (int i = 0; i < 100; ++i) {
        process.write("Hello");
        do {
            QVERIFY(process.waitForReadyRead(5000));
        } while (process.bytesAvailable() < 5);
        QCOMPARE(process.readAll(), QByteArray("Hello"));
    }

    process.write("", 1);
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::readTimeoutAndThenCrash()
{

    QProcess process;
    process.start("testProcessEcho/testProcessEcho");
    if (process.state() != QProcess::Starting)
        QCOMPARE(process.state(), QProcess::Running);

    QVERIFY(process.waitForStarted(5000));
    QCOMPARE(process.state(), QProcess::Running);

    QVERIFY(!process.waitForReadyRead(5000));
    QCOMPARE(process.error(), QProcess::Timedout);

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    QSignalSpy spy(&process, &QProcess::errorOccurred);
    QSignalSpy spy2(&process, static_cast<QProcessErrorSignal>(&QProcess::error));
    QVERIFY(spy.isValid());
    QVERIFY(spy2.isValid());

    process.kill();

    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.state(), QProcess::NotRunning);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy.at(0).at(0).constData()), QProcess::Crashed);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy2.at(0).at(0).constData()), QProcess::Crashed);
}

void tst_QProcess::waitForFinished()
{
    QProcess process;

    process.start("testProcessOutput/testProcessOutput");

    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    QString output = process.readAll();
    QCOMPARE(output.count("\n"), 10*1024);

    process.start("blurdybloop");
    QVERIFY(!process.waitForFinished());
    QCOMPARE(process.error(), QProcess::FailedToStart);
}

void tst_QProcess::deadWhileReading()
{
    QProcess process;

    process.start("testProcessDeadWhileReading/testProcessDeadWhileReading");

    QString output;

    QVERIFY(process.waitForStarted(5000));
    while (process.waitForReadyRead(5000))
        output += process.readAll();

    QCOMPARE(output.count("\n"), 10*1024);
    process.waitForFinished();
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::restartProcessDeadlock()
{

    // The purpose of this test is to detect whether restarting a
    // process in the finished() connected slot causes a deadlock
    // because of the way QProcessManager uses its locks.
    QProcess process;
    connect(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished),
            this, &tst_QProcess::restartProcess);

    process.start("testProcessEcho/testProcessEcho");

    QCOMPARE(process.write("", 1), qlonglong(1));
    QVERIFY(process.waitForFinished(5000));

    QObject::disconnect(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished), Q_NULLPTR, Q_NULLPTR);

    QCOMPARE(process.write("", 1), qlonglong(1));
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::restartProcess()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    QVERIFY(process);
    process->start("testProcessEcho/testProcessEcho");
}

void tst_QProcess::closeWriteChannel()
{
    QByteArray testData("Data to read");
    QProcess more;
    more.start("testProcessEOF/testProcessEOF");

    QVERIFY(more.waitForStarted(5000));
    QVERIFY(!more.waitForReadyRead(250));
    QCOMPARE(more.error(), QProcess::Timedout);

    QCOMPARE(more.write(testData), qint64(testData.size()));

    QVERIFY(!more.waitForReadyRead(250));
    QCOMPARE(more.error(), QProcess::Timedout);

    more.closeWriteChannel();
    // During closeWriteChannel() call, we might also get an I/O completion
    // on the read pipe. So, take this into account before waiting for
    // the new incoming data.
    while (more.bytesAvailable() < testData.size())
        QVERIFY(more.waitForReadyRead(5000));
    QCOMPARE(more.readAll(), testData);

    if (more.state() == QProcess::Running)
        QVERIFY(more.waitForFinished(5000));
    QCOMPARE(more.exitStatus(), QProcess::NormalExit);
    QCOMPARE(more.exitCode(), 0);
}

void tst_QProcess::closeReadChannel()
{
    for (int i = 0; i < 10; ++i) {
        QProcess::ProcessChannel channel1 = QProcess::StandardOutput;
        QProcess::ProcessChannel channel2 = QProcess::StandardError;

        QProcess proc;
        proc.start("testProcessEcho2/testProcessEcho2");
        QVERIFY(proc.waitForStarted(5000));
        proc.closeReadChannel(i&1 ? channel2 : channel1);
        proc.setReadChannel(i&1 ? channel2 : channel1);
        proc.write("Data");

        QVERIFY(!proc.waitForReadyRead(5000));
        QVERIFY(proc.readAll().isEmpty());

        proc.setReadChannel(i&1 ? channel1 : channel2);

        while (proc.bytesAvailable() < 4 && proc.waitForReadyRead(5000))
        { }

        QCOMPARE(proc.readAll(), QByteArray("Data"));

        proc.write("", 1);
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
        QCOMPARE(proc.exitCode(), 0);
    }
}

void tst_QProcess::openModes()
{
    QProcess proc;
    QVERIFY(!proc.isOpen());
    QCOMPARE(proc.openMode(), QProcess::NotOpen);
    proc.start("testProcessEcho3/testProcessEcho3");
    QVERIFY(proc.waitForStarted(5000));
    QVERIFY(proc.isOpen());
    QCOMPARE(proc.openMode(), QProcess::ReadWrite);
    QVERIFY(proc.isReadable());
    QVERIFY(proc.isWritable());

    proc.write("Data");

    proc.closeWriteChannel();

    QVERIFY(proc.isWritable());
    QCOMPARE(proc.openMode(), QProcess::ReadWrite);

    while (proc.bytesAvailable() < 4 && proc.waitForReadyRead(5000))
    { }

    QCOMPARE(proc.readAll().constData(), QByteArray("Data").constData());

    proc.closeReadChannel(QProcess::StandardOutput);

    QCOMPARE(proc.openMode(), QProcess::ReadWrite);
    QVERIFY(proc.isReadable());

    proc.closeReadChannel(QProcess::StandardError);

    QCOMPARE(proc.openMode(), QProcess::ReadWrite);
    QVERIFY(proc.isReadable());

    proc.close();
    QVERIFY(!proc.isOpen());
    QVERIFY(!proc.isReadable());
    QVERIFY(!proc.isWritable());
    QCOMPARE(proc.state(), QProcess::NotRunning);
}

void tst_QProcess::emitReadyReadOnlyWhenNewDataArrives()
{

    QProcess proc;
    connect(&proc, &QIODevice::readyRead, this, &tst_QProcess::exitLoopSlot);
    QSignalSpy spy(&proc, &QProcess::readyRead);
    QVERIFY(spy.isValid());

    proc.start("testProcessEcho/testProcessEcho");

    QCOMPARE(spy.count(), 0);

    proc.write("A");

    QTestEventLoop::instance().enterLoop(5);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Operation timed out");

    QCOMPARE(spy.count(), 1);

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(QTestEventLoop::instance().timeout());
    QVERIFY(!proc.waitForReadyRead(250));

    QObject::disconnect(&proc, &QIODevice::readyRead, Q_NULLPTR, Q_NULLPTR);
    proc.write("B");
    QVERIFY(proc.waitForReadyRead(5000));

    proc.write("", 1);
    QVERIFY(proc.waitForFinished(5000));
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QCOMPARE(proc.exitCode(), 0);
}

void tst_QProcess::hardExit()
{
    QProcess proc;

    proc.start("testProcessEcho/testProcessEcho");

    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));

#if defined(Q_OS_QNX)
    // QNX may lose the kill if it's delivered while the forked process
    // is doing the exec that morphs it into testProcessEcho.  It's very
    // unlikely that a normal application would do such a thing.  Make
    // sure the test doesn't accidentally try to do it.
    proc.write("A");
    QVERIFY(proc.waitForReadyRead(5000));
#endif

    proc.kill();

    QVERIFY(proc.waitForFinished(5000));
    QCOMPARE(int(proc.state()), int(QProcess::NotRunning));
    QCOMPARE(int(proc.error()), int(QProcess::Crashed));
}

void tst_QProcess::softExit()
{
    QProcess proc;
    QCOMPARE(proc.processId(), 0);
    proc.start("testSoftExit/testSoftExit");

    QVERIFY(proc.waitForStarted(10000));
    QVERIFY(proc.waitForReadyRead(10000));

    QVERIFY(proc.processId() > 0);

    proc.terminate();

    QVERIFY(proc.waitForFinished(10000));
    QCOMPARE(int(proc.state()), int(QProcess::NotRunning));
    QCOMPARE(int(proc.error()), int(QProcess::UnknownError));
}

class SoftExitProcess : public QProcess
{
    Q_OBJECT
public:
    bool waitedForFinished;

    SoftExitProcess(int n) : waitedForFinished(false), n(n), killing(false)
    {
        connect(this, static_cast<QProcessFinishedSignal2>(&QProcess::finished),
                this, &SoftExitProcess::finishedSlot);

        switch (n) {
        case 0:
            setReadChannelMode(QProcess::MergedChannels);
            connect(this, &QIODevice::readyRead, this, &SoftExitProcess::terminateSlot);
            break;
        case 1:
            connect(this, &QProcess::readyReadStandardOutput,
                    this, &SoftExitProcess::terminateSlot);
            break;
        case 2:
            connect(this, &QProcess::readyReadStandardError,
                    this, &SoftExitProcess::terminateSlot);
            break;
        case 3:
            connect(this, &QProcess::started,
                    this, &SoftExitProcess::terminateSlot);
            break;
        case 4:
            setReadChannelMode(QProcess::MergedChannels);
            connect(this, SIGNAL(channelReadyRead(int)), this, SLOT(terminateSlot()));
            break;
        default:
            connect(this, &QProcess::stateChanged,
                    this, &SoftExitProcess::terminateSlot);
            break;
        }
    }

    void writeAfterStart(const char *buf, int count)
    {
        dataToWrite = QByteArray(buf, count);
    }

    void start(const QString &program)
    {
        QProcess::start(program);
        writePendingData();
    }

public slots:
    void terminateSlot()
    {
        writePendingData(); // In cases 3 and 5 we haven't written the data yet.
        if (killing || (n == 5 && state() != Running)) {
            // Don't try to kill the process before it is running - that can
            // be hazardous, as the actual child process might not be running
            // yet. Also, don't kill it "recursively".
            return;
        }
        killing = true;
        readAll();
        terminate();
        if ((waitedForFinished = waitForFinished(5000)) == false) {
            kill();
            if (state() != NotRunning)
                waitedForFinished = waitForFinished(5000);
        }
    }

    void finishedSlot(int, QProcess::ExitStatus)
    {
        waitedForFinished = true;
    }

private:
    void writePendingData()
    {
        if (!dataToWrite.isEmpty()) {
            write(dataToWrite);
            dataToWrite.clear();
        }
    }

private:
    int n;
    bool killing;
    QByteArray dataToWrite;
};

void tst_QProcess::softExitInSlots_data()
{
    QTest::addColumn<QString>("appName");
    QTest::addColumn<int>("signalToConnect");

    QByteArray dataTagPrefix("gui app ");
#ifndef QT_NO_WIDGETS
    for (int i = 0; i < 6; ++i) {
        QTest::newRow(dataTagPrefix + QByteArray::number(i))
                << "testGuiProcess/testGuiProcess" << i;
    }
#endif

    dataTagPrefix = "console app ";
    for (int i = 0; i < 6; ++i) {
        QTest::newRow(dataTagPrefix + QByteArray::number(i))
                << "testProcessEcho2/testProcessEcho2" << i;
    }
}

void tst_QProcess::softExitInSlots()
{
    QFETCH(QString, appName);
    QFETCH(int, signalToConnect);

    SoftExitProcess proc(signalToConnect);
    proc.writeAfterStart("OLEBOLE", 8); // include the \0
    proc.start(appName);
    QTRY_VERIFY_WITH_TIMEOUT(proc.waitedForFinished, 10000);
    QCOMPARE(proc.state(), QProcess::NotRunning);
}

void tst_QProcess::mergedChannels()
{
    QProcess process;
    process.setReadChannelMode(QProcess::MergedChannels);
    QCOMPARE(process.readChannelMode(), QProcess::MergedChannels);

    process.start("testProcessEcho2/testProcessEcho2");

    QVERIFY(process.waitForStarted(5000));

    for (int i = 0; i < 100; ++i) {
        QCOMPARE(process.write("abc"), qlonglong(3));
        while (process.bytesAvailable() < 6)
            QVERIFY(process.waitForReadyRead(5000));
        QCOMPARE(process.readAll(), QByteArray("aabbcc"));
    }

    process.closeWriteChannel();
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::forwardedChannels_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("inmode");
    QTest::addColumn<QByteArray>("outdata");
    QTest::addColumn<QByteArray>("errdata");

    QTest::newRow("separate") << int(QProcess::SeparateChannels) << int(QProcess::ManagedInputChannel)
                              << QByteArray() << QByteArray();
    QTest::newRow("forwarded") << int(QProcess::ForwardedChannels) << int(QProcess::ManagedInputChannel)
                               << QByteArray("forwarded") << QByteArray("forwarded");
    QTest::newRow("stdout") << int(QProcess::ForwardedOutputChannel) << int(QProcess::ManagedInputChannel)
                            << QByteArray("forwarded") << QByteArray();
    QTest::newRow("stderr") << int(QProcess::ForwardedErrorChannel) << int(QProcess::ManagedInputChannel)
                            << QByteArray() << QByteArray("forwarded");
    QTest::newRow("fwdinput") << int(QProcess::ForwardedErrorChannel) << int(QProcess::ForwardedInputChannel)
                            << QByteArray() << QByteArray("input");
}

void tst_QProcess::forwardedChannels()
{
    QFETCH(int, mode);
    QFETCH(int, inmode);
    QFETCH(QByteArray, outdata);
    QFETCH(QByteArray, errdata);

    QProcess process;
    process.start("testForwarding/testForwarding", QStringList() << QString::number(mode) << QString::number(inmode));
    QVERIFY(process.waitForStarted(5000));
    QCOMPARE(process.write("input"), 5);
    process.closeWriteChannel();
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    const char *err;
    switch (process.exitCode()) {
    case 0: err = "ok"; break;
    case 1: err = "processChannelMode is wrong"; break;
    case 11: err = "inputChannelMode is wrong"; break;
    case 2: err = "failed to start"; break;
    case 3: err = "failed to write"; break;
    case 4: err = "did not finish"; break;
    case 5: err = "unexpected stdout"; break;
    case 6: err = "unexpected stderr"; break;
    case 13: err = "parameter error"; break;
    default: err = "unknown exit code"; break;
    }
    QVERIFY2(!process.exitCode(), err);
    QCOMPARE(process.readAllStandardOutput(), outdata);
    QCOMPARE(process.readAllStandardError(), errdata);
}

void tst_QProcess::atEnd()
{
    QProcess process;

    process.start("testProcessEcho/testProcessEcho");
    process.write("abcdefgh\n");

    while (process.bytesAvailable() < 8)
        QVERIFY(process.waitForReadyRead(5000));

    QTextStream stream(&process);
    QVERIFY(!stream.atEnd());
    QString tmp = stream.readLine();
    QVERIFY(stream.atEnd());
    QCOMPARE(tmp, QString::fromLatin1("abcdefgh"));

    process.write("", 1);
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

class TestThread : public QThread
{
    Q_OBJECT
public:
    inline int code()
    {
        return exitCode;
    }

protected:
    inline void run()
    {
        exitCode = 90210;

        QProcess process;
        connect(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished),
                this, &TestThread::catchExitCode, Qt::DirectConnection);

        process.start("testProcessEcho/testProcessEcho");

        QCOMPARE(process.write("abc\0", 4), qint64(4));
        exitCode = exec();
    }

protected slots:
    inline void catchExitCode(int exitCode)
    {
        this->exitCode = exitCode;
        exit(exitCode);
    }

private:
    int exitCode;
};

void tst_QProcess::processInAThread()
{
    for (int i = 0; i < 10; ++i) {
        TestThread thread;
        thread.start();
        QVERIFY(thread.wait(10000));
        QCOMPARE(thread.code(), 0);
    }
}

void tst_QProcess::processesInMultipleThreads()
{
    for (int i = 0; i < 10; ++i) {
        // run from 1 to 10 threads, but run at least some tests
        // with more threads than the ideal
        int threadCount = i;
        if (i > 7)
            threadCount = qMax(threadCount, QThread::idealThreadCount() + 2);

        QVector<TestThread *> threads(threadCount);
        for (int j = 0; j < threadCount; ++j)
            threads[j] = new TestThread;
        for (int j = 0; j < threadCount; ++j)
            threads[j]->start();
        for (int j = 0; j < threadCount; ++j) {
            QVERIFY(threads[j]->wait(10000));
        }
        for (int j = 0; j < threadCount; ++j) {
            QCOMPARE(threads[j]->code(), 0);
        }
        qDeleteAll(threads);
    }
}

void tst_QProcess::waitForFinishedWithTimeout()
{
    QProcess process;

    process.start("testProcessEcho/testProcessEcho");

    QVERIFY(process.waitForStarted(5000));
    QVERIFY(!process.waitForFinished(1));

    process.write("", 1);

    QVERIFY(process.waitForFinished());
}

void tst_QProcess::waitForReadyReadInAReadyReadSlot()
{
    QProcess process;
    connect(&process, &QIODevice::readyRead, this, &tst_QProcess::waitForReadyReadInAReadyReadSlotSlot);
    connect(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished),
            this, &tst_QProcess::exitLoopSlot);
    bytesAvailable = 0;

    process.start("testProcessEcho/testProcessEcho");
    QVERIFY(process.waitForStarted(5000));

    QSignalSpy spy(&process, &QProcess::readyRead);
    QVERIFY(spy.isValid());
    process.write("foo");
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spy.count(), 1);

    process.disconnect();
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QVERIFY(process.bytesAvailable() > bytesAvailable);
}

void tst_QProcess::waitForReadyReadInAReadyReadSlotSlot()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    QVERIFY(process);
    bytesAvailable = process->bytesAvailable();
    process->write("bar", 4);
    QVERIFY(process->waitForReadyRead(5000));
    QTestEventLoop::instance().exitLoop();
}

void tst_QProcess::waitForBytesWrittenInABytesWrittenSlot()
{
    QProcess process;
    connect(&process, &QIODevice::bytesWritten, this, &tst_QProcess::waitForBytesWrittenInABytesWrittenSlotSlot);
    bytesAvailable = 0;

    process.start("testProcessEcho/testProcessEcho");
    QVERIFY(process.waitForStarted(5000));

    QSignalSpy spy(&process, &QProcess::bytesWritten);
    QVERIFY(spy.isValid());
    process.write("f");
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spy.count(), 1);
    process.write("", 1);
    process.disconnect();
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::waitForBytesWrittenInABytesWrittenSlotSlot()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    QVERIFY(process);
    process->write("b");
    QVERIFY(process->waitForBytesWritten(5000));
    QTestEventLoop::instance().exitLoop();
}

void tst_QProcess::spaceArgsTest_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QString>("stringArgs");

    // arg1 | arg2
    QTest::newRow("arg1 arg2") << (QStringList() << QString::fromLatin1("arg1") << QString::fromLatin1("arg2"))
                               << QString::fromLatin1("arg1 arg2");
    // "arg1" | ar "g2
    QTest::newRow("\"\"\"\"arg1\"\"\"\" \"ar \"\"\"g2\"") << (QStringList() << QString::fromLatin1("\"arg1\"") << QString::fromLatin1("ar \"g2"))
                                                          << QString::fromLatin1("\"\"\"\"arg1\"\"\"\" \"ar \"\"\"g2\"");
    // ar g1 | a rg 2
    QTest::newRow("\"ar g1\" \"a rg 2\"") << (QStringList() << QString::fromLatin1("ar g1") << QString::fromLatin1("a rg 2"))
                                          << QString::fromLatin1("\"ar g1\" \"a rg 2\"");
    // -lar g1 | -l"ar g2"
    QTest::newRow("\"-lar g1\" \"-l\"\"\"ar g2\"\"\"\"") << (QStringList() << QString::fromLatin1("-lar g1") << QString::fromLatin1("-l\"ar g2\""))
                                                         << QString::fromLatin1("\"-lar g1\" \"-l\"\"\"ar g2\"\"\"\"");
    // ar"g1
    QTest::newRow("ar\"\"\"\"g1") << (QStringList() << QString::fromLatin1("ar\"g1"))
                                  << QString::fromLatin1("ar\"\"\"\"g1");
    // ar/g1
    QTest::newRow("ar\\g1") << (QStringList() << QString::fromLatin1("ar\\g1"))
                            << QString::fromLatin1("ar\\g1");
    // ar\g"1
    QTest::newRow("ar\\g\"\"\"\"1") << (QStringList() << QString::fromLatin1("ar\\g\"1"))
                                    << QString::fromLatin1("ar\\g\"\"\"\"1");
    // arg\"1
    QTest::newRow("arg\\\"\"\"1") << (QStringList() << QString::fromLatin1("arg\\\"1"))
                                  << QString::fromLatin1("arg\\\"\"\"1");
    // """"
    QTest::newRow("\"\"\"\"\"\"\"\"\"\"\"\"") << (QStringList() << QString::fromLatin1("\"\"\"\""))
                                              << QString::fromLatin1("\"\"\"\"\"\"\"\"\"\"\"\"");
    // """" | "" ""
    QTest::newRow("\"\"\"\"\"\"\"\"\"\"\"\" \"\"\"\"\"\"\" \"\"\"\"\"\"\"") << (QStringList() << QString::fromLatin1("\"\"\"\"") << QString::fromLatin1("\"\" \"\""))
                                                                            << QString::fromLatin1("\"\"\"\"\"\"\"\"\"\"\"\" \"\"\"\"\"\"\" \"\"\"\"\"\"\"");
    // ""  ""
    QTest::newRow("\"\"\"\"\"\"\" \"\" \"\"\"\"\"\"\" (bogus double quotes)") << (QStringList() << QString::fromLatin1("\"\"  \"\""))
                                                                              << QString::fromLatin1("\"\"\"\"\"\"\" \"\" \"\"\"\"\"\"\"");
    // ""  ""
    QTest::newRow(" \"\"\"\"\"\"\" \"\" \"\"\"\"\"\"\"   (bogus double quotes)") << (QStringList() << QString::fromLatin1("\"\"  \"\""))
                                                                                 << QString::fromLatin1(" \"\"\"\"\"\"\" \"\" \"\"\"\"\"\"\"   ");
}

static QByteArray startFailMessage(const QString &program, const QProcess &process)
{
    QByteArray result = "Process '";
    result += program.toLocal8Bit();
    result += "' failed to start: ";
    result += process.errorString().toLocal8Bit();
    return result;
}

void tst_QProcess::spaceArgsTest()
{
    QFETCH(QStringList, args);
    QFETCH(QString, stringArgs);

    QStringList programs;
    programs << QString::fromLatin1("testProcessSpacesArgs/nospace")
             << QString::fromLatin1("testProcessSpacesArgs/one space")
             << QString::fromLatin1("testProcessSpacesArgs/two space s");

    QProcess process;

    for (int i = 0; i < programs.size(); ++i) {
        QString program = programs.at(i);
        process.start(program, args);

        QByteArray errorMessage;
        bool started = process.waitForStarted();
        if (!started)
            errorMessage = startFailMessage(program, process);
        QVERIFY2(started, errorMessage.constData());
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);

        QStringList actual = QString::fromLatin1(process.readAll()).split("|");
        QVERIFY(!actual.isEmpty());
        // not interested in the program name, it might be different.
        actual.removeFirst();

        QCOMPARE(actual, args);

        if (program.contains(QLatin1Char(' ')))
            program = QLatin1Char('"') + program + QLatin1Char('"');

        if (!stringArgs.isEmpty())
            program += QLatin1Char(' ') + stringArgs;

        errorMessage.clear();
        process.start(program);
        started = process.waitForStarted(5000);
        if (!started)
            errorMessage = startFailMessage(program, process);

        QVERIFY2(started, errorMessage.constData());
        QVERIFY(process.waitForFinished(5000));

        actual = QString::fromLatin1(process.readAll()).split("|");
        QVERIFY(!actual.isEmpty());
        // not interested in the program name, it might be different.
        actual.removeFirst();

        QCOMPARE(actual, args);
    }
}

#if defined(Q_OS_WIN)

void tst_QProcess::nativeArguments()
{
    QProcess proc;

    // This doesn't actually need special quoting, so it is pointless to use
    // native arguments here, but that's not the point of this test.
    proc.setNativeArguments("hello kitty, \"*\"!");

    proc.start(QString::fromLatin1("testProcessSpacesArgs/nospace"), QStringList());

    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QCOMPARE(proc.exitCode(), 0);

    QStringList actual = QString::fromLatin1(proc.readAll()).split(QLatin1Char('|'));
    QVERIFY(!actual.isEmpty());
    // not interested in the program name, it might be different.
    actual.removeFirst();
    QStringList expected;
    expected << "hello" << "kitty," << "*!";
    QCOMPARE(actual, expected);
}

void tst_QProcess::createProcessArgumentsModifier()
{
    int calls = 0;
    const QString reversedCommand = "lamroNssecorPtset/lamroNssecorPtset";
    QProcess process;
    process.setCreateProcessArgumentsModifier([&calls] (QProcess::CreateProcessArguments *args)
    {
        calls++;
        std::reverse(args->arguments, args->arguments + wcslen(args->arguments) - 1);
    });
    process.start(reversedCommand);
    QVERIFY2(process.waitForStarted(), qUtf8Printable(process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(calls, 1);

    process.setCreateProcessArgumentsModifier(QProcess::CreateProcessArgumentModifier());
    QVERIFY(!process.waitForStarted());
    QCOMPARE(calls, 1);
}
#endif // Q_OS_WIN

void tst_QProcess::exitCodeTest()
{
    for (int i = 0; i < 255; ++i) {
        QProcess process;
        process.start("testExitCodes/testExitCodes " + QString::number(i));
        QVERIFY(process.waitForFinished(5000));
        QCOMPARE(process.exitCode(), i);
        QCOMPARE(process.error(), QProcess::UnknownError);
    }
}

void tst_QProcess::failToStart()
{
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");

    QProcess process;
    QSignalSpy stateSpy(&process, &QProcess::stateChanged);
    QSignalSpy errorSpy(&process, &QProcess::errorOccurred);
    QSignalSpy errorSpy2(&process, static_cast<QProcessErrorSignal>(&QProcess::error));
    QSignalSpy finishedSpy(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished));
    QSignalSpy finishedSpy2(&process, static_cast<QProcessFinishedSignal2>(&QProcess::finished));

    QVERIFY(stateSpy.isValid());
    QVERIFY(errorSpy.isValid());
    QVERIFY(errorSpy2.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(finishedSpy2.isValid());

// OS X and HP-UX have a really low default process limit (~100), so spawning
// to many processes here will cause test failures later on.
#if defined Q_OS_HPUX
   const int attempts = 15;
#elif defined Q_OS_MAC
   const int attempts = 15;
#else
   const int attempts = 50;
#endif

    for (int j = 0; j < 8; ++j) {
        for (int i = 0; i < attempts; ++i) {
            QCOMPARE(errorSpy.count(), j * attempts + i);
            QCOMPARE(errorSpy2.count(), j * attempts + i);
            process.start("/blurp");

            switch (j) {
            case 0:
            case 1:
                QVERIFY(!process.waitForStarted());
                break;
            case 2:
            case 3:
                QVERIFY(!process.waitForFinished());
                break;
            case 4:
            case 5:
                QVERIFY(!process.waitForReadyRead());
                break;
            case 6:
            case 7:
            default:
                QVERIFY(!process.waitForBytesWritten());
                break;
            }

            QCOMPARE(process.error(), QProcess::FailedToStart);
            QCOMPARE(errorSpy.count(), j * attempts + i + 1);
            QCOMPARE(errorSpy2.count(), j * attempts + i + 1);
            QCOMPARE(finishedSpy.count(), 0);
            QCOMPARE(finishedSpy2.count(), 0);

            int it = j * attempts + i + 1;

            QCOMPARE(stateSpy.count(), it * 2);
            QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(it * 2 - 2).at(0)), QProcess::Starting);
            QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(it * 2 - 1).at(0)), QProcess::NotRunning);
        }
    }
}

void tst_QProcess::failToStartWithWait()
{
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QProcess process;
    QEventLoop loop;
    QSignalSpy errorSpy(&process, &QProcess::errorOccurred);
    QSignalSpy errorSpy2(&process, static_cast<QProcessErrorSignal>(&QProcess::error));
    QSignalSpy finishedSpy(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished));
    QSignalSpy finishedSpy2(&process, static_cast<QProcessFinishedSignal2>(&QProcess::finished));

    QVERIFY(errorSpy.isValid());
    QVERIFY(errorSpy2.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(finishedSpy2.isValid());

    for (int i = 0; i < 50; ++i) {
        process.start("/blurp", QStringList() << "-v" << "-debug");
        process.waitForStarted();

        QCOMPARE(process.error(), QProcess::FailedToStart);
        QCOMPARE(errorSpy.count(), i + 1);
        QCOMPARE(errorSpy2.count(), i + 1);
        QCOMPARE(finishedSpy.count(), 0);
        QCOMPARE(finishedSpy2.count(), 0);
    }
}

void tst_QProcess::failToStartWithEventLoop()
{
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QProcess process;
    QEventLoop loop;
    QSignalSpy errorSpy(&process, &QProcess::errorOccurred);
    QSignalSpy errorSpy2(&process, static_cast<QProcessErrorSignal>(&QProcess::error));
    QSignalSpy finishedSpy(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished));
    QSignalSpy finishedSpy2(&process, static_cast<QProcessFinishedSignal2>(&QProcess::finished));

    QVERIFY(errorSpy.isValid());
    QVERIFY(errorSpy2.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(finishedSpy2.isValid());

    // The error signal may be emitted before start() returns
    connect(&process, &QProcess::errorOccurred, &loop, &QEventLoop::quit, Qt::QueuedConnection);


    for (int i = 0; i < 50; ++i) {
        process.start("/blurp", QStringList() << "-v" << "-debug");

        loop.exec();

        QCOMPARE(process.error(), QProcess::FailedToStart);
        QCOMPARE(errorSpy.count(), i + 1);
        QCOMPARE(errorSpy2.count(), i + 1);
        QCOMPARE(finishedSpy.count(), 0);
        QCOMPARE(finishedSpy2.count(), 0);
    }
}

void tst_QProcess::failToStartEmptyArgs_data()
{
    QTest::addColumn<int>("startOverload");
    QTest::newRow("start(QString, QStringList, OpenMode)") << 0;
    QTest::newRow("start(QString, OpenMode)") << 1;
    QTest::newRow("start(OpenMode)") << 2;
}

void tst_QProcess::failToStartEmptyArgs()
{
    QFETCH(int, startOverload);
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");

    QProcess process;
    QSignalSpy errorSpy(&process, static_cast<QProcessErrorSignal>(&QProcess::error));
    QVERIFY(errorSpy.isValid());

    switch (startOverload) {
    case 0:
        process.start(QString(), QStringList(), QIODevice::ReadWrite);
        break;
    case 1:
        process.start(QString(), QIODevice::ReadWrite);
        break;
    case 2:
        process.start(QIODevice::ReadWrite);
        break;
    default:
        QFAIL("Unhandled QProcess::start overload.");
    };

    QVERIFY(!process.waitForStarted());
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(process.error(), QProcess::FailedToStart);
}

void tst_QProcess::removeFileWhileProcessIsRunning()
{
    QFile file(m_temporaryDir.path() + QLatin1String("/removeFile.txt"));
    QVERIFY(file.open(QFile::WriteOnly));

    QProcess process;
    process.start("testProcessEcho/testProcessEcho");

    QVERIFY(process.waitForStarted(5000));

    QVERIFY(file.remove());

    process.write("", 1);
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::setEnvironment_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("value");

    QTest::newRow("setting-empty") << "tst_QProcess" << "";
    QTest::newRow("setting") << "tst_QProcess" << "value";

#ifdef Q_OS_WIN
    QTest::newRow("unsetting") << "PROMPT" << QString();
    QTest::newRow("overriding") << "PROMPT" << "value";
#else
    QTest::newRow("unsetting") << "PATH" << QString();
    QTest::newRow("overriding") << "PATH" << "value";
#endif
}

void tst_QProcess::setEnvironment()
{
    // make sure our environment variables are correct
    QVERIFY(qgetenv("tst_QProcess").isEmpty());
    QVERIFY(!qgetenv("PATH").isEmpty());
#ifdef Q_OS_WIN
    QVERIFY(!qgetenv("PROMPT").isEmpty());
#endif

    QFETCH(QString, name);
    QFETCH(QString, value);
    QString executable = QDir::currentPath() + "/testProcessEnvironment/testProcessEnvironment";

    {
        QProcess process;
        QStringList environment = QProcess::systemEnvironment();
        if (value.isNull()) {
            int pos;
            QRegExp rx(name + "=.*");
#ifdef Q_OS_WIN
            rx.setCaseSensitivity(Qt::CaseInsensitive);
#endif
            while ((pos = environment.indexOf(rx)) != -1)
                environment.removeAt(pos);
        } else {
            environment.append(name + '=' + value);
        }
        process.setEnvironment(environment);
        process.start(executable, QStringList() << name);

        QVERIFY(process.waitForFinished());
        if (value.isNull())
            QCOMPARE(process.exitCode(), 1);
        else if (!value.isEmpty())
            QCOMPARE(process.exitCode(), 0);

        QCOMPARE(process.readAll(), value.toLocal8Bit());
    }

    // re-do the test but set the environment twice, to make sure
    // that the latter addition overrides
    // this test doesn't make sense in unsetting
    if (!value.isNull()) {
        QProcess process;
        QStringList environment = QProcess::systemEnvironment();
        environment.prepend(name + "=This is not the right value");
        environment.append(name + '=' + value);
        process.setEnvironment(environment);
        process.start(executable, QStringList() << name);

        QVERIFY(process.waitForFinished());
        if (!value.isEmpty())
            QCOMPARE(process.exitCode(), 0);

        QCOMPARE(process.readAll(), value.toLocal8Bit());
    }
}

void tst_QProcess::setProcessEnvironment_data()
{
    setEnvironment_data();
}

void tst_QProcess::setProcessEnvironment()
{
    // make sure our environment variables are correct
    QVERIFY(qgetenv("tst_QProcess").isEmpty());
    QVERIFY(!qgetenv("PATH").isEmpty());
#ifdef Q_OS_WIN
    QVERIFY(!qgetenv("PROMPT").isEmpty());
#endif

    QFETCH(QString, name);
    QFETCH(QString, value);
    QString executable = QDir::currentPath() + "/testProcessEnvironment/testProcessEnvironment";

    {
        QProcess process;
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        if (value.isNull())
            environment.remove(name);
        else
            environment.insert(name, value);
        process.setProcessEnvironment(environment);
        process.start(executable, QStringList() << name);

        QVERIFY(process.waitForFinished());
        if (value.isNull())
            QCOMPARE(process.exitCode(), 1);
        else if (!value.isEmpty())
            QCOMPARE(process.exitCode(), 0);

        QCOMPARE(process.readAll(), value.toLocal8Bit());
    }
}

void tst_QProcess::systemEnvironment()
{
    QVERIFY(!QProcess::systemEnvironment().isEmpty());
    QVERIFY(!QProcessEnvironment::systemEnvironment().isEmpty());

    QVERIFY(QProcessEnvironment::systemEnvironment().contains("PATH"));
    QVERIFY(!QProcess::systemEnvironment().filter(QRegExp("^PATH=", Qt::CaseInsensitive)).isEmpty());
}

void tst_QProcess::spaceInName()
{
    QProcess process;
    process.start("test Space In Name/testSpaceInName", QStringList());
    QVERIFY(process.waitForStarted());
    process.write("", 1);
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_QProcess::lockupsInStartDetached()
{
    // Check that QProcess doesn't cause a lock up at this program's
    // exit if a thread was started and we tried to run a program that
    // doesn't exist. Before Qt 4.2, this used to lock up on Unix due
    // to calling ::exit instead of ::_exit if execve failed.

    QObject *dummy = new QObject(this);
    QHostInfo::lookupHost(QString("something.invalid"), dummy, SLOT(deleteLater()));
    QProcess::execute("yjhbrty");
    QProcess::startDetached("yjhbrty");
}

void tst_QProcess::atEnd2()
{
    QProcess process;

    process.start("testProcessEcho/testProcessEcho");
    process.write("Foo\nBar\nBaz\nBodukon\nHadukan\nTorwukan\nend\n");
    process.putChar('\0');
    QVERIFY(process.waitForFinished());
    QList<QByteArray> lines;
    while (!process.atEnd()) {
        lines << process.readLine();
    }
    QCOMPARE(lines.size(), 7);
}

void tst_QProcess::waitForReadyReadForNonexistantProcess()
{
    // Start a program that doesn't exist, process events and then try to waitForReadyRead
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QProcess process;
    QSignalSpy errorSpy(&process, &QProcess::errorOccurred);
    QSignalSpy errorSpy2(&process, static_cast<QProcessErrorSignal>(&QProcess::error));
    QSignalSpy finishedSpy1(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished));
    QSignalSpy finishedSpy2(&process, static_cast<QProcessFinishedSignal2>(&QProcess::finished));

    QVERIFY(errorSpy.isValid());
    QVERIFY(errorSpy2.isValid());
    QVERIFY(finishedSpy1.isValid());
    QVERIFY(finishedSpy2.isValid());

    QVERIFY(!process.waitForReadyRead()); // used to crash
    process.start("doesntexist");
    QVERIFY(!process.waitForReadyRead());
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(errorSpy2.count(), 1);
    QCOMPARE(errorSpy2.at(0).at(0).toInt(), 0);
    QCOMPARE(finishedSpy1.count(), 0);
    QCOMPARE(finishedSpy2.count(), 0);
}

void tst_QProcess::setStandardInputFile()
{
    static const char data[] = "A bunch\1of\2data\3\4\5\6\7...";
    QProcess process;
    QFile file(m_temporaryDir.path() + QLatin1String("/data-sif"));

    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data, sizeof data);
    file.close();

    process.setStandardInputFile(file.fileName());
    process.start("testProcessEcho/testProcessEcho");

    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
        QByteArray all = process.readAll();
    QCOMPARE(all.size(), int(sizeof data) - 1); // testProcessEcho drops the ending \0
    QVERIFY(all == data);

    QProcess process2;
    process2.setStandardInputFile(QProcess::nullDevice());
    process2.start("testProcessEcho/testProcessEcho");
    QVERIFY(process2.waitForFinished());
    all = process2.readAll();
    QCOMPARE(all.size(), 0);
}

void tst_QProcess::setStandardOutputFile_data()
{
    QTest::addColumn<int>("channelToTest");
    QTest::addColumn<int>("_channelMode");
    QTest::addColumn<bool>("append");

    QTest::newRow("stdout-truncate") << int(QProcess::StandardOutput)
                                     << int(QProcess::SeparateChannels)
                                     << false;
    QTest::newRow("stdout-append") << int(QProcess::StandardOutput)
                                   << int(QProcess::SeparateChannels)
                                   << true;

    QTest::newRow("stderr-truncate") << int(QProcess::StandardError)
                                     << int(QProcess::SeparateChannels)
                                     << false;
    QTest::newRow("stderr-append") << int(QProcess::StandardError)
                                   << int(QProcess::SeparateChannels)
                                   << true;

    QTest::newRow("merged-truncate") << int(QProcess::StandardOutput)
                                     << int(QProcess::MergedChannels)
                                     << false;
    QTest::newRow("merged-append") << int(QProcess::StandardOutput)
                                   << int(QProcess::MergedChannels)
                                   << true;
}

void tst_QProcess::setStandardOutputFile()
{
    static const char data[] = "Original data. ";
    static const char testdata[] = "Test data.";

    QFETCH(int, channelToTest);
    QFETCH(int, _channelMode);
    QFETCH(bool, append);

    QProcess::ProcessChannelMode channelMode = QProcess::ProcessChannelMode(_channelMode);
    QIODevice::OpenMode mode = append ? QIODevice::Append : QIODevice::Truncate;

    // create the destination file with data
    QFile file(m_temporaryDir.path() + QLatin1String("/data-stdof-") + QLatin1String(QTest::currentDataTag()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data, sizeof data - 1);
    file.close();

    // run the process
    QProcess process;
    process.setReadChannelMode(channelMode);
    if (channelToTest == QProcess::StandardOutput)
        process.setStandardOutputFile(file.fileName(), mode);
    else
        process.setStandardErrorFile(file.fileName(), mode);

    process.start("testProcessEcho2/testProcessEcho2");
    process.write(testdata, sizeof testdata);
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    // open the file again and verify the data
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray all = file.readAll();
    file.close();

    int expectedsize = sizeof testdata - 1;
    if (mode == QIODevice::Append) {
        QVERIFY(all.startsWith(data));
        expectedsize += sizeof data - 1;
    }
    if (channelMode == QProcess::MergedChannels) {
        expectedsize += sizeof testdata - 1;
    } else {
        QVERIFY(all.endsWith(testdata));
    }

    QCOMPARE(all.size(), expectedsize);
}

void tst_QProcess::setStandardOutputFileNullDevice()
{
    static const char testdata[] = "Test data.";

    QProcess process;
    process.setStandardOutputFile(QProcess::nullDevice());
    process.start("testProcessEcho2/testProcessEcho2");
    process.write(testdata, sizeof testdata);
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(process.bytesAvailable(), Q_INT64_C(0));

    QVERIFY(!QFileInfo(QProcess::nullDevice()).isFile());
}

void tst_QProcess::setStandardOutputFileAndWaitForBytesWritten()
{
    static const char testdata[] = "Test data.";

    QFile file(m_temporaryDir.path() + QLatin1String("/data-stdofawfbw"));
    QProcess process;
    process.setStandardOutputFile(file.fileName());
    process.start("testProcessEcho2/testProcessEcho2");
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    process.write(testdata, sizeof testdata);
    process.waitForBytesWritten();
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    // open the file again and verify the data
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray all = file.readAll();
    file.close();

    QCOMPARE(all, QByteArray::fromRawData(testdata, sizeof testdata - 1));
}

void tst_QProcess::setStandardOutputProcess_data()
{
    QTest::addColumn<bool>("merged");
    QTest::addColumn<bool>("waitForBytesWritten");
    QTest::newRow("separate") << false << false;
    QTest::newRow("separate with waitForBytesWritten") << false << true;
    QTest::newRow("merged") << true << false;
}

void tst_QProcess::setStandardOutputProcess()
{
    QProcess source;
    QProcess sink;

    QFETCH(bool, merged);
    QFETCH(bool, waitForBytesWritten);
    source.setReadChannelMode(merged ? QProcess::MergedChannels : QProcess::SeparateChannels);
    source.setStandardOutputProcess(&sink);

    source.start("testProcessEcho2/testProcessEcho2");
    sink.start("testProcessEcho2/testProcessEcho2");

    QByteArray data("Hello, World");
    source.write(data);
    if (waitForBytesWritten)
        source.waitForBytesWritten();
    source.closeWriteChannel();
    QVERIFY(source.waitForFinished());
    QCOMPARE(source.exitStatus(), QProcess::NormalExit);
    QCOMPARE(source.exitCode(), 0);
    QVERIFY(sink.waitForFinished());
    QCOMPARE(sink.exitStatus(), QProcess::NormalExit);
    QCOMPARE(sink.exitCode(), 0);
    QByteArray all = sink.readAll();

    if (!merged)
        QCOMPARE(all, data);
    else
        QCOMPARE(all, QByteArray("HHeelllloo,,  WWoorrlldd"));
}

void tst_QProcess::fileWriterProcess()
{
    const QByteArray line = QByteArrayLiteral(" -- testing testing 1 2 3\n");
    QByteArray stdinStr;
    stdinStr.reserve(5000 * (4 + line.size()) + 1);
    for (int i = 0; i < 5000; ++i) {
        stdinStr += QByteArray::number(i);
        stdinStr += line;
    }

    QTime stopWatch;
    stopWatch.start();
    const QString fileName = m_temporaryDir.path() + QLatin1String("/fileWriterProcess.txt");
    const QString binary = QDir::currentPath() + QLatin1String("/fileWriterProcess/fileWriterProcess");

    do {
        if (QFile::exists(fileName))
            QVERIFY(QFile::remove(fileName));
        QProcess process;
        process.setWorkingDirectory(m_temporaryDir.path());
        process.start(binary, QIODevice::ReadWrite | QIODevice::Text);
        process.write(stdinStr);
        process.closeWriteChannel();
        while (process.bytesToWrite()) {
            QVERIFY(stopWatch.elapsed() < 3500);
            QVERIFY(process.waitForBytesWritten(2000));
        }
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(QFile(fileName).size(), qint64(stdinStr.size()));
    } while (stopWatch.elapsed() < 3000);
}

void tst_QProcess::detachedWorkingDirectoryAndPid()
{
    qint64 pid;

    QFile infoFile(m_temporaryDir.path() + QLatin1String("/detachedinfo.txt"));
    if (infoFile.exists())
        QVERIFY(infoFile.remove());

    QString workingDir = QDir::currentPath() + "/testDetached";

    QVERIFY(QFile::exists(workingDir));

    QStringList args;
    args << infoFile.fileName();
    QVERIFY(QProcess::startDetached(QDir::currentPath() + QLatin1String("/testDetached/testDetached"), args, workingDir, &pid));

    QFileInfo fi(infoFile);
    fi.setCaching(false);
    //The guard counter ensures the test does not hang if the sub process fails.
    //Instead, the test will fail when trying to open & verify the sub process output file.
    for (int guard = 0; guard < 100 && fi.size() == 0; guard++) {
        QTest::qSleep(100);
    }

    QVERIFY(infoFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString actualWorkingDir = QString::fromUtf8(infoFile.readLine());
    actualWorkingDir.chop(1); // strip off newline
    QByteArray processIdString = infoFile.readLine();
    processIdString.chop(1);
    infoFile.close();
    infoFile.remove();

    bool ok = false;
    qint64 actualPid = processIdString.toLongLong(&ok);
    QVERIFY(ok);

    QCOMPARE(actualWorkingDir, workingDir);
    QCOMPARE(actualPid, pid);
}

void tst_QProcess::switchReadChannels()
{
    const char data[] = "ABCD";

    QProcess process;

    process.start("testProcessEcho2/testProcessEcho2");
    process.write(data);
    process.closeWriteChannel();
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    for (int i = 0; i < 4; ++i) {
        process.setReadChannel(QProcess::StandardOutput);
        QCOMPARE(process.read(1), QByteArray(&data[i], 1));
        process.setReadChannel(QProcess::StandardError);
        QCOMPARE(process.read(1), QByteArray(&data[i], 1));
    }

    process.ungetChar('D');
    process.setReadChannel(QProcess::StandardOutput);
    process.ungetChar('D');
    process.setReadChannel(QProcess::StandardError);
    QCOMPARE(process.read(1), QByteArray("D"));
    process.setReadChannel(QProcess::StandardOutput);
    QCOMPARE(process.read(1), QByteArray("D"));
}

void tst_QProcess::discardUnwantedOutput()
{
    QProcess process;

    process.setProgram("testProcessEcho2/testProcessEcho2");
    process.start(QIODevice::WriteOnly);
    process.write("Hello, World");
    process.closeWriteChannel();
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    process.setReadChannel(QProcess::StandardOutput);
    QCOMPARE(process.bytesAvailable(), Q_INT64_C(0));
    process.setReadChannel(QProcess::StandardError);
    QCOMPARE(process.bytesAvailable(), Q_INT64_C(0));
}

// Q_OS_WIN - setWorkingDirectory will chdir before starting the process on unices
void tst_QProcess::setWorkingDirectory()
{
    QProcess process;
    process.setWorkingDirectory("test");

    // use absolute path because on Windows, the executable is relative to the parent's CWD
    // while on Unix with fork it's relative to the child's (with posix_spawn, it could be either).
    process.start(QFileInfo("testSetWorkingDirectory/testSetWorkingDirectory").absoluteFilePath());

    QVERIFY2(process.waitForFinished(), process.errorString().toLocal8Bit());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    QByteArray workingDir = process.readAllStandardOutput();
    QCOMPARE(QDir("test").canonicalPath(), QDir(workingDir.constData()).canonicalPath());
}

void tst_QProcess::setNonExistentWorkingDirectory()
{
    QProcess process;
    process.setWorkingDirectory("this/directory/should/not/exist/for/sure");

    // use absolute path because on Windows, the executable is relative to the parent's CWD
    // while on Unix with fork it's relative to the child's (with posix_spawn, it could be either).
    process.start(QFileInfo("testSetWorkingDirectory/testSetWorkingDirectory").absoluteFilePath());
    QVERIFY(!process.waitForFinished());
    QCOMPARE(int(process.error()), int(QProcess::FailedToStart));

#ifdef Q_OS_UNIX
#  ifdef QPROCESS_USE_SPAWN
    QEXPECT_FAIL("", "QProcess cannot detect failure to start when using posix_spawn()", Continue);
#  endif
    QVERIFY2(process.errorString().startsWith("chdir:"), process.errorString().toLocal8Bit());
#endif
}

void tst_QProcess::startFinishStartFinish()
{
    QProcess process;

    for (int i = 0; i < 3; ++i) {
        QCOMPARE(process.state(), QProcess::NotRunning);

        process.start("testProcessOutput/testProcessOutput");
        QVERIFY(process.waitForReadyRead(10000));
        QCOMPARE(QString::fromLatin1(process.readLine().trimmed()),
                 QString("0 -this is a number"));
        if (process.state() != QProcess::NotRunning) {
            QVERIFY(process.waitForFinished(10000));
            QCOMPARE(process.exitStatus(), QProcess::NormalExit);
            QCOMPARE(process.exitCode(), 0);
        }
    }
}

void tst_QProcess::invalidProgramString_data()
{
    QTest::addColumn<QString>("programString");
    QTest::newRow("null string") << QString();
    QTest::newRow("empty string") << QString("");
    QTest::newRow("only blank string") << QString("  ");
}

void tst_QProcess::invalidProgramString()
{
    QFETCH(QString, programString);
    QProcess process;

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    QSignalSpy spy(&process, &QProcess::errorOccurred);
    QSignalSpy spy2(&process, static_cast<QProcessErrorSignal>(&QProcess::error));
    QVERIFY(spy.isValid());
    QVERIFY(spy2.isValid());

    process.start(programString);
    QCOMPARE(process.error(), QProcess::FailedToStart);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy2.count(), 1);

    QVERIFY(!QProcess::startDetached(programString));
}

void tst_QProcess::onlyOneStartedSignal()
{
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    QProcess process;

    QSignalSpy spyStarted(&process,  &QProcess::started);
    QSignalSpy spyFinished(&process, static_cast<QProcessFinishedSignal2>(&QProcess::finished));

    QVERIFY(spyStarted.isValid());
    QVERIFY(spyFinished.isValid());

    process.start("testProcessNormal/testProcessNormal");
    QVERIFY(process.waitForStarted(5000));
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(spyStarted.count(), 1);
    QCOMPARE(spyFinished.count(), 1);

    spyStarted.clear();
    spyFinished.clear();

    process.start("testProcessNormal/testProcessNormal");
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(spyStarted.count(), 1);
    QCOMPARE(spyFinished.count(), 1);
}

class BlockOnReadStdOut : public QObject
{
    Q_OBJECT
public:
    BlockOnReadStdOut(QProcess *process)
    {
        connect(process, &QProcess::readyReadStandardOutput, this, &BlockOnReadStdOut::block);
    }

public slots:
    void block()
    {
        QThread::sleep(1);
    }
};

void tst_QProcess::finishProcessBeforeReadingDone()
{
    QProcess process;
    BlockOnReadStdOut blocker(&process);
    QEventLoop loop;
    connect(&process, static_cast<QProcessFinishedSignal1>(&QProcess::finished),
            &loop, &QEventLoop::quit);
    process.start("testProcessOutput/testProcessOutput");
    QVERIFY(process.waitForStarted());
    loop.exec();
    QStringList lines = QString::fromLocal8Bit(process.readAllStandardOutput()).split(
            QRegExp(QStringLiteral("[\r\n]")), QString::SkipEmptyParts);
    QVERIFY(!lines.isEmpty());
    QCOMPARE(lines.last(), QStringLiteral("10239 -this is a number"));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

//-----------------------------------------------------------------------------
void tst_QProcess::waitForStartedWithoutStart()
{
    QProcess process;
    QVERIFY(!process.waitForStarted(5000));
}

//-----------------------------------------------------------------------------
void tst_QProcess::startStopStartStop()
{
    // we actually do start-stop x 3 :-)
    QProcess process;
    process.start("testProcessNormal/testProcessNormal");
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    process.start("testExitCodes/testExitCodes", QStringList() << "1");
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 1);

    process.start("testProcessNormal/testProcessNormal");
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

//-----------------------------------------------------------------------------
void tst_QProcess::startStopStartStopBuffers_data()
{
    QTest::addColumn<int>("channelMode1");
    QTest::addColumn<int>("channelMode2");

    QTest::newRow("separate-separate") << int(QProcess::SeparateChannels) << int(QProcess::SeparateChannels);
    QTest::newRow("separate-merged") << int(QProcess::SeparateChannels) << int(QProcess::MergedChannels);
    QTest::newRow("merged-separate") << int(QProcess::MergedChannels) << int(QProcess::SeparateChannels);
    QTest::newRow("merged-merged") << int(QProcess::MergedChannels) << int(QProcess::MergedChannels);
    QTest::newRow("merged-forwarded") << int(QProcess::MergedChannels) << int(QProcess::ForwardedChannels);
}

void tst_QProcess::startStopStartStopBuffers()
{
    QFETCH(int, channelMode1);
    QFETCH(int, channelMode2);

    QProcess process;
    process.setProcessChannelMode(QProcess::ProcessChannelMode(channelMode1));
    process.start("testProcessHang/testProcessHang");
    QVERIFY2(process.waitForReadyRead(), process.errorString().toLocal8Bit());
    if (channelMode1 == QProcess::SeparateChannels || channelMode1 == QProcess::ForwardedOutputChannel) {
        process.setReadChannel(QProcess::StandardError);
        if (process.bytesAvailable() == 0)
            QVERIFY(process.waitForReadyRead());
        process.setReadChannel(QProcess::StandardOutput);
    }

    // We want to test that the write buffer still has bytes after the child
    // exiting. We do that by writing to a child process that never reads. We
    // just have to write more data than a pipe can hold, so that even if
    // QProcess finds the pipe writable (during waitForFinished() or in the
    // QWindowsPipeWriter thread), some data will remain. The worst case I know
    // of is Linux, which defaults to 64 kB of buffer.

    process.write(QByteArray(128 * 1024, 'a'));
    QVERIFY(process.bytesToWrite() > 0);
    process.kill();

    QVERIFY(process.waitForFinished());

#ifndef Q_OS_WIN
    // confirm that our buffers are still full
    // Note: this doesn't work on Windows because our buffers are drained into
    // QWindowsPipeWriter before being sent to the child process.
    QVERIFY(process.bytesToWrite() > 0);
    QVERIFY(process.bytesAvailable() > 0); // channelMode1 is not ForwardedChannels
    if (channelMode1 == QProcess::SeparateChannels || channelMode1 == QProcess::ForwardedOutputChannel) {
        process.setReadChannel(QProcess::StandardError);
        QVERIFY(process.bytesAvailable() > 0);
        process.setReadChannel(QProcess::StandardOutput);
    }
#endif

    process.setProcessChannelMode(QProcess::ProcessChannelMode(channelMode2));
    process.start("testProcessEcho2/testProcessEcho2", QIODevice::ReadWrite | QIODevice::Text);

    // the buffers should now be empty
    QCOMPARE(process.bytesToWrite(), qint64(0));
    QCOMPARE(process.bytesAvailable(), qint64(0));
    process.setReadChannel(QProcess::StandardError);
    QCOMPARE(process.bytesAvailable(), qint64(0));
    process.setReadChannel(QProcess::StandardOutput);

    process.write("line3\n");
    process.closeWriteChannel();
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);

    if (channelMode2 == QProcess::MergedChannels) {
        QCOMPARE(process.readAll(), QByteArray("lliinnee33\n\n"));
    } else if (channelMode2 != QProcess::ForwardedChannels) {
        QCOMPARE(process.readAllStandardOutput(), QByteArray("line3\n"));
        if (channelMode2 == QProcess::SeparateChannels)
            QCOMPARE(process.readAllStandardError(), QByteArray("line3\n"));
    }
}

void tst_QProcess::processEventsInAReadyReadSlot_data()
{
    QTest::addColumn<bool>("callWaitForReadyRead");

    QTest::newRow("no waitForReadyRead") << false;
    QTest::newRow("waitForReadyRead") << true;
}

void tst_QProcess::processEventsInAReadyReadSlot()
{
    // Test whether processing events in a readyReadXXX slot crashes. (QTBUG-48697)
    QFETCH(bool, callWaitForReadyRead);
    QProcess process;
    QObject::connect(&process, &QProcess::readyReadStandardOutput,
                     this, &tst_QProcess::processApplicationEvents);
    process.start("testProcessEcho/testProcessEcho");
    QVERIFY(process.waitForStarted());
    const QByteArray data(156, 'x');
    process.write(data.constData(), data.size() + 1);
    if (callWaitForReadyRead)
        QVERIFY(process.waitForReadyRead());
    if (process.state() == QProcess::Running)
        QVERIFY(process.waitForFinished());
}

#endif //QT_NO_PROCESS

QTEST_MAIN(tst_QProcess)
#include "tst_qprocess.moc"
