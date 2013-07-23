/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QRegExp>
#include <QtCore/QDebug>
#include <QtCore/QMetaType>
#include <QtNetwork/QHostInfo>
#include <stdlib.h>

#ifndef QT_NO_PROCESS
# if defined(Q_OS_WIN)
#  include <windows.h>
# endif

Q_DECLARE_METATYPE(QProcess::ExitStatus);
Q_DECLARE_METATYPE(QProcess::ProcessState);
#endif

#define QPROCESS_VERIFY(Process, Fn) \
{ \
const bool ret = Process.Fn; \
if (ret == false) \
    qWarning("QProcess error: %d: %s", Process.error(), qPrintable(Process.errorString())); \
QVERIFY(ret); \
}

class tst_QProcess : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

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
#ifndef Q_OS_WINCE
    void echoTest_data();
    void echoTest();
    void echoTest2();
#ifdef Q_OS_WIN
    void echoTestGui();
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
    void forwardedChannels();
    void forwardedChannelsOutput();
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
    void setStandardOutputProcess_data();
    void setStandardOutputProcess();
    void removeFileWhileProcessIsRunning();
    void fileWriterProcess();
    void switchReadChannels();
#ifdef Q_OS_WIN
    void setWorkingDirectory();
#endif // Q_OS_WIN
#endif // not Q_OS_WINCE

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
#endif
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

    // keep these at the end, since they use lots of processes and sometimes
    // caused obscure failures to occur in tests that followed them (esp. on the Mac)
    void failToStart();
    void failToStartWithWait();
    void failToStartWithEventLoop();

protected slots:
    void readFromProcess();
    void exitLoopSlot();
#ifndef Q_OS_WINCE
    void restartProcess();
    void waitForReadyReadInAReadyReadSlotSlot();
    void waitForBytesWrittenInABytesWrittenSlotSlot();
#endif

private:
    QProcess *process;
    qint64 bytesAvailable;
#endif //QT_NO_PROCESS
};

void tst_QProcess::initTestCase()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
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

//-----------------------------------------------------------------------------
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

    process = new QProcess;
    QSignalSpy spy(process, SIGNAL(stateChanged(QProcess::ProcessState)));
    QVERIFY(spy.isValid());
    connect(process, SIGNAL(readyRead()), this, SLOT(readFromProcess()));

    /* valgrind dislike SUID binaries(those that have the `s'-flag set), which
     * makes it fail to start the process. For this reason utilities like `ping' won't
     * start, when the auto test is run through `valgrind'. */
    process->start("testProcessNormal/testProcessNormal");
    if (process->state() != QProcess::Starting)
        QCOMPARE(process->state(), QProcess::Running);
    QVERIFY2(process->waitForStarted(5000), qPrintable(process->errorString()));
    QCOMPARE(process->state(), QProcess::Running);
    QTRY_COMPARE(process->state(), QProcess::NotRunning);

    delete process;
    process = 0;

    QCOMPARE(spy.count(), 3);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(spy.at(0).at(0)), QProcess::Starting);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(spy.at(1).at(0)), QProcess::Running);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(spy.at(2).at(0)), QProcess::NotRunning);
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void tst_QProcess::execute()
{
    QCOMPARE(QProcess::execute("testProcessNormal/testProcessNormal",
                               QStringList() << "arg1" << "arg2"), 0);
    QCOMPARE(QProcess::execute("nonexistingexe"), -2);
}

//-----------------------------------------------------------------------------
void tst_QProcess::startDetached()
{
    QProcess proc;
    QVERIFY(proc.startDetached("testProcessNormal/testProcessNormal",
                               QStringList() << "arg1" << "arg2"));
    QCOMPARE(QProcess::startDetached("nonexistingexe"), false);
}

//-----------------------------------------------------------------------------
void tst_QProcess::readFromProcess()
{
    int lines = 0;
    while (process->canReadLine()) {
        ++lines;
        QByteArray line = process->readLine();
    }
}

//-----------------------------------------------------------------------------
void tst_QProcess::crashTest()
{
    qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");
    process = new QProcess;
    QSignalSpy stateSpy(process, SIGNAL(stateChanged(QProcess::ProcessState)));
    QVERIFY(stateSpy.isValid());
    process->start("testProcessCrash/testProcessCrash");
    QVERIFY(process->waitForStarted(5000));

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QSignalSpy spy(process, SIGNAL(error(QProcess::ProcessError)));
    QSignalSpy spy2(process, SIGNAL(finished(int,QProcess::ExitStatus)));

    QVERIFY(spy.isValid());
    QVERIFY(spy2.isValid());

    QVERIFY(process->waitForFinished(30000));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy.at(0).at(0).constData()), QProcess::Crashed);

    QCOMPARE(spy2.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ExitStatus *>(spy2.at(0).at(1).constData()), QProcess::CrashExit);

    QCOMPARE(process->exitStatus(), QProcess::CrashExit);

    delete process;
    process = 0;

    QCOMPARE(stateSpy.count(), 3);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(0).at(0)), QProcess::Starting);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(1).at(0)), QProcess::Running);
    QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(2).at(0)), QProcess::NotRunning);
}

//-----------------------------------------------------------------------------
void tst_QProcess::crashTest2()
{
    process = new QProcess;
    process->start("testProcessCrash/testProcessCrash");
    QVERIFY(process->waitForStarted(5000));

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QSignalSpy spy(process, SIGNAL(error(QProcess::ProcessError)));
    QSignalSpy spy2(process, SIGNAL(finished(int,QProcess::ExitStatus)));

    QVERIFY(spy.isValid());
    QVERIFY(spy2.isValid());

    QObject::connect(process, SIGNAL(finished(int)), this, SLOT(exitLoopSlot()));

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Failed to detect crash : operation timed out");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy.at(0).at(0).constData()), QProcess::Crashed);

    QCOMPARE(spy2.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ExitStatus *>(spy2.at(0).at(1).constData()), QProcess::CrashExit);

    QCOMPARE(process->exitStatus(), QProcess::CrashExit);

    delete process;
    process = 0;
}

#ifndef Q_OS_WINCE
//Reading and writing to a process is not supported on Qt/CE
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------

void tst_QProcess::echoTest()
{
    QFETCH(QByteArray, input);

    process = new QProcess;
    connect(process, SIGNAL(readyRead()), this, SLOT(exitLoopSlot()));

    process->start("testProcessEcho/testProcessEcho");
    QVERIFY(process->waitForStarted(5000));

    process->write(input);

    QTime stopWatch;
    stopWatch.start();
    do {
        QVERIFY(process->isOpen());
        QTestEventLoop::instance().enterLoop(2);
    } while (stopWatch.elapsed() < 60000 && process->bytesAvailable() < input.size());
    if (stopWatch.elapsed() >= 60000)
        QFAIL("Timed out");

    QByteArray message = process->readAll();
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

    process->write("", 1);

    QVERIFY(process->waitForFinished(5000));


    delete process;
    process = 0;
}
#endif

//-----------------------------------------------------------------------------
void tst_QProcess::exitLoopSlot()
{
    QTestEventLoop::instance().exitLoop();
}

//-----------------------------------------------------------------------------

#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::echoTest2()
{

    process = new QProcess;
    connect(process, SIGNAL(readyRead()), this, SLOT(exitLoopSlot()));

    process->start("testProcessEcho2/testProcessEcho2");
    QVERIFY(process->waitForStarted(5000));
    QVERIFY(!process->waitForReadyRead(250));
    QCOMPARE(process->error(), QProcess::Timedout);

    process->write("Hello");
    QSignalSpy spy1(process, SIGNAL(readyReadStandardOutput()));
    QSignalSpy spy2(process, SIGNAL(readyReadStandardError()));

    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());

    QTime stopWatch;
    stopWatch.start();
    forever {
        QTestEventLoop::instance().enterLoop(1);
        if (stopWatch.elapsed() >= 30000)
            QFAIL("Timed out");
        process->setReadChannel(QProcess::StandardOutput);
        qint64 baso = process->bytesAvailable();

        process->setReadChannel(QProcess::StandardError);
        qint64 base = process->bytesAvailable();
        if (baso == 5 && base == 5)
            break;
    }

    QVERIFY(spy1.count() > 0);
    QVERIFY(spy2.count() > 0);

    QCOMPARE(process->readAllStandardOutput(), QByteArray("Hello"));
    QCOMPARE(process->readAllStandardError(), QByteArray("Hello"));

    process->write("", 1);
    QVERIFY(process->waitForFinished(5000));

    delete process;
    process = 0;
}
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
//Batch files are not supported on Winfows CE
// Reading and writing to a process is not supported on Qt/CE
//-----------------------------------------------------------------------------
void tst_QProcess::echoTestGui()
{
    QProcess process;

    process.start("testProcessEchoGui/testProcessEchoGui");


    process.write("Hello");
    process.write("q");

    QVERIFY(process.waitForFinished(50000));

    QCOMPARE(process.readAllStandardOutput(), QByteArray("Hello"));
    QCOMPARE(process.readAllStandardError(), QByteArray("Hello"));
}
#endif // !Q_OS_WINCE && Q_OS_WIN

//-----------------------------------------------------------------------------
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
//Batch files are not supported on Winfows CE
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

    QVERIFY(proc.bytesAvailable() > 0);

    QVERIFY(proc.readAll().startsWith(output));
}
#endif // !Q_OS_WINCE && Q_OS_WIN

//-----------------------------------------------------------------------------
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
    process = new QProcess;
    QFETCH(QStringList, processList);
    QFETCH(QList<QProcess::ExitStatus>, exitStatus);

    QCOMPARE(exitStatus.count(), processList.count());
    for (int i = 0; i < processList.count(); ++i) {
        process->start(processList.at(i));
        QVERIFY(process->waitForStarted(5000));
        QVERIFY(process->waitForFinished(30000));

        QCOMPARE(process->exitStatus(), exitStatus.at(i));
    }

    process->deleteLater();
    process = 0;
}
//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::loopBackTest()
{

    process = new QProcess;
    process->start("testProcessEcho/testProcessEcho");
    QVERIFY(process->waitForStarted(5000));

    for (int i = 0; i < 100; ++i) {
        process->write("Hello");
        do {
            QVERIFY(process->waitForReadyRead(5000));
        } while (process->bytesAvailable() < 5);
        QCOMPARE(process->readAll(), QByteArray("Hello"));
    }

    process->write("", 1);
    QVERIFY(process->waitForFinished(5000));

    delete process;
    process = 0;
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::readTimeoutAndThenCrash()
{

    process = new QProcess;
    process->start("testProcessEcho/testProcessEcho");
    if (process->state() != QProcess::Starting)
        QCOMPARE(process->state(), QProcess::Running);

    QVERIFY(process->waitForStarted(5000));
    QCOMPARE(process->state(), QProcess::Running);

    QVERIFY(!process->waitForReadyRead(5000));
    QCOMPARE(process->error(), QProcess::Timedout);

    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    QSignalSpy spy(process, SIGNAL(error(QProcess::ProcessError)));
    QVERIFY(spy.isValid());

    process->kill();

    QVERIFY(process->waitForFinished(5000));
    QCOMPARE(process->state(), QProcess::NotRunning);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const QProcess::ProcessError *>(spy.at(0).at(0).constData()), QProcess::Crashed);

    delete process;
    process = 0;
}
#endif

void tst_QProcess::waitForFinished()
{
    QProcess process;

    process.start("testProcessOutput/testProcessOutput");

#if !defined(Q_OS_WINCE)
    QVERIFY(process.waitForFinished(5000));
#else
    QVERIFY(process.waitForFinished(30000));
#endif
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

#if defined (Q_OS_WINCE)
    QEXPECT_FAIL("", "Reading and writing to a process is not supported on Qt/CE", Continue);
#endif
    QString output = process.readAll();
    QCOMPARE(output.count("\n"), 10*1024);

    process.start("blurdybloop");
    QVERIFY(!process.waitForFinished());
    QCOMPARE(process.error(), QProcess::FailedToStart);
}

#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
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
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::restartProcessDeadlock()
{

    // The purpose of this test is to detect whether restarting a
    // process in the finished() connected slot causes a deadlock
    // because of the way QProcessManager uses its locks.
    QProcess proc;
    process = &proc;
    connect(process, SIGNAL(finished(int)), this, SLOT(restartProcess()));

    process->start("testProcessEcho/testProcessEcho");

    QCOMPARE(process->write("", 1), qlonglong(1));
    QVERIFY(process->waitForFinished(5000));

    process->disconnect(SIGNAL(finished(int)));

    QCOMPARE(process->write("", 1), qlonglong(1));
    QVERIFY(process->waitForFinished(5000));
}

void tst_QProcess::restartProcess()
{
    process->start("testProcessEcho/testProcessEcho");
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::closeWriteChannel()
{
    QProcess more;
    more.start("testProcessEOF/testProcessEOF");

    QVERIFY(more.waitForStarted(5000));
    QVERIFY(!more.waitForReadyRead(250));
    QCOMPARE(more.error(), QProcess::Timedout);

    QVERIFY(more.write("Data to read") != -1);

    QVERIFY(!more.waitForReadyRead(250));
    QCOMPARE(more.error(), QProcess::Timedout);

    more.closeWriteChannel();

    QVERIFY(more.waitForReadyRead(5000));
    QVERIFY(more.readAll().startsWith("Data to read"));

    if (more.state() == QProcess::Running)
        more.write("q");
    QVERIFY(more.waitForFinished(5000));
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE"
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
    }
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::openModes()
{
    QProcess proc;
    QVERIFY(!proc.isOpen());
    QVERIFY(proc.openMode() == QProcess::NotOpen);
    proc.start("testProcessEcho3/testProcessEcho3");
    QVERIFY(proc.waitForStarted(5000));
    QVERIFY(proc.isOpen());
    QVERIFY(proc.openMode() == QProcess::ReadWrite);
    QVERIFY(proc.isReadable());
    QVERIFY(proc.isWritable());

    proc.write("Data");

    proc.closeWriteChannel();

    QVERIFY(proc.isWritable());
    QVERIFY(proc.openMode() == QProcess::ReadWrite);

    while (proc.bytesAvailable() < 4 && proc.waitForReadyRead(5000))
    { }

    QCOMPARE(proc.readAll().constData(), QByteArray("Data").constData());

    proc.closeReadChannel(QProcess::StandardOutput);

    QVERIFY(proc.openMode() == QProcess::ReadWrite);
    QVERIFY(proc.isReadable());

    proc.closeReadChannel(QProcess::StandardError);

    QVERIFY(proc.openMode() == QProcess::ReadWrite);
    QVERIFY(proc.isReadable());

    proc.close();
    QVERIFY(!proc.isOpen());
    QVERIFY(!proc.isReadable());
    QVERIFY(!proc.isWritable());
    QCOMPARE(proc.state(), QProcess::NotRunning);
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::emitReadyReadOnlyWhenNewDataArrives()
{

    QProcess proc;
    connect(&proc, SIGNAL(readyRead()), this, SLOT(exitLoopSlot()));
    QSignalSpy spy(&proc, SIGNAL(readyRead()));
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

    QObject::disconnect(&proc, SIGNAL(readyRead()), 0, 0);
    proc.write("B");
    QVERIFY(proc.waitForReadyRead(5000));

    proc.write("", 1);
    QVERIFY(proc.waitForFinished(5000));
}
#endif

//-----------------------------------------------------------------------------
void tst_QProcess::hardExit()
{
    QProcess proc;

#if defined(Q_OS_WINCE)
    proc.start("testSoftExit/testSoftExit");
#else
    proc.start("testProcessEcho/testProcessEcho");
#endif

#ifndef Q_OS_WINCE
    QVERIFY(proc.waitForStarted(5000));
#else
    QVERIFY(proc.waitForStarted(10000));
#endif

    proc.kill();

    QVERIFY(proc.waitForFinished(5000));
    QCOMPARE(int(proc.state()), int(QProcess::NotRunning));
    QCOMPARE(int(proc.error()), int(QProcess::Crashed));
}

//-----------------------------------------------------------------------------
void tst_QProcess::softExit()
{
    QProcess proc;

    proc.start("testSoftExit/testSoftExit");

    QVERIFY(proc.waitForStarted(10000));
#if !defined(Q_OS_WINCE)
    QVERIFY(proc.waitForReadyRead(10000));
#endif

    proc.terminate();

    QVERIFY(proc.waitForFinished(10000));
    QCOMPARE(int(proc.state()), int(QProcess::NotRunning));
    QCOMPARE(int(proc.error()), int(QProcess::UnknownError));
}

#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
class SoftExitProcess : public QProcess
{
    Q_OBJECT
public:
    bool waitedForFinished;

    SoftExitProcess(int n) : waitedForFinished(false), n(n), killing(false)
    {
        connect(this, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(finishedSlot(int,QProcess::ExitStatus)));

        switch (n) {
        case 0:
            setReadChannelMode(QProcess::MergedChannels);
            connect(this, SIGNAL(readyRead()), this, SLOT(terminateSlot()));
            break;
        case 1:
            connect(this, SIGNAL(readyReadStandardOutput()),
                    this, SLOT(terminateSlot()));
            break;
        case 2:
            connect(this, SIGNAL(readyReadStandardError()),
                    this, SLOT(terminateSlot()));
            break;
        case 3:
            connect(this, SIGNAL(started()),
                    this, SLOT(terminateSlot()));
            break;
        case 4:
        default:
            connect(this, SIGNAL(stateChanged(QProcess::ProcessState)),
                    this, SLOT(terminateSlot()));
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
        writePendingData(); // In cases 3 and 4 we haven't written the data yet.
        if (killing || (n == 4 && state() != Running)) {
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

//-----------------------------------------------------------------------------
void tst_QProcess::softExitInSlots_data()
{
    QTest::addColumn<QString>("appName");

#ifndef QT_NO_WIDGETS
    QTest::newRow("gui app") << "testGuiProcess/testGuiProcess";
#endif
    QTest::newRow("console app") << "testProcessEcho2/testProcessEcho2";
}
//-----------------------------------------------------------------------------

void tst_QProcess::softExitInSlots()
{
    QFETCH(QString, appName);

    for (int i = 0; i < 5; ++i) {
        SoftExitProcess proc(i);
        proc.writeAfterStart("OLEBOLE", 8); // include the \0
        proc.start(appName);
        QTRY_VERIFY(proc.waitedForFinished);
        QCOMPARE(proc.state(), QProcess::NotRunning);
    }
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
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
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::forwardedChannels()
{
    QProcess process;
    process.setReadChannelMode(QProcess::ForwardedChannels);
    QCOMPARE(process.readChannelMode(), QProcess::ForwardedChannels);

    process.start("testProcessEcho2/testProcessEcho2");

    QVERIFY(process.waitForStarted(5000));
    QCOMPARE(process.write("forwarded\n"), qlonglong(10));
    QVERIFY(!process.waitForReadyRead(250));
    QCOMPARE(process.bytesAvailable(), qlonglong(0));

    process.closeWriteChannel();
    QVERIFY(process.waitForFinished(5000));
}
#endif

#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::forwardedChannelsOutput()
{
    QProcess process;
    process.start("testForwarding/testForwarding");
    QVERIFY(process.waitForStarted(5000));
    QVERIFY(process.waitForFinished(5000));
    QVERIFY(!process.exitCode());
    QByteArray data = process.readAll();
    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains("forwarded"));
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
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
}
#endif

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
        connect(&process, SIGNAL(finished(int)), this, SLOT(catchExitCode(int)),
                Qt::DirectConnection);

        process.start("testProcessEcho/testProcessEcho");

#if !defined(Q_OS_WINCE)
        QCOMPARE(process.write("abc\0", 4), qint64(4));
#endif
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

//-----------------------------------------------------------------------------
void tst_QProcess::processInAThread()
{
    for (int i = 0; i < 10; ++i) {
        TestThread thread;
        thread.start();
        QVERIFY(thread.wait(10000));
        QCOMPARE(thread.code(), 0);
    }
}

//-----------------------------------------------------------------------------
void tst_QProcess::processesInMultipleThreads()
{
    for (int i = 0; i < 10; ++i) {
        TestThread thread1;
        TestThread thread2;
        TestThread thread3;

        thread1.start();
        thread2.start();
        thread3.start();

        QVERIFY(thread2.wait(10000));
        QVERIFY(thread3.wait(10000));
        QVERIFY(thread1.wait(10000));

        QCOMPARE(thread1.code(), 0);
        QCOMPARE(thread2.code(), 0);
        QCOMPARE(thread3.code(), 0);
    }
}

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::waitForFinishedWithTimeout()
{
    process = new QProcess(this);

    process->start("testProcessEcho/testProcessEcho");

    QVERIFY(process->waitForStarted(5000));
    QVERIFY(!process->waitForFinished(1));

    process->write("", 1);

    QVERIFY(process->waitForFinished());

    delete process;
    process = 0;
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::waitForReadyReadInAReadyReadSlot()
{
    process = new QProcess(this);
    connect(process, SIGNAL(readyRead()), this, SLOT(waitForReadyReadInAReadyReadSlotSlot()));
    connect(process, SIGNAL(finished(int)), this, SLOT(exitLoopSlot()));
    bytesAvailable = 0;

    process->start("testProcessEcho/testProcessEcho");
    QVERIFY(process->waitForStarted(5000));

    QSignalSpy spy(process, SIGNAL(readyRead()));
    QVERIFY(spy.isValid());
    process->write("foo");
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spy.count(), 1);

    process->disconnect();
    QVERIFY(process->waitForFinished(5000));
    QVERIFY(process->bytesAvailable() > bytesAvailable);
    delete process;
    process = 0;
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::waitForReadyReadInAReadyReadSlotSlot()
{
    bytesAvailable = process->bytesAvailable();
    process->write("bar", 4);
    QVERIFY(process->waitForReadyRead(5000));
    QTestEventLoop::instance().exitLoop();
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::waitForBytesWrittenInABytesWrittenSlot()
{
    process = new QProcess(this);
    connect(process, SIGNAL(bytesWritten(qint64)), this, SLOT(waitForBytesWrittenInABytesWrittenSlotSlot()));
    bytesAvailable = 0;

    process->start("testProcessEcho/testProcessEcho");
    QVERIFY(process->waitForStarted(5000));

    QSignalSpy spy(process, SIGNAL(bytesWritten(qint64)));
    QVERIFY(spy.isValid());
    process->write("f");
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(spy.count(), 1);
    process->write("", 1);
    process->disconnect();
    QVERIFY(process->waitForFinished());
    delete process;
    process = 0;
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::waitForBytesWrittenInABytesWrittenSlotSlot()
{
    process->write("b");
    QVERIFY(process->waitForBytesWritten(5000));
    QTestEventLoop::instance().exitLoop();
}
#endif
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void tst_QProcess::spaceArgsTest()
{
    QFETCH(QStringList, args);
    QFETCH(QString, stringArgs);

    QStringList programs;
    programs << QString::fromLatin1("testProcessSpacesArgs/nospace")
             << QString::fromLatin1("testProcessSpacesArgs/one space")
             << QString::fromLatin1("testProcessSpacesArgs/two space s");

    process = new QProcess(this);

    for (int i = 0; i < programs.size(); ++i) {
        QString program = programs.at(i);
        process->start(program, args);

#if defined(Q_OS_WINCE)
        const int timeOutMS = 10000;
#else
        const int timeOutMS = 5000;
#endif
        QByteArray errorMessage;
        bool started = process->waitForStarted(timeOutMS);
        if (!started)
            errorMessage = startFailMessage(program, *process);
        QVERIFY2(started, errorMessage.constData());
        QVERIFY(process->waitForFinished(timeOutMS));

#if !defined(Q_OS_WINCE)
        QStringList actual = QString::fromLatin1(process->readAll()).split("|");
#endif
#if !defined(Q_OS_WINCE)
        QVERIFY(!actual.isEmpty());
        // not interested in the program name, it might be different.
        actual.removeFirst();

        QCOMPARE(actual, args);
#endif

        if (program.contains(" "))
            program = "\"" + program + "\"";

        if (!stringArgs.isEmpty())
            program += QString::fromLatin1(" ") + stringArgs;

        errorMessage.clear();
        process->start(program);
        started = process->waitForStarted(5000);
        if (!started)
            errorMessage = startFailMessage(program, *process);

        QVERIFY2(started, errorMessage.constData());
        QVERIFY(process->waitForFinished(5000));

#if !defined(Q_OS_WINCE)
        actual = QString::fromLatin1(process->readAll()).split("|");
#endif
#if !defined(Q_OS_WINCE)
        QVERIFY(!actual.isEmpty());
        // not interested in the program name, it might be different.
        actual.removeFirst();

        QCOMPARE(actual, args);
#endif
    }

    delete process;
    process = 0;
}

#if defined(Q_OS_WIN)

//-----------------------------------------------------------------------------
void tst_QProcess::nativeArguments()
{
    QProcess proc;

    // This doesn't actually need special quoting, so it is pointless to use
    // native arguments here, but that's not the point of this test.
    proc.setNativeArguments("hello kitty, \"*\"!");

    proc.start(QString::fromLatin1("testProcessSpacesArgs/nospace"), QStringList());

#if !defined(Q_OS_WINCE)
    QVERIFY(proc.waitForStarted(5000));
    QVERIFY(proc.waitForFinished(5000));
#else
    QVERIFY(proc.waitForStarted(10000));
    QVERIFY(proc.waitForFinished(10000));
#endif

#if defined(Q_OS_WINCE)
    // WinCE test outputs to a file, so check that
    FILE* file = fopen("\\temp\\qprocess_args_test.txt","r");
    QVERIFY(file);
    char buf[256];
    fgets(buf, 256, file);
    fclose(file);
    QStringList actual = QString::fromLatin1(buf).split("|");
#else
    QStringList actual = QString::fromLatin1(proc.readAll()).split("|");
#endif
    QVERIFY(!actual.isEmpty());
    // not interested in the program name, it might be different.
    actual.removeFirst();
    QStringList expected;
#if defined(Q_OS_WINCE)
    expected << "hello" << "kitty," << "\"*\"!"; // Weird, weird ...
#else
    expected << "hello" << "kitty," << "*!";
#endif
    QCOMPARE(actual, expected);
}

#endif

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void tst_QProcess::failToStart()
{
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");

    QProcess process;
    QSignalSpy stateSpy(&process, SIGNAL(stateChanged(QProcess::ProcessState)));
    QSignalSpy errorSpy(&process, SIGNAL(error(QProcess::ProcessError)));
    QSignalSpy finishedSpy(&process, SIGNAL(finished(int)));
    QSignalSpy finishedSpy2(&process, SIGNAL(finished(int,QProcess::ExitStatus)));

    QVERIFY(stateSpy.isValid());
    QVERIFY(errorSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(finishedSpy2.isValid());

// Mac OS X and HP-UX have a really low default process limit (~100), so spawning
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
            QCOMPARE(finishedSpy.count(), 0);
            QCOMPARE(finishedSpy2.count(), 0);

            int it = j * attempts + i + 1;

            QCOMPARE(stateSpy.count(), it * 2);
            QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(it * 2 - 2).at(0)), QProcess::Starting);
            QCOMPARE(qvariant_cast<QProcess::ProcessState>(stateSpy.at(it * 2 - 1).at(0)), QProcess::NotRunning);
        }
    }
}

//-----------------------------------------------------------------------------
void tst_QProcess::failToStartWithWait()
{
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QProcess process;
    QEventLoop loop;
    QSignalSpy errorSpy(&process, SIGNAL(error(QProcess::ProcessError)));
    QSignalSpy finishedSpy(&process, SIGNAL(finished(int)));
    QSignalSpy finishedSpy2(&process, SIGNAL(finished(int,QProcess::ExitStatus)));

    QVERIFY(errorSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(finishedSpy2.isValid());

    for (int i = 0; i < 50; ++i) {
        process.start("/blurp", QStringList() << "-v" << "-debug");
        process.waitForStarted();

        QCOMPARE(process.error(), QProcess::FailedToStart);
        QCOMPARE(errorSpy.count(), i + 1);
        QCOMPARE(finishedSpy.count(), 0);
        QCOMPARE(finishedSpy2.count(), 0);
    }
}

//-----------------------------------------------------------------------------
void tst_QProcess::failToStartWithEventLoop()
{
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QProcess process;
    QEventLoop loop;
    QSignalSpy errorSpy(&process, SIGNAL(error(QProcess::ProcessError)));
    QSignalSpy finishedSpy(&process, SIGNAL(finished(int)));
    QSignalSpy finishedSpy2(&process, SIGNAL(finished(int,QProcess::ExitStatus)));

    QVERIFY(errorSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(finishedSpy2.isValid());

    // The error signal may be emitted before start() returns
    connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()), Qt::QueuedConnection);


    for (int i = 0; i < 50; ++i) {
        process.start("/blurp", QStringList() << "-v" << "-debug");

        loop.exec();

        QCOMPARE(process.error(), QProcess::FailedToStart);
        QCOMPARE(errorSpy.count(), i + 1);
        QCOMPARE(finishedSpy.count(), 0);
        QCOMPARE(finishedSpy2.count(), 0);
    }
}

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::removeFileWhileProcessIsRunning()
{
    QFile file("removeFile.txt");
    QVERIFY(file.open(QFile::WriteOnly));

    QProcess process;
    process.start("testProcessEcho/testProcessEcho");

    QVERIFY(process.waitForStarted(5000));

    QVERIFY(file.remove());

    process.write("", 1);
    QVERIFY(process.waitForFinished(5000));
}
#endif
//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// OS doesn't support environment variables
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
#endif
//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// OS doesn't support environment variables
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
#endif
//-----------------------------------------------------------------------------
void tst_QProcess::systemEnvironment()
{
#if defined (Q_OS_WINCE)
    // there is no concept of system variables on Windows CE as there is no console
    QVERIFY(QProcess::systemEnvironment().isEmpty());
    QVERIFY(QProcessEnvironment::systemEnvironment().isEmpty());
#else
    QVERIFY(!QProcess::systemEnvironment().isEmpty());
    QVERIFY(!QProcessEnvironment::systemEnvironment().isEmpty());

    QVERIFY(QProcessEnvironment::systemEnvironment().contains("PATH"));
    QVERIFY(!QProcess::systemEnvironment().filter(QRegExp("^PATH=", Qt::CaseInsensitive)).isEmpty());
#endif
}

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::spaceInName()
{
    QProcess process;
    process.start("test Space In Name/testSpaceInName", QStringList());
    QVERIFY(process.waitForStarted());
    process.write("", 1);
    QVERIFY(process.waitForFinished());
}
#endif

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
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
#endif

//-----------------------------------------------------------------------------
void tst_QProcess::waitForReadyReadForNonexistantProcess()
{
    // Start a program that doesn't exist, process events and then try to waitForReadyRead
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    QProcess process;
    QSignalSpy errorSpy(&process, SIGNAL(error(QProcess::ProcessError)));
    QSignalSpy finishedSpy1(&process, SIGNAL(finished(int)));
    QSignalSpy finishedSpy2(&process, SIGNAL(finished(int,QProcess::ExitStatus)));

    QVERIFY(errorSpy.isValid());
    QVERIFY(finishedSpy1.isValid());
    QVERIFY(finishedSpy2.isValid());

    QVERIFY(!process.waitForReadyRead()); // used to crash
    process.start("doesntexist");
    QVERIFY(!process.waitForReadyRead());
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(finishedSpy1.count(), 0);
    QCOMPARE(finishedSpy2.count(), 0);
}

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::setStandardInputFile()
{
    static const char data[] = "A bunch\1of\2data\3\4\5\6\7...";
    QProcess process;
    QFile file("data");

    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data, sizeof data);
    file.close();

    process.setStandardInputFile("data");
    process.start("testProcessEcho/testProcessEcho");

    QPROCESS_VERIFY(process, waitForFinished());
        QByteArray all = process.readAll();
    QCOMPARE(all.size(), int(sizeof data) - 1); // testProcessEcho drops the ending \0
    QVERIFY(all == data);
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
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
    QFile file("data");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data, sizeof data - 1);
    file.close();

    // run the process
    QProcess process;
    process.setReadChannelMode(channelMode);
    if (channelToTest == QProcess::StandardOutput)
        process.setStandardOutputFile("data", mode);
    else
        process.setStandardErrorFile("data", mode);

    process.start("testProcessEcho2/testProcessEcho2");
    process.write(testdata, sizeof testdata);
    QPROCESS_VERIFY(process,waitForFinished());

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
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::setStandardOutputProcess_data()
{
    QTest::addColumn<bool>("merged");
    QTest::newRow("separate") << false;
    QTest::newRow("merged") << true;
}

void tst_QProcess::setStandardOutputProcess()
{

    QProcess source;
    QProcess sink;

    QFETCH(bool, merged);
    source.setReadChannelMode(merged ? QProcess::MergedChannels : QProcess::SeparateChannels);
    source.setStandardOutputProcess(&sink);

    source.start("testProcessEcho2/testProcessEcho2");
    sink.start("testProcessEcho2/testProcessEcho2");

    QByteArray data("Hello, World");
    source.write(data);
    source.closeWriteChannel();
    QPROCESS_VERIFY(source, waitForFinished());
    QPROCESS_VERIFY(sink, waitForFinished());
    QByteArray all = sink.readAll();

    if (!merged)
        QCOMPARE(all, data);
    else
        QCOMPARE(all, QByteArray("HHeelllloo,,  WWoorrlldd"));
}
#endif

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::fileWriterProcess()
{
    QString stdinStr;
    for (int i = 0; i < 5000; ++i)
        stdinStr += QString::fromLatin1("%1 -- testing testing 1 2 3\n").arg(i);

    QTime stopWatch;
    stopWatch.start();
    do {
        QFile::remove("fileWriterProcess.txt");
        QProcess process;
        process.start("fileWriterProcess/fileWriterProcess",
                      QIODevice::ReadWrite | QIODevice::Text);
        process.write(stdinStr.toLatin1());
        process.closeWriteChannel();
        while (process.bytesToWrite()) {
            QVERIFY(stopWatch.elapsed() < 3500);
            QVERIFY(process.waitForBytesWritten(2000));
        }
        QVERIFY(process.waitForFinished());
        QCOMPARE(QFile("fileWriterProcess.txt").size(), qint64(stdinStr.size()));
    } while (stopWatch.elapsed() < 3000);
}
#endif

//-----------------------------------------------------------------------------
void tst_QProcess::detachedWorkingDirectoryAndPid()
{
    qint64 pid;

#ifdef Q_OS_WINCE
    QTest::qSleep(1000);
#endif

    QFile infoFile(QDir::currentPath() + QLatin1String("/detachedinfo.txt"));
    infoFile.remove();

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

//-----------------------------------------------------------------------------
#ifndef Q_OS_WINCE
// Reading and writing to a process is not supported on Qt/CE
void tst_QProcess::switchReadChannels()
{
    const char data[] = "ABCD";

    QProcess process;

    process.start("testProcessEcho2/testProcessEcho2");
    process.write(data);
    process.closeWriteChannel();
    QVERIFY(process.waitForFinished(5000));

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
#endif

//-----------------------------------------------------------------------------
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
// Q_OS_WIN - setWorkingDirectory will chdir before starting the process on unices
// Windows CE does not support working directory logic
void tst_QProcess::setWorkingDirectory()
{
    process = new QProcess;
    process->setWorkingDirectory("test");
    process->start("testSetWorkingDirectory/testSetWorkingDirectory");
    QVERIFY(process->waitForFinished());

    QByteArray workingDir = process->readAllStandardOutput();
    QCOMPARE(QDir("test").canonicalPath(), QDir(workingDir.constData()).canonicalPath());

    delete process;
    process = 0;
}
#endif

//-----------------------------------------------------------------------------
void tst_QProcess::startFinishStartFinish()
{
    QProcess process;

    for (int i = 0; i < 3; ++i) {
        QCOMPARE(process.state(), QProcess::NotRunning);

        process.start("testProcessOutput/testProcessOutput");
#if !defined(Q_OS_WINCE)
        QVERIFY(process.waitForReadyRead(10000));
        QCOMPARE(QString::fromLatin1(process.readLine().trimmed()),
                 QString("0 -this is a number"));
#endif
        if (process.state() != QProcess::NotRunning)
            QVERIFY(process.waitForFinished(10000));
    }
}

//-----------------------------------------------------------------------------
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
    QSignalSpy spy(&process, SIGNAL(error(QProcess::ProcessError)));
    QVERIFY(spy.isValid());

    process.start(programString);
    QCOMPARE(process.error(), QProcess::FailedToStart);
    QCOMPARE(spy.count(), 1);

    QVERIFY(!QProcess::startDetached(programString));
}

//-----------------------------------------------------------------------------
void tst_QProcess::onlyOneStartedSignal()
{
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    QProcess process;

    QSignalSpy spyStarted(&process,  SIGNAL(started()));
    QSignalSpy spyFinished(&process, SIGNAL(finished(int,QProcess::ExitStatus)));

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
    QCOMPARE(spyStarted.count(), 1);
    QCOMPARE(spyFinished.count(), 1);
}

//-----------------------------------------------------------------------------

class BlockOnReadStdOut : public QObject
{
    Q_OBJECT
public:
    BlockOnReadStdOut(QProcess *process)
    {
        connect(process, SIGNAL(readyReadStandardOutput()), SLOT(block()));
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
    connect(&process, SIGNAL(finished(int)), &loop, SLOT(quit()));
    process.start("testProcessOutput/testProcessOutput");
    QVERIFY(process.waitForStarted());
    loop.exec();
    QStringList lines = QString::fromLocal8Bit(process.readAllStandardOutput()).split(
            QRegExp(QStringLiteral("[\r\n]")), QString::SkipEmptyParts);
    QVERIFY(!lines.isEmpty());
    QCOMPARE(lines.last(), QStringLiteral("10239 -this is a number"));
}

#endif //QT_NO_PROCESS

QTEST_MAIN(tst_QProcess)
#include "tst_qprocess.moc"
