/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
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
#include <QtConcurrentRun>
#include <qlockfile.h>
#include <qtemporarydir.h>
#include <qsysinfo.h>
#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
#include <unistd.h>
#include <sys/time.h>
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#  include <qt_windows.h>
#endif

#include <private/qlockfile_p.h>  // for getLockFileHandle()

class tst_QLockFile : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void lockUnlock();
    void lockOutOtherProcess();
    void lockOutOtherThread();
    void raceWithOtherThread();
    void waitForLock_data();
    void waitForLock();
    void staleLockFromCrashedProcess_data();
    void staleLockFromCrashedProcess();
    void staleLockFromCrashedProcessReusedPid();
    void staleShortLockFromBusyProcess();
    void staleLongLockFromBusyProcess();
    void staleLockRace();
    void noPermissions();
    void noPermissionsWindows();
    void corruptedLockFile();
    void corruptedLockFileInTheFuture();
    void hostnameChange();
    void differentMachines();
    void reboot();

private:
    static bool overwriteLineInLockFile(QFile &f, int line, const QString &newLine);
    static bool overwritePidInLockFile(const QString &filePath, qint64 pid);

public:
    QString m_helperApp;
    QTemporaryDir dir;
};

void tst_QLockFile::initTestCase()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    QSKIP("This test requires deploying and running external console applications");
#elif !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    // chdir to our testdata path and execute helper apps relative to that.
    QString testdata_dir = QFileInfo(QFINDTESTDATA("qlockfiletesthelper")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
    m_helperApp = "qlockfiletesthelper/qlockfile_test_helper";
#endif // QT_CONFIG(process)
}

void tst_QLockFile::lockUnlock()
{
    const QString fileName = dir.path() + "/lock1";
    QVERIFY(!QFile(fileName).exists());
    QLockFile lockFile(fileName);
    QVERIFY(lockFile.lock());
    QVERIFY(lockFile.isLocked());
    QCOMPARE(int(lockFile.error()), int(QLockFile::NoError));
    QVERIFY(QFile::exists(fileName));

    // Recursive locking is not allowed
    // (can't test lock() here, it would wait forever)
    QVERIFY(!lockFile.tryLock());
    QCOMPARE(int(lockFile.error()), int(QLockFile::LockFailedError));
    qint64 pid;
    QString hostname, appname;
    QVERIFY(lockFile.getLockInfo(&pid, &hostname, &appname));
    QCOMPARE(pid, QCoreApplication::applicationPid());
    QCOMPARE(appname, qAppName());
    QVERIFY(!lockFile.tryLock(200));
    QCOMPARE(int(lockFile.error()), int(QLockFile::LockFailedError));

    // Unlock deletes the lock file
    lockFile.unlock();
    QCOMPARE(int(lockFile.error()), int(QLockFile::NoError));
    QVERIFY(!lockFile.isLocked());
    QVERIFY(!QFile::exists(fileName));
}

void tst_QLockFile::lockOutOtherProcess()
{
#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    // Lock
    const QString fileName = dir.path() + "/lockOtherProcess";
    QLockFile lockFile(fileName);
    QVERIFY(lockFile.lock());

    // Other process can't acquire lock
    QProcess proc;
    proc.start(m_helperApp, QStringList() << fileName);
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), int(QLockFile::LockFailedError));

    // Unlock
    lockFile.unlock();
    QVERIFY(!QFile::exists(fileName));

    // Other process can now acquire lock
    int ret = QProcess::execute(m_helperApp, QStringList() << fileName);
    QCOMPARE(ret, int(QLockFile::NoError));
    // Lock doesn't survive process though (on clean exit)
    QVERIFY(!QFile::exists(fileName));
#endif // QT_CONFIG(process)
}

static QLockFile::LockError tryLockFromThread(const QString &fileName)
{
    QLockFile lockInThread(fileName);
    lockInThread.tryLock();
    return lockInThread.error();
}

void tst_QLockFile::lockOutOtherThread()
{
    const QString fileName = dir.path() + "/lockOtherThread";
    QLockFile lockFile(fileName);
    QVERIFY(lockFile.lock());

    // Other thread can't acquire lock
    QFuture<QLockFile::LockError> ret = QtConcurrent::run<QLockFile::LockError>(tryLockFromThread, fileName);
    QCOMPARE(ret.result(), QLockFile::LockFailedError);

    lockFile.unlock();

    // Now other thread can acquire lock
    QFuture<QLockFile::LockError> ret2 = QtConcurrent::run<QLockFile::LockError>(tryLockFromThread, fileName);
    QCOMPARE(ret2.result(), QLockFile::NoError);
}

static QLockFile::LockError lockFromThread(const QString &fileName)
{
    QLockFile lockInThread(fileName);
    lockInThread.lock();
    return lockInThread.error();
}

// QTBUG-38853, best way to trigger it was to add a QThread::sleep(1) in QLockFilePrivate::getLockInfo() after the first readLine.
// Then (on Windows), the QFile::remove() in unlock() (called by the first thread who got the lock, in the destructor)
// would fail due to the existing reader on the file. Fixed by checking the return value of QFile::remove() in unlock().
void tst_QLockFile::raceWithOtherThread()
{
    const QString fileName = dir.path() + "/raceWithOtherThread";
    QFuture<QLockFile::LockError> ret = QtConcurrent::run<QLockFile::LockError>(lockFromThread, fileName);
    QFuture<QLockFile::LockError> ret2 = QtConcurrent::run<QLockFile::LockError>(lockFromThread, fileName);
    QCOMPARE(ret.result(), QLockFile::NoError);
    QCOMPARE(ret2.result(), QLockFile::NoError);
}

static bool lockFromThread(const QString &fileName, int sleepMs, QSemaphore *semThreadReady, QSemaphore *semMainThreadDone)
{
    QLockFile lockFile(fileName);
    if (!lockFile.lock()) {
        qWarning() << "Locking failed" << lockFile.error();
        return false;
    }
    semThreadReady->release();
    QThread::msleep(sleepMs);
    semMainThreadDone->acquire();
    lockFile.unlock();
    return true;
}

void tst_QLockFile::waitForLock_data()
{
    QTest::addColumn<int>("testNumber");
    QTest::addColumn<int>("threadSleepMs");
    QTest::addColumn<bool>("releaseEarly");
    QTest::addColumn<int>("tryLockTimeout");
    QTest::addColumn<bool>("expectedResult");

    int tn = 0; // test number
    QTest::newRow("wait_forever_succeeds") << ++tn << 500 << true << -1   << true;
    QTest::newRow("wait_longer_succeeds")  << ++tn << 500 << true << 1000 << true;
    QTest::newRow("wait_zero_fails")       << ++tn << 500 << false << 0    << false;
    QTest::newRow("wait_not_enough_fails") << ++tn << 500 << false << 100  << false;
}

void tst_QLockFile::waitForLock()
{
    QFETCH(int, testNumber);
    QFETCH(int, threadSleepMs);
    QFETCH(bool, releaseEarly);
    QFETCH(int, tryLockTimeout);
    QFETCH(bool, expectedResult);

    const QString fileName = dir.path() + "/waitForLock" + QString::number(testNumber);
    QLockFile lockFile(fileName);
    QSemaphore semThreadReady, semMainThreadDone;
    // Lock file from a thread
    QFuture<bool> ret = QtConcurrent::run<bool>(lockFromThread, fileName, threadSleepMs, &semThreadReady, &semMainThreadDone);
    semThreadReady.acquire();

    if (releaseEarly) // let the thread release the lock after threadSleepMs
        semMainThreadDone.release();

    QCOMPARE(lockFile.tryLock(tryLockTimeout), expectedResult);
    if (expectedResult)
        QCOMPARE(int(lockFile.error()), int(QLockFile::NoError));
    else
        QCOMPARE(int(lockFile.error()), int(QLockFile::LockFailedError));

    if (!releaseEarly) // only let the thread release the lock now
        semMainThreadDone.release();

    QVERIFY(ret); // waits for the thread to finish
}

void tst_QLockFile::staleLockFromCrashedProcess_data()
{
    QTest::addColumn<int>("staleLockTime");

    // Test both use cases for QLockFile, should make no difference here.
    QTest::newRow("short") << 30000;
    QTest::newRow("long") << 0;
}

void tst_QLockFile::staleLockFromCrashedProcess()
{
#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    QFETCH(int, staleLockTime);
    const QString fileName = dir.path() + "/staleLockFromCrashedProcess";

    int ret = QProcess::execute(m_helperApp, QStringList() << fileName << "-uncleanexit");
    QCOMPARE(ret, int(QLockFile::NoError));
    QTRY_VERIFY(QFile::exists(fileName));

    QLockFile secondLock(fileName);
    secondLock.setStaleLockTime(staleLockTime);
    // tryLock detects and removes the stale lock (since the PID is dead)
#ifdef Q_OS_WIN
    // It can take a bit of time on Windows, though.
    QVERIFY(secondLock.tryLock(30000));
#else
    QVERIFY(secondLock.tryLock());
#endif
    QCOMPARE(int(secondLock.error()), int(QLockFile::NoError));
#endif // QT_CONFIG(process)
}

void tst_QLockFile::staleLockFromCrashedProcessReusedPid()
{
#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#elif defined(Q_OS_WINRT) || defined(QT_PLATFORM_UIKIT)
    QSKIP("We cannot retrieve information about other processes on this platform.");
#else
    const QString fileName = dir.path() + "/staleLockFromCrashedProcessReusedPid";

    int ret = QProcess::execute(m_helperApp, QStringList() << fileName << "-uncleanexit");
    QCOMPARE(ret, int(QLockFile::NoError));
    QVERIFY(QFile::exists(fileName));
    QVERIFY(overwritePidInLockFile(fileName, QCoreApplication::applicationPid()));

    QLockFile secondLock(fileName);
    qint64 pid = 0;
    QVERIFY(secondLock.getLockInfo(&pid, 0, 0));
    QCOMPARE(pid, QCoreApplication::applicationPid());
    secondLock.setStaleLockTime(0);
    QVERIFY(secondLock.tryLock());
    QCOMPARE(int(secondLock.error()), int(QLockFile::NoError));
#endif // QT_CONFIG(process)
}

void tst_QLockFile::staleShortLockFromBusyProcess()
{
#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    const QString fileName = dir.path() + "/staleLockFromBusyProcess";

    QProcess proc;
    proc.start(m_helperApp, QStringList() << fileName << "-busy");
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QTRY_VERIFY(QFile::exists(fileName));

    QLockFile secondLock(fileName);
    QVERIFY(!secondLock.tryLock()); // held by other process
    QCOMPARE(int(secondLock.error()), int(QLockFile::LockFailedError));
    qint64 pid;
    QString hostname, appname;
    QTRY_VERIFY(secondLock.getLockInfo(&pid, &hostname, &appname));
#ifdef Q_OS_UNIX
    QCOMPARE(pid, proc.processId());
#endif

    secondLock.setStaleLockTime(100);
    QTest::qSleep(100); // make the lock stale
    // We can't "steal" (delete+recreate) a lock file from a running process
    // until the file descriptor is closed.
    QVERIFY(!secondLock.tryLock());

    proc.waitForFinished();
    QVERIFY(secondLock.tryLock());
#endif // QT_CONFIG(process)
}

void tst_QLockFile::staleLongLockFromBusyProcess()
{
#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    const QString fileName = dir.path() + "/staleLockFromBusyProcess";

    QProcess proc;
    proc.start(m_helperApp, QStringList() << fileName << "-busy");
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QTRY_VERIFY(QFile::exists(fileName));

    QLockFile secondLock(fileName);
    secondLock.setStaleLockTime(0);
    QVERIFY(!secondLock.tryLock(100)); // never stale
    QCOMPARE(int(secondLock.error()), int(QLockFile::LockFailedError));
    qint64 pid;
    QTRY_VERIFY(secondLock.getLockInfo(&pid, NULL, NULL));
    QVERIFY(pid > 0);

    // As long as the other process is running, we can't remove the lock file
    QVERIFY(!secondLock.removeStaleLockFile());

    proc.waitForFinished();
#endif // QT_CONFIG(process)
}

static QString tryStaleLockFromThread(const QString &fileName)
{
    QLockFile lockInThread(fileName + ".lock");
    lockInThread.setStaleLockTime(1000);
    if (!lockInThread.lock())
        return "Error locking: " + QString::number(lockInThread.error());

    // The concurrent use of the file below (write, read, delete) is protected by the lock file above.
    // (provided that it doesn't become stale due to this operation taking too long)
    QFile theFile(fileName);
    if (!theFile.open(QIODevice::WriteOnly))
        return "Couldn't open for write";
    theFile.write("Hello world");
    theFile.flush();
    theFile.close();
    QFile reader(fileName);
    if (!reader.open(QIODevice::ReadOnly))
        return "Couldn't open for read";
    const QByteArray read = reader.readAll();
    if (read != "Hello world")
        return "File didn't have the expected contents:" + read;
    reader.remove();
    return QString();
}

void tst_QLockFile::staleLockRace()
{
#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    // Multiple threads notice a stale lock at the same time
    // Only one thread should delete it, otherwise a race will ensue
    const QString fileName = dir.path() + "/sharedFile";
    const QString lockName = fileName + ".lock";
    int ret = QProcess::execute(m_helperApp, QStringList() << lockName << "-uncleanexit");
    QCOMPARE(ret, int(QLockFile::NoError));
    QTRY_VERIFY(QFile::exists(lockName));

    QThreadPool::globalInstance()->setMaxThreadCount(10);
    QFutureSynchronizer<QString> synchronizer;
    for (int i = 0; i < 8; ++i)
        synchronizer.addFuture(QtConcurrent::run<QString>(tryStaleLockFromThread, fileName));
    synchronizer.waitForFinished();
    foreach (const QFuture<QString> &future, synchronizer.futures())
        QVERIFY2(future.result().isEmpty(), qPrintable(future.result()));
#endif // QT_CONFIG(process)
}

void tst_QLockFile::noPermissions()
{
#if defined(Q_OS_WIN)
    // A readonly directory still allows us to create files, on Windows.
    QSKIP("No permission testing on Windows");
#elif defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
    if (::geteuid() == 0)
        QSKIP("Test is not applicable with root privileges");
#endif
    // Restore permissions so that the QTemporaryDir cleanup can happen
    class PermissionRestorer
    {
        QString m_path;
    public:
        PermissionRestorer(const QString& path)
            : m_path(path)
        {}

        ~PermissionRestorer()
        {
            QFile file(m_path);
            file.setPermissions(QFile::Permissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        }
    };

    const QString fileName = dir.path() + "/staleLock";
    QFile dirAsFile(dir.path()); // I have to use QFile to change a dir's permissions...
    QVERIFY2(dirAsFile.setPermissions(QFile::Permissions{}), qPrintable(dir.path())); // no permissions
    PermissionRestorer permissionRestorer(dir.path());

    QLockFile lockFile(fileName);
    QVERIFY(!lockFile.lock());
    QCOMPARE(int(lockFile.error()), int(QLockFile::PermissionError));
}

enum ProcessProperty {
    ElevatedProcess = 0x1,
    VirtualStore = 0x2
};

Q_DECLARE_FLAGS(ProcessProperties, ProcessProperty)
Q_DECLARE_OPERATORS_FOR_FLAGS(ProcessProperties)

static inline ProcessProperties processProperties()
{
    ProcessProperties result;
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    HANDLE processToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken)) {
        DWORD elevation; // struct containing a DWORD, not present in some MinGW headers.
        DWORD cbSize = sizeof(elevation);
        if (GetTokenInformation(processToken, TokenElevation, &elevation, cbSize, &cbSize)
            && elevation) {
            result |= ElevatedProcess;
        }
        // Check for UAC virtualization (compatibility mode for old software
        // allowing it to write to system folders by mirroring them under
        // "\Users\...\AppData\Local\VirtualStore\", which is typically the case
        // for MinGW).
        DWORD virtualStoreEnabled = 0;
        cbSize = sizeof(virtualStoreEnabled);
        if (GetTokenInformation(processToken, TokenVirtualizationEnabled, &virtualStoreEnabled, cbSize, &cbSize)
            && virtualStoreEnabled) {
            result |= VirtualStore;
        }
        CloseHandle(processToken);
    }
#endif
    return result;
}

void tst_QLockFile::noPermissionsWindows()
{
    // Windows: Do the permissions test in a system directory in which
    // files cannot be created.
#if !defined(Q_OS_WIN) || defined(Q_OS_WINRT)
    QSKIP("This test is for desktop Windows only");
#endif
#ifdef Q_OS_WIN
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows7)
        QSKIP("This test requires at least Windows 7");
#endif
    if (const int p = processProperties()) {
        const QByteArray message = "This test cannot be run (properties=0x"
            + QByteArray::number(p, 16) + ')';
        QSKIP(message.constData());
    }

    const QString fileName = QFile::decodeName(qgetenv("ProgramFiles"))
        + QLatin1Char('/') + QCoreApplication::applicationName()
        + QDateTime::currentDateTime().toString(QStringLiteral("yyMMddhhmm"));
    QLockFile lockFile(fileName);
    QVERIFY(!lockFile.lock());
    QCOMPARE(int(lockFile.error()), int(QLockFile::PermissionError));
}

void tst_QLockFile::corruptedLockFile()
{
    const QString fileName = dir.path() + "/corruptedLockFile";

    {
        // Create a empty file. Typically the result of a computer crash or hard disk full.
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly));
    }

    QLockFile secondLock(fileName);
    secondLock.setStaleLockTime(100);
    QVERIFY(secondLock.tryLock(10000));
    QCOMPARE(int(secondLock.error()), int(QLockFile::NoError));
}

void tst_QLockFile::corruptedLockFileInTheFuture()
{
#if !defined(Q_OS_UNIX)
    QSKIP("This tests needs utimes");
#else
    // This test is the same as the previous one, but the corruption was so there is a corrupted
    // .rmlock whose timestamp is in the future

    const QString fileName = dir.path() + "/corruptedLockFile.rmlock";

    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly));
    }

    struct timeval times[2];
    gettimeofday(times, 0);
    times[1].tv_sec = (times[0].tv_sec += 600);
    times[1].tv_usec = times[0].tv_usec;
    utimes(fileName.toLocal8Bit(), times);

    QTest::ignoreMessage(QtInfoMsg, "QLockFile: Lock file '" + fileName.toUtf8() + "' has a modification time in the future");
    corruptedLockFile();
#endif
}

void tst_QLockFile::hostnameChange()
{
    const QByteArray hostid = QSysInfo::machineUniqueId();
    if (hostid.isEmpty())
        QSKIP("Could not get a unique host ID on this machine");

    QString lockFile = dir.path() + "/hostnameChangeLock";
    QLockFile lock1(lockFile);
    QVERIFY(lock1.lock());

    {
        // now modify it
        QFile f;
        QVERIFY(f.open(QLockFilePrivate::getLockFileHandle(&lock1),
                       QIODevice::ReadWrite | QIODevice::Text,
                       QFile::DontCloseHandle));
        QVERIFY(overwriteLineInLockFile(f, 3, "this is not a hostname"));
    }

    {
        // we should fail to lock
        QLockFile lock2(lockFile);
        QVERIFY(!lock2.tryLock(1000));
    }
}

void tst_QLockFile::differentMachines()
{
    const QByteArray hostid = QSysInfo::machineUniqueId();
    if (hostid.isEmpty())
        QSKIP("Could not get a unique host ID on this machine");

    QString lockFile = dir.path() + "/differentMachinesLock";
    QLockFile lock1(lockFile);
    QVERIFY(lock1.lock());

    {
        // now modify it
        QFile f;
        QVERIFY(f.open(QLockFilePrivate::getLockFileHandle(&lock1),
                       QIODevice::ReadWrite | QIODevice::Text,
                       QFile::DontCloseHandle));
        QVERIFY(overwriteLineInLockFile(f, 1, QT_STRINGIFY(INT_MAX)));
        QVERIFY(overwriteLineInLockFile(f, 4, "this is not a UUID"));
    }

    {
        // we should fail to lock
        QLockFile lock2(lockFile);
        QVERIFY(!lock2.tryLock(1000));
    }
}

void tst_QLockFile::reboot()
{
    const QByteArray bootid = QSysInfo::bootUniqueId();
    if (bootid.isEmpty())
        QSKIP("Could not get a unique boot ID on this machine");

    // create a lock so we can get its contents
    QString lockFile = dir.path() + "/rebootLock";
    QLockFile lock1(lockFile);
    QVERIFY(lock1.lock());

    QFile f(lockFile);
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));
    auto lines = f.readAll().split('\n');
    f.close();

    lock1.unlock();

    // now recreate the file simulating a reboot
    QVERIFY(f.open(QFile::WriteOnly | QFile::Text));
    lines[4] = "this is not a UUID";
    f.write(lines.join('\n'));
    f.close();

    // we should succeed in locking
    QVERIFY(lock1.tryLock(0));
}

bool tst_QLockFile::overwritePidInLockFile(const QString &filePath, qint64 pid)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadWrite | QFile::Text)) {
        qErrnoWarning("Cannot open %s", qPrintable(filePath));
        return false;
    }
    return overwriteLineInLockFile(f, 1, QString::number(pid));
}

bool tst_QLockFile::overwriteLineInLockFile(QFile &f, int line, const QString &newLine)
{
    f.seek(0);
    QByteArray buf = f.readAll();
    QStringList lines = QString::fromUtf8(buf).split('\n');
    if (lines.size() < 3 && lines.size() < line - 1) {
        qWarning("Unexpected lockfile content.");
        return false;
    }
    lines[line - 1] = newLine;
    f.seek(0);
    buf = lines.join('\n').toUtf8();
    f.resize(buf.size());
    return f.write(buf) == buf.size();
}

struct LockFileUsageInGlobalDtor
{
    ~LockFileUsageInGlobalDtor() {
        QLockFile lockFile(QDir::currentPath() + "/lastlock");
        QVERIFY(lockFile.lock());
        QVERIFY(lockFile.isLocked());
    }
};
LockFileUsageInGlobalDtor s_instance;

QTEST_MAIN(tst_QLockFile)
#include "tst_qlockfile.moc"
